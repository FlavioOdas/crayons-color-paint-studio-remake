#include "app.h"

#include "launcher.h"
#include "studio.h"

void View::place(int x, int y, int w, int h, int frame, COLORREF fill) {
    rect = {x + frame, y + frame, x + w - frame, y + h - frame};
    border = frame;
    surface.create(w - 2 * frame, h - 2 * frame, fill);
}

void App::startLauncher(long now) {
    next_ = std::make_unique<Launcher>(*this);
    now_ = now;
    tick(now);
}

void App::startLevel(int level, long now) {
    next_ = std::make_unique<Studio>(*this, level);
    now_ = now;
}

void App::tick(long now) {
    now_ = now;
    if (next_) {  // scene switches happen between events, never inside one
        scene_ = std::move(next_);
        scene_->start(now);
    }
    if (scene_) scene_->tick(now);
    if (next_) {
        scene_ = std::move(next_);
        scene_->start(now);
    }
}

void App::compose(Surface& screen) const {
    PatBlt(screen.dc(), 0, 0, screen.width(), screen.height(), BLACKNESS);
    if (!scene_) return;
    std::vector<const View*> views;
    scene_->views(&views);
    for (const View* v : views) {
        if (!v->visible || !v->surface.valid()) continue;
        if (v->border) {
            HBRUSH brush = CreateSolidBrush(v->borderColour);
            const RECT frame{v->rect.left - v->border, v->rect.top - v->border, v->rect.right + v->border,
                             v->rect.bottom + v->border};
            FillRect(screen.dc(), &frame, brush);
            DeleteObject(brush);
        }
        v->surface.blitTo(screen.dc(), v->rect.left, v->rect.top);
    }
}
