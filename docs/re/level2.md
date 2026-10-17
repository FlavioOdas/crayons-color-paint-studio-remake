# Level 2 (CRAYON2.EXE) — differences from CRAYON1

CRAYON2 is the same code base as CRAYON1 built with more tools enabled. Established with
`tools/fnmatch.py` (pairs functions by normalised code: 262 identical, 55 near-identical, 8 new
in segments 1–6), `tools/dsmap.py` (globals) and `tools/fndiff.py` / `tools/all_cases.py`.

## Globals

Every CRAYON2 DS global is CRAYON1's + 0x0E (FormCreate's 77 assignments are identical after
the shift). The remake keeps CRAYON1's names; below, "C2 1062 (= 1054)" means CRAYON2's address
and its CRAYON1 equivalent.

Form variables: C1 gfc4 Tmain, g102a Tthumbs, g1026 TFillStyles, g1022 TFontForm (segment 3),
g101c Taboutme, g1018 Ttextinpt, g1012 Trotator, g100e TActivity, g100a Tjukebox, g1006
Torders. C2: gfd2, g1038, g1034, g1030 (TFontForm, now segment 1), g102a, g1026, g1020, g101c,
g1018, g1014 — the same forms, + 0x0E.

## Tools

TabSet1 has 11 tabs (icons 1, 2, 3, 4, 5, 6, 12, 7, 8, 9, 11); DS:1ab8 = tab + 1 as in
CRAYON1. The option×tool table (C2 FormMouseDown 2:868c, tables cs:8568…) has the same cases as
CRAYON1's under this mapping of tools (C2 → C1):

| C2 | tab | tool | C1 |
|---|---|---|---|
| 1 | 0 | crayon | 1 |
| 2 | 1 | paintbrush | 2 |
| 3 | 2 | clown | 3 |
| 4 | 3 | 3D | 13 |
| 5 | 4 | shapes | 4 |
| 6 | 5 | shadow text ("Font") | 14 |
| 7 | 6 | ABCD | 5 |
| 8 | 7 | eraser | 6 |
| 9 | 8 | Magic Hat | 15 |
| 10 | 9 | pictures | 7 |
| 11 | 10 | PC | 9 |
| 12 | — | coloring book / Magic Color / activities (no tab) | 8 |

So CRAYON1's unreachable tools 13–15 are level 2's 3D, shadow text and Magic Hat. Option
sheets (2:b10c): a900, a901, a902, a903, a904, a905, a914, a906, a907, a908, a910.

Differences in the option cases (everything else identical):
- CRAYON1 resets a variant when an option is chosen; CRAYON2 keeps it (it has panels to pick
  it): brush options 1/4 `w0ffc = 3`, option 3 `w0ffa = 3`; shapes options 2/4/8/9
  `w0fe6/w0fe8/w0fea = 1`, `w0fe2 = 3`; crayon option 5 `w0fe4 = 2` — CRAYON1 only.
- Crayon option 7: C1 straight line (0x26); C2 mode `w0ff0 == 1 ? 0x0c : 0x0d` + b104a.
  Crayon option 8: C2 straight line 0x26 + b1042 (C1 nothing).
- Brush option 8: C2 mode 0x0f + b1042 + b1043; option 9: C2 mode 0x0e + b1042 + b1043.
- Clown option 8: C1 horizon (1018:157c); C2 grass mode 0x04 + b104b. Option 9: C1 squiggle
  (1018:17d8); C2 nothing.
- Tool 12 (no tab): coloring book, Magic Color and the activity pickers 0x0b/0x0c.

## Option panels

CRAYON2 paints 25 panels (FormMouseDown tail 2:a41a; FormPaint lists the same set in another
order). Painter, flag (C1 name), slot y, variable → picture:

