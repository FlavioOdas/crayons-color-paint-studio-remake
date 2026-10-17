// BiPict1, the drawing area: mouse events, the per-mode dispatch (DS:1ac0) and the drawing
// routines of segment 6 (selector 1028), segment 3 (1010) and segment 4 (1018).
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

#include "studio.h"
#include "textdialogs.h"

namespace {

// A pen or brush selected into a DC for the scope, like the original's
// CreatePen/SelectObject … SelectObject/DeleteObject pairs.
struct PenIn {
    HDC dc;
    HPEN pen;
    HGDIOBJ old;
    PenIn(HDC d, int width, COLORREF c, int style = PS_SOLID) : dc(d), pen(CreatePen(style, width, c)), old(SelectObject(d, pen)) {}
    ~PenIn() { SelectObject(dc, old), DeleteObject(pen); }
};

struct BrushIn {
    HDC dc;
    HBRUSH brush;
    HGDIOBJ old;
    BrushIn(HDC d, COLORREF c) : dc(d), brush(CreateSolidBrush(c)), old(SelectObject(d, brush)) {}
    ~BrushIn() { SelectObject(dc, old), DeleteObject(brush); }
};

void line(HDC dc, int x1, int y1, int x2, int y2) {  // 1028:165f
    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);
}

int distance(int x1, int y1, int x2, int y2) {  // 1028:03db: Round(Sqrt(dx² + dy²)), Single
    const float dx = float(std::abs(x2 - x1)), dy = float(std::abs(y2 - y1));
    return int(std::nearbyint(std::sqrt(dx * dx + dy * dy)));
}

}  // namespace

int Studio::random(int range) {  // System.Random: RandSeed := RandSeed * 134775813 + 1
    randSeed_ = randSeed_ * 134775813u + 1;
    return int((uint64_t(randSeed_) * uint32_t(range)) >> 32);
}

COLORREF Studio::strokeColour() {  // 1008:1652
    return multiColour_ ? colour(random(159)) : current_;
}

static std::string hex(int v) {
    char b[16];
    std::snprintf(b, sizeof b, "0x%02x", v);
    return b;
}

void Studio::unported(const char* routine) {
    static std::set<std::string> reported;
    if (reported.insert(routine).second) std::fprintf(stderr, "studio: drawing routine not ported yet: %s\n", routine);
}

void Studio::pictureMouseDown(int x, int y, int button) {  // 1008:8cfc
    if (mode_ == 0x33 && button == 1) {
        b1004 = true;
        return;
    }
    if (b105b) {  // a Magic Hat selection is marked: grab it, or drop it with a click outside
        if (x >= selection_.left && x < selection_.right && y >= selection_.top && y < selection_.bottom) {
            grabSelection(x, y);
        } else {
            restoreBackup();
            b105b = false;
            mode_ = 0;
        }
    }
    if (typing_) {
        textFinish();
        return;
    }
    if (button == 1) return;
    drawing_ = true;
    press(x, y);
}

void Studio::pictureMouseMove(int x, int y) {  // 1008:8d84
    modeCursor();
    if (hintArmed_) hideHint();
    if (mode_ != 0x10 && drawing_) drag(x, y);
}

void Studio::pictureMouseUp(int x, int y) {  // 1008:84bd
    if (drawing_) drawing_ = false;
    release(x, y);
}

void Studio::press(int x, int y) {  // 1008:0d0a
    if (mode_ == 0x37) return;
    lastX_ = startX_ = x;
    lastY_ = startY_ = y;
    const int w = pictureWidth(), h = pictureHeight();
    switch (mode_) {
    case 0x01: playSound(1), drawLine(x, y, x + 1, y, thickness_, current_); break;
    case 0x02:
        playSound(21);
        drawLine(x, y, 0, 0, thickness_, current_);
        drawLine(x, y, w, 0, thickness_, current_);
        drawLine(x, y, w, h, thickness_, current_);
        drawLine(x, y, 0, h, thickness_, current_);
        break;
    case 0x03:
        playSound(22);
        drawLine(x, y, x, h, thickness_, current_);
        drawLine(x, y, x, 0, thickness_, current_);
        drawLine(x, y, 0, y, thickness_, current_);
        drawLine(x, y, w, y, thickness_, current_);
        break;
    case 0x05: case 0x14: case 0x1d: case 0x1e:  // bands with thickness 2 (rainbow) or 10 (shades)
        if (mode_ == 0x05) playSound(15);
        w0ffe = thickness_;
        thickness_ = mode_ == 0x05 ? 2 : 10;
        setupBrushColours();
        brushBand(x, y, x + 1, y, w0ffc);
        break;
    case 0x08: case 0x0e: case 0x0f: case 0x15:
        playSound(mode_ == 0x08 ? 9 : mode_ == 0x0e ? 19 : mode_ == 0x0f ? 20 : 16);
        setupBrushColours();
        brushBand(x, y, x + 1, y, w0ffc);
        break;
    case 0x09: playSound(10), spray(x, y, thickness_, w0fec); break;
    case 0x0a:
        playSound(11);
        drawLine(x, y, x, y, 12, current_);
        spray(x, y, 1, 3);
        break;
    case 0x0b: playSound(12), mirrorLine(x + 1, y + 1, w0fe4, current_); break;
    case 0x0c: case 0x0d: playSound(mode_ == 0x0c ? 13 : 14), speedLine(x, y, x, y, current_); break;
    case 0x10: splatter(x, y, current_), playSound(23); break;
    case 0x11: drawLine(x, y, x + 1, y, thickness_, current_); break;
    case 0x13: playSound(18), burst(x, y, current_); break;
    case 0x16: playSound(2), drawLine(x, y, x + 1, y, thickness_, current_); break;
    case 0x04: playSound(4), grass(x, y, w0fee); break;
    case 0x06: fan(x, y, current_), playSound(7); break;
    case 0x07: playSound(8), stamp(x, y, w0ff4, current_); break;
    case 0x19: playSound(26), roller(x, y, w0ffa); break;
    case 0x1a: playSound(27), bubble(x, y, current_); break;
    case 0x1b: playSound(28), ball(x, y, current_); break;
    case 0x2a: ringBurst(x, y, w0ff8); break;
    case 0x51: textStart(x, y); break;
    case 0x33:  // place the chosen clip art (1008:0d5b)
        saveBackup();
        clip_ = Surface(), clipWork_ = Surface();
        loadBmp(app_.assets().files().read(chosen_), &clip_);
        loadBmp(app_.assets().files().read(chosen_), &clipWork_);
        placeClipArt(x, y);
        break;
    case 0x2f: dynamite(x, y); break;
    case 0x30: train(x, y); break;
    case 0x32: walker(x, y); break;
    case 0x38: playSound(6), arcBrush(x, y, thickness_, current_); break;
    case 0x1f: rubberShape(Shape::Rectangle, x, y, 0, 1); break;
    case 0x20: rubberShape(Shape::Rounded, x, y, 0, 1); break;
    case 0x21: rubberShape(Shape::Ellipse, x, y, 0, 1); break;
    case 0x22: rubberShape(Shape::Triangle, x, y, w0fe6, 1); break;
    case 0x23: rubberShape(Shape::Trapezoid, x, y, w0fe8, 1); break;
    case 0x24: rubberShape(Shape::Star, x, y, w0fea, 1); break;
    case 0x25: rubberShape(Shape::Octagon, x, y, 0, 1); break;
    case 0x28: rubberShape(Shape::Hexagon, x, y, 0, 1); break;
    case 0x29: rubberShape(Shape::Diamond, x, y, 0, 1); break;
    case 0x36: boxOutline(x, y); break;
    case 0x26: straightLine(x, y, 1); break;
    case 0x39: playSound(3), eraserSquare(x, y); break;
    case 0x3a: playSound(5), floodFill(x, y, fillStyle_); break;
    case 0x50: playSound(5), magicFill(x, y); break;
    case 0x17: playSound(24), drawLine(x, y, x + 1, y, thickness_, current_); break;
    case 0x18: playSound(25), drawLine(x, y, x + 1, y, thickness_, current_); break;
    case 0x27: unported("mode 0x27 press (DS:1af8 point list)"); break;
    case 0x2b: ring3D(x, y, 1); break;
    case 0x2c: shaded3D(1, x, y, 1); break;
    case 0x2d: shaded3D(2, x, y, 1); break;
    case 0x2e: shaded3D(3, x, y, 1); break;
    case 0x3b: tube3D(x, y, 1); break;
    case 0x3c: box3D(x, y, 1, 2, 5, 2); break;
    case 0x3d: playSound(29), smudge(x, y); break;
    case 0x3e: playSound(30), smear(x, y, 4); break;
    case 0x3f: case 0x40: case 0x45: case 0x46: case 0x47:
    case 0x4b: case 0x4c: case 0x4d: case 0x4e: selectBox(x, y); break;
    case 0x4f: break;
    case 0x41: resizeArea(x, y, 1, w0fdc); break;
    case 0x42: flipArea(x, y, 1, w0fd8); break;
    case 0x43: negativeArea(x, y, 1); break;
    case 0x44: replaceColour(x, y); break;
    case 0x48: case 0x49: moveSelection(x, y, lastX_, lastY_, 3); break;
    case 0x4a: moveSelection(x, y, lastX_, lastY_, w0fd8); break;
    default:
        if (mode_) unported(("press, mode " + hex(mode_)).c_str());
        break;
    }
}

