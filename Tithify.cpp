#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <winhttp.h>
#include <gdiplus.h>
#include <ctime>
#include <string>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <shlobj.h>
#include <commdlg.h>
#include <cmath>
#include <mmsystem.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")

using namespace Gdiplus;

// ── App Version ──────────────────────────────────────────────────────────────
#define APP_VERSION L"3.6.4"
#define GITHUB_REPO_API L"/repos/aayushlbef/Tithify/releases/latest"
#define GITHUB_RELEASE_URL L"https://github.com/aayushlbef/Tithify/releases/tag/"
#define WM_UPDATE_AVAILABLE      (WM_USER + 2)
#define WM_UPDATE_NOT_FOUND      (WM_USER + 3)
#define WM_UPDATE_ERROR          (WM_USER + 4)
#define WM_HOLIDAYS_LOADED       (WM_USER + 5)
#define WM_HOLIDAYS_FAILED       (WM_USER + 6)
#define WM_UPDATE_DOWNLOADING    (WM_USER + 7)
#define WM_UPDATE_INSTALL_FAILED (WM_USER + 8)

// ── Global State ─────────────────────────────────────────────────────────────
ULONG_PTR g_gdiplusToken;
HWND g_hWnd = NULL;
extern HWND g_hCalWnd;
extern bool g_isCalendarOpen;
extern HWND g_hReminderWnd;
extern bool g_isReminderDashboardOpen;
extern HWND g_hAlarmAlertWnd;
extern bool g_isAlarmAlertOpen;
extern HWND g_hDatePopupWnd;
extern bool g_isDatePopupOpen;
void CloseCalendarDatePopup();
bool g_setupMode = true;
int g_xPos = 500, g_yPos = 1000;
POINT g_dragStart = { 0, 0 };
bool g_isDragging = false;
bool g_isMenuOpen = false;
bool g_showDay = true;

// DPI scale factor (1.0 = 96 DPI, 1.25 = 120 DPI, 1.5 = 144 DPI, etc.)
float g_dpiScale = 1.0f;

bool g_hiddenForFullscreen = false;
bool g_hiddenForTaskbar = false;
bool g_isOnTaskbar = true;
bool g_zIndexDesktop = false;
int g_currentShiftX = 0;
int g_currentShiftY = 0;

// ── Background Customization State ───────────────────────────────────────────
int g_bgPresetMode = 0;            // 0 = Transparent (Default), 1 = Custom Color, 2 = Dark Glass, 3 = Light Glass, 4 = Solid Black, 5 = Solid White, 6 = Red Accent, 7 = Blue Accent
COLORREF g_bgColorRGB = RGB(31, 31, 31);
int g_bgOpacity = 200;             // Opacity level (0..255)

// ── Custom Menu State ────────────────────────────────────────────────────────
HWND g_hMenuWnd = NULL;
int g_menuXPos = 0, g_menuYPos = 0;
int g_menuBaseXPos = 0;   // original x before flyout shifts it
int g_menuHoverIndex = -1;
bool g_bgFlyoutOpen  = false;   // is BG colour flyout visible?
int  g_bgFlyoutHover = -1;      // hovered swatch 0-7, or 8 = slider
bool g_bgSliderDrag  = false;   // opacity slider being dragged

// ── Theme Detection ──────────────────────────────────────────────────────────
bool g_isLightTheme = false;   // true = Windows light theme, false = dark
wchar_t g_latestVersion[64] = {0};  // Filled by update-check thread

bool DetectWindowsTheme() {
    // Returns true if Windows is using a light taskbar/system theme
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0, size = sizeof(DWORD), type = REG_DWORD;
        // SystemUsesLightTheme controls the taskbar/system chrome color
        if (RegQueryValueExW(hKey, L"SystemUsesLightTheme", NULL, &type,
                             (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return (value != 0);
        }
        RegCloseKey(hKey);
    }
    return false;  // Default to dark theme if registry read fails
}

// ── Update Checker ───────────────────────────────────────────────────────────
// Compare "3.3.0" vs "3.4.0" style version strings
bool IsVersionNewer(const wchar_t* current, const wchar_t* latest) {
    int cMaj = 0, cMin = 0, cPat = 0;
    int lMaj = 0, lMin = 0, lPat = 0;
    const wchar_t* c = current;
    const wchar_t* l = latest;
    if (*c == L'v' || *c == L'V') c++;
    if (*l == L'v' || *l == L'V') l++;
    swscanf(c, L"%d.%d.%d", &cMaj, &cMin, &cPat);
    swscanf(l, L"%d.%d.%d", &lMaj, &lMin, &lPat);
    if (lMaj != cMaj) return lMaj > cMaj;
    if (lMin != cMin) return lMin > cMin;
    return lPat > cPat;
}

// Minimal JSON extractor: finds "key":"value" and writes value into out[]
bool ExtractJsonString(const char* json, const char* key, wchar_t* out, int outLen) {
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char* p = strstr(json, needle);
    if (!p) return false;
    p += strlen(needle);
    while (*p == ' ' || *p == ':' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != '"') return false;
    p++;  // skip opening quote
    int i = 0;
    while (*p && *p != '"' && i < outLen - 1) {
        out[i++] = (wchar_t)(*p++);
    }
    out[i] = 0;
    return i > 0;
}

DWORD WINAPI CheckForUpdateThread(LPVOID lpParam) {
    bool isManual = (bool)(INT_PTR)lpParam;

    HINTERNET hSession = WinHttpOpen(L"Tithify/" APP_VERSION,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        if (isManual) PostMessage(g_hWnd, WM_UPDATE_ERROR, 0, 0);
        return 0;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        if (isManual) PostMessage(g_hWnd, WM_UPDATE_ERROR, 0, 0);
        return 0;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", GITHUB_REPO_API,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        if (isManual) PostMessage(g_hWnd, WM_UPDATE_ERROR, 0, 0);
        return 0;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        if (isManual) PostMessage(g_hWnd, WM_UPDATE_ERROR, 0, 0);
        return 0;
    }

    // Read the full response body
    std::string response;
    char buf[4096];
    DWORD bytesRead = 0;
    while (WinHttpReadData(hRequest, buf, sizeof(buf) - 1, &bytesRead) && bytesRead > 0) {
        buf[bytesRead] = 0;
        response += buf;
        bytesRead = 0;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Extract tag_name from the JSON response
    wchar_t tagName[64] = {0};
    if (ExtractJsonString(response.c_str(), "tag_name", tagName, 64)) {
        if (IsVersionNewer(APP_VERSION, tagName)) {
            wcscpy_s(g_latestVersion, tagName);
            PostMessage(g_hWnd, WM_UPDATE_AVAILABLE, 0, 0);
        } else {
            if (isManual) PostMessage(g_hWnd, WM_UPDATE_NOT_FOUND, 0, 0);
        }
    } else {
        if (isManual) PostMessage(g_hWnd, WM_UPDATE_ERROR, 0, 0);
    }
    return 0;
}

// ── Auto-Updater: Visible Console Window ─────────────────────────────────────
// Writes a PowerShell script to %TEMP% and opens it in a visible console
// window so the user can watch the download progress and installation steps.
static void LaunchUpdaterConsole(const wchar_t* version) {
    // ── Build the temp .ps1 path ──────────────────────────────────────────────
    wchar_t tempDir[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, tempDir);
    wchar_t ps1Path[MAX_PATH] = {0};
    swprintf(ps1Path, MAX_PATH, L"%sTithify_Updater.ps1", tempDir);

    // Convert version to narrow for snprintf
    char versionA[64] = {0};
    WideCharToMultiByte(CP_UTF8, 0, version, -1, versionA, sizeof(versionA), NULL, NULL);

    // ── Build the PowerShell script content ──────────────────────────────────
    char script[4096] = {0};
    snprintf(script, sizeof(script),
        "$ErrorActionPreference = 'Stop'\r\n"
        "$version = '%s'\r\n"
        "$url     = \"https://github.com/aayushlbef/Tithify/releases/download/$version/Tithify_Setup.exe\"\r\n"
        "$tmp     = \"$env:TEMP\\Tithify_Update_$version.exe\"\r\n"
        "$appExe  = \"$env:LOCALAPPDATA\\Tithify\\Tithify.exe\"\r\n"
        "\r\n"
        "Write-Host ''\r\n"
        "Write-Host '  ==================================================' -ForegroundColor Cyan\r\n"
        "Write-Host '   Tithify -- Auto Updater' -ForegroundColor Cyan\r\n"
        "Write-Host '  ==================================================' -ForegroundColor DarkGray\r\n"
        "Write-Host ''\r\n"
        "Write-Host \"  New version : $version\" -ForegroundColor White\r\n"
        "Write-Host \"  Source      : $url\" -ForegroundColor DarkGray\r\n"
        "Write-Host ''\r\n"
        "\r\n"
        "# -- Step 1: Download ------------------------------------------------\r\n"
        "Write-Host '[1/3] Downloading installer...' -ForegroundColor Cyan\r\n"
        "Write-Host ''\r\n"
        "try {\r\n"
        "    if (Get-Command curl.exe -ErrorAction SilentlyContinue) {\r\n"
        "        curl.exe -L --progress-bar -o $tmp $url\r\n"
        "        if ($LASTEXITCODE -ne 0) { throw \"curl exited with code $LASTEXITCODE\" }\r\n"
        "    } else {\r\n"
        "        $ProgressPreference = 'Continue'\r\n"
        "        Invoke-WebRequest -Uri $url -OutFile $tmp -UseBasicParsing\r\n"
        "    }\r\n"
        "} catch {\r\n"
        "    Write-Host ''\r\n"
        "    Write-Host \"[-] Download failed: $_\" -ForegroundColor Red\r\n"
        "    Write-Host ''\r\n"
        "    Read-Host 'Press Enter to close'\r\n"
        "    exit 1\r\n"
        "}\r\n"
        "\r\n"
        "if (-not (Test-Path $tmp) -or (Get-Item $tmp).Length -lt 100000) {\r\n"
        "    Write-Host '[-] Downloaded file appears corrupted. Please try again.' -ForegroundColor Red\r\n"
        "    Read-Host 'Press Enter to close'\r\n"
        "    exit 1\r\n"
        "}\r\n"
        "\r\n"
        "Write-Host ''\r\n"
        "Write-Host '[+] Download complete!' -ForegroundColor Green\r\n"
        "Write-Host ''\r\n"
        "\r\n"
        "# -- Step 2: Install -------------------------------------------------\r\n"
        "Write-Host '[2/3] Installing Tithify...' -ForegroundColor Cyan\r\n"
        "Write-Host '      Running installer silently, please wait...' -ForegroundColor DarkGray\r\n"
        "Write-Host ''\r\n"
        "Start-Process -FilePath $tmp -ArgumentList '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-' -Wait\r\n"
        "Remove-Item $tmp -Force -ErrorAction SilentlyContinue\r\n"
        "Write-Host '[+] Installation complete!' -ForegroundColor Green\r\n"
        "Write-Host ''\r\n"
        "\r\n"
        "# -- Step 3: Relaunch ------------------------------------------------\r\n"
        "Write-Host '[3/3] Launching Tithify...' -ForegroundColor Cyan\r\n"
        "Start-Sleep -Seconds 1\r\n"
        "if (Test-Path $appExe) {\r\n"
        "    Start-Process -FilePath $appExe\r\n"
        "    Write-Host '[+] Widget launched successfully.' -ForegroundColor Green\r\n"
        "} else {\r\n"
        "    Write-Host \"[!] Could not find widget at: $appExe\" -ForegroundColor Yellow\r\n"
        "}\r\n"
        "\r\n"
        "Write-Host ''\r\n"
        "Write-Host '  Update complete! This window will close in 5 seconds.' -ForegroundColor Cyan\r\n"
        "Write-Host '  ==================================================' -ForegroundColor DarkGray\r\n"
        "Start-Sleep -Seconds 5\r\n",
        versionA
    );

    // ── Write the script file to disk ────────────────────────────────────────
    HANDLE hFile = CreateFileW(ps1Path, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(hFile, script, (DWORD)strlen(script), &written, NULL);
    CloseHandle(hFile);

    // ── Launch in a visible PowerShell console window ─────────────────────────
    wchar_t args[MAX_PATH + 128] = {0};
    swprintf(args, MAX_PATH + 128,
        L"-NoProfile -ExecutionPolicy Bypass -File \"%ls\"",
        ps1Path);

    ShellExecuteW(NULL, L"open", L"powershell.exe", args, NULL, SW_NORMAL);
}


// ── Fullscreen App Detection ─────────────────────────────────────────────────
bool IsShellProcess(DWORD pid) {
    if (pid == 0) return true;

    // Check explorer process id
    DWORD explorerPid = 0;
    HWND hShell = GetShellWindow();
    if (hShell) {
        GetWindowThreadProcessId(hShell, &explorerPid);
    } else {
        HWND hTb = FindWindow(L"Shell_TrayWnd", NULL);
        if (hTb) GetWindowThreadProcessId(hTb, &explorerPid);
    }
    if (pid == explorerPid) return true;

    // Check process executable name
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        wchar_t exePath[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
            const wchar_t* fileName = wcsrchr(exePath, L'\\');
            if (fileName) fileName++;
            else fileName = exePath;

            if (_wcsicmp(fileName, L"explorer.exe") == 0 ||
                _wcsicmp(fileName, L"StartMenuExperienceHost.exe") == 0 ||
                _wcsicmp(fileName, L"SearchHost.exe") == 0 ||
                _wcsicmp(fileName, L"ShellExperienceHost.exe") == 0 ||
                _wcsicmp(fileName, L"TextInputHost.exe") == 0 ||
                _wcsicmp(fileName, L"ScreenClippingHost.exe") == 0 ||
                _wcsicmp(fileName, L"SnippingTool.exe") == 0 ||
                _wcsicmp(fileName, L"SnippingToolApp.exe") == 0 ||
                _wcsicmp(fileName, L"SnipAndSketch.exe") == 0 ||
                _wcsicmp(fileName, L"GameBar.exe") == 0 ||
                _wcsicmp(fileName, L"GameBarFTServer.exe") == 0 ||
                _wcsicmp(fileName, L"LockApp.exe") == 0 ||
                _wcsicmp(fileName, L"SystemSettings.exe") == 0 ||
                wcsstr(fileName, L"ScreenClip") != NULL ||
                wcsstr(fileName, L"Snipping") != NULL) {
                CloseHandle(hProcess);
                return true;
            }
        }
        CloseHandle(hProcess);
    }
    return false;
}

// Returns true if the foreground window is in true fullscreen mode
// (e.g. YouTube fullscreen, media players, games, F11 browser) on the same monitor as Tithify.
bool IsFullscreenAppRunning() {
    if (!g_hWnd || g_setupMode) return false;

    // 1. Primary Check: Windows notification state
    // When in standard desktop mode (working with apps, browsing web, Start menu, Show Desktop):
    // SHQueryUserNotificationState reports QUNS_ACCEPTS_NOTIFICATIONS (5) or QUNS_APP (7).
    // Windows only sets QUNS_BUSY (2), QUNS_RUNNING_D3D_FULL_SCREEN (3), or QUNS_PRESENTATION_MODE (4)
    // when a genuine full-screen application (like YouTube fullscreen, video players, games) is active.
    QUERY_USER_NOTIFICATION_STATE quns = (QUERY_USER_NOTIFICATION_STATE)0;
    if (SUCCEEDED(SHQueryUserNotificationState(&quns))) {
        if (quns != QUNS_BUSY &&
            quns != QUNS_RUNNING_D3D_FULL_SCREEN &&
            quns != QUNS_PRESENTATION_MODE) {
            return false;
        }
    }

    // 2. Verify the active foreground window
    HWND hForeground = GetForegroundWindow();
    if (!hForeground) return false;

    // Desktop and shell windows are never fullscreen apps
    if (hForeground == GetDesktopWindow() || hForeground == GetShellWindow())
        return false;

    // Ignore our own widget windows
    if (hForeground == g_hWnd || hForeground == g_hCalWnd || hForeground == g_hMenuWnd ||
        hForeground == g_hReminderWnd || hForeground == g_hAlarmAlertWnd || hForeground == g_hDatePopupWnd)
        return false;

    // Ignore transparent / click-through overlay windows (e.g. snipping tool overlay)
    LONG_PTR exStyle = GetWindowLongPtrW(hForeground, GWL_EXSTYLE);
    if (exStyle & WS_EX_TRANSPARENT) {
        return false;
    }

    // Ignore Windows Explorer / Desktop / Taskbar / Shell / Snipping window classes
    wchar_t cls[256] = {0};
    if (GetClassNameW(hForeground, cls, 256) > 0) {
        if (wcscmp(cls, L"WorkerW") == 0 ||
            wcscmp(cls, L"Progman") == 0 ||
            wcscmp(cls, L"Shell_TrayWnd") == 0 ||
            wcscmp(cls, L"Shell_SecondaryTrayWnd") == 0 ||
            wcscmp(cls, L"SHELLDLL_DefView") == 0 ||
            wcscmp(cls, L"SysListView32") == 0 ||
            wcscmp(cls, L"Windows.UI.Core.CoreWindow") == 0 ||
            wcscmp(cls, L"XamlExplorerHostIslandWindow") == 0 ||
            wcscmp(cls, L"Xaml_WindowedPopupClass") == 0 ||
            wcscmp(cls, L"TopLevelWindowForOverflowXamlIsland") == 0 ||
            wcscmp(cls, L"ScreenClippingHost") == 0 ||
            wcscmp(cls, L"SnippingTool") == 0 ||
            wcsstr(cls, L"ScreenClip") != NULL ||
            wcsstr(cls, L"Snipping") != NULL) {
            return false;
        }
    }

    // Check window title for snipping tools
    wchar_t title[256] = {0};
    if (GetWindowTextW(hForeground, title, 256) > 0) {
        if (wcsstr(title, L"Snipping") != NULL ||
            wcsstr(title, L"Screen Clipping") != NULL ||
            wcsstr(title, L"Snip & Sketch") != NULL) {
            return false;
        }
    }

    // Check process ID: never consider shell host or snipping tool processes as fullscreen apps
    DWORD fgPid = 0;
    GetWindowThreadProcessId(hForeground, &fgPid);
    if (IsShellProcess(fgPid)) {
        return false;
    }

    // 3. Multi-Monitor Awareness: only hide if fullscreen app is on the same monitor as Tithify
    HMONITOR hMonApp = MonitorFromWindow(hForeground, MONITOR_DEFAULTTONEAREST);
    HMONITOR hMonWidget = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
    if (!hMonApp || !hMonWidget || hMonApp != hMonWidget) {
        return false;
    }

    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(hMonApp, &mi)) return false;

    RECT rcWnd;
    if (!GetWindowRect(hForeground, &rcWnd)) return false;

    // 4. Geometry Check: ensure window covers the entire monitor
    bool coversMonitor = (rcWnd.left   <= mi.rcMonitor.left  &&
                          rcWnd.top    <= mi.rcMonitor.top   &&
                          rcWnd.right  >= mi.rcMonitor.right &&
                          rcWnd.bottom >= mi.rcMonitor.bottom);
    if (!coversMonitor) return false;

    return true;
}

// ── Desktop Host Window Resolver ──────────────────────────────────────────────
HWND GetDesktopHostWindow() {
    HWND hProgman = FindWindowW(L"Progman", NULL);
    if (hProgman && FindWindowExW(hProgman, NULL, L"SHELLDLL_DefView", NULL)) {
        return hProgman;
    }
    HWND hWorker = NULL;
    while ((hWorker = FindWindowExW(NULL, hWorker, L"WorkerW", NULL)) != NULL) {
        if (FindWindowExW(hWorker, NULL, L"SHELLDLL_DefView", NULL)) {
            return hWorker;
        }
    }
    return hProgman ? hProgman : GetShellWindow();
}

// ── Real-Time Shell / Foreground Event Hook ──────────────────────────────────
HWINEVENTHOOK g_hEventHook = NULL;

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
                           LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (!g_hWnd || !IsWindow(g_hWnd) || g_hiddenForFullscreen || g_hiddenForTaskbar) return;

    // When foreground window or focus changes across the system (Start, Search, Taskbar, Show Desktop),
    // immediately ensure Tithify is not iconic and sits in the proper z-order.
    if (IsIconic(g_hWnd)) {
        ShowWindow(g_hWnd, SW_RESTORE);
    }
    if (!g_isMenuOpen && !g_isCalendarOpen) {
        bool keepOnTop = (g_setupMode || g_isOnTaskbar || g_zIndexDesktop);
        if (keepOnTop) {
            if (g_isOnTaskbar) {
                HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
                if (hTaskbar && (HWND)GetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT) != hTaskbar) {
                    SetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT, (LONG_PTR)hTaskbar);
                }
            } else {
                if ((HWND)GetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT) != NULL) {
                    SetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT, 0);
                }
            }
            SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        } else {
            HWND hDesktop = GetDesktopHostWindow();
            if (hDesktop && (HWND)GetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT) != hDesktop) {
                SetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT, (LONG_PTR)hDesktop);
            }
            SetWindowPos(g_hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
    }
}

// ── Auto-Hide Taskbar Sync & Animation State ─────────────────────────────────
struct TaskbarSyncState {
    bool isAutoHide = false;
    bool isCompletelyHidden = false;
    int shiftX = 0;
    int shiftY = 0;
};

TaskbarSyncState GetTaskbarSyncState() {
    TaskbarSyncState state;
    APPBARDATA abd = { sizeof(APPBARDATA) };
    UINT abState = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    state.isAutoHide = (abState & ABS_AUTOHIDE) != 0;
    if (!state.isAutoHide) return state;

    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    if (!hTaskbar) return state;

    RECT rcTaskbar;
    if (!GetWindowRect(hTaskbar, &rcTaskbar)) return state;

    HMONITOR hMon = MonitorFromWindow(hTaskbar, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    static int s_expandedHeight = 48;
    static int s_expandedWidth = 48;

    int tbHeight = rcTaskbar.bottom - rcTaskbar.top;
    int tbWidth = rcTaskbar.right - rcTaskbar.left;
    if (tbHeight > 10) s_expandedHeight = tbHeight;
    if (tbWidth > 10) s_expandedWidth = tbWidth;

    // Bottom Taskbar:
    if (rcTaskbar.bottom >= mi.rcMonitor.bottom - 10) {
        int normalTop = mi.rcMonitor.bottom - s_expandedHeight;
        state.shiftY = rcTaskbar.top - normalTop;
        if (state.shiftY < 0) state.shiftY = 0;
        if (tbHeight <= 4 || rcTaskbar.top >= mi.rcMonitor.bottom - 4) {
            state.isCompletelyHidden = true;
        }
    }
    // Top Taskbar:
    else if (rcTaskbar.top <= mi.rcMonitor.top + 10) {
        int normalBottom = mi.rcMonitor.top + s_expandedHeight;
        state.shiftY = rcTaskbar.bottom - normalBottom;
        if (state.shiftY > 0) state.shiftY = 0;
        if (tbHeight <= 4 || rcTaskbar.bottom <= mi.rcMonitor.top + 4) {
            state.isCompletelyHidden = true;
        }
    }
    // Right Taskbar:
    else if (rcTaskbar.right >= mi.rcMonitor.right - 10) {
        int normalLeft = mi.rcMonitor.right - s_expandedWidth;
        state.shiftX = rcTaskbar.left - normalLeft;
        if (state.shiftX < 0) state.shiftX = 0;
        if (tbWidth <= 4 || rcTaskbar.left >= mi.rcMonitor.right - 4) {
            state.isCompletelyHidden = true;
        }
    }
    // Left Taskbar:
    else if (rcTaskbar.left <= mi.rcMonitor.left + 10) {
        int normalRight = mi.rcMonitor.left + s_expandedWidth;
        state.shiftX = rcTaskbar.right - normalRight;
        if (state.shiftX > 0) state.shiftX = 0;
        if (tbWidth <= 4 || rcTaskbar.right <= mi.rcMonitor.left + 4) {
            state.isCompletelyHidden = true;
        }
    }

    return state;
}

// ── Taskbar vs Desktop Location Auto-Detection ───────────────────────────────
bool IsPositionOnTaskbar(int x, int y, int w, int h) {
    RECT rcWidget = { x, y, x + w, y + h };
    POINT center = { x + w / 2, y + h / 2 };

    HMONITOR hMon = MonitorFromRect(&rcWidget, MONITOR_DEFAULTTONEAREST);
    if (!hMon) return false;

    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(hMon, &mi)) return false;

    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    RECT rcTaskbar = { 0 };
    bool taskbarFound = false;
    if (hTaskbar) {
        taskbarFound = (GetWindowRect(hTaskbar, &rcTaskbar) != 0);
    }

    int tbHeight = rcTaskbar.bottom - rcTaskbar.top;
    int tbWidth = rcTaskbar.right - rcTaskbar.left;

    RECT rcTaskbarArea = { 0 };

    // 1. Standard taskbar: Monitor work area differs from monitor rect
    if (mi.rcWork.bottom < mi.rcMonitor.bottom) {
        rcTaskbarArea = { mi.rcMonitor.left, mi.rcWork.bottom, mi.rcMonitor.right, mi.rcMonitor.bottom };
    } else if (mi.rcWork.top > mi.rcMonitor.top) {
        rcTaskbarArea = { mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcWork.top };
    } else if (mi.rcWork.left > mi.rcMonitor.left) {
        rcTaskbarArea = { mi.rcMonitor.left, mi.rcMonitor.top, mi.rcWork.left, mi.rcMonitor.bottom };
    } else if (mi.rcWork.right < mi.rcMonitor.right) {
        rcTaskbarArea = { mi.rcWork.right, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom };
    } else if (taskbarFound) {
        // 2. Auto-hide taskbar (work area equals monitor rect)
        if (tbHeight > 10 && tbWidth > 10) {
            rcTaskbarArea = rcTaskbar;
        } else {
            // Taskbar is collapsed auto-hide bar: synthesize expanded rect at the monitor edge
            int defH = (int)(48 * g_dpiScale);
            if (rcTaskbar.bottom >= mi.rcMonitor.bottom - 10) {
                rcTaskbarArea = { mi.rcMonitor.left, mi.rcMonitor.bottom - defH, mi.rcMonitor.right, mi.rcMonitor.bottom };
            } else if (rcTaskbar.top <= mi.rcMonitor.top + 10) {
                rcTaskbarArea = { mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.top + defH };
            } else if (rcTaskbar.right >= mi.rcMonitor.right - 10) {
                rcTaskbarArea = { mi.rcMonitor.right - defH, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom };
            } else if (rcTaskbar.left <= mi.rcMonitor.left + 10) {
                rcTaskbarArea = { mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.left + defH, mi.rcMonitor.bottom };
            }
        }
    } else {
        // 3. Fallback: bottom 48px of primary monitor
        int defH = (int)(48 * g_dpiScale);
        rcTaskbarArea = { mi.rcMonitor.left, mi.rcMonitor.bottom - defH, mi.rcMonitor.right, mi.rcMonitor.bottom };
    }

    // Test if widget center point or >30% overlap lies in taskbar area
    if (PtInRect(&rcTaskbarArea, center)) {
        return true;
    }

    RECT rcIntersect;
    if (IntersectRect(&rcIntersect, &rcWidget, &rcTaskbarArea)) {
        int intersectArea = (rcIntersect.right - rcIntersect.left) * (rcIntersect.bottom - rcIntersect.top);
        int widgetArea = w * h;
        if (intersectArea > widgetArea / 3) {
            return true;
        }
    }

    return false;
}

void UpdateWidgetMode() {
    if (!g_hWnd) return;
    int winW = (int)((g_showDay ? 190 : 155) * g_dpiScale);
    int winH = (int)(48 * g_dpiScale);

    g_isOnTaskbar = IsPositionOnTaskbar(g_xPos, g_yPos, winW, winH);

    // In Win32, an owned popup window is always placed above its owner in Z-order.
    // - When on taskbar: owner is Shell_TrayWnd so Tithify stays naturally above the taskbar.
    // - When on desktop: owner is Progman / WorkerW (desktop host) so Tithify stays directly
    //   above the desktop wallpaper and never disappears when "Show Desktop" (Win+D) is invoked.
    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    HWND hDesktop = GetDesktopHostWindow();

    bool keepOnTop = (g_setupMode || g_isOnTaskbar || g_zIndexDesktop);

    if (g_isOnTaskbar) {
        if (hTaskbar) SetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT, (LONG_PTR)hTaskbar);
        else SetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT, 0);
    } else {
        if (!keepOnTop && hDesktop) {
            SetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT, (LONG_PTR)hDesktop);
        } else {
            SetWindowLongPtr(g_hWnd, GWLP_HWNDPARENT, 0);
        }
    }

    LONG_PTR exStyle = GetWindowLongPtr(g_hWnd, GWL_EXSTYLE);
    if (keepOnTop) {
        // Setup mode (unlocked for dragging), taskbar mode, or desktop with z-index enabled: keep TOPMOST
        if (!(exStyle & WS_EX_TOPMOST)) {
            SetWindowLongPtr(g_hWnd, GWL_EXSTYLE, exStyle | WS_EX_TOPMOST);
        }
        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
    } else {
        // Locked on desktop without z-index: behave like a desktop icon (bottom of window stack)
        if (exStyle & WS_EX_TOPMOST) {
            SetWindowLongPtr(g_hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TOPMOST);
        }
        SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        SetWindowPos(g_hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
    }
}



std::wstring GetConfigPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    size_t pos = ws.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        ws = ws.substr(0, pos);
    }
    return ws + L"\\tithify.cfg";
}

void RenderWidget(HWND hWnd);

void LoadConfig() {
    std::wstring path = GetConfigPath();
    std::string npath(path.begin(), path.end());
    FILE* file = fopen(npath.c_str(), "r");
    if (!file) {
        // Fallback for legacy widget.cfg
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring ws(exePath);
        size_t pos = ws.find_last_of(L"\\/");
        if (pos != std::wstring::npos) ws = ws.substr(0, pos);
        std::wstring legacyPath = ws + L"\\widget.cfg";
        std::string nLegacy(legacyPath.begin(), legacyPath.end());
        file = fopen(nLegacy.c_str(), "r");
    }
    if (file) {
        int x, y, setup, showDay = 1, bgPreset = 0, opacity = 200, zIndex = 0;
        unsigned int colorVal = (unsigned int)RGB(31, 31, 31);
        int n = fscanf(file, "%d,%d,%d,%d,%u,%d,%d,%d", &x, &y, &setup, &showDay, &colorVal, &opacity, &bgPreset, &zIndex);
        if (n >= 3) {
            g_xPos = x;
            g_yPos = y;
            g_setupMode = (setup != 0);
            if (n >= 4) g_showDay = (showDay != 0);
            if (n >= 5) g_bgColorRGB = (COLORREF)colorVal;
            if (n >= 6) g_bgOpacity = opacity;
            if (n >= 7) g_bgPresetMode = bgPreset;
            if (n >= 8) g_zIndexDesktop = (zIndex != 0);
        }
        fclose(file);
    }
}

void SaveConfig() {
    std::wstring path = GetConfigPath();
    std::string npath(path.begin(), path.end());
    FILE* file = fopen(npath.c_str(), "w");
    if (file) {
        fprintf(file, "%d,%d,%d,%d,%u,%d,%d,%d",
                g_xPos, g_yPos, g_setupMode ? 1 : 0, g_showDay ? 1 : 0,
                (unsigned int)g_bgColorRGB, g_bgOpacity, g_bgPresetMode,
                g_zIndexDesktop ? 1 : 0);
        fclose(file);
    }
}

bool PickCustomBackgroundColor(HWND hWndOwner) {
    static COLORREF custColors[16] = {
        RGB(255,255,255), RGB(0,0,0), RGB(240,240,240), RGB(32,32,32),
        RGB(230,50,50), RGB(50,180,50), RGB(50,120,240), RGB(240,180,50),
        RGB(150,50,200), RGB(50,200,200), RGB(255,128,0), RGB(128,128,128),
        RGB(64,64,64), RGB(192,192,192), RGB(255,192,203), RGB(128,0,0)
    };

    CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };
    cc.hwndOwner = hWndOwner;
    cc.lpCustColors = custColors;
    cc.rgbResult = g_bgColorRGB;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorW(&cc)) {
        g_bgColorRGB = cc.rgbResult;
        g_bgPresetMode = 1; // Custom color mode
        SaveConfig();
        if (g_hWnd) RenderWidget(g_hWnd);
        return true;
    }
    return false;
}

// ── Startup Registry Helpers ────────────────────────────────────────────────
static const wchar_t* STARTUP_REG_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* STARTUP_REG_VALUE = L"Tithify";