| painter | flag | y | picture |
|---|---|---|---|
| 46b9 | 104f | 330 | w0fea 1 a51, 2 a52 |
| 48d1 | 1050 | 330 | w0fe8 1 a53, 2 a54 |
| 49e5 | 1051 | 330 | w0fe6 1..3 a55..a57 |
| 47c5 | 1053 | 330 | w0fe2 3 a61, 4 a62 |
| 30fd | 1042 | 230 | thickness 1..10 a29..a20 |
| 3535 | 105c | 280 | a84 ("Font") |
| 3385 | 105e | 330 | w0fd6 2..7 a90..a95 |
| 3660 | 1054 | 230 | w0fde 1..10 a79..a70 |
| 38cc | 1055 | 280 | w0fe0 2 a80, 4 a81, 5 a82, 7 a83 |
| 4f23 | 1043 | 280 | w0ffc 1 a11, 2 a12, 3 a13, 4 a14 |
| 5088 | 1044 | 280 | w0ffa 4 a7, 3 a6 |
| 3a33 | 1056 | 280 | w0fdc 1 a65, 2 a64 |
| 44a1 | 1057 | 230 | w0fda 1 a8, 2 a9 |
| 428b | 1045 | 230 | w0ff8 2 a8, 1 a9 |
| 4397 | 1058 | 230 | w0fd8 4 a66, 3 a67 |
| 4e0d | 1046 | 380 | DS:1041 ? a17 : a18 |
| 4bf5 | 1047 | 230 | w0ff6 1 a15, 2 a16 |
| 3d91 | 1048 | 230 | w0ff4 3 a30, 1 a31, 2 a32 |
| 3ed2 | 1049 | 230 | w0ff2 1..3 a33..a35 |
| 4d01 | 104a | 230 | w0ff0 1 a37, 2 a36 |
| 4154 | 104b | 230 | w0fee 1..3 a38..a40 |
| 45ad | 104c | 280 | w0fec 1 a41, 3 a42 |
| 4013 | 1052 | 280 | w0fe4 2 a58, 1 a59, 3 a60 |
| 3b7f | 104e | 230 | DS:1abe 2 a43, 3 a44, 4 a45, 5 a49, 6 a48, 7 a47, 8 a46, 9 a50 |
| 4b15 | 104d | 280 | a3 (fill styles) |

Clicks on them (C2 FormMouseDown, in this order, each with sound 96 and the panel repaint):
thickness arrows; b1054 arrows on w0fde 1..10 (4..24 / 54..76, y 256..277); b1055 quadrants of
280..330 → w0fe0 2/4 (top) 5/7 (bottom); b1043 → w0ffc 1 (6..32, 282..308), 3 (5..38,
313..323), 2 (40..63, 282..310), 4 (64..76, 296..326); b1044 → w0ffa 4 (left) / 3 (right);
b104d fill styles; b105c → TFontForm (no sound; saveBackup, ShowModal, Refresh,
restoreBackup); b1047, b1045, b1058, b1057, b104c as CRAYON1; b104f → w0fea 1/2 and repaint
46b9 at once; b1053 → w0fe2 3/4 and 47c5; b1056, b1050, b1048 as CRAYON1; b1052 thirds of
280..330 → w0fe4 2/1/3; b105e six cells of 330..380 → w0fd6 2,3,4 / 5,6,7; b1051 thirds of
330..380 → w0fe6 1/2/3; b1049 thirds of 230..280 → w0ff2 1/2/3 with mode 0x1e/0x1d/0x14;
b104e 230..310 grid → DS:1abe 2,3,4 / 9,–,8 / 5,6,7 (light direction); b104a halves →
mode 0x0d w0ff0 2 / mode 0x0c w0ff0 1; b104b thirds → w0fee 1/2/3; b1046 as CRAYON1.

## Hints

FormPaint loads `a938.asl` (CRAYON1 `a939.asl`). The right-click map is 2:0846 (CRAYON1
1008:081c) — to port.

## Done in the remake

Tool mapping (`Studio::tool()`), option differences, sheets, the 25 panels and their clicks
(`drawPanels2`, `formMouseDown`), hints (`formHint2`, a938.asl, tab hints 130 + i), the 3D tool
(`shaded3D`, `ring3D`, `tube3D`, `box3D`, `smudge`, `smear`), the Magic Hat
(`selectBox` … `replaceColour`), and shadow text plus its dialogs and Font form.

## Shadow text (tool 14) — decoded and ported

