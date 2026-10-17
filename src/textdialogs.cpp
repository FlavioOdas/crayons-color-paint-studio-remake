#include "textdialogs.h"

#include <algorithm>
#include <cmath>

namespace {

RECT bounds(const dfm::Component* component, RECT fallback) {
    return component ? RECT{component->left(), component->top(), component->left() + component->width(),
                            component->top() + component->height()} : fallback;
}

void paintEdit(HDC dc, RECT rect, const std::string& value, int fontHeight, bool selected) {
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &rect, white);
    DeleteObject(white);
    HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
    FrameRect(dc, &rect, black);
    DeleteObject(black);
    HFONT font = CreateFontA(fontHeight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "System");
    HGDIOBJ old = SelectObject(dc, font);
    SaveDC(dc);
    IntersectClipRect(dc, rect.left + 2, rect.top + 1, rect.right - 2, rect.bottom - 1);
    SetBkMode(dc, TRANSPARENT);
    if (selected && !value.empty()) {
        SIZE size{};
        GetTextExtentPoint32A(dc, value.c_str(), int(value.size()), &size);
        HBRUSH navy = CreateSolidBrush(RGB(0, 0, 128));
        RECT highlight{rect.left + 2, rect.top + 2, rect.left + 2 + size.cx, rect.bottom - 2};
        FillRect(dc, &highlight, navy);
        DeleteObject(navy);
        SetTextColor(dc, RGB(255, 255, 255));
    } else SetTextColor(dc, RGB(0, 0, 0));
    TextOutA(dc, rect.left + 2, rect.top + 2, value.c_str(), int(value.size()));
    RestoreDC(dc, -1);
    SelectObject(dc, old);
    DeleteObject(font);
}

void editKey(std::string& value, bool& selected, wchar_t character) {
    if (character == L'\b') {
        if (selected) value.clear();
        else if (!value.empty()) value.pop_back();
        selected = false;
        return;
    }
    if (character < 32 || value.size() >= 255) return;
    char ansi = 0;
    if (character < 0x80) ansi = char(character);
    else if (WideCharToMultiByte(1252, 0, &character, 1, &ansi, 1, nullptr, nullptr) != 1) return;
    if (selected) value.clear();
    selected = false;
    value += ansi;
}

void image(const dfm::Component* form, ImageBox& box, const char* name) {
    loadImage(box, form ? form->find(name) : nullptr);
}

int CALLBACK addFont(const LOGFONTA* font, const TEXTMETRICA*, DWORD, LPARAM data) {
    auto* names = reinterpret_cast<std::vector<std::string>*>(data);
    if (font->lfFaceName[0] != '@') names->emplace_back(font->lfFaceName);
    return 1;
}

int rotatorTicks = 0;  // DS:1016: FormShow does not reset this counter

}  // namespace

TextInputDialog::TextInputDialog(App& app, int level, std::string* text, std::function<void(int)> sound)
    : Dialog(app, 3), text_(text), entry_(*text), sound_(std::move(sound)) {
    const dfm::Component* form = app.assets().form(level, "TTEXTINPT");
    colour = RGB(0, 0, 0);
    caption = L"Enter Text to be Displayed:";
    image(form, okay_, "Image1"), image(form, cancel_, "Image2");
    edit_ = bounds(form ? form->find("Edit1") : nullptr, {9, 30, 330, 53});
    setBounds(form ? form->left() : 153, form ? form->top() : 111,
              (form ? form->integer("ClientWidth", 408) : 408) + 6,
              (form ? form->integer("ClientHeight", 88) : 88) + 25);
}

void TextInputDialog::paintClient(HDC dc) {
    okay_.paint(dc), cancel_.paint(dc);
    paintEdit(dc, edit_, entry_, -13, selected_);
}

void TextInputDialog::accept() {  // 1000:32bf: StrPCopy(DS:1572, Edit1.Text)
    *text_ = entry_;
    close();
}

void TextInputDialog::cancel() {  // 1000:3237
    sound_(92);
    text_->clear();
    entry_.clear();
    close();
}

