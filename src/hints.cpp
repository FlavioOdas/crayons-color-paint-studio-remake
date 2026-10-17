#include "hints.h"

#include <algorithm>

#include "app.h"

namespace {

HFONT labelFont() {  // Label1..3: Font.Name = 'System', Height = -13, black
    return CreateFontW(-13, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, L"System");
}

}  // namespace

void HintSource::load(const std::vector<uint8_t>& text) {  // TStringList.LoadFromFile
    lines.clear();
    std::string line;
    for (uint8_t c : text) {
        if (c == '\r') continue;
        if (c == '\n') lines.push_back(line), line.clear();
        else line += char(c);
    }
    if (!line.empty()) lines.push_back(line);
}

const std::string* HintSource::text(int hint) const {
    if (hint < 0 || hint > 0x122 || hint >= int(lines.size())) return nullptr;
    const std::string& t = lines[size_t(hint)];
    return t.empty() || t[0] == ' ' ? nullptr : &t;
}

void HintSource::speak(int hint) const {
    if (host && audioHelp && *audioHelp) host->playSound("snd\\hv" + std::to_string(hint + 1) + ".asl");
}

void HintPanel::place(const std::string& text, int x, int y, int limit) {
    // A further line starts only when text follows the bar; the third runs to the end.
    count = 0;
    size_t i = 0;
    while (count < 3 && (count == 0 || i < text.size())) {
        const size_t end = count < 2 ? std::min(text.find('|', i), text.size()) : text.size();
        lines[count++] = text.substr(i, end - i);
        i = end + 1;
    }
    for (int k = count; k < 3; ++k) lines[k].clear();
    // The labels AutoSize; an empty caption is measured as one space.
    HDC dc = CreateCompatibleDC(nullptr);
    HFONT font = labelFont();
    HGDIOBJ old = SelectObject(dc, font);
    int widest = 0;
    for (const std::string& l : lines) {
        const std::string measured = l.empty() ? " " : l;
        SIZE size{};
        GetTextExtentPoint32A(dc, measured.c_str(), int(measured.size()), &size);
        widest = std::max(widest, int(size.cx));
        lineHeight = int(size.cy);
    }
    SelectObject(dc, old);
    DeleteObject(font);
    DeleteDC(dc);
    const int width = widest + 10, height = count * lineHeight + 2;
    const int left = x + width >= limit ? limit - width : x;
    rect = {left, y + 16, left + width, y + 16 + height};
    visible = true;
}

void HintPanel::paint(HDC dc, int dx, int dy) const {
    if (!visible) return;
    // BorderStyle = bsSingle without Ctl3D: a black line; Color = clYellow.
    const RECT r{rect.left + dx, rect.top + dy, rect.right + dx, rect.bottom + dy};
    HBRUSH black = CreateSolidBrush(RGB(0, 0, 0)), yellow = CreateSolidBrush(RGB(255, 255, 0));
    FillRect(dc, &r, black);
    const RECT inner{r.left + 1, r.top + 1, r.right - 1, r.bottom - 1};
    FillRect(dc, &inner, yellow);
    DeleteObject(black), DeleteObject(yellow);
    HFONT font = labelFont();
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, OPAQUE);
    SetBkColor(dc, RGB(255, 255, 0));
    SetTextColor(dc, RGB(0, 0, 0));
    for (int i = 0; i < count; ++i)  // labels at (2, i · height) in the panel's client area
        TextOutA(dc, inner.left + 2, inner.top + i * lineHeight, lines[i].c_str(), int(lines[i].size()));
    SelectObject(dc, old);
    DeleteObject(font);
}