void Studio::drag(int x, int y) {  // 1008:1644
    const COLORREF c = strokeColour();
    const int w = pictureWidth(), h = pictureHeight();
    switch (mode_) {
    case 0x01: drawLine(lastX_, lastY_, x, y, thickness_, c); break;
    case 0x02:
        drawLine(x, y, 0, 0, thickness_, c);
        drawLine(x, y, w, 0, thickness_, c);
        drawLine(x, y, w, h, thickness_, c);
        drawLine(x, y, 0, h, thickness_, c);
        break;
    case 0x05: case 0x08: case 0x0e: case 0x14: case 0x15: case 0x1d: case 0x1e:
        brushBand(lastX_, lastY_, x, y, w0ffc);
        break;
    case 0x0f: setupBrushColours(), brushBand(lastX_, lastY_, x, y, w0ffc); break;
    case 0x09: spray(x, y, thickness_, w0fec); break;
    case 0x0a:
        drawLine(lastX_, lastY_, x, y, 12, current_);
        spray(x, y, 1, 3);
        break;
    case 0x0b: mirrorLine(x, y, w0fe4, c); break;
    case 0x0c: case 0x0d: speedLine(lastX_, lastY_, x, y, current_); break;
    case 0x13: burst(x, y, c); break;
    case 0x16: dripSwipe(lastX_, lastY_, x, y); break;
    case 0x04: grass(x, y, w0fee); break;
    case 0x06: fan(x, y, c); break;
    case 0x07: stamp(x, y, w0ff4, c); break;
    case 0x11: beadLine(lastX_, lastY_, x, y, c); break;
    case 0x19: roller(x, y, w0ffa); break;
    case 0x1a: bubble(x, y, c); break;
    case 0x1b: ball(x, y, c); break;
    case 0x38:  // the clown picks a random shade of the current family
        arcBrush(x, y, thickness_, multiColour_ ? colour(family_ * 10 + random(10)) : current_);
        break;
    case 0x39: eraserSquare(x, y); break;
    case 0x1f: rubberShape(Shape::Rectangle, x, y, 0, 1); break;
    case 0x20: rubberShape(Shape::Rounded, x, y, 0, 1); break;
    case 0x21: rubberShape(Shape::Ellipse, x, y, 0, 1); break;
    case 0x22: rubberShape(Shape::Triangle, x, y, w0fe6, 1); break;
    case 0x23: rubberShape(Shape::Trapezoid, x, y, w0fe8, 1); break;
    case 0x24: rubberShape(Shape::Star, x, y, w0fea, 1); break;
    case 0x25: rubberShape(Shape::Octagon, x, y, 0, 1); break;
    case 0x28: rubberShape(Shape::Hexagon, x, y, 0, 1); break;
    case 0x29: rubberShape(Shape::Diamond, x, y, 0, 1); break;
    case 0x36: boxOutline(x, y); break;
    case 0x37: parallelogram(x, y, w0fe2, 1); break;
    case 0x26: straightLine(x, y, 1); break;
    case 0x33: placeClipArt(x, y); break;
    case 0x2b: ring3D(x, y, 1); break;
    case 0x2c: shaded3D(1, x, y, 1); break;
    case 0x2d: shaded3D(2, x, y, 1); break;
    case 0x2e: shaded3D(3, x, y, 1); break;
    case 0x3b: tube3D(x, y, 1); break;
    case 0x3c: box3D(x, y, 1, 2, 5, 2); break;
    case 0x3d: smudge(x, y); break;
    case 0x3e: smear(x, y, 4); break;
    case 0x3f: case 0x40: case 0x45: case 0x46: case 0x47:
    case 0x4b: case 0x4c: case 0x4d: case 0x4e: selectBox(x, y); break;
    case 0x4f: case 0x50: case 0x51: break;  // 1008:1683: outside the drag jump table (modes 1..0x4e)
    case 0x41: resizeArea(x, y, 1, w0fdc); break;
    case 0x42: flipArea(x, y, 1, w0fd8); break;
    case 0x43: negativeArea(x, y, 1); break;
    case 0x48: case 0x49: moveSelection(x, y, lastX_, lastY_, 3); break;
    case 0x4a: moveSelection(x, y, lastX_, lastY_, w0fd8); break;
    case 0x03:  // each line in a random one of the three shades (the first pick is unused)
        random(3);
        drawLine(x, y, x, h, thickness_, colour(shadeIndex_[random(3)]));
        drawLine(x, y, x, 0, thickness_, colour(shadeIndex_[random(3)]));
        drawLine(x, y, 0, y, thickness_, colour(shadeIndex_[random(3)]));
        drawLine(x, y, w, y, thickness_, colour(shadeIndex_[random(3)]));
        break;
    case 0x17: drawLine(startX_, startY_, x, y, thickness_, c); break;
    case 0x18: {  // a line and its mirror image through the starting point
        drawLine(startX_, startY_, x, y, thickness_, c);
        const int mx = x <= startX_ ? startX_ - x + startX_ : startX_ - (x - startX_);
        const int my = y <= startY_ ? startY_ - y + startY_ : startY_ - (y - startY_);
        drawLine(startX_, startY_, mx, my, thickness_, c);
        break;
    }
    default:
        if (mode_) unported(("drag, mode " + hex(mode_)).c_str());
        break;
    }
    lastX_ = x, lastY_ = y;
}

void Studio::release(int x, int y) {  // 1008:84bd
    const bool moved = x != startX_ && y != startY_;  // the shapes need a real drag
    switch (mode_) {
    case 0x1f: if (moved) rubberShape(Shape::Rectangle, x, y, 0, 2), playSound(31); break;
    case 0x20: if (moved) rubberShape(Shape::Rounded, x, y, 0, 2), playSound(32); break;
    case 0x21: if (moved) rubberShape(Shape::Ellipse, x, y, 0, 2), playSound(33); break;
    case 0x22: if (moved) rubberShape(Shape::Triangle, x, y, w0fe6, 2), playSound(34); break;
    case 0x23: if (moved) rubberShape(Shape::Trapezoid, x, y, w0fe8, 2), playSound(35); break;
    case 0x24: if (moved) rubberShape(Shape::Star, x, y, w0fea, 2), playSound(36); break;
    case 0x25: if (moved) rubberShape(Shape::Octagon, x, y, 0, 2), playSound(37); break;
    case 0x28: if (moved) rubberShape(Shape::Hexagon, x, y, 0, 2), playSound(41); break;
    case 0x29: if (moved) rubberShape(Shape::Diamond, x, y, 0, 2), playSound(42); break;
    case 0x26: if (x != startX_ || y != startY_) straightLine(x, y, 2), playSound(38); break;
    case 0x2b: if (moved) ring3D(x, y, 2), playSound(43); break;
    case 0x2c: if (moved) shaded3D(1, x, y, 2), playSound(44); break;
    case 0x2d: if (moved) shaded3D(2, x, y, 2), playSound(47); break;
    case 0x2e: if (moved) shaded3D(3, x, y, 2), playSound(45); break;
    case 0x3b: if (x != startX_ || y != startY_) playSound(39), tube3D(x, y, 2); break;
    case 0x3c: if (moved) playSound(46), box3D(x, y, 2, w0fe0, w0fde, w0fe0); break;
    // The Magic Hat (level 2's tool 9 = CRAYON1's 15)
    case 0x3f: if (moved) mosaic(), playSound(48); break;
    case 0x40: if (moved) randomMosaic(), playSound(49); break;
    case 0x41: if (moved) resizeArea(x, y, 2, w0fdc); break;
    case 0x42: if (moved) flipArea(x, y, 2, w0fd8), playSound(50); break;
    case 0x43: if (moved) negativeArea(x, y, 2), playSound(51); break;
    case 0x45: case 0x46: case 0x47:  // the box is chosen: mark it and wait for a drag (1008:8b6c)
        mode_ = mode_ == 0x45 ? 0x48 : mode_ == 0x46 ? 0x49 : 0x4a;
        eraseSelectBox();
        saveBackup();
        markSelection();
        b105b = true;
        break;
    case 0x48: case 0x49: case 0x4a:  // dropped (1008:8acc)
        playSound(mode_ == 0x48 ? 58 : mode_ == 0x49 ? 59 : 60);
        moveSelection(x, y, lastX_, lastY_, mode_ == 0x4a ? w0fd8 : 3);
        selectionPicture_ = Surface();
        mode_ = mode_ == 0x48 ? 0x45 : mode_ == 0x49 ? 0x46 : 0x47;
        b105b = false;
        break;
    case 0x4b: case 0x4c: case 0x4d: case 0x4e:  // 1008:84bd: Shadow Text ShowModal
        if (!moved) break;
        eraseSelectBox();
        saveBackup();
        screenCursor_ = 0;
        {
            const int selectedMode = mode_;
            auto dialog = std::make_unique<TextInputDialog>(app_, level_, &text_, [this](int n) { playSound(n); });
            dialog->onClose = [this, x, y, selectedMode] {
                restoreBackup();
                playSound(selectedMode == 0x4b ? 53 : selectedMode == 0x4c ? 52 : selectedMode == 0x4d ? 54 : 55);
                shadowBoxText(x, y);
            };
            showDialog(std::move(dialog));
        }
        break;
    case 0x4f: {  // 1008:84bd: rotation dialog, then 1008:43c0
        saveBackup();
        screenCursor_ = 0;
        auto dialog = std::make_unique<RotatorDialog>(app_, level_, &text_, &w1aa6, current_,
                                                       [this](int n) { playSound(n); });
        dialog->onClose = [this, x, y] { restoreBackup(); shadowRotatedText(x, y); };
        showDialog(std::move(dialog));
        break;
    }
    case 0x36: mode_ = 0x37, drawing_ = true; break;  // now moving sets the slant
    case 0x37:
        playSound(56);
        parallelogram(x, y, w0fe2, 2);
        mode_ = 0x36, drawing_ = false;
        break;
    case 0x05: case 0x14: case 0x1d: case 0x1e: thickness_ = w0ffe; break;
    case 0x2f: mode_ = 0; break;
    case 0x33:
        if (!b1004) {
            playSound(57);
            placeClipArt(x, y);
            clip_ = Surface(), clipWork_ = Surface();
        } else {
            b1004 = false;
        }
        break;
    default: {
        // the other modes with a release case (1008:8423); everything else does nothing
        static const std::set<int> kReleaseCases = {0x27};
        if (kReleaseCases.count(mode_)) unported(("release, mode " + hex(mode_)).c_str());
        break;
    }
    }
}