void TextInputDialog::mouseDown(int x, int y, int button) {
    if (button == 1) {
        if (okay_.contains(x, y)) showHint(x, y, 149);  // 1000:2dfb
        else if (cancel_.contains(x, y)) showHint(x, y, 150);
        return;
    }
    if (okay_.contains(x, y)) accept();
    else if (cancel_.contains(x, y)) cancel();
    else if (x >= edit_.left && x < edit_.right && y >= edit_.top && y < edit_.bottom) selected_ = false;
}

void TextInputDialog::keyChar(wchar_t character) {  // Edit1KeyUp 1000:334c
    if (character == L'\r') accept();
    else editKey(entry_, selected_, character);
    repaint();
}

RotatorDialog::RotatorDialog(App& app, int level, std::string* text, int* angle, COLORREF colour,
                             std::function<void(int)> sound)
    : Dialog(app, 3), text_(text), angle_(angle), colour_(colour), sound_(std::move(sound)) {
    const dfm::Component* form = app.assets().form(level, "TROTATOR");
    this->colour = RGB(0, 0, 0);
    caption = L"rotator";
    image(form, okay_, "Image1"), image(form, cancel_, "Image2");
    edit_ = bounds(form ? form->find("Edit1") : nullptr, {10, 124, 367, 148});
    setBounds(form ? form->left() : 112, form ? form->top() : 128,
              (form ? form->integer("ClientWidth", 377) : 377) + 6,
              (form ? form->integer("ClientHeight", 158) : 158) + 25);
    nextTick_ = app.now() + 200;  // FormShow 1000:27e9 enables Timer1
}

void RotatorDialog::setAngle(int value) {  // ScrollBar1Scroll 1000:2728
    *angle_ = std::clamp(value, 0, 359);
    repaint();
}

void RotatorDialog::tick(long now) {  // Timer1Timer 1000:2833
    if (initialized_ || now < nextTick_) return;
    ++rotatorTicks;
    if (rotatorTicks >= 3) {
        entry_ = *text_;
        selected_ = true;
        initialized_ = true;
        repaint();
    } else nextTick_ += 200;
}

void RotatorDialog::paintClient(HDC dc) {  // 1000:252d preview
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    RECT preview{6, 5, 131, 105};
    FillRect(dc, &preview, white);
    DeleteObject(white);
    HPEN black = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HGDIOBJ oldPen = SelectObject(dc, black);
    MoveToEx(dc, 63, 55, nullptr), LineTo(dc, 73, 55);
    MoveToEx(dc, 68, 50, nullptr), LineTo(dc, 68, 60);
    SelectObject(dc, oldPen);
    DeleteObject(black);
    const int height = -int(std::nearbyint(10.0f * (96.0f / 72.0f)));
    HFONT example = CreateFontA(height, 0, *angle_ * 10, *angle_ * 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Arial");
    HGDIOBJ old = SelectObject(dc, example);
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_CENTER | TA_BASELINE);
    SetTextColor(dc, colour_);
    TextOutA(dc, 68, 55, "Example", 7);
    SetTextAlign(dc, TA_LEFT | TA_TOP);
    SelectObject(dc, old);
    DeleteObject(example);
    HBRUSH face = CreateSolidBrush(RGB(192, 192, 192));
    RECT bar{133, 5, 150, 105};
    FillRect(dc, &bar, face);
    DeleteObject(face);
    RECT thumb{134, 20 + (*angle_ * 68 / 359), 149, 34 + (*angle_ * 68 / 359)};
    HBRUSH shadow = CreateSolidBrush(RGB(128, 128, 128));
    FrameRect(dc, &bar, shadow), FrameRect(dc, &thumb, shadow);
    DeleteObject(shadow);
    HFONT label = CreateFontA(-13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "System");
    old = SelectObject(dc, label);
    SetTextColor(dc, RGB(255, 255, 0));
    RECT labelRect{157, 52, 264, 68};
    const std::string captionText = std::to_string(*angle_) + " Degrees";
    DrawTextA(dc, captionText.c_str(), int(captionText.size()), &labelRect, DT_CENTER | DT_SINGLELINE);
    SelectObject(dc, old);
    DeleteObject(label);
    okay_.paint(dc), cancel_.paint(dc);
    paintEdit(dc, edit_, entry_, -16, selected_);
}

