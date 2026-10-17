#include "gamefiles.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "installer.h"
#include "iso9660.h"

namespace gamefiles {
namespace {

// The fonts the original installer added to Windows (CD root, docs/formats/data.md).
const char* const kFonts[] = {"CRAYON__.TTF", "CREEPER_.TTF", "MEXACALI.TTF", "TREEHOUS.TTF", "WALTERS_.TTF"};
// A file from each part of the data; a compact installation lacks the ART/SND ones.
const char* const kSentinels[] = {"CRAYONST.EXE", "CRAYON1.EXE", "CRAYON2.EXE", "A1.ASL", "A950.ASL", "ACTIVITY\\A1400.ASL", "ART\\CB0111.BMP",
                                  "ART\\LEVEL1.BMP", "SND\\PC1.MID", "SND\\HV100.ASL"};

std::wstring widen(const std::string& s) { return std::wstring(s.begin(), s.end()); }


bool endsWith(const std::string& s, const std::string& tail) {
    return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

bool readWhole(const std::wstring& path, std::vector<uint8_t>* out) {
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return false;
    _fseeki64(f, 0, SEEK_END);
    const int64_t size = _ftelli64(f);
    _fseeki64(f, 0, SEEK_SET);
    out->resize(size_t(size));
    const bool ok = fread(out->data(), 1, out->size(), f) == out->size();
    fclose(f);
    return ok;
}

bool writeWhole(const std::wstring& path, const std::vector<uint8_t>& data) {
    SHCreateDirectoryExW(nullptr, path.substr(0, path.find_last_of(L'\\')).c_str(), nullptr);
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) return false;
    const bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}


bool isArchive(const std::vector<uint8_t>& head) {
    return head.size() >= 4 && head[0] == 0x13 && head[1] == 0x5d && head[2] == 0x65 && head[3] == 0x8c;
}

bool importArchive(const std::vector<uint8_t>& raw, const std::wstring& target, const Progress& progress,
                   size_t extra, std::wstring* error) {
    std::vector<installer::Entry> entries;
    std::string message;
    if (!installer::list(raw, &entries, &message)) return *error = widen(message), false;
    const size_t total = entries.size() + extra;
    size_t done = 0;
    for (const installer::Entry& e : entries) {
        if (progress) progress(done, total);
        ++done;
        std::vector<uint8_t> data;
        if (!installer::extract(raw, e, &data, &message))
            return *error = widen(e.name + ": " + message), false;
        const std::wstring path = target + L"\\" + (e.folder.empty() ? L"" : widen(e.folder) + L"\\") + widen(e.name);
        if (!writeWhole(path, data)) return *error = L"Could not write " + path, false;
    }
    return true;
}

bool fromImage(const std::wstring& source, const std::wstring& target, const Progress& progress, std::wstring* error) {
    iso9660::Image image;
    std::string message;
    if (!image.open(source, &message)) return *error = widen(message), false;
    std::vector<uint8_t> archive;
    if (!image.read("CRAYONS.Z", &archive))
        return *error = L"This CD image has no CRAYONS.Z: it does not look like the Crayons CD.", false;
    if (!importArchive(archive, target, progress, std::size(kFonts), error)) return false;
    for (const char* font : kFonts) {
        std::vector<uint8_t> data;
        if (image.read(font, &data)) writeWhole(target + L"\\FONTS\\" + widen(font), data);
    }
    return true;
}

// CRAYONS.Z with the fonts beside it (a CD drive, or a copy of the CD's files).
bool fromArchive(const std::wstring& archivePath, const std::wstring& target, const Progress& progress,
                 std::wstring* error) {
    std::vector<uint8_t> raw;
    if (!readWhole(archivePath, &raw)) return *error = L"Could not read " + archivePath, false;
    if (!importArchive(raw, target, progress, 0, error)) return false;
    const std::wstring dir = archivePath.substr(0, archivePath.find_last_of(L"\\/") + 1);
    for (const char* font : kFonts) {
        std::vector<uint8_t> data;
        if (readWhole(dir + widen(font), &data)) writeWhole(target + L"\\FONTS\\" + widen(font), data);
    }
    return true;
}

void collect(const std::wstring& dir, const std::wstring& rel, std::vector<std::wstring>* files) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\" + rel + L"*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        const std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) collect(dir, rel + name + L"\\", files);
        else files->push_back(rel + name);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

bool fromInstallation(const std::wstring& dir, const std::wstring& target, const Progress& progress,
                      std::wstring* error) {
    if (!isComplete(dir))
        return *error = L"This Crayons folder is a compact installation (pictures and sounds stay on the CD). "
                        L"Please choose the CD or its image instead.", false;
    std::vector<std::wstring> files;
    collect(dir, L"", &files);
    for (size_t i = 0; i < files.size(); ++i) {
        if (progress) progress(i, files.size());
        const std::wstring& rel = files[i];
        std::vector<uint8_t> data;
        if (!readWhole(dir + L"\\" + rel, &data) || !writeWhole(target + L"\\" + rel, data))
            return *error = L"Could not copy " + rel, false;
    }
    wchar_t windows[MAX_PATH];
    GetWindowsDirectoryW(windows, MAX_PATH);
    for (const char* font : kFonts) {  // installed by the original setup
        std::vector<uint8_t> data;
        if (readWhole(std::wstring(windows) + L"\\Fonts\\" + widen(font), &data))
            writeWhole(target + L"\\FONTS\\" + widen(font), data);
    }
    return true;
}

std::wstring cueToBin(const std::wstring& cue) {
    std::vector<uint8_t> text;
    if (!readWhole(cue, &text)) return cue;
    const std::string s(text.begin(), text.end());
    const size_t at = s.find("FILE \"");
    if (at == std::string::npos) return cue;
    const size_t end = s.find('"', at + 6);
    return cue.substr(0, cue.find_last_of(L"\\/") + 1) + widen(s.substr(at + 6, end - at - 6));
}

}  // namespace

