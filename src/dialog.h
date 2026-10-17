// A Delphi form shown modally over the studio, drawn with Windows 95 chrome as in the manual's
// screenshots (4-px 3D frame, 18-px navy caption, 1-px separator: outer = client + 8 x 27).
// Coordinates: Left/Top/Width/Height are the window's (screen = the 640x480 logical screen),
// event coordinates are client coordinates.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "app.h"
#include "hints.h"

class Dialog {
public:
    static constexpr int kCaption = 18;
    explicit Dialog(App& app, int frame = 4, bool systemMenu = false)
        : app_(app), kFrame(frame), kNonClientTop(frame ? frame + kCaption + 1 : 0), systemMenu_(systemMenu) {}
    virtual ~Dialog() = default;

    void setBounds(int left, int top, int width, int height);  // window size, like Delphi
    int clientWidth() const { return width_ - 2 * kFrame; }
    int clientHeight() const { return height_ - kNonClientTop - kFrame; }
    RECT clientRect() const;  // on the screen
    const View& view() const { return view_; }
    void repaint();  // chrome + paintClient
    bool closed() const { return closed_; }

    virtual void paintClient(HDC dc) = 0;
    virtual void mouseDown(int x, int y, int button) { (void)x, (void)y, (void)button; }
    virtual void mouseMove(int x, int y) { (void)x, (void)y; }
    virtual void keyChar(wchar_t c) { (void)c; }
    virtual void tick(long now) { (void)now; }
    virtual HCURSOR cursor(int x, int y) { (void)x, (void)y; return LoadCursor(nullptr, IDC_ARROW); }
    void chromeMouseDown(int x, int y, int button);

    // Right-click hints: each form's own Panel1 and Timer2 (250 ms), e.g. Tjukebox 1000:0570.
    void setHints(const HintSource* hints) { hints_ = hints; }
    void hintTimer(long now);  // Timer2Timer: from now on a move hides the panel
    void hintMove();           // the controls' MouseMove handlers

    std::wstring caption;
    COLORREF colour = RGB(192, 192, 192);  // client background (Form.Color)
    std::function<void()> onClose;          // the code after ShowModal returned

protected:
    void close();  // ModalResult / Close
    void showHint(int x, int y, int hint);  // at client (x, y + 16), kept inside ClientWidth
    App& app_;
    const int kFrame;        // 4 = bsSizeable (default), 3 = bsSingle, 0 = bsNone (no chrome)
    const int kNonClientTop;

private:
    View view_;
    int left_ = 0, top_ = 0, width_ = 0, height_ = 0;
    bool closed_ = false;
    bool systemMenu_ = false;
    const HintSource* hints_ = nullptr;
    HintPanel hint_;
    bool hintTimer_ = false, hintArmed_ = false;
    long hintDue_ = 0;
};

// TImage: a picture drawn in a client rectangle (Stretch, or clipped at its top-left).
struct ImageBox {
    RECT rect{};
    Surface picture;
    std::shared_ptr<void> icon;  // a TIcon picture (HICON), drawn with DrawIcon
    bool stretch = false, visible = true;
    bool contains(int x, int y) const { return visible && x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom; }
    void paint(HDC dc) const;
};

// Loads a TImage's picture from the form: a bitmap, or an icon (AutoSize: 32x32).
void loadImage(ImageBox& box, const dfm::Component* component);

// TListBox (owner-drawn by Windows): items in a font, the selected one highlighted in navy with
// a dotted focus rectangle, as in the manual's screenshots.
struct ListBoxView {
    RECT rect{};
    std::vector<std::string> items;
    int itemIndex = -1, topIndex = 0, itemHeight = 16;
    int fontHeight = -13;
    COLORREF colour = RGB(255, 255, 255), textColour = RGB(0, 0, 0);
    bool border = true;
    int hit(int x, int y) const;  // item under a client point, or -1
    void paint(HDC dc) const;
};
