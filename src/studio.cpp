#include "studio.h"

#include <algorithm>
#include <cstdio>

#include "activity.h"
#include "help.h"
#include "pcdialogs.h"
#include "textdialogs.h"
#include "thumbs.h"

namespace {

// 1028:0452: x1 <= x < x2 and y1 <= y < y2.
bool inRect(int x, int y, int x1, int x2, int y1, int y2) { return x >= x1 && x < x2 && y >= y1 && y < y2; }

// BitBlt that, like a Win16 memory DC, never reads outside the source bitmap.
void blit(HDC dest, int x, int y, int w, int h, const Surface& from, int sx = 0, int sy = 0) {
    w = std::min(w, from.width() - sx);
    h = std::min(h, from.height() - sy);
    if (w > 0 && h > 0) BitBlt(dest, x, y, w, h, from.dc(), sx, sy, SRCCOPY);
}

}  // namespace

COLORREF Studio::colour(int index) const {
    const std::vector<COLORREF>& c = app_.assets().colours();
    return index >= 0 && index < int(c.size()) ? c[size_t(index)] : RGB(0, 0, 0);
}

void Studio::notYet(const char* what) { std::fprintf(stderr, "studio: not ported yet: %s\n", what); }

int Studio::tool() const {  // DS:1ab8 as CRAYON1's tool number (docs/re/level2.md)
    if (level_ != 2) return tool_;
    static const int kLevel2[] = {0, 1, 2, 3, 13, 4, 14, 5, 6, 15, 7, 9, 8};
    return tool_ >= 1 && tool_ <= 12 ? kLevel2[tool_] : 0;
}

void Studio::start(long now) {
    form_.place(0, 0, kScreenWidth, kScreenHeight);  // borderless, maximised, Color = clBlack
    hint_.visible = false;
    current_ = colour(family_ * 10 + shade_);
    updateShadeIndexes();
    randSeed_ = app_.host().randomSeed();  // Randomize (FormCreate 1008:5813)
    // TabSet1 (form resource TMAIN): owner-drawn icon tabs. CRAYON1 has 9, CRAYON2 11
    // (TabSet1DrawTab 1008:9226 picks the icon by index; CRAYON2 2:b3c3).
    const dfm::Component* form = app_.assets().form(level_, "TMAIN");
    const dfm::Component* tabs = form ? form->find("TabSet1") : nullptr;
    if (tabs) {
        tabSet_.tabHeight = tabs->integer("TabHeight", 20);
        tabSet_.startMargin = tabs->integer("StartMargin", 5);
        tabSet_.endMargin = tabs->integer("EndMargin", 15);
        const auto it = tabs->props.find("Tabs.Strings");
        tabSet_.tabCount = it != tabs->props.end() ? int(it->second.list.size()) : 0;
    }
    tabSet_.backgroundColour = RGB(0, 0, 0);  // BackgroundColor = clBlack
    tabSet_.selectedColour = RGB(0, 0, 0);    // SelectedColor = clBlack
    tabSet_.unselectedColour = RGB(255, 255, 255);
    static const int kIcons1[] = {1, 2, 3, 5, 12, 7, 9, 10, 11};
    static const int kIcons2[] = {1, 2, 3, 4, 5, 6, 12, 7, 8, 9, 11};
    for (int i = 0; i < tabSet_.tabCount; ++i) {
        const int icon = level_ == 1 ? kIcons1[std::min(i, 8)] : kIcons2[std::min(i, 10)];
        tabIcons_.push_back(static_cast<HICON>(
            ne::createIcon(app_.assets().exe(level_), "ICON_" + std::to_string(icon), false, 32)));
    }
    tabSet_.measureTab = [](int) { return 0x20; };  // TabSet1MeasureTab 1008:91fe
    tabSet_.drawTab = [this](HDC canvas, const RECT&, int index, bool) {  // 1008:9226
        if (index > 10 || index >= int(tabIcons_.size()) || !tabIcons_[size_t(index)]) return;
        DrawIcon(canvas, index * 40 + (index < 7 ? 13 : 20), 0, tabIcons_[size_t(index)]);
    };
    // FormCreate: the option sheet of the first tool, animated by Timer1 every 125 ms.
    loadBmp(app_.assets().files().read(level_ == 2 ? "a900.asl" : "a950.asl"), &optionSheet_);
    timer1Due_ = now + timer1Interval_;
    formPaint();
}

void Studio::tick(long now) {
    if (dialog_) dialog_->hintTimer(now), dialog_->tick(now);
    if (timer2On_ && now >= timer2Due_) hintArmed_ = true, timer2On_ = false;  // Timer2Timer 1008:9972
    while (timer1On_ && now >= timer1Due_) {
        timer1Due_ += timer1Interval_;
        timer1();
    }
}

void Studio::timer1() {  // 1008:8e10: the option icons' idle animation, frames 0→5→0
    blit(formDC(), 100, 10, 400, 40, optionSheet_, 0, 41 * frame_);
    if (frameBack_ == 0) {
        if (++frame_ > 5) frame_ = 4, frameBack_ = 1;
    } else if (--frame_ < 0) {
        frame_ = 1, frameBack_ = 0;
    }
}

