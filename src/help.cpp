#include "help.h"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace help {
namespace {

using Bytes = std::span<const uint8_t>;

uint16_t word(Bytes bytes, size_t offset) {
    return offset + 2 <= bytes.size() ? uint16_t(bytes[offset] | bytes[offset + 1] << 8) : 0;
}

uint32_t dword(Bytes bytes, size_t offset) {
    return uint32_t(word(bytes, offset)) | uint32_t(word(bytes, offset + 2)) << 16;
}

void appendWord(std::string* out, uint16_t value) {
    out->push_back(char(value));
    out->push_back(char(value >> 8));
}

void appendDword(std::string* out, uint32_t value) {
    appendWord(out, uint16_t(value));
    appendWord(out, uint16_t(value >> 16));
}

bool writeFile(const std::wstring& path, const std::string& contents) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const bool okay = WriteFile(file, contents.data(), DWORD(contents.size()), &written, nullptr) && written == contents.size();
    CloseHandle(file);
    return okay;
}

std::string escape(Bytes bytes) {
    std::string out;
    for (uint8_t byte : bytes) {
        if (byte == '&') out += "&amp;";
        else if (byte == '<') out += "&lt;";
        else if (byte == '>') out += "&gt;";
        else if (byte == '"') out += "&quot;";
        else if (byte >= 32 || byte == '\t') out += char(byte);
    }
    return out;
}

