#include "gfx.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

Surface& Surface::operator=(Surface&& other) noexcept {
    if (this != &other) {
        release();
        dc_ = other.dc_, bitmap_ = other.bitmap_, old_ = other.old_, pixels_ = other.pixels_;
        width_ = other.width_, height_ = other.height_;
        other.dc_ = nullptr, other.bitmap_ = nullptr, other.old_ = nullptr, other.pixels_ = nullptr;
        other.width_ = other.height_ = 0;
    }
    return *this;
}

void Surface::release() {
    if (dc_) {
        SelectObject(dc_, old_);
        DeleteDC(dc_);
    }
    if (bitmap_) DeleteObject(bitmap_);
    dc_ = nullptr, bitmap_ = nullptr, pixels_ = nullptr, width_ = height_ = 0;
}

bool Surface::create(int width, int height, COLORREF fill) {
    release();
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    dc_ = CreateCompatibleDC(nullptr);
    bitmap_ = dc_ ? CreateDIBSection(dc_, &info, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
    if (!bitmap_) return release(), false;
    old_ = SelectObject(dc_, bitmap_);
    pixels_ = static_cast<uint32_t*>(bits);
    width_ = width, height_ = height;
    const uint32_t value = uint32_t(GetRValue(fill)) << 16 | uint32_t(GetGValue(fill)) << 8 | GetBValue(fill);
    std::fill(pixels_, pixels_ + size_t(width) * size_t(height), value);
    return true;
}

bool Surface::writeBmp(const wchar_t* path) const {
    if (!pixels_) return false;
    BITMAPINFOHEADER info{sizeof(info), width_, -height_, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
    const DWORD size = DWORD(width_) * DWORD(height_) * 4;
    BITMAPFILEHEADER file{0x4d42, DWORD(sizeof(file) + sizeof(info) + size), 0, 0, DWORD(sizeof(file) + sizeof(info))};
    GdiFlush();
    FILE* f = _wfopen(path, L"wb");
    if (!f) return false;
    const bool ok = fwrite(&file, sizeof file, 1, f) == 1 && fwrite(&info, sizeof info, 1, f) == 1 &&
                    fwrite(pixels_, 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

bool Surface::writeBmp24(const wchar_t* path) const {
    if (!pixels_) return false;
    GdiFlush();
    const DWORD stride = DWORD((width_ * 3 + 3) & ~3), size = stride * DWORD(height_);
    BITMAPINFOHEADER info{sizeof(info), width_, height_, 1, 24, BI_RGB, size, 0, 0, 0, 0};
    BITMAPFILEHEADER file{0x4d42, DWORD(sizeof(file) + sizeof(info) + size), 0, 0, DWORD(sizeof(file) + sizeof(info))};
    std::vector<uint8_t> rows(size);
    for (int y = 0; y < height_; ++y) {
        const uint32_t* src = pixels_ + size_t(height_ - 1 - y) * size_t(width_);
        uint8_t* dst = rows.data() + size_t(y) * stride;
        for (int x = 0; x < width_; ++x) dst[3 * x] = uint8_t(src[x]), dst[3 * x + 1] = uint8_t(src[x] >> 8), dst[3 * x + 2] = uint8_t(src[x] >> 16);
    }
    FILE* f = _wfopen(path, L"wb");
    if (!f) return false;
    const bool ok = fwrite(&file, sizeof file, 1, f) == 1 && fwrite(&info, sizeof info, 1, f) == 1 &&
                    fwrite(rows.data(), 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

bool loadBmp(std::span<const uint8_t> bmp, Surface* out) {
    if (bmp.size() < 14 + sizeof(BITMAPCOREHEADER) || bmp[0] != 'B' || bmp[1] != 'M') return false;
    DWORD bits = 0;
    std::memcpy(&bits, bmp.data() + 10, 4);
    DWORD headerSize = 0;
    std::memcpy(&headerSize, bmp.data() + 14, 4);
    if (bits >= bmp.size() || bits < 14 + headerSize) return false;
    // Header and colour table copied so biSizeImage can be filled in: the files often leave it
    // 0, which Win16 accepted but Win32 GDI rejects for RLE bitmaps.
    std::vector<uint8_t> info(bmp.begin() + 14, bmp.begin() + bits);
    int width = 0, height = 0;
    if (headerSize == sizeof(BITMAPCOREHEADER)) {
        const auto* core = reinterpret_cast<const BITMAPCOREHEADER*>(info.data());
        width = core->bcWidth, height = core->bcHeight;
    } else {
        auto* header = reinterpret_cast<BITMAPINFOHEADER*>(info.data());
        if (header->biSizeImage == 0) header->biSizeImage = DWORD(bmp.size() - bits);
        width = header->biWidth, height = std::abs(header->biHeight);
    }
    if (width <= 0 || height <= 0 || !out->create(width, height)) return false;
    // RLE "skip" codes leave pixels untouched: black, like a fresh DDB in the original.
    StretchDIBits(out->dc(), 0, 0, width, height, 0, 0, width, height, bmp.data() + bits,
                  reinterpret_cast<const BITMAPINFO*>(info.data()), DIB_RGB_COLORS, SRCCOPY);
    GdiFlush();
    return true;
}

std::vector<COLORREF> programColours(const ne::Module& exe) {
    // Each entry is compiled as: push r / push g / push b (6A xx or 68 xx xx), pop dx / pop bx /
    // pop ax (5A 5B 58), mov ah,bl (8A E3), xor dh,dh (32 F6), mov [table+4i],ax (A3 lo hi),
    // mov [table+4i+2],dx (89 16 lo hi). The table is DS:167a in both levels.
    std::map<uint16_t, COLORREF> entries;
    for (int s = 1; s <= exe.segmentCount() && entries.size() < 211; ++s) {
        const std::span<const uint8_t> c = exe.segment(s);
        for (size_t p = 0; p + 32 < c.size(); ++p) {
            uint8_t rgb[3];
            size_t q = p;
            bool ok = true;
            for (uint8_t& v : rgb) {
                if (c[q] == 0x6A) v = c[q + 1], q += 2;
                else if (c[q] == 0x68) v = c[q + 1], q += 3;
                else ok = false;
                if (!ok) break;
            }
            static const uint8_t kTail[] = {0x5A, 0x5B, 0x58, 0x8A, 0xE3, 0x32, 0xF6, 0xA3};
            if (!ok || std::memcmp(&c[q], kTail, sizeof kTail) != 0) continue;
            const uint16_t at = uint16_t(c[q + 8] | c[q + 9] << 8);
            if (at >= 0x167a && at < 0x167a + 4 * 211 && (at - 0x167a) % 4 == 0)
                entries[at] = RGB(rgb[0], rgb[1], rgb[2]);
            p = q + 9;
        }
    }
    std::vector<COLORREF> colours;
    if (entries.size() != 211) return colours;
    for (const auto& [at, c] : entries) colours.push_back(c);
    return colours;
}
