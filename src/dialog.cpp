#include "dialog.h"

#include <algorithm>

namespace {

void hline(HDC dc, int x1, int x2, int y, COLORREF c) {
    for (int x = x1; x <= x2; ++x) SetPixelV(dc, x, y, c);
}

void vline(HDC dc, int x, int y1, int y2, COLORREF c) {
    for (int y = y1; y <= y2; ++y) SetPixelV(dc, x, y, c);
}

void fill(HDC dc, int x1, int y1, int x2, int y2, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    const RECT r{x1, y1, x2, y2};
    FillRect(dc, &r, b);
    DeleteObject(b);
}

constexpr COLORREF kFace = RGB(192, 192, 192), kLight = RGB(255, 255, 255), kShadow = RGB(128, 128, 128),
                   kDark = RGB(0, 0, 0), kCaptionColour = RGB(0, 0, 128);

}  // namespace

void Dialog::setBounds(int left, int top, int width, int height) {
    left_ = left, top_ = top, width_ = width, height_ = height;
    view_.place(left, top, width, height);
    repaint();
}

RECT Dialog::clientRect() const {
    return {left_ + kFrame, top_ + kNonClientTop, left_ + width_ - kFrame, top_ + height_ - kFrame};
}

void Dialog::repaint() {
    HDC dc = view_.surface.dc();
    const int w = width_, h = height_;
    if (kFrame == 0) {  // bsNone: the client area is the whole window
        fill(dc, 0, 0, w, h, colour);
        SaveDC(dc);
        IntersectClipRect(dc, 0, 0, w, h);
        paintClient(dc);
        hint_.paint(dc);
        RestoreDC(dc, -1);
        return;
    }
    fill(dc, 0, 0, w, h, kFace);
    // Raised 3D frame: face, highlight, then face; bottom/right: black, shadow, face
    // (4 px for a sizeable form, 3 for bsSingle).
    hline(dc, 0, w - 1, 0, kFace), vline(dc, 0, 0, h - 1, kFace);
    hline(dc, 1, w - 2, 1, kLight), vline(dc, 1, 1, h - 2, kLight);
    hline(dc, 0, w - 1, h - 1, kDark), vline(dc, w - 1, 0, h - 1, kDark);
    hline(dc, 1, w - 2, h - 2, kShadow), vline(dc, w - 2, 1, h - 2, kShadow);
    // Caption bar and text.
    fill(dc, kFrame, kFrame, w - kFrame, kFrame + kCaption, kCaptionColour);
    HFONT font = CreateFontW(-11, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, L"MS Sans Serif");
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, kLight);
    RECT text{kFrame + 2, kFrame, w - kFrame - 22, kFrame + kCaption};
    DrawTextW(dc, caption.c_str(), int(caption.size()), &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
    DeleteObject(font);
    // Close box (16x14); Taboutme has biSystemMenu, while the other modal forms do not.
    const int bx = w - kFrame - 2 - 16, by = kFrame + 2;
    fill(dc, bx, by, bx + 16, by + 14, kFace);
    hline(dc, bx, bx + 15, by, kLight), vline(dc, bx, by, by + 13, kLight);
    hline(dc, bx, bx + 15, by + 13, kDark), vline(dc, bx + 15, by, by + 13, kDark);
    hline(dc, bx + 1, bx + 14, by + 12, kShadow), vline(dc, bx + 14, by + 1, by + 12, kShadow);
    for (int i = 0; i < 6; ++i) {  // an embossed X: white offset by one, grey on top
        for (int t = 0; t < 2; ++t) {
            SetPixelV(dc, bx + 5 + i + t + 1, by + 4 + i + 1, kLight);
            SetPixelV(dc, bx + 10 - i + t + 1, by + 4 + i + 1, kLight);
        }
    }
    for (int i = 0; i < 6; ++i) {
        for (int t = 0; t < 2; ++t) {
            SetPixelV(dc, bx + 4 + i + t, by + 3 + i, systemMenu_ ? kDark : kShadow);
            SetPixelV(dc, bx + 9 - i + t, by + 3 + i, systemMenu_ ? kDark : kShadow);
        }
    }
    // Separator line and client area.
    hline(dc, kFrame, w - kFrame - 1, kFrame + kCaption, kFace);
    const int cx = kFrame, cy = kNonClientTop;
    fill(dc, cx, cy, w - kFrame, h - kFrame, colour);
    SaveDC(dc);
    IntersectClipRect(dc, cx, cy, w - kFrame, h - kFrame);
    SetViewportOrgEx(dc, cx, cy, nullptr);
    paintClient(dc);
    hint_.paint(dc);
    RestoreDC(dc, -1);
}

void Dialog::showHint(int x, int y, int hint) {
    const std::string* text = hints_ ? hints_->text(hint) : nullptr;
    if (!text) return;
    hint_.place(*text, x, y, clientWidth());
    repaint();
    hints_->speak(hint);
    hintTimer_ = true, hintArmed_ = false;  // DS:1066 = 1, DS:1067 = 0, Timer2 on
    hintDue_ = app_.now() + 250;
}

