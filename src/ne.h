// Resources and code bytes of the original 16-bit executables (NE format). The remake reads
// form layouts and pictures, icons, cursors and a few constant tables from the player's
// CRAYONST/CRAYON1/CRAYON2.EXE; it never runs them.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace ne {

constexpr uint16_t kCursor = 1, kBitmap = 2, kIcon = 3, kRcData = 10, kGroupCursor = 12, kGroupIcon = 14;

class Module {
public:
    bool load(std::vector<uint8_t> bytes, std::string* error);
    bool loaded() const { return !bytes_.empty(); }

    // A resource by type and name; numeric names are written "#12". Empty when absent.
    std::span<const uint8_t> resource(uint16_t type, const std::string& name) const;
    std::span<const uint8_t> resource(uint16_t type, uint16_t id) const { return resource(type, "#" + std::to_string(id)); }
    // Code/data bytes of segment `number` (1-based), without relocations applied.
    std::span<const uint8_t> segment(int number) const;
    int segmentCount() const { return int(segments_.size()); }

private:
    struct Resource {
        uint16_t type;
        std::string name;  // upper case, or "#id"
        uint32_t offset, length;
    };
    struct Segment {
        uint32_t offset, length;
    };
    std::vector<uint8_t> bytes_;
    std::vector<Resource> resources_;
    std::vector<Segment> segments_;
};

// HICON/HCURSOR from a Win16 icon or cursor group (the resource formats match Win32's).
// `size` = preferred icon size (0 = the group's first entry).
void* createIcon(const Module& module, const std::string& groupName, bool cursor, int size = 0);

}  // namespace ne
