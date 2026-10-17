// TActivity (Activits, CRAYON1 segment 1): the list of Magic Color pictures — titles, a
// thumbnail and the page file, from activity\z1xx/z2xx/z3xx.asl.
#pragma once

#include <functional>

#include "dialog.h"

class ActivityDialog : public Dialog {
public:
    // picker = DS:1ab6: 11 things to do, 12 3D things, 13 MagiColor, 14 storybooks
    ActivityDialog(App& app, int level, int picker, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;

    bool chosen = false;  // DS:105a
    std::string path;     // DS:4712

private:
    void preview();  // Image3 = activity\ + the thumbnail list's item
    std::function<void(int)> sound_;
    ListBoxView titles_;
    std::vector<std::string> thumbs_, pages_;
    ImageBox ok_, exit_, thumb_;
};
