#include "pcdialogs.h"

namespace {

void imageFromForm(const dfm::Component* form, ImageBox& box, const char* name) {
    loadImage(box, form ? form->find(name) : nullptr);
}

int windowWidth(const dfm::Component* form, int frame, int fallback) {
    return (form ? form->integer("ClientWidth", fallback) : fallback) + 2 * frame;
}

}  // namespace

JukeboxDialog::JukeboxDialog(App& app, int level, bool* sounds, bool* music, bool* audioHelp,
                             std::function<void(int)> sound)
    : Dialog(app), sounds_(sounds), music_(music), audioHelp_(audioHelp), sound_(std::move(sound)) {
    const dfm::Component* form = app.assets().form(level, "TJUKEBOX");
    colour = RGB(0, 0, 0);
    caption = L"jukebox";
    imageFromForm(form, background_, "Image1");
    imageFromForm(form, soundsIcon_, "Image4");
    imageFromForm(form, musicIcon_, "Image7");
    imageFromForm(form, helpIcon_, "Image11");
    imageFromForm(form, exit_, "Image8");
    imageFromForm(form, soundsOn_, "Image2"), imageFromForm(form, soundsOff_, "Image3");
    imageFromForm(form, musicOn_, "Image5"), imageFromForm(form, musicOff_, "Image6");
    imageFromForm(form, helpOn_, "Image9"), imageFromForm(form, helpOff_, "Image10");
    const char* names[3] = {"Label1", "Label2", "Label3"};
    for (int i = 0; i < 3; ++i) {
        const dfm::Component* l = form ? form->find(names[i]) : nullptr;
        if (!l) continue;
        labels_[i] = {{l->left(), l->top(), l->left() + l->width(), l->top() + l->height()}, l->string("Caption"),
                      l->integer("Font.Height", -13)};
    }
    const int clientHeight = form ? form->integer("ClientHeight", 200) : 200;
    setBounds(form ? form->left() : 172, form ? form->top() : 98, windowWidth(form, kFrame, 321),
              clientHeight + kNonClientTop + kFrame);
    update();
}

void JukeboxDialog::update() {  // Image.Picture := (on ? ImageN : ImageN+1).Picture
    soundsIcon_.icon = (*sounds_ ? soundsOn_ : soundsOff_).icon;
    musicIcon_.icon = (*music_ ? musicOn_ : musicOff_).icon;
    helpIcon_.icon = (*audioHelp_ ? helpOn_ : helpOff_).icon;
    repaint();
}

void JukeboxDialog::paintClient(HDC dc) {
    background_.paint(dc);
    soundsIcon_.paint(dc), musicIcon_.paint(dc), helpIcon_.paint(dc), exit_.paint(dc);
    SetBkMode(dc, OPAQUE);
    SetBkColor(dc, RGB(0, 0, 0));
    SetTextColor(dc, RGB(0, 255, 255));  // clAqua on clBlack
    for (const Text& t : labels_) {
        HFONT font = CreateFontW(t.height, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, L"System");
        HGDIOBJ old = SelectObject(dc, font);
        TextOutA(dc, t.rect.left, t.rect.top, t.text.c_str(), int(t.text.size()));
        SelectObject(dc, old);
        DeleteObject(font);
    }
}

void JukeboxDialog::mouseDown(int x, int y, int button) {
    if (button == 1) {  // the images' MouseDown with the right button: hints
        if (soundsIcon_.contains(x, y)) showHint(x, y, 128);      // Image4MouseDown 1000:0b27
        else if (musicIcon_.contains(x, y)) showHint(x, y, 127);  // Image7MouseDown 1000:0bb8
        else if (helpIcon_.contains(x, y)) showHint(x, y, 129);   // Image11MouseDown 1000:0c48
        else if (exit_.contains(x, y)) showHint(x, y, 155);       // Image8MouseDown 1000:0e9b
        return;
    }
    if (soundsIcon_.contains(x, y)) *sounds_ = !*sounds_, update();          // 1000:0b27
    else if (musicIcon_.contains(x, y)) *music_ = !*music_, update();        // 1000:0bb8
    else if (helpIcon_.contains(x, y)) *audioHelp_ = !*audioHelp_, update(); // 1000:0c48
    else if (exit_.contains(x, y)) sound_(92), close();                      // Image8Click 1000:0aa8
}

AboutDialog::AboutDialog(App& app, int level, std::function<void(int)> sound) : Dialog(app, 3, true), sound_(std::move(sound)) {
    const dfm::Component* form = app.assets().form(level, "TABOUTME");
    colour = RGB(0, 0, 0);
    caption = L"About This Program";
    loadBmp(app.assets().files().read("a912.asl"), &picture_);  // FormActivate 1000:371b
    const int clientHeight = form ? form->integer("ClientHeight", 313) : 313;
    setBounds(form ? form->left() : 260, form ? form->top() : 119, windowWidth(form, kFrame, 277),
              clientHeight + kNonClientTop + kFrame);
    sound_(108);  // FormShow 1000:368e
}

