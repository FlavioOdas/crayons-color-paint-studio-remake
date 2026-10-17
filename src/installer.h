// Reads the original Crayons installer archives (CRAYONS.Z, ICON.Z): InstallShield 3 with
// PKWARE DCL "implode" compression. Ported from the CyberPest remake; format notes in
// docs/formats/data.md, reference version tools/is3x.py.
#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace installer {

struct Entry {
    std::string folder;  // "" or e.g. "ART" (backslash-separated, as stored)
    std::string name;    // "CB0111.BMP"
    uint32_t size = 0, packedSize = 0, offset = 0;
};

// Lists the archive's files in table order. Returns false with a message on failure.
bool list(std::span<const uint8_t> archive, std::vector<Entry>* entries, std::string* error);

// Decompresses one listed file.
bool extract(std::span<const uint8_t> archive, const Entry& entry, std::vector<uint8_t>* out, std::string* error);

}  // namespace installer
