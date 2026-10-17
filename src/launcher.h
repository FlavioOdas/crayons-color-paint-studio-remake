// CRAYONST.EXE: the splash picture for 3 s (Tsplash.Timer1, 1:03cb) over the age picker
// (Tagepick); clicking "I'm 3-6 yrs." starts CRAYON1, "I'm 7-12 yrs." CRAYON2 (1:027b, 1:01ed).
#pragma once

#include "app.h"

class Launcher : public Scene {
public:
    explicit Launcher(App& app) : Scene(app) {}
    void start(long now) override;
    void tick(long now) override;
    void mouseDown(int x, int y, int button) override;
    void views(std::vector<const View*>* out) const override;

private:
    View picker_, splash_;
    RECT younger_{}, older_{};  // Image2 / Image3 click areas
    long splashUntil_ = 0;
};
