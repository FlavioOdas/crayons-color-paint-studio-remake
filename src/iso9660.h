// Reads files from the player's CD image: a plain .iso (2048-byte sectors) or a raw .bin
// (2352-byte MODE1 sectors, as in CRAYONS.bin/.cue). ISO 9660 level 1 names, no Joliet needed.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace iso9660 {

class Image {
public:
    Image() = default;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    ~Image();

    bool open(const std::wstring& path, std::string* error);
    // `path` uses / or \ separators and is case-insensitive ("CRAYONS.Z", "ART\\CB0111.BMP").
    bool read(const std::string& path, std::vector<uint8_t>* out) const;
    // Names of the files in the root directory.
    std::vector<std::string> rootFiles() const;

private:
    struct Record {
        std::string name;
        uint32_t extent = 0, size = 0;
        bool directory = false;
    };
    bool readSectors(uint32_t lba, uint32_t bytes, std::vector<uint8_t>* out) const;
    std::vector<Record> directory(const Record& dir) const;

    FILE* file_ = nullptr;
    uint32_t sectorSize_ = 2048, dataOffset_ = 0;
    Record root_;
};

}  // namespace iso9660
