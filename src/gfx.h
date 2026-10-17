// Drawing surfaces. The original draws with GDI on the screen; the remake draws the same GDI
// calls on 32-bit DIB sections and scales the result to the window. True colour matches the
// original running on a high-colour display (the 211-colour palette only matters as RGB values).
#pragma once

#include <windows.h>

#include <cstdint>
#include <span>
#include <vector>

#include "ne.h"

class Surface {
public:
    Surface() = default;
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;
    Surface(Surface&& other) noexcept { *this = std::move(other); }
    Surface& operator=(Surface&& other) noexcept;
    ~Surface() { release(); }

    bool create(int width, int height, COLORREF fill = RGB(0, 0, 0));
    bool valid() const { return dc_ != nullptr; }
    HDC dc() const { return dc_; }
    int width() const { return width_; }
    int height() const { return height_; }
    uint32_t* pixels() const { return pixels_; }  // top-down 0x00RRGGBB
    void blitTo(HDC dest, int x, int y, DWORD rop = SRCCOPY) const { BitBlt(dest, x, y, width_, height_, dc_, 0, 0, rop); }
    bool writeBmp(const wchar_t* path) const;    // 32-bit (test snapshots)
    bool writeBmp24(const wchar_t* path) const;  // 24-bit bottom-up, readable everywhere

private:
    void release();
    HDC dc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ old_ = nullptr;
    uint32_t* pixels_ = nullptr;
    int width_ = 0, height_ = 0;
};

// Decodes a .BMP file (1/4/8/24-bit, plain or RLE) into a new surface. False when not a BMP.
bool loadBmp(std::span<const uint8_t> bmp, Surface* out);

// The program's 211 fixed colours (16 families x 10 shades, 50 Magic Color near-whites, 1
// more), read from the RGB() constants of the original palette routine (sub_6_0491) in
// CRAYON1.EXE/CRAYON2.EXE. Empty when the routine cannot be found.
std::vector<COLORREF> programColours(const ne::Module& exe);
