// The remake's core: a 640x480 logical screen made of the original's windows (a form and its
// child controls), each with its own surface, composited in z-order like Windows did. The
// platform layer (main.cpp) feeds input/time and presents the result scaled to the window.
#pragma once

#include <windows.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "assets.h"
#include "gfx.h"

constexpr int kScreenWidth = 640, kScreenHeight = 480;

// A window of the original program: a form or child control with its own client surface.
struct View {
    RECT rect{};             // client area on the logical screen
    int border = 0;          // WS_BORDER-style frame drawn around the client (pixels)
    COLORREF borderColour = RGB(0, 0, 0);
    bool visible = true;
    Surface surface;
    bool contains(int x, int y) const { return visible && x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom; }
    void place(int x, int y, int w, int h, int frame = 0, COLORREF fill = RGB(0, 0, 0));
};

// Services the platform provides (sounds, message boxes, quitting).
class Host {
public:
    virtual ~Host() = default;
    virtual void playSound(const std::string& path) = 0;  // a WAV data file, asynchronous
    virtual int messageBox(const std::wstring& text, const std::wstring& caption, UINT flags) = 0;
    virtual void quit() = 0;
    virtual uint32_t randomSeed() = 0;  // Randomize; fixed in test runs so snapshots repeat
    // Shows the screen now and waits `delayMs` — for the original's synchronous animations
    // (busy-waits on GetTickCount, or drawing loops that were slow on 1990s hardware).
    virtual void showFrame(int delayMs) = 0;
    // MCI music: "open <file> alias midi" + "play midi from 0 notify"; App::musicNotify follows
    // when it ends. stopMusic = "close midi".
    virtual void playMusic(const std::string& path) = 0;
    virtual void stopMusic() = 0;
    // Prints the picture on the default printer as PRNDLL.HARDCOPY did. False when there is none.
    virtual bool print(const Surface& picture) = 0;
    virtual void openDocument(const std::wstring& path) = 0;
};

class App;

class Scene {
public:
    explicit Scene(App& app) : app_(app) {}
    virtual ~Scene() = default;
    virtual void start(long now) { (void)now; }
    virtual void tick(long now) { (void)now; }
    virtual void mouseDown(int x, int y, int button) { (void)x, (void)y, (void)button; }
    virtual void mouseMove(int x, int y, int buttons) { (void)x, (void)y, (void)buttons; }
    virtual void mouseUp(int x, int y, int button) { (void)x, (void)y, (void)button; }
    virtual void keyDown(int key) { (void)key; }
    virtual void keyChar(wchar_t c) { (void)c; }
    virtual void musicNotify() {}
    virtual HCURSOR cursor(int x, int y) { (void)x, (void)y; return LoadCursor(nullptr, IDC_ARROW); }
    // Views bottom to top.
    virtual void views(std::vector<const View*>* out) const = 0;

protected:
    App& app_;
};

class App {
public:
    App(Assets& assets, Host& host) : assets_(assets), host_(host) {}
    Assets& assets() { return assets_; }
    Host& host() { return host_; }

    void startLauncher(long now);
    void startLevel(int level, long now);  // 1 = younger (CRAYON1), 2 = older (CRAYON2)
    void tick(long now);
    void mouseDown(int x, int y, int button) { if (scene_) scene_->mouseDown(x, y, button); }
    void mouseMove(int x, int y, int buttons) { if (scene_) scene_->mouseMove(x, y, buttons); }
    void mouseUp(int x, int y, int button) { if (scene_) scene_->mouseUp(x, y, button); }
    void keyDown(int key) { if (scene_) scene_->keyDown(key); }
    void keyChar(wchar_t c) { if (scene_) scene_->keyChar(c); }
    void musicNotify() { if (scene_) scene_->musicNotify(); }
    HCURSOR cursor(int x, int y) { return scene_ ? scene_->cursor(x, y) : LoadCursor(nullptr, IDC_ARROW); }
    // Composes the views into `screen` (kScreenWidth x kScreenHeight).
    void compose(Surface& screen) const;
    long now() const { return now_; }

private:
    Assets& assets_;
    Host& host_;
    std::unique_ptr<Scene> scene_, next_;
    long now_ = 0;
};
