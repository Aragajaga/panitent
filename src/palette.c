#include "precomp.h"

#include <stdint.h>
#include <stdio.h>
#include <windowsx.h>

#include "palette.h"

#include "color_context.h"

palette_dialog_t g_palette_dialog;

uint32_t palette_colors[] = {
    0xFF000000, /* Black */
    0xFFFFFFFF, /* White */
    0xFFCCCCCC, /* Light Gray */
    0xFF888888, /* Gray */
    0xFF444444, /* Dark Gray */

    0xFFFF8888, /* Light Red */
    0xFFFFFF88, /* Light Yellow */
    0xFF88FF88, /* Light Green */
    0xFF88FFFF, /* Light Cyan */
    0xFF8888FF, /* Light Blue */
    0xFFFF88FF, /* Light Magenta */

    0xFFFF0000, /* Red */
    0xFFFFFF00, /* Yellow */
    0xFF00FF00, /* Green */
    0xFF00FFFF, /* Cyan */
    0xFF0000FF, /* Blue */
    0xFFFF00FF, /* Magenta */

    0xFF880000, /* Crimson */
    0xFF888800, /* Gold */
    0xFF008800, /* Dark Green */
    0xFF008888, /* Dark Cyan */
    0xFF000088, /* Dark Blue */
    0xFF880088, /* Dark Magenta */
};

uint32_t abgr_to_argb(uint32_t abgr)
{
  uint32_t b = (abgr >> 16) & 0xFF;
  uint32_t r = abgr & 0xFF;
  return (abgr & 0xFF00FF00) | (r << 16) | b;
}

int swatch_size   = 16;
int swatch_margin = 2;

void PaletteWindow_DrawSwatch(HDC hdc, int x, int y, COLORREF color)
{
  HDC original = SelectObject(hdc, GetStockObject(DC_BRUSH));

  SetDCBrushColor(hdc, color);
  Rectangle(hdc, x, y, x + swatch_size, y + swatch_size);

  SelectObject(hdc, original);
}

void PaletteWindow_OnPaint(HWND hwnd)
{
  PAINTSTRUCT ps;
  HDC hdc;

  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);

  if (width_indices < 1)
    width_indices = 1;

  hdc = BeginPaint(hwnd, &ps);

  PaletteWindow_DrawSwatch(hdc, 10, 10,
      (COLORREF)abgr_to_argb(g_color_context.fg_color) & 0x00FFFFFF);
  PaletteWindow_DrawSwatch(hdc, 40, 10,
      (COLORREF)abgr_to_argb(g_color_context.bg_color) & 0x00FFFFFF);

  uint32_t color = 0x00ff0000;
  for (size_t i = 0; i < sizeof(palette_colors) / sizeof(uint32_t); i++) {
    PaletteWindow_DrawSwatch(hdc,
                10 + (i % width_indices) * (swatch_size + swatch_margin),
                40 + (i / width_indices) * (swatch_size + swatch_margin),
                (COLORREF)(palette_colors[i] & 0x00FFFFFF));

    color >>= 1;
  }

  EndPaint(hwnd, &ps);
}

int PaletteWindow_PosToSwatchIndex(int x, int y)
{
  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int width_indices = (rc.right - 20) / (swatch_size + swatch_margin);
  int swatch_outer  = swatch_size + swatch_margin;

  x -= 10;
  y -= 40;

  int index = y / swatch_outer * width_indices + x / swatch_outer;
  printf("Index: %d\n", index);

  return index;
}

uint32_t get_color(int index)
{
  return palette_colors[index];
}

void PaletteWindow_OnLButtonUp(HWND hwnd, LPARAM lParam)
{
  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int swatch_count   = sizeof(palette_colors) / sizeof(uint32_t);
  int width_indices  = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) &&
      ((y - 40) < ((swatch_size + swatch_margin) * height_indices))) {
    int index = PaletteWindow_PosToSwatchIndex(x, y);
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.fg_color = abgr_to_argb(get_color(index));

    RECT rc = {10, 10, 34, 34};
    InvalidateRect(hwnd, &rc, FALSE);
  }
}

void PaletteWindow_OnRButtonUp(HWND hwnd, LPARAM lParam)
{
  RECT rc = {0};
  GetClientRect(g_palette_dialog.win_handle, &rc);
  int swatch_count   = sizeof(palette_colors) / sizeof(uint32_t);
  int width_indices  = (rc.right - 20) / (swatch_size + swatch_margin);
  int height_indices = swatch_count / width_indices + 1;

  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);

  if (((x - 10) < ((swatch_size + swatch_margin) * width_indices)) &&
      ((y - 40) < ((swatch_size + swatch_margin) * height_indices))) {
    int index = PaletteWindow_PosToSwatchIndex(x, y);
    if (index < 0 || index >= swatch_count)
      return;

    g_color_context.bg_color = abgr_to_argb(get_color(index));

    RECT rc = {40, 10, 64, 34};
    InvalidateRect(hwnd, &rc, FALSE);
  }
}

void Palette_ColorChangeObserver(void* userData, uint32_t fg, uint32_t bg)
{
  HWND hWnd = (HWND)userData;
  if (!hWnd)
    return;

  InvalidateRect(hWnd, NULL, TRUE);
}

LRESULT CALLBACK PaletteWindow_WndProc(HWND hwnd,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam)
{
  switch (message) {
  case WM_CREATE:
    {
      RegisterColorObserver(Palette_ColorChangeObserver, (void*)hwnd);
      g_palette_dialog.win_handle = hwnd;
    }
    break;
  case WM_PAINT:
    PaletteWindow_OnPaint(hwnd);
    break;
  case WM_LBUTTONUP:
    PaletteWindow_OnLButtonUp(hwnd, lparam);
    break;
  case WM_RBUTTONUP:
    PaletteWindow_OnRButtonUp(hwnd, lparam);
    break;
  case WM_DESTROY:
    RemoveColorObserver(Palette_ColorChangeObserver, (void*)hwnd);
    break;
  default:
    return DefWindowProc(hwnd, message, wparam, lparam);
    break;
  }

  return 0;
}

void PaletteWindow_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex    = {0};
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = (WNDPROC)PaletteWindow_WndProc;
  wcex.cbWndExtra    = 0;
  wcex.cbClsExtra    = 0;
  wcex.hInstance     = hInstance;
  wcex.hIcon         = NULL;
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcex.lpszClassName = L"Win32Class_PaletteWindow";
  wcex.lpszMenuName  = NULL;
  wcex.hIconSm       = NULL;
  ATOM classAtom     = RegisterClassEx(&wcex);

  if (!classAtom)
    printf("Failed to register PaletteWindow");
}

void PaletteWindow_Create(HWND parent)
{
  HWND hPalette = CreateWindowEx(WS_EX_PALETTEWINDOW,
                                 L"Win32Class_PaletteWindow",
                                 L"Palette",
                                 WS_CAPTION | WS_THICKFRAME | WS_VISIBLE,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 300,
                                 200,
                                 parent,
                                 NULL,
                                 GetModuleHandle(NULL),
                                 NULL);

  if (!hPalette)
    printf("Failed to create window");
}
