#include "assets.h"

#include <cctype>
#include <cstdio>

namespace {

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(uint8_t(c)));
    return s;
}

}  // namespace

bool Assets::open(const std::wstring& dataDirectory, std::wstring* error) {
    files_.open(dataDirectory);
    static const char* const kExes[] = {"CRAYONST.EXE", "CRAYON1.EXE", "CRAYON2.EXE"};
    for (int i = 0; i < 3; ++i) {
        std::string message;
        if (!exes_[i].load(files_.read(kExes[i]), &message))
            return *error = L"Cannot read " + std::wstring(kExes[i], kExes[i] + strlen(kExes[i])) + L" in " + dataDirectory, false;
    }
    colours_ = programColours(exes_[1]);
    if (colours_.size() != 211) return *error = L"CRAYON1.EXE is not the version this remake knows.", false;
    // The CD's fonts, private to this process (the original installer added them to Windows).
    for (const char* font : {"CRAYON__.TTF", "CREEPER_.TTF", "MEXACALI.TTF", "TREEHOUS.TTF", "WALTERS_.TTF"}) {
        std::vector<uint8_t> data = files_.read(std::string("FONTS\\") + font);
        DWORD count = 0;
        if (!data.empty()) AddFontMemResourceEx(data.data(), DWORD(data.size()), nullptr, &count);
    }
    return true;
}

const Surface& Assets::bitmap(const std::string& path) {
    const std::string key = lower(path);
    auto it = bitmaps_.find(key);
    if (it != bitmaps_.end()) return it->second;
    Surface s;
    if (!loadBmp(files_.read(key), &s)) {
        std::fprintf(stderr, "assets: missing bitmap %s\n", key.c_str());
        s.create(1, 1);
    }
    return bitmaps_.emplace(key, std::move(s)).first->second;
}

const dfm::Component* Assets::form(int exe, const std::string& name) {
    const std::string key = std::to_string(exe) + ":" + name;
    auto it = forms_.find(key);
    if (it != forms_.end()) return it->second.get();
    std::string error;
    auto form = dfm::parse(exes_[exe].resource(ne::kRcData, name), &error);
    if (!form) std::fprintf(stderr, "assets: form %s: %s\n", key.c_str(), error.c_str());
    return forms_.emplace(key, std::move(form)).first->second.get();
}
