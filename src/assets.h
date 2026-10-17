// Loaded original resources: the data folder, the three executables and cached bitmaps.
#pragma once

#include <map>
#include <memory>
#include <string>

#include "dfm.h"
#include "gamefiles.h"
#include "gfx.h"
#include "ne.h"

class Assets {
public:
    bool open(const std::wstring& dataDirectory, std::wstring* error);
    const gamefiles::Store& files() const { return files_; }

    // A bitmap data file ("a920.asl", "art\\cb0111.bmp"), cached. Never null: a missing
    // file gives an empty 1x1 surface (and is logged once).
    const Surface& bitmap(const std::string& path);
    // A form from CRAYONST.EXE ("TSPLASH") or a level's executable (1 or 2).
    const dfm::Component* form(int exe, const std::string& name);
    const ne::Module& exe(int exe) const { return exes_[exe]; }  // 0 = CRAYONST, 1, 2
    const std::vector<COLORREF>& colours() const { return colours_; }

private:
    gamefiles::Store files_;
    ne::Module exes_[3];
    std::vector<COLORREF> colours_;
    std::map<std::string, Surface> bitmaps_;
    std::map<std::string, std::unique_ptr<dfm::Component>> forms_;
};