void Studio::drawLine(int x1, int y1, int x2, int y2, int width, COLORREF c) {  // 1028:1a1f
    PenIn pen(pictureDC(), width, c);
    line(pictureDC(), x1, y1, x2, y2);
}

void Studio::updateShadeIndexes() {  // 1028:2076
    shadeIndex_[0] = family_ * 10 + std::max(shade_, 2);
    shadeIndex_[1] = shadeIndex_[0] - 1;
    shadeIndex_[2] = shadeIndex_[0] - 2;
}

void Studio::setupBrushColours() {  // 1028:2da9: the colours of the brush band's lines, by mode
    const int n = thickness_ * 3;
    const int base = family_ * 10;
    switch (mode_) {
    case 0x08:  // random colours
        for (int i = 0; i <= n; ++i) brushColours_[i] = colour(random(160));
        break;
    case 0x05:  // the rainbow marker: red, green, blue (thickness is forced to 2)
        brushColours_[0] = brushColours_[1] = colour(95);
        brushColours_[2] = brushColours_[3] = colour(115);
        brushColours_[4] = brushColours_[5] = colour(30);
        break;
    case 0x0e:
        for (int i = 0; i <= n; ++i) brushColours_[i] = current_;
        break;
    case 0x0f: case 0x15:  // random among the current shade and the two lighter ones
        for (int i = 0; i <= n; ++i) brushColours_[i] = colour(shadeIndex_[random(3)]);
        break;
    case 0x14: {  // dark to light, then light to dark in pairs
        for (int i = 0, k = 9; i <= 9; ++i, --k) brushColours_[i] = colour(base + k);
        for (int i = 10, pair = 0, k = 0; i <= 29; ++i) {
            brushColours_[i] = colour(base + k);
            if (++pair == 2) pair = 0, ++k;
        }
        break;
    }
    case 0x1d:  // dark to light in threes
        for (int i = 0, three = 0, k = 9; i <= 29; ++i) {
            brushColours_[i] = colour(base + k);
            if (++three == 3) three = 0, --k;
        }
        break;
    case 0x1e:  // light to dark in threes
        for (int i = 0, three = 0, k = 0; i <= 29; ++i) {
            brushColours_[i] = colour(base + k);
            if (++three == 3) three = 0, ++k;
        }
        break;
    }
}

void Studio::brushBand(int x1, int y1, int x2, int y2, int direction) {  // 1028:308e
    // thickness·3 parallel 2-pixel lines in the brush colours, offset along a diagonal (1, 2),
    // vertically (4) or horizontally (3).
    HDC dc = pictureDC();
    const int n = thickness_ * 3 - 1;
    int dx = 0, dy = 0;
    if (direction == 1) dy = n / 2, dx = -(n / 2);
    else if (direction == 2) dy = -(n / 2), dx = -(n / 2);
    else if (direction == 4) dy = -(n / 2), dx = 0;
    else if (direction == 3) dy = 0, dx = -(n / 2);
    for (int i = 0; i <= n; ++i) {
        PenIn pen(dc, 2, brushColours_[i & 63]);
        line(dc, x1 + dx, y1 + dy, x2 + dx, y2 + dy);
        if (direction == 1) --dy, ++dx;
        else if (direction == 2) ++dy, ++dx;
        else if (direction == 4) ++dy;
        else if (direction == 3) ++dx;
    }
}

void Studio::spray(int x, int y, int width, int kind) {  // 1028:2adf
    HDC dc = pictureDC();
    const int count = width < 2 ? 20 : width * 10;
    const int r = width * 8;
    if (kind == 1) {  // fine dots in the current colour
        for (int i = 0; i <= count; ++i) {
            const int dx = random(r * 2) - r, dy = random(r * 2) - r;
            if (distance(x, y, x + dx, y + dy) <= r) SetPixel(dc, x + dx, y + dy, current_);
        }
        return;
    }
    for (int k = 0; k < 3; ++k) {  // pellets in the current shade and the two lighter ones
        PenIn pen(dc, width, shade(k));
        for (int i = 0; i <= count / 3; ++i) {
            const int dx = random(r * 2) - r, dy = random(r * 2) - r;
            if (distance(x, y, x + dx, y + dy) > r) continue;
            if (width < 2) line(dc, x + dx, y + dy, x + dx + 1, y + dy + 1);
            else line(dc, x + dx, y + dy, x + dx, y + dy);
        }
    }
}

void Studio::mirrorLine(int x, int y, int kind, COLORREF c) {  // 1028:3274
    // The stroke and its mirror images across the picture's centre lines: 2 = left/right,
    // 1 = top/bottom, 3 = both and the point reflection.
    HDC dc = pictureDC();
    PenIn pen(dc, thickness_, c);
    line(dc, lastX_, lastY_, x, y);
    const int cx = pictureWidth() / 2, cy = pictureHeight() / 2;
    auto mirror = [](int v, int centre) { return centre + (v > centre ? -std::abs(v - centre) : std::abs(v - centre)); };
    const int lastXm = mirror(lastX_, cx), xm = mirror(x, cx), lastYm = mirror(lastY_, cy), ym = mirror(y, cy);
    if (kind == 2) {
        line(dc, xm, y, lastXm, lastY_);
    } else if (kind == 1) {
        line(dc, x, ym, lastX_, lastYm);
    } else if (kind == 3) {
        line(dc, xm, y, lastXm, lastY_);
        line(dc, x, ym, lastX_, lastYm);
        line(dc, xm, ym, lastXm, lastYm);
    }
}

void Studio::speedLine(int x1, int y1, int x2, int y2, COLORREF c) {  // 1028:3456
    const int speed = (std::abs(x2 - x1) + std::abs(y2 - y1)) / 2;
    const bool slowIsThick = mode_ == 0x0c;
    int width;
    if (speed >= 0 && speed <= 3) width = slowIsThick ? 8 : 2;
    else if (speed >= 4 && speed <= 8) width = slowIsThick ? 6 : 4;
    else if (speed >= 9 && speed <= 15) width = slowIsThick ? 4 : 6;
    else width = slowIsThick ? 2 : 8;  // (above 100 the original used an uninitialised width)
    PenIn pen(pictureDC(), width, c);
    line(pictureDC(), x1, y1, x2, y2);
}

void Studio::splatter(int x, int y, COLORREF c) {  // 1028:35b7
    HDC dc = pictureDC();
    PenIn pen(dc, thickness_, c);
    BrushIn brush(dc, c);
    const int t = thickness_, spread = t * 10;
    const int n = random(30) + 5;
    for (int i = 0; i <= n; ++i) {
        const int px = x + (random(t * 20) - spread);
        const int py = y + (random(t * 20) - spread);
        const int rx = random(t), ry = random(t);
        line(dc, x, y, px, py);
        Ellipse(dc, px - (rx + 1), py - (ry + 1), px + rx + 1, py + ry + 1);
    }
    const int rx = random(t * 4), ry = random(t * 4);
    Ellipse(dc, x - (rx + 1), y - (ry + 1), x + rx + 1, y + ry + 1);
}

void Studio::burst(int x, int y, COLORREF c) {  // 1028:3875
    (void)c;  // the original computes a family from it and never uses it
    HDC dc = pictureDC();
    for (int k : {0, 2, 1}) {  // pens: current shade, then the lightest, then the middle one
        PenIn pen(dc, 1, shade(k));
        const int n = random(50) + 20;
        for (int i = 0; i <= n; ++i) {
            const int dx = random(60) - 30, dy = random(60) - 30;
            if (distance(x, y, x + dx, y + dy) <= thickness_ * 6) line(dc, x, y, x + dx, y + dy);
        }
    }
}

void Studio::dripSwipe(int x1, int y1, int x2, int y2) {  // 1028:3b40: paint with the odd drip
    const int dripY = y2 + random(thickness_ * 10) + 10;
    drawLine(x1, y1, x2, y2, thickness_, current_);
    if (random(10) >= 2) return;
    drawLine(x2, y2, x2, dripY, 1, shade(0));
    drawLine(x2 + 1, y2, x2 + 1, dripY, 1, shade(1));
    drawLine(x2 + 2, y2, x2 + 2, dripY, 1, shade(2));
    HDC dc = pictureDC();
    const int cx = x2 + 1;
    int rx = random(thickness_) + 5, ry = random(thickness_) + 5;
    for (int k = 0; k < 3; ++k) {  // the drop: three shrinking ellipses, dark to light
        PenIn pen(dc, 1, shade(k));
        BrushIn brush(dc, shade(k));
        if (k == 1) rx -= rx / 4, ry -= ry / 4;
        if (k == 2) rx -= rx / 2, ry -= ry / 2;
        Ellipse(dc, cx - rx, dripY - ry, cx + rx, dripY + ry);
    }
}