void Dialog::hintTimer(long now) {
    if (hintTimer_ && now >= hintDue_) hintTimer_ = false, hintArmed_ = true;
}

void Dialog::hintMove() {
    // Panel1.Visible := False. (The original's Timer2 also leaves the shared DS:1067 set, so
    // after the dialog closes the main form's next move over the picture restores a stale
    // undo copy over it; the remake keeps the dialogs' hint state to themselves.)
    if (!hintArmed_ || !hint_.visible) return;
    hint_.visible = false;
    repaint();
}

void Dialog::close() {
    closed_ = true;
}

void Dialog::chromeMouseDown(int x, int y, int button) {
    if (!systemMenu_ || button != 0) return;
    const int left = left_ + width_ - kFrame - 2 - 16;
    const int top = top_ + kFrame + 2;
    if (x >= left && x < left + 16 && y >= top && y < top + 14) close();
}

void loadImage(ImageBox& box, const dfm::Component* c) {
    if (!c) return;
    box.rect = {c->left(), c->top(), c->left() + c->width(), c->top() + c->height()};
    box.picture = Surface();
    box.icon.reset();
    if (loadBmp(c->bitmap(), &box.picture)) {
        if (c->flag("AutoSize", false))
            box.rect.right = box.rect.left + box.picture.width(), box.rect.bottom = box.rect.top + box.picture.height();
        return;
    }
    const std::span<const uint8_t> ico = c->icon();
    if (ico.size() < 6 + 16) return;
    // First directory entry: size at +8, offset at +12 (ICONDIRENTRY), then the DIB image.
    const uint32_t bytes = ico[6 + 8] | ico[6 + 9] << 8 | ico[6 + 10] << 16 | uint32_t(ico[6 + 11]) << 24;
    const uint32_t offset = ico[6 + 12] | ico[6 + 13] << 8 | ico[6 + 14] << 16 | uint32_t(ico[6 + 15]) << 24;
    if (size_t(offset) + bytes > ico.size()) return;
    std::vector<uint8_t> data(ico.begin() + offset, ico.begin() + offset + bytes);
    HICON h = CreateIconFromResourceEx(data.data(), bytes, TRUE, 0x00030000, 32, 32, LR_DEFAULTCOLOR);
    if (!h) return;
    box.icon = std::shared_ptr<void>(h, [](void* p) { DestroyIcon(static_cast<HICON>(p)); });
    if (c->flag("AutoSize", false)) box.rect.right = box.rect.left + 32, box.rect.bottom = box.rect.top + 32;
}

void ImageBox::paint(HDC dc) const {
    if (visible && icon) {
        DrawIcon(dc, rect.left, rect.top, static_cast<HICON>(icon.get()));
        return;
    }
    if (!visible || !picture.valid()) return;
    const int w = rect.right - rect.left, h = rect.bottom - rect.top;
    if (stretch) {  // Canvas.StretchDraw with the DC's default mode, BLACKONWHITE
        StretchBlt(dc, rect.left, rect.top, w, h, picture.dc(), 0, 0, picture.width(), picture.height(), SRCCOPY);
    } else {
        BitBlt(dc, rect.left, rect.top, std::min(w, picture.width()), std::min(h, picture.height()), picture.dc(), 0, 0,
               SRCCOPY);
    }
}

int ListBoxView::hit(int x, int y) const {
    if (x < rect.left || x >= rect.right || y < rect.top || y >= rect.bottom) return -1;
    const int i = topIndex + (y - rect.top - (border ? 1 : 0)) / itemHeight;
    return i >= 0 && i < int(items.size()) ? i : -1;
}

void ListBoxView::paint(HDC dc) const {
    fill(dc, rect.left, rect.top, rect.right, rect.bottom, colour);
    if (border) {  // Ctl3D = False: a 1-pixel black frame
        HBRUSH b = CreateSolidBrush(RGB(0, 0, 0));
        FrameRect(dc, &rect, b);
        DeleteObject(b);
    }
    HFONT font = CreateFontW(fontHeight, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, L"System");
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    const int inset = border ? 1 : 0;
    SaveDC(dc);
    IntersectClipRect(dc, rect.left + inset, rect.top + inset, rect.right - inset, rect.bottom - inset);
    for (int i = topIndex; i < int(items.size()); ++i) {
        const int y = rect.top + inset + (i - topIndex) * itemHeight;
        if (y >= rect.bottom) break;
        RECT r{rect.left + inset, y, rect.right - inset, y + itemHeight};
        if (i == itemIndex) {
            fill(dc, r.left, r.top, r.right, r.bottom, RGB(0, 0, 128));
            SetTextColor(dc, RGB(255, 255, 255));
        } else {
            SetTextColor(dc, textColour);
        }
        TextOutA(dc, r.left + 2, y, items[size_t(i)].c_str(), int(items[size_t(i)].size()));
        if (i == itemIndex) {
            SetTextColor(dc, RGB(0, 0, 0));
            SetBkColor(dc, RGB(255, 255, 255));
            DrawFocusRect(dc, &r);
        }
    }
    RestoreDC(dc, -1);
    SelectObject(dc, old);
    DeleteObject(font);
}
