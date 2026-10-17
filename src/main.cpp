// Crayons Color & Paint Studio — native Windows remake.
//
// Command line (testing and setup):
//   --import <source> [--out <dir>]   import the player's CD image / CRAYONS.Z / installation
//   --data <dir>                      use this data folder instead of %APPDATA%\Crayons\data
//   --level <1|2>                     skip the age picker
//   --snapshot <bmp>                  run headlessly with a simulated clock, write the screen, exit
//   --time <ms>                       with --snapshot: simulated time before the frame is taken
//   --click <x,y,ms>                  with --snapshot: left click at 640x480 coordinates; repeatable
//   --drag <x1,y1,x2,y2,ms>           with --snapshot: press, move over 400 ms, release; repeatable
//   --type <text,ms>                  with --snapshot: type characters (\r = Enter); repeatable
//   --rclick <x,y,ms>, --move <x,y,ms> with --snapshot: right click, mouse move; repeatable
//   --cursor                          with --snapshot: draw the mouse cursor at the last position
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <winspool.h>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "app.h"
#include "assets.h"
#include "gamefiles.h"

namespace {

std::string narrow(const std::wstring& s) {
    std::string out(size_t(WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr)), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), int(out.size()), nullptr, nullptr);
    out.pop_back();
    return out;
}

int runImport(const std::wstring& source, std::wstring target) {
    if (target.empty()) target = gamefiles::dataDirectory();
    std::wstring error;
    int lastPercent = -1;
    const bool ok = gamefiles::import(source, target, [&](size_t done, size_t total) {
        const int percent = int(done * 100 / (total ? total : 1));
        if (percent / 10 != lastPercent / 10) std::printf("import: %d%%\n", percent), std::fflush(stdout);
        lastPercent = percent;
    }, &error);
    std::printf(ok ? "import: ok -> %s\n" : "import: FAILED: %s\n", narrow(ok ? target : error).c_str());
    return ok ? 0 : 1;
}

