// Right-click hints of the main form (see hints.h) and the mouse cursors (Screen.Cursor).
#include "studio.h"

#include "ne.h"

namespace {

bool inRect(int x, int y, int x1, int x2, int y1, int y2) { return x >= x1 && x < x2 && y >= y1 && y < y2; }

constexpr int kNoHint = 300;

}  // namespace

void Studio::loadHints() {  // FormCreate 1008:6466: TStringList.LoadFromFile('a939.asl')
    hints_.load(app_.assets().files().read(level_ == 2 ? "a938.asl" : "a939.asl"));  // CRAYON2 2:7ba5
    hints_.audioHelp = &audioHelp_;
    hints_.host = &app_.host();
}

int Studio::formHint(int x, int y) const {  // 1008:081c: which hint a right click asks for
    if (level_ == 2) return formHint2(x, y);
    int hint = kNoHint;
    if (inRect(x, y, 100, 500, 10, 50)) {  // the options strip
        const int option = (x - 100) / 40;
        switch (tool_) {
        case 1: if (option <= 7) hint = option; break;
        case 2: if (option <= 7) hint = 8 + option; break;
        case 3:
            if (option <= 7) hint = 0x12 + option;
            if (option == 8) hint = 0x9f;
            if (option == 9) hint = 0xa0;
            break;
        case 4: if (option <= 9) hint = 0x23 + option; break;
        case 5: if (option <= 9) hint = 0x32 + option; break;
        case 6: if (option <= 5) hint = 0x3c + option; break;
        case 7: if (option <= 2) hint = 0x4c + option; break;
        case 8: if (option <= 1) hint = 0xa1 + option; break;
        case 9: if (option <= 5) hint = 0x4f + option; break;
        }
    }
    if (inRect(x, y, 0, 80, 0, 180)) hint = inRect(x, y, 0, 44, 132, 156) ? 0x6d : 0x55;  // UNDO, palette
    if (inRect(x, y, 0, 80, 180, 215)) hint = 0x56;                                        // shades
    if (inRect(x, y, 0, 80, 230, 280)) {  // the first option panel
        if (b1042) hint = 0x57;
        if (b1048) hint = 0x60;
        if (b1047) hint = x < 40 ? 0x61 : 0x77;
        if (b1045) hint = x < 40 ? 0x5f : 0x75;
    }
    if (inRect(x, y, 0, 80, 280, 330)) {  // the second
        if (b104c) hint = x < 40 ? 0x5e : 0x74;
        if (b104d) hint = 0x5c;
    }
    if (inRect(x, y, 0, 80, 380, 430)) hint = x < 40 ? 0x58 : 0x73;  // the third
    if (inRect(x, y, 0, 80, 430, 480)) hint = 0x59;                   // help
    if (inRect(x, y, pictureWidth() + 80 - 40, pictureWidth() + 80, 0, 60)) hint = 0x8f;  // EXIT
    return hint;
}

int Studio::formHint2(int x, int y) const {  // CRAYON2 2:0846
    int hint = kNoHint;
    if (inRect(x, y, 100, 500, 10, 50)) {  // the options strip, by CRAYON2's tool number
        static const int kFirst[] = {0, 0, 8, 0x12, 0x1b, 0x23, 0x2d, 0x32, 0x3c, 0x42, 0x4c, 0x4f};
        static const int kCount[] = {0, 9, 10, 9, 8, 10, 5, 10, 6, 10, 3, 6};
        const int option = (x - 100) / 40;
        if (tool_ >= 1 && tool_ <= 11 && option < kCount[tool_]) {
            hint = kFirst[tool_] + option;
            if (tool_ == 1 && option == 7) hint = 0x6f;  // the crayon's options 7 and 8: 0x6f, 7
            if (tool_ == 1 && option == 8) hint = 7;
        }
    }
    if (inRect(x, y, 0, 80, 0, 180)) hint = inRect(x, y, 0, 44, 132, 156) ? 0x6d : 0x55;  // UNDO, palette
    if (inRect(x, y, 0, 80, 180, 215)) hint = 0x56;                                        // shades
    if (inRect(x, y, 0, 80, 230, 280)) {
        if (b1042) hint = 0x57;
        if (b1054) hint = 0x64;
        if (b1048) hint = 0x60;
        if (b104b) {
            hint = 0x7b;
            if (x < 28) hint = 0x62;
            if (x > 54) hint = 0x7c;
        }
        if (b1047) hint = x < 40 ? 0x61 : 0x77;
        if (b104a) hint = x < 40 ? 0x6f : 0x7a;
        if (b1045) hint = x < 40 ? 0x5f : 0x75;
        if (b1057) hint = 0x6e;
        if (b1058) hint = mode_ == 0x42 ? (x < 40 ? 0x70 : 0x78) : (x < 40 ? 0x71 : 0x79);
    }
    if (inRect(x, y, 0, 80, 280, 330)) {
        if (b105c) hint = 0x6a;
        if (b1055) hint = 0x65;
        if (b1056) hint = x < 40 ? 0x6c : 0x76;
        if (b1052) {
            hint = 0x8d;
            if (x < 28) hint = 0x5b;
            if (x > 54) hint = 0x8e;
        }
        if (b104c) hint = x < 40 ? 0x5e : 0x74;
        if (b104d) hint = 0x5c;
        if (b1043) hint = 0x5a;
        if (b1044) hint = 0x5d;
    }
    if (inRect(x, y, 0, 80, 330, 380)) {
        if (b104f) hint = 0x68;
        if (b1053) hint = 0x69;
        if (b1050) hint = 0x67;
        if (b1051) hint = 0x66;
        if (b105e) hint = mode_ == 0x4c ? 0x72 : 0x6b;
    }
    if (inRect(x, y, 0, 80, 230, 310) && b104e) hint = 0x63;
    if (inRect(x, y, 0, 80, 380, 430)) hint = x < 40 ? 0x58 : 0x73;
    if (inRect(x, y, 0, 80, 430, 480)) hint = 0x59;
    if (inRect(x, y, pictureWidth() + 80 - 40, pictureWidth() + 80, 0, 60)) hint = 0x8f;
    return hint;
}