void Studio::formPaint() {  // 1008:6390
    const int clientWidth = form_.surface.width(), clientHeight = form_.surface.height();
    if (firstPaint_) {
        tabs_.place(82, 60, clientWidth - 82, 32);
        tabSet_.create(tabs_.surface.width(), tabs_.surface.height());
        paintTabs();
        // BiPict1: TPanel, BorderStyle = bsSingle (1-pixel frame), Color = clWhite.
        picture_.place(82, 92, clientWidth - 82, clientHeight - 94, 1, RGB(255, 255, 255));
        // The picture backup and undo bitmaps are BiPict1-sized (1008:6422).
        backup_.create(pictureWidth(), pictureHeight(), RGB(255, 255, 255));
        undo_.create(pictureWidth(), pictureHeight(), RGB(255, 255, 255));
        loadCursors();
        loadHints();  // 1008:6466 (crayons.ini's first line, the data path, is not needed)
        b1046 = true;
    }
    Assets& a = app_.assets();
    HDC dc = formDC();
    // Brush and pen colour DS:1676 (palette 69, black), then the two black bands.
    SelectObject(dc, GetStockObject(BLACK_BRUSH));
    SelectObject(dc, GetStockObject(BLACK_PEN));
    Rectangle(dc, 0, 0, clientWidth, 150);
    Rectangle(dc, 0, 0, 100, clientHeight);
    const Surface& strip = a.bitmap("a920.asl");  // left strip with the help "?"
    blit(dc, 0, 0, 80, strip.height(), strip);
    blit(dc, 0, 0, 81, 179, a.bitmap("a1.asl"));  // the juggler's colour palette
    const Surface& top = a.bitmap("a919.asl");
    blit(dc, 80, 0, top.width(), top.height(), top);
    const char* exitSign = pictureWidth() < 700 ? "a911a.asl" : pictureWidth() > 900 ? "a911c.asl" : "a911b.asl";
    blit(dc, pictureWidth() - 140 + 80, 0, 140, 60, a.bitmap(exitSign));
    restoreLeftStrip();
    drawShades();
    if (firstPaint_) {
        firstPaint_ = false;
        drawToolPointer(0);
        musicTrack_ = 0;
        startMusic();
    }
    drawPanels();
}

void Studio::restoreLeftStrip() {  // 1008:2d76
    const Surface& strip = app_.assets().bitmap("a920.asl");
    blit(formDC(), 0, 180, 81, strip.height(), strip, 0, 180);
}

