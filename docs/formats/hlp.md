# CRAYONS.HLP (WinHelp 3.1)

Observed in the player's original `CRAYONS.HLP`; decoded by `tools/hlp.py` and the native
converter in `src/help.cpp`. The ReactOS WinHelp reader was a format cross-check:
<https://github.com/reactos/reactos/blob/master/base/applications/winhlp32/hlpfile.c>.

- Header magic `0x00035f3f`; internal B+ tree directory starts at the header's offset 4.
  Subfiles have a 9-byte wrapper, then their data. The directory and `|CONTEXT` trees have
  38-byte headers and linked leaf pages. `|CONTEXT` maps topic hashes to logical offsets.
- `|SYSTEM` flags are zero: the 135 topics' strings are uncompressed. `|TOPIC` has 4096-byte
  blocks with 12-byte block headers; the 4084-byte payloads are concatenated for records.
  Each record has a 21-byte header, a formatting stream, and a text stream. Types observed:
  `0x02` topic header, `0x20` display paragraph (465), `0x23` table (228).
- A topic's logical offset is `block * 0x8000 + sum(paragraph lengths since block start)`.
  The formatting streams contain font/line/paragraph commands, hash links, and references
  to internal `|bmN` pictures. Context hashes resolve to the nearest preceding topic.
- There are 161 `|bmN` pictures: 113 DIBs (2 uncompressed, 111 RLE), 48 RLE-packed WMFs.
  `src/help.cpp` converts DIBs to BMP files and plays WMFs into 32-bit DIBs using Windows GDI;
  the browser can display the resulting BMPs without another package.
- The runtime converter writes `help-html/index.html` and `bmN.bmp` under the selected data
  folder, then asks the OS to open the HTML in the default browser. The snapshot host does
  not launch the browser. It presents a topic index, topic links, text, and images. It does
  not emulate WinHelp's search, popup windows, macro actions, original font table, or image
  hotspots; tables are flattened into text/inline images. No conversion output is committed.