All addresses CRAYON1 (the code exists there, unreachable); CRAYON2 equivalents via
`work/fnmatch12.txt`. Text buffer = DS:1572 (the ABCD tool's buffer, a PChar, 256 bytes; pcalls
prints it as `ds:5490` — **pcalls shows DS offsets of pushed addresses in decimal**). Shadow
font name = Pascal string DS:1168 (`ds:4456`), set by TFontForm. DS:1aa6 is reused: the fitted
font height (41aa) and the rotation angle (Trotator; FormCreate default 45).

Modes (options of tool 14: 0 → 0x4b + b105c; 1 → 0x4d + b105c, b105e; 2 → 0x4e + b105c;
3 → 0x4f + b105c, b1042; 4 → 0x4c + b105c, b105e):
- 0x4b–0x4e press/drag: `selectBox` (1018:2118, the XOR box). Release (only if x ≠ start and
  y ≠ start): `eraseSelectBox` (21ff), saveBackup, Screen.Cursor := crDefault, Ttextinpt
  ShowModal (C1 g1018), BiPict1.Refresh, restoreBackup, then sound + drawing with the box
  (start, end): 0x4b sound 53 → 1008:41aa(sx, sy, x, y); 0x4c sound 52 → 3cfc(sx, sy, x, y,
  w0fd6); 0x4d sound 54 → 47fb(…, w0fd6); 0x4e sound 55 → 4628(sx, sy, x, y).
- 0x4f: no press/drag case; release: saveBackup, crDefault, Trotator ShowModal (C1 g1012),
  Refresh, restoreBackup, 1008:43c0(x, y).

Drawing routines (all return at once when lstrlen(DS:1572) < 1; box normalised to
x1 ≤ x2, y1 ≤ y2; SetBkMode TRANSPARENT; font = CreateFont(-h, 0, …, face DS:1168)):
- Font fit (shared): ratio = Single(Screen.PixelsPerInch / 72.0) (96 dpi → 1.333…);
  h = 10, lo = 2, hi = 152; loop { if |hi − lo| < G or hi == lo: stop; mid = (hi − lo)/2 + lo;
  h = Round(mid · ratio); w = 1008:3c3f(h) = GetTextExtent width of DS:1572 in the font of
  height −h; if w == box width: stop; if w > box: hi = mid; if w < box: lo = mid } while
  |hi − lo| ≥ G. G = 2 in 41aa and 4628, 3 in 3cfc and 47fb. Only the width is fitted.
- 41aa (0x4b, plain): DS:1aa6 = h; TA_LEFT|TA_TOP; text colour = current; TextOut(x1, y1).
- 3cfc (0x4c, extruded): d = h/3; shadow colour = palette[family·10 + 9]; by w0fd6:
  2: shadow i = 0..d at (x1 + i, y1 + i), text at (x1 + d + 1, y1 + d + 1);
  3: (x1, y1 + i), text (x1, y1 + d + 1); 4: (x1 − i, y1 + i), text (x1 − d − 1, y1 + d + 1);
  5: (x1 + i, y1 + d + 1 − i), text (x1 + d + 1, y1); 6: (x1, y1 + d + 1 − i), text (x1, y1);
  7: (x1 + d + 1 − i, y1 + d + 1 − i), text (x1, y1). Text in the current colour last.
- 47fb (0x4d, drop shadow): d = h/10; one shadow (darkest shade) at (x1 ∓ d, y1 ∓ d) by
  w0fd6: 2 (−d, −d), 3 (0, −d), 4 (+d, −d), 5 (−d, +d), 6 (0, +d), 7 (+d, +d); then the text at
  (x1, y1) in the current colour.
- 4628 (0x4e, growing 3D): cx = (x2 − x1)/2 + x1; fit (G = 2) → h; d = box width / 8; height
  H = −h + d (i.e. h − d); for i = 0..d: if H − i < 0: 1008:451d(cx, y2 − i, H − i, darkest
  shade); finally 451d(cx, y2 − d, H − d, current colour). 451d(x, y, height, colour):
  TA_CENTER|TA_BASELINE (30), transparent, CreateFont(height, …, DS:1168), TextOut(x, y).
