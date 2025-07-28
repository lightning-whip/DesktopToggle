#include <windows.h>
#include <shellapi.h>

// ��ԴID����
#define IDI_MAIN_ICON 101

#define HOTKEY_ID 1
#define HOTKEY_COMBINATION (MOD_CONTROL | MOD_ALT)
#define HOTKEY_VKEY 'D'

// ������ض���
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1001
#define ID_EXIT 1002
#define ID_TOGGLE 1003
#define ID_ABOUT 1004

// ��������
HWND FindDesktopListView();
void ToggleDesktopIcons();
void ToggleTaskbar();
void ToggleDesktopAndTaskbar();
BOOL CALLBACK EnumDesktopProc(HWND hwnd, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);
void ShowContextMenu(HWND hwnd);
void ShowTrayMessage(HWND hwnd, const char* title, const char* message);

// ȫ�ֱ���
HWND g_hDesktopListView = NULL;
HWND g_hMainWindow = NULL;
HWND g_hTaskbar = NULL;
NOTIFYICONDATA g_nid = {0};
BOOL g_bTaskbarVisible = TRUE;
BOOL g_bDesktopVisible = TRUE;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ���ؿ���̨����
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        ShowWindow(consoleWindow, SW_HIDE);
    }
    
    // ��ȡ���������
    g_hTaskbar = FindWindow("Shell_TrayWnd", NULL);
    
    // ע�ᴰ����
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DesktopToggle";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "�޷�ע�ᴰ����", "����", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // �������ش���
    g_hMainWindow = CreateWindow(
        "DesktopToggle", "����ͼ���л���", 0,
        0, 0, 0, 0, NULL, NULL, hInstance, NULL
    );
    
    if (!g_hMainWindow) {
        MessageBox(NULL, "�޷���������", "����", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // ע���ȼ�
    if (!RegisterHotKey(g_hMainWindow, HOTKEY_ID, HOTKEY_COMBINATION, HOTKEY_VKEY)) {
        MessageBox(NULL, "�޷�ע���ȼ� Ctrl+Alt+D\n������Ȩ�޲�����ȼ��ѱ�ռ��", "����", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // ����ϵͳ����ͼ��
    CreateTrayIcon(g_hMainWindow);
    ShowTrayMessage(g_hMainWindow, "����ͼ���л���", "����������\n��ݼ�: Ctrl+Alt+D");
    
    // ��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // ������Դ
    UnregisterHotKey(g_hMainWindow, HOTKEY_ID);
    RemoveTrayIcon(g_hMainWindow);
    
    // �˳�ǰȷ���������ɼ�
    if (!g_bTaskbarVisible && g_hTaskbar) {
        ShowWindow(g_hTaskbar, SW_SHOW);
    }
    
    return (int)msg.wParam;
}

// ���ڹ��̺���
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_HOTKEY:
            if (wParam == HOTKEY_ID) {
                ToggleDesktopAndTaskbar();
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
                    // �ֶ��л�
                    ToggleDesktopAndTaskbar();
                    break;
                case ID_ABOUT:
                    MessageBox(hwnd, 
                        "����ͼ���л��� v1.4\n\n"
                        "��ݼ�: Ctrl+Alt+D\n"
                        "ͬʱ�л�����ͼ�����������ʾ/����\n"
                        "�Ҽ�����ͼ����ֶ��л����˳�����\n\n"
                        "����: Lightning", 
                        "����", MB_OK | MB_ICONINFORMATION);
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

//����ϵͳ����ͼ��
void CreateTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = ID_TRAYICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    
    //ʹ���Զ���ͼ��
    HINSTANCE hInstance = GetModuleHandle(NULL);
    g_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    strcpy(g_nid.szTip, "����ͼ���л��� - Ctrl+Alt+D");
    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

void RemoveTrayIcon(HWND hwnd) {
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TOGGLE, "�л�����ͼ���������");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_ABOUT, "����");
    AppendMenu(hMenu, MF_STRING, ID_EXIT, "�˳�");
    
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

// ö�����洰�ڵĻص�����
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
                return FALSE; // �ҵ��ˣ�ֹͣö��
            }
        }
    }
    return TRUE;
}

// ��������ListView�ؼ�
HWND FindDesktopListView() {
    g_hDesktopListView = NULL;
    
    // ������Progman�в���
    HWND progman = FindWindow("Progman", NULL);
    if (progman) {
        EnumChildWindows(progman, EnumDesktopProc, 0);
    }
    
    // �����Progman��û�ҵ�����WorkerW�в���
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

// �л���������ʾ״̬
void ToggleTaskbar() {
    if (!g_hTaskbar) {
        g_hTaskbar = FindWindow("Shell_TrayWnd", NULL);
    }
    
    if (g_hTaskbar) {
        if (g_bTaskbarVisible) {
            // ����������
            ShowWindow(g_hTaskbar, SW_HIDE);
            g_bTaskbarVisible = FALSE;
        } else {
            // ��ʾ������
            ShowWindow(g_hTaskbar, SW_SHOW);
            g_bTaskbarVisible = TRUE;
        }
        
        // ͬʱ����ʼ��ť��Windows 7�����ϣ�
        HWND startButton = FindWindowEx(g_hTaskbar, NULL, "Start", NULL);
        if (startButton) {
            ShowWindow(startButton, g_bTaskbarVisible ? SW_SHOW : SW_HIDE);
        }
        
        // ˢ�¹�������
        SystemParametersInfo(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE);
    }
}

// �л�����ͼ����ʾ״̬
void ToggleDesktopIcons() {
    // ����ListView�ؼ�
    HWND listView = FindDesktopListView();
    
    if (listView) {
        // ֱ���л�ListView����ʾ״̬
        BOOL isVisible = IsWindowVisible(listView);
        ShowWindow(listView, isVisible ? SW_HIDE : SW_SHOW);
        g_bDesktopVisible = !isVisible;
    } else {
        // ����Ҳ���ListView��ʹ��ԭ���ķ�����Ϊ��ѡ
        HWND hProgman = FindWindow("Progman", NULL);
        if (hProgman) {
            PostMessage(hProgman, WM_COMMAND, 0x7402, 0);
        }
        
        Sleep(200);
        
        // ˢ������
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
        g_bDesktopVisible = !g_bDesktopVisible;
    }
}

// ͬʱ�л�����ͼ���������
void ToggleDesktopAndTaskbar() {
    // ��ȡ��ǰ״̬
    HWND listView = FindDesktopListView();
    BOOL wasDesktopVisible = listView ? IsWindowVisible(listView) : g_bDesktopVisible;
    
    // ͬʱ�л�����ͼ���������
    ToggleDesktopIcons();
    ToggleTaskbar();
    
    // ��ʾ�л����֪ͨ
    const char* message;
    if (wasDesktopVisible) {
        message = "����ͼ���������������";
    } else {
        message = "����ͼ�������������ʾ";
    }
    ShowTrayMessage(g_hMainWindow, "�����л���", message);
}