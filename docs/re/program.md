# Crayons — program structure

How established: NE headers/segments (`tools/ne_dump.py`), Delphi form resources
(`tools/dfm.py`), published-method tables (`tools/delphi_methods.py`), disassembly
(`tools/ne_disasm.py`) and Ghidra decompilation (`work/ghidra/crayon1.c`).

## Three 16-bit Delphi 1 programs (observed)

All three are Win16 NE executables (linker 6.1, "Borland Delphi … Portions Copyright (c)
1983,95 Borland"), so none of them runs on 64-bit Windows. Internal name of the product
was **PaintCraft** by Rhode Island Softsystems (RISS) — left-over shareware strings.

- `CRAYONST.EXE` — forms `Tsplash` (640×480 picture, 3 s `TTimer`) and `Tagepick`
  (640×480 picture with two invisible 170×160 click areas at (79,279) and (385,280)).
  The handlers `WinExec` `Crayon1.exe` (3–6) or `Crayon2.exe` (7–12); on failure: "There was
  a problem starting the program!".
- `CRAYON1.EXE` (younger) and `CRAYON2.EXE` (older) are **one code base built twice**: same
  ten forms, same library segments; they differ in the app segments 1–4, the main tab set
  (9 tools vs 11) and `TActivity` (12 vs 7 components).

## Segments → Delphi units (CRAYON1, observed from strings/RTTI)

Ghidra selector = `0x1000 + 8*(segment-1)`; `sub_<seg>_<off>` labels come from `ne_disasm.py`.

| Seg | Selector | Unit(s) | Notes |
|---|---|---|---|
| 1 | 1000 | program `Crayons`, `Orderme`, `Midistuf` (Tjukebox), `Activits` (TActivity), `Rotext` (Trotator), `Editdlg` (Ttextinpt), `About` (Taboutme), `Fstyles` (TFillStyles) | dialogs |
| 2 | 1008 | `Mainunit` (Tmain) | 39 KB: the paint program itself |
| 3 | 1010 | font dialog (TFontForm) + VCL `Clipbrd` | |
| 4 | 1018 | VCL `Printers` | |
| 5 | 1020 | `Thmbunit` (Tthumbs) | thumbnail picker (pictures, open, delete) |
| 6 | 1028 | unnamed helper unit (no classes) | refers to `snd\snd*.asl`, `snd\hv*.asl`, `pa0x.asl`, `pb*.asl` |
| 7–15 | 1030–1070 | VCL: `MPlayer`, `Buttons`, `Tabs`, `ExtCtrls`/`Menus`, `StdCtrls`, `Graphics`, `Controls`, `Forms`, `Classes` | library |
| 16 | 1078 | `System`/`SysUtils` RTL | `sub_16_28a8` = stack check (`mov ax,N; call; sub sp,N` prologue) |
| 17 | 1080 | DGROUP | globals `DAT_1080_xxxx` |

App code is roughly segments 1, 2, 3 (part), 5 and 6 — about 250 functions.

## Main form `Tmain` (observed)

Borderless, maximised, black, `KeyPreview`. Components: `TabSet1` (owner-drawn tool tabs),
`MediaPlayer1` (hidden), `BiPict1` (white `TPanel` — the drawing area), `Panel1` with
`Label1..3` (yellow hint balloon), `Timer1`, `Timer2` (250 ms).
Published handlers and addresses (CRAYON1): `FormCreate 1008:5551`, `FormPaint 1008:6390`,
`FormMouseDown 1008:6ef9` (toolbar/option clicks), `BiPict1MouseDown 1008:8cfc`,
`BiPict1MouseMove 1008:8d84`, `BiPict1MouseUp 1008:84bd`, `Timer1Timer 1008:8e10`,
`FormKeyPress 1008:9951`, `MediaPlayer1Notify 1008:6319`, plus `AppMessage`,
`AppActivate`/`AppDeactivate`, `WndProc`. Full list: `work/methods_crayon1.tsv`.

- `FormCreate` builds a **logical palette** (`CreatePalette`, handle `DS:1468`) of 256 entries:
  211 colours from the table at `DS:167a` (4 bytes each; filled by `sub_6_0491` with inline
  `RGB()` constants) and 45 black. Layout of the 211 (observed, `work/palette211.json`):
  entries 0–159 = 16 colour families × 10 shades, light to dark (the juggler's balls and the
  shading palette); entries 160–209 = 50 distinct **near-white** colours (R,B ∈ 250..255,
  G = 255, or R = 255, G ∈ 251..254) — exactly the colours Magic Color pages use for their
  hidden regions; entry 210 = (43,199,183). sets dozens of tool-state globals
  (`DAT_1080_0fd6`…`0ffc`, `1040`…`1065`, `1aa6`…`1ad4`) and loads ~40 custom cursors.
- `FormPaint` selects/realises that palette and lays the interface out **relative to the
  client size**, blitting the `A*.ASL` bitmaps. On first paint it sets `TabSet1` bounds to
  (82, 60, ClientWidth−82, 32) and `BiPict1` (the drawing area) to (82, 92, ClientWidth−82,
  ClientHeight−94) — at 640×480 that is **558×386, the size of the Magic Color pages**, so
  640×480 is the design resolution. Black bands: `Rectangle(0,0,ClientWidth,150)` and
  `Rectangle(0,0,100,ClientHeight)` in the palette colour at `DS:1676`.
- Drawing tools call **GDI directly** (seg 6 and 3: `CreatePen`, `SetROP2`, `Polygon`,
  `ExtFloodFill`, `CreatePolygonRgn`/`SelectClipRgn`, `RoundRect`, `Arc`, `Get/SetPixel`,
  `BitBlt`/`StretchBlt`) on the drawing area's DC, so the same calls on an 8-bit DIB section
  with the same palette should reproduce the original pixels.

## Other forms (observed in the form resources)

`Torders` (order info, `a945.asl`), `Tjukebox` (sound/music switches, `snd\pc*.mid` via MCI
`open … alias midi` / `play midi from 0 notify`), `TActivity` (Magic Color list, `activity\z*.asl`),
`Trotator` (text rotation, scroll bar in degrees), `Ttextinpt` (text entry), `Taboutme`
(`a912.asl`), `TFontForm` (font list with "Hello" sample), `TFillStyles` (patterns),
`Tthumbs` (4 thumbnails + up/down + "killme" = delete). Dumped to `work/forms/<exe>/`.

## External dependencies (observed imports)

`MMSYSTEM.mciSendString` (MIDI), `MMSYSTEM.sndPlaySound` (WAV), `USER.WinHelp`
(`crayons.hlp`), `PRNDLL.HARDCOPY` / `PICTODISKSELECTED` (printing), `KERNEL.GetProfileString/Int`
(WIN.INI), fonts by name (`Creeper`, `Walters`, `Crayon`, `Mexacali`, `Treehouse`, `Arial`).
Saved pictures: `user\yp*.bmp`. A shareware usage counter ("You have N uses of this program
left", "PaintCraft cannot be set up on this system!") is still in `Mainunit` — whether it
is reachable in retail is TODO.