- 43c0 (0x4f, rotated): h = Round(thickness · 7 · ratio) (as the ABCD tool), CreateFont(−h, 0,
  escapement = orientation = DS:1aa6 · 10, …, DS:1168), TA_CENTER|TA_BASELINE, current colour,
  TextOut(x, y).

Dialogs (C1 segment 1; forms in work/forms/crayon1/TTEXTINPT, TROTATOR, TFONTFORM):
- Ttextinpt "Enter Text to be Displayed:" (bsSingle, 408×88, black; Edit1 at 9,30 321×23;
  Image1 OK at 350,3, Image2 cancel at 350,51): FormShow 1000:3377 Edit1.Text :=
  StrPas(DS:1572), select all; Image1Click 32bf: clear DS:1572, StrPCopy(DS:1572,
  Edit1.Text), hide hint, Close; Edit1KeyUp 334c: Enter → Image1Click; Image2Click 3237:
  sound 92, clear DS:1572, Edit1.Text := '', Close (so nothing is drawn); right-click hints
  149 (Image1), 150 (Image2) via 1000:2dfb.
- Trotator (377×158; Panel1 preview 125×100 at 6,5; ScrollBar1 vertical 0..359, LargeChange
  10 at 133,5; Label1 "45 Degrees" at 157,52 centred; Edit1 at 10,124; Image1 OK 299,9; Image2
  cancel 299,58; Timer1 200 ms): FormShow 27e9 enables Timer1; Timer1Timer 2833: DS:1016++,
  from the 3rd tick: Edit1.Text := StrPas(DS:1572), select all, ScrollBar1.Position :=
  DS:1aa6, preview 1000:252d, Timer1 off (DS:1016 is never reset). ScrollBar1Scroll 2728:
  DS:1aa6 = position, preview. Image1Click 2744: sound 91, DS:1572 := Edit1.Text, DS:1aa6 =
  position, Close. Image2Click 28e1: sound 92, clear DS:1572, Edit1.Text := '', Close.
  Edit1KeyUp Enter → Image1Click. Hints 151, 154.
  Preview 252d: Caption := 'Text Rotation'; in Panel1's DC: white rectangle (0,0,125,100)
  (WHITE_PEN, WHITE_BRUSH), black cross lines (57,50)-(67,50) and (62,45)-(62,55), font Arial
  height −Round(10 · ppi/72), escapement/orientation DS:1aa6·10, TA_CENTER|TA_BASELINE,
  current colour, 'Example' at (62, 50); Label1 := IntToStr(DS:1aa6) + ' Degrees'.
- TFontForm (394×161; ListBox1 at 4,2 225×157 black, ItemHeight 16; Label1 'Hello' sample
  233,3 160×80 centred; Image1 OK 296,97): CRAYON2 FormShow 1:3653 copies DS:1176
  (CRAYON1 DS:1168) into its current selection and scans Screen.Fonts for a matching item;
  Image1Click 3743 hides the hint panel, sounds 91, closes; ListBox1MouseDown 3799 sounds
  89 and stores the clicked font name in DS:1176 and Label1.Font.Name. It opens from the
  "Font" panel (b105c, FormMouseDown 2:8cff). Ported in `src/textdialogs.cpp`.

## ABCD typing tool (tool 5) — CRAYON2 comparison

`tools/fnmatch.py` pairs CRAYON2 2:b62a with CRAYON1 2:9465 (caret), and 2:b70e with
2:9549 (start): the code is identical after the +0x0e DGROUP shift. CRAYON2 2:b789 is the
font-creation prefix split from CRAYON1 2:95c4; 2:b800 is the remaining key handler and
2:b990 is the finish handler, split from CRAYON1 2:97cb (2:ba77 is a false disassembler
function start inside it). Comparing the raw calls with `tools/fndiff.py` shows the same
backspace, 253-byte limit, Enter-to-finish, text extent, TextOut, and caret behavior. Both
read the font at DS:1068 (CRAYON2 DS:1076) and text at DS:1572 (CRAYON2 DS:1580). No
level-specific implementation branch is needed. Observed in the `level2-abcd` snapshot.

Mode 0x27 (polygon, 1010:18f1) remains unreachable from either level's options.
