#include "launcher.h"

namespace {

RECT boundsOf(const dfm::Component* c, int dx, int dy) {
    if (!c) return {};
    return {c->left() + dx, c->top() + dy, c->left() + c->width() + dx, c->top() + c->height() + dy};
}

// A form's client picture (Image1, Align = alClient) drawn into a fresh view of the form's size,
// centred on the logical screen as Position = poScreenCenter does.
void showForm(View* view, const dfm::Component* form) {
    const int w = form ? form->width() : kScreenWidth, h = form ? form->height() : kScreenHeight;
    view->place((kScreenWidth - w) / 2, (kScreenHeight - h) / 2, w, h);
    const dfm::Component* image = form ? form->find("Image1") : nullptr;
    Surface picture;
    if (image && loadBmp(image->bitmap(), &picture)) picture.blitTo(view->surface.dc(), 0, 0);
}

}  // namespace

void Launcher::start(long now) {
    const dfm::Component* picker = app_.assets().form(0, "TAGEPICK");
    showForm(&picker_, picker);
    if (picker) {
        younger_ = boundsOf(picker->find("Image2"), picker_.rect.left, picker_.rect.top);
        older_ = boundsOf(picker->find("Image3"), picker_.rect.left, picker_.rect.top);
    }
    // Tagepick.FormShow (1:019e) shows the splash modally; its 3000 ms timer closes it.
    const dfm::Component* splash = app_.assets().form(0, "TSPLASH");
    showForm(&splash_, splash);
    const dfm::Component* timer = splash ? splash->find("Timer1") : nullptr;
    splashUntil_ = now + (timer ? timer->integer("Interval", 1000) : 3000);
}

void Launcher::tick(long now) {
    if (splash_.visible && now >= splashUntil_) splash_.visible = false;
}

void Launcher::mouseDown(int x, int y, int button) {
    if (button != 0 || splash_.visible) return;  // the modal splash takes no clicks
    const POINT p{x, y};
    if (PtInRect(&younger_, p)) app_.startLevel(1, app_.now());
    else if (PtInRect(&older_, p)) app_.startLevel(2, app_.now());
}

void Launcher::views(std::vector<const View*>* out) const {
    out->push_back(&picker_);
    out->push_back(&splash_);
}
