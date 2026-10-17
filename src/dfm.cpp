#include "dfm.h"

#include <cstring>

namespace dfm {
namespace {

class Reader {
public:
    explicit Reader(std::span<const uint8_t> d) : d_(d) {}
    bool ok() const { return ok_; }
    uint8_t peek() { return p_ < d_.size() ? d_[p_] : (ok_ = false, 0); }
    uint8_t u8() { return p_ < d_.size() ? d_[p_++] : (ok_ = false, 0); }
    std::span<const uint8_t> take(size_t n) {
        if (p_ + n > d_.size()) return ok_ = false, std::span<const uint8_t>{};
        p_ += n;
        return d_.subspan(p_ - n, n);
    }
    template <typename T>
    T num() {
        T v{};
        const std::span<const uint8_t> b = take(sizeof(T));
        if (!b.empty()) std::memcpy(&v, b.data(), sizeof(T));
        return v;
    }
    std::string sstr() {
        const std::span<const uint8_t> b = take(u8());
        return std::string(b.begin(), b.end());
    }
    Value value();
    void props(Component* c);
    std::unique_ptr<Component> component();

private:
    std::span<const uint8_t> d_;
    size_t p_ = 0;
    bool ok_ = true;
};

Value Reader::value() {
    Value v;
    switch (u8()) {  // TValueType
    case 0: break;
    case 1:
        v.kind = Value::Kind::List;
        while (ok_ && peek() != 0) v.list.push_back(value());
        u8();
        break;
    case 2: v.kind = Value::Kind::Int, v.i = num<int8_t>(); break;
    case 3: v.kind = Value::Kind::Int, v.i = num<int16_t>(); break;
    case 4: v.kind = Value::Kind::Int, v.i = num<int32_t>(); break;
    case 5: {  // 80-bit extended
        const uint64_t mantissa = num<uint64_t>();
        const uint16_t exponent = num<uint16_t>();
        v.kind = Value::Kind::Float;
        if (mantissa || (exponent & 0x7FFF)) {
            v.f = double(mantissa) / 9223372036854775808.0;
            for (int e = (exponent & 0x7FFF) - 16383; e > 0; --e) v.f *= 2;
            for (int e = (exponent & 0x7FFF) - 16383; e < 0; ++e) v.f /= 2;
            if (exponent & 0x8000) v.f = -v.f;
        }
        break;
    }
    case 6: v.kind = Value::Kind::String, v.s = sstr(); break;
    case 7: v.kind = Value::Kind::Ident, v.s = sstr(); break;
    case 8: v.kind = Value::Kind::Bool, v.i = 0; break;
    case 9: v.kind = Value::Kind::Bool, v.i = 1; break;
    case 10: {
        const std::span<const uint8_t> b = take(num<uint32_t>());
        v.kind = Value::Kind::Binary, v.binary.assign(b.begin(), b.end());
        break;
    }
    case 11:
        v.kind = Value::Kind::Set;
        for (std::string s = sstr(); ok_ && !s.empty(); s = sstr()) v.set.push_back(s);
        break;
    case 12: {
        const std::span<const uint8_t> b = take(num<uint32_t>());
        v.kind = Value::Kind::String, v.s.assign(b.begin(), b.end());
        break;
    }
    case 13: v.kind = Value::Kind::Null; break;  // nil
    default: ok_ = false; break;
    }
    return v;
}

void Reader::props(Component* c) {
    for (std::string name = sstr(); ok_ && !name.empty(); name = sstr()) c->props[name] = value();
}

std::unique_ptr<Component> Reader::component() {
    auto c = std::make_unique<Component>();
    if ((peek() & 0xF0) == 0xF0) {  // Delphi 2+ filer flags; not used by Delphi 1
        const uint8_t flags = u8();
        if (flags & 2) value();
    }
    c->className = sstr();
    c->name = sstr();
    props(c.get());
    while (ok_ && peek() != 0) c->children.push_back(component());
    u8();
    return c;
}

}  // namespace

int Component::integer(const std::string& prop, int fallback) const {
    const auto it = props.find(prop);
    return it != props.end() && it->second.kind == Value::Kind::Int ? int(it->second.i) : fallback;
}

std::string Component::string(const std::string& prop) const {
    const auto it = props.find(prop);
    return it != props.end() ? it->second.s : std::string();
}

bool Component::flag(const std::string& prop, bool fallback) const {
    const auto it = props.find(prop);
    return it != props.end() && it->second.kind == Value::Kind::Bool ? it->second.i != 0 : fallback;
}

std::span<const uint8_t> Component::bitmap(const std::string& prop) const {
    const auto it = props.find(prop);
    if (it == props.end() || it->second.binary.empty()) return {};
    std::span<const uint8_t> b = it->second.binary;
    // Picture.Data = <shortstring class name><u32 size><.BMP>; Glyph.Data = <u32 size><.BMP>.
    if (b[0] < 32 && size_t(b[0]) + 1 < b.size()) {
        const std::string cls(b.begin() + 1, b.begin() + 1 + b[0]);
        if (cls != "TBitmap") return {};
        b = b.subspan(1 + b[0]);
    }
    if (b.size() > 4 && b[0] != 'B') b = b.subspan(4);
    return b.size() > 2 && b[0] == 'B' && b[1] == 'M' ? b : std::span<const uint8_t>{};
}

std::span<const uint8_t> Component::icon(const std::string& prop) const {
    const auto it = props.find(prop);
    if (it == props.end() || it->second.binary.empty()) return {};
    std::span<const uint8_t> b = it->second.binary;
    if (b[0] != 5 || b.size() < 6 + 6 || std::string(b.begin() + 1, b.begin() + 6) != "TIcon") return {};
    return b.subspan(6);  // an .ICO file: reserved 0, type 1, count, entries
}

const Component* Component::find(const std::string& componentName) const {
    if (name == componentName) return this;
    for (const auto& child : children)
        if (const Component* c = child->find(componentName)) return c;
    return nullptr;
}

std::unique_ptr<Component> parse(std::span<const uint8_t> data, std::string* error) {
    if (data.size() < 4 || std::memcmp(data.data(), "TPF0", 4) != 0) return *error = "not a Delphi form", nullptr;
    Reader r(data.subspan(4));
    auto root = r.component();
    if (!r.ok()) return *error = "damaged form data", nullptr;
    return root;
}

}  // namespace dfm
