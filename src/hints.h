// The yellow hint panels: every form of the paint program has a Panel1 with Label1..3 that a
// right click fills from a939.asl (the main form 1008:034f, Tjukebox 1000:0570, TActivity
// 1000:113c, Trotator 1000:20b7, Ttextinpt 1000:2dfb, Tthumbs 1020:02d3, 1010:01f9), and, with
// audio help on (DS:1061), the spoken hint snd\hv<n+1>.asl.
#pragma once

#include <windows.h>

#include <string>
#include <vector>

class Host;

struct HintSource {
    std::vector<std::string> lines;   // a939.asl (the TStringList at DS:146a)
    const bool* audioHelp = nullptr;  // DS:1061
    Host* host = nullptr;
    void load(const std::vector<uint8_t>& text);
    // The hint's text, or nullptr when there is none (beyond 290, empty, or starting with a space).
    const std::string* text(int hint) const;
    void speak(int hint) const;
};

struct HintPanel {
    bool visible = false;
    RECT rect{};  // the panel window, including its 1-px border, in its parent's client coordinates
    std::string lines[3];
    int count = 0, lineHeight = 16;
    // Panel1.Left = x, Top = y + 16; lines split at '|'; Width = widest label + 10, Height =
    // lines · label height + 2; moved left to end at `limit` when x + Width >= limit.
    void place(const std::string& text, int x, int y, int limit);
    void paint(HDC dc, int dx = 0, int dy = 0) const;  // drawn at rect offset by (dx, dy)
};