void RotatorDialog::accept() {  // 1000:2744
    sound_(91);
    *text_ = entry_;
    close();
}

void RotatorDialog::cancel() {  // 1000:28e1
    sound_(92);
    text_->clear();
    entry_.clear();
    close();
}

void RotatorDialog::mouseDown(int x, int y, int button) {
    if (button == 1) {
        if (okay_.contains(x, y)) showHint(x, y, 151);
        else if (cancel_.contains(x, y)) showHint(x, y, 154);
        return;
    }
    if (okay_.contains(x, y)) accept();
    else if (cancel_.contains(x, y)) cancel();
    else if (x >= 133 && x < 150 && y >= 5 && y < 105) {
        if (y < 20) setAngle(*angle_ - 1);
        else if (y >= 90) setAngle(*angle_ + 1);
        else setAngle((y - 20) * 359 / 70);
    } else if (x >= edit_.left && x < edit_.right && y >= edit_.top && y < edit_.bottom) selected_ = false;
}

void RotatorDialog::keyChar(wchar_t character) {  // Edit1KeyUp → Image1Click
    if (character == L'\r') accept();
    else editKey(entry_, selected_, character);
    repaint();
}

FontDialog::FontDialog(App& app, int level, std::string* face, std::function<void(int)> sound)
    : Dialog(app, 3), face_(face), sound_(std::move(sound)) {
    const dfm::Component* form = app.assets().form(level, "TFONTFORM");
    colour = RGB(0, 0, 0);
    image(form, okay_, "Image1");
    const dfm::Component* list = form ? form->find("ListBox1") : nullptr;
    list_.rect = bounds(list, {4, 2, 229, 159});
    list_.itemHeight = list ? list->integer("ItemHeight", 16) : 16;
    list_.colour = RGB(0, 0, 0), list_.textColour = RGB(255, 255, 255);
    sample_ = bounds(form ? form->find("Label1") : nullptr, {233, 3, 393, 83});
    HDC dc = GetDC(nullptr);
    LOGFONTA query{};
    query.lfCharSet = DEFAULT_CHARSET;
    EnumFontFamiliesExA(dc, &query, addFont, reinterpret_cast<LPARAM>(&list_.items), 0);
    ReleaseDC(nullptr, dc);
    std::sort(list_.items.begin(), list_.items.end());
    list_.items.erase(std::unique(list_.items.begin(), list_.items.end()), list_.items.end());
    auto found = std::find(list_.items.begin(), list_.items.end(), *face_);
    if (found != list_.items.end()) {
        list_.itemIndex = int(found - list_.items.begin());
        list_.topIndex = std::max(0, list_.itemIndex - 4);
    }
    setBounds(form ? form->left() : 268, form ? form->top() : 143,
              (form ? form->integer("ClientWidth", 394) : 394) + 6,
              (form ? form->integer("ClientHeight", 161) : 161) + 25);
}

void FontDialog::paintClient(HDC dc) {
    list_.paint(dc);
    HFONT font = CreateFontA(-40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, face_->c_str());
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    RECT sample = sample_;
    DrawTextA(dc, "Hello", 5, &sample, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old);
    DeleteObject(font);
    okay_.paint(dc);
}

void FontDialog::mouseDown(int x, int y, int button) {  // 1000:3799
    if (button != 0) return;
    if (okay_.contains(x, y)) {
        sound_(91);  // Image1Click 1000:3743
        close();
        return;
    }
    const int hit = list_.hit(x, y);
    if (hit >= 0) {
        sound_(89);
        list_.itemIndex = hit;
        *face_ = list_.items[size_t(hit)];  // DS:1168, Label1.Font.Name
        repaint();
    }
}