void Studio::drawShades() {  // 1008:29fd: two rows of five shades under the juggler, X on the current one
    HDC dc = formDC();
    blit(dc, 0, 179, 81, 34, app_.assets().bitmap("a920.asl"), 0, 179);
    HPEN pen = CreatePen(PS_SOLID, 1, colour(64));  // DS:177a, grey
    HGDIOBJ oldPen = SelectObject(dc, pen);
    for (int i = 0, x = 0; i < 5; ++i, x += 16) {
        HBRUSH top = CreateSolidBrush(colour(family_ * 10 + i));
        HGDIOBJ old = SelectObject(dc, top);
        Ellipse(dc, x, 180, x + 16, 196);
        HBRUSH bottom = CreateSolidBrush(colour(family_ * 10 + i + 5));
        SelectObject(dc, bottom);
        Ellipse(dc, x, 196, x + 16, 212);
        SelectObject(dc, old);
        DeleteObject(top);
        DeleteObject(bottom);
    }
    // The marker: red (DS:17f6), or yellow (DS:17c6) on the red family.
    HPEN marker = CreatePen(PS_SOLID, 1, family_ != 9 ? colour(95) : colour(83));
    SelectObject(dc, marker);
    DeleteObject(pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    const int x = (shade_ < 5 ? shade_ : shade_ - 5) * 16, y = shade_ < 5 ? 180 : 196;
    Rectangle(dc, x, y, x + 16, y + 16);
    MoveToEx(dc, x, y, nullptr);  // 1028:165f twice: the X
    LineTo(dc, x + 16, y + 16);
    MoveToEx(dc, x + 16, y, nullptr);
    LineTo(dc, x, y + 16);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(marker);
    current_ = colour(family_ * 10 + shade_);
    updateShadeIndexes();
    if (b104d) drawPanel(280, "a3.asl");  // 1008:377d
}

void Studio::drawToolPointer(int tab) {  // 1008:3a68: the red triangle over the selected option
    pointerTab_ = tab;
    HDC dc = formDC();
    blit(dc, 100, 50, 500, 12, app_.assets().bitmap("a922.asl"));
    if (tab >= 11) return;
    HPEN pen = CreatePen(PS_SOLID, 1, colour(95));
    HBRUSH brush = CreateSolidBrush(colour(95));
    HGDIOBJ oldPen = SelectObject(dc, pen), oldBrush = SelectObject(dc, brush);
    const int x = tab * 40;
    const POINT points[] = {{x + 120, 51}, {x + 100, 59}, {x + 140, 59}, {x + 120, 51}};
    Polygon(dc, points, 4);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void Studio::drawPanel(int y, const char* file) {
    // The painters pass (0, y, 79, y + 50) to BitBlt as if it took a rectangle: 79 wide and
    // 50 rows taller than the 80x51 pictures, so in effect the whole picture at (0, y).
    if (file) blit(formDC(), 0, y, 79, y + 50, app_.assets().bitmap(file));
}

void Studio::drawPanels() {  // 1008:83a4 (also the tail of FormPaint 1008:68a2)
    static const char* const kThickness[] = {"a29.asl", "a28.asl", "a27.asl", "a26.asl", "a25.asl",
                                             "a24.asl", "a23.asl", "a22.asl", "a21.asl", "a20.asl"};
    if (level_ == 2) return drawPanels2();
    if (b1042) drawPanel(230, thickness_ >= 1 && thickness_ <= 10 ? kThickness[thickness_ - 1] : nullptr);  // 2eb1
    if (b1056) drawPanel(280, w0fdc == 1 ? "a65.asl" : w0fdc == 2 ? "a64.asl" : nullptr);               // 310d
    if (b1057) drawPanel(230, w0fda == 1 ? "a8.asl" : w0fda == 2 ? "a9.asl" : nullptr);                 // 356e
    if (b1045) drawPanel(230, w0ff8 == 2 ? "a8.asl" : w0ff8 == 1 ? "a9.asl" : nullptr);                 // 3358
    if (b1058) drawPanel(230, w0fd8 == 4 ? "a66.asl" : w0fd8 == 3 ? "a67.asl" : nullptr);               // 3464
    if (b1046) drawPanel(380, multiColour_ ? "a17.asl" : "a18.asl");                                    // 3969
    if (b1047) drawPanel(230, w0ff6 == 1 ? "a15.asl" : w0ff6 == 2 ? "a16.asl" : nullptr);               // 385d
    if (b1048) drawPanel(230, w0ff4 == 3 ? "a30.asl" : w0ff4 == 1 ? "a31.asl" : w0ff4 == 2 ? "a32.asl" : nullptr);  // 3221
    if (b104c) drawPanel(280, w0fec == 1 ? "a41.asl" : w0fec == 3 ? "a42.asl" : nullptr);              // 367a
    if (b104d) drawPanel(280, "a3.asl");                                                                // 377d
}

void Studio::drawPanels2() {  // CRAYON2 2:a41a: 25 panels (docs/re/level2.md)
    using Pick = std::initializer_list<std::pair<int, const char*>>;
    auto pick = [](int value, Pick choices) -> const char* {
        for (const auto& [v, file] : choices)
            if (v == value) return file;
        return nullptr;
    };
    static const char* const kThickness[] = {"a29.asl", "a28.asl", "a27.asl", "a26.asl", "a25.asl",
                                             "a24.asl", "a23.asl", "a22.asl", "a21.asl", "a20.asl"};
    static const char* const kDepth[] = {"a79.asl", "a78.asl", "a77.asl", "a76.asl", "a75.asl",
                                         "a74.asl", "a73.asl", "a72.asl", "a71.asl", "a70.asl"};
    if (b104f) drawPanel(330, pick(w0fea, {{1, "a51.asl"}, {2, "a52.asl"}}));                          // 46b9
    if (b1050) drawPanel(330, pick(w0fe8, {{1, "a53.asl"}, {2, "a54.asl"}}));                          // 48d1
    if (b1051) drawPanel(330, pick(w0fe6, {{1, "a55.asl"}, {2, "a56.asl"}, {3, "a57.asl"}}));          // 49e5
    if (b1053) drawPanel(330, pick(w0fe2, {{3, "a61.asl"}, {4, "a62.asl"}}));                          // 47c5
    if (b1042) drawPanel(230, thickness_ >= 1 && thickness_ <= 10 ? kThickness[thickness_ - 1] : nullptr);  // 30fd
    if (b105c) drawPanel(280, "a84.asl");                                                             // 3535
    if (b105e) drawPanel(330, pick(w0fd6, {{2, "a90.asl"}, {3, "a91.asl"}, {4, "a92.asl"}, {5, "a93.asl"},
                                           {6, "a94.asl"}, {7, "a95.asl"}}));                         // 3385
    if (b1054) drawPanel(230, w0fde >= 1 && w0fde <= 10 ? kDepth[w0fde - 1] : nullptr);                // 3660
    if (b1055) drawPanel(280, pick(w0fe0, {{2, "a80.asl"}, {4, "a81.asl"}, {5, "a82.asl"}, {7, "a83.asl"}}));  // 38cc
    if (b1043) drawPanel(280, pick(w0ffc, {{1, "a11.asl"}, {2, "a12.asl"}, {3, "a13.asl"}, {4, "a14.asl"}}));  // 4f23
    if (b1044) drawPanel(280, pick(w0ffa, {{4, "a7.asl"}, {3, "a6.asl"}}));                            // 5088
    if (b1056) drawPanel(280, pick(w0fdc, {{1, "a65.asl"}, {2, "a64.asl"}}));                          // 3a33
    if (b1057) drawPanel(230, pick(w0fda, {{1, "a8.asl"}, {2, "a9.asl"}}));                            // 44a1
    if (b1045) drawPanel(230, pick(w0ff8, {{2, "a8.asl"}, {1, "a9.asl"}}));                            // 428b
    if (b1058) drawPanel(230, pick(w0fd8, {{4, "a66.asl"}, {3, "a67.asl"}}));                          // 4397
    if (b1046) drawPanel(380, multiColour_ ? "a17.asl" : "a18.asl");                                   // 4e0d
    if (b1047) drawPanel(230, pick(w0ff6, {{1, "a15.asl"}, {2, "a16.asl"}}));                          // 4bf5
    if (b1048) drawPanel(230, pick(w0ff4, {{3, "a30.asl"}, {1, "a31.asl"}, {2, "a32.asl"}}));          // 3d91
    if (b1049) drawPanel(230, pick(w0ff2, {{1, "a33.asl"}, {2, "a34.asl"}, {3, "a35.asl"}}));          // 3ed2
    if (b104a) drawPanel(230, pick(w0ff0, {{1, "a37.asl"}, {2, "a36.asl"}}));                          // 4d01
    if (b104b) drawPanel(230, pick(w0fee, {{1, "a38.asl"}, {2, "a39.asl"}, {3, "a40.asl"}}));          // 4154
    if (b104c) drawPanel(280, pick(w0fec, {{1, "a41.asl"}, {3, "a42.asl"}}));                          // 45ad
    if (b1052) drawPanel(280, pick(w0fe4, {{2, "a58.asl"}, {1, "a59.asl"}, {3, "a60.asl"}}));          // 4013
    if (b104e) drawPanel(230, pick(w1abe, {{2, "a43.asl"}, {3, "a44.asl"}, {4, "a45.asl"}, {5, "a49.asl"},
                                           {6, "a48.asl"}, {7, "a47.asl"}, {8, "a46.asl"}, {9, "a50.asl"}}));  // 3b7f
    if (b104d) drawPanel(280, "a3.asl");                                                              // 4b15
}

void Studio::clearPanelFlags() {  // 1008:779f
    b1042 = b1043 = b1044 = b1045 = b1047 = b1046 = b1048 = b1049 = b104a = b104b = b104c = b104e = b104d =
        b104f = b1050 = b1051 = b1052 = b1053 = b1054 = b1055 = b1056 = b1058 = b1057 = b1059 = b105c = b105e = false;
}

void Studio::playSound(int n) {  // 1028:000f
    if (soundsOn_) app_.host().playSound("snd\\snd" + std::to_string(n) + ".asl");
}

void Studio::saveBackup() {  // 1028:0127
    blit(backup_.dc(), 0, 0, pictureWidth(), pictureHeight(), picture_.surface);
}

void Studio::restoreBackup() {  // 1028:0281
    blit(picture_.surface.dc(), 0, 0, pictureWidth(), pictureHeight(), backup_);
}

void Studio::saveUndo() {  // 1028:01d4: BiPict1 -> DS:69f0 (only the hints use it)
    blit(undo_.dc(), 0, 0, pictureWidth(), pictureHeight(), picture_.surface);
}

void Studio::restoreUndo() {  // 1028:032e
    blit(picture_.surface.dc(), 0, 0, pictureWidth(), pictureHeight(), undo_);
}

void Studio::showDialog(std::unique_ptr<Dialog> dialog) {
    dialog_ = std::move(dialog);
    dialog_->setHints(&hints_);
}

void Studio::finishDialog() {
    if (!dialog_ || !dialog_->closed()) return;
    std::unique_ptr<Dialog> closedDialog = std::move(dialog_);
    std::function<void()> then = std::move(closedDialog->onClose);
    if (then) then();
}

namespace {
bool inside(const RECT& r, int x, int y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }
}  // namespace

void Studio::mouseDown(int x, int y, int button) {
    if (button == 0) {
        capture_ = dialog_ ? Capture::Dialog
                 : tabs_.contains(x, y) ? Capture::Tabs
                 : picture_.contains(x, y) ? Capture::Picture
                 : form_.contains(x, y) ? Capture::Form : Capture::None;
    }
    if (!dialog_ && hint_.contains(x, y)) return;  // Panel1 has no handlers
    if (dialog_) {  // modal: the client, plus Taboutme's system-menu close box
        const RECT c = dialog_->clientRect();
        if (inside(c, x, y)) dialog_->mouseDown(x - c.left, y - c.top, button);
        else dialog_->chromeMouseDown(x, y, button);
        if (dialog_->closed()) finishDialog();
        else {
            dialog_->repaint();
        }
        return;
    }
    if (tabs_.contains(x, y)) {  // TabSet1: TTabSet.MouseDown 1040:2c52 → SetTabIndex → OnClick
        if (button == 1) {      // TabSet1MouseDown 1008:9996: the tab's hint
            const int tx = x - tabs_.rect.left, ty = y - tabs_.rect.top;
            showHint(tx + 80, ty + 60, (level_ == 2 ? 0x82 : 0xa3) + (tx > 44 ? (tx - 10) / 40 : 0));  // 2:bb5b
        }
        if (button != 0) return;
        const int tab = tabSet_.hitTest(x - tabs_.rect.left, y - tabs_.rect.top);
        if (tab >= 0 && tab != tabSet_.tabIndex()) {
            tabSet_.setTabIndex(tab);
            paintTabs();
            tabClicked(tab);
        }
        return;
    }
    if (picture_.contains(x, y)) {
        pictureMouseDown(x - picture_.rect.left, y - picture_.rect.top, button);
        return;
    }
    if (form_.contains(x, y)) formMouseDown(x, y, button);
}

void Studio::mouseMove(int x, int y, int) {
    if (dialog_) {
        const RECT c = dialog_->clientRect();
        if (inside(c, x, y)) dialog_->hintMove(), dialog_->mouseMove(x - c.left, y - c.top);
        return;
    }
    // Windows sends the moves to the window under the cursor, or to the captured one while
    // a button is held (Delphi captures on mouse down).
    if (hint_.contains(x, y)) return;  // Panel1 has no handlers
    const Capture over = capture_ != Capture::None ? capture_
                       : tabs_.contains(x, y) ? Capture::Tabs
                       : picture_.contains(x, y) ? Capture::Picture : Capture::Form;
    if (over == Capture::Picture) {
        pictureMouseMove(x - picture_.rect.left, y - picture_.rect.top);
        return;
    }
    // FormMouseMove 1008:935e / TabSet1MouseMove 93d8
    if (hintShown_ && hintArmed_) hideHint();
    screenCursor_ = 0;  // 1008:6d6a: Screen.Cursor := crDefault
}

void Studio::mouseUp(int x, int y, int button) {
    if (button != 0) return;
    const Capture captured = capture_;
    capture_ = Capture::None;
    if (dialog_) return;
    if (captured == Capture::Picture) pictureMouseUp(x - picture_.rect.left, y - picture_.rect.top);
}


void Studio::tabClicked(int tab) {  // TabSet1Click 1008:91d2
    playSound(93);
    selectTool(tab);
    mode_ = 0;
}

void Studio::selectTool(int tab) {  // 1008:8fc7
    static const char* const kSheets[] = {"a950.asl", "a951.asl", "a952.asl", "a904.asl", "a914.asl",
                                          "a953.asl", "a908.asl", "a954.asl", "a910.asl"};
    static const char* const kSheets2[] = {"a900.asl", "a901.asl", "a902.asl", "a903.asl", "a904.asl", "a905.asl",
                                           "a914.asl", "a906.asl", "a907.asl", "a908.asl", "a910.asl"};  // 2:b10c
    timer1On_ = false;
    optionSheet_ = Surface();
    if (level_ == 2 && tab >= 0 && tab <= 10) loadBmp(app_.assets().files().read(kSheets2[tab]), &optionSheet_);
    else if (level_ != 2 && tab >= 0 && tab <= 8) loadBmp(app_.assets().files().read(kSheets[tab]), &optionSheet_);
    frame_ = frameBack_ = 0;
    timer1Interval_ = 125;
    timer1On_ = true;
    timer1Due_ = app_.now() + timer1Interval_;
    tool_ = tab + 1;
    drawToolPointer(12);
}

void Studio::formMouseDown(int x, int y, int button) {  // 1008:6ef9
    bool handled = false;  // [bp-0x105]: repaint the option panels at the end
    if (hintShown_) {
        hideHint();
        handled = true;
    }
    if (typing_) {
        textFinish();
        handled = true;
    }
    if (b105b) {
        handled = true;
        restoreBackup();
        b105b = false;
        mode_ = 0;
    }
    if (button == 1) {  // right button: a hint
        const int hint = formHint(x, y);
        if (hint < 300) showHint(x, y, hint);
        return;
    }
    if (inRect(x, y, 0, 80, 430, 480)) {  // the "?": help
        saveBackup();
        std::wstring error;
        const std::wstring path = help::exportHtml(app_.assets().files(), &error);
        if (path.empty()) app_.host().messageBox(error, L"Crayons Help", MB_ICONWARNING);
        else app_.host().openDocument(path);
        handled = true;
    }
    if (inRect(x, y, pictureWidth() + 40, pictureWidth() + 80, 0, 40)) {  // EXIT
        // 1008:4b55, then Close → FormDestroy 1008:8ee1 ("close midi"); the launcher had
        // already ended, so the program is over.
        if (app_.host().messageBox(L"Do you really want to quit?", L"Just Checking...", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            playSound(99);
            frame(650);  // let the goodbye sound play out (0.65 s)
            app_.host().stopMusic();
            app_.host().quit();
        }
    }
    if (inRect(x, y, 0, 80, 0, 133)) {  // the juggler's balls: colour families
        playSound(98);
        struct Ball { int x1, x2, y1, y2; };
        static const Ball kBalls[16] = {{61, 76, 75, 90}, {63, 88, 55, 70}, {64, 79, 119, 134}, {55, 70, 95, 110},
                                        {1, 16, 49, 64},  {31, 46, 32, 47}, {26, 41, 77, 93},   {5, 20, 9, 24},
                                        {0, 15, 28, 43},  {45, 60, 3, 18},  {63, 78, 35, 50},   {60, 75, 15, 30},
                                        {32, 47, 62, 77}, {2, 17, 70, 85},  {24, 39, 3, 18},    {27, 43, 47, 63}};
        for (int i = 0; i < 16; ++i)
            if (inRect(x, y, kBalls[i].x1, kBalls[i].x2, kBalls[i].y1, kBalls[i].y2)) family_ = i;
        saveBackup();
        shade_ = 5;
        drawShades();
        return;
    }
    if (inRect(x, y, 0, 43, 134, 157)) {  // UNDO
        playSound(97);
        restoreBackup();
        return;
    }
    if (b1042 && inRect(x, y, 0, 80, 256, 277)) {  // thickness arrows
        handled = true;
        playSound(96);
        if (inRect(x, y, 4, 24, 256, 277) && thickness_ > 1) --thickness_;
        if (inRect(x, y, 54, 76, 256, 277) && thickness_ < 10) ++thickness_;
    }
    if (level_ == 2) {  // CRAYON2 2:8a7b…8caf: the 3D box depth and direction, brush and roller direction
        if (b1054 && inRect(x, y, 0, 80, 256, 277)) {
            handled = true;
            playSound(96);
            if (inRect(x, y, 4, 24, 256, 277) && w0fde > 1) --w0fde;
            if (inRect(x, y, 54, 76, 256, 277) && w0fde < 10) ++w0fde;
        }
        if (b1055 && inRect(x, y, 0, 80, 280, 330)) {
            handled = true;
            playSound(96);
            if (inRect(x, y, 0, 40, 280, 305)) w0fe0 = 2;
            if (inRect(x, y, 40, 80, 280, 305)) w0fe0 = 4;
            if (inRect(x, y, 0, 40, 305, 330)) w0fe0 = 5;
            if (inRect(x, y, 40, 80, 305, 330)) w0fe0 = 7;
        }
        if (b1043 && inRect(x, y, 0, 80, 280, 330)) {
            handled = true;
            playSound(96);
            if (inRect(x, y, 6, 32, 282, 308)) w0ffc = 1;
            if (inRect(x, y, 5, 38, 313, 323)) w0ffc = 3;
            if (inRect(x, y, 40, 63, 282, 310)) w0ffc = 2;
            if (inRect(x, y, 64, 76, 296, 326)) w0ffc = 4;
        }
        if (b1044 && inRect(x, y, 0, 80, 280, 330)) {
            handled = true;
            playSound(96);
            if (inRect(x, y, 0, 40, 280, 330)) w0ffa = 4;
            if (inRect(x, y, 40, 80, 280, 330)) w0ffa = 3;
        }
    }
    if (b104d && inRect(x, y, 0, 80, 280, 330)) {
        handled = true;
        saveBackup();
        auto dialog = std::make_unique<FillStylesDialog>(app_, level_, mode_ == 0x3a, &fillStyle_,
                                                         [this](int n) { playSound(n); });
        dialog->onClose = [this] { restoreBackup(); };  // after BiPict1.Refresh
        showDialog(std::move(dialog));
    }
    if (level_ == 2 && b105c && inRect(x, y, 0, 80, 280, 330)) {  // "Font": TFontForm (2:8cff)
        handled = true;
        saveBackup();
        auto dialog = std::make_unique<FontDialog>(app_, level_, &shadowFont_, [this](int n) { playSound(n); });
        dialog->onClose = [this] { restoreBackup(); };
        showDialog(std::move(dialog));
    }
    if (b1047 && inRect(x, y, 0, 80, 230, 280)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 230, 280)) w0ff6 = 1, mode_ = 0x1b;
        if (inRect(x, y, 40, 80, 230, 280)) w0ff6 = 2, mode_ = 0x1a;
    }
    if (b1045 && inRect(x, y, 0, 80, 230, 280)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 230, 280)) w0ff8 = 2;
        if (inRect(x, y, 40, 80, 230, 280)) w0ff8 = 1;
    }
    if (b1058 && inRect(x, y, 0, 80, 230, 280)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 230, 280)) w0fd8 = 4;
        if (inRect(x, y, 40, 80, 230, 280)) w0fd8 = 3;
    }
    if (b1057 && inRect(x, y, 0, 80, 230, 280)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 230, 280)) w0fda = 1, mode_ = 0x3f;
        if (inRect(x, y, 40, 80, 230, 280)) w0fda = 2, mode_ = 0x40;
    }
    if (b104c && inRect(x, y, 0, 80, 280, 330)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 280, 330)) w0fec = 1;
        if (inRect(x, y, 40, 80, 280, 330)) w0fec = 3;
    }
    if (level_ == 2) {  // 2:8f73…904c: the star's and parallelogram's orientation, shown at once
        if (b104f && inRect(x, y, 0, 80, 330, 380)) {
            playSound(96);
            handled = true;
            if (inRect(x, y, 0, 40, 330, 380)) w0fea = 1;
            if (inRect(x, y, 40, 80, 330, 380)) w0fea = 2;
            drawPanels2();
        }
        if (b1053 && inRect(x, y, 0, 80, 330, 380)) {
            playSound(96);
            handled = true;
            if (inRect(x, y, 0, 40, 330, 380)) w0fe2 = 3;
            if (inRect(x, y, 40, 80, 330, 380)) w0fe2 = 4;
            drawPanels2();
        }
    }
    if (b1056 && inRect(x, y, 0, 80, 280, 330)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 280, 330)) w0fdc = 2;
        if (inRect(x, y, 40, 80, 280, 330)) w0fdc = 1;
    }
    if (b1050 && inRect(x, y, 0, 80, 330, 380)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 330, 380)) w0fe8 = 1;
        if (inRect(x, y, 40, 80, 330, 380)) w0fe8 = 2;
    }
    if (b1048 && inRect(x, y, 0, 80, 230, 280)) {
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 27, 230, 280)) w0ff4 = 3;
        if (inRect(x, y, 28, 54, 230, 280)) w0ff4 = 1;
        if (inRect(x, y, 55, 80, 230, 280)) w0ff4 = 2;
    }
    if (level_ == 2) {  // 2:91b1…967f
        auto third = [&](int y1, int y2, int a, int b, int c) {  // 0..27, 28..54, 55..80
            int v = 0;
            if (inRect(x, y, 0, 27, y1, y2)) v = a;
            if (inRect(x, y, 28, 54, y1, y2)) v = b;
            if (inRect(x, y, 55, 80, y1, y2)) v = c;
            return v;
        };
        if (b1052 && inRect(x, y, 0, 80, 280, 330)) {  // the mirror lines' direction (mode 0x0b)
            playSound(96);
            handled = true;
            if (int v = third(280, 330, 2, 1, 3)) w0fe4 = v;
        }
        if (b105e && inRect(x, y, 0, 80, 330, 380)) {  // shadow direction: six cells
            playSound(96);
            handled = true;
            if (int v = third(330, 355, 2, 3, 4)) w0fd6 = v;
            if (int v = third(356, 380, 5, 6, 7)) w0fd6 = v;
        }
        if (b1051 && inRect(x, y, 0, 80, 330, 380)) {
            playSound(96);
            handled = true;
            if (int v = third(330, 380, 1, 2, 3)) w0fe6 = v;
        }
        if (b1049 && inRect(x, y, 0, 80, 230, 280)) {
            playSound(96);
            handled = true;
            static const int kModes[] = {0, 0x1e, 0x1d, 0x14};
            if (int v = third(230, 280, 1, 2, 3)) w0ff2 = v, mode_ = kModes[v];
        }
        if (b104e && inRect(x, y, 0, 80, 230, 310)) {  // the light direction of 3D shapes
            playSound(96);
            handled = true;
            if (int v = third(230, 257, 2, 3, 4)) w1abe = v;
            if (inRect(x, y, 0, 27, 258, 284)) w1abe = 9;
            if (inRect(x, y, 55, 80, 258, 284)) w1abe = 8;
            if (int v = third(285, 310, 5, 6, 7)) w1abe = v;
        }
        if (b104a && inRect(x, y, 0, 80, 230, 280)) {
            playSound(96);
            handled = true;
            if (inRect(x, y, 0, 40, 230, 280)) mode_ = 0x0d, w0ff0 = 2;
            if (inRect(x, y, 40, 80, 230, 280)) mode_ = 0x0c, w0ff0 = 1;
        }
        if (b104b && inRect(x, y, 0, 80, 230, 280)) {  // grass length
            playSound(96);
            handled = true;
            if (int v = third(230, 280, 1, 2, 3)) w0fee = v;
        }
    }
    if (b1046 && inRect(x, y, 0, 80, 380, 430)) {  // the clown (multi-colour) / business man
        playSound(96);
        handled = true;
        if (inRect(x, y, 0, 40, 380, 430)) multiColour_ = true;
        if (inRect(x, y, 40, 80, 380, 430)) multiColour_ = false;
    }
    if (y >= 180 && y < 213 && x < 80) {  // the shades
        playSound(95);
        handled = true;
        shade_ = y <= 196 ? x / 16 : x / 16 + 5;
        drawShades();
        saveBackup();
        return;
    }
    if (inRect(x, y, 100, 500, 10, 50)) {  // the options strip
        playSound(94);
        handled = true;
        saveBackup();
        clearPanelFlags();
        if (!optionClicked((x - 100) / 40)) return;
    }
    if (handled) {  // 1008:839f
        restoreLeftStrip();
        drawPanels();
        drawShades();
    }
}

