#include "thumbs.h"

#include <windows.h>

#include <algorithm>
#include <cctype>

namespace {

std::vector<std::string> findFiles(const std::wstring& root, const std::string& pattern) {
    std::vector<std::string> names;  // FindFirst/FindNext order (sorted on NTFS and CDs)
    WIN32_FIND_DATAW fd;
    const std::wstring wide(pattern.begin(), pattern.end());
    HANDLE h = FindFirstFileW((root + L"\\" + wide).c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return names;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::string name;
        for (const wchar_t* c = fd.cFileName; *c; ++c) name += char(*c);
        names.push_back(name);
    } while (FindNextFileW(h, &fd) && names.size() < 0xf9);
    FindClose(h);
    return names;
}

std::string folderOf(int picker) { return picker == 5 || picker == 8 ? "user\\" : "art\\"; }

}  // namespace

std::string ThumbsDialog::pattern(int picker) {  // 1008:4c27 / 1020:0dab
    switch (picker) {
    case 1: return "art\\cb*.bmp";
    case 4: return "art\\level*.bmp";
    case 5: case 8: return "user\\yp*.bmp";
    case 6: return "art\\M*.bmp";
    case 7: return "art\\F*.bmp";
    }
    return "";
}

bool ThumbsDialog::anyPictures(App& app, int picker) {
    return !findFiles(app.assets().files().root(), pattern(picker)).empty();
}

ThumbsDialog::ThumbsDialog(App& app, int level, int picker, std::function<void(int)> sound)
    : Dialog(app), picker_(picker), sound_(std::move(sound)) {
    const dfm::Component* form = app.assets().form(level, "TTHUMBS");
    colour = RGB(0, 0, 0);  // Color = clBlack
    auto fromForm = [&](ImageBox& box, const char* name) { loadImage(box, form ? form->find(name) : nullptr); };
    fromForm(up_, "upbutton");
    fromForm(down_, "downbutton");
    fromForm(exit_, "killme");
    if (const dfm::Component* l = form ? form->find("Label1") : nullptr)
        labelRect_ = {l->left(), l->top(), l->left() + l->width(), l->top() + l->height()};
    // FormShow 1020:0dab: Stretch off, Width 200, then per picture set.
    int top = 94, height = 330;
    const bool clipArt = picker == 6 || picker == 7;
    if (clipArt) {
        top = 1, height = 450;
        static const POINT kAt[4] = {{1, 1}, {202, 1}, {1, 202}, {202, 202}};
        for (int i = 0; i < 4; ++i) images_[i].rect = {kAt[i].x, kAt[i].y, kAt[i].x + 200, kAt[i].y + 200};
    } else {
        static const POINT kAt[4] = {{1, 1}, {202, 1}, {1, 152}, {202, 152}};
        for (int i = 0; i < 4; ++i) {
            images_[i].rect = {kAt[i].x, kAt[i].y, kAt[i].x + 200, kAt[i].y + 150};
            images_[i].stretch = true;
        }
    }
    switch (picker) {
    case 1: caption = L"Coloring Book Pictures", sound_(105); break;
    case 6: caption = L"Clip Art Pictures -- Black / White", sound_(100); break;
    case 7: caption = L"Clip Art Pictures -- Color", sound_(100); break;
    case 4: caption = L"Background Images", sound_(102); break;
    case 8: caption = L"Choose a Picture to Delete", sound_(103); break;
    case 5: caption = L"My Pictures -- Choose Picture to Open", sound_(104); break;
    }
    const int width = (form ? form->integer("ClientWidth", 526) : 526) + 2 * kFrame;
    for (const std::string& name : findFiles(app.assets().files().root(), pattern(picker)))
        files_.push_back(folderOf(picker) + name);
    count_ = int(files_.size()) - 1;
    first_ = 0;
    last_ = count_ > 3 ? 3 : count_;
    setBounds(82, top, width, height);
    showPage(first_, last_);
}

void ThumbsDialog::showPage(int first, int last) {  // 1020:072e
    Surface blank;  // 'a920.bmp': a black 1x1 picture clears each image
    loadBmp(app_.assets().files().read("a920.bmp"), &blank);
    for (int k = 0; k < 4; ++k) {
        ImageBox& box = images_[k];
        box.picture = Surface();
        loadBmp(app_.assets().files().read("a920.bmp"), &box.picture);
        if (first + k > last) continue;
        const std::string& file = files_[size_t(first + k)];
        if (picker_ == 6 || picker_ == 7) {  // "art\MH01.BMP": the letter after M/F says wide or tall
            const size_t slash = file.find_last_of('\\');
            const bool wide = slash + 2 < file.size() && std::toupper(uint8_t(file[slash + 2])) == 'H';
            box.rect.right = box.rect.left + (wide ? 200 : 100);
            box.rect.bottom = box.rect.top + (wide ? 100 : 200);
            orient_[k] = wide ? 4 : 3;
        }
        box.picture = Surface();
        loadBmp(app_.assets().files().read(file), &box.picture);
    }
    label_ = std::to_string(first + 1) + " - " + std::to_string(last + 1) + " of " + std::to_string(count_ + 1);
    repaint();
}

void ThumbsDialog::paintClient(HDC dc) {
    for (const ImageBox& box : images_) box.paint(dc);
    up_.paint(dc), down_.paint(dc), exit_.paint(dc);
    // Label1: System font, yellow (black background as in the manual's screenshots).
    HFONT font = CreateFontW(-13, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, L"System");
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 0));
    TextOutA(dc, labelRect_.left, labelRect_.top, label_.c_str(), int(label_.size()));
    SelectObject(dc, old);
    DeleteObject(font);
}

void ThumbsDialog::choose(int index) {  // Image1Click 1020:1813 (Image2..4 check the page first)
    if (first_ + index > last_) return;
    if (picker_ == 6 || picker_ == 7) orientation = orient_[index];
    path = files_[size_t(first_ + index)];
    chosen = true;
    close();
}

void ThumbsDialog::mouseDown(int x, int y, int button) {
    if (button == 1) {  // right button: hints (1020:02d3)
        if (up_.contains(x, y)) showHint(x, y, 125);          // upbuttonMouseDown 1020:1a23
        else if (down_.contains(x, y)) showHint(x, y, 126);   // downbuttonMouseDown 1020:1aba
        else if (exit_.contains(x, y)) showHint(x, y, 147);   // killmeMouseDown 1020:1e0e
        return;
    }
    for (int k = 0; k < 4; ++k) {
        if (images_[k].contains(x, y)) return choose(k);
    }
    if (up_.contains(x, y)) {  // 1020:1a23: back three (sic), or wrap to the last page
        sound_(106);
        if (first_ - 3 >= 0) {
            first_ -= 3, last_ -= 3;
        } else if (count_ > 3) {
            last_ = count_, first_ = last_ - 3;
        } else {
            first_ = 0, last_ = std::min(3, count_);
        }
        showPage(first_, last_);
    } else if (down_.contains(x, y)) {  // 1020:1aba: on four, or to the end, then wrap
        sound_(107);
        if (last_ + 4 <= count_) {
            first_ += 4, last_ += 4;
        } else if (last_ < count_) {
            last_ = count_, first_ = last_ - 3;
            if (first_ < 0) first_ = 0, last_ = std::min(3, count_);
        } else {
            first_ = 0, last_ = std::min(3, count_);
        }
        showPage(first_, last_);
    } else if (exit_.contains(x, y)) {  // killmeClick 1020:17b2
        sound_(92);
        close();
    }
}
