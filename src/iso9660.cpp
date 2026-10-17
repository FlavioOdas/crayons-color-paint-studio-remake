#include "iso9660.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace iso9660 {
namespace {

constexpr uint32_t kSector = 2048;

uint32_t le32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24; }

std::string upper(std::string s) {
    for (char& c : s) c = char(std::toupper(uint8_t(c)));
    return s;
}

}  // namespace

Image::~Image() {
    if (file_) fclose(file_);
}

bool Image::open(const std::wstring& path, std::string* error) {
    file_ = _wfopen(path.c_str(), L"rb");
    if (!file_) return *error = "The disc image could not be opened.", false;
    // The primary volume descriptor is sector 16: "\1CD001". Try cooked then raw sectors.
    for (auto [size, offset] : {std::pair{2048u, 0u}, std::pair{2352u, 16u}}) {
        sectorSize_ = size, dataOffset_ = offset;
        std::vector<uint8_t> pvd;
        if (readSectors(16, kSector, &pvd) && pvd[0] == 1 && std::memcmp(&pvd[1], "CD001", 5) == 0) {
            root_.extent = le32(&pvd[156 + 2]);
            root_.size = le32(&pvd[156 + 10]);
            root_.directory = true;
            return true;
        }
    }
    return *error = "This is not a CD image (no ISO 9660 volume found).", false;
}

bool Image::readSectors(uint32_t lba, uint32_t bytes, std::vector<uint8_t>* out) const {
    out->resize(bytes);
    for (uint32_t done = 0; done < bytes; ++lba) {
        const uint32_t n = std::min(kSector, bytes - done);
        if (_fseeki64(file_, int64_t(lba) * sectorSize_ + dataOffset_, SEEK_SET) != 0 ||
            fread(out->data() + done, 1, n, file_) != n)
            return false;
        done += n;
    }
    return true;
}

std::vector<Image::Record> Image::directory(const Record& dir) const {
    std::vector<Record> records;
    std::vector<uint8_t> d;
    if (!readSectors(dir.extent, dir.size, &d)) return records;
    for (size_t p = 0; p < d.size();) {
        const uint8_t length = d[p];
        if (length == 0) {  // records never cross a sector boundary
            p = (p / kSector + 1) * kSector;
            continue;
        }
        if (p + length > d.size() || length < 34) break;
        const uint8_t* r = &d[p];
        Record rec;
        rec.extent = le32(r + 2), rec.size = le32(r + 10), rec.directory = r[25] & 2;
        rec.name.assign(reinterpret_cast<const char*>(r + 33), r[32]);
        p += length;
        if (rec.name.size() == 1 && uint8_t(rec.name[0]) <= 1) continue;  // "." and ".."
        rec.name = upper(rec.name.substr(0, rec.name.find(';')));
        if (!rec.name.empty() && rec.name.back() == '.') rec.name.pop_back();
        records.push_back(std::move(rec));
    }
    return records;
}

bool Image::read(const std::string& path, std::vector<uint8_t>* out) const {
    Record current = root_;
    std::string rest = upper(path);
    std::replace(rest.begin(), rest.end(), '/', '\\');
    while (!rest.empty()) {
        const size_t slash = rest.find('\\');
        const std::string part = rest.substr(0, slash);
        rest = slash == std::string::npos ? "" : rest.substr(slash + 1);
        bool found = false;
        for (const Record& r : directory(current)) {
            if (r.name == part) {
                current = r, found = true;
                break;
            }
        }
        if (!found || (current.directory != !rest.empty())) return false;
    }
    return !current.directory && readSectors(current.extent, current.size, out);
}

std::vector<std::string> Image::rootFiles() const {
    std::vector<std::string> names;
    for (const Record& r : directory(root_))
        if (!r.directory) names.push_back(r.name);
    return names;
}

}  // namespace iso9660
