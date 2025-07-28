#include <windows.h>
#include <shellapi.h>

// 资源ID定义
#define IDI_MAIN_ICON 101

#define HOTKEY_ID 1
#define HOTKEY_COMBINATION (MOD_CONTROL | MOD_ALT)
#define HOTKEY_VKEY 'D'

// 托盘相关定义
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1001
#define ID_EXIT 1002
#define ID_TOGGLE 1003
#define ID_ABOUT 1004

// 函数声明
HWND FindDesktopListView();
void ToggleDesktopIcons();
BOOL CALLBACK EnumDesktopProc(HWND hwnd, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);
void ShowContextMenu(HWND hwnd);
void ShowTrayMessage(HWND hwnd, const char* title, const char* message);

// 全局变量
HWND g_hDesktopListView = NULL;
HWND g_hMainWindow = NULL;
NOTIFYICONDATA g_nid = {0};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 隐藏控制台窗口
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        ShowWindow(consoleWindow, SW_HIDE);
    }
    
    // 注册窗口类
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DesktopToggle";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "无法注册窗口类", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // 创建隐藏窗口
    g_hMainWindow = CreateWindow(
        "DesktopToggle", "桌面图标切换器", 0,
        0, 0, 0, 0, NULL, NULL, hInstance, NULL
    );
    
    if (!g_hMainWindow) {
        MessageBox(NULL, "无法创建窗口", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // 注册热键
    if (!RegisterHotKey(g_hMainWindow, HOTKEY_ID, HOTKEY_COMBINATION, HOTKEY_VKEY)) {
        MessageBox(NULL, "无法注册热键 Ctrl+Alt+D\n可能是权限不足或热键已被占用", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // 创建系统托盘图标
    CreateTrayIcon(g_hMainWindow);
    ShowTrayMessage(g_hMainWindow, "桌面图标切换器", "程序已启动\n快捷键: Ctrl+Alt+D");
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理资源
    UnregisterHotKey(g_hMainWindow, HOTKEY_ID);
    RemoveTrayIcon(g_hMainWindow);
    
    return (int)msg.wParam;
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_HOTKEY:
            if (wParam == HOTKEY_ID) {
                HWND listView = FindDesktopListView();
                BOOL wasVisible = listView ? IsWindowVisible(listView) : TRUE;
                
                ToggleDesktopIcons();
                
                // 显示切换结果通知
                const char* message = wasVisible ? "桌面图标已隐藏" : "桌面图标已显示";
                ShowTrayMessage(hwnd, "桌面图标切换器", message);
            }
            break;
            
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
                ShowContextMenu(hwnd);
            }
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TOGGLE:
                    // 手动切换
                    SendMessage(hwnd, WM_HOTKEY, HOTKEY_ID, 0);
                    break;
                case ID_ABOUT:
                    MessageBox(hwnd, 
                        "桌面图标切换器 v1.3\n\n"
                        "快捷键: Ctrl+Alt+D\n"
                        "右键托盘图标可手动切换或退出程序\n\n"
                        "作者: Lightning", 
                        "关于", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

//创建系统托盘图标
void CreateTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = ID_TRAYICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    
    //使用自定义图标
    HINSTANCE hInstance = GetModuleHandle(NULL);
    g_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    strcpy(g_nid.szTip, "桌面图标切换器 - Ctrl+Alt+D");
    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

void RemoveTrayIcon(HWND hwnd) {
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TOGGLE, "切换桌面图标");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_ABOUT, "关于");
    AppendMenu(hMenu, MF_STRING, ID_EXIT, "退出");
    
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
    PostMessage(hwnd, WM_NULL, 0, 0);
}

void ShowTrayMessage(HWND hwnd, const char* title, const char* message) {
    g_nid.uFlags = NIF_INFO;
    g_nid.dwInfoFlags = NIIF_INFO;
    strcpy(g_nid.szInfoTitle, title);
    strcpy(g_nid.szInfo, message);
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

// 枚举桌面窗口的回调函数
BOOL CALLBACK EnumDesktopProc(HWND hwnd, LPARAM lParam) {
    char className[256];
    GetClassName(hwnd, className, sizeof(className));
    
    if (strcmp(className, "SysListView32") == 0) {
        HWND parent = GetParent(hwnd);
        if (parent) {
            char parentClass[256];
            GetClassName(parent, parentClass, sizeof(parentClass));
            if (strcmp(parentClass, "SHELLDLL_DefView") == 0) {
                g_hDesktopListView = hwnd;
                return FALSE; // 找到了，停止枚举
            }
        }
    }
    return TRUE;
}

// 查找桌面ListView控件
HWND FindDesktopListView() {
    g_hDesktopListView = NULL;
    
    // 首先在Progman中查找
    HWND progman = FindWindow("Progman", NULL);
    if (progman) {
        EnumChildWindows(progman, EnumDesktopProc, 0);
    }
    
    // 如果在Progman中没找到，在WorkerW中查找
    if (!g_hDesktopListView) {
        HWND workerw = NULL;
        do {
            workerw = FindWindowEx(NULL, workerw, "WorkerW", NULL);
            if (workerw) {
                EnumChildWindows(workerw, EnumDesktopProc, 0);
                if (g_hDesktopListView) break;
            }
        } while (workerw);
    }
    
    return g_hDesktopListView;
}

// 切换桌面图标显示状态
void ToggleDesktopIcons() {
    // 查找ListView控件
    HWND listView = FindDesktopListView();
    
    if (listView) {
        // 直接切换ListView的显示状态
        BOOL isVisible = IsWindowVisible(listView);
        ShowWindow(listView, isVisible ? SW_HIDE : SW_SHOW);
    } else {
        // 如果找不到ListView，使用原来的方法作为备选
        HWND hProgman = FindWindow("Progman", NULL);
        if (hProgman) {
            PostMessage(hProgman, WM_COMMAND, 0x7402, 0);
        }
        
        Sleep(200);
        
        // 刷新桌面
        POINT oldPos;
        GetCursorPos(&oldPos);
        
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        SetCursorPos(screenWidth / 2, screenHeight / 2);
        
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        Sleep(100);
        
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        keybd_event('V', 0, 0, 0);
        keybd_event('V', 0, KEYEVENTF_KEYUP, 0);
        Sleep(100);
        keybd_event('D', 0, 0, 0);
        keybd_event('D', 0, KEYEVENTF_KEYUP, 0);
        
        Sleep(200);
        keybd_event(VK_ESCAPE, 0, 0, 0);
        keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
        
        SetCursorPos(oldPos.x, oldPos.y);
    }
}