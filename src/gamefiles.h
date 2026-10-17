// The player's original Crayons files: finding them, importing them once from the CD (image,
// drive or installer archive) into %APPDATA%\Crayons\data, and reading them at run time.
// Paths are the original program's relative paths ("A1.ASL", "ART\CB0111.BMP", "SND\PC1.MID");
// the CD's fonts are kept as "FONTS\<name>.TTF".
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace gamefiles {

// %APPDATA%\Crayons\data (not created).
std::wstring dataDirectory();

// data/ beside this executable, for a private portable copy (not created).
std::wstring portableDataDirectory();

// True when `directory` holds a full set of the files the remake needs.
bool isComplete(const std::wstring& directory);

using Progress = std::function<void(size_t done, size_t total)>;

// Copies the program's data out of `source` into `target`: a CD image (.iso, .bin or its
// .cue), a CD drive or folder holding CRAYONS.Z, CRAYONS.Z itself, or a full installation
// folder. The original executables are kept: their resources (form layouts, pictures, icons,
// cursors) are part of the game data; they are never run. Returns false with a message.
bool import(const std::wstring& source, const std::wstring& target, const Progress& progress, std::wstring* error);

class Store {
public:
    void open(std::wstring directory) { root_ = std::move(directory); }
    const std::wstring& root() const { return root_; }
    bool exists(const std::string& path) const;
    std::vector<uint8_t> read(const std::string& path) const;  // empty when missing
    std::wstring fullPath(const std::string& path) const;

private:
    std::wstring root_;
};

}  // namespace gamefiles
