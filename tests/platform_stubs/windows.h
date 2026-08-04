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

#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_CONTROL 0x11
#define VK_F1 0x70
#define VK_F5 0x74

#define CS_OWNDC 0x0020U
#define WS_OVERLAPPEDWINDOW 0x00CF0000UL
#define WS_VISIBLE 0x10000000UL
#define CW_USEDEFAULT ((int)0x80000000UL)
#define PM_REMOVE 0x0001U
#define IDC_ARROW ((LPCSTR)32512)

#define PFD_DRAW_TO_WINDOW 0x00000004UL
#define PFD_SUPPORT_OPENGL 0x00000020UL
#define PFD_DOUBLEBUFFER 0x00000001UL
#define PFD_TYPE_RGBA 0
#define PFD_MAIN_PLANE 0

#define GetFileExInfoStandard 0

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

BOOL GetFileAttributesExA(LPCSTR, int, LPVOID);
long CompareFileTime(const FILETIME *, const FILETIME *);
HCURSOR LoadCursor(HINSTANCE, LPCSTR);
BOOL RegisterClassA(const WNDCLASSA *);
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

#endif
