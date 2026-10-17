#include "tabset.h"

#include <algorithm>

namespace {

constexpr int kEdgeWidth = 9;
// Windows 3.1 default system colours (Delphi 1 clBtnShadow = -17, clWindowFrame = -7,
// clBtnHighlight = -21).
constexpr COLORREF kBtnShadow = RGB(128, 128, 128), kWindowFrame = RGB(0, 0, 0), kBtnHighlight = RGB(255, 255, 255);
constexpr COLORREF kMask = RGB(0x80, 0x80, 0);  // $00008080

void polyline(HDC dc, COLORREF colour, std::initializer_list<POINT> points) {
    HPEN pen = CreatePen(PS_SOLID, 1, colour);
    HGDIOBJ old = SelectObject(dc, pen);
    std::vector<POINT> p(points);
    Polyline(dc, p.data(), int(p.size()));
    SelectObject(dc, old);
    DeleteObject(pen);
}

void polygon(HDC dc, COLORREF colour, std::initializer_list<POINT> points) {
    HPEN pen = CreatePen(PS_SOLID, 1, colour);
    HBRUSH brush = CreateSolidBrush(colour);
    HGDIOBJ oldPen = SelectObject(dc, pen), oldBrush = SelectObject(dc, brush);
    std::vector<POINT> p(points);
    Polygon(dc, p.data(), int(p.size()));
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void line(HDC dc, COLORREF colour, int x1, int y1, int x2, int y2) {
    polyline(dc, colour, {{x1, y1}, {x2, y2}});
}

}  // namespace

void TabSet::create(int width, int height) {
    width_ = width, height_ = height;
    createEdgeParts();
}

void TabSet::createEdgeParts() {  // 1040:21d2
    const int h = tabHeight - 1;
    for (int i = 0; i < 4; ++i) {
        Surface& s = edgeParts_[i];
        s.create(kEdgeWidth, tabHeight, kMask);
        HDC dc = s.dc();
        switch (i) {
        case 0:  // 1040:1e7d — leading edge of the selected tab
            polygon(dc, selectedColour, {{3, 0}, {8, h}, {9, h}, {9, 0}, {3, 0}});
            line(dc, kBtnShadow, 0, 0, 4, 0);
            line(dc, kBtnHighlight, 4, 1, 9, h);
            polyline(dc, kWindowFrame, {{0, 1}, {3, 1}, {8, h}, {9, h}});
            break;
        case 1:  // 1040:1b79 — leading edge of an unselected tab
            line(dc, kBtnShadow, 0, 0, 10, 0);
            polygon(dc, unselectedColour, {{3, 1}, {8, h}, {9, h}, {9, 1}, {3, 1}});
            polyline(dc, kWindowFrame, {{0, 1}, {10, 1}, {3, 1}, {8, h}, {9, h}});
            break;
        case 2:  // 1040:1ce4 — trailing edge of the selected tab
            polygon(dc, selectedColour, {{6, 1}, {2, h}, {0, h}, {0, 0}, {10, 0}});
            polyline(dc, kBtnShadow, {{6, 0}, {10, 0}, {6, 1}, {1, h}, {0, h}});
            polyline(dc, kWindowFrame, {{9, 1}, {7, 1}, {2, h}, {-1, h}});
            break;
        case 3:  // 1040:2024 — trailing edge of an unselected tab
            line(dc, kBtnShadow, -1, 0, 10, 0);
            polygon(dc, unselectedColour, {{6, 1}, {1, h}, {0, h}, {0, 1}, {6, 1}});
            line(dc, kBtnShadow, -1, 0, 10, 0);
            polyline(dc, kWindowFrame, {{0, 1}, {10, 1}, {7, 1}, {2, h}, {-1, h}});
            break;
        }
        GdiFlush();
    }
}

void TabSet::paintEdge(int x, int y, int, int edge) {  // 1040:23b3
    auto draw = [&](int part) {  // TImageList.Draw: masked copy
        const Surface& s = edgeParts_[part];
        GdiFlush();  // the mask colour RGB(0x80,0x80,0) is 0x808000 as a 0x00RRGGBB pixel
        for (int py = 0; py < s.height(); ++py) {
            const int ty = y + py;
            if (ty < 0 || ty >= memBitmap_.height()) continue;
            for (int px = 0; px < s.width(); ++px) {
                const int tx = x + px;
                const uint32_t c = s.pixels()[py * s.width() + px];
                if (tx < 0 || tx >= memBitmap_.width() || c == 0x808000) continue;
                memBitmap_.pixels()[ty * memBitmap_.width() + tx] = c;
            }
        }
    };
    switch (edge) {  // jump table at 1040:23a5
    case 1: draw(0); break;            // etFirstIsSel
    case 2: draw(1); break;            // etFirstNotSel
    case 3: draw(2); break;            // etLastIsSel
    case 4: draw(3); break;            // etLastNotSel
    case 5: draw(3), draw(0); break;   // etNotSelToSel
    case 6: draw(1), draw(2); break;   // etSelToNotSel
    case 7: draw(1), draw(3); break;   // etNotSelToNotSel
    }
}

void TabSet::calcTabPositions(int start, int stop, std::vector<TabPos>* out) const {  // 1040:131c
    out->clear();
    for (int i = firstIndex_; i < tabCount; ++i) {
        const int size = measureTab ? measureTab(i) : 0;
        const int next = start + size + kEdgeWidth;
        if (next > stop) break;
        out->push_back({start, size});
        start = next;
    }
}

void TabSet::setTabIndex(int index) {
    if (index >= -1 && index < tabCount) tabIndex_ = index;
}

int TabSet::hitTest(int x, int y) const {  // 1040:2c52
    if (y > tabHeight) return -1;
    const int slack = tabHeight / 2 > y ? 3 : 4;
    std::vector<TabPos> tabs;
    calcTabPositions(startMargin + kEdgeWidth, width_ - endMargin, &tabs);
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (x >= tabs[i].start - slack && x <= tabs[i].start + tabs[i].size + slack) return firstIndex_ + int(i);
    }
    return -1;
}