void Studio::showHint(int x, int y, int hint) {  // 1008:034f
    const std::string* text = hints_.text(hint);
    if (!text) return;
    saveUndo();  // the panel covered BiPict1, which is repainted from the undo copy when it goes
    HintPanel panel;
    panel.place(*text, x, y, pictureWidth());  // moved left to end at DS:1ac6
    const RECT& r = panel.rect;
    hint_.place(r.left, r.top, r.right - r.left, r.bottom - r.top, 1, RGB(255, 255, 0));
    panel.paint(hint_.surface.dc(), -r.left - 1, -r.top - 1);
    hint_.visible = true;
    hints_.speak(hint);
    hintShown_ = true;   // DS:1066
    hintArmed_ = false;  // DS:1067: set by Timer2 (250 ms); then a move hides the panel
    timer2On_ = true;
    timer2Due_ = app_.now() + 250;
}

void Studio::hideHint() {  // FormMouseDown 1008:6f0d, FormMouseMove 935e, TabSet1MouseMove 93d8, BiPict1MouseMove 8d93
    hint_.visible = false;
    hintShown_ = hintArmed_ = false;
    restoreUndo();
    drawToolPointer(pointerTab_);
}

void Studio::loadCursors() {  // FormCreate 1008:5551: Screen.Cursors[i] := LoadCursor('CURSOR_n')
    auto resource = [](int index) {
        if (index <= 18) return index;
        if (index <= 23) return index + 2;  // 19–23 → CURSOR_21–25
        if (index <= 31) return index + 2;  // 24–31 → CURSOR_26–33
        if (index == 32) return 20;
        return index;                       // 34, 36, 37
    };
    for (int i = 1; i < kCursors; ++i) {
        if (i == 33 || i == 35) continue;
        cursors_[i] = static_cast<HCURSOR>(ne::createIcon(app_.assets().exe(level_), "CURSOR_" + std::to_string(resource(i)), true));
    }
}

void Studio::modeCursor() {  // 1008:69d6, from BiPict1MouseMove
    if (b1043) {  // the brush's direction
        static const int kBrush[] = {0, 3, 5, 10, 8};
        if (w0ffc >= 1 && w0ffc <= 4) screenCursor_ = kBrush[w0ffc];
        return;
    }
    if (b1044) {
        screenCursor_ = w0ffa == 3 ? 4 : 21;
        return;
    }
    if (mode_ == 0x39) {
        screenCursor_ = 30;
        return;
    }
    if (mode_ == 0x3d || mode_ == 0x3e) {  // by the thickness
        static const int kThickness[] = {24, 24, 25, 26, 27, 28, 29, 30, 31, 32};
        if (thickness_ >= 1 && thickness_ <= 10) screenCursor_ = kThickness[thickness_ - 1];
        return;
    }
    switch (mode_) {
    case 0x00: screenCursor_ = 0; break;
    case 0x01: screenCursor_ = 36; break;
    case 0x04: screenCursor_ = 7; break;
    case 0x05: screenCursor_ = 37; break;
    case 0x06: screenCursor_ = 18; break;
    case 0x07: screenCursor_ = 19; break;
    case 0x09: screenCursor_ = 16; break;
    case 0x0a: screenCursor_ = 12; break;
    case 0x0b: case 0x0d: screenCursor_ = 2; break;
    case 0x0c: screenCursor_ = 14; break;
    case 0x1a: case 0x1b: screenCursor_ = 17; break;
    case 0x27: screenCursor_ = 13; break;
    case 0x2a: screenCursor_ = 23; break;
    case 0x2f: screenCursor_ = 22; break;
    case 0x33: case 0x48: case 0x49: case 0x4a: screenCursor_ = 20; break;
    case 0x3a: case 0x50: screenCursor_ = 11; break;
    case 0x3f: case 0x40: case 0x41: case 0x42: case 0x43: case 0x45: case 0x46: case 0x47: screenCursor_ = 15; break;
    case 0x44: screenCursor_ = 34; break;
    case 0x4f: case 0x51: screenCursor_ = 9; break;
    default: screenCursor_ = 1; break;
    }
}

HCURSOR Studio::cursor(int, int) {
    // Screen.Cursor: crDefault leaves each control its own (all arrows here).
    if (screenCursor_ > 0 && screenCursor_ < kCursors && cursors_[screenCursor_]) return cursors_[screenCursor_];
    return LoadCursor(nullptr, IDC_ARROW);
}