bool IsStartupEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, STARTUP_REG_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;
    DWORD type = 0, size = 0;
    bool exists = (RegQueryValueExW(hKey, STARTUP_REG_VALUE, NULL, &type, NULL, &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return exists;
}

void SetStartupEnabled(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, STARTUP_REG_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return;
    if (enable) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring val = std::wstring(L"\"") + exePath + L"\"";
        RegSetValueExW(hKey, STARTUP_REG_VALUE, 0, REG_SZ,
                       (const BYTE*)val.c_str(), (DWORD)((val.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hKey, STARTUP_REG_VALUE);
    }
    RegCloseKey(hKey);
}

// ── Nepali Date Calculator (Pure C++) ────────────────────────────────────────
#include "bs_data.h"

const wchar_t* kNepaliMonthNamesEN[] = {
    L"Baisakh", L"Jestha", L"Ashadh", L"Shrawan", L"Bhadra", L"Ashwin",
    L"Kartik", L"Mangsir", L"Poush", L"Magh", L"Falgun", L"Chaitra"
};

const wchar_t* kNepaliMonthNamesNP[] = {
    L"\u092C\u0948\u0936\u093E\u0916",             // Baisakh (बैशाख)
    L"\u091C\u0947\u0920",                         // Jestha (जेठ)
    L"\u0905\u0938\u093E\u0930",                   // Ashadh (असार)
    L"\u0936\u094D\u0930\u093E\u0935\u0923",       // Shrawan (श्रावण)
    L"\u092D\u0926\u094C",                         // Bhadra (भदौ)
    L"\u0905\u0938\u094B\u091C",                   // Ashwin (असोज)
    L"\u0915\u093E\u0930\u094D\u0924\u093F\u0915", // Kartik (कार्तिक)
    L"\u092E\u0902\u0938\u093F\u0930",             // Mangsir (मंसिर)
    L"\u092A\u0941\u0937",                         // Poush (पुष)
    L"\u092E\u093E\u0918",                         // Magh (माघ)
    L"\u092B\u093E\u0932\u094D\u0917\u0941\u0928", // Falgun (फाल्गुन)
    L"\u091A\u0948\u0924"                          // Chaitra (चैत)
};

const wchar_t* kEnglishMonthNames[] = {
    L"January", L"February", L"March", L"April", L"May", L"June",
    L"July", L"August", L"September", L"October", L"November", L"December"
};

const wchar_t* kNepaliDayNamesEN[] = {
    L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday"
};

const wchar_t* kNepaliDayNamesNP[] = {
    L"\u0906\u0907\u0924\u092C\u093E\u0930",  // Sunday (आइतबार)
    L"\u0938\u094B\u092E\u092C\u093E\u0930",  // Monday (सोमबार)
    L"\u092E\u0902\u0917\u0932\u092C\u093E\u0930", // Tuesday (मंगलबार)
    L"\u092C\u0941\u0927\u092C\u093E\u0930",  // Wednesday (बुधबार)
    L"\u092C\u093F\u0939\u0940\u092C\u093E\u0930", // Thursday (बिहीबार)
    L"\u0936\u0941\u0915\u094D\u0930\u092C\u093E\u0930", // Friday (शुक्रबार)
    L"\u0936\u0928\u093F\u092C\u093E\u0930"   // Saturday (शनिबार)
};

// ── Forward declaration for Calendar window handle ───────────────────────────
extern HWND g_hCalWnd;

// ── Nepali Public Holidays & Notable Festivals Engine ────────────────────────
struct NepaliHoliday {
    int month;          // 1 = Baisakh, ..., 12 = Chaitra
    int day;            // Day of BS month (1..32)
    bool isPublicHoliday;
    std::wstring titleNP;
    std::wstring titleEN;
    std::wstring category;
    std::wstring description;
};

enum HolidayFetchState {
    FETCH_IDLE,
    FETCH_SUCCESS,
    FETCH_DOWNLOADING,
    FETCH_ERROR_OFFLINE
};

#define HOLIDAY_FILE_MAGIC 0x4857444E
#define HOLIDAY_FILE_VERSION 1
#define ENCODE_XOR_KEY 0x5A

int g_loadedHolidayYear = 0;
std::vector<NepaliHoliday> g_currentYearHolidays;
HolidayFetchState g_holidayFetchState = FETCH_IDLE;
bool g_isHolidayFetchInProgress = false;
DWORD g_lastHolidayFetchAttemptTime = 0;

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], len, NULL, NULL);
    return str;
}

std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], len);
    return wstr;
}

std::wstring GetHolidayCachePath(int bsYear) {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dir = std::wstring(appDataPath) + L"\\Tithify";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\holidays_" + std::to_wstring(bsYear) + L".dat";
    }
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    size_t pos = ws.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        ws = ws.substr(0, pos);
    }
    return ws + L"\\holidays_" + std::to_wstring(bsYear) + L".dat";
}

bool SaveEncodedHolidayFile(int bsYear, const std::vector<NepaliHoliday>& holidays) {
    std::wstring filePath = GetHolidayCachePath(bsYear);
    FILE* f = _wfopen(filePath.c_str(), L"wb");
    if (!f) return false;

    uint32_t magic = HOLIDAY_FILE_MAGIC;
    uint16_t version = HOLIDAY_FILE_VERSION;
    uint16_t year = (uint16_t)bsYear;
    uint16_t count = (uint16_t)holidays.size();

    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&year, sizeof(year), 1, f);
    fwrite(&count, sizeof(count), 1, f);

    for (const auto& h : holidays) {
        uint8_t m = (uint8_t)h.month;
        uint8_t d = (uint8_t)h.day;
        uint8_t pub = h.isPublicHoliday ? 1 : 0;
        fwrite(&m, sizeof(m), 1, f);
        fwrite(&d, sizeof(d), 1, f);
        fwrite(&pub, sizeof(pub), 1, f);

        auto writeEncStr = [&](const std::wstring& ws) {
            std::string utf8 = WideToUtf8(ws);
            uint16_t len = (uint16_t)utf8.size();
            fwrite(&len, sizeof(len), 1, f);
            for (size_t i = 0; i < utf8.size(); ++i) {
                uint8_t b = (uint8_t)utf8[i] ^ ENCODE_XOR_KEY;
                fwrite(&b, 1, 1, f);
            }
        };

        writeEncStr(h.titleNP);
        writeEncStr(h.titleEN);
        writeEncStr(h.category);
        writeEncStr(h.description);
    }
    fclose(f);
    return true;
}

bool LoadEncodedHolidayFile(int bsYear, std::vector<NepaliHoliday>& outHolidays) {
    std::wstring filePath = GetHolidayCachePath(bsYear);
    FILE* f = _wfopen(filePath.c_str(), L"rb");
    if (!f) return false;

    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t year = 0;
    uint16_t count = 0;

    if (fread(&magic, sizeof(magic), 1, f) != 1 || magic != HOLIDAY_FILE_MAGIC) { fclose(f); return false; }
    if (fread(&version, sizeof(version), 1, f) != 1 || version != HOLIDAY_FILE_VERSION) { fclose(f); return false; }
    if (fread(&year, sizeof(year), 1, f) != 1 || year != bsYear) { fclose(f); return false; }
    if (fread(&count, sizeof(count), 1, f) != 1) { fclose(f); return false; }

    outHolidays.clear();
    for (uint16_t i = 0; i < count; ++i) {
        NepaliHoliday h;
        uint8_t m = 0, d = 0, pub = 0;
        if (fread(&m, sizeof(m), 1, f) != 1) break;
        if (fread(&d, sizeof(d), 1, f) != 1) break;
        if (fread(&pub, sizeof(pub), 1, f) != 1) break;
        h.month = m;
        h.day = d;
        h.isPublicHoliday = (pub != 0);

        auto readEncStr = [&]() -> std::wstring {
            uint16_t len = 0;
            if (fread(&len, sizeof(len), 1, f) != 1) return L"";
            std::string utf8(len, '\0');
            for (uint16_t j = 0; j < len; ++j) {
                uint8_t b = 0;
                if (fread(&b, 1, 1, f) != 1) break;
                utf8[j] = (char)(b ^ ENCODE_XOR_KEY);
            }
            return Utf8ToWide(utf8);
        };

        h.titleNP = readEncStr();
        h.titleEN = readEncStr();
        h.category = readEncStr();
        h.description = readEncStr();
        outHolidays.push_back(h);
    }
    fclose(f);
    return !outHolidays.empty();
}

bool ParseHolidaysJson(const std::string& json, int expectedYear, std::vector<NepaliHoliday>& outHolidays) {
    outHolidays.clear();
    const char* p = json.c_str();
    const char* holArray = strstr(p, "\"holidays\"");
    if (!holArray) return false;

    const char* openBracket = strchr(holArray, '[');
    if (!openBracket) return false;

    p = openBracket + 1;
    while (*p && *p != ']') {
        const char* objStart = strchr(p, '{');
        if (!objStart) break;
        const char* objEnd = strchr(objStart, '}');
        if (!objEnd) break;

        std::string obj(objStart, objEnd - objStart + 1);
        NepaliHoliday h;
        h.month = 0;
        h.day = 0;
        h.isPublicHoliday = false;

        const char* mStr = strstr(obj.c_str(), "\"month\"");
        if (mStr) {
            mStr += 7;
            while (*mStr == ' ' || *mStr == ':') mStr++;
            h.month = atoi(mStr);
        }

        const char* dStr = strstr(obj.c_str(), "\"day\"");
        if (dStr) {
            dStr += 5;
            while (*dStr == ' ' || *dStr == ':') dStr++;
            h.day = atoi(dStr);
        }

        const char* pubStr = strstr(obj.c_str(), "\"isPublicHoliday\"");
        if (pubStr) {
            pubStr += 17;
            while (*pubStr == ' ' || *pubStr == ':') pubStr++;
            if (strncmp(pubStr, "true", 4) == 0) h.isPublicHoliday = true;
        }

        auto extractStr = [](const std::string& src, const char* key) -> std::wstring {
            std::string searchKey = std::string("\"") + key + "\"";
            const char* kPos = strstr(src.c_str(), searchKey.c_str());
            if (!kPos) return L"";
            kPos += searchKey.size();
            while (*kPos == ' ' || *kPos == ':') kPos++;
            if (*kPos != '"') return L"";
            kPos++;

            std::string val = "";
            while (*kPos && *kPos != '"') {
                if (*kPos == '\\' && *(kPos + 1)) {
                    kPos++;
                    if (*kPos == 'u' && isxdigit(*(kPos+1)) && isxdigit(*(kPos+2)) && isxdigit(*(kPos+3)) && isxdigit(*(kPos+4))) {
                        char hex[5] = { *(kPos+1), *(kPos+2), *(kPos+3), *(kPos+4), 0 };
                        wchar_t wc = (wchar_t)strtol(hex, NULL, 16);
                        wchar_t tempW[2] = { wc, 0 };
                        char tempUtf8[8] = { 0 };
                        WideCharToMultiByte(CP_UTF8, 0, tempW, 1, tempUtf8, sizeof(tempUtf8), NULL, NULL);
                        val += tempUtf8;
                        kPos += 4;
                    } else if (*kPos == 'n') {
                        val += '\n';
                    } else if (*kPos == 't') {
                        val += '\t';
                    } else if (*kPos == '"') {
                        val += '"';
                    } else if (*kPos == '\\') {
                        val += '\\';
                    } else {
                        val += *kPos;
                    }
                } else {
                    val += *kPos;
                }
                kPos++;
            }
            return Utf8ToWide(val);
        };

        h.titleNP = extractStr(obj, "titleNP");
        h.titleEN = extractStr(obj, "titleEN");
        h.category = extractStr(obj, "category");
        h.description = extractStr(obj, "description");

        if (h.month >= 1 && h.month <= 12 && h.day >= 1 && h.day <= 32) {
            outHolidays.push_back(h);
        }

        p = objEnd + 1;
    }

    return !outHolidays.empty();
}

bool HttpDownloadString(const wchar_t* host, const wchar_t* path, std::string& outBody) {
    HINTERNET hSession = WinHttpOpen(L"Tithify/" APP_VERSION,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    // Fast timeouts so offline / DNS failures return quickly and never freeze threads
    WinHttpSetTimeouts(hSession, 2000, 2500, 3000, 3000);

    HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bool success = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL)) {
        
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

        if (statusCode == 200) {
            std::string response;
            char buf[4096];
            DWORD bytesRead = 0;
            while (WinHttpReadData(hRequest, buf, sizeof(buf) - 1, &bytesRead) && bytesRead > 0) {
                buf[bytesRead] = 0;
                response.append(buf, bytesRead);
                bytesRead = 0;
            }
            if (!response.empty()) {
                outBody = response;
                success = true;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

DWORD WINAPI FetchHolidaysThread(LPVOID lpParam) {
    int bsYear = (int)(INT_PTR)lpParam;
    g_isHolidayFetchInProgress = true;
    g_holidayFetchState = FETCH_DOWNLOADING;
    g_lastHolidayFetchAttemptTime = GetTickCount();

    std::string jsonBody;
    bool downloaded = false;

    // 1. Try primary GitHub raw URL
    std::wstring path1 = L"/aayushlbef/tithify-public/main/data/holidays_" + std::to_wstring(bsYear) + L".json";
    if (HttpDownloadString(L"raw.githubusercontent.com", path1.c_str(), jsonBody)) {
        downloaded = true;
    }

    // 2. Try secondary jsDelivr CDN
    if (!downloaded) {
        std::wstring path2 = L"/gh/aayushlbef/tithify-public@main/data/holidays_" + std::to_wstring(bsYear) + L".json";
        if (HttpDownloadString(L"cdn.jsdelivr.net", path2.c_str(), jsonBody)) {
            downloaded = true;
        }
    }

    if (downloaded) {
        std::vector<NepaliHoliday> parsed;
        if (ParseHolidaysJson(jsonBody, bsYear, parsed)) {
            SaveEncodedHolidayFile(bsYear, parsed);
            g_currentYearHolidays = parsed;
            g_loadedHolidayYear = bsYear;
            g_holidayFetchState = FETCH_SUCCESS;
            g_isHolidayFetchInProgress = false;
            if (g_hWnd) PostMessage(g_hWnd, WM_HOLIDAYS_LOADED, (WPARAM)bsYear, 0);
            return 0;
        }
    }

    // Failed to download and not available in cache
    g_holidayFetchState = FETCH_ERROR_OFFLINE;
    g_isHolidayFetchInProgress = false;
    if (g_hWnd) PostMessage(g_hWnd, WM_HOLIDAYS_FAILED, (WPARAM)bsYear, 0);
    return 0;
}

void EnsureHolidaysLoadedForCurrentYear(int bsYear, bool forceRefresh = false) {
    if (!forceRefresh && g_loadedHolidayYear == bsYear && g_holidayFetchState == FETCH_SUCCESS) {
        return;
    }

    // 1. Try encoded cache file first if not force refresh
    if (!forceRefresh) {
        std::vector<NepaliHoliday> cached;
        if (LoadEncodedHolidayFile(bsYear, cached)) {
            g_currentYearHolidays = cached;
            g_loadedHolidayYear = bsYear;
            g_holidayFetchState = FETCH_SUCCESS;
            return;
        }
    }

    // 2. Prevent multiple concurrent fetch threads
    if (g_isHolidayFetchInProgress) {
        return;
    }

    // 3. If offline failure already occurred and not force refresh, rate-limit retries (30-second cooldown)
    if (!forceRefresh && g_holidayFetchState == FETCH_ERROR_OFFLINE) {
        DWORD now = GetTickCount();
        if (now - g_lastHolidayFetchAttemptTime < 30000) {
            return;
        }
    }

    g_lastHolidayFetchAttemptTime = GetTickCount();
    HANDLE hThread = CreateThread(NULL, 0, FetchHolidaysThread, (LPVOID)(INT_PTR)bsYear, 0, NULL);
    if (hThread) CloseHandle(hThread);
}

const NepaliHoliday* GetHolidayForBS(int bsYear, int bsMonth, int bsDay) {
    if (g_holidayFetchState != FETCH_SUCCESS || g_loadedHolidayYear != bsYear) {
        return NULL;
    }
    for (size_t i = 0; i < g_currentYearHolidays.size(); ++i) {
        if (g_currentYearHolidays[i].month == bsMonth && g_currentYearHolidays[i].day == bsDay) {
            return &g_currentYearHolidays[i];
        }
    }
    return NULL;
}

inline int GetBSDaysInMonth(int bsYear, int bsMonth) {
    if (bsYear < 1975 || bsYear > 2100 || bsMonth < 1 || bsMonth > 12) return 30;
    return bs_month_days[bsYear - 1975][bsMonth - 1];
}

inline long long BSToDaysSinceEpoch(int bsYear, int bsMonth, int bsDay) {
    if (bsYear < 1975 || bsYear > 2100 || bsMonth < 1 || bsMonth > 12) return -1;
    long long days = 0;
    for (int y = 1975; y < bsYear; ++y) {
        for (int m = 0; m < 12; ++m) {
            days += bs_month_days[y - 1975][m];
        }
    }
    for (int m = 1; m < bsMonth; ++m) {
        days += bs_month_days[bsYear - 1975][m - 1];
    }
    days += (bsDay - 1);
    return days;
}

inline int GetBSMonthStartDayOfWeek(int bsYear, int bsMonth) {
    long long days = BSToDaysSinceEpoch(bsYear, bsMonth, 1);
    if (days < 0) return 0;
    return (int)((6 + days) % 7); // 0 = Sun, 1 = Mon, ..., 6 = Sat
}

inline bool BSToAD(int bsYear, int bsMonth, int bsDay, int& adYear, int& adMonth, int& adDay, int& outDayOfWeek) {
    long long days = BSToDaysSinceEpoch(bsYear, bsMonth, bsDay);
    if (days < 0) return false;
    outDayOfWeek = (int)((6 + days) % 7);

    SYSTEMTIME stRef = { 0 };
    stRef.wYear = 1918;
    stRef.wMonth = 4;
    stRef.wDay = 13;
    FILETIME ftRef;
    SystemTimeToFileTime(&stRef, &ftRef);

    ULARGE_INTEGER uRef;
    uRef.LowPart = ftRef.dwLowDateTime;
    uRef.HighPart = ftRef.dwHighDateTime;
    uRef.QuadPart += (ULONGLONG)days * 864000000000ULL;

    FILETIME ftTarget;
    ftTarget.dwLowDateTime = uRef.LowPart;
    ftTarget.dwHighDateTime = uRef.HighPart;
    SYSTEMTIME stTarget;
    FileTimeToSystemTime(&ftTarget, &stTarget);

    adYear = stTarget.wYear;
    adMonth = stTarget.wMonth;
    adDay = stTarget.wDay;
    return true;
}

inline bool ADToBS(int adYear, int adMonth, int adDay, int& bsYear, int& bsMonth, int& bsDay, int& outDayOfWeek) {
    SYSTEMTIME stNow = { 0 };
    stNow.wYear = (WORD)adYear;
    stNow.wMonth = (WORD)adMonth;
    stNow.wDay = (WORD)adDay;
    FILETIME ftNow;
    SystemTimeToFileTime(&stNow, &ftNow);

    SYSTEMTIME stRef = { 0 };
    stRef.wYear = 1918;
    stRef.wMonth = 4;
    stRef.wDay = 13;
    FILETIME ftRef;
    SystemTimeToFileTime(&stRef, &ftRef);

    ULARGE_INTEGER uNow, uRef;
    uNow.LowPart = ftNow.dwLowDateTime;
    uNow.HighPart = ftNow.dwHighDateTime;
    uRef.LowPart = ftRef.dwLowDateTime;
    uRef.HighPart = ftRef.dwHighDateTime;

    if (uNow.QuadPart < uRef.QuadPart) return false;
    long long diff = uNow.QuadPart - uRef.QuadPart;
    long long days = diff / 864000000000LL;
    outDayOfWeek = (int)((6 + days) % 7);

    int y = 1975;
    int m = 1;
    int d = 1;

    while (y <= 2100) {
        int dim = bs_month_days[y - 1975][m - 1];
        if (days >= dim) {
            days -= dim;
            m++;
            if (m > 12) {
                m = 1;
                y++;
            }
        } else {
            break;
        }
    }
    d += (int)days;
    if (y > 2100) return false;

    bsYear = y;
    bsMonth = m;
    bsDay = d;
    return true;
}

inline void GetCurrentBSDate(int& outYear, int& outMonth, int& outDay, int& outDayOfWeek) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    ADToBS(st.wYear, st.wMonth, st.wDay, outYear, outMonth, outDay, outDayOfWeek);
}

inline std::wstring ToDevanagariNum(int num) {
    static const wchar_t devanagariDigits[] = { L'०', L'१', L'२', L'३', L'४', L'५', L'६', L'७', L'८', L'९' };
    wchar_t buf[32];
    swprintf(buf, 32, L"%d", num);
    std::wstring result = L"";
    for (int i = 0; buf[i] != 0; ++i) {
        if (buf[i] >= L'0' && buf[i] <= L'9') {
            result += devanagariDigits[buf[i] - L'0'];
        } else {
            result += buf[i];
        }
    }
    return result;
}

std::wstring GetNepaliDateString() {
    static WORD s_lastDay = 0, s_lastMonth = 0, s_lastYear = 0;
    static std::wstring s_cachedDateStr = L"";

    SYSTEMTIME st;
    GetLocalTime(&st);
    
    // Minimal resource check: only recalculate if the day has changed!
    if (st.wDay == s_lastDay && st.wMonth == s_lastMonth && st.wYear == s_lastYear && !s_cachedDateStr.empty()) {
        return s_cachedDateStr;
    }
    
    s_lastDay = st.wDay;
    s_lastMonth = st.wMonth;
    s_lastYear = st.wYear;
    
    int bs_year, bs_month, bs_day, bs_dow;
    if (!ADToBS(st.wYear, st.wMonth, st.wDay, bs_year, bs_month, bs_day, bs_dow)) {
        return L"Date Error";
    }
    
    wchar_t buffer[64];
    swprintf(buffer, 64, L"%d / %02d / %02d", bs_year, bs_month, bs_day);
    s_cachedDateStr = std::wstring(buffer);
    
    return s_cachedDateStr;
}

// ── Tithi (Lunar Day) Calculation ────────────────────────────────────────────
// Returns the tithi index (0-29) for a given Gregorian date.
// Tithi 0-14 = Shukla Pratipada..Purnima, 15-29 = Krishna Pratipada..Amavasya.
static int CalcTithiIndex(int adYear, int adMonth, int adDay) {
    // Compute Julian Day Number at noon UTC for the given Gregorian date
    int a = (14 - adMonth) / 12;
    int y = adYear + 4800 - a;
    int m = adMonth + 12 * a - 3;
    double jd = adDay + (153 * m + 2) / 5 + 365.0 * y + y / 4 - y / 100 + y / 400 - 32045.0;
    // Hindu tithi is determined at sunrise (~6 AM Nepal time = 12:15 AM UTC).
    // Subtract 0.49 days to shift from noon UTC to Nepal sunrise.
    jd -= 0.49;
    // Reference: new moon (Amavasya) on 2000-01-06 at 18:14 UTC = JD ~2451550.26
    // Synodic month = 29.53058867 days
    double synodicMonth = 29.53058867;
    double newMoonRef = 2451550.26;
    double daysSinceNew = fmod(jd - newMoonRef, synodicMonth);
    if (daysSinceNew < 0) daysSinceNew += synodicMonth;
    // Each tithi spans synodicMonth/30 days. Index 0 = Shukla Pratipada.
    int tithiIdx = (int)(daysSinceNew / (synodicMonth / 30.0));
    if (tithiIdx < 0) tithiIdx = 0;
    if (tithiIdx > 29) tithiIdx = 29;
    return tithiIdx;
}

static std::wstring GetTithiString(int adYear, int adMonth, int adDay) {
    static const wchar_t* tithiNames[] = {
        L"Pratipada", L"Dwitiya",    L"Tritiya",    L"Chaturthi",
        L"Panchami",  L"Shashthi",   L"Saptami",    L"Ashtami",
        L"Navami",    L"Dashami",    L"Ekadashi",   L"Dwadashi",
        L"Trayodashi",L"Chaturdashi",L"Purnima",    // Shukla 1–15
        L"Pratipada", L"Dwitiya",    L"Tritiya",    L"Chaturthi",
        L"Panchami",  L"Shashthi",   L"Saptami",    L"Ashtami",
        L"Navami",    L"Dashami",    L"Ekadashi",   L"Dwadashi",
        L"Trayodashi",L"Chaturdashi",L"Amavasya"    // Krishna 1–15
    };
    int idx = CalcTithiIndex(adYear, adMonth, adDay);
    bool isKrishna = (idx >= 15);
    const wchar_t* paksha = isKrishna ? L"Krishna" : L"Shukla";
    return std::wstring(paksha) + L": " + tithiNames[idx];
}

// ── Shared Drawing Helpers ───────────────────────────────────────────────────
void AddRoundedRectangle(GraphicsPath& path, REAL x, REAL y, REAL w, REAL h, REAL r) {
    REAL d = r * 2.0f;
    if (d > w) d = w;
    if (d > h) d = h;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();
}

void DrawNepaliFlag(Graphics& graphics, REAL cx, REAL top, REAL h) {
    REAL flagWidth = 0.822f * h;
    SolidBrush blueBrush(Color(255, 0, 56, 147));
    SolidBrush crimsonBrush(Color(255, 220, 20, 60));
    SolidBrush whiteBrush(Color(255, 255, 255, 255));

    PointF polyOuter[5] = {
        PointF(cx, top),
        PointF(cx + 0.765f * h, top + 0.543f * h),
        PointF(cx + 0.266f * h, top + 0.543f * h),
        PointF(cx + 0.822f * h, top + h),
        PointF(cx, top + h)
    };
    graphics.FillPolygon(&blueBrush, polyOuter, 5);

    PointF polyInner[5] = {
        PointF(cx + 0.042f * h, top + 0.073f * h),
        PointF(cx + 0.674f * h, top + 0.501f * h),
        PointF(cx + 0.232f * h, top + 0.501f * h),
        PointF(cx + 0.715f * h, top + 0.958f * h),
        PointF(cx + 0.042f * h, top + 0.958f * h)
    };
    graphics.FillPolygon(&crimsonBrush, polyInner, 5);
    
    // Moon
    REAL moonOuterDiam = 0.20f * h;
    REAL moonOuterX = cx + 0.22f * h - (moonOuterDiam / 2.0f);
    REAL moonOuterY = top + 0.32f * h - (moonOuterDiam / 2.0f);
    graphics.FillEllipse(&whiteBrush, moonOuterX, moonOuterY, moonOuterDiam, moonOuterDiam);
    
    REAL moonInnerDiam = 0.18f * h;
    REAL moonInnerX = moonOuterX + 0.01f * h;
    REAL moonInnerY = moonOuterY - 0.04f * h;
    graphics.FillEllipse(&crimsonBrush, moonInnerX, moonInnerY, moonInnerDiam, moonInnerDiam);
    
    REAL moonStarDiam = 0.09f * h;
    REAL moonStarX = cx + 0.22f * h - (moonStarDiam / 2.0f);
    REAL moonStarY = top + 0.33f * h - (moonStarDiam / 2.0f);
    graphics.FillEllipse(&whiteBrush, moonStarX, moonStarY, moonStarDiam, moonStarDiam);

    // Sun
    REAL sunDiam = 0.18f * h;
    REAL sunX = cx + 0.26f * h - (sunDiam / 2.0f);
    REAL sunY = top + 0.73f * h - (sunDiam / 2.0f);
    graphics.FillEllipse(&whiteBrush, sunX, sunY, sunDiam, sunDiam);
}

void DrawIconButton(Graphics& graphics, const RectF& rect, const wchar_t* text, bool isHovered, bool isLight, Font& font) {
    GraphicsPath path;
    AddRoundedRectangle(path, rect.X, rect.Y, rect.Width, rect.Height, 6.0f * g_dpiScale);
    
    if (isHovered) {
        SolidBrush hoverBg(isLight ? Color(35, 0, 0, 0) : Color(45, 255, 255, 255));
        graphics.FillPath(&hoverBg, &path);
        Pen hoverBorder(isLight ? Color(70, 0, 0, 0) : Color(70, 255, 255, 255), 1.0f);
        graphics.DrawPath(&hoverBorder, &path);
    } else {
        SolidBrush normalBg(isLight ? Color(15, 0, 0, 0) : Color(20, 255, 255, 255));
        graphics.FillPath(&normalBg, &path);
    }

    SolidBrush textBrush(isLight ? (isHovered ? Color(255, 0, 0, 0) : Color(220, 40, 40, 40))
                                 : (isHovered ? Color(255, 255, 255, 255) : Color(220, 220, 220, 220)));
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    graphics.DrawString(text, -1, &font, rect, &format, &textBrush);
}

// ── Custom Reminder & Event Engine ───────────────────────────────────────────
struct CustomReminder {
    int id;               // unique timestamp id
    int bsYear;           // BS Year (e.g. 2082)
    int bsMonth;          // 1..12
    int bsDay;            // 1..32
    std::wstring title;   // Event title / name
    std::wstring desc;    // Description / details
    bool hasAlarm;        // Sound alarm enabled
    int alarmHour;        // 0..23 (24h)
    int alarmMinute;      // 0..59
    bool isAllDay;        // All-day event
    bool isTriggered;     // Already alerted
};

std::vector<CustomReminder> g_customReminders;
bool g_isReminderDashboardOpen = false;
HWND g_hReminderWnd = NULL;
RECT g_rcReminderDashboard = { 0, 0, 0, 0 };

bool g_isAlarmAlertOpen = false;
HWND g_hAlarmAlertWnd = NULL;
RECT g_rcAlarmAlert = { 0, 0, 0, 0 };
bool g_isAlarmSoundPlaying = false;
HANDLE g_hAlarmThread = NULL;
CustomReminder g_activeAlertReminder;

extern NOTIFYICONDATAW g_nid;
void RenderCalendar(HWND hWnd, BYTE alpha = 255, int overrideX = -1, int overrideY = -1);

std::wstring GetRemindersFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring dir = std::wstring(appDataPath) + L"\\Tithify";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\reminders.dat";
    }
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    size_t pos = ws.find_last_of(L"\\/");
    if (pos != std::wstring::npos) ws = ws.substr(0, pos);
    return ws + L"\\reminders.dat";
}

void LoadReminders() {
    g_customReminders.clear();
    std::wstring path = GetRemindersFilePath();
    FILE* file = _wfopen(path.c_str(), L"r");
    if (!file) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring ws(exePath);
        size_t pos = ws.find_last_of(L"\\/");
        if (pos != std::wstring::npos) ws = ws.substr(0, pos);
        std::wstring localPath = ws + L"\\reminders.dat";
        file = _wfopen(localPath.c_str(), L"r");
    }
    if (file) {
        char line[2048];
        while (fgets(line, sizeof(line), file)) {
            int len = (int)strlen(line);
            while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
                line[--len] = '\0';
            }
            if (len == 0) continue;

            std::vector<std::string> parts;
            std::string cur;
            for (int i = 0; i <= len; i++) {
                if (line[i] == '\t' || line[i] == '\0') {
                    parts.push_back(cur);
                    cur.clear();
                } else {
                    cur += line[i];
                }
            }
            if (parts.size() >= 10) {
                CustomReminder r;
                r.id = atoi(parts[0].c_str());
                r.bsYear = atoi(parts[1].c_str());
                r.bsMonth = atoi(parts[2].c_str());
                r.bsDay = atoi(parts[3].c_str());
                r.hasAlarm = (atoi(parts[4].c_str()) != 0);
                r.alarmHour = atoi(parts[5].c_str());
                r.alarmMinute = atoi(parts[6].c_str());
                r.isAllDay = (atoi(parts[7].c_str()) != 0);
                r.isTriggered = (atoi(parts[8].c_str()) != 0);
                r.title = Utf8ToWide(parts[9]);
                r.desc = (parts.size() >= 11) ? Utf8ToWide(parts[10]) : L"";
                g_customReminders.push_back(r);
            }
        }
        fclose(file);
    }
}

void SaveReminders() {
    std::wstring path = GetRemindersFilePath();
    FILE* file = _wfopen(path.c_str(), L"w");
    if (file) {
        for (size_t i = 0; i < g_customReminders.size(); i++) {
            const CustomReminder& r = g_customReminders[i];
            std::string utf8Title = WideToUtf8(r.title);
            std::string utf8Desc = WideToUtf8(r.desc);
            for (size_t c = 0; c < utf8Title.size(); c++) {
                if (utf8Title[c] == '\t' || utf8Title[c] == '\n' || utf8Title[c] == '\r') utf8Title[c] = ' ';
            }
            for (size_t c = 0; c < utf8Desc.size(); c++) {
                if (utf8Desc[c] == '\t' || utf8Desc[c] == '\n' || utf8Desc[c] == '\r') utf8Desc[c] = ' ';
            }
            fprintf(file, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
                    r.id, r.bsYear, r.bsMonth, r.bsDay,
                    r.hasAlarm ? 1 : 0, r.alarmHour, r.alarmMinute,
                    r.isAllDay ? 1 : 0, r.isTriggered ? 1 : 0,
                    utf8Title.c_str(), utf8Desc.c_str());
        }
        fclose(file);
    }
}

CustomReminder* FindReminderForBS(int y, int m, int d) {
    for (size_t i = 0; i < g_customReminders.size(); i++) {
        if (g_customReminders[i].bsYear == y && g_customReminders[i].bsMonth == m && g_customReminders[i].bsDay == d) {
            return &g_customReminders[i];
        }
    }
    return NULL;
}

bool HasReminderForBS(int y, int m, int d) {
    return FindReminderForBS(y, m, d) != NULL;
}

void DeleteReminderForBS(int y, int m, int d) {
    for (auto it = g_customReminders.begin(); it != g_customReminders.end(); ) {
        if (it->bsYear == y && it->bsMonth == m && it->bsDay == d) {
            it = g_customReminders.erase(it);
        } else {
            ++it;
        }
    }
    SaveReminders();
}

void AddOrUpdateReminder(const CustomReminder& rem) {
    bool found = false;
    for (size_t i = 0; i < g_customReminders.size(); i++) {
        if (g_customReminders[i].bsYear == rem.bsYear && g_customReminders[i].bsMonth == rem.bsMonth && g_customReminders[i].bsDay == rem.bsDay) {
            g_customReminders[i] = rem;
            found = true;
            break;
        }
    }
    if (!found) {
        g_customReminders.push_back(rem);
    }
    SaveReminders();
}

std::wstring FormatAlarmTime(int h24, int minute) {
    int h12 = h24 % 12;
    if (h12 == 0) h12 = 12;
    bool isPM = (h24 >= 12);
    wchar_t buf[32];
    swprintf_s(buf, 32, L"%02d:%02d %ls", h12, minute, isPM ? L"PM" : L"AM");
    return buf;
}