bool Studio::optionClicked(int option) {  // 1008:7824: what each option of each tool selects
    drawToolPointer(option);
    if (option > 9) return true;
    // Tools as CRAYON1 numbers them (tool()): 1 crayon, 2 paintbrush, 3 clown, 4 shapes, 5 ABCD,
    // 6 eraser, 7 pictures, 8 coloring book, 9 PC; 13 3D, 14 shadow text, 15 Magic Hat (level 2).
    auto set = [&](int mode, std::initializer_list<bool*> flags) {
        mode_ = mode;
        for (bool* f : flags) *f = true;
    };
    auto none = [&] { drawToolPointer(12); };
    // CRAYON2 keeps the variants (it has panels to pick them) and adds options 7–9 to the first
    // three tools (docs/re/level2.md).
    const bool level1 = level_ != 2;
    switch (option) {
    case 0:
        switch (tool()) {
        case 1: set(0x01, {&b1042, &b1046}); break;
        case 2: set(0x16, {&b1042}); break;
        case 3: set(0x07, {&b1046, &b1048}); break;
        case 4: set(0x21, {&b1042, &b104d}); break;
        case 5: set(0x51, {&b1042}), font_ = "Creeper"; break;
        case 6: wipe(random(8) + 1); break;
        case 7:  // colorful icon; original 1008:7900 used picker 6
            restoreLeftStrip();
            choosePicture(7, [this](bool chosen) { mode_ = chosen ? 0x33 : 0; });
            return false;
        case 8:  // a coloring-book page (1008:795c)
            restoreLeftStrip();
            choosePicture(1, [this](bool chosen) {
                if (!chosen) return;
                w1aa8 = 2;
                loadStretched();
                saveBackup();
                restoreBackup();
                tabSet_.setTabIndex(1);  // SetTabIndex runs TabSet1Click
                paintTabs();
                tabClicked(1);
                selectTool(1);
                drawToolPointer(2);
                tool_ = 2;
                mode_ = 0x3a;  // the paint can
                b104d = true;
                fillStyle_ = 1;
                drawPanel(280, "a3.asl");
                b1059 = true;  // fills stop at black outlines
                restoreBackup();
            });
            return false;
        case 9: savePicture(); break;
        case 13: set(0x2c, {&b104e}); break;
        case 14: b105c = true, mode_ = 0x4b; break;
        case 15: mode_ = 0x45; break;
        }
        break;
    case 1:
        switch (tool()) {
        case 1: set(0x17, {&b1042, &b1046}); break;
        case 2: set(0x15, {&b1042, &b1043}), w0ffc = level1 ? 3 : w0ffc; break;
        case 3: set(0x06, {&b1042, &b1046}); break;
        case 4: set(0x1f, {&b1042, &b104d}); break;
        case 5: set(0x51, {&b1042}), font_ = "Walters"; break;
        case 6: mode_ = 0x2f; break;
        case 7:  // monochrome icon; original 1008:7ae9 used picker 7
            restoreLeftStrip();
            choosePicture(6, [this](bool chosen) { mode_ = chosen ? 0x33 : 0; });
            return false;
        case 8: {  // Magic Color (1008:7b45)
            restoreLeftStrip();
            picker_ = 0xd;
            auto dialog = std::make_unique<ActivityDialog>(app_, level_, picker_, [this](int n) { playSound(n); });
            ActivityDialog* d = dialog.get();
            dialog->onClose = [this, d] {
                if (d->chosen) {
                    chosen_ = d->path;
                    w1aa8 = 2;
                    whiteOut();
                    Surface page;  // 1008:532c: the page at full size
                    if (loadBmp(app_.assets().files().read(chosen_), &page))
                        BitBlt(pictureDC(), 0, 0, page.width(), page.height(), page.dc(), 0, 0, SRCCOPY);
                    saveBackup();
                    restoreBackup();
                    tabSet_.setTabIndex(1);
                    paintTabs();
                    tabClicked(1);
                    selectTool(1);
                    drawToolPointer(2);
                    tool_ = 2;
                    mode_ = 0x50;  // the magic paint can
                    b104d = false;
                }
                restoreBackup();
            };
            showDialog(std::move(dialog));
            return false;
        }
        case 9:  // open one of the child's pictures (1008:7bed)
            restoreLeftStrip();
            choosePicture(5, [this](bool chosen) {
                if (chosen) loadStretched(), saveBackup();
                restoreBackup();
            });
            return false;
        case 13: set(0x2d, {&b104e}); break;
        case 14: set(0x4d, {&b105c, &b105e}); break;
        case 15: mode_ = 0x46; break;
        }
        break;
    case 2:
        switch (tool()) {
        case 1: mode_ = 0x0a; break;
        case 2:
            set(0x3a, {&b104d});
            if (fillStyle_ > 15) fillStyle_ = 1;
            break;
        case 3: set(0x03, {&b1042}); break;
        case 4: set(0x22, {&b1042, &b1051, &b104d}), w0fe6 = level1 ? 1 : w0fe6; break;
        case 5: set(0x51, {&b1042}), font_ = "Crayon"; break;
        case 6: shrinkAway(); break;
        case 7:  // a background (1008:7d33)
            restoreLeftStrip();
            choosePicture(4, [this](bool chosen) {
                if (chosen) loadStretched(), saveBackup();
                restoreBackup();
            });
            return false;
        case 8: none(); break;
        case 9: printPicture(); break;
        case 13: set(0x2e, {&b104e}); break;
        case 14: set(0x4e, {&b105c}); break;
        case 15: set(w0fda == 1 ? 0x3f : 0x40, {&b1057}); break;
        }
        break;
    case 3:
        switch (tool()) {
        case 1: mode_ = 0x05, w0ffc = 1; break;
        case 2: set(0x19, {&b1042, &b1044}), w0ffa = level1 ? 3 : w0ffa; break;
        case 3: set(w0ff6 == 1 ? 0x1b : 0x1a, {&b1046, &b1047}); break;
        case 4: set(0x20, {&b1042, &b104d}); break;
        case 5: set(0x51, {&b1042}), font_ = "Mexacali"; break;
        case 6: shatter(); break;
        case 7: case 8: none(); break;
        case 9:  // delete one of the child's pictures (1008:7e94), without asking
            choosePicture(8, [this](bool chosen) {
                if (chosen) DeleteFileW(app_.assets().files().fullPath(chosen_).c_str());
                restoreBackup();
            });
            return false;
        case 13: mode_ = 0x2b; break;
        case 14: set(0x4f, {&b105c, &b1042}); break;
        case 15: horizon(); break;
        }
        break;
    case 4:
        switch (tool()) {
        case 1: set(0x02, {&b1042, &b1046}); break;
        case 2: set(0x08, {&b1042, &b1043}), w0ffc = level1 ? 3 : w0ffc; break;
        case 3: set(0x11, {&b1042, &b1046}); break;
        case 4: set(0x23, {&b1042, &b104d, &b1050}), w0fe8 = level1 ? 1 : w0fe8; break;
        case 5: set(0x51, {&b1042}), font_ = "Treehouse"; break;
        case 6: sweepUp(); break;
        case 7: case 8: none(); break;
        case 9:  // About (g101c)
            showDialog(std::make_unique<AboutDialog>(app_, level_, [this](int n) { playSound(n); }));
            dialog_->onClose = [this] { restoreBackup(); };
            return false;
        case 13: set(0x3b, {&b1042}); break;
        case 14: set(0x4c, {&b105c, &b105e}); break;
        case 15: squiggle(); break;
        }
        break;
    case 5:
        switch (tool()) {
        case 1: set(0x0b, {&b1042, &b1046, &b1052}), w0fe4 = level1 ? 2 : w0fe4; break;
        case 2: set(0x10, {&b1042}); break;
        case 3: set(0x32, {&b1042}); break;
        case 4: set(0x29, {&b1042, &b104d}); break;
        case 5: case 7: case 8: case 14: none(); break;
        case 6: mode_ = 0x39; break;
        case 9: {  // the jukebox (1008:80b6)
            const bool musicWas = musicOn_;
            showDialog(std::make_unique<JukeboxDialog>(app_, level_, &soundsOn_, &musicOn_, &audioHelp_,
                                                       [this](int n) { playSound(n); }));
            dialog_->onClose = [this, musicWas] {
                restoreBackup();
                if (musicWas && !musicOn_) stopMusic();
                if (!musicWas && musicOn_) musicPlaying_ = false, startMusic();
            };
            return false;
        }
        case 13: set(0x3c, {&b1054, &b1055}); break;
        case 15: set(0x41, {&b1056}); break;
        }
        break;
    case 6:
        switch (tool()) {
        case 1: set(0x13, {&b1042}); break;
        case 2: set(0x09, {&b1042, &b104c}); break;
        case 3: mode_ = 0x30; break;
        case 4: set(0x28, {&b1042, &b104d}); break;
        case 5: case 6: case 7: case 8: case 14: none(); break;
        case 9:  // the leftover shareware order form (g1006), behind an empty option
            showDialog(std::make_unique<OrdersDialog>(app_, level_, [this](int n) { playSound(n); }));
            dialog_->onClose = [this] { restoreBackup(); };
            return false;
        case 13: set(0x3d, {&b1042}); break;
        case 15: set(0x42, {&b1058}); break;
        }
        break;
    case 7:
        switch (tool()) {
        case 1:
            if (level1) set(0x26, {&b1042});
            else set(w0ff0 == 1 ? 0x0c : 0x0d, {&b104a});
            break;
        case 2: set(0x2a, {&b1045}); break;
        case 3: set(0x38, {&b1042, &b1046}); break;
        case 4: set(0x25, {&b1042, &b104d}); break;
        case 13: set(0x3e, {&b1042}); break;
        case 15: mode_ = 0x43; break;
        default: none(); break;
        }
        break;
    case 8:
        switch (tool()) {
        case 1:
            if (level1) none();
            else set(0x26, {&b1042});
            break;
        case 2:
            if (level1) none();
            else set(0x0f, {&b1042, &b1043});
            break;
        case 3:
            if (level1) horizon();
            else set(0x04, {&b104b});
            break;
        case 4: set(0x24, {&b104f, &b1042, &b104d}), w0fea = level1 ? 1 : w0fea; break;
        case 15: mode_ = 0x44; break;
        default: none(); break;
        }
        break;
    case 9:
        switch (tool()) {
        case 2:
            if (level1) none();
            else set(0x0e, {&b1042, &b1043});
            break;
        case 3:
            if (level1) squiggle();
            else none();
            break;
        case 4: set(0x36, {&b1042, &b104d, &b1053}), w0fe2 = level1 ? 3 : w0fe2; break;
        case 15: set(0x47, {&b1058}); break;
        default: none(); break;
        }
        break;
    }
    return true;
}

