// Delphi 1's TTabSet (VCL unit Tabs, CRAYON1 segment 9 = selector 1040), ported for the tool
// tabs: owner-drawn tabs with slanted edges over a dithered background.
#pragma once

#include <windows.h>

#include <functional>
#include <vector>

#include "gfx.h"

class TabSet {
public:
    // Published properties as the form sets them.
    int tabHeight = 20;
    int startMargin = 5, endMargin = 15;
    COLORREF backgroundColour = RGB(0, 0, 0), selectedColour = RGB(0, 0, 0), unselectedColour = RGB(255, 255, 255);
    bool ditherBackground = true;
    bool ownerDraw = true;
    int tabCount = 0;

    // OnMeasureTab / OnDrawTab.
    std::function<int(int index)> measureTab;
    std::function<void(HDC canvas, const RECT& r, int index, bool selected)> drawTab;

    void create(int width, int height);  // allocates the edge parts and memory bitmap
    int tabIndex() const { return tabIndex_; }
    void setTabIndex(int index);         // 1040:2a55 (without OnChange)
    // 1040:2c52: index of the tab under a left click at (x, y) (client coordinates), or -1.
    int hitTest(int x, int y) const;
    // 1040:13f3: paints the whole control into `target` (the control's client surface).
    void paint(Surface& target);

private:
    struct TabPos {
        int start, size;
    };
    void createEdgeParts();                  // 1040:21d2
    void paintEdge(int x, int y, int h, int edge);  // 1040:23b3
    void calcTabPositions(int start, int stop, std::vector<TabPos>* out) const;  // 1040:131c

    Surface memBitmap_;
    Surface edgeParts_[4];  // TImageList of 9 x tabHeight pieces, mask colour $008080
    int width_ = 0, height_ = 0, tabIndex_ = 0, firstIndex_ = 0;
};
