// The paint program (Mainunit / Tmain of CRAYON1.EXE and CRAYON2.EXE). Addresses in comments
// are CRAYON1's (Ghidra selector 1008 = segment 2, 1028 = segment 6); see docs/re/mainunit.md.
// Globals keep their DGROUP address in the name until their meaning is known.
#pragma once

#include <memory>

#include "app.h"
#include "dialog.h"
#include "hints.h"
#include "tabset.h"

class Studio : public Scene {
public:
    Studio(App& app, int level) : Scene(app), level_(level) {}
    void start(long now) override;
    void tick(long now) override;
    void mouseDown(int x, int y, int button) override;
    void mouseMove(int x, int y, int buttons) override;
    void mouseUp(int x, int y, int button) override;
    void views(std::vector<const View*>* out) const override;
    HCURSOR cursor(int x, int y) override;
    void keyChar(wchar_t c) override;  // FormKeyPress 1008:9951
    void musicNotify() override;       // MM_MCINOTIFY in AppMessage 1008:6019

private:
    // Tmain events
    void formPaint();                                   // 1008:6390
    void formMouseDown(int x, int y, int button);       // 1008:6ef9
    bool optionClicked(int option);                     // 1008:7790 (options strip); false = skip the panel redraw
    void showDialog(std::unique_ptr<Dialog> dialog);    // ShowModal; its onClose continues the caller
    void finishDialog();                                // run ShowModal continuation after a click or Enter
    void choosePicture(int picker, std::function<void(bool chosen)> then);  // 1008:4c27 + Tthumbs
    void loadStretched();                               // 1008:5256: DS:4712 stretched over the picture
    void savePicture();                                 // 1008:5012: user\YPxxxxxx.BMP
    void printPicture();                                // 1008:4db4
    void startMusic();                                  // 1008:61ac: the next of snd\pc1..20.mid
    void stopMusic();                                   // 1008:62d8
    void placeClipArt(int x, int y);                    // 1010:2948 (mode 0x33)
    void tabClicked(int tab);                           // TabSet1Click 1008:91d2
    void selectTool(int tab);                           // 1008:8fc7
    void timer1();                                      // 1008:8e10
    // painters
    void restoreLeftStrip();                            // 1008:2d76
    void drawShades();                                  // 1008:29fd
    void drawToolPointer(int tab);                      // 1008:3a68
    void drawPanels();                                  // the flag-driven list at 1008:83a4
    void drawPanels2();                                 // CRAYON2's (2:a41a)
    void drawPanel(int y, const char* file);            // the shared body of 1008:2eb1…3969
    void paintTabs() { tabSet_.paint(tabs_.surface); }
    // 1028 helpers
    void playSound(int n);                              // 1028:000f
    void saveBackup();                                  // 1028:0127: BiPict1 -> DS:69ec
    void restoreBackup();                               // 1028:0281: DS:69ec -> BiPict1
    void restoreUndo();                                 // 1028:032e: DS:69f0 -> BiPict1
    void clearPanelFlags();
    int tool() const;  // DS:1ab8 as CRAYON1 numbers it (level 2's 3D, shadow text, Magic Hat = 13–15)
    void notYet(const char* what);
    // Right-click hints (studio_hints.cpp)
    void loadHints();                                   // FormCreate 1008:6466: a939.asl
    int formHint(int x, int y) const;                   // 1008:081c: 300 = none
    int formHint2(int x, int y) const;                  // CRAYON2's map (2:0846)
    void showHint(int x, int y, int hint);              // 1008:034f: Panel1 + snd\hv<n+1>.asl
    void hideHint();                                    // the shared hide in the mouse handlers
    void saveUndo();                                    // 1028:01d4: BiPict1 -> DS:69f0
    void loadCursors();                                 // FormCreate: Screen.Cursors[1..37]
    void modeCursor();                                  // 1008:69d6: Screen.Cursor for the mode
    // BiPict1 (studio_paint.cpp): events, the per-mode dispatch and the drawing routines.
    void pictureMouseDown(int x, int y, int button);   // BiPict1MouseDown 1008:8cfc
    void pictureMouseMove(int x, int y);               // BiPict1MouseMove 1008:8d84
    void pictureMouseUp(int x, int y);                 // BiPict1MouseUp 1008:84bd
    void press(int x, int y);                          // 1008:0d0a
    void drag(int x, int y);                           // 1008:1644
    void release(int x, int y);                        // the case in 1008:84bd
    int random(int range);                             // System.Random (1078:346d)
    COLORREF strokeColour();                           // 1008:1652: the clown picks any shade
    HDC pictureDC() { return picture_.surface.dc(); }  // GetDC(BiPict1.Handle), DS:1af6
    void drawLine(int x1, int y1, int x2, int y2, int width, COLORREF c);  // 1028:1a1f
    void updateShadeIndexes();                                            // 1028:2076
    void setupBrushColours();                                             // 1028:2da9
    void brushBand(int x1, int y1, int x2, int y2, int direction);        // 1028:308e
    void spray(int x, int y, int width, int kind);                        // 1028:2adf
    void mirrorLine(int x, int y, int kind, COLORREF c);                  // 1028:3274
    void speedLine(int x1, int y1, int x2, int y2, COLORREF c);           // 1028:3456
    void splatter(int x, int y, COLORREF c);                              // 1028:35b7
    void burst(int x, int y, COLORREF c);                                 // 1028:3875
    void dripSwipe(int x1, int y1, int x2, int y2);                       // 1028:3b40
    void shadedShape(int x, int y, int rx, int ry, int shape, int light, int family);  // 1028:20f0
    void grass(int x, int y, int length);                                 // 1028:2786
    void fan(int x, int y, COLORREF c);                                   // 1028:285a
    void stamp(int x, int y, int kind, COLORREF c);                       // 1028:28df
    void roller(int x, int y, int direction);                             // 1028:3ec1
    void bubble(int x, int y, COLORREF c);                                // 1028:40de
    void ball(int x, int y, COLORREF c);                                  // 1028:41fa
    void ringBurst(int x, int y, int kind);                               // 1028:4277
    void beadLine(int x1, int y1, int x2, int y2, COLORREF c);            // 1028:379b
    void arcBrush(int x, int y, int width, COLORREF c);                   // 1028:1ac0
    void eraserSquare(int x, int y);                                      // 1028:1c5b
    HBRUSH fillBrush(COLORREF c, int style);                              // 1028:174f
    void floodFill(int x, int y, int style);                              // 1028:7170
    void magicFill(int x, int y);                                         // 1028:79f2
    // The shapes tool: an XOR rubber band while dragging (phase 1), the final filled shape on
    // release (phase 2). 1028:4e8f rectangle, 5175 rounded, 54f3 ellipse, 57ea triangle,
    // 5e1e trapezoid, 1010:218c diamond, 1b93 hexagon, 11df octagon, 09e6 star; fill styles
    // 16+ continue in 1028:48ae / 1028:44c6.
    enum class Shape { Rectangle, Rounded, Ellipse, Triangle, Trapezoid, Diamond, Hexagon, Octagon, Star };
    void rubberShape(Shape shape, int x, int y, int kind, int phase);
    std::vector<POINT> shapePoints(Shape shape, int kind, int x, int y) const;
    void shadedFill(int shapeId, int x1, int y1, int x2, int y2, int rw = 0, int rh = 0);  // 1028:48ae / 44c6
    void boxOutline(int x, int y);                                        // 1028:6b79
    void parallelogram(int x, int y, int direction, int phase);           // 1028:6c74
    void straightLine(int x, int y, int phase);                           // 1010:1775
    // The 3D tool (level 2): shaded shapes, ring, tube, box, smudge and smear.
    void shaded3D(int shape, int x, int y, int phase);                    // 1028:64b7 / 6869 / 6649
    void ring3D(int x, int y, int phase);                                 // 1010:25db
    void tube3D(int x, int y, int phase);                                 // 1010:2d86
    void box3D(int x, int y, int phase, int front, int depth, int side);  // 1028:72c2
    void smudge(int x, int y);                                            // 1028:1d5e
    void smear(int x, int y, int kind);                                   // 1028:1f31
    // The Magic Hat (level 2): boxes on the picture and what is done to them.
    void selectBox(int x, int y);                                         // 1018:2118
    void eraseSelectBox();                                                // 1018:21ff
    void markSelection();                                                 // 1018:22ca
    void grabSelection(int x, int y);                                     // 1008:8bc2
    void moveSelection(int x, int y, int fromX, int fromY, int kind);     // 1010:2b6a
    void mosaic();                                                        // 1018:168d
    void randomMosaic();                                                  // 1018:190f
    void resizeArea(int x, int y, int phase, int kind);                   // 1018:1a85
    void flipArea(int x, int y, int phase, int kind);                     // 1018:1c75
    void negativeArea(int x, int y, int phase);                           // 1018:1e7d
    void replaceColour(int x, int y);                                     // 1018:1ff6
    // Eraser and magic effects (segment 4, selector 1018; studio_effects.cpp).
    void frame(int delayMs) { app_.host().showFrame(delayMs); }
    void whiteOut();                     // 1018:04db
    void wipe(int kind);                 // 1018:0549: kinds 1–4 lines, 5–8 squares (0788)
    void squares(int kind);              // 1018:0788
    void shrinkAway();                   // 1018:0002
    void sweepUp();                      // 1018:0147
    void shatter();                      // 1018:02f9
    void horizon();                      // 1018:157c
    void squiggle();                     // 1018:17d8
    void dynamite(int x, int y);         // 1018:0a5c
    void train(int x, int y);            // 1018:0d3c
    void walker(int x, int y);           // 1018:1136
    // The ABCD tool (mode 0x51): typing on the picture.
    void textStart(int x, int y);                                         // 1008:9549
    void textKey(uint8_t key);                                            // 1008:95c4
    void textFinish();                                                    // 1008:97cb
    void drawCaret();                                                     // 1008:9465
    HFONT textFont();
    int shadowTextWidth(int height);                                      // 1008:3c3f
    int shadowFitHeight(int width, int gap);                              // shared by 1008:41aa/3cfc/47fb/4628
    void shadowBoxText(int x, int y);                                     // 1008:41aa/3cfc/47fb/4628
    void shadowCenteredText(int x, int y, int height, COLORREF colour);   // 1008:451d
    void shadowRotatedText(int x, int y);                                 // 1008:43c0
    COLORREF shade(int k) const { return colour(shadeIndex_[k]); }        // palette[DS:1a16/1a1a/1a1e]
    void unported(const char* routine);

