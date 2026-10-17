# Crayons Color & Paint Studio — native remake

This is a Windows 11 replacement executable for the 1997 Crayons Color & Paint Studio CD.
It uses the original game's pictures, sounds, fonts, forms and help file at run time.

## Run

Download the [complete Windows 11 ZIP](https://github.com/FlavioOdas/crayons-color-paint-studio-remake/releases/download/v0.1.0-preview/Crayons-Windows-11.zip).
It contains `crayons.exe` and the original game data; no second download or CD is needed.

Extract the ZIP to a writable folder before running `crayons.exe` (not inside the ZIP or
`Program Files`). No emulator or separate runtime is needed. Saves and generated help are
written inside the selected `data` folder.

Keep the included `data` folder beside the exe when moving it. The repository owner states
they have redistribution rights for the included original game files.

### Windows security warning

This preview executable is not code-signed. Windows Defender SmartScreen may show
“Windows protected your PC” because it does not yet recognize the app. If you trust this
download from the official release, select **More info** then **Run anyway**. Do not turn
off Windows security globally. If Windows reports a specific malware detection or blocks
the app without a **Run anyway** option, stop and report the exact warning instead.
Code signing by a trusted publisher is the proper long-term improvement, though a new
signed app can still receive reputation warnings. See
[Microsoft's SmartScreen guidance](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation).

## Build from source

The project uses CMake and a C++20 compiler targeting 64-bit Windows. From WSL with the
repository toolchain installed:

```sh
source ~/tools/env.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw.cmake
cmake --build build -j16
tools/package.sh
# Complete release ZIP, built only from an authorized original CD copy:
tools/package_portable.sh original/CRAYONS.ISO
```

`tools/package.sh` creates `build/crayons-win11.zip` containing only the executable and this
README. `tools/package_portable.sh` creates `build/Crayons-Windows-11.zip` with only the
executable, this README and imported data from the supplied original CD. Both outputs are
ignored by Git. Only the complete ZIP is attached to the GitHub release; original game files
are never committed to the source repository.

No open-source license for the code or original game content is granted by this README.