void Studio::shadedShape(int x, int y, int rx, int ry, int shape, int light, int family) {  // 1028:20f0
    // Ten shapes from the family's darkest shade to its lightest, each inset per side by
    // i·radius/k, where the divisors (11 = centred, 14/8 = towards/away) place the highlight in
    // one of nine directions. Single-precision maths and Round (to nearest even) as in Delphi.
    static const float kDivisors[9][4] = {  // left, right, top, bottom; light 1..9
        {11, 11, 11, 11}, {14, 8, 14, 8}, {11, 11, 14, 8}, {8, 14, 14, 8}, {14, 8, 8, 14},
        {11, 11, 8, 14},  {8, 14, 8, 14}, {8, 14, 11, 11}, {14, 8, 11, 11}};
    const float left = float(x - rx), top = float(y - ry), right = float(x + rx), bottom = float(y + ry);
    float d[4] = {0, 0, 0, 0};
    if (light >= 1 && light <= 9) {
        const float* k = kDivisors[light - 1];
        d[0] = (float(x) - left) / k[0];
        d[1] = (right - float(x)) / k[1];
        d[2] = (float(y) - top) / k[2];
        d[3] = (bottom - float(y)) / k[3];
    }
    auto round = [](float v) { return int(std::nearbyint(v)); };
    HDC dc = pictureDC();
    for (int i = 0; i <= 9; ++i) {
        const COLORREF c = colour(family * 10 + 9 - i);
        PenIn pen(dc, 1, c);
        BrushIn brush(dc, c);
        const float fi = float(i);
        const int l = round(left) + round(fi * d[0]);
        const int r = round(right) - round(fi * d[1]);
        const int t = round(top) + round(fi * d[2]);
        const int b = round(bottom) - round(fi * d[3]);
        if (shape == 1) Ellipse(dc, l, t, r, b);
        else if (shape == 2) Rectangle(dc, l, t, r, b);
        else if (shape == 3) RoundRect(dc, l, t, r, b, std::abs(l - r) / 2, std::abs(t - b) / 2);
    }
}

void Studio::grass(int x, int y, int length) {  // 1028:2786
    const int n = random(6);
    int height = 0, spread = 0;
    if (length == 1) height = 15, spread = 20;
    else if (length == 2) height = 25, spread = 35;
    else if (length == 3) height = 45, spread = 55;
    for (int i = 0; i <= n + 3; ++i) {
        const int dx = random(spread) - spread / 2;
        const int dy = random(height);
        drawLine(x, y, x + dx, y - dy, 1, colour(shadeIndex_[random(3)]));
    }
}

void Studio::fan(int x, int y, COLORREF c) {  // 1028:285a
    const int spread = thickness_ * 10;
    for (int i = 0; i <= 7; ++i) {
        const int dx = random(thickness_ * 20) - spread, dy = random(thickness_ * 20) - spread;
        drawLine(x, y, x + dx, y + dy, thickness_, c);
    }
}

void Studio::stamp(int x, int y, int kind, COLORREF c) {  // 1028:28df: square, circle or triangle
    HDC dc = pictureDC();
    PenIn pen(dc, 1, c);
    BrushIn brush(dc, c);
    const int r = random(20) + 5;
    const int variant = random(3) + 1;
    if (kind == 1) {
        Rectangle(dc, x - r, y - r, x + r, y + r);
    } else if (kind == 2) {
        Ellipse(dc, x - r, y - r, x + r, y + r);
    } else if (kind == 3) {
        POINT p[3] = {};
        if (variant == 1) p[0] = {x, y - r}, p[1] = {x - r, y + r}, p[2] = {x + r, y + r};
        else if (variant == 2) p[0] = {x - r, y - r}, p[1] = {x + r, y + r}, p[2] = {x - r, y + r};
        else p[0] = {x - r, y - r}, p[1] = {x - r, y + r}, p[2] = {x + r, y};
        Polygon(dc, p, 3);
    }
}

void Studio::roller(int x, int y, int direction) {  // 1028:3ec1: a paint roller print with speckles
    HDC dc = pictureDC();
    PenIn pen(dc, 1, current_);
    BrushIn brush(dc, current_);
    const int halfWidth = direction == 3 ? thickness_ * 3 : thickness_ * 6;
    const int halfHeight = direction == 3 ? thickness_ * 6 : thickness_ * 3;
    Rectangle(dc, x - halfWidth, y - halfHeight, x + halfWidth, y + halfHeight);
    for (int i = 0; i <= thickness_ * 30; ++i) {
        for (int k = 1; k <= 2; ++k) {
            const int dx = random(halfWidth * 2) - halfWidth, dy = random(halfHeight * 2) - halfHeight;
            SetPixel(dc, x + dx, y + dy, shade(k));
        }
    }
}

void Studio::bubble(int x, int y, COLORREF c) {  // 1028:40de
    HDC dc = pictureDC();
    PenIn pen(dc, 1, c);
    HGDIOBJ old = SelectObject(dc, GetStockObject(NULL_BRUSH));  // LOGBRUSH style BS_NULL
    const int r = random(10) + 5;
    const int cx = x + random(30) - 15, cy = y + random(30) - 15;
    Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(dc, old);
}

void Studio::ball(int x, int y, COLORREF c) {  // 1028:41fa
    const int dx = random(30) - 15, dy = random(30) - 15, r = random(15) + 3;
    const int family = c == current_ ? family_ : random(15);
    shadedShape(x + dx, y + dy, r, r, 1, 5, family);
}

void Studio::ringBurst(int x, int y, int kind) {  // 1028:4277: rings or squares in random colours
    HDC dc = pictureDC();
    HPEN pens[101];
    for (HPEN& p : pens) p = CreatePen(PS_SOLID, 2, colour(random(160)));
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    HGDIOBJ oldPen = nullptr;
    const int n = random(60) + 30;
    for (int i = 1; i <= n; ++i) {
        HGDIOBJ previous = SelectObject(dc, pens[random(100)]);
        if (!oldPen) oldPen = previous;
        if (kind == 1) Ellipse(dc, x - i, y - i, x + i, y + i);
        else if (kind == 2) Rectangle(dc, x - i, y - i, x + i, y + i);
    }
    playSound(kind == 1 ? 77 : 78);
    if (oldPen) SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    for (HPEN p : pens) DeleteObject(p);
}

void Studio::beadLine(int x1, int y1, int x2, int y2, COLORREF c) {  // 1028:379b
    // A line that drops a hanging shaded ball every 40 pixels.
    drawLine(x1, y1, x2, y2, thickness_, current_);
    if (distance(x2, y2, startX_, startY_) <= 40) return;
    drawLine(x2, y2, x2, y2 + thickness_ * 5, thickness_, current_);
    const int family = c == current_ ? family_ : random(16);
    playSound(17);
    shadedShape(x2, y2 + thickness_ * 5, thickness_ * 3, thickness_ * 3, 1, 1, family);
    startX_ = x2, startY_ = y2;
}

void Studio::arcBrush(int x, int y, int width, COLORREF c) {  // 1028:1ac0: random arcs
    PenIn pen(pictureDC(), width, c);
    const int n = random(width * 5) + 2;
    auto offset = [&] {
        int r = random(n);
        return r % 2 == 0 ? -r : r;
    };
    const int sx = x + offset(), sy = y + offset(), ex = x + offset(), ey = y + offset();
    Arc(pictureDC(), x - n, y - n, x + n, y + n, sx, sy, ex, ey);
}

void Studio::eraserSquare(int x, int y) {  // 1028:1c5b: a 20x20 square of white (palette 60)
    HDC dc = pictureDC();
    PenIn pen(dc, 1, colour(60));
    BrushIn brush(dc, colour(60));
    Rectangle(dc, x - 10, y - 10, x + 10, y + 10);
}

