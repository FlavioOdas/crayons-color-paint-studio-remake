#include "studio.h"

#include <algorithm>
#include <cmath>

int Studio::shadowTextWidth(int height) {  // 1008:3c3f
    HDC dc = pictureDC();
    HFONT font = CreateFontA(-height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadowFont_.c_str());
    HGDIOBJ old = SelectObject(dc, font);
    SIZE size{};
    GetTextExtentPoint32A(dc, text_.c_str(), int(text_.size()), &size);
    SelectObject(dc, old);
    DeleteObject(font);
    return size.cx;
}

int Studio::shadowFitHeight(int width, int gap) {  // 1008:41aa, 3cfc, 47fb, 4628
    const float ratio = 96.0f / 72.0f;
    int height = 10, low = 2, high = 152;
    while (std::abs(high - low) >= gap && high != low) {
        const int middle = (high - low) / 2 + low;
        height = int(std::nearbyint(float(middle) * ratio));
        const int measured = shadowTextWidth(height);
        if (measured == width) break;
        if (measured > width) high = middle;
        else low = middle;
    }
    return height;
}

void Studio::shadowCenteredText(int x, int y, int height, COLORREF textColour) {  // 1008:451d
    HDC dc = pictureDC();
    HFONT font = CreateFontA(height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadowFont_.c_str());
    HGDIOBJ old = SelectObject(dc, font);
    const int oldMode = SetBkMode(dc, TRANSPARENT);
    const UINT oldAlign = SetTextAlign(dc, TA_CENTER | TA_BASELINE);
    const COLORREF oldColour = SetTextColor(dc, textColour);
    TextOutA(dc, x, y, text_.c_str(), int(text_.size()));
    SetTextColor(dc, oldColour);
    SetTextAlign(dc, oldAlign);
    SetBkMode(dc, oldMode);
    SelectObject(dc, old);
    DeleteObject(font);
}

void Studio::shadowBoxText(int x, int y) {
    if (text_.empty()) return;
    const int left = std::min(startX_, x), top = std::min(startY_, y);
    const int right = std::max(startX_, x), bottom = std::max(startY_, y);
    const int width = right - left;
    const int height = shadowFitHeight(width, mode_ == 0x4c || mode_ == 0x4d ? 3 : 2);
    if (mode_ == 0x4e) {  // 1008:4628: growing 3D
        const int depth = width / 8, centre = (right - left) / 2 + left;
        for (int i = 0; i <= depth; ++i) {
            const int fontHeight = -height + depth - i;
            if (fontHeight < 0) shadowCenteredText(centre, bottom - i, fontHeight, colour(family_ * 10 + 9));
        }
        shadowCenteredText(centre, bottom - depth, -height, current_);
        return;
    }
    if (mode_ == 0x4b) w1aa6 = height;  // 1008:41aa: shares DS:1aa6 with the rotator
    HDC dc = pictureDC();
    HFONT font = CreateFontA(-height, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadowFont_.c_str());
    HGDIOBJ old = SelectObject(dc, font);
    const int oldMode = SetBkMode(dc, TRANSPARENT);
    const UINT oldAlign = SetTextAlign(dc, TA_LEFT | TA_TOP);
    const COLORREF oldColour = GetTextColor(dc);
    auto put = [&](int px, int py) { TextOutA(dc, px, py, text_.c_str(), int(text_.size())); };
    int faceX = left, faceY = top;
    if (mode_ == 0x4c) {  // 1008:3cfc: extruded text
        const int depth = height / 3;
        SetTextColor(dc, colour(family_ * 10 + 9));
        for (int i = 0; i <= depth; ++i) {
            switch (w0fd6) {
            case 2: put(left + i, top + i); break;
            case 3: put(left, top + i); break;
            case 4: put(left - i, top + i); break;
            case 5: put(left + i, top + depth + 1 - i); break;
            case 6: put(left, top + depth + 1 - i); break;
            case 7: put(left + depth + 1 - i, top + depth + 1 - i); break;
            }
        }
        switch (w0fd6) {
        case 2: faceX += depth + 1; faceY += depth + 1; break;
        case 3: faceY += depth + 1; break;
        case 4: faceX -= depth + 1; faceY += depth + 1; break;
        case 5: faceX += depth + 1; break;
        case 7: break;
        }
    } else if (mode_ == 0x4d) {  // 1008:47fb: drop shadow
        const int depth = height / 10;
        int dx = 0, dy = 0;
        if (w0fd6 == 2 || w0fd6 == 5) dx = -depth;
        if (w0fd6 == 4 || w0fd6 == 7) dx = depth;
        if (w0fd6 >= 2 && w0fd6 <= 4) dy = -depth;
        if (w0fd6 >= 5 && w0fd6 <= 7) dy = depth;
        SetTextColor(dc, colour(family_ * 10 + 9));
        put(left + dx, top + dy);
    }
    SetTextColor(dc, current_);
    put(faceX, faceY);
    SetTextColor(dc, oldColour);
    SetTextAlign(dc, oldAlign);
    SetBkMode(dc, oldMode);
    SelectObject(dc, old);
    DeleteObject(font);
}

void Studio::shadowRotatedText(int x, int y) {  // 1008:43c0
    if (text_.empty()) return;
    const int height = -int(std::nearbyint(float(thickness_ * 7) * (96.0f / 72.0f)));
    HDC dc = pictureDC();
    HFONT font = CreateFontA(height, 0, w1aa6 * 10, w1aa6 * 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             shadowFont_.c_str());
    HGDIOBJ old = SelectObject(dc, font);
    const int oldMode = SetBkMode(dc, TRANSPARENT);
    const UINT oldAlign = SetTextAlign(dc, TA_CENTER | TA_BASELINE);
    const COLORREF oldColour = SetTextColor(dc, current_);
    TextOutA(dc, x, y, text_.c_str(), int(text_.size()));
    SetTextColor(dc, oldColour);
    SetTextAlign(dc, oldAlign);
    SetBkMode(dc, oldMode);
    SelectObject(dc, old);
    DeleteObject(font);
}