void AboutDialog::paintClient(HDC dc) {
    if (picture_.valid()) BitBlt(dc, 0, 0, clientWidth(), clientHeight(), picture_.dc(), 0, 0, SRCCOPY);
}

void AboutDialog::mouseDown(int x, int y, int) {  // FormMouseDown 1000:36a3
    if (clientHeight() - 40 >= y) return;
    const int right = clientWidth() / 2 + 20, left = right - 40;
    if (x >= left && x <= right) sound_(92), close();
}

OrdersDialog::OrdersDialog(App& app, int level, std::function<void(int)> sound) : Dialog(app, 3) {
    const dfm::Component* form = app.assets().form(level, "TORDERS");
    colour = RGB(0, 0, 0);
    caption = L"How to get your own copy of PaintCraft";
    imageFromForm(form, exit_, "Image1");
    if (const dfm::Component* m = form ? form->find("Memo1") : nullptr)
        memo_ = {m->left(), m->top(), m->left() + m->width(), m->top() + m->height()};
    const std::vector<uint8_t> text = app.assets().files().read("a945.asl");  // FormShow 1000:026a
    std::string line;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') continue;
        if (text[i] == '\n') lines_.push_back(line), line.clear();
        else line += char(text[i]);
    }
    if (!line.empty()) lines_.push_back(line);
    const int clientHeight = form ? form->integer("ClientHeight", 273) : 273;
    setBounds(form ? form->left() : 95, form ? form->top() : 70, windowWidth(form, kFrame, 427),
              clientHeight + kNonClientTop + kFrame);
    sound(2);
}

void OrdersDialog::paintClient(HDC dc) {
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &memo_, white);
    DeleteObject(white);
    // ScrollBars = ssBoth: plain Windows 95 scroll bars along the right and bottom edges
    HBRUSH grey = CreateSolidBrush(RGB(192, 192, 192));
    const RECT vertical{memo_.right - 16, memo_.top, memo_.right, memo_.bottom - 16};
    const RECT horizontal{memo_.left, memo_.bottom - 16, memo_.right - 16, memo_.bottom};
    const RECT corner{memo_.right - 16, memo_.bottom - 16, memo_.right, memo_.bottom};
    FillRect(dc, &vertical, grey), FillRect(dc, &horizontal, grey), FillRect(dc, &corner, grey);
    DeleteObject(grey);
    HFONT font = CreateFontW(-13, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, L"System");
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    SaveDC(dc);
    IntersectClipRect(dc, memo_.left, memo_.top, memo_.right - 16, memo_.bottom - 16);
    int y = memo_.top;
    for (const std::string& l : lines_) {
        TextOutA(dc, memo_.left + 1, y, l.c_str(), int(l.size()));
        y += 16;
    }
    RestoreDC(dc, -1);
    SelectObject(dc, old);
    DeleteObject(font);
    exit_.paint(dc);
}

void OrdersDialog::mouseDown(int x, int y, int) {  // Image1Click 1000:02b2
    if (exit_.contains(x, y)) close();
}

FillStylesDialog::FillStylesDialog(App& app, int level, bool paintCan, int* style, std::function<void(int)> sound)
    : Dialog(app, 0), style_(style) {
    const dfm::Component* form = app.assets().form(level, "TFILLSTYLES");
    colour = RGB(0, 0, 0);
    imageFromForm(form, chart_, "Image1");  // at (0, -1)
    const int width = form ? form->integer("ClientWidth", 257) : 257;
    if (paintCan) setBounds(82, 280, width, 68);  // FormShow 1000:3933
    else setBounds(82, 94, width, 222);
    sound(90);
}

void FillStylesDialog::paintClient(HDC dc) {
    chart_.paint(dc);
}

void FillStylesDialog::mouseDown(int x, int y, int button) {  // Image1MouseDown 1000:3994
    if (button != 0) return;  // TODO: hints (Image1MouseMove 1000:3a63, Timer2 1000:3a3a)
    y -= chart_.rect.top;     // Image1's coordinates
    if (y < 34) *style_ = x / 32;
    if (y >= 34 && y < 68) *style_ = x / 32 + 8;
    if (y >= 68 && y < 94) *style_ = x / 32 + 16;
    if (y >= 94 && y < 160) *style_ = x / 64 + 24;
    if (y >= 160) *style_ = x / 64 + 28;
    close();
}