HBRUSH Studio::fillBrush(COLORREF c, int style) {  // 1028:174f
    static const int kHatches[] = {HS_HORIZONTAL, HS_VERTICAL, HS_CROSS, HS_BDIAGONAL, HS_FDIAGONAL, HS_DIAGCROSS};
    if (style == 0) return static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    if (style == 1) return CreateSolidBrush(c);
    if (style >= 2 && style <= 7) return CreateHatchBrush(kHatches[style - 2], c);
    if (style >= 8 && style <= 15) {  // pa00–pa07: the pattern's black pixels take the colour
        Surface pattern;
        if (!loadBmp(app_.assets().files().read("pa0" + std::to_string(style - 8) + ".asl"), &pattern))
            return CreateSolidBrush(c);
        for (int py = 0; py <= 7; ++py)
            for (int px = 0; px <= 7; ++px)
                if (GetPixel(pattern.dc(), px, py) == RGB(0, 0, 0)) SetPixel(pattern.dc(), px, py, c);
        GdiFlush();
        // CreatePatternBrush needs a bitmap that outlives the Surface: copy it into a DDB.
        HDC screen = GetDC(nullptr);
        HBITMAP bitmap = CreateCompatibleBitmap(screen, 8, 8);
        HDC mem = CreateCompatibleDC(screen);
        HGDIOBJ old = SelectObject(mem, bitmap);
        BitBlt(mem, 0, 0, 8, 8, pattern.dc(), 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteDC(mem);
        ReleaseDC(nullptr, screen);
        HBRUSH brush = CreatePatternBrush(bitmap);
        DeleteObject(bitmap);
        return brush;
    }
    return CreateSolidBrush(c);
}

void Studio::floodFill(int x, int y, int style) {  // 1028:7170: the paint can
    HDC dc = pictureDC();
    const COLORREF clicked = GetPixel(dc, x, y);
    HBRUSH brush = fillBrush(current_, style);
    if (style < 8) SetBkMode(dc, TRANSPARENT);
    HGDIOBJ old = SelectObject(dc, brush);
    const COLORREF unchanged = b1059 ? colour(69) : current_;
    if (clicked != unchanged) ExtFloodFill(dc, x, y, clicked, FLOODFILLSURFACE);
    SetBkMode(dc, OPAQUE);
    SelectObject(dc, old);
    DeleteObject(brush);
}

std::vector<POINT> Studio::shapePoints(Shape shape, int kind, int x, int y) const {
    // Corner points from the stroke's start (DS:1ace/1ad0) to (x, y); "half" and "fifth" are
    // truncated towards zero, as the original's |d|/n with the sign put back.
    const int sx = startX_, sy = startY_;
    if (shape == Shape::Triangle) {  // 1028:57ea
        if (kind == 1) return {{sx, y}, {x, y}, {sx + (x - sx) / 2, sy}};
        if (kind == 2) return {{sx, sy}, {x, y}, {sx, y}};
        if (kind == 3) return {{sx, sy}, {sx, y}, {x, sy + (y - sy) / 2}};
        return {};
    }
    if (shape == Shape::Trapezoid) {  // 1028:5e1e
        if (kind == 1) {
            const int a = (x - sx) / 5;
            return {{sx, y}, {x, y}, {sx + 4 * a, sy}, {sx + a, sy}};
        }
        if (kind == 2) {
            const int a = (y - sy) / 5;
            return {{sx, sy}, {sx, y}, {x, sy + 4 * a}, {x, sy + a}};
        }
        return {};
    }
    if (shape == Shape::Diamond) {  // 1010:218c
        const int mx = sx + (x - sx) / 2, my = sy + (y - sy) / 2;
        return {{sx, my}, {mx, y}, {x, my}, {mx, sy}};
    }
    if (shape == Shape::Hexagon) {  // 1010:1b93: Single maths, Round to nearest even
        const float f = float(std::abs(sx - x)) / 5.0f, g = float(std::abs(y - sy)) / 2.0f;
        const int a = int(std::nearbyint(f)), b = int(std::nearbyint(4.0f * f)), c = int(std::nearbyint(g));
        const int x1 = x < sx ? sx - a : sx + a, x4 = x < sx ? sx - b : sx + b;
        const int my = y < sy ? sy - c : sy + c;
        return {{sx, my}, {x1, y}, {x4, y}, {x, my}, {x4, sy}, {x1, sy}};
    }
    if (shape == Shape::Octagon) {  // 1010:11df: thirds
        const int a = (x - sx) / 3, b = (y - sy) / 3;
        const int x1 = sx + a, x2 = sx + 2 * a, y1 = sy + b, y2 = sy + 2 * b;
        return {{sx, y1}, {sx, y2}, {x1, y}, {x2, y}, {x, y2}, {x, y1}, {x2, sy}, {x1, sy}};
    }
    if (shape == Shape::Star) {  // 1010:09e6: four points, upright (1) or leaning (2)
        const int ax = std::abs(sx - x) / 10, hx = std::abs(sx - x) / 2;
        const int ay = std::abs(sy - y) / 10, hy = std::abs(sy - y) / 2;
        const int cx = x < sx ? sx - hx : sx + hx, cy = y < sy ? sy - hy : sy + hy;
        const int nearX = x < sx ? cx + ax : cx - ax, farX = x < sx ? cx - ax : cx + ax;
        const int nearY = y < sy ? cy + ay : cy - ay, farY = y < sy ? cy - ay : cy + ay;
        if (kind == 1)
            return {{sx, cy}, {nearX, farY}, {cx, y}, {farX, farY}, {x, cy}, {farX, nearY}, {cx, sy}, {nearX, nearY}};
        if (kind == 2)
            return {{sx, sy}, {nearX, cy}, {sx, y}, {cx, farY}, {x, y}, {farX, cy}, {x, sy}, {cx, nearY}};
        return {};
    }
    return {{sx, sy}, {x, y}};
}

void Studio::rubberShape(Shape shape, int x, int y, int kind, int phase) {
    HDC dc = pictureDC();
    auto draw = [&](int px, int py, bool fill) {
        if (shape == Shape::Rectangle) {
            Rectangle(dc, startX_, startY_, px, py);
        } else if (shape == Shape::Ellipse) {
            Ellipse(dc, startX_, startY_, px, py);
        } else {
            const std::vector<POINT> p = shapePoints(shape, kind, px, py);
            if (p.empty()) return;
            if (fill) {
                Polygon(dc, p.data(), int(p.size()));
            } else {
                for (size_t i = 0; i < p.size(); ++i) line(dc, p[i].x, p[i].y, p[(i + 1) % p.size()].x, p[(i + 1) % p.size()].y);
            }
        }
    };
    auto roundRect = [&](int px, int py) {
        int rw = std::abs(startX_ - px), rh = std::abs(startY_ - py);
        if (rw) rw /= 4;
        if (rh) rh /= 4;
        RoundRect(dc, startX_, startY_, px, py, rw, rh);
    };
    auto outline = [&](int px, int py) {
        if (shape == Shape::Rounded) roundRect(px, py);
        else draw(px, py, false);
    };
    // Erase the previous rubber band: XOR white, hollow (R2_XORPEN = 7).
    SetROP2(dc, R2_XORPEN);
    {
        PenIn pen(dc, 1, colour(60));
        HGDIOBJ old = SelectObject(dc, GetStockObject(NULL_BRUSH));
        outline(lastX_, lastY_);
        SelectObject(dc, old);
    }
    const COLORREF c = phase == 1 ? colour(60) : current_;
    const int width = phase == 1 ? 1 : thickness_;
    if (phase == 2) {
        if (shape != Shape::Rectangle && shape != Shape::Rounded && shape != Shape::Ellipse) {
            const std::vector<POINT> p = shapePoints(shape, kind, x, y);
            pointCount_ = int(p.size());
            for (size_t i = 0; i < p.size(); ++i) points_[i] = p[i];
        }
        if (fillStyle_ < 16) {
            SetROP2(dc, R2_COPYPEN);
            PenIn pen(dc, width, c);
            HBRUSH brush = fillBrush(c, fillStyle_);
            if (fillStyle_ < 8) SetBkMode(dc, TRANSPARENT);
            HGDIOBJ old = SelectObject(dc, brush);
            if (shape == Shape::Rounded) roundRect(x, y);
            else draw(x, y, true);
            SetBkMode(dc, OPAQUE);
            SelectObject(dc, old);
            DeleteObject(brush);
        }
    } else {
        PenIn pen(dc, width, c);
        HGDIOBJ old = SelectObject(dc, GetStockObject(NULL_BRUSH));
        outline(x, y);
        SelectObject(dc, old);
    }
    SetROP2(dc, R2_COPYPEN);
    if (phase == 2 && fillStyle_ >= 16) {
        static const int kShapeIds[] = {0x1f, 0x20, 0x21, 0x22, 0x23, 0x29, 0x28, 0x25, 0x24};  // = the modes
        int rw = 0, rh = 0;
        if (shape == Shape::Rounded) {
            rw = std::abs(startX_ - x) / 4, rh = std::abs(startY_ - y) / 4;
        }
        shadedFill(kShapeIds[int(shape)], startX_, startY_, x, y, rw, rh);
    }
}

void Studio::shadedFill(int shapeId, int x1, int y1, int x2, int y2, int rw, int rh) {
    // 1028:48ae (styles 16–23, shaded) and 1028:44c6 (24–31, textures): the shape's region clips
    // a fill of its bounding box. The callers pass the stroke's start and end (the parallelogram
    // its points' bounding box) and the rounded rectangle its corner ellipse.
    HDC dc = pictureDC();
    const int left = std::min(x1, x2), right = std::max(x1, x2), top = std::min(y1, y2), bottom = std::max(y1, y2);
    HRGN region = nullptr;
    switch (shapeId) {
    case 0x1f: region = CreateRectRgn(left, top, right, bottom); break;
    case 0x20: region = CreateRoundRectRgn(left, top, right, bottom, rw, rh); break;
    case 0x21: region = CreateEllipticRgn(left, top, right, bottom); break;
    case 0x22: case 0x23: case 0x24: case 0x25: case 0x27: case 0x28: case 0x29: case 0x36:
        region = CreatePolygonRgn(points_, pointCount_, ALTERNATE);  // DS:1af8, DS:1aba
        break;
    default: return;  // the original selects an uninitialised handle
    }
    SelectClipRgn(dc, region);
    if (fillStyle_ < 24) {  // 1028:48ae: ten bands of 2-px lines, one per shade of the family
        // style 16..23 -> direction (1 vertical lines left to right, 2 horizontal lines top to
        // bottom, 3 and 4 the two diagonals) and whether the bands go from dark to light
        static const struct { int direction; bool darkFirst; } kStyles[] = {
            {1, true}, {1, false}, {2, true}, {2, false}, {3, true}, {4, false}, {4, true}, {3, false}};
        const auto style = kStyles[fillStyle_ - 16];
        const int width = right - left, height = bottom - top;
        const int span = style.direction == 1 ? width : style.direction == 2 ? height : width + height;
        const float step = float(span) / 10.0f;  // Single; i·step is rounded in extended precision
        for (int i = 0; i < 10; ++i) {
            PenIn pen(dc, 2, colour(family_ * 10 + (style.darkFirst ? 9 - i : i)));
            const int from = int(std::nearbyint(double(i) * step)), to = int(std::nearbyint(double(i + 1) * step));
            const int origin = style.direction == 2 ? top : left;
            for (int v = origin + from; v <= origin + to; ++v) {
                switch (style.direction) {
                case 1: line(dc, v, top, v, bottom); break;
                case 2: line(dc, left, v, right, v); break;
                case 3: line(dc, v, top, v - height, bottom); break;
                case 4: line(dc, v, bottom, v - height, top); break;
                }
            }
        }
    } else {  // 1028:44c6: pb1..pb8 tiled (54x54) from the top-left corner
        const Surface& tile = app_.assets().bitmap(("pb" + std::to_string(fillStyle_ - 23) + ".asl").c_str());
        const int columns = (right - left) / 54 + 1, rows = (bottom - top) / 54 + 1;
        for (int row = 0; row <= rows; ++row)
            for (int column = 0; column <= columns; ++column)
                BitBlt(dc, left + column * 54, top + row * 54, 54, 54, tile.dc(), 0, 0, SRCCOPY);
    }
    SelectClipRgn(dc, nullptr);
    DeleteObject(region);
}

namespace {
void xorRectangle(HDC dc, int x1, int y1, int x2, int y2) {  // 1028:1689: four lines
    line(dc, x1, y1, x2, y1);
    line(dc, x1, y1, x1, y2);
    line(dc, x1, y2, x2, y2);
    line(dc, x2, y1, x2, y2);
}
}  // namespace

void Studio::boxOutline(int x, int y) {  // 1028:6b79: first step of the parallelogram
    HDC dc = pictureDC();
    SetROP2(dc, R2_XORPEN);
    {
        PenIn pen(dc, 1, colour(60));
        xorRectangle(dc, startX_, startY_, lastX_, lastY_);
        xorRectangle(dc, startX_, startY_, x, y);
    }
    SetROP2(dc, R2_COPYPEN);
    // DS:1af8 = (dx, dy), 1afc = (x, y), 1b00 = the start
    points_[0] = {x - startX_, y - startY_};
    points_[1] = {x, y};
    points_[2] = {startX_, startY_};
}

void Studio::parallelogram(int x, int y, int direction, int phase) {  // 1028:6c74: second step
    const int dx = points_[0].x, dy = points_[0].y;
    const POINT corner = points_[1];
    startX_ = points_[2].x, startY_ = points_[2].y;
    const int sx = startX_, sy = startY_;
    HDC dc = pictureDC();
    SetROP2(dc, R2_XORPEN);
    {
        PenIn pen(dc, 1, colour(60));
        auto outline = [&](int px, int py) {
            if (direction == 3) {  // vertical sides
                line(dc, sx, sy, sx, sy + dy);
                line(dc, sx, sy + dy, px, py);
                line(dc, px, py, px, py - dy);
                line(dc, px, py - dy, sx, sy);
            } else {  // horizontal sides
                line(dc, sx, sy, sx + dx, sy);
                line(dc, sx + dx, sy, px, py);
                line(dc, px, py, px - dx, py);
                line(dc, px - dx, py, sx, sy);
            }
        };
        outline(lastX_, lastY_);
        if (phase == 1) outline(x, y);
        if (phase == 2) SetPixel(dc, corner.x, corner.y, colour(60));  // SetPixel ignores the XOR mode
    }
    SetROP2(dc, R2_COPYPEN);
    if (phase != 2) return;
    points_[0] = {sx, sy};
    points_[2] = {x, y};
    if (direction == 3) points_[1] = {sx, sy + dy}, points_[3] = {x, y - dy};
    else points_[1] = {sx + dx, sy}, points_[3] = {x - dx, y};
    pointCount_ = 4;
    if (fillStyle_ < 16) {
        PenIn pen(dc, thickness_, current_);
        HBRUSH brush = fillBrush(current_, fillStyle_);
        if (fillStyle_ < 8) SetBkMode(dc, TRANSPARENT);
        HGDIOBJ old = SelectObject(dc, brush);
        Polygon(dc, points_, 4);
        SetBkMode(dc, OPAQUE);
        SelectObject(dc, old);
        DeleteObject(brush);
    } else {
        int x1 = points_[0].x, y1 = points_[0].y, x2 = x1, y2 = y1;  // the four points' bounding box
        for (int i = 1; i < 4; ++i) {
            x1 = std::min<int>(x1, points_[i].x), x2 = std::max<int>(x2, points_[i].x);
            y1 = std::min<int>(y1, points_[i].y), y2 = std::max<int>(y2, points_[i].y);
        }
        shadedFill(0x36, x1, y1, x2, y2);
    }
}

void Studio::straightLine(int x, int y, int phase) {  // 1010:1775: a rubber-band straight line
    HDC dc = pictureDC();
    const COLORREF c = phase == 1 ? colour(60) : current_;
    SetROP2(dc, R2_XORPEN);
    {
        PenIn pen(dc, 1, c);
        line(dc, startX_, startY_, lastX_, lastY_);
    }
    if (phase == 2) {
        SetROP2(dc, R2_COPYPEN);
        PenIn pen(dc, thickness_, c);
        line(dc, startX_, startY_, x, y);
    } else {
        PenIn pen(dc, 1, c);
        line(dc, startX_, startY_, x, y);
    }
    SetROP2(dc, R2_COPYPEN);
}

HFONT Studio::textFont() {
    // Height = -Round(thickness·7 · PixelsPerInch / 72), PixelsPerInch = 96 (Screen, DS:6b58).
    const float scale = 96.0f / 72.0f;
    const int height = -int(std::nearbyint(float(thickness_ * 7) * scale));
    return CreateFontA(height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, font_.c_str());
}

void Studio::drawCaret() {  // 1008:9465
    HDC dc = pictureDC();
    PenIn pen(dc, 1, current_);
    BrushIn brush(dc, current_);
    Rectangle(dc, caret_.left, caret_.top, caret_.left + caret_.right, caret_.top + caret_.bottom);
}

void Studio::textStart(int x, int y) {  // 1008:9549
    saveBackup();
    textX_ = x, textY_ = y - 16;
    typing_ = true;
    text_.clear();
    caret_ = {textX_, textY_, thickness_, thickness_ * 9};
    drawCaret();
}

void Studio::textKey(uint8_t key) {  // 1008:95c4
    restoreBackup();
    HDC dc = pictureDC();
    HFONT font = textFont();
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, current_);
    bool finish = false;
    if (key == 8) {
        if (!text_.empty()) text_.pop_back();
    } else if (text_.size() > 253) {
        SelectObject(dc, old);
        DeleteObject(font);
        return;
    } else if (key == 13) {
        finish = true;
    } else {
        text_.push_back(char(key));
    }
    if (finish) {
        SelectObject(dc, old);
        DeleteObject(font);
        textFinish();
        return;
    }
    SIZE extent{};
    GetTextExtentPoint32A(dc, text_.c_str(), int(text_.size()), &extent);
    TextOutA(dc, textX_, textY_, text_.c_str(), int(text_.size()));
    SelectObject(dc, old);
    DeleteObject(font);
    caret_.left = textX_ + extent.cx;
    drawCaret();
}