void TabSet::paint(Surface& target) {  // 1040:13f3
    const int bitmapHeight = std::max(height_, tabHeight + 5);
    if (memBitmap_.width() != width_ || memBitmap_.height() != bitmapHeight) memBitmap_.create(width_, bitmapHeight);
    std::vector<TabPos> tabs;
    calcTabPositions(startMargin + kEdgeWidth, width_ - endMargin, &tabs);  // (no scroller needed)
    HDC dc = memBitmap_.dc();
    // Background: 8x8 brush of BackgroundColor with white on even/odd pixels (1040:25b6).
    GdiFlush();
    const uint32_t bg = uint32_t(GetRValue(backgroundColour)) << 16 | uint32_t(GetGValue(backgroundColour)) << 8 |
                        GetBValue(backgroundColour);
    for (int y = 0; y < memBitmap_.height(); ++y)
        for (int x = 0; x < memBitmap_.width(); ++x)
            memBitmap_.pixels()[y * memBitmap_.width() + x] = ditherBackground && (x % 2) == (y % 2) ? 0xFFFFFF : bg;
    line(dc, kBtnShadow, 0, 0, memBitmap_.width() + 1, 0);
    line(dc, kWindowFrame, 0, 1, memBitmap_.width() + 1, 1);
    const int visible = int(tabs.size());
    for (int t = 0; t < visible; ++t) {
        const bool isFirst = t == 0, isLast = t == visible - 1;
        const bool isSelected = t + firstIndex_ == tabIndex_, isPrevSelected = t + firstIndex_ - 1 == tabIndex_;
        int trailing = 0, leading;
        if (isLast) trailing = isSelected ? 3 : 4;
        if (isFirst) leading = isSelected ? 1 : 2;
        else leading = isPrevSelected ? 6 : isSelected ? 5 : 7;
        const TabPos& pos = tabs[size_t(t)];
        GdiFlush();
        if (leading) paintEdge(pos.start - kEdgeWidth, 0, tabHeight - 1, leading);
        RECT r{pos.start, 0, pos.start + pos.size, tabHeight};
        SetBkColor(dc, isSelected ? selectedColour : unselectedColour);
        ExtTextOutW(dc, pos.start, 2, ETO_OPAQUE, &r, nullptr, 0, nullptr);
        if (ownerDraw && drawTab) drawTab(dc, r, t + firstIndex_, isSelected);
        GdiFlush();
        if (trailing) paintEdge(pos.start + pos.size, 0, tabHeight - 1, trailing);
        line(dc, kWindowFrame, pos.start, tabHeight - 1, pos.start + pos.size, tabHeight - 1);
        if (isSelected) {
            line(dc, kBtnShadow, pos.start, tabHeight - 2, pos.start + pos.size, tabHeight - 2);
        } else {
            line(dc, kWindowFrame, pos.start, 1, pos.start + pos.size, 1);
            line(dc, kBtnShadow, pos.start, 0, pos.start + pos.size + 1, 0);
        }
    }
    GdiFlush();
    BitBlt(target.dc(), 0, 0, target.width(), target.height(), dc, 0, 0, SRCCOPY);  // Canvas.Draw(0, 0, MemBitmap)
}