void Studio::choosePicture(int picker, std::function<void(bool)> then) {  // 1008:4c27 + ShowModal
    if (!ThumbsDialog::anyPictures(app_, picker)) {
        app_.host().messageBox(L"There are no pictures to see.", L"Important!", MB_ICONINFORMATION);
        return;
    }
    auto dialog = std::make_unique<ThumbsDialog>(app_, level_, picker, [this](int n) { playSound(n); });
    ThumbsDialog* d = dialog.get();
    dialog->onClose = [this, d, then] {
        if (d->chosen) chosen_ = d->path, clipOrientation_ = d->orientation ? d->orientation : clipOrientation_;
        then(d->chosen);
    };
    showDialog(std::move(dialog));
}

void Studio::loadStretched() {  // 1008:5256
    Surface picture;
    if (loadBmp(app_.assets().files().read(chosen_), &picture))
        StretchBlt(pictureDC(), 0, 0, pictureWidth(), pictureHeight(), picture.dc(), 0, 0, picture.width(),
                   picture.height(), SRCCOPY);
    saveBackup();
}

void Studio::savePicture() {  // 1008:5012
    // "YP" and six random letters or digits; the picture is BiPict1's size (DS:1ac6 x 1ac8).
    std::string name = "YP";
    for (int i = 0; i <= 5; ++i) name += random(10) < 5 ? char('A' + random(26)) : char('0' + random(10));
    const std::string path = "user\\" + name + ".BMP";
    const std::wstring full = app_.assets().files().fullPath(path);
    if (GetFileAttributesW(full.c_str()) != INVALID_FILE_ATTRIBUTES) {
        app_.host().messageBox(L"Error -- Picture NOT Saved.", L"There is a problem...", MB_ICONEXCLAMATION);
        return;
    }
    Surface copy;
    copy.create(pictureWidth(), pictureHeight());
    BitBlt(copy.dc(), 0, 0, pictureWidth(), pictureHeight(), pictureDC(), 0, 0, SRCCOPY);
    CreateDirectoryW(app_.assets().files().fullPath("user").c_str(), nullptr);
    if (copy.writeBmp24(full.c_str()))
        app_.host().messageBox(L"Picture Saved!", L"It is done!", MB_ICONINFORMATION);
    else
        app_.host().messageBox(L"Error -- Picture NOT Saved.", L"There is a problem...", MB_ICONEXCLAMATION);
}