void Studio::textFinish() {  // 1008:97cb
    restoreBackup();
    HDC dc = pictureDC();
    HFONT font = textFont();
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, current_);
    TextOutA(dc, textX_, textY_, text_.c_str(), int(text_.size()));
    SelectObject(dc, old);
    DeleteObject(font);
    typing_ = false;
}

void Studio::keyChar(wchar_t c) {  // FormKeyPress 1008:9951 (KeyPreview): Delphi 1 gets ANSI chars
    if (dialog_) {
        dialog_->keyChar(c);
        finishDialog();
        return;
    }
    if (!typing_) return;
    char ansi = 0;
    if (c < 0x80) ansi = char(c);
    else if (WideCharToMultiByte(1252, 0, &c, 1, &ansi, 1, nullptr, nullptr) != 1) return;
    textKey(uint8_t(ansi));
}

void Studio::placeClipArt(int x, int y) {  // 1010:2948: the picture (and its mask) follows the mouse
    int hw = 100, hh = 50, maskX = 0, maskY = 100;              // 4: wide, mask below
    if (clipOrientation_ == 3) hw = 50, hh = 100, maskX = 100, maskY = 0;  // tall, mask on the right
    else if (clipOrientation_ == 2) hw = 100, hh = 100, maskX = 200, maskY = 0;
    if (!clip_.valid() || !clipWork_.valid()) return;
    HDC dc = pictureDC();
    BitBlt(dc, lastX_ - hw, lastY_ - hh, 2 * hw, 2 * hh, backup_.dc(), lastX_ - hw, lastY_ - hh, SRCCOPY);
    BitBlt(clipWork_.dc(), 0, 0, 2 * hw, 2 * hh, backup_.dc(), x - hw, y - hh, SRCCOPY);
    BitBlt(clipWork_.dc(), 0, 0, 2 * hw, 2 * hh, clip_.dc(), maskX, maskY, 0x008800C6);  // SRCAND
    BitBlt(clipWork_.dc(), 0, 0, 2 * hw, 2 * hh, clip_.dc(), 0, 0, 0x00EE0086);          // SRCPAINT
    BitBlt(dc, x - hw, y - hh, 2 * hw, 2 * hh, clipWork_.dc(), 0, 0, SRCCOPY);
}

void Studio::magicFill(int x, int y) {  // 1028:79f2: reveal a Magic Color region
    // Near-white palette entry 160 + k (k = 0..47) stands for family k / 3, shade 3, 5 or 7.
    HDC dc = pictureDC();
    const COLORREF clicked = GetPixel(dc, x, y);
    int target = 0;
    for (int k = 0; k <= 47; ++k)
        if (colour(160 + k) == clicked) target = (k / 3) * 10 + 3 + 2 * (k % 3);
    HBRUSH brush = CreateSolidBrush(colour(target));
    HGDIOBJ old = SelectObject(dc, brush);
    if (target != 0) ExtFloodFill(dc, x, y, clicked, FLOODFILLSURFACE);
    SelectObject(dc, old);
    DeleteObject(brush);
}

