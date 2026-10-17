# Crayons Color & Paint Studio — CD and data formats

Established by reading the bytes of the user's CD image (`CRAYONS.ISO`, volume `CRAYONS`,
created 1997-01-28, product 8662, version 1.0). Tools: `tools/iso9660.py`, `tools/is3x.py`,
`tools/bmp2png.py`. "Observed" = seen in bytes; "inferred" = deduced from code/manual.

## CD layout (observed)

| Path | What |
|---|---|
| `SETUP.EXE`, `SETUP.INS`, `SETUP.PKG`, `_SETUP*.LIB`, `_INST*.EX_` | InstallShield 3 installer |
| `CRAYONS.Z` (8,170,648 B) | **Typical install**: InstallShield 3 archive with the whole program (683 files) |
| `ICON.Z` (3,868,670 B) | **Compact install**: same programs, fewer ART/SND files (those stay on the CD) |
| `ART/` (307), `SND/` (82 `HV*`) | Loose copies used by a compact install |
| `*.TTF` | Fonts the installer adds to Windows: `CRAYON__`, `CREEPER_`, `MEXACALI`, `TREEHOUS`, `WALTERS_` |
| `DOCS/XCRAYONS.PDF` | Online manual (PDF 1.1, LZW streams; `tools/pdftext.py` extracts the text) |
| `CATALOG/`, `ACRO_V2/`, `REGCARD/`, `RUNPDF.EXE` | Expert Software catalogue, Acrobat Reader 2, registration card — not part of the program |

`CRAYONS.Z` and `ICON.Z` use the same InstallShield 3 format as CyberPest's `DATA.Z`
(magic `13 5D 65 8C`, PKWARE DCL implode); `tools/is3x.py` extracts them. Everything the
program needs is in `CRAYONS.Z` (+ the five fonts from the CD root).

## Installed program (contents of CRAYONS.Z, observed)

| File | What |
|---|---|
| `CRAYONST.EXE` | Launcher: splash, then "I'm 3-6 yrs." / "I'm 7-12 yrs." picker; `WinExec`s one of the two below |
| `CRAYON1.EXE` | Younger Kids level (ages 3–6) |
| `CRAYON2.EXE` | Older Kids level (ages 7–12) |
| `PRNDLL.DLL` | Printing helper (`HARDCOPY`, `PICTODISKSELECTED` exports) |
| `CRAYONS.HLP` | WinHelp file |
| `CRAYONS.INI`, `USER/USER.DAT` | Empty placeholders |
| `*.ASL`, `ACTIVITY/`, `ART/`, `SND/` | Data, below |

## `.ASL` files are disguised standard files (observed)

No custom container: every `.ASL` starts with a normal file signature.

| Kind | Count | Signature |
|---|---|---|
| Windows BMP, 8-bit (plain or BI_RLE8), one 4-bit | 359 incl. ART | `BM` |
| WAVE, 8-bit PCM (11,025 or 22,050 Hz, mono or stereo) | 193 | `RIFF....WAVE` |
| Plain text, CRLF lines | 13 | — |

`ART/*.BMP` and `SND/*.MID` (Standard MIDI) are not disguised.

## File families

| Name | Size / format | Use (from manual, file names and code strings) |
|---|---|---|
| `A1`–`A96.ASL` | mostly 80×51 / 80×81, 8-bit | Toolbar and tool-option buttons, several state variants each |
| `A900`–`A954.ASL` | 400×250 sheets | Tool-option panels, 6 rows = states (observed visually) |
| `A911A/B/C.ASL` | 140×59 | "Crayons Color & Paint Studio — EXIT" sign states |
| `A912.ASL` | 279×336, 4-bit | About box picture |
| `A919.ASL` | 943×59 | Horizontal strip (inferred: main-toolbar background) |
| `A920.ASL` | 80×768 | Vertical strip with "?" (help) |
| `A922.ASL` | 500×12 | Horizontal border strip |
| `A930`–`A932.ASL` | 320×106 / 264×106 | Picture-picker strips (explosion, train, stick figure) |
| `A938`/`A939.ASL` | text | Tool hint captions (one per line, by tool id); A938 for one level, A939 for the other (inferred) |
| `A945.ASL` | text | Leftover shareware nag ("PaintCraft", RISS) — unused in retail (inferred) |
| `PA00`–`PA07.ASL` | 8×8 | Fill patterns ("Paint Patterns") |
| `PB1`–`PB8.ASL` | 54×54 | Textures (bricks, stone, wood, confetti, grass…) |
| `ART/CB0xyy.BMP` | 592×416, 1-bit | Coloring Book pages (three series 01/02/03) |
| `ART/FH*`, `ART/FV*` | 200×200, 8-bit | Full-colour pictures: picture + mask in one bitmap — `H` = two 200×100 halves stacked, `V` = two 100×200 halves side by side |
| `ART/MH*`, `ART/MV*` | 200×200, 8-bit | Black-and-white pictures, same picture+mask layout |
| `ART/LEVEL*.BMP` | 640×480, 8-bit | Background scenes |
| `ACTIVITY/A1400`–`A1412.ASL` | 558×386, 8-bit | **Magic Color** pages (below) |
| `ACTIVITY/Z1xx/Z2xx/Z3xx.ASL` | text | Activity lists: names / thumbnails / page files |
| `ACTIVITY/D0.BMP` | 90×90 | Thumbnail placeholder |
| `SND/SND*.ASL` | WAVE | Sound effects |
| `SND/HV*.ASL` | WAVE | Voice clips (inferred: spoken hints, `snd\hv` + number in code) |
| `SND/PC1`–`PC20.MID` | MIDI | Jukebox music (`snd\pc` + number, played through MCI `sequencer`) |
| `user\yp*.bmp` | BMP | The child's saved pictures ("My Pictures") |

### Magic Color pages (observed)

A Magic Color page looks blank: each region has its own palette index whose colour is
almost white (e.g. index 190 = 250,255,252; 174 = 253,255,254), outlines are index 0. The
"paint can" then reveals the real colour of the region the child clicks. How the real
colour is derived from the near-white entry is in the code (TODO: `TActivity` / fill tool).

### Activity lists (observed)

`Z100`/`Z101`/`Z102` hold titles for "Things to do!", "3D things to create!" and "MagiColor
Pictures"; `Z2xx` name their thumbnails, `Z3xx` their page files. Only the Magic Color set
ships (`A1400`–`A1412`, `D0.BMP`); `A11xx`, `A12xx`, `b*.bmp`, `c*.bmp` and the "Storybooks"
lists (`Z103` …) are referenced but absent — cut content.