std::string escape(const std::string& value) {
    return escape(Bytes(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
}

bool compressedUnsigned(Bytes bytes, size_t* cursor, uint32_t* value) {
    if (*cursor >= bytes.size()) return false;
    if (bytes[*cursor] & 1) {
        if (*cursor + 2 > bytes.size()) return false;
        *value = word(bytes, *cursor) >> 1;
        *cursor += 2;
    } else *value = bytes[(*cursor)++] >> 1;
    return true;
}

bool compressedUnsignedLong(Bytes bytes, size_t* cursor, uint32_t* value) {
    if (*cursor >= bytes.size()) return false;
    if (bytes[*cursor] & 1) {
        if (*cursor + 4 > bytes.size()) return false;
        *value = dword(bytes, *cursor) >> 1;
        *cursor += 4;
    } else {
        if (*cursor + 2 > bytes.size()) return false;
        *value = word(bytes, *cursor) >> 1;
        *cursor += 2;
    }
    return true;
}

bool compressedLong(Bytes bytes, size_t* cursor, int32_t* value) {
    if (*cursor >= bytes.size()) return false;
    if (bytes[*cursor] & 1) {
        if (*cursor + 4 > bytes.size()) return false;
        *value = int32_t((dword(bytes, *cursor) - 0x80000000u) / 2);
        *cursor += 4;
    } else {
        if (*cursor + 2 > bytes.size()) return false;
        *value = (int32_t(word(bytes, *cursor)) - 0x8000) / 2;
        *cursor += 2;
    }
    return true;
}

class Archive {
public:
    explicit Archive(Bytes data) : data_(data) {}

    bool readDirectory() {
        if (dword(data_, 0) != 0x00035f3f) return false;
        Bytes directory = fileAt(dword(data_, 4));
        if (directory.size() < 38 || word(directory, 0) != 0x293b) return false;
        const uint16_t pageSize = word(directory, 4);
        const uint16_t pageCount = word(directory, 30);
        const uint16_t levels = word(directory, 32);
        if (!pageSize || !pageCount || !levels || pageSize > 8192) return false;
        const Bytes pages = directory.subspan(38);
        int page = word(directory, 26);
        for (int level = 1; level < levels; ++level) {
            if (size_t(page + 1) * pageSize > pages.size()) return false;
            page = int16_t(word(pages.subspan(size_t(page) * pageSize), 4));
        }
        std::set<int> seen;
        while (page >= 0 && page < pageCount && seen.insert(page).second) {
            if (size_t(page + 1) * pageSize > pages.size()) return false;
            const Bytes leaf = pages.subspan(size_t(page) * pageSize, pageSize);
            const int count = word(leaf, 2);
            size_t cursor = 8;
            for (int i = 0; i < count; ++i) {
                size_t end = cursor;
                while (end < leaf.size() && leaf[end]) ++end;
                if (end + 5 > leaf.size()) return false;
                const std::string name(reinterpret_cast<const char*>(leaf.data() + cursor), end - cursor);
                files_[name] = dword(leaf, end + 1);
                cursor = end + 5;
            }
            page = int16_t(word(leaf, 6));
        }
        return !files_.empty();
    }

    Bytes file(const std::string& name) const {
        const auto found = files_.find(name);
        return found == files_.end() ? Bytes{} : fileAt(found->second);
    }

    std::map<uint32_t, uint32_t> context() const {
        std::map<uint32_t, uint32_t> out;
        const Bytes tree = file("|CONTEXT");
        if (tree.size() < 38 || word(tree, 0) != 0x293b) return out;
        const uint16_t pageSize = word(tree, 4), pageCount = word(tree, 30);
        if (!pageSize || !pageCount) return out;
        const Bytes pages = tree.subspan(38);
        int page = word(tree, 26);
        for (int level = 1; level < word(tree, 32); ++level) {
            if (size_t(page + 1) * pageSize > pages.size()) return out;
            page = int16_t(word(pages.subspan(size_t(page) * pageSize), 4));
        }
        std::set<int> seen;
        while (page >= 0 && page < pageCount && seen.insert(page).second) {
            if (size_t(page + 1) * pageSize > pages.size()) break;
            const Bytes leaf = pages.subspan(size_t(page) * pageSize, pageSize);
            const int count = word(leaf, 2);
            for (int i = 0; i < count && 16 + size_t(i) * 8 <= leaf.size(); ++i)
                out[dword(leaf, 8 + size_t(i) * 8)] = dword(leaf, 12 + size_t(i) * 8);
            page = int16_t(word(leaf, 6));
        }
        return out;
    }

private:
    Bytes fileAt(uint32_t offset) const {
        if (offset + 9 > data_.size()) return {};
        const uint32_t size = dword(data_, offset + 4);
        if (size > data_.size() - offset - 9) return {};
        return data_.subspan(offset + 9, size);
    }

    Bytes data_;
    std::map<std::string, uint32_t> files_;
};

struct Record {
    uint8_t type = 0;
    Bytes format, text;
    uint32_t position = 0, block = 0;
    int page = -1;
};

struct Page {
    std::string title, body;
    uint32_t offset = 0;
};

std::vector<uint8_t> topicStream(Bytes file) {
    std::vector<uint8_t> out;
    for (size_t block = 0; block < file.size(); block += 4096) {
        if (file.size() - block < 12) break;
        const size_t end = std::min(block + 4096, file.size());
        out.insert(out.end(), file.begin() + block + 12, file.begin() + end);
    }
    return out;
}

bool records(Bytes stream, std::vector<Record>* out, std::vector<Page>* pages) {
    uint32_t position = 12, oldBlock = UINT32_MAX, offset = 0;
    std::set<uint32_t> seen;
    while (position != 0xffffffffu && seen.insert(position).second && out->size() < 100000) {
        if (position < 12) return false;
        const uint32_t block = (position - 12) >> 14;
        const uint32_t inBlock = (position - 12) & 0x3fff;
        const size_t start = size_t(block) * 4084 + inBlock;
        if (start + 21 > stream.size()) break;
        const uint32_t length = dword(stream, start), dataOffset = dword(stream, start + 16);
        if (length < 21 || start + length > stream.size() || dataOffset < 21 || dataOffset > length) return false;
        if (block != oldBlock) oldBlock = block, offset = 0;
        const Bytes record = stream.subspan(start, length);
        const uint8_t type = record[20];
        const uint32_t textSize = dword(record, 4);
        Bytes text = record.subspan(dataOffset);
        if (textSize <= text.size()) text = text.first(textSize);
        else return false;
        const Bytes format = record.subspan(21, dataOffset - 21);
        if (type == 2) {
            std::string title(reinterpret_cast<const char*>(text.data()), text.size());
            const size_t end = title.find('\0');
            if (end != std::string::npos) title.resize(end);
            pages->push_back({title, "", block * 0x8000 + offset});
        } else if (type == 0x20 || type == 0x23) {
            size_t cursor = 0;
            int32_t ignored = 0;
            uint32_t paragraphLength = 0;
            if (!compressedLong(format, &cursor, &ignored) ||
                !compressedUnsigned(format, &cursor, &paragraphLength)) return false;
            offset += paragraphLength;
        }
        out->push_back({type, format, text, position, block, int(pages->size()) - 1});
        position = dword(record, 12);
    }
    return !pages->empty();
}

std::string pageLink(uint32_t hash, const std::map<uint32_t, uint32_t>& context, const std::vector<Page>& pages) {
    const auto found = context.find(hash);
    if (found == context.end()) return {};
    const auto page = std::upper_bound(pages.begin(), pages.end(), found->second,
                                     [](uint32_t value, const Page& entry) { return value < entry.offset; });
    if (page == pages.begin()) return {};
    return "#topic-" + std::to_string(size_t(page - pages.begin() - 1));
}

std::string paragraph(const Record& record, const std::map<uint32_t, uint32_t>& context,
                      const std::vector<Page>& pages) {
    size_t format = 0, text = 0;
    int32_t ignored = 0;
    uint32_t paragraphLength = 0;
    if (!compressedLong(record.format, &format, &ignored) ||
        !compressedUnsigned(record.format, &format, &paragraphLength)) return {};
    int columns = 1;
    if (record.type == 0x23) {
        if (format + 2 > record.format.size()) return {};
        columns = record.format[format++];
        const uint8_t tableType = record.format[format++];
        if (tableType == 0 || tableType == 2) format += 2;
        format += size_t(columns) * 4;
        if (columns < 1 || columns > 40) return {};
    }
    std::string out = "<p>";
    bool linkOpen = false;
    for (int column = 0; column < columns && format < record.format.size(); ++column) {
        if (record.type == 0x23) {
            if (format + 5 > record.format.size()) break;
            if (int16_t(word(record.format, format)) == -1) break;
            format += 5;
            if (column) out += " &nbsp; ";
        }
        if (format + 6 > record.format.size()) break;
        format += 4;
        const uint16_t bits = word(record.format, format);
        format += 2;
        if (bits & 0x0001) format += 4;
        for (uint16_t bit : {0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040}) if (bits & bit) format += 2;
        if (bits & 0x0100) format += 3;
        if (bits & 0x0200) {
            if (format + 2 > record.format.size()) break;
            const int tabs = word(record.format, format);
            format += 2;
            for (int tab = 0; tab < tabs && format + 2 <= record.format.size(); ++tab) {
                const uint16_t value = word(record.format, format);
                format += 2 + ((value & 0x4000) ? 2 : 0);
            }
        }
        while (format < record.format.size() && text < record.text.size()) {
            size_t end = text;
            while (end < record.text.size() && record.text[end]) ++end;
            out += escape(record.text.subspan(text, end - text));
            text = std::min(end + 1, record.text.size());
            if (format >= record.format.size()) break;
            const uint8_t command = record.format[format];
            if (command == 0xff) { ++format; break; }
            if (command == 0x80) format += 3;
            else if (command == 0x81) ++format, out += "<br>";
            else if (command == 0x82) ++format, out += "<br>";
            else if (command == 0x83) ++format, out += " &nbsp; ";
            else if (command == 0x89) {
                ++format;
                if (linkOpen) out += "</a>", linkOpen = false;
            } else if (command == 0x8b) ++format, out += "&nbsp;";
            else if (command == 0x8c) ++format, out += "&#8209;";
            else if (command >= 0xe0 && command <= 0xe7 && format + 5 <= record.format.size()) {
                const std::string target = pageLink(dword(record.format, format + 1), context, pages);
                if (linkOpen) out += "</a>";
                linkOpen = !target.empty();
                if (linkOpen) out += "<a href=\"" + target + "\">";
                format += 5;
            } else if (command >= 0x86 && command <= 0x88 && format + 2 <= record.format.size()) {
                const uint8_t kind = record.format[format + 1];
                format += 2;
                int32_t size = 0;
                if (!compressedLong(record.format, &format, &size) || size < 0) break;
                if (kind == 0x22) {
                    uint32_t ignoredHotspot = 0;
                    if (!compressedUnsigned(record.format, &format, &ignoredHotspot)) break;
                }
                if (format + size > record.format.size()) break;
                if ((kind == 0x03 || kind == 0x22) && size >= 4 && word(record.format, format) == 0) {
                    const uint16_t index = word(record.format, format + 2);
                    out += "<img src=\"bm" + std::to_string(index) + ".bmp\" alt=\"Help illustration\">";
                }
                format += size;
            } else if ((command == 0xc8 || command == 0xcc || command >= 0xea) &&
                       format + 3 <= record.format.size()) format += 3 + word(record.format, format + 1);
            else if (command == 0x20) format += 5;
            else if (command == 0x21) format += 3;
            else ++format;
        }
    }
    if (linkOpen) out += "</a>";
    out += "</p>\n";
    return out;
}

std::vector<uint8_t> unpackRle(Bytes input, size_t expected) {
    std::vector<uint8_t> out;
    out.reserve(expected);
    for (size_t at = 0; at < input.size() && out.size() < expected;) {
        const uint8_t code = input[at++];
        const size_t count = code & 0x7f;
        if (count > expected - out.size()) return {};
        if (code & 0x80) {
            if (at + count > input.size()) return {};
            out.insert(out.end(), input.begin() + at, input.begin() + at + count);
            at += count;
        } else {
            if (at >= input.size()) return {};
            out.insert(out.end(), count, input[at++]);
        }
    }
    return out.size() == expected ? out : std::vector<uint8_t>{};
}

std::string bmpHeader(int width, int height, int bits, Bytes palette, size_t pixels) {
    std::string out = "BM";
    appendDword(&out, uint32_t(54 + palette.size() + pixels));
    appendDword(&out, 0);
    appendDword(&out, uint32_t(54 + palette.size()));
    appendDword(&out, 40);
    appendDword(&out, uint32_t(width));
    appendDword(&out, uint32_t(height));
    appendWord(&out, 1);
    appendWord(&out, uint16_t(bits));
    appendDword(&out, BI_RGB);
    appendDword(&out, uint32_t(pixels));
    appendDword(&out, 0), appendDword(&out, 0);
    appendDword(&out, uint32_t(palette.size() / 4));
    appendDword(&out, 0);
    return out;
}

bool exportBitmap(Bytes file, const std::wstring& path) {
    if (file.size() < 12 || word(file, 0) != 0x706c || word(file, 2) < 1) return false;
    const uint32_t start = dword(file, 4);
    if (start + 2 > file.size() || file[start] != 6) return false;
    const uint8_t packing = file[start + 1];
    Bytes image = file.subspan(start);
    size_t cursor = 2;
    uint32_t ignored = 0, planes = 0, bits = 0, width = 0, height = 0, colourCount = 0;
    for (int i = 0; i < 2; ++i) if (!compressedUnsignedLong(image, &cursor, &ignored)) return false;
    if (!compressedUnsigned(image, &cursor, &planes) || !compressedUnsigned(image, &cursor, &bits) ||
        !compressedUnsignedLong(image, &cursor, &width) || !compressedUnsignedLong(image, &cursor, &height) ||
        !compressedUnsignedLong(image, &cursor, &colourCount)) return false;
    if (!compressedUnsignedLong(image, &cursor, &ignored)) return false;
    uint32_t compressedSize = 0, hotspotSize = 0;
    if (!compressedUnsignedLong(image, &cursor, &compressedSize) ||
        !compressedUnsignedLong(image, &cursor, &hotspotSize) || cursor + 8 > image.size()) return false;
    const uint32_t dataOffset = dword(image, cursor);
    cursor += 8;
    if (planes != 1 || !width || !height || width > 4096 || height > 4096 ||
        (bits != 1 && bits != 4 && bits != 8 && bits != 24)) return false;
    const size_t colours = colourCount ? colourCount : bits <= 8 ? 1u << bits : 0;
    if (cursor + colours * 4 > image.size() || dataOffset + compressedSize > image.size()) return false;
    const Bytes palette = image.subspan(cursor, colours * 4);
    const size_t expected = ((width * bits + 31) / 32) * 4 * height;
    if (expected > 32 * 1024 * 1024) return false;
    Bytes packed = image.subspan(dataOffset, compressedSize);
    std::vector<uint8_t> decoded;
    if (packing == 0) decoded.assign(packed.begin(), packed.end());
    else if (packing == 1) decoded = unpackRle(packed, expected);
    if (decoded.size() != expected) return false;
    std::string out = bmpHeader(int(width), int(height), int(bits), palette, decoded.size());
    out.append(reinterpret_cast<const char*>(palette.data()), palette.size());
    out.append(reinterpret_cast<const char*>(decoded.data()), decoded.size());
    return writeFile(path, out);
}

bool exportMetaFile(Bytes file, const std::wstring& path) {
    if (file.size() < 12 || word(file, 0) != 0x706c) return false;
    const uint32_t start = dword(file, 4);
    if (start + 2 > file.size() || file[start] != 8) return false;
    const uint8_t packing = file[start + 1];
    const Bytes image = file.subspan(start);
    size_t cursor = 2;
    uint32_t mapMode = 0, decodedSize = 0, packedSize = 0, hotspot = 0;
    if (!compressedUnsigned(image, &cursor, &mapMode) || cursor + 4 > image.size()) return false;
    const int width = word(image, cursor), height = word(image, cursor + 2);
    cursor += 4;
    if (!compressedUnsignedLong(image, &cursor, &decodedSize) ||
        !compressedUnsignedLong(image, &cursor, &packedSize) ||
        !compressedUnsignedLong(image, &cursor, &hotspot) || cursor + 8 > image.size()) return false;
    const uint32_t dataOffset = dword(image, cursor);
    if (width < 1 || height < 1 || width > 4096 || height > 4096 ||
        decodedSize > 32 * 1024 * 1024 || dataOffset + packedSize > image.size()) return false;
    const Bytes packed = image.subspan(dataOffset, packedSize);
    std::vector<uint8_t> decoded;
    if (packing == 0) decoded.assign(packed.begin(), packed.end());
    else if (packing == 1) decoded = unpackRle(packed, decodedSize);
    if (decoded.size() != decodedSize) return false;
    HMETAFILE metafile = SetMetaFileBitsEx(UINT(decoded.size()), decoded.data());
    if (!metafile) return false;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void* pixels = nullptr;
    HDC screen = GetDC(nullptr), dc = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!bitmap) { DeleteMetaFile(metafile); DeleteDC(dc); ReleaseDC(nullptr, screen); return false; }
    HGDIOBJ old = SelectObject(dc, bitmap);
    RECT area{0, 0, width, height};
    FillRect(dc, &area, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    SetMapMode(dc, MM_ANISOTROPIC);
    SetWindowExtEx(dc, width, height, nullptr);
    SetViewportExtEx(dc, width, height, nullptr);
    const bool drawn = PlayMetaFile(dc, metafile) != 0;
    GdiFlush();
    std::string out;
    if (drawn) {
        const size_t pixelSize = size_t(width) * height * 4;
        out = bmpHeader(width, -height, 32, {}, pixelSize);
        out.append(static_cast<const char*>(pixels), pixelSize);
    }
    SelectObject(dc, old);
    DeleteObject(bitmap);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    DeleteMetaFile(metafile);
    return drawn && writeFile(path, out);
}

}  // namespace

std::wstring exportHtml(const gamefiles::Store& files, std::wstring* error) {
    const std::vector<uint8_t> file = files.read("CRAYONS.HLP");
    Archive archive(file);
    if (!archive.readDirectory()) {
        *error = L"The original CRAYONS.HLP could not be read.";
        return {};
    }
    const Bytes topicFile = archive.file("|TOPIC");
    if (topicFile.empty()) {
        *error = L"The original help file contains no topics.";
        return {};
    }
    const std::vector<uint8_t> topic = topicStream(topicFile);
    std::vector<Record> entries;
    std::vector<Page> pages;
    if (!records(topic, &entries, &pages)) {
        *error = L"The original help topics could not be decoded.";
        return {};
    }
    const auto context = archive.context();
    for (const Record& entry : entries)
        if (entry.page >= 0 && (entry.type == 0x20 || entry.type == 0x23))
            pages[size_t(entry.page)].body += paragraph(entry, context, pages);
    const std::wstring directory = files.root() + L"\\help-html";
    if (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        *error = L"The HTML help folder could not be created.";
        return {};
    }
    for (int index = 0; index < 1000; ++index) {
        const Bytes picture = archive.file("|bm" + std::to_string(index));
        if (picture.empty()) continue;
        const std::wstring path = directory + L"\\bm" + std::to_wstring(index) + L".bmp";
        const uint32_t start = dword(picture, 4);
        if (start + 2 <= picture.size() && picture[start] == 6) exportBitmap(picture, path);
        else if (start + 2 <= picture.size() && picture[start] == 8) exportMetaFile(picture, path);
    }
    std::string html = "<!doctype html><html><head><meta charset=\"windows-1252\"><title>Crayons Help</title>"
                       "<style>body{font:16px Arial,sans-serif;max-width:1100px;margin:auto;padding:20px;color:#222}"
                       "nav{columns:3}nav a{display:block;padding:2px}article{border-top:1px solid #bbb;padding:24px 0}"
                       "article img{display:block;max-width:100%;margin:12px 0}a{color:#0645ad}"
                       "p{line-height:1.5}h1,h2{color:#172f78}</style></head><body><h1>Crayons Help</h1><nav>";
    for (size_t index = 0; index < pages.size(); ++index)
        html += "<a href=\"#topic-" + std::to_string(index) + "\">" +
                escape(pages[index].title.empty() ? "Topic " + std::to_string(index + 1) : pages[index].title) + "</a>";
    html += "</nav>";
    for (size_t index = 0; index < pages.size(); ++index)
        html += "<article id=\"topic-" + std::to_string(index) + "\"><h2>" +
                escape(pages[index].title.empty() ? "Topic " + std::to_string(index + 1) : pages[index].title) +
                "</h2>" + pages[index].body + "</article>";
    html += "</body></html>";
    const std::wstring path = directory + L"\\index.html";
    if (!writeFile(path, html)) {
        *error = L"The HTML help page could not be written.";
        return {};
    }
    return path;
}

}  // namespace help