// Asks for the player's CD (image, drive or CRAYONS.Z) and imports it. "" when cancelled.
std::wstring askForGameData(const std::wstring& target) {
    for (;;) {
        if (MessageBoxW(nullptr,
                        L"Crayons needs the files from your original Crayons Color & Paint Studio CD.\n\n"
                        L"Choose the CD image (.iso, .bin or .cue), or CRAYONS.Z on the CD. The files are "
                        L"copied once, so the CD is not needed afterwards.",
                        L"Crayons Color & Paint Studio", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
            return L"";
        wchar_t path[MAX_PATH] = L"";
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof ofn;
        ofn.lpstrFilter = L"Crayons CD (*.iso;*.bin;*.cue;CRAYONS.Z)\0*.iso;*.bin;*.cue;CRAYONS.Z\0All files\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        if (!GetOpenFileNameW(&ofn)) return L"";
        std::wstring error;
        HCURSOR old = SetCursor(LoadCursor(nullptr, IDC_WAIT));
        const bool ok = gamefiles::import(path, target, nullptr, &error);
        SetCursor(old);
        if (ok) return target;
        MessageBoxW(nullptr, error.c_str(), L"Crayons Color & Paint Studio", MB_ICONWARNING);
    }
}

struct ScriptEvent {
    long at;
    enum { Click, RightClick, Move, Drag, Type } kind;
    int x1, y1, x2, y2;
    std::wstring text;
};

class WindowHost : public Host {
public:
    explicit WindowHost(Assets& assets, bool silent) : assets_(assets), silent_(silent) {
        if (!silent_) musicThread_ = std::thread([this] { musicLoop(); });
    }
    ~WindowHost() override {
        {
            std::lock_guard lock(musicMutex_);
            musicStopping_ = true;
            musicQueue_.clear();
        }
        musicCv_.notify_one();
        if (musicThread_.joinable()) musicThread_.join();
    }
    void playSound(const std::string& path) override {
        if (silent_) return;
        sound_ = assets_.files().read(path);  // kept alive while it plays
        if (!sound_.empty()) PlaySoundW(reinterpret_cast<LPCWSTR>(sound_.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    }
    int messageBox(const std::wstring& text, const std::wstring& caption, UINT flags) override {
        if (silent_) return (flags & 0xF) == MB_YESNO || (flags & 0xF) == MB_YESNOCANCEL ? IDYES : IDOK;
        return MessageBoxW(window, text.c_str(), caption.c_str(), flags);
    }
    uint32_t randomSeed() override { return silent_ ? 12345u : GetTickCount(); }
    void showFrame(int delayMs) override {
        if (silent_ || !window) return;
        InvalidateRect(window, nullptr, FALSE);
        UpdateWindow(window);
        if (delayMs > 0) Sleep(DWORD(delayMs));
    }
    void playMusic(const std::string& path) override {
        if (silent_ || !window) return;
        queueMusic({assets_.files().fullPath(path), window});
    }
    void stopMusic() override {
        if (!silent_) queueMusic({});
    }
    bool print(const Surface& picture) override {
        if (silent_) return true;
        wchar_t name[512];
        DWORD size = 512;
        if (!GetDefaultPrinterW(name, &size)) return false;
        HDC dc = CreateDCW(L"WINSPOOL", name, nullptr, nullptr);
        if (!dc) return false;
        // PRNDLL.HARDCOPY: the page width, and 0.6 of the height on a portrait page.
        const int w = GetDeviceCaps(dc, HORZRES);
        int h = GetDeviceCaps(dc, VERTRES);
        if (h > w) h = h / 2 + (h / 2) / 5;
        DOCINFOW doc = {sizeof doc, L"CRAYONS", nullptr, nullptr, 0};
        if (StartDocW(dc, &doc) > 0) {
            StartPage(dc);
            BITMAPINFO info = {};
            info.bmiHeader = {sizeof(BITMAPINFOHEADER), picture.width(), -picture.height(), 1, 32, BI_RGB, 0, 0, 0, 0, 0};
            GdiFlush();
            StretchDIBits(dc, 0, 0, w, h, 0, 0, picture.width(), picture.height(), picture.pixels(), &info,
                          DIB_RGB_COLORS, SRCCOPY);
            EndPage(dc);
            EndDoc(dc);
        }
        DeleteDC(dc);
        return true;
    }
    void openDocument(const std::wstring& path) override {
        if (silent_) return;
        ShellExecuteW(window, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    void quit() override {
        quitRequested = true;
        if (window) DestroyWindow(window);
    }
    HWND window = nullptr;
    bool quitRequested = false;

private:
    struct MusicCommand {
        std::wstring path;
        HWND callback = nullptr;
    };

    void queueMusic(MusicCommand command) {
        {
            std::lock_guard lock(musicMutex_);
            musicQueue_.push_back(std::move(command));
        }
        musicCv_.notify_one();
    }

    void musicLoop() {
        bool open = false;
        for (;;) {
            std::unique_lock lock(musicMutex_);
            musicCv_.wait(lock, [this] { return musicStopping_ || !musicQueue_.empty(); });
            if (musicStopping_) break;
            MusicCommand command = std::move(musicQueue_.front());
            musicQueue_.pop_front();
            lock.unlock();
            if (open) {
                mciSendStringW(L"close midi", nullptr, 0, nullptr);
                open = false;
            }
            if (command.path.empty()) continue;
            const std::wstring request = L"open \"" + command.path + L"\" type sequencer alias midi";
            if (mciSendStringW(request.c_str(), nullptr, 0, command.callback)) continue;
            open = true;
            if (mciSendStringW(L"play midi from 0 notify", nullptr, 0, command.callback)) {
                mciSendStringW(L"close midi", nullptr, 0, nullptr);
                open = false;
            }
        }
        if (open) mciSendStringW(L"close midi", nullptr, 0, nullptr);
    }

    Assets& assets_;
    bool silent_;
    std::vector<uint8_t> sound_;
    std::mutex musicMutex_;
    std::condition_variable musicCv_;
    std::deque<MusicCommand> musicQueue_;
    bool musicStopping_ = false;
    std::thread musicThread_;
};

struct Program {
    Assets assets;
    std::unique_ptr<WindowHost> host;
    std::unique_ptr<App> app;
    Surface screen;
    DWORD started = 0;
    RECT game{};  // where the 640x480 picture goes in the client area
    bool showCursor = false;  // snapshots: --cursor
    POINT mouse{};
    std::map<std::pair<HCURSOR, int>, HCURSOR> scaledCursors;
} program;

long elapsed() { return long(GetTickCount() - program.started); }

void layout(HWND hwnd) {
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
    for (const auto& [key, cursor] : program.scaledCursors) DestroyCursor(cursor);
    program.scaledCursors.clear();
    RECT client;
    GetClientRect(hwnd, &client);
    const int w = client.right, h = client.bottom;
    double scale = std::min(double(w) / kScreenWidth, double(h) / kScreenHeight);
    if (scale >= 1 && std::floor(scale) / scale >= 0.85) scale = std::floor(scale);  // crisp integer scale when close
    const int gw = int(kScreenWidth * scale), gh = int(kScreenHeight * scale);
    program.game = {(w - gw) / 2, (h - gh) / 2, (w - gw) / 2 + gw, (h - gh) / 2 + gh};
}

HCURSOR windowCursor(HCURSOR source) {
    const int gameWidth = program.game.right - program.game.left;
    if (!source || gameWidth <= 0 || gameWidth == kScreenWidth) return source;
    const auto key = std::make_pair(source, gameWidth);
    if (auto found = program.scaledCursors.find(key); found != program.scaledCursors.end()) return found->second;
    ICONINFO info{};
    if (!GetIconInfo(source, &info)) return source;
    BITMAP bitmap{};
    const HBITMAP image = info.hbmColor ? info.hbmColor : info.hbmMask;
    const int measured = image ? GetObjectW(image, sizeof bitmap, &bitmap) : 0;
    const int width = measured ? bitmap.bmWidth : 0;
    const int height = measured ? bitmap.bmHeight / (info.hbmColor ? 1 : 2) : 0;
    if (info.hbmMask) DeleteObject(info.hbmMask);
    if (info.hbmColor) DeleteObject(info.hbmColor);
    if (width <= 0 || height <= 0) return source;
    const int scaledWidth = std::max(1, int(std::lround(double(width) * gameWidth / kScreenWidth)));
    const int scaledHeight = std::max(1, int(std::lround(double(height) * gameWidth / kScreenWidth)));
    HCURSOR scaled = static_cast<HCURSOR>(CopyImage(source, IMAGE_CURSOR, scaledWidth, scaledHeight, 0));
    if (!scaled) return source;
    program.scaledCursors.emplace(key, scaled);
    return scaled;
}

POINT toGame(LPARAM lp) {
    const RECT& g = program.game;
    const int x = short(LOWORD(lp)), y = short(HIWORD(lp));
    const int gw = std::max(1L, g.right - g.left), gh = std::max(1L, g.bottom - g.top);
    return {std::clamp(int((x - g.left) * kScreenWidth / gw), 0, kScreenWidth - 1),
            std::clamp(int((y - g.top) * kScreenHeight / gh), 0, kScreenHeight - 1)};
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    App* app = program.app.get();
    switch (msg) {
    case WM_SIZE: layout(hwnd); InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_DPICHANGED: {
        const RECT& bounds = *reinterpret_cast<const RECT*>(lp);
        SetWindowPos(hwnd, nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_TIMER:
        app->tick(elapsed());
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        app->compose(program.screen);
        RECT client;
        GetClientRect(hwnd, &client);
        const RECT& g = program.game;
        HBRUSH black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        RECT bands[4] = {{0, 0, client.right, g.top}, {0, g.bottom, client.right, client.bottom},
                         {0, g.top, g.left, g.bottom}, {g.right, g.top, client.right, g.bottom}};
        for (const RECT& r : bands) FillRect(dc, &r, black);
        SetStretchBltMode(dc, COLORONCOLOR);
        StretchBlt(dc, g.left, g.top, g.right - g.left, g.bottom - g.top, program.screen.dc(), 0, 0, kScreenWidth,
                   kScreenHeight, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: {
        SetCapture(hwnd);
        const POINT p = toGame(lp);
        app->mouseDown(p.x, p.y, msg == WM_LBUTTONDOWN ? 0 : 1);
        return 0;
    }
    case WM_LBUTTONUP: case WM_RBUTTONUP: {
        ReleaseCapture();
        const POINT p = toGame(lp);
        app->mouseUp(p.x, p.y, msg == WM_LBUTTONUP ? 0 : 1);
        return 0;
    }
    case WM_MOUSEMOVE: {
        const POINT p = toGame(lp);
        app->mouseMove(p.x, p.y, (wp & MK_LBUTTON ? 1 : 0) | (wp & MK_RBUTTON ? 2 : 0));
        SetCursor(windowCursor(app->cursor(p.x, p.y)));  // Screen.Cursor := … takes effect at once
        return 0;
    }
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(hwnd, &p);
            const POINT g = toGame(MAKELPARAM(p.x, p.y));
            SetCursor(windowCursor(app->cursor(g.x, g.y)));
            return TRUE;
        }
        break;
    case MM_MCINOTIFY:
        if (wp == MCI_NOTIFY_SUCCESSFUL) app->musicNotify();
        return 0;
    case WM_KEYDOWN: app->keyDown(int(wp)); return 0;
    case WM_CHAR: app->keyChar(wchar_t(wp)); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int runWindow(int level) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = static_cast<HICON>(ne::createIcon(program.assets.exe(0), "MAINICON", false, 32));
    wc.lpszClassName = L"CrayonsRemake";
    RegisterClassW(&wc);
    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int scale = std::max(1, int(std::min((work.right - work.left) * 0.9 / kScreenWidth, (work.bottom - work.top) * 0.9 / kScreenHeight)));
    RECT r{0, 0, kScreenWidth * scale, kScreenHeight * scale};
    const DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&r, style, FALSE);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Crayons Color & Paint Studio", style, CW_USEDEFAULT, CW_USEDEFAULT,
                                r.right - r.left, r.bottom - r.top, nullptr, nullptr, instance, nullptr);
    program.host->window = hwnd;
    program.started = GetTickCount();
    if (level) program.app->startLevel(level, 0);
    else program.app->startLauncher(0);
    ShowWindow(hwnd, SW_SHOW);
    SetTimer(hwnd, 1, 10, nullptr);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

int runSnapshot(const std::wstring& path, long duration, std::vector<ScriptEvent> script, int level) {
    App& app = *program.app;
    if (level) app.startLevel(level, 0);
    else app.startLauncher(0);
    std::sort(script.begin(), script.end(), [](const ScriptEvent& a, const ScriptEvent& b) { return a.at < b.at; });
    size_t next = 0;
    struct Drag { ScriptEvent e; } ;
    std::vector<ScriptEvent> dragging;
    for (long now = 0; now <= duration && !program.host->quitRequested; now += 10) {
        while (next < script.size() && script[next].at <= now) {
            const ScriptEvent& e = script[next++];
            if (e.kind == ScriptEvent::Type) {
                for (wchar_t c : e.text) app.keyChar(c);
            } else if (e.kind == ScriptEvent::Click || e.kind == ScriptEvent::RightClick) {
                const int button = e.kind == ScriptEvent::Click ? 0 : 1;
                app.mouseMove(e.x1, e.y1, 0);
                app.mouseDown(e.x1, e.y1, button);
                app.mouseUp(e.x1, e.y1, button);
                program.mouse = {e.x1, e.y1};
            } else if (e.kind == ScriptEvent::Move) {
                app.mouseMove(e.x1, e.y1, 0);
                program.mouse = {e.x1, e.y1};
            } else {
                app.mouseMove(e.x1, e.y1, 0);
                app.mouseDown(e.x1, e.y1, 0);
                dragging.push_back(e);
            }
        }
        for (auto it = dragging.begin(); it != dragging.end();) {
            const double t = std::min(1.0, double(now - it->at) / 400.0);
            const int x = int(it->x1 + (it->x2 - it->x1) * t), y = int(it->y1 + (it->y2 - it->y1) * t);
            app.mouseMove(x, y, 1);
            program.mouse = {x, y};
            if (t >= 1.0) {
                app.mouseUp(x, y, 0);
                it = dragging.erase(it);
            } else {
                ++it;
            }
        }
        app.tick(now);
    }
    app.compose(program.screen);
    if (program.showCursor) {  // --cursor: draw the cursor at the last scripted position
        HCURSOR cursor = app.cursor(program.mouse.x, program.mouse.y);
        ICONINFO info{};
        if (cursor && GetIconInfo(cursor, &info)) {
            DrawIconEx(program.screen.dc(), program.mouse.x - int(info.xHotspot), program.mouse.y - int(info.yHotspot), cursor,
                       0, 0, 0, nullptr, DI_NORMAL);
            if (info.hbmMask) DeleteObject(info.hbmMask);
            if (info.hbmColor) DeleteObject(info.hbmColor);
        }
    }
    return program.screen.writeBmp(path.c_str()) ? 0 : 1;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args(argv + 1, argv + argc);
    LocalFree(argv);
    std::wstring importSource, importTarget, dataDir, snapshot;
    long duration = 0;
    int level = 0;
    std::vector<ScriptEvent> script;
    for (size_t i = 0; i < args.size(); ++i) {
        const bool more = i + 1 < args.size();
        const std::wstring& key = args[i];
        const std::wstring value = more ? args[i + 1] : L"";
        if (key == L"--import" && more) importSource = value, ++i;
        else if (key == L"--out" && more) importTarget = value, ++i;
        else if (key == L"--data" && more) dataDir = value, ++i;
        else if (key == L"--level" && more) level = _wtoi(value.c_str()), ++i;
        else if (key == L"--snapshot" && more) snapshot = value, ++i;
        else if (key == L"--time" && more) duration = _wtol(value.c_str()), ++i;
        else if (key == L"--cursor") program.showCursor = true;
        else if ((key == L"--click" || key == L"--rclick" || key == L"--move") && more) {
            ScriptEvent e{0, key == L"--click" ? ScriptEvent::Click : key == L"--rclick" ? ScriptEvent::RightClick : ScriptEvent::Move,
                          0, 0, 0, 0, L""};
            if (swscanf(value.c_str(), L"%d,%d,%ld", &e.x1, &e.y1, &e.at) == 3) script.push_back(e);
            ++i;
        } else if (key == L"--type" && more) {
            const size_t comma = value.rfind(L',');
            if (comma != std::wstring::npos) {
                ScriptEvent e{_wtol(value.c_str() + comma + 1), ScriptEvent::Type, 0, 0, 0, 0, L""};
                for (size_t k = 0; k < comma; ++k) {
                    if (value[k] == L'\\' && k + 1 < comma && value[k + 1] == L'r') e.text += L'\r', ++k;
                    else if (value[k] == L'\\' && k + 1 < comma && value[k + 1] == L'b') e.text += L'\b', ++k;
                    else e.text += value[k];
                }
                script.push_back(e);
            }
            ++i;
        } else if (key == L"--drag" && more) {
            ScriptEvent e{0, ScriptEvent::Drag, 0, 0, 0, 0, L""};
            if (swscanf(value.c_str(), L"%d,%d,%d,%d,%ld", &e.x1, &e.y1, &e.x2, &e.y2, &e.at) == 5) script.push_back(e);
            ++i;
        }
    }
    if (!importSource.empty()) return runImport(importSource, importTarget);
    if (dataDir.empty()) {
        dataDir = gamefiles::portableDataDirectory();
        if (!gamefiles::isComplete(dataDir)) dataDir = gamefiles::dataDirectory();
        if (!gamefiles::isComplete(dataDir)) {
            if (!snapshot.empty()) return std::fprintf(stderr, "no game data (use --data)\n"), 2;
            if (askForGameData(dataDir).empty()) return 0;
        }
    }
    std::wstring error;
    if (!program.assets.open(dataDir, &error)) {
        if (snapshot.empty()) MessageBoxW(nullptr, error.c_str(), L"Crayons Color & Paint Studio", MB_ICONERROR);
        else std::fprintf(stderr, "%s\n", narrow(error).c_str());
        return 1;
    }
    program.screen.create(kScreenWidth, kScreenHeight);
    program.host = std::make_unique<WindowHost>(program.assets, !snapshot.empty());
    program.app = std::make_unique<App>(program.assets, *program.host);
    if (!snapshot.empty()) return runSnapshot(snapshot, duration, std::move(script), level);
    return runWindow(level);
}
