// Tthumbs (Thmbunit, CRAYON1 segment 5 = selector 1020): the picture chooser used for the
// coloring book, clip art, backgrounds, opening and deleting the child's pictures.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "dialog.h"

class ThumbsDialog : public Dialog {
public:
    // picker = DS:1ab6: 1 coloring book, 4 backgrounds, 5 open, 6 clip art black/white,
    // 7 clip art colour, 8 delete.
    ThumbsDialog(App& app, int level, int picker, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;

    bool chosen = false;    // DS:105a
    std::string path;       // DS:4712, relative to the data folder
    int orientation = 0;    // DS:1aa2: 4 = wide (200x100 + mask below), 3 = tall (100x200 + mask right)

    // 1008:4c27: is there anything to show? (the original says "There are no pictures to see.")
    static bool anyPictures(App& app, int picker);
    static std::string pattern(int picker);

private:
    void showPage(int first, int last);  // 1020:072e
    void choose(int index);              // Image1..4Click 1020:1813…195d

    int picker_;
    std::function<void(int)> sound_;
    std::vector<std::string> files_;
    int count_ = 0, first_ = 0, last_ = 0;  // DS:1032 (last index), 1034, 1036
    ImageBox images_[4], up_, down_, exit_;
    int orient_[4] = {};                    // DS:1038…103e
    std::string label_;
    RECT labelRect_{};
};