    COLORREF colour(int index) const;  // the program palette (DS:167a + 4*index)
    HDC formDC() { return form_.surface.dc(); }
    int pictureWidth() const { return picture_.surface.width() + 2; }    // DS:1ac6 = BiPict1.Width
    int pictureHeight() const { return picture_.surface.height() + 2; }  // DS:1ac8

    int level_;
    View form_, tabs_, picture_, hint_;  // Tmain, TabSet1, BiPict1, Panel1
    TabSet tabSet_;
    std::vector<HICON> tabIcons_;
    Surface backup_, undo_, optionSheet_;  // DS:69ec, DS:69f0, DS:69f4
    long timer1Due_ = 0;
    int timer1Interval_ = 125;
    bool timer1On_ = true;

    // Mainunit globals (FormCreate 1008:5551 defaults)
    bool firstPaint_ = true;     // DS:1040
    int family_ = 9, shade_ = 5;  // DS:1ad4, DS:1ad2
    COLORREF current_ = 0;        // DS:1672: the drawing colour
    int mode_ = 1;                // DS:1ac0: what dragging on the picture does
    int tool_ = 1;                // DS:1ab8: tab + 1
    int picker_ = 0;              // DS:1ab6: picture set for the Tthumbs dialog
    int thickness_ = 2;           // DS:1ac2 (1..10)
    int pointerTab_ = 0;          // DS:0fd0
    int frame_ = 0, frameBack_ = 0;  // DS:1002, DS:1000: option sheet animation
    bool multiColour_ = false;    // DS:1041: the clown (true) / business man (false)
    bool soundsOn_ = true;        // DS:105f
    bool musicOn_ = true;         // DS:1060
    bool audioHelp_ = true;       // DS:1061: spoken hints
    bool musicPlaying_ = false;   // DS:1062
    int musicTrack_ = 0;          // DS:0fd4
    int w1abe = 2, w1aa8 = 1, w1aa6 = 0x2d;
    int w0ffc = 1, w0ffa = 4, w0ff8 = 2, w0ff6 = 1, w0ff4 = 3, w0ff2 = 1, w0ff0 = 1, w0fee = 2, w0fec = 1,
        w0fea = 1, w0fe8 = 1, w0fe6 = 1, w0fe4 = 2, w0fe2 = 4, w0fe0 = 2, w0fde = 3, w0fdc = 1, w0fda = 1,
        w0fd8 = 4, w0fd6 = 2;
    // option-panel flags DS:1042…105e (1042 thickness, 1046 clown/business man)
    bool b1042 = true, b1043 = false, b1044 = false, b1045 = false, b1046 = false, b1047 = false, b1048 = false,
         b1049 = false, b104a = false, b104b = false, b104c = false, b104d = false, b104e = false, b104f = false,
         b1050 = false, b1051 = false, b1052 = false, b1053 = false, b1054 = false, b1055 = false, b1056 = false,
         b1057 = false, b1058 = false, b1059 = false, b105b = false, b105c = false, b105e = false;
    std::string font_ = "Arial";  // DS:1068 (text tool font name)
    std::string shadowFont_ = "Arial";  // DS:1168 (FormCreate 1008:56b3)
    std::unique_ptr<Dialog> dialog_;
    std::string chosen_;           // DS:4712: the picture chosen in Tthumbs (data-relative)
    int clipOrientation_ = 4;      // DS:1aa2
    Surface clip_, clipWork_;      // DS:69fc, DS:69f8
    int lastX_ = 0, lastY_ = 0;    // DS:1aca, DS:1acc: previous mouse position while drawing
    int startX_ = 0, startY_ = 0;  // DS:1ace, DS:1ad0: where the stroke began
    bool drawing_ = false;         // DS:1005
    // Which window took the left-button press: Delphi controls capture the mouse on a press,
    // so the moves and the release go there (a dialog closed by the press keeps them).
    enum class Capture { None, Dialog, Tabs, Picture, Form } capture_ = Capture::None;
    bool b1004 = false;
    HintSource hints_;                // DS:146a: a939.asl
    bool hintShown_ = false;          // DS:1066: Panel1 shows a hint
    bool hintArmed_ = false;          // DS:1067: Timer2 fired, the next move hides it
    bool timer2On_ = false;           // Timer2 (250 ms)
    long timer2Due_ = 0;
    static constexpr int kCursors = 38;
    HCURSOR cursors_[kCursors] = {};  // Screen.Cursors[1..37] (never destroyed, like the original's)
    int screenCursor_ = 0;            // Screen.Cursor (0 = crDefault)
    bool typing_ = false;          // DS:1064
    std::string text_;             // DS:1572
    int textX_ = 0, textY_ = 0;    // DS:1a9e, DS:1aa0
    RECT caret_{};                 // DS:0fc8..0fce: x, y, width, height
    int w0ffe = 0;                 // DS:0ffe: thickness saved by some modes
    POINT points_[64] = {};        // DS:1af8: polygon points
    RECT selection_{};             // DS:1aaa, 1aac, 1aae, 1ab0: the Magic Hat's box (x1, y1, x2, y2)
    int grabX_ = 0, grabY_ = 0;    // DS:1ab2, DS:1ab4: where in the box it was grabbed
    Surface selectionPicture_;     // DS:69e8: the lifted box
    int pointCount_ = 0;           // DS:1aba
    int fillStyle_ = 1;            // DS:1ac4: 0 hollow, 1 solid, 2–7 hatches, 8–15 patterns pa00–pa07
    uint32_t randSeed_ = 0;        // System.RandSeed
    int shadeIndex_[3] = {0, 0, 0};  // DS:1a16, 1a1a, 1a1e: current shade and the two lighter ones
    COLORREF brushColours_[64] = {};  // DS:1a22: the brush band's line colours
};