// ---- The 3D tool (level 2; CRAYON1's unreachable tool 13) -----------------------------------

namespace {
struct HollowXor {  // the rubber bands: R2_XORPEN, a white 1-px pen and a hollow brush
    HDC dc;
    HPEN pen;
    HGDIOBJ oldPen, oldBrush;
    HollowXor(HDC d, COLORREF white) : dc(d), pen(CreatePen(PS_SOLID, 1, white)) {
        SetROP2(dc, R2_XORPEN);
        oldPen = SelectObject(dc, pen);
        oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    }
    ~HollowXor() {
        SelectObject(dc, oldPen), SelectObject(dc, oldBrush), DeleteObject(pen);
        SetROP2(dc, R2_COPYPEN);
    }
};
}  // namespace

void Studio::shaded3D(int shape, int x, int y, int phase) {
    // 1028:64b7 (1 ellipse), 1028:6869 (2 rectangle), 1028:6649 (3 rounded rectangle): an XOR
    // rubber band, then the shaded shape (1028:20f0) lit from DS:1abe over the dragged box.
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        auto outline = [&](int px, int py) {
            if (shape == 1) Ellipse(dc, startX_, startY_, px, py);
            else if (shape == 2) Rectangle(dc, startX_, startY_, px, py);
            else RoundRect(dc, startX_, startY_, px, py, std::abs(startX_ - px) / 2, std::abs(startY_ - py) / 2);
        };
        outline(lastX_, lastY_);
        if (phase == 1) outline(x, y);
    }
    if (phase != 2) return;
    const int rx = std::abs(x - startX_) / 2, ry = std::abs(y - startY_) / 2;
    const int cx = x > startX_ ? startX_ + rx : x + rx, cy = y > startY_ ? startY_ + ry : y + ry;
    shadedShape(cx, cy, rx, ry, shape, w1abe, family_);
}

void Studio::ring3D(int x, int y, int phase) {  // 1010:25db: a ring, dark at both edges
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        Ellipse(dc, startX_, startY_, lastX_, lastY_);
        if (phase != 2) Ellipse(dc, startX_, startY_, x, y);
    }
    if (phase != 2) return;
    HPEN pens[10];
    for (int i = 0; i < 10; ++i) pens[i] = CreatePen(PS_SOLID, 2, colour(family_ * 10 + i));
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    const int left = std::min(startX_, x), right = std::max(startX_, x);
    const int top = std::min(startY_, y), bottom = std::max(startY_, y);
    auto ring = [&](int pen, int d) {
        HGDIOBJ old = SelectObject(dc, pens[pen]);
        Ellipse(dc, left - d, top - d, right + d, bottom + d);
        SelectObject(dc, old);
    };
    for (int i = 0; i < 10; ++i) ring(9 - i, 19 - i);  // outside: darkest at the edge
    for (int i = 0; i < 10; ++i) ring(i, 9 - i);       // inside: darkest at the box
    SelectObject(dc, oldBrush);
    for (HPEN p : pens) DeleteObject(p);
}

void Studio::tube3D(int x, int y, int phase) {  // 1010:2d86: a rubber-band line drawn as a tube
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        line(dc, startX_, startY_, lastX_, lastY_);
        if (phase != 2) line(dc, startX_, startY_, x, y);
    }
    if (phase != 2) return;
    HPEN pens[10];
    for (int i = 0; i < 10; ++i) pens[i] = CreatePen(PS_SOLID, 2, colour(family_ * 10 + i));
    // Pairs of lines on both sides of the centre line, from the thickness + 1 inwards; the
    // shade steps from the darkest (9) by 3 (thickness 1), 2 (2 and 3) or 1.
    const int step = thickness_ == 1 ? 3 : thickness_ == 2 || thickness_ == 3 ? 2 : 1;
    int direction = 1;  // diagonal "\" (default), 2 "/", 3 horizontal, 4 vertical
    if ((x < startX_ && y < startY_) || (startX_ < x && startY_ < y)) direction = 2;
    if (y == startY_) direction = 3;
    if (x == startX_) direction = 4;
    int pen = 9, offset = thickness_ + 1;
    const int sx = startX_, sy = startY_;
    for (int i = 0; i <= thickness_ + 1; ++i) {
        HGDIOBJ old = SelectObject(dc, pens[pen]);
        const int o = offset;
        switch (direction) {
        case 1: line(dc, sx + o, sy + o, x + o, y + o), line(dc, sx - o, sy - o, x - o, y - o); break;
        case 2: line(dc, sx + o, sy - o, x + o, y - o), line(dc, sx - o, sy + o, x - o, y + o); break;
        case 3: line(dc, sx, sy + o, x, y + o), line(dc, sx, sy - o, x, y - o); break;
        case 4: line(dc, sx + o, sy, x + o, y), line(dc, sx - o, sy, x - o, y); break;
        }
        SelectObject(dc, old);
        if (pen > 0) pen -= step;
        --offset;
    }
    for (HPEN p : pens) DeleteObject(p);
}

void Studio::box3D(int x, int y, int phase, int front, int depth, int side) {
    // 1028:72c2: a box seen from a corner: the front face and two receding faces, each a shaded
    // fill (1028:48ae) clipped to its polygon (DS:1af8, 4 points); `front` (DS:0fe0: 2, 4, 5, 7)
    // picks the gradients, `side` the corner it recedes to, `depth` (DS:0fde) how far.
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        xorRectangle(dc, startX_, startY_, lastX_, lastY_);
        if (phase == 1) xorRectangle(dc, startX_, startY_, x, y);
    }
    if (phase == 1) return;
    const int savedStyle = fillStyle_;
    const int l = std::min(x, startX_), r = std::max(x, startX_), t = std::min(y, startY_), b = std::max(y, startY_);
    auto face = [&](std::initializer_list<POINT> points, int style, int x1, int y1, int x2, int y2) {
        pointCount_ = 4;
        int i = 0;
        for (const POINT& p : points) points_[i++] = p;
        if (style) fillStyle_ = style;
        shadedFill(0x27, x1, y1, x2, y2);
    };
    auto pick = [&](int s2, int s5, int s4, int s7) {  // DS:1ac4 by `front`; 0 keeps the current
        return front == 2 ? s2 : front == 5 ? s5 : front == 4 ? s4 : front == 7 ? s7 : 0;
    };
    face({{l, t}, {r, t}, {r, b}, {l, b}}, pick(0x17, 0x15, 0x16, 0x14), l, t, r, b);
    // depth · |d| / 10 (Single: the quotient is rounded to Single after the multiply)
    const int dx = int(std::nearbyint(float(double(std::abs(x - startX_)) / 10.0 * depth)));
    const int dy = int(std::nearbyint(float(double(std::abs(y - startY_)) / 10.0 * depth)));
    const int topStyle = pick(0x12, 0x13, 0x12, 0x13);
    if (side == 2) face({{l - dx, t - dy}, {r - dx, t - dy}, {r, t}, {l, t}}, topStyle, l - dx, t - dy, r, t);
    else if (side == 5) face({{l - dx, b + dy}, {r - dx, b + dy}, {r, b}, {l, b}}, topStyle, l - dx, b, r, b + dy);
    else if (side == 4) face({{r + dx, t - dy}, {r, t}, {l, t}, {l + dx, t - dy}}, topStyle, l, t - dy, r + dx, t);
    else if (side == 7) face({{l + dx, b + dy}, {r + dx, b + dy}, {r, b}, {l, b}}, topStyle, l, b, r + dx, b + dy);
    if (side == 2) face({{l - dx, t - dy}, {l - dx, b - dy}, {l, b}, {l, t}}, pick(0x10, 0x12, 0x11, 0x11), l - dx, t - dy, l, b);
    else if (side == 5) face({{l - dx, t + dy}, {l - dx, b + dy}, {l, b}, {l, t}}, pick(0x10, 0x10, 0x11, 0x10), l - dx, t, l, b + dy);
    else if (side == 4) face({{r + dx, t - dy}, {r + dx, b - dy}, {r, b}, {r, t}}, pick(0x11, 0x10, 0x11, 0x11), r, t - dy, r + dx, b);
    else if (side == 7) face({{r, t}, {r + dx, t + dy}, {r + dx, b + dy}, {r, b}}, pick(0x11, 0x10, 0x10, 0x11), r, t, r + dx, b + dy);
    fillStyle_ = savedStyle;
}

namespace {
int smudgeRadius(int thickness) {  // 1028:1d5e / 1f31: thickness 1..10
    static const int kRadius[] = {2, 3, 4, 5, 6, 8, 10, 12, 14, 16};
    return thickness >= 1 && thickness <= 10 ? kRadius[thickness - 1] : 0;
}
}  // namespace

void Studio::smudge(int x, int y) {  // 1028:1d5e: swap thickness·10 + 1 random pairs of pixels
    HDC dc = pictureDC();
    const int r = smudgeRadius(thickness_);
    for (int i = 0; i <= thickness_ * 10; ++i) {
        const int dx1 = random(r * 2) - r, dy1 = random(r * 2) - r, dx2 = random(r * 2) - r, dy2 = random(r * 2) - r;
        COLORREF a = GetPixel(dc, x + dx1, y + dy1), b = GetPixel(dc, x + dx2, y + dy2);
        if (a == CLR_INVALID) a = RGB(255, 255, 255);  // outside BiPict1: GetPixel's -1
        if (b == CLR_INVALID) b = RGB(255, 255, 255);
        SetPixel(dc, x + dx1, y + dy1, b);
        SetPixel(dc, x + dx2, y + dy2, a);
    }
}

