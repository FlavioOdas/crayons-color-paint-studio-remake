// Eraser and "magic" effects of segment 4 (selector 1018). The originals draw synchronously
// inside the click; animations that busy-waited on GetTickCount keep their interval, and
// drawing loops that were visibly slow on 1990s hardware get a short paced frame (frame()).
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "studio.h"

namespace {

constexpr DWORD kSrcAnd = 0x008800C6, kSrcPaint = 0x00EE0086;

void line(HDC dc, int x1, int y1, int x2, int y2) {  // 1028:165f
    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);
}

struct Gdi {
    HDC dc;
    HGDIOBJ oldPen = nullptr, oldBrush = nullptr;
    HPEN pen = nullptr;
    HBRUSH brush = nullptr;
    explicit Gdi(HDC d) : dc(d) {}
    void usePen(int width, COLORREF c) {
        HPEN p = CreatePen(PS_SOLID, width, c);
        HGDIOBJ old = SelectObject(dc, p);
        if (!oldPen) oldPen = old;
        if (pen) DeleteObject(pen);
        pen = p;
    }
    void useBrush(COLORREF c) {
        HBRUSH b = CreateSolidBrush(c);
        HGDIOBJ old = SelectObject(dc, b);
        if (!oldBrush) oldBrush = old;
        if (brush) DeleteObject(brush);
        brush = b;
    }
    ~Gdi() {
        if (oldPen) SelectObject(dc, oldPen);
        if (oldBrush) SelectObject(dc, oldBrush);
        if (pen) DeleteObject(pen);
        if (brush) DeleteObject(brush);
    }
};

}  // namespace

void Studio::whiteOut() {  // 1018:04db
    PatBlt(pictureDC(), 0, 0, pictureWidth(), pictureHeight(), WHITENESS);
}

void Studio::wipe(int kind) {  // 1018:0549: white lines sweeping the picture
    if (kind >= 5) return squares(kind - 4);
    HDC dc = pictureDC();
    Gdi g(dc);
    g.usePen(1, colour(60));
    const int w = pictureWidth(), h = pictureHeight();
    playSound(64 + kind);
    if (kind == 1) {  // from both sides to the middle
        for (int c = 0, e = w; c <= w / 2 + 1; ++c, --e) {
            line(dc, c, 0, c, h), line(dc, e, 0, e, h);
            if (c % 8 == 0) frame(5);
        }
    } else if (kind == 2) {  // from the middle out
        for (int c = w / 2, e = w / 2; c <= w; ++c, --e) {
            line(dc, c, 0, c, h), line(dc, e, 0, e, h);
            if (c % 8 == 0) frame(5);
        }
    } else if (kind == 3) {  // from top and bottom to the middle
        for (int c = 0, e = h; c <= h / 2 + 1; ++c, --e) {
            line(dc, 0, c, w, c), line(dc, 0, e, w, e);
            if (c % 6 == 0) frame(5);
        }
    } else if (kind == 4) {  // from the middle to top and bottom
        for (int c = h / 2, e = h / 2; c <= h; ++c, --e) {
            line(dc, 0, c, w, c), line(dc, 0, e, w, e);
            if (c % 6 == 0) frame(5);
        }
    }
}

void Studio::squares(int kind) {  // 1018:0788: 4x4 white squares, row by row or column by column
    HDC dc = pictureDC();
    Gdi g(dc);
    g.usePen(1, colour(60));
    g.useBrush(colour(60));
    const int cols = pictureWidth() / 4, rows = pictureHeight() / 4;
    playSound(68 + kind);
    if (kind == 1 || kind == 2) {  // rows: top down (1) or bottom up (2)
        for (int i = 0; i <= rows; ++i) {
            const int r = kind == 1 ? i : rows - i;
            for (int c = 0; c <= cols; ++c) Rectangle(dc, c * 4, r * 4, c * 4 + 4, r * 4 + 4);
            frame(15);
        }
    } else if (kind == 3 || kind == 4) {  // columns: left to right (3) or right to left (4)
        for (int i = 0; i <= cols; ++i) {
            const int c = kind == 3 ? i : cols - i;
            for (int r = 0; r <= rows; ++r) Rectangle(dc, c * 4, r * 4, c * 4 + 4, r * 4 + 4);
            frame(10);
        }
    }
}

void Studio::shrinkAway() {  // 1018:0002: the picture shrinks into the middle and vanishes
    saveBackup();
    HDC dc = pictureDC();
    const int w = pictureWidth(), h = pictureHeight(), dx = w / 25, dy = h / 25;
    playSound(63);
    for (int s = 1; s <= 12; ++s) {
        StretchBlt(dc, dx * s, dy * s, w - 2 * dx * s, h - 2 * dy * s, backup_.dc(), 0, 0, w - 2, h - 2, SRCCOPY);
        frame(40);
    }
    whiteOut();
}

