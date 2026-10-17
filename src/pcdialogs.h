// Small dialogs of CRAYON1 segment 1: the PC tool's Tjukebox (Midistuf), Taboutme (About) and
// Torders (Orderme — a leftover shareware order form, reachable from an empty option slot), and
// the shapes' and paint can's TFillStyles chart.
#pragma once

#include <functional>

#include "dialog.h"

class JukeboxDialog : public Dialog {
public:
    // flags: DS:105f sounds, DS:1060 music, DS:1061 audio help (spoken hints)
    JukeboxDialog(App& app, int level, bool* sounds, bool* music, bool* audioHelp, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;

private:
    void update();  // FormShow 1000:09b9: each switch shows its on or off picture
    bool *sounds_, *music_, *audioHelp_;
    std::function<void(int)> sound_;
    ImageBox background_, soundsIcon_, musicIcon_, helpIcon_, exit_;
    ImageBox soundsOn_, soundsOff_, musicOn_, musicOff_, helpOn_, helpOff_;
    struct Text { RECT rect; std::string text; int height; } labels_[3];
};

class AboutDialog : public Dialog {  // Taboutme: a912.asl; closes on a click at the bottom centre
public:
    AboutDialog(App& app, int level, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;

private:
    std::function<void(int)> sound_;
    Surface picture_;
};

class OrdersDialog : public Dialog {  // Torders: a945.asl in a read-only memo, Image1 closes
public:
    OrdersDialog(App& app, int level, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;

private:
    std::vector<std::string> lines_;
    ImageBox exit_;
    RECT memo_{};
};

class FillStylesDialog : public Dialog {  // TFillStyles: a borderless chart under the options
public:
    // Opened from the fill panel (FormMouseDown 1008:72e8); the paint can (mode 0x3a) shows only
    // the first two rows. A left click stores the style in *style (DS:1ac4) and closes.
    FillStylesDialog(App& app, int level, bool paintCan, int* style, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;

private:
    int* style_;
    ImageBox chart_;
};
