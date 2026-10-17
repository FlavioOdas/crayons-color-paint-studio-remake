cmake_minimum_required(VERSION 3.20)
project(Crayons LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(crayons WIN32
    src/main.cpp
    src/app.cpp
    src/assets.cpp
    src/dfm.cpp
    src/gfx.cpp
    src/hints.cpp
    src/help.cpp
    src/launcher.cpp
    src/ne.cpp
    src/studio.cpp
    src/studio_paint.cpp
    src/studio_shadow.cpp
    src/studio_effects.cpp
    src/studio_hints.cpp
    src/dialog.cpp
    src/thumbs.cpp
    src/activity.cpp
    src/pcdialogs.cpp
    src/textdialogs.cpp
    src/tabset.cpp
    src/gamefiles.cpp
    src/installer.cpp
    src/iso9660.cpp
)
target_compile_definitions(crayons PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX
    _WIN32_WINNT=0x0A00 WINVER=0x0A00)  # Windows 10+
target_link_libraries(crayons PRIVATE winmm gdi32 user32 shell32 ole32 comdlg32 winspool)
if(MSVC)
    target_compile_options(crayons PRIVATE /W4)
    set_property(TARGET crayons PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_options(crayons PRIVATE -Wall -Wextra -Wno-unused-private-field)  # ported globals land before their users
endif()
if(MINGW)
    # One self-contained exe: no libc++/libunwind DLLs next to it.
    target_link_options(crayons PRIVATE -static -municode)
endif()