void Studio::sweepUp() {  // 1018:0147: a figure pushes the picture up and out
    Surface sprite;  // a63.asl: picture at x 0, mask at x 80, 80x150 each
    loadBmp(app_.assets().files().read("a63.asl"), &sprite);
    HDC dc = pictureDC();
    const int w = pictureWidth(), h = pictureHeight(), step = h / 25, band = h / 10;
    playSound(64);
    for (int i = 1; i <= 20; ++i) {
        const int y = h - step * i;
        PatBlt(dc, 0, y, w, h - y, WHITENESS);
        BitBlt(dc, w / 2 - 40, y, 80, 150, sprite.dc(), 80, 0, kSrcAnd);
        BitBlt(dc, w / 2 - 40, y, 80, 150, sprite.dc(), 0, 0, kSrcPaint);
        StretchBlt(dc, 0, 0, w, band, dc, 0, band, w, y - band, SRCCOPY);
        frame(40);
    }
    whiteOut();
}

void Studio::shatter() {  // 1018:02f9: ragged pieces that shrink into the middle
    saveBackup();
    HDC dc = pictureDC();
    const int w = pictureWidth(), h = pictureHeight(), dx = w / 20, dy = h / 20;
    HRGN regions[10] = {};
    for (int s = 1; s <= 9; ++s) {
        const int ox = dx * s, oy = dy * s, rw = w - 2 * ox, rh = h - 2 * oy;
        for (int i = 0; i <= 50; ++i) {
            const int px = ox + random(rw);
            const int py = oy + random(rh);
            points_[i] = {px, py};
        }
        pointCount_ = 51;
        whiteOut();
        regions[s] = CreatePolygonRgn(points_, pointCount_, ALTERNATE);
        SelectClipRgn(dc, regions[s]);
        StretchBlt(dc, ox, oy, rw, rh, backup_.dc(), 0, 0, w - 2, h - 2, SRCCOPY);
        if (s == 1) playSound(61);
        frame(60);
    }
    whiteOut();  // (still clipped to the last piece, as in the original)
    SelectClipRgn(dc, nullptr);
    for (HRGN r : regions)
        if (r) DeleteObject(r);
}

void Studio::horizon() {  // 1018:157c: the picture shrinks towards the horizon (no white-out)
    HDC dc = pictureDC();
    const int w = pictureWidth(), h = pictureHeight(), dx = w / 25, dy = h / 25;
    for (int s = 1; s <= 12; ++s) {
        StretchBlt(dc, dx * s, dy * s, w - 2 * dx * s, h - 2 * dy * s, backup_.dc(), 0, 0, w - 2, h - 2, SRCCOPY);
        frame(40);
    }
}

void Studio::squiggle() {  // 1018:17d8: rows shifted in waves, ten passes from the backup
    HDC dc = pictureDC();
    const int w = pictureWidth(), h = pictureHeight();
    for (int pass = 0; pass <= 9; ++pass) {
        bool right = random(10) > 4;
        int length = random(50) + 10;
        int offset = random(length) + 7;
        for (int row = 1; row <= h; ++row) {
            BitBlt(dc, right ? offset : -offset, row, w - 2, 1, backup_.dc(), 0, row, SRCCOPY);
            if (++offset >= length) {
                right = !right;
                offset = 0;
                length = random(50) + 10;
            }
        }
        frame(40);
    }
}

void Studio::dynamite(int x, int y) {  // 1018:0a5c: a lit stick of dynamite, then the blast
    HDC dc = pictureDC();
    Gdi g(dc);
    g.usePen(8, current_);
    g.useBrush(colour(60));
    Surface sheet, under;  // a930.asl: four 76x49 frames at x 1/81/161/241, pictures at y 1, masks at y 54
    loadBmp(app_.assets().files().read("a930.asl"), &sheet);
    under.create(76, 49);
    const int sx = x - 38, sy = y - 25;
    BitBlt(under.dc(), 0, 0, 76, 49, dc, sx, sy, SRCCOPY);
    playSound(62);
    static const int kFrameX[8] = {1, 1, 81, 81, 161, 161, 241, 241};
    for (int i = 0; i < 8; ++i) {
        frame(90);  // the original polls GetTickCount for 90 ms
        BitBlt(dc, sx, sy, 76, 49, under.dc(), 0, 0, SRCCOPY);
        BitBlt(dc, sx, sy, 76, 49, sheet.dc(), kFrameX[i], 54, kSrcAnd);
        BitBlt(dc, sx, sy, 76, 49, sheet.dc(), kFrameX[i], 1, kSrcPaint);
    }
    for (int k = 2; k <= 80; ++k) {
        Ellipse(dc, x - 4 * k, y - 4 * k, x + 4 * k, y + 4 * k);
        if (k % 2 == 0) frame(10);
    }
    whiteOut();
}

