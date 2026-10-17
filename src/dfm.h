// Delphi 1 form resources ("TPF0" binary .DFM in RCDATA): component tree with properties.
// The remake takes window layouts and pictures straight from the original forms.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace dfm {

struct Value {
    enum class Kind { Null, Int, Float, String, Ident, Bool, Binary, Set, List } kind = Kind::Null;
    int64_t i = 0;
    double f = 0;
    std::string s;                // String/Ident
    std::vector<uint8_t> binary;  // Binary
    std::vector<std::string> set; // Set members
    std::vector<Value> list;      // List items
};

struct Component {
    std::string className, name;
    std::map<std::string, Value> props;  // "Left", "Picture.Data", "Tabs.Strings", ...
    std::vector<std::unique_ptr<Component>> children;

    int integer(const std::string& prop, int fallback = 0) const;
    std::string string(const std::string& prop) const;
    bool flag(const std::string& prop, bool fallback) const;
    // Left/Top/Width/Height (forms: ClientWidth/ClientHeight).
    int left() const { return integer("Left"); }
    int top() const { return integer("Top"); }
    int width() const { return integer("Width", integer("ClientWidth")); }
    int height() const { return integer("Height", integer("ClientHeight")); }
    // The .BMP inside a TImage's Picture.Data / TBitBtn Glyph.Data (empty when none).
    std::span<const uint8_t> bitmap(const std::string& prop = "Picture.Data") const;
    // The .ICO inside a Picture.Data holding a TIcon (empty when none).
    std::span<const uint8_t> icon(const std::string& prop = "Picture.Data") const;
    const Component* find(const std::string& componentName) const;  // depth-first
};

// Parses a form resource. Returns nullptr (and a message) on malformed data.
std::unique_ptr<Component> parse(std::span<const uint8_t> data, std::string* error);

}  // namespace dfm
