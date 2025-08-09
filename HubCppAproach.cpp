/*
COMPILATION DONE VIA: 

g++ HubCppAproach.cpp -municode -std=c++17 -mwindows -o HubCppAproach.exe
*/


#include <windows.h>
#include <shellapi.h>
#include <map>
#include <string>
#include <algorithm>
#include <fstream>
#include <regex>

// Define ARRAYSIZE macro if not available
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

// Function declarations
std::map<std::string,std::string> readScripts(const std::string& filename);
static std::wstring utf8_to_w(const std::string& s);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void ShowContextMenu(HWND hWnd, POINT pt);

// reserve a block of IDs for scripts
#define ID_TRAY_APP_ICON       5000
#define ID_MENU_CHECK_UPDATES  6000
#define ID_MENU_QUIT_APP       6001
#define WM_TRAYICON            (WM_USER + 1)
#define ID_MENU_SCRIPT_BASE    7000

// Win32 "lParam" values for mouse events on a tray icon:
// Note: These are already defined in winuser.h, commenting out to avoid redefinition warnings
// #define WM_LBUTTONUP   0x0202
// #define WM_RBUTTONUP   0x0205

// Global Variables
HINSTANCE      g_hInst       = nullptr;
NOTIFYICONDATA g_nid         = {};    // Structure that describes our tray icon
HWND           g_hWnd        = nullptr; // Hidden window that receives tray‐icon messages

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    //hiding terminal window
    HWND hConsole = GetConsoleWindow();
    if (hConsole) {
        // Hide the console window
        ShowWindow(hConsole, SW_HIDE);
    }

    // 1) Register a "hidden" window class (so we have an HWND to receive messages).
    WNDCLASSEX wcex = {};
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = 0;
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = g_hInst;
    wcex.lpszClassName = TEXT("ScriptsHubHiddenWndClass");

    if (!RegisterClassEx(&wcex)) {
        MessageBox(nullptr, TEXT("Failed to register window class"), TEXT("Error"), MB_ICONERROR);
        return 1;
    }

    // 2) Create the hidden window (0×0 size, no visible bits).
    g_hWnd = CreateWindow(
        wcex.lpszClassName,
        TEXT("ScriptsHub"),  // Window name (not shown)
        WS_OVERLAPPEDWINDOW,
        0, 0, 0, 0,                    // x, y, width, height = 0 makes it invisible
        nullptr,
        nullptr,
        g_hInst,
        nullptr);

    if (!g_hWnd) {
        MessageBox(nullptr, TEXT("Failed to create hidden window"), TEXT("Error"), MB_ICONERROR);
        return 1;
    }

    // 3) Fill out NOTIFYICONDATA so we can add a tray icon
    ZeroMemory(&g_nid, sizeof(NOTIFYICONDATA));
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = g_hWnd;
    g_nid.uID              = ID_TRAY_APP_ICON;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    // Load custom icon from file (silently fallback to default if not found)
    HICON hCustomIcon = (HICON)LoadImage(
        nullptr,
        TEXT("appIcon.ico"),
        IMAGE_ICON,
        0, 0,  // Let Windows choose appropriate size
        LR_LOADFROMFILE | LR_DEFAULTSIZE
    );

    // Use custom icon if loaded, otherwise use default application icon
    g_nid.hIcon = hCustomIcon ? hCustomIcon : LoadIcon(nullptr, IDI_APPLICATION);
    lstrcpy(g_nid.szTip, TEXT("ScriptsHub")); // Use Windows API function for string copy

    // 4) Add the icon to the system tray
    if (!Shell_NotifyIcon(NIM_ADD, &g_nid)) {
        MessageBox(nullptr, TEXT("Failed to create app"), TEXT("Error"), MB_ICONERROR);
        return 1;
    }

    // 5) Enter the message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 6) Before exiting, cleanup
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    if (hCustomIcon) DestroyIcon(hCustomIcon);  // Clean up custom icon
    return (int)msg.wParam;
}

// Function implementations below main

// readScripts: parse AppSettings.conf → map<name,path>
std::map<std::string,std::string> readScripts(const std::string& filename) {
    std::map<std::string,std::string> scripts;
    std::ifstream file(filename);
    if (!file.is_open()) return scripts;
    
    std::string line;
    std::regex scriptRegex("\"([^\"]+)\":\\s*\"([^\"]+)\"");
    std::smatch match;
    
    while (std::getline(file, line)) {
        if (std::regex_search(line, match, scriptRegex)) {
            std::string name = match[1].str();
            std::string path = match[2].str();
            scripts[name] = path;
        }
    }
    
    file.close();
    return scripts;
}

// WndProc: Handle Windows messages (including our custom WM_TRAYICON)
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
    switch (message){
        case WM_TRAYICON:{
            // Check that this notification is for our tray ID
            if ((UINT)wParam == ID_TRAY_APP_ICON) {
                if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP){
                    // Get the cursor position in screen coordinates
                    POINT pt;
                    GetCursorPos(&pt);

                    // Show our context menu at that point
                    ShowContextMenu(hWnd, pt);
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            // LOWORD(wParam) is the ID of the menu item clicked
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_MENU_CHECK_UPDATES:
                {
                    // Open current folder in Explorer AND reload scripts
                    ShellExecuteA(nullptr, "open", ".", nullptr, nullptr, SW_SHOWNORMAL);
                    // Also reload scripts silently for next menu display
                    auto scripts = readScripts("AppSettings.conf");
                    break;
                }
                case ID_MENU_QUIT_APP:
                    DestroyWindow(hWnd);  // This will trigger WM_DESTROY → exit loop
                    break;

                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ShowContextMenu: Builds and displays a Win32 popup menu at (pt.x, pt.y)
void ShowContextMenu(HWND hWnd, POINT pt)
{
    // 0) read your scripts
    auto scripts = readScripts("AppSettings.conf");

    // 1) create the popup
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    // 2) append one menu item per script
    int idx = 0;
    for (auto& kv : scripts) {
        UINT id = ID_MENU_SCRIPT_BASE + idx++;
        std::wstring wname = utf8_to_w(kv.first);
        AppendMenuW(hMenu, MF_STRING, id, wname.c_str());
    }

    // 3) separator + "Open Location"
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_MENU_CHECK_UPDATES, L"Open Location");

    // 4) separator + "Close"
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_MENU_QUIT_APP, L"Close");

    // 5) show it
    SetForegroundWindow(hWnd);
    const UINT uFlags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
    int cmd = TrackPopupMenu(hMenu, uFlags, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);

    // 6) dispatch
    if (cmd >= ID_MENU_SCRIPT_BASE && cmd < ID_MENU_SCRIPT_BASE + (int)scripts.size()) {
        int scriptIndex = cmd - ID_MENU_SCRIPT_BASE;
        auto it = std::next(scripts.begin(), scriptIndex);
        // launch the script path
        ShellExecuteA(nullptr, "open", it->second.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    else {
        switch (cmd) {
        case ID_MENU_CHECK_UPDATES:
            PostMessage(hWnd, WM_COMMAND, ID_MENU_CHECK_UPDATES, 0);
            break;
        case ID_MENU_QUIT_APP:
            PostMessage(hWnd, WM_COMMAND, ID_MENU_QUIT_APP, 0);
            break;
        default:
            break;
        }
    }
}

// utf8_to_w: helper function, UTF-8 std::string → UTF-16 std::wstring
static std::wstring utf8_to_w(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (n <= 0) return {};
    std::wstring w; w.resize(n);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}

