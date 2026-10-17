# Crayons Color & Paint Studio — native remake

This is a Windows 11 replacement executable for the 1997 Crayons Color & Paint Studio CD.
It uses the original game's pictures, sounds, fonts, forms and help file at run time.

## Run

Download the plug-and-play Windows package from the
[preview release](https://github.com/FlavioOdas/crayons-color-paint-studio-remake/releases/tag/v0.1.0-preview).

Extract the ZIP to a writable folder before running `crayons.exe` (not inside the ZIP or
`Program Files`). No emulator or separate runtime is needed. Saves and generated help are
written inside the selected `data` folder.

- The release `crayons-portable-win11.zip` includes data imported from the original CD in
  a `data` folder beside the exe. Keep that folder with the exe when moving
  it. It starts immediately, without a CD prompt. The repository owner states they have
  redistribution rights for the included original game files.
- The data-free `crayons-win11.zip` needs the original CD on first launch. Choose its
  `.iso`, `.bin`, `.cue`, or `CRAYONS.Z` file; the imported data then lives in
  `%APPDATA%\Crayons\data` and the CD is no longer needed.

## Build from source

The project uses CMake and a C++20 compiler targeting 64-bit Windows. From WSL with the
repository toolchain installed:

```sh
source ~/tools/env.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw.cmake
cmake --build build -j16
tools/package.sh
# Portable release, built only from an authorized original CD copy:
tools/package_portable.sh original/CRAYONS.ISO
```

`tools/package.sh` creates `build/crayons-win11.zip` containing only the executable and this
README. `tools/package_portable.sh` creates `build/crayons-portable-win11.zip` with only the
executable, this README and imported data from the supplied original CD. Both outputs are
ignored by Git. Only the portable ZIP is attached to the GitHub release; original game files
are never committed to the source repository.

No open-source license for the code or original game content is granted by this README.