void Studio::smear(int x, int y, int kind) {  // 1028:1f31: rows of a strip shifted at random
    if (kind != 4) return;
    HDC dc = pictureDC();
    const int r = smudgeRadius(thickness_);
    const int left = x - r;
    for (int row = y - r; row <= y + r; ++row) {
        const int shift = random(r * 4);
        if (left != r && left - r >= 0) BitBlt(dc, left + shift - 2 * r, row, 2 * r, 1, dc, left - r, row, SRCCOPY);
    }
}

// ---- The Magic Hat (level 2; CRAYON1's unreachable tool 15) ---------------------------------

void Studio::selectBox(int x, int y) {  // 1018:2118: the XOR box being dragged, kept in DS:1aaa…1ab0
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        xorRectangle(dc, startX_, startY_, lastX_, lastY_);
        xorRectangle(dc, startX_, startY_, x, y);
    }
    selection_ = {startX_, startY_, x, y};
}

void Studio::eraseSelectBox() {  // 1018:21ff
    HollowXor band(pictureDC(), colour(60));
    xorRectangle(pictureDC(), selection_.left, selection_.top, selection_.right, selection_.bottom);
}

void Studio::markSelection() {  // 1018:22ca: five dotted blue frames around the box
    HDC dc = pictureDC();
    const int l = std::min(selection_.left, selection_.right), r = std::max(selection_.left, selection_.right);
    const int t = std::min(selection_.top, selection_.bottom), b = std::max(selection_.top, selection_.bottom);
    {
        PenIn pen(dc, 1, RGB(0, 0, 255), PS_DOT);
        for (int d = 0; d <= 4; ++d) xorRectangle(dc, l - d, t - d, r + d, b + d);
    }
    // The left/top corner is normalised but the right/bottom one is not (only clamped): a box
    // dragged leftwards or upwards cannot be grabbed, as in the original.
    selection_.left = l, selection_.top = t;
    if (selection_.right >= pictureWidth()) selection_.right = pictureWidth() - 1;
    if (selection_.bottom >= pictureHeight()) selection_.bottom = pictureHeight() - 1;
}

void Studio::grabSelection(int x, int y) {  // 1008:8bc2: lift the marked box into DS:69e8
    restoreBackup();  // without the frames
    HDC dc = pictureDC();
    const int w = selection_.right - selection_.left, h = selection_.bottom - selection_.top;
    selectionPicture_ = Surface();
    if (w > 0 && h > 0) {
        selectionPicture_.create(w, h, RGB(255, 255, 255));
        BitBlt(selectionPicture_.dc(), 0, 0, w, h, dc, selection_.left, selection_.top, SRCCOPY);
    }
    if (mode_ == 0x48) {  // cut: a white hole (WHITE_PEN, WHITE_BRUSH)
        HGDIOBJ pen = SelectObject(dc, GetStockObject(WHITE_PEN)), brush = SelectObject(dc, GetStockObject(WHITE_BRUSH));
        Rectangle(dc, selection_.left, selection_.top, selection_.right, selection_.bottom);
        SelectObject(dc, pen), SelectObject(dc, brush);
    }
    grabX_ = x - selection_.left, grabY_ = y - selection_.top;  // DS:1ab2, DS:1ab4
    saveBackup();
}

void Studio::moveSelection(int x, int y, int fromX, int fromY, int kind) {  // 1010:2b6a
    if (!selectionPicture_.valid()) return;
    HDC dc = pictureDC();
    const int w = selectionPicture_.width(), h = selectionPicture_.height();
    BitBlt(dc, fromX - grabX_, fromY - grabY_, w, h, backup_.dc(), fromX - grabX_, fromY - grabY_, SRCCOPY);
    if (mode_ != 0x4a) {
        BitBlt(dc, x - grabX_, y - grabY_, w, h, selectionPicture_.dc(), 0, 0, SRCCOPY);
    } else if (kind == 3) {  // the copy flipper: upside down
        StretchBlt(dc, x - grabX_, y - grabY_, w, h, selectionPicture_.dc(), 0, h - 1, w, -(h - 1), SRCCOPY);
    } else {  // mirrored
        StretchBlt(dc, x - grabX_, y - grabY_, w, h, selectionPicture_.dc(), w - 1, 0, -(w - 1), h, SRCCOPY);
    }
}

void Studio::mosaic() {  // 1018:168d: 5x5 blocks of the colour at their centre
    eraseSelectBox();
    HDC dc = pictureDC();
    const int l = std::min(selection_.left, selection_.right), r = std::max(selection_.left, selection_.right);
    const int t = std::min(selection_.top, selection_.bottom), b = std::max(selection_.top, selection_.bottom);
    int cy = t + 3;
    do {
        int cx = l + 3;
        do {
            const COLORREF c = GetPixel(dc, cx, cy);
            for (int py = cy - 2; py <= cy + 2; ++py)
                for (int px = cx - 2; px <= cx + 2; ++px) SetPixel(dc, px, py, c);
            cx += 5;
        } while (cx < r);
        cy += 5;
    } while (cy < b);
}

void Studio::randomMosaic() {  // 1018:190f: round dabs of random size, one size per row
    eraseSelectBox();
    HDC dc = pictureDC();
    const int l = std::min(selection_.left, selection_.right), r = std::max(selection_.left, selection_.right);
    const int t = std::min(selection_.top, selection_.bottom), b = std::max(selection_.top, selection_.bottom);
    int cy = t + 3;
    do {
        int cx = l + 3;
        const int size = random(6), radius = size + 1;
        do {
            const COLORREF c = GetPixel(dc, cx, cy);
            for (int py = cy - radius; py <= cy + radius; ++py)
                for (int px = cx - radius; px <= cx + radius; ++px)
                    if (distance(cx, cy, px, py) <= radius) SetPixel(dc, px, py, c);
            cx += size + 2;
        } while (cx < r);
        cy += 5;
    } while (cy < b);
}

namespace {
struct Box { int l, t, r, b; };
Box normalised(int x1, int y1, int x2, int y2) {
    return {std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2)};
}
}  // namespace

void Studio::resizeArea(int x, int y, int phase, int kind) {  // 1018:1a85: the size changer
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        xorRectangle(dc, startX_, startY_, lastX_, lastY_);
        if (phase == 1) xorRectangle(dc, startX_, startY_, x, y);
    }
    if (phase == 1) return;
    const Box a = normalised(x, y, startX_, startY_);
    const int w = pictureWidth(), h = pictureHeight();
    if (kind == 2) {  // bigger: the box fills the picture
        StretchBlt(dc, 0, 0, w, h, dc, a.l, a.t, a.r - a.l, a.b - a.t, SRCCOPY);
    } else {  // smaller: the picture (as saved when the option was chosen) shrinks into the box
        BitBlt(dc, 0, 0, w, h, dc, 0, 0, WHITENESS);
        StretchBlt(dc, a.l, a.t, a.r - a.l, a.b - a.t, backup_.dc(), 0, 0, w - 2, h - 2, SRCCOPY);
    }
}

void Studio::flipArea(int x, int y, int phase, int kind) {  // 1018:1c75: the flipper
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        xorRectangle(dc, startX_, startY_, lastX_, lastY_);
        if (phase == 1) xorRectangle(dc, startX_, startY_, x, y);
    }
    if (phase == 1) return;
    Box a = normalised(x, y, startX_, startY_);
    if (a.r >= pictureWidth()) a.r = pictureWidth() - 3;
    if (a.b >= pictureHeight()) a.b = pictureHeight() - 3;
    if (a.l < 1) a.l = 1;
    if (a.t < 1) a.t = 1;
    const int w = a.r - a.l, h = a.b - a.t;
    if (kind == 3) StretchBlt(dc, a.l, a.t, w, h, dc, a.l, a.b, w, -h, SRCCOPY);  // upside down
    else StretchBlt(dc, a.l, a.t, w, h, dc, a.r, a.t, -w, h, SRCCOPY);            // mirrored
}

void Studio::negativeArea(int x, int y, int phase) {  // 1018:1e7d
    HDC dc = pictureDC();
    {
        HollowXor band(dc, colour(60));
        xorRectangle(dc, startX_, startY_, lastX_, lastY_);
        if (phase == 1) xorRectangle(dc, startX_, startY_, x, y);
    }
    if (phase == 1) return;
    const Box a = normalised(x, y, startX_, startY_);
    BitBlt(dc, a.l, a.t, a.r - a.l, a.b - a.t, dc, a.l, a.t, DSTINVERT);
}

void Studio::replaceColour(int x, int y) {  // 1018:1ff6: every pixel of the clicked colour
    HDC dc = pictureDC();
    const COLORREF clicked = GetPixel(dc, x, y);
    if (clicked == current_) return;
    GdiFlush();
    // The original asks GetPixel for every pixel of BiPict1 (0..Width, 0..Height inclusive);
    // the DIB is changed directly here.
    Surface& s = picture_.surface;
    const uint32_t from = (GetRValue(clicked) << 16) | (GetGValue(clicked) << 8) | GetBValue(clicked);
    const uint32_t to = (GetRValue(current_) << 16) | (GetGValue(current_) << 8) | GetBValue(current_);
    uint32_t* pixels = s.pixels();
    for (int i = 0; i < s.width() * s.height(); ++i)
        if ((pixels[i] & 0xFFFFFF) == from) pixels[i] = to;
}
