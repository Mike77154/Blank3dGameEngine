#ifndef B3D_TEST_WINDOWS_H
#define B3D_TEST_WINDOWS_H

#include <stddef.h>

typedef void *HANDLE;
typedef HANDLE HINSTANCE;
typedef HANDLE HWND;
typedef HANDLE HDC;
typedef HANDLE HGLRC;
typedef HANDLE HCURSOR;
typedef HANDLE HMENU;
typedef HANDLE HICON;
typedef HANDLE HBRUSH;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned long DWORD_PTR;
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef long LRESULT;
typedef long LONG;
typedef void (*PROC)(void);
typedef int BOOL;
typedef char CHAR;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef void *LPVOID;

#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

#define TRUE 1
#define FALSE 0
#define NULL_HANDLE ((HANDLE)0)

#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xffffUL))
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffffUL))
#define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))

#define WA_INACTIVE 0
#define WM_ACTIVATE 0x0006U
#define WM_SETFOCUS 0x0007U
#define WM_KILLFOCUS 0x0008U
#define WM_SIZE 0x0005U
#define WM_CLOSE 0x0010U
#define WM_DESTROY 0x0002U
#define WM_KEYDOWN 0x0100U
#define WM_KEYUP 0x0101U
#define WM_LBUTTONDOWN 0x0201U
#define WM_LBUTTONUP 0x0202U
#define WM_RBUTTONDOWN 0x0204U
#define WM_RBUTTONUP 0x0205U
#define WM_MOUSEWHEEL 0x020AU

#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SELECT 0x29
#define VK_EXECUTE 0x2B
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_APPS 0x5D
#define VK_POWER 0x5E
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F5 0x74
#define VK_F24 0x87
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_VOLUME_MUTE 0xAD
#define VK_VOLUME_DOWN 0xAE
#define VK_VOLUME_UP 0xAF
#define VK_OEM_1 0xBA
#define VK_OEM_PLUS 0xBB
#define VK_OEM_COMMA 0xBC
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2 0xBF
#define VK_OEM_3 0xC0
#define VK_OEM_4 0xDB
#define VK_OEM_5 0xDC
#define VK_OEM_6 0xDD
#define VK_OEM_7 0xDE
#define VK_OEM_8 0xDF
#define VK_OEM_102 0xE2
#define VK_OEM_NEC_EQUAL 0x92
#define VK_PROCESSKEY 0xE5
#define VK_CANCEL 0x03
#define VK_CRSEL 0xF7
#define VK_EXSEL 0xF8
#define VK_CONTROL 0x11

#define CS_OWNDC 0x0020U
#define WS_OVERLAPPEDWINDOW 0x00CF0000UL
#define WS_VISIBLE 0x10000000UL
#define WS_POPUP 0x80000000UL
#define WS_THICKFRAME 0x00040000UL
#define WS_MAXIMIZEBOX 0x00010000UL
#define GWL_STYLE (-16)
#define HWND_TOPMOST ((HWND)(long)-1L)
#define HWND_NOTOPMOST ((HWND)(long)-2L)
#define HWND_TOP ((HWND)0)
#define SWP_NOSIZE 0x0001U
#define SWP_NOMOVE 0x0002U
#define SWP_NOZORDER 0x0004U
#define SWP_FRAMECHANGED 0x0020U
#define SWP_SHOWWINDOW 0x0040U
#define SW_SHOW 5
#define SW_HIDE 0
#define SW_MINIMIZE 6
#define SW_MAXIMIZE 3
#define SW_RESTORE 9
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define CW_USEDEFAULT ((int)0x80000000UL)
#define PM_REMOVE 0x0001U
#define IDC_ARROW ((LPCSTR)32512)

#define PFD_DRAW_TO_WINDOW 0x00000004UL
#define PFD_SUPPORT_OPENGL 0x00000020UL
#define PFD_DOUBLEBUFFER 0x00000001UL
#define PFD_TYPE_RGBA 0
#define PFD_MAIN_PLANE 0

#define GetFileExInfoStandard 0
#define INVALID_FILE_ATTRIBUTES ((DWORD)0xFFFFFFFFUL)
#define INVALID_HANDLE_VALUE ((HANDLE)(long)-1L)
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010UL

typedef struct tagPOINT { long x; long y; } POINT;
typedef struct tagRECT { long left; long top; long right; long bottom; } RECT;
typedef struct _FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME;
typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA;

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[260];
    CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef struct tagWNDCLASSA {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASSA;

typedef struct tagPIXELFORMATDESCRIPTOR {
    WORD nSize;
    WORD nVersion;
    DWORD dwFlags;
    BYTE iPixelType;
    BYTE cColorBits;
    BYTE cRedBits;
    BYTE cRedShift;
    BYTE cGreenBits;
    BYTE cGreenShift;
    BYTE cBlueBits;
    BYTE cBlueShift;
    BYTE cAlphaBits;
    BYTE cAlphaShift;
    BYTE cAccumBits;
    BYTE cAccumRedBits;
    BYTE cAccumGreenBits;
    BYTE cAccumBlueBits;
    BYTE cAccumAlphaBits;
    BYTE cDepthBits;
    BYTE cStencilBits;
    BYTE cAuxBuffers;
    BYTE iLayerType;
    BYTE bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG;

short GetAsyncKeyState(int);
DWORD GetFileAttributesA(LPCSTR);
BOOL GetFileAttributesExA(LPCSTR, int, LPVOID);
HANDLE FindFirstFileA(LPCSTR, WIN32_FIND_DATAA *);
BOOL FindNextFileA(HANDLE, WIN32_FIND_DATAA *);
BOOL FindClose(HANDLE);
long CompareFileTime(const FILETIME *, const FILETIME *);
HCURSOR LoadCursor(HINSTANCE, LPCSTR);
BOOL RegisterClassA(const WNDCLASSA *);
BOOL UnregisterClassA(LPCSTR, HINSTANCE);
BOOL AdjustWindowRect(RECT *, DWORD, BOOL);
HWND CreateWindowA(LPCSTR, LPCSTR, DWORD, int, int, int, int,
                   HWND, HMENU, HINSTANCE, LPVOID);
HDC GetDC(HWND);
int ReleaseDC(HWND, HDC);
BOOL DestroyWindow(HWND);
int ChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR *);
BOOL SetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR *);
HGLRC wglCreateContext(HDC);
BOOL wglMakeCurrent(HDC, HGLRC);
BOOL wglDeleteContext(HGLRC);
BOOL SwapBuffers(HDC);
BOOL PeekMessage(MSG *, HWND, UINT, UINT, UINT);
BOOL TranslateMessage(const MSG *);
LRESULT DispatchMessage(const MSG *);
void PostQuitMessage(int);
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM);
DWORD GetTickCount(void);
void Sleep(DWORD);
BOOL GetCursorPos(POINT *);
BOOL SetCursorPos(int, int);
BOOL ClientToScreen(HWND, POINT *);
BOOL ScreenToClient(HWND, POINT *);
BOOL GetClientRect(HWND, RECT *);
BOOL ClipCursor(const RECT *);
int ShowCursor(BOOL);
HWND SetCapture(HWND);
BOOL ReleaseCapture(void);
BOOL SetWindowTextA(HWND, LPCSTR);
int GetSystemMetrics(int);
LONG SetWindowLongA(HWND, int, LONG);
BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT);
BOOL ShowWindow(HWND, int);
PROC wglGetProcAddress(LPCSTR);

#endif