// ── Audio Alarm Chime Loop ───────────────────────────────────────────────────
DWORD WINAPI AlarmSoundThread(LPVOID lpParam) {
    while (g_isAlarmSoundPlaying) {
        Beep(784, 140); // G5
        if (!g_isAlarmSoundPlaying) break;
        Sleep(40);
        Beep(880, 140); // A5
        if (!g_isAlarmSoundPlaying) break;
        Sleep(40);
        Beep(1046, 220); // C6
        if (!g_isAlarmSoundPlaying) break;
        PlaySoundW(L"SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC);
        for (int i = 0; i < 15 && g_isAlarmSoundPlaying; i++) {
            Sleep(100);
        }
    }
    return 0;
}

void PlayAlarmSoundLoop() {
    if (g_isAlarmSoundPlaying) return;
    g_isAlarmSoundPlaying = true;
    g_hAlarmThread = CreateThread(NULL, 0, AlarmSoundThread, NULL, 0, NULL);
}

void StopAlarmSound() {
    g_isAlarmSoundPlaying = false;
    PlaySoundW(NULL, NULL, 0);
    if (g_hAlarmThread) {
        WaitForSingleObject(g_hAlarmThread, 350);
        CloseHandle(g_hAlarmThread);
        g_hAlarmThread = NULL;
    }
}

// ── Alarm Alert Popup Window ─────────────────────────────────────────────────
void RenderAlarmAlert(HWND hWnd) {
    if (!hWnd) return;

    float s = g_dpiScale;
    int rawW = (int)(440 * s);
    int rawH = (int)(250 * s);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rawW;
    bmi.bmiHeader.biHeight = -rawH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Graphics g(hdcMem);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.Clear(Color(0, 0, 0, 0));

    REAL w = (REAL)rawW;
    REAL h = (REAL)rawH;

    // Card background
    Color bgCardColor = g_isLightTheme ? Color(255, 255, 255, 255) : Color(255, 26, 27, 32);
    Color borderColor = Color(255, 220, 38, 38);

    GraphicsPath cardPath;
    REAL r = 16.0f * s;
    REAL d = r * 2.0f;
    cardPath.AddArc(0.0f, 0.0f, d, d, 180.0f, 90.0f);
    cardPath.AddArc(w - d, 0.0f, d, d, 270.0f, 90.0f);
    cardPath.AddArc(w - d, h - d, d, d, 0.0f, 90.0f);
    cardPath.AddArc(0.0f, h - d, d, d, 90.0f, 90.0f);
    cardPath.CloseFigure();

    SolidBrush cardBrush(bgCardColor);
    g.FillPath(&cardBrush, &cardPath);
    Pen borderPen(borderColor, 2.0f * s);
    g.DrawPath(&borderPen, &cardPath);

    // Header bar
    REAL headerH = 42.0f * s;
    FontFamily fontFamily(L"Segoe UI");
    Font brandFont(&fontFamily, 13.0f * s, FontStyleBold, UnitPixel);
    Color brandColor = g_isLightTheme ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255);
    SolidBrush brandBrush(brandColor);

    StringFormat fmtNearCenter;
    fmtNearCenter.SetAlignment(StringAlignmentNear);
    fmtNearCenter.SetLineAlignment(StringAlignmentCenter);

    RectF brandRect(20.0f * s, 0.0f, 260.0f * s, headerH);
    g.DrawString(L"\u23F0  TITHIFY EVENT ALARM", -1, &brandFont, brandRect, &fmtNearCenter, &brandBrush);

    // Close button [X]
    REAL closeBtnSize = 26.0f * s;
    REAL closeBtnX = w - 16.0f * s - closeBtnSize;
    REAL closeBtnY = (headerH - closeBtnSize) / 2.0f;
    Font closeFont(&fontFamily, 12.0f * s, FontStyleRegular, UnitPixel);
    StringFormat fmtCenter;
    fmtCenter.SetAlignment(StringAlignmentCenter);
    fmtCenter.SetLineAlignment(StringAlignmentCenter);
    RectF closeRect(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
    SolidBrush closeBrush(g_isLightTheme ? Color(180, 80, 80, 80) : Color(220, 200, 200, 200));
    g.DrawString(L"\u2715", -1, &closeFont, closeRect, &fmtCenter, &closeBrush);

    Color divColor = g_isLightTheme ? Color(30, 0, 0, 0) : Color(35, 255, 255, 255);
    Pen divPen(divColor, 1.0f * s);
    g.DrawLine(&divPen, 16.0f * s, headerH, w - 16.0f * s, headerH);

    // Body Area: Glowing Alarm Bell Circle
    REAL iconBoxX = 22.0f * s;
    REAL iconBoxY = headerH + 20.0f * s;
    REAL iconBoxSize = 52.0f * s;

    SolidBrush bellBg(Color(45, 220, 38, 38));
    g.FillEllipse(&bellBg, iconBoxX, iconBoxY, iconBoxSize, iconBoxSize);
    Pen bellRingPen(Color(200, 220, 38, 38), 1.5f * s);
    g.DrawEllipse(&bellRingPen, iconBoxX, iconBoxY, iconBoxSize, iconBoxSize);

    Font bellFont(&fontFamily, 24.0f * s, FontStyleBold, UnitPixel);
    RectF bellRect(iconBoxX, iconBoxY, iconBoxSize, iconBoxSize);
    SolidBrush bellTextBrush(Color(255, 255, 255, 255));
    g.DrawString(L"\u23F0", -1, &bellFont, bellRect, &fmtCenter, &bellTextBrush);

    // Event Title
    REAL textX = iconBoxX + iconBoxSize + 16.0f * s;
    REAL textW = w - textX - 20.0f * s;

    Font titleFont(&fontFamily, 17.0f * s, FontStyleBold, UnitPixel);
    SolidBrush titleBrush(brandColor);
    RectF titleRect(textX, iconBoxY - 2.0f * s, textW, 26.0f * s);
    g.DrawString(g_activeAlertReminder.title.c_str(), -1, &titleFont, titleRect, &fmtNearCenter, &titleBrush);

    // Date & Time Subtitle
    std::wstring npDate = ToDevanagariNum(g_activeAlertReminder.bsDay) + L" " + kNepaliMonthNamesNP[g_activeAlertReminder.bsMonth - 1] + L" " + ToDevanagariNum(g_activeAlertReminder.bsYear);
    std::wstring timeStr = g_activeAlertReminder.isAllDay ? L"All-Day Event" : (L"Alarm Time: " + FormatAlarmTime(g_activeAlertReminder.alarmHour, g_activeAlertReminder.alarmMinute));
    std::wstring subStr = npDate + L"  \u2022  " + timeStr;

    Font subFont(&fontFamily, 11.5f * s, FontStyleBold, UnitPixel);
    SolidBrush subBrush(Color(255, 220, 38, 38));
    RectF subRect(textX, iconBoxY + 26.0f * s, textW, 20.0f * s);
    g.DrawString(subStr.c_str(), -1, &subFont, subRect, &fmtNearCenter, &subBrush);

    // Description
    Font descFont(&fontFamily, 12.5f * s, FontStyleRegular, UnitPixel);
    Color msgColor = g_isLightTheme ? Color(255, 60, 64, 72) : Color(255, 228, 232, 238);
    SolidBrush descBrush(msgColor);
    StringFormat fmtWrap;
    fmtWrap.SetAlignment(StringAlignmentNear);
    fmtWrap.SetLineAlignment(StringAlignmentNear);
    RectF descRect(textX, iconBoxY + 48.0f * s, textW, 50.0f * s);
    std::wstring descText = g_activeAlertReminder.desc.empty() ? L"Custom event alert scheduled for today." : g_activeAlertReminder.desc;
    g.DrawString(descText.c_str(), -1, &descFont, descRect, &fmtWrap, &descBrush);

    // Buttons
    REAL btnH = 36.0f * s;
    REAL btnY = h - 16.0f * s - btnH;

    // Snooze 5 Min Button
    REAL snoozeW = 110.0f * s;
    REAL snoozeX = w - 18.0f * s - 140.0f * s - 10.0f * s - snoozeW;
    GraphicsPath snPath; AddRoundedRectangle(snPath, snoozeX, btnY, snoozeW, btnH, 8.0f * s);
    SolidBrush snBg(g_isLightTheme ? Color(20, 0, 0, 0) : Color(25, 255, 255, 255));
    g.FillPath(&snBg, &snPath);
    Pen snBorder(g_isLightTheme ? Color(50, 0, 0, 0) : Color(50, 255, 255, 255), 1.0f * s);
    g.DrawPath(&snBorder, &snPath);
    Font btnFont(&fontFamily, 12.0f * s, FontStyleBold, UnitPixel);
    RectF snRect(snoozeX, btnY, snoozeW, btnH);
    g.DrawString(L"Snooze 5 Min", -1, &btnFont, snRect, &fmtCenter, &brandBrush);

    // Dismiss Alarm Button
    REAL dismW = 140.0f * s;
    REAL dismX = w - 18.0f * s - dismW;
    GraphicsPath dismPath; AddRoundedRectangle(dismPath, dismX, btnY, dismW, btnH, 8.0f * s);
    SolidBrush dismBg(Color(255, 220, 38, 38));
    g.FillPath(&dismBg, &dismPath);
    RectF dismRect(dismX, btnY, dismW, btnH);
    SolidBrush dismText(Color(255, 255, 255, 255));
    g.DrawString(L"\u2715 Dismiss Alarm", -1, &btnFont, dismRect, &fmtCenter, &dismText);

    // Apply alpha update
    RECT rcW; GetWindowRect(hWnd, &rcW);
    POINT ptDst = { rcW.left, rcW.top };
    SIZE sizeDst = { rawW, rawH };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeDst, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void CloseAlarmAlert();

LRESULT CALLBACK AlarmAlertWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR: {
        POINT pt; GetCursorPos(&pt);
        RECT rc; GetWindowRect(hWnd, &rc);
        int lx = pt.x - rc.left;
        int ly = pt.y - rc.top;
        float s = g_dpiScale;
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        int btnH = (int)(36 * s);
        int btnY = h - (int)(16 * s) - btnH;
        int dismW = (int)(140 * s);
        int dismX = w - (int)(18 * s) - dismW;
        int snoozeW = (int)(110 * s);
        int snoozeX = dismX - (int)(10 * s) - snoozeW;
        int closeBtnSize = (int)(26 * s);
        int closeBtnX = w - (int)(16 * s) - closeBtnSize;
        int closeBtnY = (int)((42 * s - closeBtnSize) / 2);

        bool onBtn = false;
        if (lx >= dismX && lx <= dismX + dismW && ly >= btnY && ly <= btnY + btnH) onBtn = true;
        else if (lx >= snoozeX && lx <= snoozeX + snoozeW && ly >= btnY && ly <= btnY + btnH) onBtn = true;
        else if (lx >= closeBtnX && lx <= closeBtnX + closeBtnSize && ly >= closeBtnY && ly <= closeBtnY + closeBtnSize) onBtn = true;

        if (onBtn) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }

    case WM_LBUTTONDOWN: {
        POINT pt; GetCursorPos(&pt);
        RECT rc; GetWindowRect(hWnd, &rc);
        int lx = pt.x - rc.left;
        int ly = pt.y - rc.top;
        float s = g_dpiScale;
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        int btnH = (int)(36 * s);
        int btnY = h - (int)(16 * s) - btnH;
        int dismW = (int)(140 * s);
        int dismX = w - (int)(18 * s) - dismW;
        int snoozeW = (int)(110 * s);
        int snoozeX = dismX - (int)(10 * s) - snoozeW;
        int closeBtnSize = (int)(26 * s);
        int closeBtnX = w - (int)(16 * s) - closeBtnSize;
        int closeBtnY = (int)((42 * s - closeBtnSize) / 2);

        if (lx >= dismX && lx <= dismX + dismW && ly >= btnY && ly <= btnY + btnH) {
            CloseAlarmAlert();
        } else if (lx >= snoozeX && lx <= snoozeX + snoozeW && ly >= btnY && ly <= btnY + btnH) {
            SYSTEMTIME st; GetLocalTime(&st);
            int nextMin = st.wMinute + 5;
            int nextHour = st.wHour + (nextMin / 60);
            nextMin %= 60; nextHour %= 24;
            g_activeAlertReminder.alarmHour = nextHour;
            g_activeAlertReminder.alarmMinute = nextMin;
            g_activeAlertReminder.isTriggered = false;
            AddOrUpdateReminder(g_activeAlertReminder);
            CloseAlarmAlert();
        } else if (lx >= closeBtnX && lx <= closeBtnX + closeBtnSize && ly >= closeBtnY && ly <= closeBtnY + closeBtnSize) {
            CloseAlarmAlert();
        }
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || wParam == VK_RETURN || wParam == VK_SPACE) {
            CloseAlarmAlert();
        }
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowAlarmAlert(const CustomReminder& rem) {
    g_activeAlertReminder = rem;
    PlayAlarmSoundLoop();

    HINSTANCE hInst = GetModuleHandle(NULL);
    static bool s_alarmClassRegistered = false;
    if (!s_alarmClassRegistered) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = AlarmAlertWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = L"NepaliAlarmAlertClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassExW(&wc);
        s_alarmClassRegistered = true;
    }

    float s = g_dpiScale;
    int dlgW = (int)(440 * s);
    int dlgH = (int)(250 * s);

    HMONITOR hMon = MonitorFromWindow(g_hWnd ? g_hWnd : GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    int dlgX = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - dlgW) / 2;
    int dlgY = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - dlgH) / 2;

    if (!g_hAlarmAlertWnd || !IsWindow(g_hAlarmAlertWnd)) {
        g_hAlarmAlertWnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            L"NepaliAlarmAlertClass",
            L"Tithify Event Alarm",
            WS_POPUP,
            dlgX, dlgY, dlgW, dlgH,
            NULL, NULL, hInst, NULL
        );
    } else {
        SetWindowPos(g_hAlarmAlertWnd, HWND_TOPMOST, dlgX, dlgY, dlgW, dlgH, SWP_NOACTIVATE);
    }

    g_isAlarmAlertOpen = true;
    RenderAlarmAlert(g_hAlarmAlertWnd);
    ShowWindow(g_hAlarmAlertWnd, SW_SHOWNOACTIVATE);
    SetForegroundWindow(g_hAlarmAlertWnd);
    SetFocus(g_hAlarmAlertWnd);

    // Show tray balloon toast
    NOTIFYICONDATAW nid = g_nid;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcscpy_s(nid.szInfoTitle, L"\u23F0 Tithify Event Alarm!");
    std::wstring toastText = rem.title;
    if (!rem.desc.empty()) toastText += L" - " + rem.desc;
    wcsncpy_s(nid.szInfo, toastText.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void CloseAlarmAlert() {
    StopAlarmSound();
    if (g_hAlarmAlertWnd && IsWindow(g_hAlarmAlertWnd)) {
        ShowWindow(g_hAlarmAlertWnd, SW_HIDE);
    }
    g_isAlarmAlertOpen = false;
}

// ── Background Reminder Poller Engine ────────────────────────────────────────
void CheckPendingReminders() {
    int curY = 0, curM = 0, curD = 0, curDow = 0;
    GetCurrentBSDate(curY, curM, curD, curDow);

    SYSTEMTIME st;
    GetLocalTime(&st);

    for (size_t i = 0; i < g_customReminders.size(); i++) {
        CustomReminder& rem = g_customReminders[i];
        if (rem.bsYear == curY && rem.bsMonth == curM && rem.bsDay == curD) {
            if (!rem.isTriggered) {
                bool shouldTrigger = false;
                if (rem.isAllDay) {
                    shouldTrigger = true;
                } else if (rem.hasAlarm) {
                    if (st.wHour == rem.alarmHour && st.wMinute == rem.alarmMinute) {
                        shouldTrigger = true;
                    } else if (st.wHour > rem.alarmHour || (st.wHour == rem.alarmHour && st.wMinute > rem.alarmMinute)) {
                        shouldTrigger = true;
                    }
                }

                if (shouldTrigger) {
                    rem.isTriggered = true;
                    SaveReminders();
                    ShowAlarmAlert(rem);
                    break;
                }
            }
        }
    }
}

// ── Reminder Dashboard State & Engine ────────────────────────────────────────
struct ReminderDashboardState {
    int bsYear = 2082;
    int bsMonth = 1;
    int bsDay = 1;
    std::wstring title;
    std::wstring desc;
    bool hasAlarm = true;
    bool isAllDay = false;
    int hour12 = 9;
    int minute = 0;
    bool isPM = false;

    int focusedField = 1; // 1 = Title, 2 = Description, 0 = None
    bool cursorBlink = true;
    int hoveredCtrl = -1;
    bool isDragging = false;
    POINT dragStart = { 0, 0 };
    int dlgX = 0;
    int dlgY = 0;
    int dlgW = 430;
    int dlgH = 385;
    bool isExisting = false;
} g_remState;

void CloseReminderDashboard();

void RenderReminderDashboard(HWND hWnd) {
    if (!hWnd) return;

    float s = g_dpiScale;
    int rawW = (int)(430 * s);
    int rawH = (int)(385 * s);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rawW;
    bmi.bmiHeader.biHeight = -rawH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    graphics.Clear(Color(0, 0, 0, 0));

    REAL w = (REAL)rawW;
    REAL h = (REAL)rawH;

    // Outer card background
    Color bgCardColor = g_isLightTheme ? Color(248, 245, 245, 248) : Color(245, 24, 24, 28);
    Color borderColor = g_isLightTheme ? Color(180, 200, 0, 0) : Color(200, 220, 0, 0);

    SolidBrush cardBrush(bgCardColor);
    GraphicsPath cardPath;
    REAL r = 14.0f * s;
    REAL d = r * 2.0f;
    cardPath.AddArc(0.0f, 0.0f, d, d, 180.0f, 90.0f);
    cardPath.AddArc(w - d, 0.0f, d, d, 270.0f, 90.0f);
    cardPath.AddArc(w - d, h - d, d, d, 0.0f, 90.0f);
    cardPath.AddArc(0.0f, h - d, d, d, 90.0f, 90.0f);
    cardPath.CloseFigure();
    graphics.FillPath(&cardBrush, &cardPath);
    Pen borderPen(borderColor, 1.5f * s);
    graphics.DrawPath(&borderPen, &cardPath);

    // Header bar
    REAL headerH = 40.0f * s;
    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 13.5f * s, FontStyleBold, UnitPixel);
    Font subFont(&fontFamily, 10.5f * s, FontStyleBold, UnitPixel);
    Font labelFont(&fontFamily, 10.5f * s, FontStyleBold, UnitPixel);
    Font inputFont(&fontFamily, 12.5f * s, FontStyleRegular, UnitPixel);
    Font btnFont(&fontFamily, 12.0f * s, FontStyleBold, UnitPixel);

    Color textColor = g_isLightTheme ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255);
    Color subTextColor = g_isLightTheme ? Color(180, 90, 90, 90) : Color(180, 180, 180, 180);
    SolidBrush textBrush(textColor);
    SolidBrush subTextBrush(subTextColor);

    StringFormat formatNear;
    formatNear.SetAlignment(StringAlignmentNear);
    formatNear.SetLineAlignment(StringAlignmentCenter);

    StringFormat formatCenter;
    formatCenter.SetAlignment(StringAlignmentCenter);
    formatCenter.SetLineAlignment(StringAlignmentCenter);

    StringFormat formatFar;
    formatFar.SetAlignment(StringAlignmentFar);
    formatFar.SetLineAlignment(StringAlignmentCenter);

    // Title: "SET REMINDER" or "EDIT REMINDER"
    RectF titleRect(20.0f * s, 0.0f, 170.0f * s, headerH);
    graphics.DrawString(g_remState.isExisting ? L"\u23F0 EDIT REMINDER" : L"\u23F0 SET REMINDER", -1, &titleFont, titleRect, &formatNear, &textBrush);

    // Date subtitle
    std::wstring npDateStr = ToDevanagariNum(g_remState.bsDay) + L" " + kNepaliMonthNamesNP[g_remState.bsMonth - 1] + L" " + ToDevanagariNum(g_remState.bsYear);
    int adY = 0, adM = 0, adD = 0, adDow = 0;
    BSToAD(g_remState.bsYear, g_remState.bsMonth, g_remState.bsDay, adY, adM, adD, adDow);
    std::wstring adDateStr = std::to_wstring(adD) + L" " + kEnglishMonthNames[adM - 1];
    std::wstring fullDateStr = npDateStr + L"  \u2022  " + adDateStr;

    RectF dateRect(175.0f * s, 0.0f, w - 215.0f * s, headerH);
    graphics.DrawString(fullDateStr.c_str(), -1, &subFont, dateRect, &formatFar, &subTextBrush);

    // Close [X] button
    REAL closeBtnSize = 24.0f * s;
    REAL closeBtnX = w - 16.0f * s - closeBtnSize;
    REAL closeBtnY = (headerH - closeBtnSize) / 2.0f;
    if (g_remState.hoveredCtrl == 1) {
        SolidBrush hoverCloseBrush(g_isLightTheme ? Color(30, 0, 0, 0) : Color(45, 255, 255, 255));
        graphics.FillEllipse(&hoverCloseBrush, closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
    }
    RectF closeRect(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
    graphics.DrawString(L"\u2715", -1, &subFont, closeRect, &formatCenter, &subTextBrush);

    // Header separator line
    Color lineHorizColor = g_isLightTheme ? Color(40, 0, 0, 0) : Color(40, 255, 255, 255);
    Pen lineHorizPen(lineHorizColor, 1.0f * s);
    graphics.DrawLine(&lineHorizPen, 16.0f * s, headerH, w - 16.0f * s, headerH);

    // Section 1: Event Name Input Box
    RectF lblTitleRect(22.0f * s, 48.0f * s, w - 44.0f * s, 18.0f * s);
    graphics.DrawString(L"EVENT NAME / TITLE", -1, &labelFont, lblTitleRect, &formatNear, &textBrush);

    REAL tBoxX = 22.0f * s, tBoxY = 68.0f * s, tBoxW = w - 44.0f * s, tBoxH = 34.0f * s;
    GraphicsPath tBoxPath;
    AddRoundedRectangle(tBoxPath, tBoxX, tBoxY, tBoxW, tBoxH, 6.0f * s);
    SolidBrush inputBg(g_isLightTheme ? Color(20, 0, 0, 0) : Color(25, 255, 255, 255));
    graphics.FillPath(&inputBg, &tBoxPath);
    Pen tBorderPen(g_remState.focusedField == 1 ? Color(255, 220, 38, 38) : (g_isLightTheme ? Color(50, 0, 0, 0) : Color(50, 255, 255, 255)), g_remState.focusedField == 1 ? 1.5f * s : 1.0f * s);
    graphics.DrawPath(&tBorderPen, &tBoxPath);

    RectF tTextRect(tBoxX + 10.0f * s, tBoxY, tBoxW - 20.0f * s, tBoxH);
    if (g_remState.title.empty()) {
        graphics.DrawString(L"e.g. Birthday, Meeting, Exam, Festival...", -1, &inputFont, tTextRect, &formatNear, &subTextBrush);
    } else {
        graphics.DrawString(g_remState.title.c_str(), -1, &inputFont, tTextRect, &formatNear, &textBrush);
    }

    if (g_remState.focusedField == 1 && g_remState.cursorBlink) {
        RectF measured;
        graphics.MeasureString(g_remState.title.c_str(), -1, &inputFont, tTextRect, &formatNear, &measured);
        REAL caretX = tBoxX + 10.0f * s + measured.Width;
        if (caretX > tBoxX + tBoxW - 10.0f * s) caretX = tBoxX + tBoxW - 10.0f * s;
        Pen caretPen(textColor, 1.5f * s);
        graphics.DrawLine(&caretPen, caretX, tBoxY + 7.0f * s, caretX, tBoxY + tBoxH - 7.0f * s);
    }

    // Section 2: Description Input Box
    RectF lblDescRect(22.0f * s, 108.0f * s, w - 44.0f * s, 18.0f * s);
    graphics.DrawString(L"DESCRIPTION / NOTES", -1, &labelFont, lblDescRect, &formatNear, &textBrush);

    REAL dBoxX = 22.0f * s, dBoxY = 126.0f * s, dBoxW = w - 44.0f * s, dBoxH = 50.0f * s;
    GraphicsPath dBoxPath;
    AddRoundedRectangle(dBoxPath, dBoxX, dBoxY, dBoxW, dBoxH, 6.0f * s);
    graphics.FillPath(&inputBg, &dBoxPath);
    Pen dBorderPen(g_remState.focusedField == 2 ? Color(255, 220, 38, 38) : (g_isLightTheme ? Color(50, 0, 0, 0) : Color(50, 255, 255, 255)), g_remState.focusedField == 2 ? 1.5f * s : 1.0f * s);
    graphics.DrawPath(&dBorderPen, &dBoxPath);

    StringFormat formatDescInput;
    formatDescInput.SetAlignment(StringAlignmentNear);
    formatDescInput.SetLineAlignment(StringAlignmentNear);
    RectF dTextRect(dBoxX + 10.0f * s, dBoxY + 6.0f * s, dBoxW - 20.0f * s, dBoxH - 12.0f * s);
    if (g_remState.desc.empty()) {
        graphics.DrawString(L"Short description, location, or reminder notes...", -1, &inputFont, dTextRect, &formatDescInput, &subTextBrush);
    } else {
        graphics.DrawString(g_remState.desc.c_str(), -1, &inputFont, dTextRect, &formatDescInput, &textBrush);
    }

    if (g_remState.focusedField == 2 && g_remState.cursorBlink) {
        RectF measured;
        graphics.MeasureString(g_remState.desc.c_str(), -1, &inputFont, dTextRect, &formatDescInput, &measured);
        REAL caretX = dBoxX + 10.0f * s + measured.Width;
        REAL caretY = dBoxY + 6.0f * s;
        if (caretX > dBoxX + dBoxW - 10.0f * s) caretX = dBoxX + dBoxW - 10.0f * s;
        Pen caretPen(textColor, 1.5f * s);
        graphics.DrawLine(&caretPen, caretX, caretY + 2.0f * s, caretX, caretY + 18.0f * s);
    }

    // Section 3: Alarm & Time Controls Card
    REAL aCardX = 22.0f * s, aCardY = 186.0f * s, aCardW = w - 44.0f * s, aCardH = 96.0f * s;
    GraphicsPath aCardPath;
    AddRoundedRectangle(aCardPath, aCardX, aCardY, aCardW, aCardH, 8.0f * s);
    SolidBrush aCardBg(g_isLightTheme ? Color(12, 0, 0, 0) : Color(18, 255, 255, 255));
    graphics.FillPath(&aCardBg, &aCardPath);
    Pen aCardBorder(g_isLightTheme ? Color(35, 0, 0, 0) : Color(35, 255, 255, 255), 1.0f * s);
    graphics.DrawPath(&aCardBorder, &aCardPath);

    // Row 1: Sound Alarm toggle & All-Day toggle
    RectF soundLblRect(aCardX + 12.0f * s, aCardY + 10.0f * s, 100.0f * s, 22.0f * s);
    graphics.DrawString(L"Sound Alarm", -1, &labelFont, soundLblRect, &formatNear, &textBrush);

    REAL pillX = aCardX + 112.0f * s, pillY = aCardY + 10.0f * s, pillW = 42.0f * s, pillH = 22.0f * s;
    GraphicsPath pPath;
    REAL pillR = pillH / 2.0f;
    pPath.AddArc(pillX, pillY, pillR * 2.0f, pillR * 2.0f, 90.0f, 180.0f);
    pPath.AddArc(pillX + pillW - pillR * 2.0f, pillY, pillR * 2.0f, pillR * 2.0f, 270.0f, 180.0f);
    pPath.CloseFigure();
    SolidBrush pillBrush(g_remState.hasAlarm ? Color(255, 220, 38, 38) : (g_isLightTheme ? Color(255, 200, 200, 204) : Color(255, 80, 80, 85)));
    graphics.FillPath(&pillBrush, &pPath);
    REAL knobSz = 18.0f * s;
    REAL knobX = g_remState.hasAlarm ? (pillX + pillW - knobSz - 2.0f * s) : (pillX + 2.0f * s);
    REAL knobY = pillY + (pillH - knobSz) / 2.0f;
    SolidBrush knobBrush(Color(255, 255, 255, 255));
    graphics.FillEllipse(&knobBrush, knobX, knobY, knobSz, knobSz);

    RectF allDayLblRect(aCardX + 185.0f * s, aCardY + 10.0f * s, 120.0f * s, 22.0f * s);
    graphics.DrawString(L"All-Day Event", -1, &labelFont, allDayLblRect, &formatNear, &textBrush);

    REAL allDayX = aCardX + aCardW - 54.0f * s, allDayY = aCardY + 10.0f * s, allDayW = 42.0f * s, allDayH = 22.0f * s;
    GraphicsPath adPath;
    adPath.AddArc(allDayX, allDayY, pillR * 2.0f, pillR * 2.0f, 90.0f, 180.0f);
    adPath.AddArc(allDayX + allDayW - pillR * 2.0f, allDayY, pillR * 2.0f, pillR * 2.0f, 270.0f, 180.0f);
    adPath.CloseFigure();
    SolidBrush adBrush(g_remState.isAllDay ? Color(255, 220, 38, 38) : (g_isLightTheme ? Color(255, 200, 200, 204) : Color(255, 80, 80, 85)));
    graphics.FillPath(&adBrush, &adPath);
    REAL adKnobX = g_remState.isAllDay ? (allDayX + allDayW - knobSz - 2.0f * s) : (allDayX + 2.0f * s);
    graphics.FillEllipse(&knobBrush, adKnobX, knobY, knobSz, knobSz);

    // Row 2: Time Controls
    REAL row2Y = aCardY + 48.0f * s;
    if (!g_remState.isAllDay) {
        RectF tLblRect(aCardX + 12.0f * s, row2Y + 3.0f * s, 85.0f * s, 26.0f * s);
        graphics.DrawString(L"Alarm Time:", -1, &labelFont, tLblRect, &formatNear, &textBrush);

        REAL hMinusX = aCardX + 102.0f * s, hY = row2Y + 2.0f * s, stepBtnW = 22.0f * s, stepBtnH = 26.0f * s;
        RectF hMinusRect(hMinusX, hY, stepBtnW, stepBtnH);
        DrawIconButton(graphics, hMinusRect, L"\u2212", g_remState.hoveredCtrl == 6, g_isLightTheme, btnFont);

        REAL hBoxX = hMinusX + stepBtnW + 3.0f * s, hBoxW = 32.0f * s;
        RectF hBoxRect(hBoxX, hY, hBoxW, stepBtnH);
        GraphicsPath hBoxPath; AddRoundedRectangle(hBoxPath, hBoxX, hY, hBoxW, stepBtnH, 4.0f * s);
        graphics.FillPath(&inputBg, &hBoxPath);
        graphics.DrawPath(&aCardBorder, &hBoxPath);
        wchar_t hBuf[16]; swprintf_s(hBuf, 16, L"%02d", g_remState.hour12);
        graphics.DrawString(hBuf, -1, &btnFont, hBoxRect, &formatCenter, &textBrush);

        REAL hPlusX = hBoxX + hBoxW + 3.0f * s;
        RectF hPlusRect(hPlusX, hY, stepBtnW, stepBtnH);
        DrawIconButton(graphics, hPlusRect, L"+", g_remState.hoveredCtrl == 7, g_isLightTheme, btnFont);

        REAL colonX = hPlusX + stepBtnW + 2.0f * s;
        RectF colonRect(colonX, hY, 12.0f * s, stepBtnH);
        graphics.DrawString(L":", -1, &btnFont, colonRect, &formatCenter, &textBrush);

        REAL mMinusX = colonX + 12.0f * s;
        RectF mMinusRect(mMinusX, hY, stepBtnW, stepBtnH);
        DrawIconButton(graphics, mMinusRect, L"\u2212", g_remState.hoveredCtrl == 8, g_isLightTheme, btnFont);

        REAL mBoxX = mMinusX + stepBtnW + 3.0f * s, mBoxW = 32.0f * s;
        RectF mBoxRect(mBoxX, hY, mBoxW, stepBtnH);
        GraphicsPath mBoxPath; AddRoundedRectangle(mBoxPath, mBoxX, hY, mBoxW, stepBtnH, 4.0f * s);
        graphics.FillPath(&inputBg, &mBoxPath);
        graphics.DrawPath(&aCardBorder, &mBoxPath);
        wchar_t mBuf[16]; swprintf_s(mBuf, 16, L"%02d", g_remState.minute);
        graphics.DrawString(mBuf, -1, &btnFont, mBoxRect, &formatCenter, &textBrush);

        REAL mPlusX = mBoxX + mBoxW + 3.0f * s;
        RectF mPlusRect(mPlusX, hY, stepBtnW, stepBtnH);
        DrawIconButton(graphics, mPlusRect, L"+", g_remState.hoveredCtrl == 9, g_isLightTheme, btnFont);

        REAL ampmX = mPlusX + stepBtnW + 8.0f * s, ampmW = 44.0f * s;
        RectF ampmRect(ampmX, hY, ampmW, stepBtnH);
        DrawIconButton(graphics, ampmRect, g_remState.isPM ? L"PM" : L"AM", g_remState.hoveredCtrl == 10, g_isLightTheme, btnFont);

        REAL bellX = ampmX + ampmW + 6.0f * s, bellW = 28.0f * s;
        RectF bellRect(bellX, hY, bellW, stepBtnH);
        DrawIconButton(graphics, bellRect, L"\u23F0", g_remState.hoveredCtrl == 14, g_isLightTheme, btnFont);
    } else {
        RectF allDayNoteRect(aCardX + 14.0f * s, row2Y + 6.0f * s, aCardW - 28.0f * s, 26.0f * s);
        graphics.DrawString(L"\u24D8 Alert will sound on the morning of this Nepali date.", -1, &subFont, allDayNoteRect, &formatNear, &subTextBrush);
    }

    // Section 4: Action Buttons
    REAL btnH = 34.0f * s;
    REAL btnY = h - 16.0f * s - btnH;

    if (g_remState.isExisting) {
        REAL delW = 85.0f * s;
        REAL delX = 22.0f * s;
        RectF delRect(delX, btnY, delW, btnH);
        GraphicsPath delPath; AddRoundedRectangle(delPath, delX, btnY, delW, btnH, 8.0f * s);
        bool isDelHovered = (g_remState.hoveredCtrl == 12);
        SolidBrush delBg(isDelHovered ? Color(255, 220, 38, 38) : Color(20, 220, 38, 38));
        graphics.FillPath(&delBg, &delPath);
        Pen delBorder(isDelHovered ? Color(255, 220, 38, 38) : Color(130, 220, 38, 38), 1.0f * s);
        graphics.DrawPath(&delBorder, &delPath);
        SolidBrush delText(isDelHovered ? Color(255, 255, 255, 255) : Color(255, 220, 38, 38));
        graphics.DrawString(L"Delete", -1, &btnFont, delRect, &formatCenter, &delText);
    }

    REAL cancelW = 80.0f * s;
    REAL cancelX = (g_remState.isExisting ? 180.0f : 160.0f) * s;
    RectF cancelRect(cancelX, btnY, cancelW, btnH);
    GraphicsPath cancelPath; AddRoundedRectangle(cancelPath, cancelX, btnY, cancelW, btnH, 8.0f * s);
    SolidBrush cancelBg(g_remState.hoveredCtrl == 13 ? (g_isLightTheme ? Color(35, 0, 0, 0) : Color(45, 255, 255, 255)) : (g_isLightTheme ? Color(15, 0, 0, 0) : Color(20, 255, 255, 255)));
    graphics.FillPath(&cancelBg, &cancelPath);
    Pen cancelBorder(g_isLightTheme ? Color(50, 0, 0, 0) : Color(50, 255, 255, 255), 1.0f * s);
    graphics.DrawPath(&cancelBorder, &cancelPath);
    graphics.DrawString(L"Cancel", -1, &btnFont, cancelRect, &formatCenter, &textBrush);

    REAL saveW = 145.0f * s;
    REAL saveX = w - 22.0f * s - saveW;
    RectF saveRect(saveX, btnY, saveW, btnH);
    GraphicsPath savePath; AddRoundedRectangle(savePath, saveX, btnY, saveW, btnH, 8.0f * s);
    SolidBrush saveBg(g_remState.hoveredCtrl == 11 ? Color(255, 240, 55, 55) : Color(255, 220, 38, 38));
    graphics.FillPath(&saveBg, &savePath);
    SolidBrush saveText(Color(255, 255, 255, 255));
    graphics.DrawString(g_remState.hasAlarm ? L"Save & Set Alarm" : L"Save Reminder", -1, &btnFont, saveRect, &formatCenter, &saveText);

    POINT ptDst = { g_remState.dlgX, g_remState.dlgY };
    SIZE sizeDst = { rawW, rawH };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeDst, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

int HitTestReminderDashboard(int lx, int ly, float s, REAL w, REAL h) {
    REAL headerH = 40.0f * s;
    REAL closeBtnSize = 24.0f * s;
    REAL closeBtnX = w - 16.0f * s - closeBtnSize;
    REAL closeBtnY = (headerH - closeBtnSize) / 2.0f;
    if (lx >= closeBtnX && lx <= closeBtnX + closeBtnSize && ly >= closeBtnY && ly <= closeBtnY + closeBtnSize) return 1;

    REAL tBoxX = 22.0f * s, tBoxY = 68.0f * s, tBoxW = w - 44.0f * s, tBoxH = 34.0f * s;
    if (lx >= tBoxX && lx <= tBoxX + tBoxW && ly >= tBoxY && ly <= tBoxY + tBoxH) return 2;

    REAL dBoxX = 22.0f * s, dBoxY = 126.0f * s, dBoxW = w - 44.0f * s, dBoxH = 50.0f * s;
    if (lx >= dBoxX && lx <= dBoxX + dBoxW && ly >= dBoxY && ly <= dBoxY + dBoxH) return 3;

    REAL aCardX = 22.0f * s, aCardY = 186.0f * s, aCardW = w - 44.0f * s;
    REAL pillX = aCardX + 112.0f * s, pillY = aCardY + 10.0f * s, pillW = 42.0f * s, pillH = 22.0f * s;
    if (lx >= pillX && lx <= pillX + pillW && ly >= pillY && ly <= pillY + pillH) return 4;

    REAL allDayX = aCardX + aCardW - 54.0f * s, allDayY = aCardY + 10.0f * s, allDayW = 42.0f * s, allDayH = 22.0f * s;
    if (lx >= allDayX && lx <= allDayX + allDayW && ly >= allDayY && ly <= allDayY + allDayH) return 5;

    REAL row2Y = aCardY + 48.0f * s;
    if (!g_remState.isAllDay) {
        REAL hMinusX = aCardX + 102.0f * s, hY = row2Y + 2.0f * s, stepBtnW = 22.0f * s, stepBtnH = 26.0f * s;
        if (lx >= hMinusX && lx <= hMinusX + stepBtnW && ly >= hY && ly <= hY + stepBtnH) return 6;
        REAL hBoxX = hMinusX + stepBtnW + 3.0f * s, hBoxW = 32.0f * s;
        REAL hPlusX = hBoxX + hBoxW + 3.0f * s;
        if (lx >= hPlusX && lx <= hPlusX + stepBtnW && ly >= hY && ly <= hY + stepBtnH) return 7;

        REAL colonX = hPlusX + stepBtnW + 2.0f * s;
        REAL mMinusX = colonX + 12.0f * s;
        if (lx >= mMinusX && lx <= mMinusX + stepBtnW && ly >= hY && ly <= hY + stepBtnH) return 8;
        REAL mBoxX = mMinusX + stepBtnW + 3.0f * s, mBoxW = 32.0f * s;
        REAL mPlusX = mBoxX + mBoxW + 3.0f * s;
        if (lx >= mPlusX && lx <= mPlusX + stepBtnW && ly >= hY && ly <= hY + stepBtnH) return 9;

        REAL ampmX = mPlusX + stepBtnW + 8.0f * s, ampmW = 44.0f * s;
        if (lx >= ampmX && lx <= ampmX + ampmW && ly >= hY && ly <= hY + stepBtnH) return 10;

        REAL bellX = ampmX + ampmW + 6.0f * s, bellW = 28.0f * s;
        if (lx >= bellX && lx <= bellX + bellW && ly >= hY && ly <= hY + stepBtnH) return 14;
    }

    REAL btnH = 34.0f * s;
    REAL btnY = h - 16.0f * s - btnH;
    REAL saveW = 145.0f * s;
    REAL saveX = w - 22.0f * s - saveW;
    if (lx >= saveX && lx <= saveX + saveW && ly >= btnY && ly <= btnY + btnH) return 11;

    if (g_remState.isExisting) {
        REAL delW = 85.0f * s;
        REAL delX = 22.0f * s;
        if (lx >= delX && lx <= delX + delW && ly >= btnY && ly <= btnY + btnH) return 12;
    }

    REAL cancelW = 80.0f * s;
    REAL cancelX = (g_remState.isExisting ? 180.0f : 160.0f) * s;
    if (lx >= cancelX && lx <= cancelX + cancelW && ly >= btnY && ly <= btnY + btnH) return 13;

    return 0;
}

LRESULT CALLBACK ReminderDashboardWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR: {
        POINT pt; GetCursorPos(&pt);
        int lx = pt.x - g_remState.dlgX;
        int ly = pt.y - g_remState.dlgY;
        float s = g_dpiScale;
        REAL w = (REAL)g_remState.dlgW;
        REAL h = (REAL)g_remState.dlgH;
        int hit = HitTestReminderDashboard(lx, ly, s, w, h);
        if (hit == 2 || hit == 3) {
            SetCursor(LoadCursor(NULL, IDC_IBEAM));
            return TRUE;
        } else if (hit > 0) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }

    case WM_MOUSEMOVE: {
        POINT pt; GetCursorPos(&pt);
        int lx = pt.x - g_remState.dlgX;
        int ly = pt.y - g_remState.dlgY;
        float s = g_dpiScale;
        REAL w = (REAL)g_remState.dlgW;
        REAL h = (REAL)g_remState.dlgH;

        if (g_remState.isDragging) {
            g_remState.dlgX = pt.x - g_remState.dragStart.x;
            g_remState.dlgY = pt.y - g_remState.dragStart.y;
            RenderReminderDashboard(hWnd);
            break;
        }

        int hit = HitTestReminderDashboard(lx, ly, s, w, h);
        if (hit != g_remState.hoveredCtrl) {
            g_remState.hoveredCtrl = hit;
            RenderReminderDashboard(hWnd);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        POINT pt; GetCursorPos(&pt);
        int lx = pt.x - g_remState.dlgX;
        int ly = pt.y - g_remState.dlgY;
        float s = g_dpiScale;
        REAL w = (REAL)g_remState.dlgW;
        REAL h = (REAL)g_remState.dlgH;
        int hit = HitTestReminderDashboard(lx, ly, s, w, h);

        if (hit == 0) {
            g_remState.isDragging = true;
            SetCapture(hWnd);
            g_remState.dragStart.x = pt.x - g_remState.dlgX;
            g_remState.dragStart.y = pt.y - g_remState.dlgY;
        } else if (hit == 1) {
            CloseReminderDashboard();
        } else if (hit == 2) {
            g_remState.focusedField = 1;
            RenderReminderDashboard(hWnd);
        } else if (hit == 3) {
            g_remState.focusedField = 2;
            RenderReminderDashboard(hWnd);
        } else if (hit == 4) {
            g_remState.hasAlarm = !g_remState.hasAlarm;
            RenderReminderDashboard(hWnd);
        } else if (hit == 5) {
            g_remState.isAllDay = !g_remState.isAllDay;
            RenderReminderDashboard(hWnd);
        } else if (hit == 6) {
            g_remState.hour12 = (g_remState.hour12 <= 1 ? 12 : g_remState.hour12 - 1);
            RenderReminderDashboard(hWnd);
        } else if (hit == 7) {
            g_remState.hour12 = (g_remState.hour12 >= 12 ? 1 : g_remState.hour12 + 1);
            RenderReminderDashboard(hWnd);
        } else if (hit == 8) {
            g_remState.minute = (g_remState.minute < 5 ? 55 : (g_remState.minute / 5 - 1) * 5);
            RenderReminderDashboard(hWnd);
        } else if (hit == 9) {
            g_remState.minute = (g_remState.minute >= 55 ? 0 : (g_remState.minute / 5 + 1) * 5);
            RenderReminderDashboard(hWnd);
        } else if (hit == 10) {
            g_remState.isPM = !g_remState.isPM;
            RenderReminderDashboard(hWnd);
        } else if (hit == 14) {
            Beep(784, 120); Sleep(30); Beep(880, 120); Sleep(30); Beep(1046, 200);
            PlaySoundW(L"SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC);
        } else if (hit == 12) {
            DeleteReminderForBS(g_remState.bsYear, g_remState.bsMonth, g_remState.bsDay);
            CloseReminderDashboard();
            if (g_hCalWnd) RenderCalendar(g_hCalWnd);
        } else if (hit == 13) {
            CloseReminderDashboard();
        } else if (hit == 11) {
            CustomReminder rem;
            rem.id = (int)time(NULL);
            rem.bsYear = g_remState.bsYear;
            rem.bsMonth = g_remState.bsMonth;
            rem.bsDay = g_remState.bsDay;
            rem.title = g_remState.title.empty() ? L"Custom Event" : g_remState.title;
            rem.desc = g_remState.desc;
            rem.hasAlarm = g_remState.hasAlarm;
            rem.isAllDay = g_remState.isAllDay;
            int h24 = g_remState.hour12 % 12;
            if (g_remState.isPM) h24 += 12;
            rem.alarmHour = h24;
            rem.alarmMinute = g_remState.minute;
            rem.isTriggered = false;
            AddOrUpdateReminder(rem);
            CloseReminderDashboard();
            if (g_hCalWnd) RenderCalendar(g_hCalWnd);
        }
        break;
    }

    case WM_LBUTTONUP:
        if (g_remState.isDragging) {
            g_remState.isDragging = false;
            ReleaseCapture();
        }
        break;

    case WM_CHAR: {
        wchar_t ch = (wchar_t)wParam;
        if (ch >= 32) {
            if (g_remState.focusedField == 1 && g_remState.title.length() < 40) {
                g_remState.title += ch;
                RenderReminderDashboard(hWnd);
            } else if (g_remState.focusedField == 2 && g_remState.desc.length() < 90) {
                g_remState.desc += ch;
                RenderReminderDashboard(hWnd);
            }
        } else if (ch == 8) { // Backspace
            if (g_remState.focusedField == 1 && !g_remState.title.empty()) {
                g_remState.title.pop_back();
                RenderReminderDashboard(hWnd);
            } else if (g_remState.focusedField == 2 && !g_remState.desc.empty()) {
                g_remState.desc.pop_back();
                RenderReminderDashboard(hWnd);
            }
        } else if (ch == 13) { // Enter
            if (g_remState.focusedField == 1) {
                g_remState.focusedField = 2;
                RenderReminderDashboard(hWnd);
            } else {
                CustomReminder rem;
                rem.id = (int)time(NULL);
                rem.bsYear = g_remState.bsYear;
                rem.bsMonth = g_remState.bsMonth;
                rem.bsDay = g_remState.bsDay;
                rem.title = g_remState.title.empty() ? L"Custom Event" : g_remState.title;
                rem.desc = g_remState.desc;
                rem.hasAlarm = g_remState.hasAlarm;
                rem.isAllDay = g_remState.isAllDay;
                int h24 = g_remState.hour12 % 12;
                if (g_remState.isPM) h24 += 12;
                rem.alarmHour = h24;
                rem.alarmMinute = g_remState.minute;
                rem.isTriggered = false;
                AddOrUpdateReminder(rem);
                CloseReminderDashboard();
                if (g_hCalWnd) RenderCalendar(g_hCalWnd);
            }
        } else if (ch == 9) { // Tab
            g_remState.focusedField = (g_remState.focusedField == 1) ? 2 : 1;
            RenderReminderDashboard(hWnd);
        } else if (ch == 22) { // Ctrl+V
            if (OpenClipboard(hWnd)) {
                HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                if (hData) {
                    wchar_t* pText = (wchar_t*)GlobalLock(hData);
                    if (pText) {
                        if (g_remState.focusedField == 1) {
                            for (int i = 0; pText[i] && g_remState.title.length() < 40 && pText[i] != L'\r' && pText[i] != L'\n'; i++) {
                                g_remState.title += pText[i];
                            }
                        } else if (g_remState.focusedField == 2) {
                            for (int i = 0; pText[i] && g_remState.desc.length() < 90 && pText[i] != L'\r' && pText[i] != L'\n'; i++) {
                                g_remState.desc += pText[i];
                            }
                        }
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
                RenderReminderDashboard(hWnd);
            }
        }
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            CloseReminderDashboard();
        }
        break;

    case WM_TIMER:
        if (wParam == 201) {
            g_remState.cursorBlink = !g_remState.cursorBlink;
            RenderReminderDashboard(hWnd);
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            // Keep window accessible
        }
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowReminderDashboard(HWND hParent, int bsYear, int bsMonth, int bsDay) {
    CloseCalendarDatePopup();
    HINSTANCE hInst = GetModuleHandle(NULL);
    static bool s_remClassRegistered = false;
    if (!s_remClassRegistered) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = ReminderDashboardWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = L"NepaliReminderDashboardClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassExW(&wc);
        s_remClassRegistered = true;
    }

    float s = g_dpiScale;
    g_remState.dlgW = (int)(430 * s);
    g_remState.dlgH = (int)(385 * s);
    g_remState.bsYear = bsYear;
    g_remState.bsMonth = bsMonth;
    g_remState.bsDay = bsDay;

    CustomReminder* existing = FindReminderForBS(bsYear, bsMonth, bsDay);
    if (existing) {
        g_remState.isExisting = true;
        g_remState.title = existing->title;
        g_remState.desc = existing->desc;
        g_remState.hasAlarm = existing->hasAlarm;
        g_remState.isAllDay = existing->isAllDay;
        int h = existing->alarmHour;
        g_remState.isPM = (h >= 12);
        int h12 = h % 12;
        if (h12 == 0) h12 = 12;
        g_remState.hour12 = h12;
        g_remState.minute = existing->alarmMinute;
    } else {
        g_remState.isExisting = false;
        g_remState.title = L"";
        g_remState.desc = L"";
        g_remState.hasAlarm = true;
        g_remState.isAllDay = false;
        g_remState.hour12 = 9;
        g_remState.minute = 0;
        g_remState.isPM = false;
    }
    g_remState.focusedField = 1;
    g_remState.cursorBlink = true;
    g_remState.hoveredCtrl = -1;
    g_remState.isDragging = false;

    HMONITOR hMon = MonitorFromWindow(hParent ? hParent : g_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    RECT rcParent;
    if (hParent && IsWindow(hParent)) {
        GetWindowRect(hParent, &rcParent);
        g_remState.dlgX = rcParent.left + (rcParent.right - rcParent.left - g_remState.dlgW) / 2;
        g_remState.dlgY = rcParent.top + (rcParent.bottom - rcParent.top - g_remState.dlgH) / 2;
    } else {
        g_remState.dlgX = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - g_remState.dlgW) / 2;
        g_remState.dlgY = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - g_remState.dlgH) / 2;
    }

    if (g_remState.dlgX < mi.rcWork.left + 10) g_remState.dlgX = mi.rcWork.left + 10;
    if (g_remState.dlgX + g_remState.dlgW > mi.rcWork.right - 10) g_remState.dlgX = mi.rcWork.right - g_remState.dlgW - 10;
    if (g_remState.dlgY < mi.rcWork.top + 10) g_remState.dlgY = mi.rcWork.top + 10;
    if (g_remState.dlgY + g_remState.dlgH > mi.rcWork.bottom - 10) g_remState.dlgY = mi.rcWork.bottom - g_remState.dlgH - 10;

    if (!g_hReminderWnd || !IsWindow(g_hReminderWnd)) {
        g_hReminderWnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            L"NepaliReminderDashboardClass",
            L"Reminder Dashboard",
            WS_POPUP,
            g_remState.dlgX, g_remState.dlgY, g_remState.dlgW, g_remState.dlgH,
            NULL, NULL, hInst, NULL
        );
    } else {
        SetWindowPos(g_hReminderWnd, HWND_TOPMOST, g_remState.dlgX, g_remState.dlgY, g_remState.dlgW, g_remState.dlgH, SWP_NOACTIVATE);
    }

    g_isReminderDashboardOpen = true;
    SetTimer(g_hReminderWnd, 201, 500, NULL);
    RenderReminderDashboard(g_hReminderWnd);
    ShowWindow(g_hReminderWnd, SW_SHOWNOACTIVATE);
    SetForegroundWindow(g_hReminderWnd);
    SetFocus(g_hReminderWnd);
}

void CloseReminderDashboard() {
    if (g_hReminderWnd && IsWindow(g_hReminderWnd)) {
        KillTimer(g_hReminderWnd, 201);
        ShowWindow(g_hReminderWnd, SW_HIDE);
    }
    g_isReminderDashboardOpen = false;
}

// ── Calendar Date Right-Click Action Popup (Custom Fluent Tithify Theme) ─────
HWND g_hDatePopupWnd = NULL;
bool g_isDatePopupOpen = false;

struct DatePopupState {
    int bsYear = 0;
    int bsMonth = 0;
    int bsDay = 0;
    int posX = 0;
    int posY = 0;
    int width = 0;
    int height = 0;
    int hoveredBtn = 0; // 0=none, 1=primary (Set/Edit), 2=secondary (Delete), 99=close
    bool hasReminder = false;
} g_datePopupState;

void CloseCalendarDatePopup() {
    if (g_hDatePopupWnd && IsWindow(g_hDatePopupWnd)) {
        ShowWindow(g_hDatePopupWnd, SW_HIDE);
    }
    g_isDatePopupOpen = false;
}

void RenderDatePopup(HWND hWnd) {
    if (!hWnd) return;

    int rawW = g_datePopupState.width;
    int rawH = g_datePopupState.height;
    float s = g_dpiScale;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rawW;
    bmi.bmiHeader.biHeight = -rawH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    REAL w = (REAL)rawW;
    REAL h = (REAL)rawH;

    // Card background
    GraphicsPath cardPath;
    AddRoundedRectangle(cardPath, 0.5f, 0.5f, w - 1.0f, h - 1.0f, 10.0f * s);

    SolidBrush bgBrush(g_isLightTheme ? Color(252, 255, 255, 255) : Color(248, 25, 25, 30));
    graphics.FillPath(&bgBrush, &cardPath);

    // Tithify brand crimson accent border
    Pen borderPen(Color(180, 220, 38, 38), 1.2f * s);
    graphics.DrawPath(&borderPen, &cardPath);

    FontFamily fontFamily(L"Segoe UI");
    FontFamily fontMDL2(L"Segoe MDL2 Assets");
    Font headerFont(&fontFamily, 11.5f * s, FontStyleBold, UnitPixel);
    Font btnFont(&fontFamily, 11.5f * s, FontStyleBold, UnitPixel);
    Font iconFont(&fontMDL2, 10.5f * s, FontStyleRegular, UnitPixel);

    StringFormat formatNear;
    formatNear.SetAlignment(StringAlignmentNear);
    formatNear.SetLineAlignment(StringAlignmentCenter);
    StringFormat formatCenter;
    formatCenter.SetAlignment(StringAlignmentCenter);
    formatCenter.SetLineAlignment(StringAlignmentCenter);

    // Top Header: Date icon & title
    int mIdx = g_datePopupState.bsMonth - 1;
    if (mIdx < 0) mIdx = 0; if (mIdx > 11) mIdx = 11;
    std::wstring headerText = std::wstring(kNepaliMonthNamesEN[mIdx]) + L" " +
                              std::to_wstring(g_datePopupState.bsDay) + L", " +
                              std::to_wstring(g_datePopupState.bsYear) + L" (" +
                              ToDevanagariNum(g_datePopupState.bsDay) + L" " +
                              kNepaliMonthNamesNP[mIdx] + L")";

    SolidBrush textPrimary(g_isLightTheme ? Color(255, 25, 25, 30) : Color(255, 240, 240, 245));
    SolidBrush crimsonBrush(Color(255, 220, 38, 38));

    // Bell / alarm icon on header
    RectF iconRect(12.0f * s, 6.0f * s, 16.0f * s, 22.0f * s);
    graphics.DrawString(L"\uE7ED", -1, &iconFont, iconRect, &formatCenter, &crimsonBrush);

    RectF titleRect(32.0f * s, 6.0f * s, w - 60.0f * s, 22.0f * s);
    graphics.DrawString(headerText.c_str(), -1, &headerFont, titleRect, &formatNear, &textPrimary);

    // Close button '✕'
    RectF closeRect(w - 26.0f * s, 6.0f * s, 20.0f * s, 20.0f * s);
    SolidBrush closeBrush(g_datePopupState.hoveredBtn == 99 ? Color(255, 220, 38, 38) : (g_isLightTheme ? Color(130, 0, 0, 0) : Color(150, 255, 255, 255)));
    graphics.DrawString(L"\u2715", -1, &btnFont, closeRect, &formatCenter, &closeBrush);

    // Primary Action Button: "⏰ Set Reminder / Alarm" or "⏰ Edit Reminder / Alarm"
    REAL btnX = 12.0f * s;
    REAL btnY = 32.0f * s;
    REAL btnW = w - 24.0f * s;
    REAL btnH = 36.0f * s;
    RectF primRect(btnX, btnY, btnW, btnH);

    GraphicsPath primPath;
    AddRoundedRectangle(primPath, btnX, btnY, btnW, btnH, 7.0f * s);

    bool isPrimHovered = (g_datePopupState.hoveredBtn == 1);
    SolidBrush primBg(isPrimHovered ? Color(255, 242, 60, 60) : Color(255, 220, 38, 38));
    graphics.FillPath(&primBg, &primPath);

    Pen primBorder(isPrimHovered ? Color(255, 255, 110, 110) : Color(255, 195, 25, 25), 1.0f * s);
    graphics.DrawPath(&primBorder, &primPath);

    SolidBrush primText(Color(255, 255, 255, 255));
    const wchar_t* primLabel = g_datePopupState.hasReminder ? L"\u23F0  Edit Reminder / Alarm" : L"\u23F0  Set Reminder / Alarm";
    graphics.DrawString(primLabel, -1, &btnFont, primRect, &formatCenter, &primText);

    // Secondary Action Button (Delete) if reminder exists
    if (g_datePopupState.hasReminder) {
        REAL delY = 74.0f * s;
        REAL delH = 32.0f * s;
        RectF delRect(btnX, delY, btnW, delH);

        GraphicsPath delPath;
        AddRoundedRectangle(delPath, btnX, delY, btnW, delH, 6.0f * s);

        bool isDelHovered = (g_datePopupState.hoveredBtn == 2);
        if (isDelHovered) {
            SolidBrush delBg(Color(255, 220, 38, 38));
            graphics.FillPath(&delBg, &delPath);
            SolidBrush delText(Color(255, 255, 255, 255));
            graphics.DrawString(L"\u2715  Delete Reminder", -1, &btnFont, delRect, &formatCenter, &delText);
        } else {
            SolidBrush delBg(Color(20, 220, 38, 38));
            graphics.FillPath(&delBg, &delPath);
            Pen delBorder(Color(120, 220, 38, 38), 1.0f * s);
            graphics.DrawPath(&delBorder, &delPath);
            SolidBrush delText(Color(255, 220, 38, 38));
            graphics.DrawString(L"\u2715  Delete Reminder", -1, &btnFont, delRect, &formatCenter, &delText);
        }
    }

    POINT ptDst = { g_datePopupState.posX, g_datePopupState.posY };
    SIZE sizeDst = { rawW, rawH };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeDst, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

int HitTestDatePopup(int lx, int ly, float s, float w, float h) {
    if (lx >= w - 28.0f * s && lx <= w - 4.0f * s && ly >= 4.0f * s && ly <= 28.0f * s) return 99;
    if (lx >= 12.0f * s && lx <= w - 12.0f * s && ly >= 32.0f * s && ly <= 68.0f * s) return 1;
    if (g_datePopupState.hasReminder && lx >= 12.0f * s && lx <= w - 12.0f * s && ly >= 74.0f * s && ly <= 106.0f * s) return 2;
    return 0;
}

LRESULT CALLBACK DatePopupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR: {
        POINT pt; GetCursorPos(&pt);
        int lx = pt.x - g_datePopupState.posX;
        int ly = pt.y - g_datePopupState.posY;
        float s = g_dpiScale;
        int hit = HitTestDatePopup(lx, ly, s, (float)g_datePopupState.width, (float)g_datePopupState.height);
        if (hit > 0) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }
    case WM_MOUSEMOVE: {
        POINT pt; GetCursorPos(&pt);
        int lx = pt.x - g_datePopupState.posX;
        int ly = pt.y - g_datePopupState.posY;
        float s = g_dpiScale;
        int hit = HitTestDatePopup(lx, ly, s, (float)g_datePopupState.width, (float)g_datePopupState.height);
        if (hit != g_datePopupState.hoveredBtn) {
            g_datePopupState.hoveredBtn = hit;
            RenderDatePopup(hWnd);
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        POINT pt; GetCursorPos(&pt);
        int lx = pt.x - g_datePopupState.posX;
        int ly = pt.y - g_datePopupState.posY;
        float s = g_dpiScale;
        int hit = HitTestDatePopup(lx, ly, s, (float)g_datePopupState.width, (float)g_datePopupState.height);
        if (hit == 1) {
            int y = g_datePopupState.bsYear;
            int m = g_datePopupState.bsMonth;
            int d = g_datePopupState.bsDay;
            CloseCalendarDatePopup();
            ShowReminderDashboard(g_hCalWnd, y, m, d);
        } else if (hit == 2) {
            int y = g_datePopupState.bsYear;
            int m = g_datePopupState.bsMonth;
            int d = g_datePopupState.bsDay;
            DeleteReminderForBS(y, m, d);
            CloseCalendarDatePopup();
            if (g_hCalWnd) RenderCalendar(g_hCalWnd);
        } else if (hit == 99) {
            CloseCalendarDatePopup();
        }
        break;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            HWND hOther = (HWND)lParam;
            if (hOther != g_hCalWnd && hOther != g_hReminderWnd && hOther != g_hAlarmAlertWnd) {
                CloseCalendarDatePopup();
            }
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            CloseCalendarDatePopup();
        }
        break;
    case WM_DESTROY:
        g_hDatePopupWnd = NULL;
        g_isDatePopupOpen = false;
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowCalendarDatePopup(HWND hWndParent, POINT pt, int bsYear, int bsMonth, int bsDay) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    static bool s_datePopupRegistered = false;
    if (!s_datePopupRegistered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = DatePopupWndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"TithifyDatePopupClass";
        RegisterClassExW(&wc);
        s_datePopupRegistered = true;
    }

    float s = g_dpiScale;
    bool hasReminder = HasReminderForBS(bsYear, bsMonth, bsDay);

    int popW = (int)(238.0f * s);
    int popH = (int)((hasReminder ? 116.0f : 78.0f) * s);

    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    int gapX = (int)(16.0f * s);
    int gapY = (int)(20.0f * s);

    int posX = pt.x + gapX;
    if (posX + popW > mi.rcWork.right - 10) {
        posX = pt.x - popW - gapX;
    }
    if (posX < mi.rcWork.left + 10) posX = mi.rcWork.left + 10;
    if (posX + popW > mi.rcWork.right - 10) posX = mi.rcWork.right - popW - 10;

    int posY = pt.y + gapY;
    if (posY + popH > mi.rcWork.bottom - 10) {
        posY = pt.y - popH - gapY;
    }
    if (posY < mi.rcWork.top + 10) posY = mi.rcWork.top + 10;

    g_datePopupState.bsYear = bsYear;
    g_datePopupState.bsMonth = bsMonth;
    g_datePopupState.bsDay = bsDay;
    g_datePopupState.hasReminder = hasReminder;
    g_datePopupState.posX = posX;
    g_datePopupState.posY = posY;
    g_datePopupState.width = popW;
    g_datePopupState.height = popH;
    g_datePopupState.hoveredBtn = 0;

    if (!g_hDatePopupWnd || !IsWindow(g_hDatePopupWnd)) {
        g_hDatePopupWnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            L"TithifyDatePopupClass",
            L"Tithify Date Action",
            WS_POPUP,
            posX, posY, popW, popH,
            hWndParent, NULL, hInst, NULL
        );
    } else {
        SetWindowPos(g_hDatePopupWnd, HWND_TOPMOST, posX, posY, popW, popH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    g_isDatePopupOpen = true;
    RenderDatePopup(g_hDatePopupWnd);
    ShowWindow(g_hDatePopupWnd, SW_SHOWNOACTIVATE);
    SetWindowPos(g_hDatePopupWnd, HWND_TOPMOST, posX, posY, popW, popH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

// ── Calendar Popup State & Engine ────────────────────────────────────────────
HWND g_hCalWnd = NULL;
bool g_isCalendarOpen = false;
DWORD g_lastCalCloseTime = 0;
int g_calBaseX = 0, g_calBaseY = 0;
bool g_flyoutOnLeft = false;
int g_calYear = 2082;
int g_calMonth = 11;
int g_calSelectedDay = 9;
int g_hoveredBtn = -1;
int g_hoveredCell = -1;
bool g_hasDragged = false;

RECT g_rcCalCard = { 0, 0, 0, 0 };
RECT g_rcCalEventCard = { 0, 0, 0, 0 };
bool g_calHasEvent = false;

bool IsPointInCalendar(POINT pt) {
    if (g_isDatePopupOpen && g_hDatePopupWnd && IsWindowVisible(g_hDatePopupWnd)) {
        RECT rc;
        GetWindowRect(g_hDatePopupWnd, &rc);
        if (PtInRect(&rc, pt)) return true;
    }
    if (g_isReminderDashboardOpen && g_hReminderWnd && IsWindowVisible(g_hReminderWnd)) {
        RECT rc;
        GetWindowRect(g_hReminderWnd, &rc);
        if (PtInRect(&rc, pt)) return true;
    }
    if (g_isAlarmAlertOpen && g_hAlarmAlertWnd && IsWindowVisible(g_hAlarmAlertWnd)) {
        RECT rc;
        GetWindowRect(g_hAlarmAlertWnd, &rc);
        if (PtInRect(&rc, pt)) return true;
    }
    if (!g_isCalendarOpen) return false;
    if (PtInRect(&g_rcCalCard, pt)) return true;
    if (g_calHasEvent && PtInRect(&g_rcCalEventCard, pt)) return true;
    return false;
}

enum CalendarButtonId {
    BTN_NONE = -1,
    BTN_PREV_MONTH = 102,
    BTN_NEXT_MONTH = 103,
    BTN_TODAY = 105,
    BTN_CLOSE = 106,
    BTN_RETRY_FETCH = 107,
    BTN_SET_REMINDER = 108,
    BTN_DELETE_REMINDER = 109
};

enum CalendarAnimState {
    CAL_ANIM_CLOSED = 0,
    CAL_ANIM_OPENING,
    CAL_ANIM_OPEN,
    CAL_ANIM_CLOSING
};

CalendarAnimState g_calAnimState = CAL_ANIM_CLOSED;
DWORD g_calAnimStartTime = 0;
const DWORD CAL_ANIM_OPEN_MS = 180;
const DWORD CAL_ANIM_CLOSE_MS = 140;

void HideCalendar();
void HideCalendarImmediate();
void ShowCalendar(HWND hWidgetWnd);
void ToggleCalendar(HWND hWidgetWnd);

void RenderCalendar(HWND hWnd, BYTE alpha, int overrideX, int overrideY) {
    if (!hWnd) return;

    // Enforce calendar strictly to current BS year
    int curBSY = 0, curBSM = 0, curBSD = 0, curBSDOW = 0;
    GetCurrentBSDate(curBSY, curBSM, curBSD, curBSDOW);
    g_calYear = curBSY;

    if (g_calMonth < 1) g_calMonth = 1;
    if (g_calMonth > 12) g_calMonth = 12;

    int totalD = GetBSDaysInMonth(g_calYear, g_calMonth);
    if (g_calSelectedDay < 1) g_calSelectedDay = 1;
    if (g_calSelectedDay > totalD) g_calSelectedDay = totalD;

    EnsureHolidaysLoadedForCurrentYear(g_calYear);

    const NepaliHoliday* selHoliday = GetHolidayForBS(g_calYear, g_calMonth, g_calSelectedDay);
    CustomReminder* selReminder = FindReminderForBS(g_calYear, g_calMonth, g_calSelectedDay);
    bool isOfflineMode = (g_holidayFetchState == FETCH_ERROR_OFFLINE && g_currentYearHolidays.empty());
    bool isDownloading = (g_holidayFetchState == FETCH_DOWNLOADING && g_currentYearHolidays.empty());
    bool hasEvent = (selHoliday != NULL || selReminder != NULL || isOfflineMode || isDownloading);

    HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    int baseW = 420;
    int baseH = 468;
    float s = g_dpiScale;

    float flyoutW = 380.0f;
    float flyoutH = 290.0f;
    float arrowW = 12.0f;
    float flyoutTotalW = (flyoutW + (isOfflineMode || isDownloading ? 12.0f : arrowW)) * s;

    // Determine if flyout fits on right or should flip to left side
    bool isFlyoutOnLeft = false;
    if (hasEvent) {
        if (g_calBaseX + (int)(baseW * s) + (int)flyoutTotalW > mi.rcWork.right) {
            isFlyoutOnLeft = true;
        }
    }
    g_flyoutOnLeft = isFlyoutOnLeft;

    int rawW = (int)((hasEvent ? (baseW + arrowW + flyoutW + 12.0f) : baseW) * s);
    int rawH = (int)(baseH * s);

    int baseX = (overrideX != -1) ? overrideX : g_calBaseX;
    int baseY = (overrideY != -1) ? overrideY : g_calBaseY;

    int screenX = baseX;
    float calCardOffsetX = 0.0f;
    if (hasEvent && isFlyoutOnLeft) {
        screenX = baseX - (int)flyoutTotalW;
        if (screenX < mi.rcWork.left) screenX = mi.rcWork.left;
        calCardOffsetX = (float)(baseX - screenX);
    }
    int screenY = baseY;

    g_rcCalCard.left = screenX + (int)calCardOffsetX;
    g_rcCalCard.top = screenY;
    g_rcCalCard.right = g_rcCalCard.left + (int)(baseW * s);
    g_rcCalCard.bottom = g_rcCalCard.top + (int)(baseH * s);
    g_calHasEvent = hasEvent;
    if (!hasEvent) {
        g_rcCalEventCard = { 0, 0, 0, 0 };
    }

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rawW;
    bmi.bmiHeader.biHeight = -rawH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    graphics.Clear(Color(0, 0, 0, 0));

    FontFamily fontFamily(L"Segoe UI");
    Font fontTitle(&fontFamily, 14.0f * s, FontStyleBold, UnitPixel);
    Font fontSubTitle(&fontFamily, 11.0f * s, FontStyleRegular, UnitPixel);
    Font fontButton(&fontFamily, 12.0f * s, FontStyleBold, UnitPixel);
    Font fontNavTitle(&fontFamily, 15.5f * s, FontStyleBold, UnitPixel);
    Font fontNavSub(&fontFamily, 11.5f * s, FontStyleRegular, UnitPixel);
    Font fontDayHeader(&fontFamily, 12.5f * s, FontStyleBold, UnitPixel);
    Font fontBSDay(&fontFamily, 16.5f * s, FontStyleBold, UnitPixel);
    Font fontADDay(&fontFamily, 11.0f * s, FontStyleRegular, UnitPixel);
    Font fontDetailL1(&fontFamily, 16.5f * s, FontStyleBold, UnitPixel);
    Font fontDetailL2(&fontFamily, 13.0f * s, FontStyleBold, UnitPixel);
    Font fontDetailL3(&fontFamily, 12.5f * s, FontStyleRegular, UnitPixel);
    Font fontBadge(&fontFamily, 10.0f * s, FontStyleBold, UnitPixel);

    StringFormat formatNear, formatCenter, formatFar;
    formatNear.SetAlignment(StringAlignmentNear);
    formatNear.SetLineAlignment(StringAlignmentCenter);
    formatCenter.SetAlignment(StringAlignmentCenter);
    formatCenter.SetLineAlignment(StringAlignmentCenter);
    formatFar.SetAlignment(StringAlignmentFar);
    formatFar.SetLineAlignment(StringAlignmentCenter);

    // ── 1. Render Calendar Base Card ─────────────────────────────────────────
    graphics.TranslateTransform(calCardOffsetX, 0.0f);

    GraphicsPath cardPath;
    AddRoundedRectangle(cardPath, 1.0f * s, 1.0f * s, (baseW - 2.0f) * s, (baseH - 2.0f) * s, 14.0f * s);
    
    if (g_isLightTheme) {
        SolidBrush cardBrush(Color(255, 255, 255, 255));
        graphics.FillPath(&cardBrush, &cardPath);
        Pen cardPen(Color(50, 0, 0, 0), 1.0f * s);
        graphics.DrawPath(&cardPen, &cardPath);
    } else {
        SolidBrush cardBrush(Color(255, 30, 30, 36));
        graphics.FillPath(&cardBrush, &cardPath);
        Pen cardPen(Color(65, 255, 255, 255), 1.0f * s);
        graphics.DrawPath(&cardPen, &cardPath);
    }

    // Top Header (Flag + Title + Today Button + Close Button)
    DrawNepaliFlag(graphics, 18.0f * s, 16.0f * s, 26.0f * s);

    SolidBrush textPrimary(g_isLightTheme ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255));
    SolidBrush textSubtle(g_isLightTheme ? Color(180, 100, 100, 100) : Color(180, 170, 170, 175));
    SolidBrush crimsonText(Color(255, 235, 35, 65));

    RectF titleRect(48.0f * s, 14.0f * s, 240.0f * s, 18.0f * s);
    graphics.DrawString(L"Nepali Calendar", -1, &fontTitle, titleRect, &formatNear, &textPrimary);

    RectF subTitleRect(48.0f * s, 33.0f * s, 240.0f * s, 16.0f * s);
    graphics.DrawString(L"\u0928\u0947\u092A\u093E\u0932\u0940 \u092A\u093E\u0924\u094D\u0930\u094B \u2022 Bikram Sambat", -1, &fontSubTitle, subTitleRect, &formatNear, &textSubtle);

    RectF todayBtnRect(300.0f * s, 14.0f * s, 68.0f * s, 30.0f * s);
    DrawIconButton(graphics, todayBtnRect, L"Today", g_hoveredBtn == BTN_TODAY, g_isLightTheme, fontButton);

    RectF closeBtnRect(374.0f * s, 14.0f * s, 30.0f * s, 30.0f * s);
    DrawIconButton(graphics, closeBtnRect, L"\u2715", g_hoveredBtn == BTN_CLOSE, g_isLightTheme, fontButton);

    // Navigation Bar (< Prev Month, Month Year, Next Month >)
    RectF prevMonthRect(18.0f * s, 60.0f * s, 36.0f * s, 34.0f * s);
    DrawIconButton(graphics, prevMonthRect, L"\u2039", g_hoveredBtn == BTN_PREV_MONTH, g_isLightTheme, fontButton);

    RectF nextMonthRect(366.0f * s, 60.0f * s, 36.0f * s, 34.0f * s);
    DrawIconButton(graphics, nextMonthRect, L"\u203A", g_hoveredBtn == BTN_NEXT_MONTH, g_isLightTheme, fontButton);

    int mIdx = g_calMonth - 1;
    std::wstring navTitle = std::wstring(kNepaliMonthNamesEN[mIdx]) + L" " + std::to_wstring(g_calYear) +
                           L"  \u2022  " + kNepaliMonthNamesNP[mIdx] + L" " + ToDevanagariNum(g_calYear);
    RectF navTitleRect(58.0f * s, 58.0f * s, 304.0f * s, 22.0f * s);
    graphics.DrawString(navTitle.c_str(), -1, &fontNavTitle, navTitleRect, &formatCenter, &textPrimary);

    int adY1 = 0, adM1 = 0, adD1 = 0, dow1 = 0;
    int adY2 = 0, adM2 = 0, adD2 = 0, dow2 = 0;
    BSToAD(g_calYear, g_calMonth, 1, adY1, adM1, adD1, dow1);
    BSToAD(g_calYear, g_calMonth, totalD, adY2, adM2, adD2, dow2);

    std::wstring adRangeStr;
    if (adM1 == adM2) {
        adRangeStr = std::wstring(kEnglishMonthNames[adM1 - 1]) + L" " + std::to_wstring(adD1) +
                     L" - " + std::to_wstring(adD2) + L", " + std::to_wstring(adY1);
    } else {
        adRangeStr = std::wstring(kEnglishMonthNames[adM1 - 1]) + L" " + std::to_wstring(adD1) +
                     L" - " + kEnglishMonthNames[adM2 - 1] + L" " + std::to_wstring(adD2) +
                     L", " + std::to_wstring(adY2);
    }
    RectF navSubRect(58.0f * s, 80.0f * s, 304.0f * s, 16.0f * s);
    graphics.DrawString(adRangeStr.c_str(), -1, &fontNavSub, navSubRect, &formatCenter, &textSubtle);

    // Days of Week Header Row
    const wchar_t* dowNames[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
    float colW = 55.0f * s;
    float rowH = 48.0f * s;
    float gridLeft = 18.0f * s;
    float gridTop = 104.0f * s;

    for (int i = 0; i < 7; i++) {
        RectF headerRect(gridLeft + i * colW, gridTop, colW, 26.0f * s);
        SolidBrush dowBrush(i == 6 ? Color(255, 235, 35, 65) : (i == 0 ? (g_isLightTheme ? Color(255, 200, 30, 30) : Color(255, 255, 100, 100)) : (g_isLightTheme ? Color(255, 100, 100, 100) : Color(255, 170, 170, 175))));
        graphics.DrawString(dowNames[i], -1, &fontDayHeader, headerRect, &formatCenter, &dowBrush);
    }

    // Month Days Grid (6 Rows x 7 Columns)
    int startDow = GetBSMonthStartDayOfWeek(g_calYear, g_calMonth);
    float cellsTop = 138.0f * s;

    float selectedCellCenterY = cellsTop + 24.0f * s;

    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            int cellIdx = r * 7 + c;
            int dayNum = cellIdx - startDow + 1;

            if (dayNum >= 1 && dayNum <= totalD) {
                float cellX = gridLeft + c * colW;
                float cellY = cellsTop + r * rowH;
                RectF cellRect(cellX + 2.0f * s, cellY + 2.0f * s, colW - 4.0f * s, rowH - 4.0f * s);

                bool isToday = (g_calYear == curBSY && g_calMonth == curBSM && dayNum == curBSD);
                bool isSelected = (dayNum == g_calSelectedDay);
                bool isHovered = (g_hoveredCell == cellIdx);
                bool isSaturday = (c == 6);

                const NepaliHoliday* hInfo = GetHolidayForBS(g_calYear, g_calMonth, dayNum);
                bool isHoliday = (hInfo != NULL && hInfo->isPublicHoliday);
                bool isFestival = (hInfo != NULL && !hInfo->isPublicHoliday);

                if (isSelected) {
                    selectedCellCenterY = cellY + (rowH / 2.0f);
                }

                // Cell Background Styling
                GraphicsPath cellPath;
                AddRoundedRectangle(cellPath, cellRect.X, cellRect.Y, cellRect.Width, cellRect.Height, 8.0f * s);

                if (isToday) {
                    SolidBrush todayBg(Color(255, 220, 20, 60));
                    graphics.FillPath(&todayBg, &cellPath);
                } else if (isSelected) {
                    SolidBrush selBg(g_isLightTheme ? Color(40, 0, 56, 147) : Color(50, 255, 255, 255));
                    graphics.FillPath(&selBg, &cellPath);
                    Pen selPen(g_isLightTheme ? Color(180, 0, 56, 147) : Color(180, 255, 255, 255), 1.5f * s);
                    graphics.DrawPath(&selPen, &cellPath);
                } else if (isHovered) {
                    SolidBrush hovBg(g_isLightTheme ? Color(25, 0, 0, 0) : Color(30, 255, 255, 255));
                    graphics.FillPath(&hovBg, &cellPath);
                }

                // BS Day Number
                std::wstring bsDayStr = std::to_wstring(dayNum);
                RectF bsDayRect(cellRect.X, cellRect.Y + 4.0f * s, cellRect.Width, 22.0f * s);

                SolidBrush bsDayBrush(isToday ? Color(255, 255, 255, 255) :
                                      (isHoliday || isSaturday ? Color(255, 235, 35, 65) :
                                      (g_isLightTheme ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255))));

                graphics.DrawString(bsDayStr.c_str(), -1, &fontBSDay, bsDayRect, &formatCenter, &bsDayBrush);

                // English AD Day Sub-Number
                int cAdY = 0, cAdM = 0, cAdD = 0, cDow = 0;
                BSToAD(g_calYear, g_calMonth, dayNum, cAdY, cAdM, cAdD, cDow);
                std::wstring adDayStr = std::to_wstring(cAdD);
                RectF adDayRect(cellRect.X, cellRect.Y + 26.0f * s, cellRect.Width, 14.0f * s);

                SolidBrush adDayBrush(isToday ? Color(220, 255, 255, 255) :
                                      (g_isLightTheme ? Color(160, 100, 100, 100) : Color(160, 170, 170, 175)));

                graphics.DrawString(adDayStr.c_str(), -1, &fontADDay, adDayRect, &formatCenter, &adDayBrush);

                // Indicator dot for Holidays/Events
                if (isHoliday || isFestival) {
                    float dotDiam = 4.5f * s;
                    float dotX = cellRect.X + (cellRect.Width - dotDiam) / 2.0f;
                    float dotY = cellRect.Y + cellRect.Height - dotDiam - 2.0f * s;

                    SolidBrush dotBrush(isToday ? Color(255, 255, 255, 255) :
                                        (isHoliday ? Color(255, 235, 35, 65) : Color(255, 0, 120, 215)));
                    graphics.FillEllipse(&dotBrush, dotX, dotY, dotDiam, dotDiam);
                }

                // Indicator dot for Custom Reminder / Alarm
                bool hasReminder = HasReminderForBS(g_calYear, g_calMonth, dayNum);
                if (hasReminder) {
                    float rDotDiam = 5.0f * s;
                    float rDotX = cellRect.X + cellRect.Width - rDotDiam - 3.5f * s;
                    float rDotY = cellRect.Y + 3.5f * s;
                    SolidBrush remDotBrush(Color(255, 245, 175, 25)); // Amber/gold reminder accent dot
                    graphics.FillEllipse(&remDotBrush, rDotX, rDotY, rDotDiam, rDotDiam);
                }
            }
        }
    }

    // ── Tithi Bar (centered, no box) ─────────────────────────────────────────
    {
        int tAdY = 0, tAdM = 0, tAdD = 0, tDow = 0;
        BSToAD(g_calYear, g_calMonth, g_calSelectedDay, tAdY, tAdM, tAdD, tDow);

        std::wstring moonStr  = L"\u263D  ";
        std::wstring tithiVal = GetTithiString(tAdY, tAdM, tAdD);

        // Center vertically in bottom space (426..468 → midpoint 447)
        float barCenterY = 447.0f * s;
        float textH      = 22.0f * s;
        float textY      = barCenterY - textH / 2.0f;

        Font fontTithi(&fontFamily, 15.0f * s, FontStyleBold, UnitPixel);

        // Measure moon + tithi together to center the whole string
        std::wstring fullStr = moonStr + tithiVal;
        RectF measureRect(0, 0, (float)baseW * s, textH);
        RectF measured;
        graphics.MeasureString(fullStr.c_str(), -1, &fontTithi, measureRect, &formatCenter, &measured);

        float startX = ((float)baseW * s - measured.Width) / 2.0f;

        // Draw moon in gray
        SolidBrush moonBrush(g_isLightTheme ? Color(180, 120, 120, 120) : Color(180, 160, 160, 160));
        RectF moonRect(startX, textY, measured.Width, textH);
        // Draw just the moon portion
        RectF moonOnly(startX, textY, (float)baseW * s, textH);
        graphics.DrawString(moonStr.c_str(), -1, &fontTithi, moonOnly, &formatNear, &moonBrush);

        // Measure moon width so tithi starts right after
        RectF moonMeasured;
        graphics.MeasureString(moonStr.c_str(), -1, &fontTithi, measureRect, &formatNear, &moonMeasured);

        // Draw tithi value in red
        SolidBrush tithiTextBrush(g_isLightTheme ? Color(255, 30, 30, 30) : Color(255, 255, 255, 255));
        RectF tithiRect(startX + moonMeasured.Width, textY, (float)baseW * s, textH);
        graphics.DrawString(tithiVal.c_str(), -1, &fontTithi, tithiRect, &formatNear, &tithiTextBrush);
    }

    graphics.ResetTransform();

    // ── 2. Render Event Card / Offline Notice Flyout ─────────────────────────
    if (hasEvent) {
        float fx, fy;
        if (isFlyoutOnLeft) {
            fx = 6.0f * s;
            fy = (isOfflineMode || isDownloading) ? (14.0f * s) : std::max(14.0f * s, std::min((baseH - flyoutH - 14.0f) * s, selectedCellCenterY - (flyoutH / 2.0f) * s));
        } else {
            fx = calCardOffsetX + (baseW + (isOfflineMode || isDownloading ? 12.0f : arrowW)) * s;
            fy = (isOfflineMode || isDownloading) ? (14.0f * s) : std::max(14.0f * s, std::min((baseH - flyoutH - 14.0f) * s, selectedCellCenterY - (flyoutH / 2.0f) * s));
        }
        float fw = flyoutW * s;
        float fh = flyoutH * s;
        float r = 12.0f * s;

        if (isFlyoutOnLeft) {
            g_rcCalEventCard.left = screenX + (int)fx;
            g_rcCalEventCard.top = screenY + (int)fy;
            g_rcCalEventCard.right = screenX + (int)(fx + fw + arrowW * s);
            g_rcCalEventCard.bottom = screenY + (int)(fy + fh);
        } else {
            g_rcCalEventCard.left = screenX + (int)(fx - arrowW * s);
            g_rcCalEventCard.top = screenY + (int)fy;
            g_rcCalEventCard.right = screenX + (int)(fx + fw);
            g_rcCalEventCard.bottom = screenY + (int)(fy + fh);
        }

        std::wstring cardTitleNP;
        std::wstring cardTitleEN;
        std::wstring cardDescription;
        bool isPubHol = false;
        bool isOffline = false;

        bool isCustomEvent = (selReminder != NULL);

        if (isOfflineMode) {
            isOffline = true;
            cardTitleNP = L"\u0907\u0928\u094D\u091F\u0930\u0928\u0947\u091F \u091C\u0921\u093E\u0928 \u0906\u0935\u0936\u094D\u092F\u0915";
            cardTitleEN = L"Internet Connection Required";
            cardDescription = L"Please connect to the internet to load all events and public holidays for Bikram Sambat " +
                              std::to_wstring(g_calYear) + L". The calendar will automatically sync and cache once connected.";
        } else if (isDownloading) {
            cardTitleNP = L"\u0924\u093E\u0932\u093F\u0915\u093E \u0921\u093E\u0909\u0928\u0932\u094B\u0921 \u0939\u0941\u0901\u0926\u0948\u091B...";
            cardTitleEN = L"Downloading Holidays Schedule...";
            cardDescription = L"Fetching official Bikram Sambat " + std::to_wstring(g_calYear) +
                              L" public holidays and events in background. Please wait a moment...";
        } else if (selReminder && selHoliday) {
            cardTitleNP = selReminder->title;
            cardTitleEN = selReminder->isAllDay ? (L"All-Day Event \u2022 " + selHoliday->titleEN) : (L"Alarm " + FormatAlarmTime(selReminder->alarmHour, selReminder->alarmMinute) + L" \u2022 " + selHoliday->titleEN);
            cardDescription = (selReminder->desc.empty() ? L"" : (selReminder->desc + L"\n\n")) +
                              L"\u2022 Public Event: " + selHoliday->titleNP + L"\n" + selHoliday->description;
            isPubHol = selHoliday->isPublicHoliday;
        } else if (selReminder) {
            cardTitleNP = selReminder->title;
            cardTitleEN = selReminder->isAllDay ? L"All-Day Scheduled Event" : (L"Alarm set for " + FormatAlarmTime(selReminder->alarmHour, selReminder->alarmMinute));
            cardDescription = selReminder->desc.empty() ? L"Custom event / reminder set for this date." : selReminder->desc;
        } else if (selHoliday) {
            cardTitleNP = selHoliday->titleNP;
            cardTitleEN = selHoliday->titleEN;
            cardDescription = selHoliday->description;
            isPubHol = selHoliday->isPublicHoliday;
        }

        // Measure needed height for description
        StringFormat formatDesc;
        formatDesc.SetAlignment(StringAlignmentNear);
        formatDesc.SetLineAlignment(StringAlignmentNear);
        RectF layoutDesc(0, 0, (flyoutW - 48.0f) * s, 1000.0f);
        RectF measuredDesc;
        graphics.MeasureString(cardDescription.c_str(), -1, &fontDetailL3, layoutDesc, &formatDesc, &measuredDesc);

        float neededDescH = measuredDesc.Height;
        float dynamicH = 145.0f + (neededDescH / s) + (isOffline ? 45.0f : 45.0f);
        if (dynamicH < 230.0f) dynamicH = 230.0f;
        if (dynamicH > 400.0f) dynamicH = 400.0f;
        fh = dynamicH * s;

        GraphicsPath bubblePath;
        if (isOfflineMode || isDownloading) {
            AddRoundedRectangle(bubblePath, fx, fy, fw, fh, r);
        } else if (isFlyoutOnLeft) {
            // Speech bubble with triangle arrow on the RIGHT pointing to the calendar cell
            float tipX = calCardOffsetX - 2.0f * s;
            float tipY = selectedCellCenterY;
            float topY = tipY - 10.0f * s;
            float botY = tipY + 10.0f * s;

            bubblePath.AddArc(fx, fy, r * 2.0f, r * 2.0f, 180, 90);
            bubblePath.AddArc(fx + fw - r * 2.0f, fy, r * 2.0f, r * 2.0f, 270, 90);
            bubblePath.AddLine(fx + fw, fy + r, fx + fw, topY);
            bubblePath.AddLine(fx + fw, topY, tipX, tipY);
            bubblePath.AddLine(tipX, tipY, fx + fw, botY);
            bubblePath.AddLine(fx + fw, botY, fx + fw, fy + fh - r);
            bubblePath.AddArc(fx + fw - r * 2.0f, fy + fh - r * 2.0f, r * 2.0f, r * 2.0f, 0, 90);
            bubblePath.AddArc(fx, fy + fh - r * 2.0f, r * 2.0f, r * 2.0f, 90, 90);
            bubblePath.CloseFigure();
        } else {
            // Speech bubble with triangle arrow on the LEFT pointing to the calendar cell
            float tipX = calCardOffsetX + baseW * s + 2.0f * s;
            float tipY = selectedCellCenterY;
            float topY = tipY - 10.0f * s;
            float botY = tipY + 10.0f * s;

            bubblePath.AddArc(fx, fy, r * 2.0f, r * 2.0f, 180, 90);
            bubblePath.AddArc(fx + fw - r * 2.0f, fy, r * 2.0f, r * 2.0f, 270, 90);
            bubblePath.AddArc(fx + fw - r * 2.0f, fy + fh - r * 2.0f, r * 2.0f, r * 2.0f, 0, 90);
            bubblePath.AddArc(fx, fy + fh - r * 2.0f, r * 2.0f, r * 2.0f, 90, 90);
            bubblePath.AddLine(fx, fy + fh - r, fx, botY);
            bubblePath.AddLine(fx, botY, tipX, tipY);
            bubblePath.AddLine(tipX, tipY, fx, topY);
            bubblePath.AddLine(fx, topY, fx, fy + r);
            bubblePath.CloseFigure();
        }

        SolidBrush flyoutBg(g_isLightTheme ? Color(255, 255, 255, 255) : Color(255, 30, 30, 36));
        graphics.FillPath(&flyoutBg, &bubblePath);
        Pen flyoutPen(g_isLightTheme ? Color(60, 0, 0, 0) : Color(70, 255, 255, 255), 1.0f * s);
        graphics.DrawPath(&flyoutPen, &bubblePath);

        // Left accent strip
        GraphicsPath stripPath;
        AddRoundedRectangle(stripPath, fx + 12.0f * s, fy + 16.0f * s, 4.0f * s, fh - 32.0f * s, 2.0f * s);
        SolidBrush stripBrush(isPubHol ? Color(255, 220, 20, 60) : (isOffline ? Color(255, 220, 140, 20) : Color(255, 0, 120, 215)));
        graphics.FillPath(&stripBrush, &stripPath);

        int selAdY = 0, selAdM = 0, selAdD = 0, selDow = 0;
        BSToAD(g_calYear, g_calMonth, g_calSelectedDay, selAdY, selAdM, selAdD, selDow);

        StringFormat formatNearNoWrap;
        formatNearNoWrap.SetAlignment(StringAlignmentNear);
        formatNearNoWrap.SetLineAlignment(StringAlignmentCenter);
        formatNearNoWrap.SetFormatFlags(StringFormatFlagsNoWrap);

        // Top Section Line 1: Devanagari BS Date or Status Header
        std::wstring npDateStr;
        if (isOffline || isDownloading) {
            npDateStr = ToDevanagariNum(g_calYear) + L" BS \u2022 \u091A\u093E\u0921\u092A\u0930\u094D\u0935 \u0930 \u092C\u093F\u0926\u093E";
        } else {
            npDateStr = ToDevanagariNum(g_calYear) + L" " + kNepaliMonthNamesNP[mIdx] + L" " +
                        ToDevanagariNum(g_calSelectedDay) + L" \u0917\u0924\u0947, " + kNepaliDayNamesNP[selDow];
        }

        RectF npDateRect(fx + 26.0f * s, fy + 16.0f * s, fw - 165.0f * s, 22.0f * s);
        graphics.DrawString(npDateStr.c_str(), -1, &fontDetailL2, npDateRect, &formatNearNoWrap, &textPrimary);

        // Top Section Line 2: English AD Date or Sub-Header
        std::wstring enDateStr;
        if (isOffline || isDownloading) {
            enDateStr = L"Events & Public Holidays";
        } else {
            enDateStr = std::to_wstring(selAdD) + L" " + kEnglishMonthNames[selAdM - 1] + L" " +
                        std::to_wstring(selAdY) + L", " + kNepaliDayNamesEN[selDow];
        }

        RectF enDateRect(fx + 26.0f * s, fy + 40.0f * s, fw - 165.0f * s, 20.0f * s);
        graphics.DrawString(enDateStr.c_str(), -1, &fontNavSub, enDateRect, &formatNearNoWrap, &textSubtle);

        // Top Section: Category Badge on right
        RectF badgeRect(fx + fw - 132.0f * s, fy + 22.0f * s, 118.0f * s, 28.0f * s);
        GraphicsPath badgePath;
        AddRoundedRectangle(badgePath, badgeRect.X, badgeRect.Y, badgeRect.Width, badgeRect.Height, 5.0f * s);
        
        if (isPubHol) {
            SolidBrush badgeBg(Color(50, 220, 20, 60));
            graphics.FillPath(&badgeBg, &badgePath);
            Pen badgePen(Color(180, 220, 20, 60), 1.0f * s);
            graphics.DrawPath(&badgePen, &badgePath);
            graphics.DrawString(L"PUBLIC HOLIDAY", -1, &fontBadge, badgeRect, &formatCenter, &crimsonText);
        } else if (isOffline) {
            SolidBrush badgeBg(Color(50, 220, 140, 20));
            graphics.FillPath(&badgeBg, &badgePath);
            Pen badgePen(Color(180, 220, 140, 20), 1.0f * s);
            graphics.DrawPath(&badgePen, &badgePath);
            SolidBrush amberText(Color(255, 230, 140, 30));
            graphics.DrawString(L"\u26A0 OFFLINE", -1, &fontBadge, badgeRect, &formatCenter, &amberText);
        } else if (isDownloading) {
            SolidBrush badgeBg(Color(50, 0, 120, 215));
            graphics.FillPath(&badgeBg, &badgePath);
            Pen badgePen(Color(180, 0, 120, 215), 1.0f * s);
            graphics.DrawPath(&badgePen, &badgePath);
            SolidBrush blueText(Color(255, 60, 140, 255));
            graphics.DrawString(L"\u21BB SYNCING", -1, &fontBadge, badgeRect, &formatCenter, &blueText);
        } else if (isCustomEvent) {
            SolidBrush badgeBg(Color(50, 245, 175, 25));
            graphics.FillPath(&badgeBg, &badgePath);
            Pen badgePen(Color(180, 245, 175, 25), 1.0f * s);
            graphics.DrawPath(&badgePen, &badgePath);
            SolidBrush goldText(Color(255, 245, 175, 25));
            graphics.DrawString(L"\u23F0 REMINDER", -1, &fontBadge, badgeRect, &formatCenter, &goldText);
        } else {
            SolidBrush badgeBg(g_isLightTheme ? Color(40, 0, 56, 147) : Color(60, 0, 56, 147));
            graphics.FillPath(&badgeBg, &badgePath);
            Pen badgePen(Color(180, 0, 80, 200), 1.0f * s);
            graphics.DrawPath(&badgePen, &badgePath);
            SolidBrush blueText(Color(255, 60, 140, 255));
            graphics.DrawString(L"FESTIVAL / \u092A\u0930\u094D\u0935", -1, &fontBadge, badgeRect, &formatCenter, &blueText);
        }

        // Divider line
        Pen dividerPen(g_isLightTheme ? Color(25, 0, 0, 0) : Color(40, 255, 255, 255), 1.0f * s);
        graphics.DrawLine(&dividerPen, fx + 22.0f * s, fy + 68.0f * s, fx + fw - 14.0f * s, fy + 68.0f * s);

        // Event Title: Nepali & English
        RectF holTitleRect(fx + 26.0f * s, fy + 78.0f * s, fw - 42.0f * s, 28.0f * s);
        SolidBrush holTitleBrush(isPubHol ? Color(255, 240, 45, 75) : (g_isLightTheme ? Color(255, 0, 56, 147) : Color(255, 90, 170, 255)));
        graphics.DrawString(cardTitleNP.c_str(), -1, &fontDetailL1, holTitleRect, &formatNear, &holTitleBrush);

        RectF holSubTitleRect(fx + 26.0f * s, fy + 108.0f * s, fw - 42.0f * s, 24.0f * s);
        graphics.DrawString(cardTitleEN.c_str(), -1, &fontDetailL2, holSubTitleRect, &formatNear, &textPrimary);

        // Event Description
        RectF holDescRect(fx + 26.0f * s, fy + 138.0f * s, (flyoutW - 48.0f) * s, (neededDescH + 4.0f) * s);
        SolidBrush descTextBrush(g_isLightTheme ? Color(255, 45, 45, 50) : Color(255, 235, 235, 240));
        graphics.DrawString(cardDescription.c_str(), -1, &fontDetailL3, holDescRect, &formatDesc, &descTextBrush);

        // Retry button if offline
        if (isOffline) {
            RectF retryBtnRect(fx + 26.0f * s, fy + fh - 44.0f * s, 140.0f * s, 30.0f * s);
            DrawIconButton(graphics, retryBtnRect, L"\u21BB Retry / \u092A\u0941\u0928\u0903 \u092A\u094D\u0930\u092F\u093E\u0938", g_hoveredBtn == BTN_RETRY_FETCH, g_isLightTheme, fontButton);
        } else if (!isDownloading) {
            // Tithify Themed Set / Edit Reminder Button in Flyout
            bool isRemHovered = (g_hoveredBtn == BTN_SET_REMINDER);
            REAL remW = (isCustomEvent ? 150.0f : 175.0f) * s;
            REAL remH = 30.0f * s;
            REAL remX = fx + 26.0f * s;
            REAL remY = fy + fh - 44.0f * s;
            RectF remBtnRect(remX, remY, remW, remH);

            GraphicsPath remPath;
            AddRoundedRectangle(remPath, remX, remY, remW, remH, 6.0f * s);

            SolidBrush remBg(isRemHovered ? Color(255, 242, 60, 60) : Color(255, 220, 38, 38));
            graphics.FillPath(&remBg, &remPath);

            Pen remBorder(isRemHovered ? Color(255, 255, 110, 110) : Color(255, 195, 25, 25), 1.0f * s);
            graphics.DrawPath(&remBorder, &remPath);

            SolidBrush remText(Color(255, 255, 255, 255));
            const wchar_t* remLabel = isCustomEvent ? L"\u23F0  Edit Reminder" : L"\u23F0  Set Reminder";
            graphics.DrawString(remLabel, -1, &fontButton, remBtnRect, &formatCenter, &remText);

            // If it's a custom event, also show a Tithify Delete Reminder button
            if (isCustomEvent) {
                REAL delW = 105.0f * s;
                REAL delX = remX + remW + 10.0f * s;
                RectF delBtnRect(delX, remY, delW, remH);

                GraphicsPath delPath;
                AddRoundedRectangle(delPath, delX, remY, delW, remH, 6.0f * s);

                bool isDelHovered = (g_hoveredBtn == BTN_DELETE_REMINDER);
                if (isDelHovered) {
                    SolidBrush delBg(Color(255, 220, 38, 38));
                    graphics.FillPath(&delBg, &delPath);
                    SolidBrush delText(Color(255, 255, 255, 255));
                    graphics.DrawString(L"\u2715  Delete", -1, &fontButton, delBtnRect, &formatCenter, &delText);
                } else {
                    SolidBrush delBg(Color(20, 220, 38, 38));
                    graphics.FillPath(&delBg, &delPath);
                    Pen delBorder(Color(120, 220, 38, 38), 1.0f * s);
                    graphics.DrawPath(&delBorder, &delPath);
                    SolidBrush delText(Color(255, 220, 38, 38));
                    graphics.DrawString(L"\u2715  Delete", -1, &fontButton, delBtnRect, &formatCenter, &delText);
                }
            }
        }
    }

    // Apply per-pixel alpha channel
    POINT ptDst = { screenX, screenY };
    SIZE sizeDst = { rawW, rawH };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };

    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeDst, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    SetWindowPos(hWnd, NULL, screenX, screenY, rawW, rawH,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK CalendarWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR: {
        if (g_hoveredBtn != BTN_NONE || g_hoveredCell != -1) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }

    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        float s = g_dpiScale;
        float calOffset = (g_flyoutOnLeft ? (380.0f + 12.0f) : 0.0f);
        float bx = x / s - calOffset;
        float by = y / s;

        int newBtn = BTN_NONE;
        int newCell = -1;

        float flyoutBtnX = (g_flyoutOnLeft ? (6.0f + 26.0f) : (420.0f + 12.0f + 26.0f));
        float flyoutBtnY = 468.0f - 44.0f;

        if (bx >= 18.0f && bx <= 54.0f && by >= 60.0f && by <= 94.0f) {
            newBtn = BTN_PREV_MONTH;
        } else if (bx >= 366.0f && bx <= 402.0f && by >= 60.0f && by <= 94.0f) {
            newBtn = BTN_NEXT_MONTH;
        } else if (bx >= 300.0f && bx <= 368.0f && by >= 14.0f && by <= 44.0f) {
            newBtn = BTN_TODAY;
        } else if (bx >= 374.0f && bx <= 404.0f && by >= 14.0f && by <= 44.0f) {
            newBtn = BTN_CLOSE;
        } else if (g_holidayFetchState == FETCH_ERROR_OFFLINE && g_currentYearHolidays.empty() &&
                   (x / s) >= flyoutBtnX && (x / s) <= (flyoutBtnX + 140.0f) &&
                   (y / s) >= (flyoutBtnY - 6.0f) && (y / s) <= (flyoutBtnY + 36.0f)) {
            newBtn = BTN_RETRY_FETCH;
        } else if (g_calHasEvent && !(g_holidayFetchState == FETCH_ERROR_OFFLINE && g_currentYearHolidays.empty()) &&
                   g_holidayFetchState != FETCH_DOWNLOADING) {
            bool hasCustom = (FindReminderForBS(g_calYear, g_calMonth, g_calSelectedDay) != NULL);
            float remW = hasCustom ? 150.0f : 175.0f;
            if ((x / s) >= flyoutBtnX && (x / s) <= (flyoutBtnX + remW) &&
                (y / s) >= (flyoutBtnY - 6.0f) && (y / s) <= (flyoutBtnY + 36.0f)) {
                newBtn = BTN_SET_REMINDER;
            } else if (hasCustom &&
                       (x / s) >= (flyoutBtnX + remW + 10.0f) && (x / s) <= (flyoutBtnX + remW + 10.0f + 105.0f) &&
                       (y / s) >= (flyoutBtnY - 6.0f) && (y / s) <= (flyoutBtnY + 36.0f)) {
                newBtn = BTN_DELETE_REMINDER;
            }
        }

        if (newBtn == BTN_NONE && bx >= 18.0f && bx < 403.0f && by >= 138.0f && by < 426.0f) {
            int c = (int)((bx - 18.0f) / 55.0f);
            int r = (int)((by - 138.0f) / 48.0f);
            if (c >= 0 && c < 7 && r >= 0 && r < 6) {
                int cellIdx = r * 7 + c;
                int startDow = GetBSMonthStartDayOfWeek(g_calYear, g_calMonth);
                int totalD = GetBSDaysInMonth(g_calYear, g_calMonth);
                int dayNum = cellIdx - startDow + 1;
                if (dayNum >= 1 && dayNum <= totalD) {
                    newCell = cellIdx;
                }
            }
        }

        if (newBtn != g_hoveredBtn || newCell != g_hoveredCell) {
            g_hoveredBtn = newBtn;
            g_hoveredCell = newCell;
            RenderCalendar(hWnd);
        }

        TRACKMOUSEEVENT tme = { sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);
        break;
    }

    case WM_MOUSELEAVE:
        if (g_hoveredBtn != BTN_NONE || g_hoveredCell != -1) {
            g_hoveredBtn = BTN_NONE;
            g_hoveredCell = -1;
            RenderCalendar(hWnd);
        }
        break;

    case WM_LBUTTONDOWN: {
        if (g_isDatePopupOpen) {
            CloseCalendarDatePopup();
        }

        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        float s = g_dpiScale;
        float calOffset = (g_flyoutOnLeft ? (380.0f + 12.0f) : 0.0f);
        float bx = x / s - calOffset;
        float by = y / s;
        float flyoutBtnX = (g_flyoutOnLeft ? (6.0f + 26.0f) : (420.0f + 12.0f + 26.0f));
        float flyoutBtnY = 468.0f - 44.0f;

        if (bx >= 18.0f && bx <= 54.0f && by >= 60.0f && by <= 94.0f) {
            if (g_calMonth > 1) {
                g_calMonth--;
                RenderCalendar(hWnd);
            }
        } else if (bx >= 366.0f && bx <= 402.0f && by >= 60.0f && by <= 94.0f) {
            if (g_calMonth < 12) {
                g_calMonth++;
                RenderCalendar(hWnd);
            }
        } else if (bx >= 300.0f && bx <= 368.0f && by >= 14.0f && by <= 44.0f) {
            int cy, cm, cd, cdow;
            GetCurrentBSDate(cy, cm, cd, cdow);
            g_calYear = cy;
            g_calMonth = cm;
            g_calSelectedDay = cd;
            RenderCalendar(hWnd);
        } else if (bx >= 374.0f && bx <= 404.0f && by >= 14.0f && by <= 44.0f) {
            HideCalendar();
        } else if (g_hoveredBtn == BTN_RETRY_FETCH ||
                  (g_holidayFetchState == FETCH_ERROR_OFFLINE && g_currentYearHolidays.empty() &&
                   (x / s) >= flyoutBtnX && (x / s) <= (flyoutBtnX + 140.0f) &&
                   (y / s) >= (flyoutBtnY - 6.0f) && (y / s) <= (flyoutBtnY + 36.0f))) {
            EnsureHolidaysLoadedForCurrentYear(g_calYear, true);
            RenderCalendar(hWnd);
        } else if (g_hoveredBtn == BTN_SET_REMINDER ||
                  (g_calHasEvent && !(g_holidayFetchState == FETCH_ERROR_OFFLINE && g_currentYearHolidays.empty()) &&
                   g_holidayFetchState != FETCH_DOWNLOADING &&
                   (x / s) >= flyoutBtnX && (x / s) <= (flyoutBtnX + (FindReminderForBS(g_calYear, g_calMonth, g_calSelectedDay) ? 150.0f : 175.0f)) &&
                   (y / s) >= (flyoutBtnY - 6.0f) && (y / s) <= (flyoutBtnY + 36.0f))) {
            ShowReminderDashboard(hWnd, g_calYear, g_calMonth, g_calSelectedDay);
        } else if (g_hoveredBtn == BTN_DELETE_REMINDER ||
                  (g_calHasEvent && FindReminderForBS(g_calYear, g_calMonth, g_calSelectedDay) &&
                   (x / s) >= (flyoutBtnX + 160.0f) && (x / s) <= (flyoutBtnX + 160.0f + 105.0f) &&
                   (y / s) >= (flyoutBtnY - 6.0f) && (y / s) <= (flyoutBtnY + 36.0f))) {
            DeleteReminderForBS(g_calYear, g_calMonth, g_calSelectedDay);
            RenderCalendar(hWnd);
        } else if (bx >= 18.0f && bx < 403.0f && by >= 138.0f && by < 426.0f) {
            int c = (int)((bx - 18.0f) / 55.0f);
            int r = (int)((by - 138.0f) / 48.0f);
            if (c >= 0 && c < 7 && r >= 0 && r < 6) {
                int cellIdx = r * 7 + c;
                int startDow = GetBSMonthStartDayOfWeek(g_calYear, g_calMonth);
                int totalD = GetBSDaysInMonth(g_calYear, g_calMonth);
                int dayNum = cellIdx - startDow + 1;
                if (dayNum >= 1 && dayNum <= totalD) {
                    g_calSelectedDay = dayNum;
                    RenderCalendar(hWnd);
                }
            }
        }
        break;
    }

    case WM_RBUTTONDOWN:
        return 0;

    case WM_RBUTTONUP: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        float s = g_dpiScale;
        float calOffset = (g_flyoutOnLeft ? (380.0f + 12.0f) : 0.0f);
        float bx = x / s - calOffset;
        float by = y / s;

        if (bx >= 18.0f && bx < 403.0f && by >= 138.0f && by < 426.0f) {
            int c = (int)((bx - 18.0f) / 55.0f);
            int r = (int)((by - 138.0f) / 48.0f);
            if (c >= 0 && c < 7 && r >= 0 && r < 6) {
                int cellIdx = r * 7 + c;
                int startDow = GetBSMonthStartDayOfWeek(g_calYear, g_calMonth);
                int totalD = GetBSDaysInMonth(g_calYear, g_calMonth);
                int dayNum = cellIdx - startDow + 1;
                if (dayNum >= 1 && dayNum <= totalD) {
                    g_calSelectedDay = dayNum;
                    RenderCalendar(hWnd);

                    POINT screenPt;
                    GetCursorPos(&screenPt);
                    ShowCalendarDatePopup(hWnd, screenPt, g_calYear, g_calMonth, dayNum);
                }
            }
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0 && g_calMonth > 1) g_calMonth--;
        else if (delta < 0 && g_calMonth < 12) g_calMonth++;
        RenderCalendar(hWnd);
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) HideCalendar();
        else if (wParam == VK_LEFT) { if (g_calSelectedDay > 1) g_calSelectedDay--; RenderCalendar(hWnd); }
        else if (wParam == VK_RIGHT) { int maxD = GetBSDaysInMonth(g_calYear, g_calMonth); if (g_calSelectedDay < maxD) g_calSelectedDay++; RenderCalendar(hWnd); }
        else if (wParam == VK_UP) { if (g_calSelectedDay > 7) g_calSelectedDay -= 7; RenderCalendar(hWnd); }
        else if (wParam == VK_DOWN) { int maxD = GetBSDaysInMonth(g_calYear, g_calMonth); if (g_calSelectedDay + 7 <= maxD) g_calSelectedDay += 7; RenderCalendar(hWnd); }
        break;

    case WM_TIMER:
        if (wParam == 100) {
            DWORD elapsed = GetTickCount() - g_calAnimStartTime;
            float scale = g_dpiScale;

            if (g_calAnimState == CAL_ANIM_OPENING) {
                if (elapsed >= CAL_ANIM_OPEN_MS) {
                    g_calAnimState = CAL_ANIM_OPEN;
                    KillTimer(hWnd, 100);
                    RenderCalendar(hWnd, 255, g_calBaseX, g_calBaseY);
                } else {
                    float progress = (float)elapsed / (float)CAL_ANIM_OPEN_MS;
                    float ease = 1.0f - powf(1.0f - progress, 3.0f); // Cubic Ease-Out
                    BYTE alpha = (BYTE)(ease * 255.0f);
                    int offset = (int)((1.0f - ease) * (18.0f * scale));
                    int animY = g_calBaseY + offset;
                    RenderCalendar(hWnd, alpha, g_calBaseX, animY);
                }
            } else if (g_calAnimState == CAL_ANIM_CLOSING) {
                if (elapsed >= CAL_ANIM_CLOSE_MS) {
                    g_calAnimState = CAL_ANIM_CLOSED;
                    KillTimer(hWnd, 100);
                    ShowWindow(hWnd, SW_HIDE);
                } else {
                    float progress = (float)elapsed / (float)CAL_ANIM_CLOSE_MS;
                    float ease = progress * progress * progress; // Cubic Ease-In
                    BYTE alpha = (BYTE)((1.0f - ease) * 255.0f);
                    int offset = (int)(ease * (14.0f * scale));
                    int animY = g_calBaseY + offset;
                    RenderCalendar(hWnd, alpha, g_calBaseX, animY);
                }
            }
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            HWND hOther = (HWND)lParam;
            if (hOther && (hOther == g_hDatePopupWnd || hOther == g_hReminderWnd || hOther == g_hAlarmAlertWnd)) {
                break;
            }
            if (g_isReminderDashboardOpen || g_isAlarmAlertOpen) {
                break;
            }
            if (GetTickCount() - g_calAnimStartTime > 300) {
                if (g_isDatePopupOpen) CloseCalendarDatePopup();
                HideCalendar();
            }
        }
        break;

    case WM_KILLFOCUS: {
        HWND hNew = (HWND)wParam;
        if (hNew && (hNew == g_hDatePopupWnd || hNew == g_hReminderWnd || hNew == g_hAlarmAlertWnd)) {
            break;
        }
        if (g_isReminderDashboardOpen || g_isAlarmAlertOpen) {
            break;
        }
        if (GetTickCount() - g_calAnimStartTime > 300) {
            if (g_isDatePopupOpen) CloseCalendarDatePopup();
            HideCalendar();
        }
        break;
    }

    case WM_DESTROY:
        g_hCalWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void HideCalendarImmediate() {
    CloseCalendarDatePopup();
    g_isCalendarOpen = false;
    g_calAnimState = CAL_ANIM_CLOSED;
    g_lastCalCloseTime = GetTickCount();
    if (g_hCalWnd) {
        KillTimer(g_hCalWnd, 100);
        ShowWindow(g_hCalWnd, SW_HIDE);
    }
}

void HideCalendar() {
    CloseCalendarDatePopup();
    if (!g_isCalendarOpen || g_calAnimState == CAL_ANIM_CLOSED) return;
    if (g_calAnimState == CAL_ANIM_CLOSING) return;

    g_calAnimState = CAL_ANIM_CLOSING;
    g_calAnimStartTime = GetTickCount();
    g_isCalendarOpen = false;
    g_lastCalCloseTime = GetTickCount();

    if (g_hCalWnd) {
        SetTimer(g_hCalWnd, 100, 16, NULL);
    }
}

void ShowCalendar(HWND hWidgetWnd) {
    if (!g_hCalWnd) return;
    KillTimer(g_hCalWnd, 100);
    
    int cy, cm, cd, cdow;
    GetCurrentBSDate(cy, cm, cd, cdow);
    g_calYear = cy;
    g_calMonth = cm;
    g_calSelectedDay = cd;
    g_hoveredBtn = BTN_NONE;
    g_hoveredCell = -1;

    EnsureHolidaysLoadedForCurrentYear(cy);

    int calW = (int)(420 * g_dpiScale);
    int calH = (int)(468 * g_dpiScale);

    RECT rcWidget;
    GetWindowRect(hWidgetWnd, &rcWidget);

    HMONITOR hMon = MonitorFromWindow(hWidgetWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    int widgetCenterX = rcWidget.left + (rcWidget.right - rcWidget.left) / 2;
    int x = widgetCenterX - calW / 2;
    int y = rcWidget.top - calH - (int)(10 * g_dpiScale);

    // Position below widget if above screen top or work area
    if (y < mi.rcWork.top) {
        y = rcWidget.bottom + (int)(10 * g_dpiScale);
    }
    // Clamp inside vertical screen bounds
    if (y + calH > mi.rcWork.bottom) {
        y = mi.rcWork.bottom - calH - (int)(8 * g_dpiScale);
    }
    
    // Clamp inside horizontal screen bounds for the 420px calendar card
    if (x + calW > mi.rcWork.right - (int)(10 * g_dpiScale)) {
        x = mi.rcWork.right - calW - (int)(10 * g_dpiScale);
    }
    if (x < mi.rcWork.left + (int)(10 * g_dpiScale)) {
        x = mi.rcWork.left + (int)(10 * g_dpiScale);
    }

    g_calBaseX = x;
    g_calBaseY = y;
    g_isCalendarOpen = true;

    g_calAnimState = CAL_ANIM_OPEN;
    g_calAnimStartTime = GetTickCount();
    RenderCalendar(g_hCalWnd, 255, g_calBaseX, g_calBaseY);
    SetWindowPos(g_hCalWnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ShowWindow(g_hCalWnd, SW_SHOW);
    SetForegroundWindow(g_hCalWnd);
    SetFocus(g_hCalWnd);
}

void ToggleCalendar(HWND hWidgetWnd) {
    if (g_isCalendarOpen) {
        HideCalendar();
    } else {
        ShowCalendar(hWidgetWnd);
    }
}

// ── Custom Drawing Engine (Per-Pixel Alpha) ──────────────────────────────────
void RenderWidget(HWND hWnd) {
    // Automatically ensure holiday data is loaded for the current BS year
    static int s_lastCheckedBSYear = 0;
    int curBSY = 0, curBSM = 0, curBSD = 0, curBSDOW = 0;
    GetCurrentBSDate(curBSY, curBSM, curBSD, curBSDOW);
    if (curBSY != s_lastCheckedBSYear) {
        s_lastCheckedBSYear = curBSY;
        EnsureHolidaysLoadedForCurrentYear(curBSY);
    }

    // Use DPI-scaled dimensions for the render surface to get crisp pixels
    int baseWidth = g_showDay ? 190 : 155;
    int rawWidth = (int)(baseWidth * g_dpiScale);
    int rawHeight = (int)(48 * g_dpiScale);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // Create a 32-bit ARGB DIB section for proper per-pixel alpha
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rawWidth;
    bmi.bmiHeader.biHeight = -rawHeight;  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    // Clear background with per-pixel transparency
    graphics.Clear(Color(0, 0, 0, 0));

    float s = g_dpiScale;  // Shorthand for scale factor

    Color bgColor(0, 0, 0, 0);

    if (g_setupMode) {
        // Adapt background to theme so it's visible on both light and dark taskbars
        bgColor = g_isLightTheme ? Color(210, 230, 230, 230) : Color(230, 31, 31, 31);
    } else {
        BYTE a = (BYTE)g_bgOpacity;
        switch (g_bgPresetMode) {
        case 0: // Transparent / Default
            bgColor = Color(0, 0, 0, 0);
            break;
        case 1: { // Custom Color
            BYTE r = GetRValue(g_bgColorRGB);
            BYTE g = GetGValue(g_bgColorRGB);
            BYTE b = GetBValue(g_bgColorRGB);
            bgColor = Color(a, r, g, b);
            break;
        }
        case 2: // Dark Glass
            bgColor = Color(a, 31, 31, 31);
            break;
        case 3: // Light Glass
            bgColor = Color(a, 230, 230, 230);
            break;
        case 4: // Solid Black
            bgColor = Color(a, 0, 0, 0);
            break;
        case 5: // Solid White
            bgColor = Color(a, 255, 255, 255);
            break;
        case 6: // Red Accent
            bgColor = Color(a, 180, 40, 40);
            break;
        case 7: // Blue Accent
            bgColor = Color(a, 30, 110, 200);
            break;
        }
    }

    if (bgColor.GetAlpha() > 0) {
        SolidBrush bgBrush(bgColor);
        GraphicsPath bgPath;
        REAL r = 8.0f * s;
        REAL d = r * 2.0f;
        REAL w = (REAL)rawWidth;
        REAL h = (REAL)rawHeight;
        bgPath.AddArc(0.0f, 0.0f, d, d, 180.0f, 90.0f);
        bgPath.AddArc(w - d, 0.0f, d, d, 270.0f, 90.0f);
        bgPath.AddArc(w - d, h - d, d, d, 0.0f, 90.0f);
        bgPath.AddArc(0.0f, h - d, d, d, 90.0f, 90.0f);
        bgPath.CloseFigure();
        graphics.FillPath(&bgBrush, &bgPath);
    } else {
        // Invisible hit-test box: Alpha 1 ensures Windows registers mouse clicks (left click to toggle
        // calendar, right click for context menu, and dragging) across the entire widget bounds,
        // rather than letting clicks pass through transparent pixels to the taskbar.
        SolidBrush hitTestBrush(Color(1, 0, 0, 0));
        graphics.FillRectangle(&hitTestBrush, 0, 0, rawWidth, rawHeight);
    }

    // Determine text contrast based on background brightness
    bool useDarkText = g_isLightTheme;
    if (!g_setupMode && g_bgPresetMode != 0 && bgColor.GetAlpha() > 50) {
        double luminance = (0.299 * bgColor.GetR() + 0.587 * bgColor.GetG() + 0.114 * bgColor.GetB());
        useDarkText = (luminance > 140.0);
    }

    // Pre-calculate positions to create a tight, sequential layout: [Day] [Gap] [Flag] [Gap] [Date]
    REAL h = 24.0f * s;
    REAL flagWidth = 0.822f * h;
    
    // Y coordinates
    REAL cy = rawHeight / 2.0f;
    REAL top = cy - h / 2.0f;

    // 1. Measure Text and Calculate Sequential Coordinates
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 12.0f * s, FontStyleBold, UnitPixel);
    std::wstring dateStr = GetNepaliDateString();

    StringFormat formatNear;
    formatNear.SetAlignment(StringAlignmentNear);
    formatNear.SetLineAlignment(StringAlignmentCenter);

    REAL gap = 12.0f * s;
    REAL cx, dateX;

    if (g_showDay) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        const wchar_t* daysOfWeek[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
        const wchar_t* dayStr = daysOfWeek[st.wDayOfWeek];

        // Measure day string
        RectF dayMeasure;
        graphics.MeasureString(dayStr, -1, &font, RectF(0,0,1000,100), &formatNear, &dayMeasure);

        // Day box on the left
        REAL boxPaddingX = 5.0f * s;
        REAL boxWidth = dayMeasure.Width + boxPaddingX * 2.0f;
        REAL boxHeight = h - 2.0f * s;
        REAL boxX = 10.0f * s;
        REAL boxY = (rawHeight - boxHeight) / 2.0f;

        // Flag in the middle
        cx = boxX + boxWidth + gap;
        dateX = cx + flagWidth + gap;

        // 2. Render Day Box
        SolidBrush boxBrush(useDarkText ? Color(25, 0, 0, 0) : Color(35, 255, 255, 255));
        GraphicsPath path;
        REAL r = 4.0f * s;
        REAL d = r * 2.0f;
        path.AddArc(boxX, boxY, d, d, 180, 90);
        path.AddArc(boxX + boxWidth - d, boxY, d, d, 270, 90);
        path.AddArc(boxX + boxWidth - d, boxY + boxHeight - d, d, d, 0, 90);
        path.AddArc(boxX, boxY + boxHeight - d, d, d, 90, 90);
        path.CloseFigure();
        graphics.FillPath(&boxBrush, &path);

        // Draw day text centered in the box
        SolidBrush dayTextBrush(useDarkText ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255));
        StringFormat formatCenter;
        formatCenter.SetAlignment(StringAlignmentCenter);
        formatCenter.SetLineAlignment(StringAlignmentCenter);
        RectF dayTextRect(boxX, boxY, boxWidth, boxHeight);
        graphics.DrawString(dayStr, -1, &font, dayTextRect, &formatCenter, &dayTextBrush);
    } else {
        // No day box: flag on the left, date after it
        cx = 10.0f * s;
        dateX = cx + flagWidth + gap;
    }

    SolidBrush textBrush(useDarkText ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255));

    // 3. Render Flag
    DrawNepaliFlag(graphics, cx, top, h);

    // 4. Render Date Text
    RectF dateRect(dateX, 0.0f, (REAL)rawWidth - dateX, (REAL)rawHeight);
    graphics.DrawString(dateStr.c_str(), -1, &font, dateRect, &formatNear, &textBrush);

    // Apply per-pixel alpha channel to OS Window
    POINT ptDst = { g_xPos + g_currentShiftX, g_yPos + g_currentShiftY };
    SIZE sizeDst = { rawWidth, rawHeight };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeDst, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

// ── System Tray Icon Setup ───────────────────────────────────────────────────
#define WM_TRAYICON (WM_USER + 1)
NOTIFYICONDATAW g_nid = {};

void AddTrayIcon(HWND hWnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1)); // Custom embedded logo
    wcscpy_s(g_nid.szTip, L"Tithify \u2014 Nepali Date Widget");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

// ── Custom GDI+ Context Menu Engine ──────────────────────────────────────────
void HideCustomMenu() {
    if (g_hMenuWnd && IsWindowVisible(g_hMenuWnd)) {
        ShowWindow(g_hMenuWnd, SW_HIDE);
        g_isMenuOpen    = false;
        g_bgFlyoutOpen  = false;
        g_bgFlyoutHover = -1;
        g_bgSliderDrag  = false;
    }
}

void RenderCustomMenu(HWND hWnd) {
    if (!hWnd) return;

    float s = g_dpiScale;
    // When BG flyout is open expand bitmap: 385 menu + 12 arrow + 200 flyout = 597
    int rawWidth  = g_bgFlyoutOpen ? (int)(597 * s) : (int)(385 * s);
    int rawHeight = (int)(260 * s);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rawWidth;
    bmi.bmiHeader.biHeight = -rawHeight;  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    graphics.Clear(Color(0, 0, 0, 0));

    // ── Main Background Panel Card ───────────────────────────────────────────
    Color bgCardColor = g_isLightTheme ? Color(248, 245, 245, 248) : Color(245, 24, 24, 28);
    Color borderColor = g_isLightTheme ? Color(180, 200, 0, 0) : Color(200, 220, 0, 0);

    SolidBrush cardBrush(bgCardColor);
    GraphicsPath cardPath;
    REAL r = 14.0f * s;
    REAL d = r * 2.0f;
    REAL w = 385.0f * s;   // card is always 385px wide; bitmap may be wider when flyout is open
    REAL h = (REAL)rawHeight;
    cardPath.AddArc(0.0f, 0.0f, d, d, 180.0f, 90.0f);
    cardPath.AddArc(w - d, 0.0f, d, d, 270.0f, 90.0f);
    cardPath.AddArc(w - d, h - d, d, d, 0.0f, 90.0f);
    cardPath.AddArc(0.0f, h - d, d, d, 90.0f, 90.0f);
    cardPath.CloseFigure();

    graphics.FillPath(&cardBrush, &cardPath);
    Pen borderPen(borderColor, 1.5f * s);
    graphics.DrawPath(&borderPen, &cardPath);

    // ── Header Text ──────────────────────────────────────────────────────────
    FontFamily fontFamily(L"Segoe UI");
    Font titleFont(&fontFamily, 14.0f * s, FontStyleBold, UnitPixel);
    Font subFont(&fontFamily, 10.0f * s, FontStyleBold, UnitPixel);

    Color textColor = g_isLightTheme ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255);
    Color subTextColor = g_isLightTheme ? Color(180, 80, 80, 80) : Color(180, 180, 180, 180);

    SolidBrush textBrush(textColor);
    SolidBrush subTextBrush(subTextColor);

    StringFormat formatNear;
    formatNear.SetAlignment(StringAlignmentNear);
    formatNear.SetLineAlignment(StringAlignmentCenter);

    StringFormat formatCenter;
    formatCenter.SetAlignment(StringAlignmentCenter);
    formatCenter.SetLineAlignment(StringAlignmentCenter);

    StringFormat formatFar;
    formatFar.SetAlignment(StringAlignmentFar);
    formatFar.SetLineAlignment(StringAlignmentCenter);

    REAL headerH = 38.0f * s;
    RectF headerLeftRect(18.0f * s, 0.0f, (w - 36.0f * s) / 2.0f, headerH);
    RectF headerRightRect(18.0f * s, 0.0f, w - 36.0f * s, headerH);

    graphics.DrawString(L"TITHIFY", -1, &titleFont, headerLeftRect, &formatNear, &textBrush);
    wchar_t versionStr[64];
    swprintf(versionStr, 64, L"Nepali Date Widget v%ls", APP_VERSION);
    graphics.DrawString(versionStr, -1, &subFont, headerRightRect, &formatFar, &subTextBrush);

    // Header horizontal line
    Color lineHorizColor = g_isLightTheme ? Color(40, 0, 0, 0) : Color(40, 255, 255, 255);
    Pen lineHorizPen(lineHorizColor, 1.0f * s);
    graphics.DrawLine(&lineHorizPen, 16.0f * s, headerH, w - 16.0f * s, headerH);

    // ── Center Vertical Divider Line (Theme Changeable) ──────────────────────
    Color dividerColor = g_isLightTheme ? Color(140, 0, 0, 0) : Color(140, 255, 255, 255);
    Pen dividerPen(dividerColor, 1.5f * s);
    graphics.DrawLine(&dividerPen, 172.0f * s, 48.0f * s, 172.0f * s, h - 16.0f * s);

    // ── Left Column: ON / OFF Toggles ────────────────────────────────────────
    Font labelFont(&fontFamily, 12.0f * s, FontStyleBold, UnitPixel);

    struct ToggleItem {
        const wchar_t* label;
        bool state;
    };
    ToggleItem toggles[4];
    int numToggles = 0;
    toggles[numToggles++] = { L"Run at Startup", IsStartupEnabled() };
    toggles[numToggles++] = { L"Show Day", g_showDay };
    toggles[numToggles++] = { L"Lock Position", !g_setupMode };
    if (!g_isOnTaskbar && !g_setupMode) {
        toggles[numToggles++] = { L"Z-Index", g_zIndexDesktop };
    }

    for (int i = 0; i < numToggles; i++) {
        REAL rowY, rowH, pillH, knobSize;
        if (numToggles == 4) {
            rowY = (47.0f + i * 49.0f) * s;
            rowH = 42.0f * s;
            pillH = 22.0f * s;
            knobSize = 18.0f * s;
        } else {
            rowY = (52.0f + i * 62.0f) * s;
            rowH = 48.0f * s;
            pillH = 24.0f * s;
            knobSize = 20.0f * s;
        }

        // Label (tight gap before toggle)
        RectF textRect(16.0f * s, rowY, 102.0f * s, rowH);
        graphics.DrawString(toggles[i].label, -1, &labelFont, textRect, &formatNear, &textBrush);

        // iOS Toggle Pill Switch
        REAL pillX = 120.0f * s;
        REAL pillY = rowY + (rowH - pillH) / 2.0f;
        REAL pillW = 44.0f * s;
        REAL pillR = pillH / 2.0f;
        REAL pillD = pillR * 2.0f;

        GraphicsPath pillPath;
        pillPath.AddArc(pillX, pillY, pillD, pillD, 90.0f, 180.0f);
        pillPath.AddArc(pillX + pillW - pillD, pillY, pillD, pillD, 270.0f, 180.0f);
        pillPath.CloseFigure();

        Color pillBg = toggles[i].state ? Color(255, 210, 40, 40) : (g_isLightTheme ? Color(255, 200, 200, 204) : Color(255, 90, 90, 95));
        SolidBrush pillBrush(pillBg);
        graphics.FillPath(&pillBrush, &pillPath);

        // Circular Knob
        REAL knobY = pillY + (pillH - knobSize) / 2.0f;
        REAL knobX = toggles[i].state ? (pillX + pillW - knobSize - 2.0f * s) : (pillX + 2.0f * s);

        SolidBrush knobBrush(Color(255, 255, 255, 255));
        graphics.FillEllipse(&knobBrush, knobX, knobY, knobSize, knobSize);
    }

    // ── Right Column: Compact Action Buttons ─────────────────────────────────
    FontFamily fontMDL2(L"Segoe MDL2 Assets");
    Font iconFont(&fontMDL2, 11.0f * s, FontStyleRegular, UnitPixel);
    Font btnFont(&fontFamily, 11.5f * s, FontStyleBold, UnitPixel);

    struct ActionBtn {
        const wchar_t* icon;
        const wchar_t* label;
        int id;
    } buttons[5] = {
        { L"\uE787", L"Open Full Calendar", 7 },
        { L"\uE790", L"Background Color", 101 },
        { L"\uE72C", L"Check for Updates", 4 },
        { L"\uEB51", L"Support / Donate", 6 },
        { L"\uE711", L"Exit Widget", 2 }
    };

    for (int i = 0; i < 5; i++) {
        REAL btnX = 184.0f * s;
        REAL btnY = (48.0f + i * 39.0f) * s;
        REAL btnW = 185.0f * s;
        REAL btnH = 33.0f * s;

        bool isHovered  = (g_menuHoverIndex == (3 + i));
        bool isBgActive = (g_bgFlyoutOpen && buttons[i].id == 101);

        Color btnBg;
        if (isBgActive) {
            btnBg = Color(90, 220, 38, 38);
        } else if (g_isLightTheme) {
            btnBg = isHovered ? Color(60, 0, 0, 0) : Color(25, 0, 0, 0);
        } else {
            btnBg = isHovered ? Color(80, 255, 255, 255) : Color(35, 255, 255, 255);
        }

        SolidBrush btnBrush(btnBg);
        GraphicsPath btnPath;
        REAL br = 6.0f * s;
        REAL bd = br * 2.0f;
        btnPath.AddArc(btnX, btnY, bd, bd, 180.0f, 90.0f);
        btnPath.AddArc(btnX + btnW - bd, btnY, bd, bd, 270.0f, 90.0f);
        btnPath.AddArc(btnX + btnW - bd, btnY + btnH - bd, bd, bd, 0.0f, 90.0f);
        btnPath.AddArc(btnX, btnY + btnH - bd, bd, bd, 90.0f, 90.0f);
        btnPath.CloseFigure();

        graphics.FillPath(&btnBrush, &btnPath);

        // Icon (Segoe MDL2 Assets glyph)
        RectF iconRect(btnX + 6.0f * s, btnY, 20.0f * s, btnH);
        graphics.DrawString(buttons[i].icon, -1, &iconFont, iconRect, &formatCenter, &textBrush);

        // Text Label
        RectF btnTextRect(btnX + 28.0f * s, btnY, btnW - 32.0f * s, btnH);
        graphics.DrawString(buttons[i].label, -1, &btnFont, btnTextRect, &formatNear, &textBrush);
    }

    // ── BG Colour Flyout Panel ────────────────────────────────────────────────
    if (g_bgFlyoutOpen) {
        // Layout constants (all unscaled px, multiply by s for actual pixels)
        // Menu card = 385 wide. Arrow = 12. Flyout card = 200. Total = 597.
        const float arrowW  = 12.0f;
        const float flyW    = 200.0f;
        const float flyX    = 385.0f;   // bitmap-local x where arrow starts
        const float cardX   = flyX + arrowW;
        const float cardW   = flyW;
        const float cardH   = 260.0f;   // same height as menu
        const float cardR   = 12.0f;

        // Speech-bubble arrow tip aligned to "Background Color" button centre
        // Button index 1 in right column: Y = 48 + 1*39 = 87, H = 33 → centre = 103.5
        float tipY  = 103.5f;
        float topY  = tipY - 9.0f;
        float botY  = tipY + 9.0f;

        // ── Card + arrow path ──
        GraphicsPath flyPath;
        float cr = cardR * s, cd = cr * 2.0f;
        float cx = cardX * s, cy2 = 1.0f * s, cw = cardW * s, ch = cardH * s;
        flyPath.AddArc(cx, cy2, cd, cd, 180, 90);
        flyPath.AddArc(cx + cw - cd, cy2, cd, cd, 270, 90);
        flyPath.AddArc(cx + cw - cd, cy2 + ch - cd, cd, cd, 0, 90);
        flyPath.AddArc(cx, cy2 + ch - cd, cd, cd, 90, 90);
        flyPath.AddLine(cx, cy2 + ch - cr, cx, botY * s);
        flyPath.AddLine(cx, botY * s, flyX * s, tipY * s);
        flyPath.AddLine(flyX * s, tipY * s, cx, topY * s);
        flyPath.AddLine(cx, topY * s, cx, cy2 + cr);
        flyPath.CloseFigure();

        SolidBrush flyBg(g_isLightTheme ? Color(252, 250, 250, 252) : Color(252, 26, 26, 32));
        graphics.FillPath(&flyBg, &flyPath);
        Pen flyBorder(g_isLightTheme ? Color(50, 0, 0, 0) : Color(60, 255, 255, 255), 1.0f * s);
        graphics.DrawPath(&flyBorder, &flyPath);

        // ── Text brushes / fonts ──
        FontFamily ffam(L"Segoe UI");
        Font flyTitleFont(&ffam, 12.5f * s, FontStyleBold,    UnitPixel);
        Font flySubFont  (&ffam, 10.0f * s, FontStyleRegular, UnitPixel);
        Font swLblFont   (&ffam,  9.0f * s, FontStyleRegular, UnitPixel);
        Font sldrLblFont (&ffam, 10.0f * s, FontStyleBold,    UnitPixel);

        SolidBrush brPrimary(g_isLightTheme ? Color(255, 20,  20,  20 ) : Color(255, 250, 250, 255));
        SolidBrush brSub    (g_isLightTheme ? Color(160, 80,  80,  80 ) : Color(160, 170, 170, 180));

        StringFormat sfLeft, sfCtr;
        sfLeft.SetAlignment(StringAlignmentNear);   sfLeft.SetLineAlignment(StringAlignmentCenter);
        sfCtr .SetAlignment(StringAlignmentCenter); sfCtr .SetLineAlignment(StringAlignmentCenter);

        float ix = (cardX + 14.0f) * s;
        float iw = (cardW - 28.0f) * s;

        // ── Header ──
        RectF rTitle(ix, 12.0f * s, iw, 20.0f * s);
        graphics.DrawString(L"\uE790  Background", -1, &flyTitleFont, rTitle, &sfLeft, &brPrimary);
        RectF rSub(ix, 33.0f * s, iw, 14.0f * s);
        graphics.DrawString(L"Widget colour preset", -1, &flySubFont, rSub, &sfLeft, &brSub);

        // ── Divider ──
        Pen divPen(g_isLightTheme ? Color(25, 0, 0, 0) : Color(40, 255, 255, 255), 1.0f * s);
        graphics.DrawLine(&divPen, ix, 52.0f * s, (cardX + cardW - 14.0f) * s, 52.0f * s);

        // ── Swatches (4 cols × 2 rows = 8 presets) ──
        struct SwatchDef { int preset; const wchar_t* label; BYTE r, g, b; bool isClear; bool isCustom; };
        SwatchDef sw[8] = {
            { 0, L"Clear",  0,   0,   0,   true,  false },
            { 2, L"Dark",   31,  31,  31,  false, false },
            { 3, L"Light",  230, 230, 230, false, false },
            { 4, L"Black",  0,   0,   0,   false, false },
            { 5, L"White",  255, 255, 255, false, false },
            { 6, L"Red",    180, 40,  40,  false, false },
            { 7, L"Blue",   30,  110, 200, false, false },
            { 1, L"Custom", 0,   0,   0,   false, true  },
        };

        float swSz   = 36.0f * s;
        float swGapX =  6.0f * s;
        float swGapY =  8.0f * s;
        float swLblH = 13.0f * s;
        float gridW  = 4.0f * swSz + 3.0f * swGapX;
        float gridX  = cx + (cw - gridW) / 2.0f;
        float gridY  = 59.0f * s;

        for (int i = 0; i < 8; i++) {
            int   col = i % 4,  row = i / 4;
            float sx  = gridX + col * (swSz + swGapX);
            float sy  = gridY + row * (swSz + swLblH + swGapY);

            bool isActive = (g_bgPresetMode == sw[i].preset);
            bool isHov    = (g_bgFlyoutHover == i);

            // Rounded swatch path
            GraphicsPath swPath;
            float sr = 7.0f * s, sd = sr * 2.0f;
            swPath.AddArc(sx, sy, sd, sd, 180, 90);
            swPath.AddArc(sx + swSz - sd, sy, sd, sd, 270, 90);
            swPath.AddArc(sx + swSz - sd, sy + swSz - sd, sd, sd, 0, 90);
            swPath.AddArc(sx, sy + swSz - sd, sd, sd, 90, 90);
            swPath.CloseFigure();

            if (sw[i].isClear) {
                // Checkered "no colour" pattern
                SolidBrush chA(Color(255, 200, 200, 200)), chB(Color(255, 155, 155, 155));
                int cells = 4;
                float csz = swSz / cells;
                for (int ci = 0; ci < cells; ci++)
                    for (int cj = 0; cj < cells; cj++) {
                        SolidBrush& cb = ((ci + cj) % 2 == 0) ? chA : chB;
                        graphics.FillRectangle(&cb, sx + ci * csz, sy + cj * csz, csz, csz);
                    }
                // Slash line
                Pen slashPen(Color(180, 200, 60, 60), 2.0f * s);
                graphics.DrawLine(&slashPen, sx + 4.0f * s, sy + swSz - 4.0f * s,
                                             sx + swSz - 4.0f * s, sy + 4.0f * s);
            } else if (sw[i].isCustom) {
                BYTE cr2 = GetRValue(g_bgColorRGB), cg2 = GetGValue(g_bgColorRGB), cb2 = GetBValue(g_bgColorRGB);
                SolidBrush custBr(Color(255, cr2, cg2, cb2));
                graphics.FillPath(&custBr, &swPath);
                // "…" overlay
                Font dotFont(&ffam, 14.0f * s, FontStyleBold, UnitPixel);
                SolidBrush dotBr(Color(200, 255, 255, 255));
                RectF dotR(sx, sy, swSz, swSz);
                graphics.DrawString(L"\u2026", -1, &dotFont, dotR, &sfCtr, &dotBr);
            } else {
                SolidBrush swBr(Color(255, sw[i].r, sw[i].g, sw[i].b));
                graphics.FillPath(&swBr, &swPath);
            }

            // Border — active = red ring, hovered = subtle, normal = faint
            if (isActive) {
                Pen ringPen(Color(255, 220, 38, 38), 2.5f * s);
                graphics.DrawPath(&ringPen, &swPath);
            } else if (isHov) {
                Pen hovPen(g_isLightTheme ? Color(140, 0, 0, 0) : Color(160, 255, 255, 255), 1.5f * s);
                graphics.DrawPath(&hovPen, &swPath);
            } else {
                Pen normPen(g_isLightTheme ? Color(40, 0, 0, 0) : Color(55, 255, 255, 255), 1.0f * s);
                graphics.DrawPath(&normPen, &swPath);
            }

            // Label
            SolidBrush lblBr(isActive ? Color(255, 220, 38, 38) :
                (g_isLightTheme ? Color(170, 60, 60, 60) : Color(170, 170, 170, 175)));
            RectF lblR(sx, sy + swSz + 1.0f * s, swSz, swLblH);
            graphics.DrawString(sw[i].label, -1, &swLblFont, lblR, &sfCtr, &lblBr);
        }

        // ── Opacity Slider ──
        // Section starts after 2 rows of swatches
        float slSecY  = gridY + 2.0f * (swSz + swLblH + swGapY) + 6.0f * s;
        float slX     = ix;
        float slW     = iw;
        float trackY  = slSecY + 18.0f * s;
        float trackH  = 4.0f * s;
        float knobSz  = 13.0f * s;
        BYTE  slAlpha = (g_bgPresetMode == 0) ? 55 : 200;

        // Opacity label + percentage
        wchar_t opLbl[24];
        int pct = (g_bgPresetMode == 0) ? 0 : (int)(g_bgOpacity * 100 / 255);
        swprintf(opLbl, 24, L"Opacity  %d%%", pct);
        SolidBrush slLblBr(Color(slAlpha + 55, g_isLightTheme ? 40 : 180, g_isLightTheme ? 40 : 180, g_isLightTheme ? 40 : 185));
        RectF slLblR(slX, slSecY, slW, 16.0f * s);
        graphics.DrawString(opLbl, -1, &sldrLblFont, slLblR, &sfLeft, &slLblBr);

        // Track
        SolidBrush trackBg(g_isLightTheme ? Color(slAlpha, 180, 180, 185) : Color(slAlpha, 70, 70, 80));
        RectF trackR(slX, trackY, slW, trackH);
        graphics.FillRectangle(&trackBg, trackR);

        // Fill
        if (g_bgPresetMode != 0 && g_bgOpacity > 0) {
            float fillW = slW * (float)g_bgOpacity / 255.0f;
            SolidBrush fillBr(Color(220, 220, 38, 38));
            graphics.FillRectangle(&fillBr, slX, trackY, fillW, trackH);
        }

        // Knob
        if (g_bgPresetMode != 0) {
            float knobX = slX + slW * (float)g_bgOpacity / 255.0f - knobSz / 2.0f;
            if (knobX < slX) knobX = slX;
            if (knobX + knobSz > slX + slW) knobX = slX + slW - knobSz;
            float knobY = trackY + trackH / 2.0f - knobSz / 2.0f;
            SolidBrush knobBr(Color(255, 255, 255, 255));
            graphics.FillEllipse(&knobBr, knobX, knobY, knobSz, knobSz);
            Pen knobBd(Color(80, 0, 0, 0), 1.0f * s);
            graphics.DrawEllipse(&knobBd, knobX, knobY, knobSz, knobSz);
        }

        // ── Footer ──
        Font footFont(&ffam, 8.5f * s, FontStyleRegular, UnitPixel);
        SolidBrush footBr(g_isLightTheme ? Color(90, 80, 80, 80) : Color(90, 160, 160, 170));
        RectF footR(cx + 8.0f * s, 243.0f * s, cw - 16.0f * s, 14.0f * s);
        graphics.DrawString(L"Tap swatch to apply instantly", -1, &footFont, footR, &sfCtr, &footBr);
    }

    // Apply per-pixel alpha channel to OS Window
    // When flyout is open, make sure the extended window doesn't overflow the right edge.
    // When flyout is closed, restore the menu to its original (base) position.
    POINT ptDst = { g_menuBaseXPos, g_menuYPos };
    if (g_bgFlyoutOpen) {
        HMONITOR hMon = MonitorFromPoint(ptDst, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hMon, &mi);
        if (ptDst.x + rawWidth > mi.rcWork.right - (int)(6 * s)) {
            ptDst.x = mi.rcWork.right - rawWidth - (int)(6 * s);
            if (ptDst.x < mi.rcWork.left) ptDst.x = mi.rcWork.left;
        }
    }
    g_menuXPos = ptDst.x;   // keep in sync with actual window position for hit-testing
    SIZE sizeDst = { rawWidth, rawHeight };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeDst, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK CustomMenuWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RenderCustomMenu(hWnd);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt;
        GetCursorPos(&pt);
        int localX = pt.x - g_menuXPos;
        int localY = pt.y - g_menuYPos;

        float s = g_dpiScale;

        // If slider is being dragged, update opacity live
        if (g_bgSliderDrag && g_bgPresetMode != 0) {
            float slX = (385.0f + 12.0f + 14.0f) * s;
            float slW = (200.0f - 28.0f) * s;
            float frac = ((float)localX - slX) / slW;
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;
            g_bgOpacity = (int)(frac * 255.0f);
            SaveConfig();
            RenderWidget(g_hWnd);
            RenderCustomMenu(hWnd);
            break;
        }

        // Menu card hover (left/right columns)
        int newHover = -1;
        bool showZIndexOption = (!g_isOnTaskbar && !g_setupMode);
        if (localX >= (int)(14 * s) && localX <= (int)(168 * s)) {
            if (showZIndexOption) {
                if      (localY >= (int)(46 * s)  && localY < (int)(95 * s))  newHover = 0;
                else if (localY >= (int)(95 * s)  && localY < (int)(144 * s)) newHover = 1;
                else if (localY >= (int)(144 * s) && localY < (int)(193 * s)) newHover = 2;
                else if (localY >= (int)(193 * s) && localY <= (int)(244 * s)) newHover = 8;
            } else {
                if      (localY >= (int)(48 * s)  && localY < (int)(108 * s)) newHover = 0;
                else if (localY >= (int)(108 * s) && localY < (int)(168 * s)) newHover = 1;
                else if (localY >= (int)(168 * s) && localY <= (int)(228 * s)) newHover = 2;
            }
        } else if (localX >= (int)(176 * s) && localX <= (int)(375 * s)) {
            if      (localY >= (int)(48 * s)  && localY < (int)(84 * s))  newHover = 3;
            else if (localY >= (int)(88 * s)  && localY < (int)(124 * s)) newHover = 4;
            else if (localY >= (int)(128 * s) && localY < (int)(164 * s)) newHover = 5;
            else if (localY >= (int)(168 * s) && localY < (int)(204 * s)) newHover = 6;
            else if (localY >= (int)(208 * s) && localY <= (int)(244 * s)) newHover = 7;
        }
        if (newHover != g_menuHoverIndex) {
            g_menuHoverIndex = newHover;
            RenderCustomMenu(hWnd);
        }

        // Flyout swatch hover
        if (g_bgFlyoutOpen) {
            float cx    = (385.0f + 12.0f) * s;
            float cw    = 200.0f * s;
            float swSz  = 36.0f * s, swGapX = 6.0f * s, swGapY = 8.0f * s, swLblH = 13.0f * s;
            float gridW = 4.0f * swSz + 3.0f * swGapX;
            float gridX = cx + (cw - gridW) / 2.0f;
            float gridY = 59.0f * s;

            // Slider hit zone
            float slSecY = gridY + 2.0f * (swSz + swLblH + swGapY) + 6.0f * s;
            float trackY = slSecY + 18.0f * s;
            float slX    = (385.0f + 12.0f + 14.0f) * s;
            float slW    = (200.0f - 28.0f) * s;

            int newFlyHov = -1;
            for (int i = 0; i < 8; i++) {
                float sx = gridX + (i % 4) * (swSz + swGapX);
                float sy = gridY + (i / 4) * (swSz + swLblH + swGapY);
                if (localX >= (int)sx && localX <= (int)(sx + swSz) &&
                    localY >= (int)sy && localY <= (int)(sy + swSz)) {
                    newFlyHov = i; break;
                }
            }
            if (newFlyHov == -1 &&
                localX >= (int)slX && localX <= (int)(slX + slW) &&
                localY >= (int)(trackY - 8.0f * s) && localY <= (int)(trackY + 12.0f * s)) {
                newFlyHov = 8;
            }
            if (newFlyHov != g_bgFlyoutHover) {
                g_bgFlyoutHover = newFlyHov;
                RenderCustomMenu(hWnd);
            }
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        if (g_bgFlyoutOpen && g_bgPresetMode != 0) {
            POINT pt2; GetCursorPos(&pt2);
            int lx = pt2.x - g_menuXPos;
            int ly = pt2.y - g_menuYPos;
            float s2 = g_dpiScale;
            float swSz = 36.0f * s2, swGapX = 6.0f * s2, swGapY = 8.0f * s2, swLblH = 13.0f * s2;
            float gridY = 59.0f * s2;
            float slSecY = gridY + 2.0f * (swSz + swLblH + swGapY) + 6.0f * s2;
            float trackY = slSecY + 18.0f * s2;
            float slX    = (385.0f + 12.0f + 14.0f) * s2;
            float slW    = (200.0f - 28.0f) * s2;
            if (lx >= (int)slX && lx <= (int)(slX + slW) &&
                ly >= (int)(trackY - 8.0f * s2) && ly <= (int)(trackY + 12.0f * s2)) {
                g_bgSliderDrag = true;
                SetCapture(hWnd);
                float frac = ((float)lx - slX) / slW;
                if (frac < 0.0f) frac = 0.0f;
                if (frac > 1.0f) frac = 1.0f;
                g_bgOpacity = (int)(frac * 255.0f);
                SaveConfig();
                RenderWidget(g_hWnd);
                RenderCustomMenu(hWnd);
            }
        }
        break;
    }


    case WM_LBUTTONUP: {
        // Stop slider drag
        if (g_bgSliderDrag) {
            g_bgSliderDrag = false;
            ReleaseCapture();
            break;
        }

        POINT pt;
        GetCursorPos(&pt);
        int localX = pt.x - g_menuXPos;
        int localY = pt.y - g_menuYPos;
        float s = g_dpiScale;

        // ── Flyout swatch clicks — only when click is INSIDE the flyout area ──
        if (g_bgFlyoutOpen && localX >= (int)(385.0f * s)) {
            float cx    = (385.0f + 12.0f) * s;
            float cw    = 200.0f * s;
            float swSz  = 36.0f * s, swGapX = 6.0f * s, swGapY = 8.0f * s, swLblH = 13.0f * s;
            float gridW = 4.0f * swSz + 3.0f * swGapX;
            float gridX = cx + (cw - gridW) / 2.0f;
            float gridY = 59.0f * s;
            int   swPresets[8] = { 0, 2, 3, 4, 5, 6, 7, 1 };
            bool  swCustom [8] = { false,false,false,false,false,false,false,true };

            for (int i = 0; i < 8; i++) {
                float sx = gridX + (i % 4) * (swSz + swGapX);
                float sy = gridY + (i / 4) * (swSz + swLblH + swGapY);
                if (localX >= (int)sx && localX <= (int)(sx + swSz) &&
                    localY >= (int)sy && localY <= (int)(sy + swSz)) {
                    if (swCustom[i]) {
                        g_bgFlyoutOpen = false;
                        HideCustomMenu();
                        PickCustomBackgroundColor(g_hWnd);
                    } else {
                        g_bgPresetMode = swPresets[i];
                        SaveConfig();
                        RenderWidget(g_hWnd);
                        RenderCustomMenu(hWnd);
                    }
                    break;
                }
            }
            break;  // click was in flyout zone — don't process menu card
        }
        // Click was on the menu card (or gap) — fall through to menu card handling below

        // ── Menu card clicks ──────────────────────────────────────────────────
        int clicked = -1;
        bool showZIndexOption = (!g_isOnTaskbar && !g_setupMode);
        if (localX >= (int)(14 * s) && localX <= (int)(168 * s)) {
            if (showZIndexOption) {
                if      (localY >= (int)(46 * s)  && localY < (int)(95 * s))  clicked = 0;
                else if (localY >= (int)(95 * s)  && localY < (int)(144 * s)) clicked = 1;
                else if (localY >= (int)(144 * s) && localY < (int)(193 * s)) clicked = 2;
                else if (localY >= (int)(193 * s) && localY <= (int)(244 * s)) clicked = 8;
            } else {
                if      (localY >= (int)(48 * s)  && localY < (int)(108 * s)) clicked = 0;
                else if (localY >= (int)(108 * s) && localY < (int)(168 * s)) clicked = 1;
                else if (localY >= (int)(168 * s) && localY <= (int)(228 * s)) clicked = 2;
            }
        } else if (localX >= (int)(176 * s) && localX <= (int)(375 * s)) {
            if      (localY >= (int)(48 * s)  && localY < (int)(84 * s))  clicked = 3;
            else if (localY >= (int)(88 * s)  && localY < (int)(124 * s)) clicked = 4;
            else if (localY >= (int)(128 * s) && localY < (int)(164 * s)) clicked = 5;
            else if (localY >= (int)(168 * s) && localY < (int)(204 * s)) clicked = 6;
            else if (localY >= (int)(208 * s) && localY <= (int)(244 * s)) clicked = 7;
        }

        if (clicked == 0) {
            SetStartupEnabled(!IsStartupEnabled());
            RenderCustomMenu(hWnd);
        } else if (clicked == 1) {
            g_showDay = !g_showDay;
            UpdateWidgetMode();
            SaveConfig();
            RenderWidget(g_hWnd);
            RenderCustomMenu(hWnd);
        } else if (clicked == 2) {
            g_setupMode = !g_setupMode;
            if (g_setupMode && g_isCalendarOpen) HideCalendar();
            UpdateWidgetMode();
            SaveConfig();
            RenderWidget(g_hWnd);
            RenderCustomMenu(hWnd);
        } else if (clicked == 8) {
            g_zIndexDesktop = !g_zIndexDesktop;
            UpdateWidgetMode();
            SaveConfig();
            RenderWidget(g_hWnd);
            RenderCustomMenu(hWnd);
        } else if (clicked == 3) {
            HideCustomMenu();
            ShowCalendar(g_hWnd);
        } else if (clicked == 4) {
            // Toggle BG colour flyout (does NOT close the menu)
            g_bgFlyoutOpen  = !g_bgFlyoutOpen;
            g_bgFlyoutHover = -1;
            RenderCustomMenu(hWnd);
        } else if (clicked == 5) {
            HideCustomMenu();
            HANDLE hThread = CreateThread(NULL, 0, CheckForUpdateThread, (LPVOID)TRUE, 0, NULL);
            if (hThread) CloseHandle(hThread);
        } else if (clicked == 6) {
            HideCustomMenu();
            ShellExecuteW(NULL, L"open", L"https://tithify.guptaaayush.com.np/#donate", NULL, NULL, SW_SHOWNORMAL);
        } else if (clicked == 7) {
            HideCustomMenu();
            PostQuitMessage(0);
        }
        break;
    }

    case WM_KILLFOCUS:
        HideCustomMenu();
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            HideCustomMenu();
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            HideCustomMenu();
        }
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowCustomMenu(HWND hWndOwner, POINT pt) {
    if (!g_hMenuWnd) return;

    float s = g_dpiScale;
    int menuW = (int)(385 * s);
    int menuH = (int)(260 * s);

    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    RECT rcWidget = { 0 };
    if (hWndOwner && IsWindow(hWndOwner)) {
        GetWindowRect(hWndOwner, &rcWidget);
    }

    int x = 0, y = 0;

    if (hWndOwner && PtInRect(&rcWidget, pt)) {
        // Right-clicked directly on the Tithify widget: center horizontally and float 10px above it
        int widgetCenterX = rcWidget.left + (rcWidget.right - rcWidget.left) / 2;
        x = widgetCenterX - menuW / 2;
        y = rcWidget.top - menuH - (int)(10 * s);

        if (y < mi.rcWork.top) {
            y = rcWidget.bottom + (int)(10 * s);
        }
    } else {
        // Right-clicked on tray icon (in taskbar notification area or overflow flyout)
        // Find the root window under the cursor (e.g. overflow flyout or taskbar)
        HWND hUnder = WindowFromPoint(pt);
        HWND hRoot = hUnder ? GetAncestor(hUnder, GA_ROOT) : NULL;
        RECT rcAnchor = { pt.x, pt.y, pt.x, pt.y };
        if (hRoot && hRoot != g_hWnd && hRoot != g_hMenuWnd && hRoot != g_hCalWnd) {
            GetWindowRect(hRoot, &rcAnchor);
        }

        x = pt.x - menuW / 2;
        // Position cleanly above the entire anchor box (overflow flyout or taskbar)
        y = rcAnchor.top - menuH - (int)(10 * s);

        if (y < mi.rcWork.top) {
            // If placed too high, position below the anchor box
            y = rcAnchor.bottom + (int)(10 * s);
        }
    }

    // Clamp inside vertical screen bounds
    if (y + menuH > mi.rcWork.bottom) {
        y = mi.rcWork.bottom - menuH - (int)(8 * s);
    }
    if (y < mi.rcWork.top) {
        y = mi.rcWork.top + (int)(8 * s);
    }

    // Clamp inside horizontal screen bounds
    if (x + menuW > mi.rcWork.right - (int)(10 * s)) {
        x = mi.rcWork.right - menuW - (int)(10 * s);
    }
    if (x < mi.rcWork.left + (int)(10 * s)) {
        x = mi.rcWork.left + (int)(10 * s);
    }

    g_menuXPos     = x;
    g_menuBaseXPos = x;   // remember original position for flyout restore
    g_menuYPos = y;
    g_menuHoverIndex = -1;
    g_bgFlyoutOpen   = false;
    g_bgFlyoutHover  = -1;
    g_bgSliderDrag   = false;
    g_isMenuOpen     = true;

    SetWindowPos(g_hMenuWnd, HWND_TOPMOST, g_menuXPos, g_menuYPos, menuW, menuH, SWP_NOACTIVATE);
    RenderCustomMenu(g_hMenuWnd);
    ShowWindow(g_hMenuWnd, SW_SHOWNOACTIVATE);
    SetForegroundWindow(g_hMenuWnd);
    SetFocus(g_hMenuWnd);
}

void ShowContextMenu(HWND hWnd, POINT pt) {
    ShowCustomMenu(hWnd, pt);
}

// ── Modern Fluent Dialog Engine ──────────────────────────────────────────────
enum ModernDialogType {
    MODERN_DLG_SUCCESS = 0,
    MODERN_DLG_UPDATE  = 1,
    MODERN_DLG_ERROR   = 2,
    MODERN_DLG_INFO    = 3
};

struct ModernDialogState {
    bool isOpen = false;
    int result = IDCANCEL;
    int hoverBtn = 0; // 0=none, 1=primary, 2=secondary, 99=close
    bool isDragging = false;
    POINT dragStart = { 0, 0 };
    int dlgX = 0;
    int dlgY = 0;
    int dlgW = 0;
    int dlgH = 0;

    wchar_t title[128] = { 0 };
    wchar_t message[512] = { 0 };
    wchar_t primaryBtn[64] = { 0 };
    wchar_t secondaryBtn[64] = { 0 };
    bool hasSecondary = false;
    int dialogType = 0;
} g_dlgState;

void RenderModernDialog(HWND hWnd) {
    if (!hWnd) return;

    int rawW = g_dlgState.dlgW;
    int rawH = g_dlgState.dlgH;
    float s = g_dpiScale;
    bool isLight = g_isLightTheme;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rawW;
    bmi.bmiHeader.biHeight = -rawH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Graphics g(hdcMem);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    g.Clear(Color(0, 0, 0, 0));

    REAL w = (REAL)rawW;
    REAL h = (REAL)rawH;

    // ── Load Application Logo ──
    HICON hIcon = (HICON)LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(1),
                                    IMAGE_ICON, (int)(64 * s), (int)(64 * s), LR_DEFAULTCOLOR);
    if (!hIcon) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            wcscat_s(exePath, L"icon.ico");
            hIcon = (HICON)LoadImageW(NULL, exePath, IMAGE_ICON, (int)(64 * s), (int)(64 * s), LR_LOADFROMFILE);
        }
    }
    Bitmap* pLogo = hIcon ? Bitmap::FromHICON(hIcon) : NULL;

    // ── Main Card Background (Solid high-contrast graphite in dark mode) ──
    Color bgCardColor = isLight ? Color(255, 255, 255, 255) : Color(255, 26, 27, 32);
    Color borderColor = isLight ? Color(60, 0, 0, 0) : Color(75, 255, 255, 255);

    GraphicsPath cardPath;
    REAL r = 16.0f * s;
    REAL d = r * 2.0f;
    cardPath.AddArc(0.0f, 0.0f, d, d, 180.0f, 90.0f);
    cardPath.AddArc(w - d, 0.0f, d, d, 270.0f, 90.0f);
    cardPath.AddArc(w - d, h - d, d, d, 0.0f, 90.0f);
    cardPath.AddArc(0.0f, h - d, d, d, 90.0f, 90.0f);
    cardPath.CloseFigure();

    SolidBrush cardBrush(bgCardColor);
    g.FillPath(&cardBrush, &cardPath);
    Pen borderPen(borderColor, 1.5f * s);
    g.DrawPath(&borderPen, &cardPath);

    // ── Header Bar ──
    REAL headerH = 42.0f * s;
    REAL hLogoSize = 22.0f * s;
    REAL hLogoX = 18.0f * s;
    REAL hLogoY = (headerH - hLogoSize) / 2.0f;
    if (pLogo && pLogo->GetWidth() > 0) {
        g.DrawImage(pLogo, RectF(hLogoX, hLogoY, hLogoSize, hLogoSize));
    }

    FontFamily fontFamily(L"Segoe UI");
    Font brandFont(&fontFamily, 13.0f * s, FontStyleBold, UnitPixel);
    Color brandColor = isLight ? Color(255, 20, 20, 20) : Color(255, 255, 255, 255);
    SolidBrush brandBrush(brandColor);

    StringFormat fmtNearCenter;
    fmtNearCenter.SetAlignment(StringAlignmentNear);
    fmtNearCenter.SetLineAlignment(StringAlignmentCenter);

    RectF brandRect(hLogoX + hLogoSize + 10.0f * s, 0.0f, 200.0f * s, headerH);
    g.DrawString(L"Tithify", -1, &brandFont, brandRect, &fmtNearCenter, &brandBrush);

    // Close button [X]
    REAL closeBtnSize = 26.0f * s;
    REAL closeBtnX = w - 16.0f * s - closeBtnSize;
    REAL closeBtnY = (headerH - closeBtnSize) / 2.0f;
    if (g_dlgState.hoverBtn == 99) {
        SolidBrush hoverCloseBrush(isLight ? Color(30, 0, 0, 0) : Color(45, 255, 255, 255));
        g.FillEllipse(&hoverCloseBrush, closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
    }
    Font closeFont(&fontFamily, 12.0f * s, FontStyleRegular, UnitPixel);
    StringFormat fmtCenter;
    fmtCenter.SetAlignment(StringAlignmentCenter);
    fmtCenter.SetLineAlignment(StringAlignmentCenter);
    RectF closeRect(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
    SolidBrush closeBrush(isLight ? Color(180, 80, 80, 80) : Color(220, 200, 200, 200));
    g.DrawString(L"\u2715", -1, &closeFont, closeRect, &fmtCenter, &closeBrush);

    Color divColor = isLight ? Color(30, 0, 0, 0) : Color(35, 255, 255, 255);
    Pen divPen(divColor, 1.0f * s);
    g.DrawLine(&divPen, 16.0f * s, headerH, w - 16.0f * s, headerH);

    // ── Body Area: App Logo + Status Badge ──
    REAL iconBoxX = 22.0f * s;
    REAL iconBoxY = headerH + 20.0f * s;
    REAL iconBoxSize = 54.0f * s;

    if (pLogo && pLogo->GetWidth() > 0) {
        g.DrawImage(pLogo, RectF(iconBoxX, iconBoxY, iconBoxSize, iconBoxSize));
    }

    // Corner Status Badge Overlay
    REAL chkSize = 22.0f * s;
    REAL chkX = iconBoxX + iconBoxSize - chkSize + 3.0f * s;
    REAL chkY = iconBoxY + iconBoxSize - chkSize + 3.0f * s;

    Color badgeColor = (g_dlgState.dialogType == MODERN_DLG_SUCCESS)
        ? Color(255, 16, 185, 129) // emerald
        : ((g_dlgState.dialogType == MODERN_DLG_UPDATE) ? Color(255, 220, 38, 38) : Color(255, 239, 68, 68)); // red or amber-red

    SolidBrush chkBgBrush(badgeColor);
    g.FillEllipse(&chkBgBrush, chkX, chkY, chkSize, chkSize);
    Pen chkRingPen(bgCardColor, 2.2f * s);
    g.DrawEllipse(&chkRingPen, chkX, chkY, chkSize, chkSize);

    REAL ccx = chkX + chkSize / 2.0f;
    REAL ccy = chkY + chkSize / 2.0f;
    if (g_dlgState.dialogType == MODERN_DLG_SUCCESS) {
        // White Checkmark
        Pen checkPen(Color(255, 255, 255, 255), 2.2f * s);
        checkPen.SetStartCap(LineCapRound);
        checkPen.SetEndCap(LineCapRound);
        checkPen.SetLineJoin(LineJoinRound);
        PointF pt1(ccx - 4.5f * s, ccy + 0.0f * s);
        PointF pt2(ccx - 1.0f * s, ccy + 3.5f * s);
        PointF pt3(ccx + 4.5f * s, ccy - 3.0f * s);
        PointF pts[3] = { pt1, pt2, pt3 };
        g.DrawLines(&checkPen, pts, 3);
    } else if (g_dlgState.dialogType == MODERN_DLG_UPDATE) {
        // White Download Arrow
        Pen arrowPen(Color(255, 255, 255, 255), 2.0f * s);
        arrowPen.SetStartCap(LineCapRound);
        arrowPen.SetEndCap(LineCapRound);
        g.DrawLine(&arrowPen, ccx, ccy - 4.5f * s, ccx, ccy + 3.5f * s);
        PointF h1(ccx - 3.0f * s, ccy + 1.0f * s);
        PointF h2(ccx, ccy + 3.8f * s);
        PointF h3(ccx + 3.0f * s, ccy + 1.0f * s);
        PointF hPts[3] = { h1, h2, h3 };
        g.DrawLines(&arrowPen, hPts, 3);
    } else {
        // White Cross
        Pen crossPen(Color(255, 255, 255, 255), 2.0f * s);
        crossPen.SetStartCap(LineCapRound);
        crossPen.SetEndCap(LineCapRound);
        REAL cr = 3.5f * s;
        g.DrawLine(&crossPen, ccx - cr, ccy - cr, ccx + cr, ccy + cr);
        g.DrawLine(&crossPen, ccx + cr, ccy - cr, ccx - cr, ccy + cr);
    }

    // ── Typography: Large, Bold, Ultra-High Contrast ──
    REAL textX = iconBoxX + iconBoxSize + 18.0f * s;
    REAL textW = w - textX - 22.0f * s;

    Font titleFont(&fontFamily, 17.5f * s, FontStyleBold, UnitPixel);
    Color titleColor = isLight ? Color(255, 15, 15, 15) : Color(255, 255, 255, 255);
    SolidBrush titleBrush(titleColor);
    RectF titleRect(textX, iconBoxY - 2.0f * s, textW, 26.0f * s);
    g.DrawString(g_dlgState.title, -1, &titleFont, titleRect, &fmtNearCenter, &titleBrush);

    Font msgFont(&fontFamily, 13.5f * s, FontStyleRegular, UnitPixel);
    // 92% luminance in dark mode (bright and crystal clear), dark slate in light mode
    Color msgColor = isLight ? Color(255, 60, 64, 72) : Color(255, 228, 232, 238);
    SolidBrush msgBrush(msgColor);

    StringFormat fmtWrap;
    fmtWrap.SetAlignment(StringAlignmentNear);
    fmtWrap.SetLineAlignment(StringAlignmentNear);
    RectF msgRect(textX, iconBoxY + 28.0f * s, textW, h - (iconBoxY + 28.0f * s) - 58.0f * s);
    g.DrawString(g_dlgState.message, -1, &msgFont, msgRect, &fmtWrap, &msgBrush);

    // ── Bottom Action Buttons ──
    REAL btnH = 38.0f * s;
    REAL btnY = h - 18.0f * s - btnH;
    REAL primaryW = g_dlgState.hasSecondary ? (118.0f * s) : (100.0f * s);
    REAL primaryX = w - 18.0f * s - primaryW;

    Font btnFont(&fontFamily, 13.5f * s, FontStyleBold, UnitPixel);

    if (g_dlgState.hasSecondary) {
        REAL secW = 94.0f * s;
        REAL secX = primaryX - 10.0f * s - secW;

        GraphicsPath secPath;
        REAL secR = 9.0f * s;
        REAL secD = secR * 2.0f;
        secPath.AddArc(secX, btnY, secD, secD, 180.0f, 90.0f);
        secPath.AddArc(secX + secW - secD, btnY, secD, secD, 270.0f, 90.0f);
        secPath.AddArc(secX + secW - secD, btnY + btnH - secD, secD, secD, 0.0f, 90.0f);
        secPath.AddArc(secX, btnY + btnH - secD, secD, secD, 90.0f, 90.0f);
        secPath.CloseFigure();

        Color secBg = (g_dlgState.hoverBtn == 2)
            ? (isLight ? Color(35, 0, 0, 0) : Color(45, 255, 255, 255))
            : (isLight ? Color(15, 0, 0, 0) : Color(20, 255, 255, 255));
        SolidBrush secBrush(secBg);
        g.FillPath(&secBrush, &secPath);

        Pen secBorder(isLight ? Color(50, 0, 0, 0) : Color(50, 255, 255, 255), 1.0f * s);
        g.DrawPath(&secBorder, &secPath);

        RectF secTextRect(secX, btnY, secW, btnH);
        SolidBrush secTextBrush(isLight ? Color(255, 30, 30, 30) : Color(255, 240, 240, 240));
        g.DrawString(g_dlgState.secondaryBtn, -1, &btnFont, secTextRect, &fmtCenter, &secTextBrush);
    }

    GraphicsPath primPath;
    REAL primR = 9.0f * s;
    REAL primD = primR * 2.0f;
    primPath.AddArc(primaryX, btnY, primD, primD, 180.0f, 90.0f);
    primPath.AddArc(primaryX + primaryW - primD, btnY, primD, primD, 270.0f, 90.0f);
    primPath.AddArc(primaryX + primaryW - primD, btnY + btnH - primD, primD, primD, 0.0f, 90.0f);
    primPath.AddArc(primaryX, btnY + btnH - primD, primD, primD, 90.0f, 90.0f);
    primPath.CloseFigure();

    Color primBg = (g_dlgState.hoverBtn == 1)
        ? Color(255, 239, 68, 68)  // hover lighter red
        : Color(255, 220, 38, 38);  // Tithify brand red
    SolidBrush primBrush(primBg);
    g.FillPath(&primBrush, &primPath);

    RectF primTextRect(primaryX, btnY, primaryW, btnH);
    SolidBrush primTextBrush(Color(255, 255, 255, 255));
    g.DrawString(g_dlgState.primaryBtn, -1, &btnFont, primTextRect, &fmtCenter, &primTextBrush);

    POINT ptDst = { g_dlgState.dlgX, g_dlgState.dlgY };
    SIZE sizeDst = { rawW, rawH };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(hWnd, hdcScreen, &ptDst, &sizeDst, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    if (pLogo) delete pLogo;
    if (hIcon) DestroyIcon(hIcon);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK ModernDialogWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR: {
        POINT pt;
        GetCursorPos(&pt);
        int lx = pt.x - g_dlgState.dlgX;
        int ly = pt.y - g_dlgState.dlgY;

        float s = g_dpiScale;
        int w = g_dlgState.dlgW;
        int h = g_dlgState.dlgH;
        int btnH = (int)(38 * s);
        int btnY = h - (int)(18 * s) - btnH;
        int primaryW = g_dlgState.hasSecondary ? (int)(118 * s) : (int)(100 * s);
        int primaryX = w - (int)(18 * s) - primaryW;

        bool onButton = false;
        if (lx >= primaryX && lx <= primaryX + primaryW && ly >= btnY && ly <= btnY + btnH) {
            onButton = true;
        } else if (g_dlgState.hasSecondary) {
            int secW = (int)(94 * s);
            int secX = primaryX - (int)(10 * s) - secW;
            if (lx >= secX && lx <= secX + secW && ly >= btnY && ly <= btnY + btnH) {
                onButton = true;
            }
        }
        int closeBtnSize = (int)(26 * s);
        int closeBtnX = w - (int)(16 * s) - closeBtnSize;
        int closeBtnY = (int)((42 * s - closeBtnSize) / 2);
        if (lx >= closeBtnX && lx <= closeBtnX + closeBtnSize && ly >= closeBtnY && ly <= closeBtnY + closeBtnSize) {
            onButton = true;
        }

        if (onButton) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }

    case WM_MOUSEMOVE: {
        POINT pt;
        GetCursorPos(&pt);
        int lx = pt.x - g_dlgState.dlgX;
        int ly = pt.y - g_dlgState.dlgY;

        if (g_dlgState.isDragging) {
            g_dlgState.dlgX = pt.x - g_dlgState.dragStart.x;
            g_dlgState.dlgY = pt.y - g_dlgState.dragStart.y;
            RenderModernDialog(hWnd);
            break;
        }

        float s = g_dpiScale;
        int w = g_dlgState.dlgW;
        int h = g_dlgState.dlgH;
        int btnH = (int)(38 * s);
        int btnY = h - (int)(18 * s) - btnH;
        int primaryW = g_dlgState.hasSecondary ? (int)(118 * s) : (int)(100 * s);
        int primaryX = w - (int)(18 * s) - primaryW;

        int newHover = 0;
        if (lx >= primaryX && lx <= primaryX + primaryW && ly >= btnY && ly <= btnY + btnH) {
            newHover = 1;
        } else if (g_dlgState.hasSecondary) {
            int secW = (int)(94 * s);
            int secX = primaryX - (int)(10 * s) - secW;
            if (lx >= secX && lx <= secX + secW && ly >= btnY && ly <= btnY + btnH) {
                newHover = 2;
            }
        }
        int closeBtnSize = (int)(26 * s);
        int closeBtnX = w - (int)(16 * s) - closeBtnSize;
        int closeBtnY = (int)((42 * s - closeBtnSize) / 2);
        if (lx >= closeBtnX && lx <= closeBtnX + closeBtnSize && ly >= closeBtnY && ly <= closeBtnY + closeBtnSize) {
            newHover = 99;
        }

        if (newHover != g_dlgState.hoverBtn) {
            g_dlgState.hoverBtn = newHover;
            RenderModernDialog(hWnd);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        POINT pt;
        GetCursorPos(&pt);
        int lx = pt.x - g_dlgState.dlgX;
        int ly = pt.y - g_dlgState.dlgY;

        float s = g_dpiScale;
        int w = g_dlgState.dlgW;
        int h = g_dlgState.dlgH;
        int btnH = (int)(38 * s);
        int btnY = h - (int)(18 * s) - btnH;
        int primaryW = g_dlgState.hasSecondary ? (int)(118 * s) : (int)(100 * s);
        int primaryX = w - (int)(18 * s) - primaryW;

        bool onBtn = (lx >= primaryX && lx <= primaryX + primaryW && ly >= btnY && ly <= btnY + btnH);
        if (!onBtn && g_dlgState.hasSecondary) {
            int secW = (int)(94 * s);
            int secX = primaryX - (int)(10 * s) - secW;
            if (lx >= secX && lx <= secX + secW && ly >= btnY && ly <= btnY + btnH) onBtn = true;
        }
        int closeBtnSize = (int)(26 * s);
        int closeBtnX = w - (int)(16 * s) - closeBtnSize;
        int closeBtnY = (int)((42 * s - closeBtnSize) / 2);
        if (lx >= closeBtnX && lx <= closeBtnX + closeBtnSize && ly >= closeBtnY && ly <= closeBtnY + closeBtnSize) onBtn = true;

        if (!onBtn) {
            g_dlgState.isDragging = true;
            SetCapture(hWnd);
            g_dlgState.dragStart.x = pt.x - g_dlgState.dlgX;
            g_dlgState.dragStart.y = pt.y - g_dlgState.dlgY;
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (g_dlgState.isDragging) {
            g_dlgState.isDragging = false;
            ReleaseCapture();
        }

        POINT pt;
        GetCursorPos(&pt);
        int lx = pt.x - g_dlgState.dlgX;
        int ly = pt.y - g_dlgState.dlgY;

        float s = g_dpiScale;
        int w = g_dlgState.dlgW;
        int h = g_dlgState.dlgH;
        int btnH = (int)(38 * s);
        int btnY = h - (int)(18 * s) - btnH;
        int primaryW = g_dlgState.hasSecondary ? (int)(118 * s) : (int)(100 * s);
        int primaryX = w - (int)(18 * s) - primaryW;

        if (lx >= primaryX && lx <= primaryX + primaryW && ly >= btnY && ly <= btnY + btnH) {
            g_dlgState.result = g_dlgState.hasSecondary ? IDYES : IDOK;
            g_dlgState.isOpen = false;
        } else if (g_dlgState.hasSecondary) {
            int secW = (int)(94 * s);
            int secX = primaryX - (int)(10 * s) - secW;
            if (lx >= secX && lx <= secX + secW && ly >= btnY && ly <= btnY + btnH) {
                g_dlgState.result = IDNO;
                g_dlgState.isOpen = false;
            }
        }
        int closeBtnSize = (int)(26 * s);
        int closeBtnX = w - (int)(16 * s) - closeBtnSize;
        int closeBtnY = (int)((42 * s - closeBtnSize) / 2);
        if (lx >= closeBtnX && lx <= closeBtnX + closeBtnSize && ly >= closeBtnY && ly <= closeBtnY + closeBtnSize) {
            g_dlgState.result = g_dlgState.hasSecondary ? IDCANCEL : IDOK;
            g_dlgState.isOpen = false;
        }
        break;
    }

    case WM_DESTROY:
        g_dlgState.isOpen = false;
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int ShowModernDialog(HWND hWndParent,
                     const wchar_t* title,
                     const wchar_t* message,
                     ModernDialogType type = MODERN_DLG_SUCCESS,
                     const wchar_t* primaryBtnText = L"OK",
                     const wchar_t* secondaryBtnText = NULL) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    static bool s_classRegistered = false;
    if (!s_classRegistered) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = ModernDialogWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = L"NepaliModernDialogClass";
        RegisterClassExW(&wc);
        s_classRegistered = true;
    }

    float s = g_dpiScale;
    g_dlgState.dlgW = (int)(440 * s);
    g_dlgState.dlgH = (int)((secondaryBtnText != NULL ? 235 : 215) * s);
    g_dlgState.dialogType = type;
    g_dlgState.hoverBtn = 0;
    g_dlgState.isDragging = false;
    g_dlgState.hasSecondary = (secondaryBtnText != NULL);
    wcscpy_s(g_dlgState.title, title ? title : L"Tithify");
    wcscpy_s(g_dlgState.message, message ? message : L"");
    wcscpy_s(g_dlgState.primaryBtn, primaryBtnText ? primaryBtnText : L"OK");
    if (secondaryBtnText) wcscpy_s(g_dlgState.secondaryBtn, secondaryBtnText);
    else g_dlgState.secondaryBtn[0] = 0;

    HMONITOR hMon = MonitorFromWindow(hWndParent ? hWndParent : (g_hWnd ? g_hWnd : GetDesktopWindow()), MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);

    g_dlgState.dlgX = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - g_dlgState.dlgW) / 2;
    g_dlgState.dlgY = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - g_dlgState.dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"NepaliModernDialogClass",
        L"Tithify Dialog",
        WS_POPUP,
        g_dlgState.dlgX, g_dlgState.dlgY, g_dlgState.dlgW, g_dlgState.dlgH,
        NULL, NULL, hInst, NULL
    );

    if (!hDlg) return IDCANCEL;

    g_dlgState.isOpen = true;
    g_dlgState.result = g_dlgState.hasSecondary ? IDCANCEL : IDOK;

    RenderModernDialog(hDlg);
    ShowWindow(hDlg, SW_SHOWNOACTIVATE);
    SetForegroundWindow(hDlg);
    SetFocus(hDlg);

    if (hWndParent && IsWindow(hWndParent)) {
        EnableWindow(hWndParent, FALSE);
    }

    MSG msg;
    while (g_dlgState.isOpen && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE) {
                g_dlgState.result = g_dlgState.hasSecondary ? IDCANCEL : IDOK;
                g_dlgState.isOpen = false;
                break;
            } else if (msg.wParam == VK_RETURN || msg.wParam == VK_SPACE) {
                g_dlgState.result = g_dlgState.hasSecondary ? IDYES : IDOK;
                g_dlgState.isOpen = false;
                break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hWndParent && IsWindow(hWndParent)) {
        EnableWindow(hWndParent, TRUE);
        SetForegroundWindow(hWndParent);
    }

    DestroyWindow(hDlg);
    return g_dlgState.result;
}

// ── Win32 Message Loop Engine ────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static UINT s_uTaskbarRestartMsg = 0;
    if (!s_uTaskbarRestartMsg) {
        s_uTaskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");
    }
    if (msg == s_uTaskbarRestartMsg && s_uTaskbarRestartMsg != 0) {
        AddTrayIcon(hWnd);
        UpdateWidgetMode();
        return 0;
    }

    switch (msg) {
    case WM_CREATE:
        AddTrayIcon(hWnd);
        SetTimer(hWnd, 1, 1000, NULL);   // Render timer
        SetTimer(hWnd, 2, 16, NULL);     // High-precision (60 FPS) animation & taskbar sync ticker
        break;

    case WM_TIMER:
        if (wParam == 1) {
            RenderWidget(hWnd);
            CheckPendingReminders();

            // Auto-recovery: periodically retry fetching holidays in background when offline
            if (g_holidayFetchState == FETCH_ERROR_OFFLINE && !g_isHolidayFetchInProgress) {
                DWORD now = GetTickCount();
                if (now - g_lastHolidayFetchAttemptTime >= 30000) {
                    int cy, cm, cd, cdow;
                    GetCurrentBSDate(cy, cm, cd, cdow);
                    EnsureHolidaysLoadedForCurrentYear(cy, true);
                }
            }
        } else if (wParam == 2) {
            // ── Click-outside-to-close for calendar ──
            if (g_isCalendarOpen && g_hCalWnd) {
                // Ignore clicks if reminder dashboard or alarm alert is open
                if (g_isReminderDashboardOpen || g_isAlarmAlertOpen) {
                    // Do not close calendar while user is interacting with reminder or alarm
                } else if (GetTickCount() - g_calAnimStartTime > 300) {
                    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                        (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ||
                        (GetAsyncKeyState(VK_MBUTTON) & 0x8000)) {
                        POINT cursor;
                        GetCursorPos(&cursor);
                        RECT rcWidget;
                        GetWindowRect(hWnd, &rcWidget);
                        // Close if click is outside both the calendar cards and the widget
                        if (!IsPointInCalendar(cursor) && !PtInRect(&rcWidget, cursor)) {
                            if (g_isDatePopupOpen) CloseCalendarDatePopup();
                            HideCalendar();
                        }
                    }
                }
            }

            // ── Fullscreen App Detection (throttled to 200ms) ──
            static DWORD s_lastFullscreenCheckTime = 0;
            static bool s_cachedFullscreen = false;
            DWORD nowTicks = GetTickCount();
            if (!g_setupMode && (nowTicks - s_lastFullscreenCheckTime >= 200)) {
                s_lastFullscreenCheckTime = nowTicks;
                s_cachedFullscreen = IsFullscreenAppRunning();
            }

            TaskbarSyncState syncState = GetTaskbarSyncState();
            bool fullscreen = s_cachedFullscreen;

            // Fullscreen hide rule: hide during fullscreen apps UNLESS auto-hide taskbar is revealed by hovering
            bool shouldHideFullscreen = fullscreen && (syncState.isAutoHide ? syncState.isCompletelyHidden : true);

            if (shouldHideFullscreen && !g_hiddenForFullscreen) {
                g_hiddenForFullscreen = true;
                if (g_isCalendarOpen) HideCalendarImmediate();
                if (g_isMenuOpen) HideCustomMenu();
                ShowWindow(hWnd, SW_HIDE);
            } else if (!shouldHideFullscreen && g_hiddenForFullscreen) {
                g_hiddenForFullscreen = false;
                if (!g_hiddenForTaskbar) {
                    ShowWindow(hWnd, SW_SHOWNOACTIVATE);
                    RenderWidget(hWnd);
                }
            }

            // ── Pixel-Accurate Taskbar Auto-Hide Animation Sync ──
            if (!g_setupMode && g_isOnTaskbar && syncState.isAutoHide) {
                if (syncState.isCompletelyHidden) {
                    if (!g_hiddenForTaskbar) {
                        g_hiddenForTaskbar = true;
                        if (g_isCalendarOpen) HideCalendarImmediate();
                        if (g_isMenuOpen) HideCustomMenu();
                        ShowWindow(hWnd, SW_HIDE);
                    }
                } else {
                    bool wasHidden = g_hiddenForTaskbar;
                    g_hiddenForTaskbar = false;

                    bool shiftChanged = (g_currentShiftX != syncState.shiftX || g_currentShiftY != syncState.shiftY);
                    g_currentShiftX = syncState.shiftX;
                    g_currentShiftY = syncState.shiftY;

                    if ((wasHidden || !IsWindowVisible(hWnd)) && !shouldHideFullscreen) {
                        ShowWindow(hWnd, SW_SHOWNOACTIVATE);
                        RenderWidget(hWnd);
                    } else if (shiftChanged && !shouldHideFullscreen && IsWindowVisible(hWnd)) {
                        SetWindowPos(hWnd, NULL, g_xPos + g_currentShiftX, g_yPos + g_currentShiftY, 0, 0,
                                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
                    }
                }
            } else {
                if (g_currentShiftX != 0 || g_currentShiftY != 0) {
                    g_currentShiftX = 0;
                    g_currentShiftY = 0;
                    if (!shouldHideFullscreen && IsWindowVisible(hWnd)) {
                        SetWindowPos(hWnd, NULL, g_xPos, g_yPos, 0, 0,
                                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
                    }
                }
                if (g_hiddenForTaskbar) {
                    g_hiddenForTaskbar = false;
                }
            }

            // ── Ensure window stays visible & un-minimized if not hidden for fullscreen or auto-hide taskbar ──
            if (!shouldHideFullscreen && !g_hiddenForTaskbar) {
                if (IsIconic(hWnd)) {
                    ShowWindow(hWnd, SW_RESTORE);
                }
                if (!IsWindowVisible(hWnd)) {
                    ShowWindow(hWnd, SW_SHOWNOACTIVATE);
                    RenderWidget(hWnd);
                }
            }

            // ── Z-Order Management & Taskbar Sync (50ms responsive check) ──
            static DWORD s_lastZCheckTime = 0;
            if (!shouldHideFullscreen && !g_hiddenForTaskbar && !g_isMenuOpen && !g_isCalendarOpen) {
                bool keepOnTop = (g_setupMode || g_isOnTaskbar || g_zIndexDesktop);
                if (keepOnTop) {
                    bool needZOrderUpdate = (nowTicks - s_lastZCheckTime >= 50);
                    if (!needZOrderUpdate && g_isOnTaskbar) {
                        HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
                        if (hTaskbar) {
                            HWND hPrev = GetWindow(hWnd, GW_HWNDPREV);
                            if (hPrev == hTaskbar) {
                                needZOrderUpdate = true;
                            }
                        }
                    }
                    if (needZOrderUpdate) {
                        s_lastZCheckTime = nowTicks;
                        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
                    }
                }
            }
        }
        break;

    // ── Block minimization completely (e.g. from Show Desktop / Win+D / 3-finger swipe) ──
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            return 0;  // Block SC_MINIMIZE!
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED && !g_hiddenForFullscreen && !g_hiddenForTaskbar) {
            ShowWindow(hWnd, SW_RESTORE);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);

    // ── Prevent Windows / Explorer / Show Desktop / Aero Peek from hiding us ──
    case WM_SHOWWINDOW:
        if (wParam == FALSE) {
            if (!g_hiddenForFullscreen && !g_hiddenForTaskbar) {
                PostMessage(hWnd, WM_USER + 99, 0, 0);
                return 0;  // Stay visible unless explicitly hidden for fullscreen or taskbar
            }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);

    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS* wp = (WINDOWPOS*)lParam;
        if (wp && !g_hiddenForFullscreen && !g_hiddenForTaskbar) {
            wp->flags &= ~SWP_HIDEWINDOW;
            // Prevent Explorer from moving window to iconic off-screen (-32000, -32000)
            if (wp->x <= -30000 || wp->y <= -30000) {
                wp->x = g_xPos;
                wp->y = g_yPos;
                wp->flags |= SWP_NOMOVE;
            }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    case WM_USER + 99:
        if (!g_hiddenForFullscreen && !g_hiddenForTaskbar) {
            if (IsIconic(hWnd)) ShowWindow(hWnd, SW_RESTORE);
            ShowWindow(hWnd, SW_SHOWNOACTIVATE);
            bool keepOnTop = (g_setupMode || g_isOnTaskbar || g_zIndexDesktop);
            if (keepOnTop) {
                if (g_isOnTaskbar) {
                    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", NULL);
                    if (hTaskbar && (HWND)GetWindowLongPtr(hWnd, GWLP_HWNDPARENT) != hTaskbar) {
                        SetWindowLongPtr(hWnd, GWLP_HWNDPARENT, (LONG_PTR)hTaskbar);
                    }
                } else {
                    if ((HWND)GetWindowLongPtr(hWnd, GWLP_HWNDPARENT) != NULL) {
                        SetWindowLongPtr(hWnd, GWLP_HWNDPARENT, 0);
                    }
                }
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            } else {
                HWND hDesktop = GetDesktopHostWindow();
                if (hDesktop && (HWND)GetWindowLongPtr(hWnd, GWLP_HWNDPARENT) != hDesktop) {
                    SetWindowLongPtr(hWnd, GWLP_HWNDPARENT, (LONG_PTR)hDesktop);
                }
                SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            }
            RenderWidget(hWnd);
        }
        break;

    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_LBUTTONDOWN:
        if (g_setupMode) {
            // Setup mode: capture mouse for dragging
            g_isDragging = true;
            g_hasDragged = false;
            SetCapture(hWnd);
            GetCursorPos(&g_dragStart);
            g_dragStart.x -= g_xPos;
            g_dragStart.y -= g_yPos;
        } else {
            // Locked mode: open/close calendar on press
            // Don't SetCapture — WS_EX_NOACTIVATE window can drop WM_LBUTTONUP
            if (GetTickCount() - g_lastCalCloseTime > 250) {
                ToggleCalendar(hWnd);
            }
        }
        break;

    case WM_MOUSEMOVE:
        if (g_isDragging && g_setupMode) {
            POINT pt;
            GetCursorPos(&pt);
            int newX = pt.x - g_dragStart.x;
            int newY = pt.y - g_dragStart.y;
            if (abs(newX - g_xPos) > 2 || abs(newY - g_yPos) > 2) {
                g_hasDragged = true;
            }
            g_xPos = newX;
            g_yPos = newY;
            RenderWidget(hWnd);
        }
        break;

    case WM_LBUTTONUP:
        if (g_isDragging) {
            g_isDragging = false;
            ReleaseCapture();
            UpdateWidgetMode();
            SaveConfig();
        }
        break;

    case WM_RBUTTONUP: {
        POINT pt;
        GetCursorPos(&pt);
        ShowContextMenu(hWnd, pt);
        break;
    }

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hWnd, pt);
        }
        break;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    // ── Update notification ──────────────────────────────────────────────────
    case WM_UPDATE_AVAILABLE: {
        wchar_t msg[320];
        swprintf(msg, 320,
            L"Version %ls is now available! Click Update Now to download and install. The widget will restart automatically.",
            g_latestVersion);
        int choice = ShowModernDialog(hWnd,
                                      L"Update Available",
                                      msg,
                                      MODERN_DLG_UPDATE,
                                      L"Update Now",
                                      L"Later");
        if (choice == IDYES || choice == IDOK) {
            // Open a visible PowerShell console that handles the full update
            LaunchUpdaterConsole(g_latestVersion);
            // Close the widget so the installer can overwrite Tithify.exe
            PostMessage(g_hWnd, WM_CLOSE, 0, 0);
        }
        break;
    }

    case WM_UPDATE_NOT_FOUND:
        ShowModernDialog(hWnd,
                         L"You're Up to Date",
                         L"You are already running the latest version of Tithify.",
                         MODERN_DLG_SUCCESS,
                         L"OK");
        break;

    case WM_UPDATE_ERROR:
        ShowModernDialog(hWnd,
                         L"Update Check Failed",
                         L"Unable to check for updates. Please verify your internet connection and try again.",
                         MODERN_DLG_ERROR,
                         L"OK");
        break;


    case WM_HOLIDAYS_LOADED:
        RenderWidget(hWnd);
        if (g_isCalendarOpen && g_hCalWnd) {
            RenderCalendar(g_hCalWnd);
        }
        break;

    case WM_HOLIDAYS_FAILED:
        if (g_isCalendarOpen && g_hCalWnd) {
            RenderCalendar(g_hCalWnd);
        }
        break;

    // ── Theme change detection ─────────────────────────────────────────────
    case WM_SETTINGCHANGE:
        if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
            g_isLightTheme = DetectWindowsTheme();
            RenderWidget(hWnd);
            if (g_isCalendarOpen && g_hCalWnd) {
                RenderCalendar(g_hCalWnd);
            }
        }
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ── Application Entrypoint ───────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"Tithify_Mutex_Unique_App_ID");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"The application is already running.", L"Tithify", MB_OK | MB_ICONINFORMATION);
        CloseHandle(hMutex);
        return 0;
    }

    // ── DPI Awareness: Render at native resolution, no bitmap scaling ────
    typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(HANDLE);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto fn = (SetProcessDpiAwarenessContextFunc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (fn) {
            fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            SetProcessDPIAware();  // Fallback for older Windows 10
        }
    }

    // Compute DPI scale
    HDC hdc = GetDC(NULL);
    g_dpiScale = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(NULL, hdc);

    LoadConfig();
    LoadReminders();

    // Detect initial Windows theme
    g_isLightTheme = DetectWindowsTheme();

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    // Register Widget Class
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"NepaliTaskbarWidgetClass";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    RegisterClassEx(&wc);

    // Register Calendar Popup Class
    WNDCLASSEX wcCal = { 0 };
    wcCal.cbSize = sizeof(WNDCLASSEX);
    wcCal.lpfnWndProc = CalendarWndProc;
    wcCal.hInstance = hInstance;
    wcCal.lpszClassName = L"NepaliCalendarPopupClass";
    wcCal.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassEx(&wcCal);

    // Register Custom Menu Popup Class
    WNDCLASSEX wcMenu = { 0 };
    wcMenu.cbSize = sizeof(WNDCLASSEX);
    wcMenu.lpfnWndProc = CustomMenuWndProc;
    wcMenu.hInstance = hInstance;
    wcMenu.lpszClassName = L"NepaliMenuPopupClass";
    wcMenu.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassEx(&wcMenu);

    HWND hTaskbar = FindWindow(L"Shell_TrayWnd", NULL);
    
    // Bounds check to ensure the widget isn't lost off-screen due to old configs
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    if (g_yPos < 0 || g_yPos >= screenHeight) {
        if (hTaskbar) {
            RECT taskbarRect;
            GetWindowRect(hTaskbar, &taskbarRect);
            g_yPos = taskbarRect.top + (taskbarRect.bottom - taskbarRect.top) / 2 - 24;
            if (g_yPos < 0 || g_yPos > screenHeight) g_yPos = screenHeight - 60;
        } else {
            g_yPos = screenHeight - 60;
        }
    }

    // DPI-scaled window size
    int winW = (int)((g_showDay ? 190 : 155) * g_dpiScale);
    int winH = (int)(48 * g_dpiScale);

    // Create main widget window
    g_hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"NepaliTaskbarWidgetClass",
        L"Nepali Date Taskbar Widget",
        WS_POPUP,
        g_xPos, g_yPos, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    // Create calendar popup window
    g_hCalWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"NepaliCalendarPopupClass",
        L"Nepali Calendar",
        WS_POPUP,
        0, 0, (int)(420 * g_dpiScale), (int)(468 * g_dpiScale),
        NULL, NULL, hInstance, NULL
    );

    // Create custom menu popup window
    g_hMenuWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"NepaliMenuPopupClass",
        L"Nepali Menu",
        WS_POPUP,
        0, 0, (int)(385 * g_dpiScale), (int)(260 * g_dpiScale),
        NULL, NULL, hInstance, NULL
    );

    // ── Apply Taskbar vs Desktop mode z-order & parenting ──
    UpdateWidgetMode();

    // ── Real-time shell event hook for instant foreground / desktop changes ──
    g_hEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        NULL, WinEventProc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    ShowWindow(g_hWnd, SW_SHOWNOACTIVATE);
    RenderWidget(g_hWnd);

    // ── Launch background update check (non-blocking) ────────────────────
    HANDLE hThread = CreateThread(NULL, 0, CheckForUpdateThread, (LPVOID)FALSE, 0, NULL);
    if (hThread) CloseHandle(hThread);  // Fire-and-forget

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_hEventHook) {
        UnhookWinEvent(g_hEventHook);
        g_hEventHook = NULL;
    }

    if (g_hCalWnd) {
        DestroyWindow(g_hCalWnd);
        g_hCalWnd = NULL;
    }
    if (g_hMenuWnd) {
        DestroyWindow(g_hMenuWnd);
        g_hMenuWnd = NULL;
    }
    if (g_hReminderWnd) {
        DestroyWindow(g_hReminderWnd);
        g_hReminderWnd = NULL;
    }
    if (g_hAlarmAlertWnd) {
        DestroyWindow(g_hAlarmAlertWnd);
        g_hAlarmAlertWnd = NULL;
    }
    if (g_hDatePopupWnd) {
        DestroyWindow(g_hDatePopupWnd);
        g_hDatePopupWnd = NULL;
    }
    StopAlarmSound();

    GdiplusShutdown(g_gdiplusToken);

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    
    return 0;
}
