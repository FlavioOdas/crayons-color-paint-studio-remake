# Mainunit (Tmain) — functions and globals

CRAYON1 addresses (selector 1008 = segment 2, 1028 = segment 6). Established from the
disassembly with `tools/pcalls.py`; "inferred" marks guesses from usage.

## Startup

- `FormCreate 1008:5551`: palette (`InitPaletteTable 1028:0491`), Tool defaults (below),
  35 cursors (`Screen.Cursors[i] := LoadCursor(hInstance, 'CURSOR_n')`, same in both levels:
  1–18 → CURSOR_1–18, 19–23 → CURSOR_21–25, 24–31 → CURSOR_26–33, 32 → CURSOR_20, 34, 36, 37 →
  CURSOR_34, 36, 37), three TBitmaps: `DS:69ec` picture backup, `DS:69f0` undo (inferred),
  `DS:69f4` the current tool's option sheet (starts as `a950.asl`); Timer1 interval 125 ms,
  enabled; Application.OnActivate/OnDeactivate/OnMessage hooks. Shareware counter: dead code.
- `FormPaint 1008:6390` (see program.md), then `RestoreLeftStrip 1008:2d76`,
  `DrawShades 1008:29fd`, on first paint `DrawToolPointer(0) 1008:3a68`, DS:0fd4 = 0 and music
  (`1008:61ac`), then the option panels whose flags are set.
- `FormActivate 1008:9452` → `1028:0281`: copies the picture backup (DS:69ec) to BiPict1.

## Tool tabs

- TabSet1 (Delphi TTabSet, ported in `src/tabset.*`): TabHeight 32, StartMargin 0, EndMargin
  15 (default), Selected/Background black, Unselected white, dithered background. Tab i
  spans x = 9 + 41·i … +32. `TabSet1MeasureTab` = 32. `TabSet1DrawTab 1008:9226` draws icon
  `ICON_n` at (40·i + 13, 0), or (40·i + 20, 0) for i ≥ 7 — CRAYON1 tabs → ICON 1, 2, 3, 5,
  12, 7, 9, 10, 11; CRAYON2 → 1, 2, 3, 4, 5, 6, 12, 7, 8, 9, 11.
- `TabSet1Click 1008:91d2`: sound 93 (`1028:000f`), `SelectTool(TabIndex) 1008:8fc7`, DS:1ac0 = 0.
- `SelectTool 1008:8fc7`: Timer1 off; option sheet by tab: a950, a951, a952, a904, a914, a953,
  a908, a954, a910; frame DS:1002 = 0, direction DS:1000 = 0; Timer1 125 ms on; DS:1ab8 =
  tab + 1; `DrawToolPointer(12)` (strip only, no triangle).
- `Timer1Timer 1008:8e10`: BitBlt(form, 100, 10, 400, 40, sheet, 0, 41·frame); frame ping-pongs
  0→5→0 (DS:1002, direction DS:1000) — the option icons' idle animation.
- `DrawToolPointer(tab) 1008:3a68`: a922.asl at (100,50) 500×12; for tab < 11 a red (palette
  95) filled triangle (x+120,51)-(x+100,59)-(x+140,59), x = 40·tab; DS:0fd0 = tab.

## Colours

- DS:1ad4 colour family (init 9 = red), DS:1ad2 shade (init 5); DS:1672 current colour =
  palette[family·10 + shade]. DS:1676 = palette[69] (black, set in 1028:0491).
- `DrawShades 1008:29fd`: a920 restore (0,179,81,34); grey pen (palette 64); shades i = 0..4 as
  16×16 ellipses at (16i,180) and (16i,196) with palette[family·10 + i] / [+5]; marker =
  square + X in red (palette 95) or yellow (palette 83) when family = 9; then `1028:2076`
  (DS:1a16 = family·10 + max(shade,2); 1a1a = −1; 1a1e = −2 — darker shades for effects,
  inferred) and, if DS:104d, `1008:377d`.

## Not yet decoded

`FormMouseDown 1008:6ef9` (toolbar, palette, options), `BiPict1MouseDown/Move/Up`, option panel
painters 1008:2eb1…3969 (flags DS:1042, 1056, 1057, 1045, 1058, 1046, 1047, 1048, 104c, 104d),
text functions 1008:3c3f…47fb, files 1008:4b55…532c, music 1008:61ac, hint 1008:034f.
Point table DS:691c… (x,y pairs around (15–65, 299–330)), set in 1028:0491 — use unknown.