void Studio::train(int x, int y) {  // 1018:0d3c: a train runs left, laying rails and sleepers
    playSound(73);
    HDC dc = pictureDC();
    Gdi g(dc);
    g.usePen(2, current_);
    Surface sheet, under;  // a931.asl: four 76x49 frames at x 1/81/161/241, pictures y 1, masks y 54
    loadBmp(app_.assets().files().read("a931.asl"), &sheet);
    under.create(76, 49);
    int tx = x - 38, ty = y - 25;            // the engine
    int ax = x + 25, ay = y + 10;            // rail 1, last point
    int bx = x + 20, by = y + 16;            // rail 2, last point
    int sleeperX = ax, railY1 = ay, railY2 = by;
    BitBlt(under.dc(), 0, 0, 76, 49, dc, tx, ty, SRCCOPY);
    const int pick = random(3) + 1;
    const int drift = pick == 1 || pick == 3 ? 1 : 0;
    static const int kFrameX[4] = {1, 0x51, 0xa1, 0xf1};
    int frameIndex = 1;
    do {
        frame(100);
        BitBlt(dc, tx, ty, 76, 49, under.dc(), 0, 0, SRCCOPY);
        const int n1 = tx + 58, n2 = tx + 68;
        line(dc, ax, ay, n1, railY1);
        line(dc, bx, by, n2, railY2);
        if (std::abs(sleeperX - n1) >= 10) {
            line(dc, n1, railY1 - 3, n2, railY2 + 3);
            sleeperX = n1;
        }
        tx -= 5;
        ay = railY1, by = railY2, ax = n1, bx = n2;
        ty += drift, railY1 += drift, railY2 += drift;
        BitBlt(under.dc(), 0, 0, 76, 49, dc, tx, ty, SRCCOPY);
        BitBlt(dc, tx, ty, 76, 49, sheet.dc(), kFrameX[frameIndex - 1], 54, kSrcAnd);
        BitBlt(dc, tx, ty, 76, 49, sheet.dc(), kFrameX[frameIndex - 1], 1, kSrcPaint);
        if (++frameIndex == 5) frameIndex = 1;
    } while (ty < pictureHeight() && ty > -49 && tx > -70);
    BitBlt(dc, tx, ty, 76, 49, under.dc(), 0, 0, SRCCOPY);
}

void Studio::walker(int x, int y) {  // 1018:1136: a little figure walks down, trailing a wavy line
    HDC dc = pictureDC();
    Gdi g(dc);
    g.usePen(thickness_, current_);
    Surface sheet, under;  // a932.asl: 49x49 frames at x 1/54/107/160/213, pictures y 1, masks y 54
    loadBmp(app_.assets().files().read("a932.asl"), &sheet);
    under.create(76, 49);
    int sx = x - 25, sy = y - 25;
    int lx0 = x - 17, ly0 = y + 18, lx1 = lx0, ly1 = ly0;
    BitBlt(under.dc(), 0, 0, 49, 49, dc, sx, sy, SRCCOPY);
    int steps = random(15) + 2;
    int slope = -(random(8) + 1);  // Random(8) + 1 times -1 (longint multiply 1078:312d)
    int count = 0, frameIndex = 1;
    bool first = true;
    static const int kBob[8] = {8, 12, 25, 36, 44, 36, 25, 12};
    static const int kFrameX[8] = {1, 0x36, 0x6b, 0xa0, 0xd5, 0xa0, 0x6b, 0x36};
    do {
        frame(100);
        BitBlt(dc, sx, sy, 49, 49, under.dc(), 0, 0, SRCCOPY);
        sy += 4, ++count, ly1 += 4;
        sx += slope;
        lx1 = sx + kBob[frameIndex - 1];
        line(dc, lx1, ly1, lx0, ly0);
        lx0 = lx1, ly0 = ly1;
        if (count >= steps) {
            slope = slope < 0 ? random(8) + 1 : -(random(8) + 1);
            count = 0;
            steps = random(15) + 2;
        }
        BitBlt(under.dc(), 0, 0, 49, 49, dc, sx, sy, SRCCOPY);
        BitBlt(dc, sx, sy, 49, 49, sheet.dc(), kFrameX[frameIndex - 1], 54, kSrcAnd);
        BitBlt(dc, sx, sy, 49, 49, sheet.dc(), kFrameX[frameIndex - 1], 1, kSrcPaint);
        if (first) first = false;  // the original plays "snd44.asl" from the program folder here:
                                   // a wrong path (the sound is SND\SND44.ASL), so only a beep
        if (frameIndex == 5) playSound(74);
        if (frameIndex == 1) playSound(75);
        if (++frameIndex > 8) frameIndex = 1;
    } while (sy < pictureHeight());
    playSound(76);
    BitBlt(dc, sx, sy, 76, 49, under.dc(), 0, 0, SRCCOPY);  // (76 wide, as in the original)
}