void Studio::startMusic() {  // 1008:61ac
    if (++musicTrack_ > 20) musicTrack_ = 1;
    app_.host().playMusic("snd\\pc" + std::to_string(musicTrack_) + ".mid");
    musicPlaying_ = true;
}

void Studio::stopMusic() { app_.host().stopMusic(); }  // 1008:62d8

void Studio::musicNotify() {  // AppMessage 1008:6019 / WndProc 1008:60c6
    stopMusic();
    if (musicOn_) startMusic();
}

void Studio::printPicture() {  // 1008:4db4
    if (app_.host().messageBox(L"Do you really want to print?", L"Making Sure!", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    Surface copy;  // the picture backup, 5 pixels smaller each way
    copy.create(pictureWidth() - 5, pictureHeight() - 5);
    BitBlt(copy.dc(), 0, 0, copy.width(), copy.height(), backup_.dc(), 0, 0, SRCCOPY);
    if (!app_.host().print(copy)) app_.host().messageBox(L"Printer driver is missing.", L"Crayons", MB_ICONWARNING);
}

void Studio::views(std::vector<const View*>* out) const {
    out->push_back(&form_);
    out->push_back(&tabs_);
    out->push_back(&picture_);
    out->push_back(&hint_);
    if (dialog_) out->push_back(&dialog_->view());
}
