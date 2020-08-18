#include <assert.h>
#include <stdint.h>
#include <commctrl.h>
#include <windowsx.h>
#include "precomp.h"

typedef struct _main_window {
  ATOM wndclass;
  HWND hwnd;
} main_window_t;

main_window_t g_main_window;

typedef struct _checker {
  int size;
  BOOL invalidate;
  HBRUSH hbrush;
} checker_t;

checker_t g_checker;

void set_checker_size(int size)
{
  InvalidateRect(g_main_window.hwnd, NULL, FALSE);
  g_checker.size = size; 
  g_checker.invalidate = TRUE;
}

#define IDCB_SIZE_SELECTOR 1001


HBRUSH get_checker_brush(HDC hdc)
{
  if (g_checker.invalidate)
  {
    HBRUSH hChecker1 = CreateSolidBrush(RGB(0xCC, 0xCC, 0xCC));
    HBRUSH hChecker2 = CreateSolidBrush(RGB(255, 255, 255));

    HBITMAP hPatBmp = CreateCompatibleBitmap(hdc, g_checker.size,
        g_checker.size);
    HDC hPatDC = CreateCompatibleDC(hdc); 
    SelectObject(hPatDC, hPatBmp);

    RECT rcBack = {0, 0, g_checker.size, g_checker.size}; 
    FillRect(hPatDC, &rcBack, hChecker2);

    RECT rcLump1 = {0, 0, g_checker.size/2, g_checker.size/2};
    FillRect(hPatDC, &rcLump1, hChecker1);

    RECT rcLump2 = {g_checker.size/2, g_checker.size/2, g_checker.size,
        g_checker.size};
    FillRect(hPatDC, &rcLump2, hChecker1);

    HBRUSH hCheckerBrush = CreatePatternBrush(hPatBmp);   
    assert(hCheckerBrush);
    g_checker.hbrush = hCheckerBrush;

    g_checker.invalidate = FALSE;
  }

  return g_checker.hbrush;
}

HWND hSizeSelector;

void main_window_onpaint(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
  PAINTSTRUCT ps;
  HDC hdc;
  hdc = BeginPaint(hwnd, &ps);

  HBRUSH hCheckerBrush = get_checker_brush(hdc);


  RECT rcCanvas;
  GetClientRect(hwnd, &rcCanvas);
  rcCanvas.top = 64;
  FillRect(hdc, &rcCanvas, hCheckerBrush);

  EndPaint(hwnd, &ps);
}

LRESULT CALLBACK main_window_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message)
  {
    case WM_CREATE:
    {
      set_checker_size(32);

      hSizeSelector = CreateWindowEx(0, WC_COMBOBOX, NULL,
          CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED
          | WS_VISIBLE, 10, 10, 100, 26, hwnd, (HMENU)IDCB_SIZE_SELECTOR,
          GetModuleHandle(NULL), NULL);

      ComboBox_AddString(hSizeSelector, L"16");
      ComboBox_AddString(hSizeSelector, L"24");
      ComboBox_AddString(hSizeSelector, L"32");
      ComboBox_AddString(hSizeSelector, L"48");
      ComboBox_AddString(hSizeSelector, L"64");
    }
      break;
    case WM_COMMAND:
      if (LOWORD(wparam) == IDCB_SIZE_SELECTOR &&
          HIWORD(wparam) == CBN_SELCHANGE)
      {
        switch (ComboBox_GetCurSel((HWND)lparam))
        {
          case 0:
            set_checker_size(16);
            break;
          case 1:
            set_checker_size(24);
            break;
          case 2:
            set_checker_size(32);
            break;
          case 3:
            set_checker_size(48);
            break;
          case 4:
            set_checker_size(64);
            break;
          default:
            set_checker_size(24);
            break;
        }
      }
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_PAINT:
      main_window_onpaint(hwnd, wparam, lparam);
      break;
    default:
      return DefWindowProc(hwnd, message, wparam, lparam);
      break;
  }

  return 0;
}

void main_window_register_class(HINSTANCE hInstance)
{
  WNDCLASSEX wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)main_window_wndproc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"Win32Class_MainWindow";
  wcex.lpszMenuName = NULL;
  wcex.hIconSm = NULL;

  ATOM class_atom = RegisterClassEx(&wcex);
  assert(class_atom);
  g_main_window.wndclass = class_atom;
}

void main_window_create()
{
  HWND handle = CreateWindowEx(0, MAKEINTATOM(g_main_window.wndclass), L"Checker",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL); 

  assert(handle);

  g_main_window.hwnd = handle;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  main_window_register_class(hInstance);
  main_window_create();
  ShowWindow(g_main_window.hwnd, nCmdShow);

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}
