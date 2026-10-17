#include "ne.h"

#include <windows.h>

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>

namespace ne {
namespace {

uint16_t u16(const std::vector<uint8_t>& b, size_t at) { return at + 2 <= b.size() ? uint16_t(b[at] | b[at + 1] << 8) : 0; }
uint32_t u32(const std::vector<uint8_t>& b, size_t at) { return u16(b, at) | uint32_t(u16(b, at + 2)) << 16; }

std::string upper(std::string s) {
    for (char& c : s) c = char(std::toupper(uint8_t(c)));
    return s;
}

}  // namespace

bool Module::load(std::vector<uint8_t> bytes, std::string* error) {
    bytes_ = std::move(bytes);
    resources_.clear();
    segments_.clear();
    const uint32_t ne = u32(bytes_, 0x3C);
    if (bytes_.size() < 0x40 || ne + 0x40 > bytes_.size() || bytes_[ne] != 'N' || bytes_[ne + 1] != 'E')
        return *error = "not a 16-bit Windows executable", bytes_.clear(), false;
    const uint32_t align = u16(bytes_, ne + 0x32);
    const uint32_t segTable = ne + u16(bytes_, ne + 0x22);
    for (uint16_t i = 0, n = u16(bytes_, ne + 0x1C); i < n; ++i) {
        const uint32_t sector = u16(bytes_, segTable + 8 * i);
        const uint32_t length = u16(bytes_, segTable + 8 * i + 2);
        segments_.push_back({sector << align, length ? length : 0x10000});
    }
    const uint32_t resTable = ne + u16(bytes_, ne + 0x24);
    const uint32_t shift = u16(bytes_, resTable);
    for (size_t p = resTable + 2; p + 8 <= bytes_.size();) {
        const uint16_t type = u16(bytes_, p), count = u16(bytes_, p + 2);
        if (type == 0) break;
        for (uint16_t k = 0; k < count; ++k) {
            const size_t e = p + 8 + 12 * k;
            const uint16_t id = u16(bytes_, e + 6);
            std::string name;
            if (id & 0x8000) {
                name = "#" + std::to_string(id & 0x7FFF);
            } else if (resTable + id < bytes_.size()) {
                const uint8_t n = bytes_[resTable + id];
                name = upper(std::string(reinterpret_cast<const char*>(&bytes_[resTable + id + 1]), n));
            }
            resources_.push_back({uint16_t(type & 0x7FFF), name, uint32_t(u16(bytes_, e)) << shift,
                                  uint32_t(u16(bytes_, e + 2)) << shift});
        }
        p += 8 + 12 * size_t(count);
    }
    return true;
}

std::span<const uint8_t> Module::resource(uint16_t type, const std::string& name) const {
    const std::string key = upper(name);
    for (const Resource& r : resources_) {
        if (r.type == type && r.name == key && size_t(r.offset) + r.length <= bytes_.size())
            return {bytes_.data() + r.offset, r.length};
    }
    return {};
}

std::span<const uint8_t> Module::segment(int number) const {
    if (number < 1 || number > int(segments_.size())) return {};
    const Segment& s = segments_[size_t(number - 1)];
    if (size_t(s.offset) + s.length > bytes_.size()) return {};
    return {bytes_.data() + s.offset, s.length};
}

void* createIcon(const Module& module, const std::string& groupName, bool cursor, int size) {
    // Group directory: reserved, type, count, then 14-byte entries ending with the RT_ICON/RT_CURSOR id.
    const std::span<const uint8_t> group = module.resource(cursor ? kGroupCursor : kGroupIcon, groupName);
    if (group.size() < 6) return nullptr;
    const int count = group[4] | group[5] << 8;
    int best = -1, bestScore = 1 << 30;
    for (int i = 0; i < count && 6 + 14 * size_t(i + 1) <= group.size(); ++i) {
        const int width = cursor ? (group[6 + 14 * i] | group[7 + 14 * i] << 8) : group[6 + 14 * i];
        const int score = size ? std::abs((width ? width : 256) - size) : i;
        if (score < bestScore) best = i, bestScore = score;
    }
    if (best < 0) return nullptr;
    const size_t e = 6 + 14 * size_t(best);
    const uint16_t id = uint16_t(group[e + 12] | group[e + 13] << 8);
    const std::span<const uint8_t> data = module.resource(cursor ? kCursor : kIcon, id);
    if (data.empty()) return nullptr;
    std::vector<uint8_t> copy(data.begin(), data.end());
    return CreateIconFromResourceEx(copy.data(), DWORD(copy.size()), cursor ? FALSE : TRUE, 0x00030000, 0, 0,
                                    LR_DEFAULTCOLOR);
}

}  // namespace ne
