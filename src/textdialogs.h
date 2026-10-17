#pragma once

#include "dialog.h"

class TextInputDialog : public Dialog {
public:
    TextInputDialog(App& app, int level, std::string* text, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;
    void keyChar(wchar_t character) override;

private:
    void accept();  // 1000:32bf
    void cancel();  // 1000:3237
    std::string* text_;
    std::string entry_;
    bool selected_ = true;
    std::function<void(int)> sound_;
    ImageBox okay_, cancel_;
    RECT edit_{};
};

class RotatorDialog : public Dialog {
public:
    RotatorDialog(App& app, int level, std::string* text, int* angle, COLORREF colour,
                  std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;
    void keyChar(wchar_t character) override;
    void tick(long now) override;

private:
    void accept();  // 1000:2744
    void cancel();  // 1000:28e1
    void setAngle(int value);  // 1000:2728
    std::string* text_;
    int* angle_;
    COLORREF colour_;
    std::string entry_;
    bool selected_ = true, initialized_ = false;
    long nextTick_ = 0;
    std::function<void(int)> sound_;
    ImageBox okay_, cancel_;
    RECT edit_{};
};

class FontDialog : public Dialog {
public:
    FontDialog(App& app, int level, std::string* face, std::function<void(int)> sound);
    void paintClient(HDC dc) override;
    void mouseDown(int x, int y, int button) override;

private:
    std::string* face_;
    std::function<void(int)> sound_;
    ImageBox okay_;
    ListBoxView list_;
    RECT sample_{};
};
