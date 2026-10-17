#include "activity.h"

namespace {

std::vector<std::string> loadLines(App& app, const std::string& path) {  // TStrings.LoadFromFile
    std::vector<std::string> lines;
    const std::vector<uint8_t> data = app.assets().files().read(path);
    std::string line;
    for (size_t i = 0; i < data.size(); ++i) {
        const char c = char(data[i]);
        if (c == '\r' || c == '\n') {
            lines.push_back(line);
            line.clear();
            if (c == '\r' && i + 1 < data.size() && data[i + 1] == '\n') ++i;
        } else {
            line += c;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

}  // namespace

ActivityDialog::ActivityDialog(App& app, int level, int picker, std::function<void(int)> sound)
    : Dialog(app, 3), sound_(std::move(sound)) {  // BorderStyle = bsSingle
    const dfm::Component* form = app.assets().form(level, "TACTIVITY");
    colour = RGB(0, 0, 0);
    auto fromForm = [&](ImageBox& box, const char* name) { loadImage(box, form ? form->find(name) : nullptr); };
    fromForm(ok_, "Image1");
    fromForm(exit_, "Image2");
    fromForm(thumb_, "Image3");
    if (const dfm::Component* l = form ? form->find("ListBox1") : nullptr) {
        titles_.rect = {l->left(), l->top(), l->left() + l->width(), l->top() + l->height()};
        titles_.itemHeight = l->integer("ItemHeight", 16);
        titles_.fontHeight = l->integer("Font.Height", -17);
        titles_.colour = RGB(0, 0, 0);
        titles_.textColour = RGB(255, 255, 255);
    }
    // FormShow 1000:169a
    const std::string n = std::to_string(picker - 11);
    static const wchar_t* const kCaptions[] = {L"Things to do!", L"3D things to create!", L"MagiColor Pictures", L"Storybooks"};
    if (picker >= 11 && picker <= 14) {
        titles_.items = loadLines(app, "activity\\z10" + n + ".asl");
        thumbs_ = loadLines(app, "activity\\z20" + n + ".asl");
        pages_ = loadLines(app, "activity\\z30" + n + ".asl");
        caption = kCaptions[picker - 11];
    }
    titles_.itemIndex = 0;
    const int width = (form ? form->integer("ClientWidth", 354) : 354) + 2 * kFrame;
    const int height = (form ? form->integer("ClientHeight", 291) : 291) + kNonClientTop + kFrame;
    setBounds(form ? form->left() : 251, form ? form->top() : 103, width, height);
    preview();
}

void ActivityDialog::preview() {
    const int i = titles_.itemIndex;
    if (i < 0 || i >= int(thumbs_.size())) return;
    Surface picture;
    if (loadBmp(app_.assets().files().read("activity\\" + thumbs_[size_t(i)]), &picture)) {
        thumb_.picture = std::move(picture);
        thumb_.rect.right = thumb_.rect.left + thumb_.picture.width();
        thumb_.rect.bottom = thumb_.rect.top + thumb_.picture.height();
    }
    repaint();
}

void ActivityDialog::paintClient(HDC dc) {
    titles_.paint(dc);
    thumb_.paint(dc), ok_.paint(dc), exit_.paint(dc);
}

void ActivityDialog::mouseDown(int x, int y, int button) {
    if (button == 1) {  // right button: hints (1000:113c) at the left edge
        if (thumb_.contains(x, y)) showHint(0, y, 172);          // Image3MouseDown 1000:1bff
        else if (PtInRect(&titles_.rect, POINT{x, y})) showHint(0, 0, 173);  // ListBox1MouseDown 1000:1da2
        else if (ok_.contains(x, y)) showHint(0, y, 174);        // Image1MouseDown 1000:1dc0
        else if (exit_.contains(x, y)) showHint(0, y, 175);      // Image2MouseDown 1000:1dec
        return;
    }
    const int item = titles_.hit(x, y);
    if (item >= 0) {  // ListBox1Click 1000:1a46
        titles_.itemIndex = item;
        sound_(34);
        preview();
    } else if (ok_.contains(x, y)) {  // Image1Click 1000:1b25
        const int i = titles_.itemIndex;
        if (i >= 0 && i < int(pages_.size())) path = "activity\\" + pages_[size_t(i)], chosen = true;
        close();
    } else if (exit_.contains(x, y)) {  // Image2Click 1000:1ac0
        sound_(36);
        close();
    }
}
