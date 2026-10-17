#include <windows.h>
#include <mmsystem.h>

#include <cstdio>
#include <string>

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::fwprintf(stderr, L"usage: test_music_transition.exe <game.exe> <test-data>\n");
        return 2;
    }
    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\" --data \"" + argv[2] + L"\" --level 1";
    STARTUPINFOW startup{sizeof startup};
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(argv[1], command.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr,
                        &startup, &process)) {
        std::fwprintf(stderr, L"could not launch game: %lu\n", GetLastError());
        return 2;
    }
    CloseHandle(process.hThread);
    HWND window = nullptr;
    for (int attempt = 0; attempt < 100; ++attempt) {
        HWND candidate = nullptr;
        while ((candidate = FindWindowExW(nullptr, candidate, L"CrayonsRemake", nullptr))) {
            DWORD owner = 0;
            GetWindowThreadProcessId(candidate, &owner);
            if (owner == process.dwProcessId) {
                window = candidate;
                break;
            }
        }
        if (window) break;
        Sleep(50);
    }
    int verdict = 0;
    if (!window) {
        std::fputs("game window did not open\n", stderr);
        verdict = 2;
    } else {
        Sleep(2000);
        DWORD_PTR reply = 0;
        if (!SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 1000, &reply)) {
            std::fputs("game was unresponsive before music transition\n", stderr);
            verdict = 2;
        } else {
            if (!PostMessageW(window, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, 0)) {
                std::fputs("could not post music notification\n", stderr);
                verdict = 2;
            } else {
                DWORD worstDelay = 0;
                for (int attempt = 0; attempt < 40; ++attempt) {
                    Sleep(50);
                    const DWORD start = GetTickCount();
                    if (!SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 1000, &reply)) {
                        std::fputs("game hung after music transition\n", stderr);
                        verdict = 1;
                        break;
                    }
                    const DWORD delay = GetTickCount() - start;
                    if (delay > worstDelay) worstDelay = delay;
                    if (delay > 250) verdict = 1;
                }
                std::printf("worst response during transition: %lu ms\n", worstDelay);
            }
        }
        PostMessageW(window, WM_CLOSE, 0, 0);
    }
    if (WaitForSingleObject(process.hProcess, 3000) == WAIT_TIMEOUT) {
        std::fputs("game did not exit cleanly\n", stderr);
        TerminateProcess(process.hProcess, 1);
        verdict = 1;
    }
    CloseHandle(process.hProcess);
    return verdict;
}