std::wstring dataDirectory() {
    wchar_t* appdata = nullptr;
    std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) dir = appdata;
    CoTaskMemFree(appdata);
    return dir + L"\\Crayons\\data";
}

std::wstring portableDataDirectory() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
        if (!length) return L"";
        if (length < path.size() - 1) {
            path.resize(length);
            break;
        }
        path.resize(path.size() * 2);
    }
    const size_t separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? L"" : path.substr(0, separator) + L"\\data";
}

bool isComplete(const std::wstring& directory) {
    for (const char* f : kSentinels)
        if (GetFileAttributesW((directory + L"\\" + widen(f)).c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    return true;
}

bool import(const std::wstring& sourceIn, const std::wstring& target, const Progress& progress, std::wstring* error) {
    std::wstring source = sourceIn;
    while (source.size() > 3 && (source.back() == L'\\' || source.back() == L'/')) source.pop_back();
    const DWORD attributes = GetFileAttributesW(source.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return *error = L"Not found: " + source, false;
    bool ok = false;
    if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
        const std::wstring dir = source.back() == L'\\' ? source : source + L"\\";
        if (GetFileAttributesW((dir + L"CRAYONS.Z").c_str()) != INVALID_FILE_ATTRIBUTES)
            ok = fromArchive(dir + L"CRAYONS.Z", target, progress, error);
        else
            ok = fromInstallation(dir.substr(0, dir.size() - 1), target, progress, error);
    } else {
        std::string lower(source.begin(), source.end());
        for (char& c : lower) c = char(std::tolower(uint8_t(c)));
        if (endsWith(lower, ".cue")) source = cueToBin(source);
        std::vector<uint8_t> head(4);
        FILE* f = _wfopen(source.c_str(), L"rb");
        if (!f) return *error = L"Could not open " + source, false;
        head.resize(fread(head.data(), 1, 4, f));
        fclose(f);
        if (isArchive(head)) {
            ok = fromArchive(source, target, progress, error);
        } else if (endsWith(lower, ".exe") || endsWith(lower, ".ini") || endsWith(lower, ".asl")) {
            ok = fromInstallation(source.substr(0, source.find_last_of(L"\\/")), target, progress, error);
        } else {
            ok = fromImage(source, target, progress, error);
        }
    }
    if (ok && !isComplete(target)) return *error = L"Some of the game's files are missing after the import.", false;
    if (ok && progress) progress(1, 1);
    return ok;
}

bool Store::exists(const std::string& path) const {
    return GetFileAttributesW(fullPath(path).c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::vector<uint8_t> Store::read(const std::string& path) const {
    std::vector<uint8_t> data;
    if (!readWhole(fullPath(path), &data)) data.clear();
    return data;
}

std::wstring Store::fullPath(const std::string& path) const {
    std::wstring p = widen(path);
    std::replace(p.begin(), p.end(), L'/', L'\\');
    return root_ + L"\\" + p;
}

}  // namespace gamefiles
