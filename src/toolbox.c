#include "precomp.h"

#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <math.h>
#include <stdio.h>

#include "primitives_context.h"
#include "option_bar.h"
#include "tool.h"
#include "toolbox.h"
#include "viewport.h"
#include "canvas.h"
#include "debug.h"
#include "resource.h"
#include "color_context.h"
#include "palette.h"

extern viewport_t g_viewport;

extern tool_t g_tool;

tool_t g_tool_circle;
tool_t g_tool_line;
tool_t g_tool_pencil;
tool_t g_tool_pointer;
tool_t g_tool_rectangle;
tool_t g_tool_text;

static BOOL fDraw = FALSE;
static POINT prev;

void toolbox_add_tool(toolbox_t* tbox, tool_t tool)
{
  tbox->tools[(size_t)tbox->tool_count] = tool;
  tbox->tool_count++;
}

void toolbox_remove_tool(toolbox_t* tbox)
{
  if (tbox->tool_count) {
    tbox->tool_count--;
  }
}

HBITMAP img_layout;

void toolbox_init(toolbox_t* tbox)
{
  tool_pointer_init();
  tool_pencil_init();
  tool_line_init();
  tool_circle_init();
  tool_rectangle_init();
  tool_text_init();

  g_tool = g_tool_pointer;

  img_layout = (HBITMAP)LoadBitmap(GetModuleHandle(NULL),
      MAKEINTRESOURCE(IDB_TOOLS));

  tbox->tools      = calloc(sizeof(tool_t), 8);
  tbox->tool_count = 0;

  toolbox_add_tool(tbox, g_tool_pointer);
  toolbox_add_tool(tbox, g_tool_pencil);
  toolbox_add_tool(tbox, g_tool_circle);
  toolbox_add_tool(tbox, g_tool_line);
  toolbox_add_tool(tbox, g_tool_rectangle);
  toolbox_add_tool(tbox, g_tool_text);
}

HTHEME hTheme = NULL;
int btnSize   = 24;
int btnOffset = 4;

void toolbox_button_draw(HDC hdc, int x, int y)
{
  if (!hTheme) {
    HWND hButton = CreateWindowEx(0,
                                  L"BUTTON",
                                  L"",
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  NULL,
                                  NULL,
                                  GetModuleHandle(NULL),
                                  NULL);
    hTheme       = OpenThemeData(hButton, L"BUTTON");
  }

  RECT rc = {x, y, x + btnSize, y + btnSize};
  DrawThemeBackgroundEx(hTheme, hdc, BP_PUSHBUTTON, PBS_NORMAL, &rc, NULL);
}

void toolbox_draw_buttons(toolbox_t* tbox, HDC hdc)
{
  BITMAP bitmap;
  HDC hdcMem;
  HGDIOBJ oldBitmap;

  hdcMem    = CreateCompatibleDC(hdc);
  oldBitmap = SelectObject(hdcMem, img_layout);
  GetObject(img_layout, sizeof(bitmap), &bitmap);

  RECT rcClient = {0};
  GetClientRect(tbox->hwnd, &rcClient);

  size_t extSize  = btnSize + btnOffset;
  size_t rowCount = rcClient.right / btnSize;

  if (rowCount < 1) {
    rowCount = 1;
  }

  for (size_t i = 0; i < tbox->tool_count; i++) {
    int x = btnOffset + (i % rowCount) * extSize;
    int y = btnOffset + (i / rowCount) * extSize;

    /* Rectangle(hdc, x, y, x+btnSize, y+btnSize); */
    toolbox_button_draw(hdc, x, y);

    int iBmp         = tbox->tools[(ptrdiff_t)i].img;
    const int offset = 4;
    TransparentBlt(hdc,
                   x + offset,
                   y + offset,
                   16,
                   bitmap.bmHeight,
                   hdcMem,
                   16 * iBmp,
                   0,
                   16,
                   bitmap.bmHeight,
                   (UINT)0x00FF00FF);
  }

  SelectObject(hdcMem, oldBitmap);
  DeleteDC(hdcMem);
}

void toolbox_onpaint(toolbox_t* tbox)
{
  PAINTSTRUCT ps;
  HDC hdc;
  hdc = BeginPaint(tbox->hwnd, &ps);

  toolbox_draw_buttons(tbox, hdc);

  EndPaint(tbox->hwnd, &ps);
}

void toolbox_onlbuttonup(toolbox_t* tbox, MOUSEEVENT mEvt)
{
  int x = LOWORD(mEvt.lParam);
  int y = HIWORD(mEvt.lParam);

  int btnHot = btnSize + 5;

  if (x >= 0 && y >= 0 && x < btnHot * 2 &&
      y < (int)tbox->tool_count * btnHot) {
    size_t extSize = btnSize + btnOffset;

    RECT rcClient = {0};
    GetClientRect(mEvt.hwnd, &rcClient);

    size_t rowCount = rcClient.right / extSize;
    if (rowCount < 1) {
      rowCount = 1;
    }

    unsigned int pressed =
        (y - btnOffset) / extSize * rowCount + (x - btnOffset) / extSize;

    if (tbox->tool_count > pressed) {
      switch (pressed) {
      case 1:
        g_tool = g_tool_pencil;
        break;
      case 2:
        g_tool = g_tool_circle;
        break;
      case 3:
        g_tool = g_tool_line;
        break;
      case 4:
        g_tool = g_tool_rectangle;
        break;
      case 5:
        g_tool = g_tool_text;
        break;
      default:
        g_tool = g_tool_pointer;
        break;
      }
    }
  }
}

LRESULT CALLBACK toolbox_wndproc(HWND hwnd,
                                 UINT msg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
  MOUSEEVENT mevt;
  mevt.hwnd   = hwnd;
  mevt.lParam = lParam;

  switch (msg) {
  case WM_CREATE:
  {
    LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;
    toolbox_t* tbox   = (toolbox_t*)cs->lpCreateParams;
    tbox->hwnd        = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)tbox);

    toolbox_init(tbox);
  } break;
  case WM_PAINT:
  {
    toolbox_t* tbox = (toolbox_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (tbox) {
      toolbox_onpaint(tbox);
    }
  } break;
  case WM_LBUTTONUP:
  {
    toolbox_t* tbox = (toolbox_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (tbox) {
      toolbox_onlbuttonup(tbox, mevt);
    }
  } break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

void toolbox_register_class()
{
  WNDCLASS wc      = {0};
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = toolbox_wndproc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = TOOLBOX_WC;

  RegisterClass(&wc);
}

void toolbox_unregister_class()
{
  UnregisterClass(TOOLBOX_WC, NULL);
}

void tool_pointer_init()
{
  g_tool_pointer.label = L"Pointer";
  g_tool_pointer.img   = 0;
}

void tool_text_onlbuttonup(MOUSEEVENT mEvt)
{
  HDC viewportdc = GetDC(g_viewport.hwnd);
  HDC bitmapdc   = CreateCompatibleDC(viewportdc);

  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  wchar_t textbuf[1024];
  DWORD textLen = 0;
  GetWindowText(g_option_bar.textstring_handle,
                textbuf,
                sizeof(textbuf) / sizeof(wchar_t));
  textLen = wcslen(textbuf);

  HFONT hFont = CreateFont(24, 0, 0, 0, 600, FALSE, FALSE, FALSE,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      DEFAULT_PITCH, L"Arial");
  HGDIOBJ hOldFont = SelectObject(bitmapdc, hFont);

  SIZE size = {0};
  GetTextExtentPoint32(bitmapdc, textbuf, textLen, &size);
  RECT textrc = {0, 0, size.cx, size.cy};

  uint8_t* bmbuf = calloc(size.cx * size.cy, sizeof(uint32_t));

  HBITMAP hbitmap =
      CreateBitmap(size.cx, size.cy, 1, sizeof(uint32_t) * 8, bmbuf);
  SelectObject(bitmapdc, hbitmap);
  SetTextColor(bitmapdc, abgr_to_argb(g_color_context.fg_color));

  DrawTextEx(bitmapdc, textbuf, -1, &textrc, 0, NULL);

  SelectObject(bitmapdc, hOldFont);

  void* inbuf = malloc(size.cx * size.cy * 8);

  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
  bmi.bmiHeader.biWidth = size.cx;
  bmi.bmiHeader.biHeight = size.cy;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  GetDIBits(bitmapdc, hbitmap, 0, size.cy, inbuf, &bmi, DIB_RGB_COLORS);
  canvas_paste_bits(g_viewport.document->canvas, inbuf, x, y, size.cx, size.cy);

  free(inbuf);
  DeleteObject(hbitmap);
  DeleteDC(bitmapdc);
}

void tool_text_init()
{
  g_tool_text.label       = L"Text";
  g_tool_text.img         = 6;
  g_tool_text.onlbuttonup = tool_text_onlbuttonup;
}

void tool_pencil_onlbuttondown(MOUSEEVENT mEvt)
{
  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  prev.x = LOWORD(mEvt.lParam);
  prev.y = HIWORD(mEvt.lParam);
}

void tool_pencil_onlbuttonup(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(mEvt.hwnd);
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    canvas_t* canvas = g_viewport.document->canvas;

    rect_t line_rect = {0};
    line_rect.x0     = prev.x;
    line_rect.y0     = prev.y;
    line_rect.x1     = x;
    line_rect.y1     = y;

    draw_line(canvas, line_rect);

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
#endif
  }
  fDraw = FALSE;
  ReleaseCapture();
}

void tool_pencil_onmousemove(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;
    hdc = GetDC(mEvt.hwnd);

    /* Draw overlay path */
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    /* Draw on canvas */
    canvas_t* canvas = g_viewport.document->canvas;

    rect_t line_rect = {0};
    line_rect.x0     = prev.x;
    line_rect.y0     = prev.y;
    line_rect.x1     = x;
    line_rect.y1     = y;

    draw_line(canvas, line_rect);

    prev.x = x;
    prev.y = y;

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
#endif
  }
}

void tool_pencil_init()
{
  g_tool_pencil.label         = L"Pencil";
  g_tool_pencil.img           = 1;
  g_tool_pencil.onlbuttonup   = tool_pencil_onlbuttonup;
  g_tool_pencil.onlbuttondown = tool_pencil_onlbuttondown;
  g_tool_pencil.onmousemove   = tool_pencil_onmousemove;
}

POINT circCenter;

void tool_circle_onlbuttondown(MOUSEEVENT mEvt)
{
  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  circCenter.x = LOWORD(mEvt.lParam);
  circCenter.y = HIWORD(mEvt.lParam);
}

void tool_circle_onlbuttonup(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
#ifdef PEN_OVERLAY
    HDC hdc;

    hdc = GetDC(mEvt.hwnd);
    MoveToEx(hdc, prev.x, prev.y, NULL);
    LineTo(hdc, x, y);
#endif

    int radius = sqrt(pow(x - circCenter.x, 2) + pow(y - circCenter.y, 2));

    canvas_t* canvas = g_viewport.document->canvas;
    draw_circle(canvas, circCenter.x, circCenter.y, radius);

#ifdef PEN_OVERLAY
    ReleaseDC(mEvt.hwnd, hdc);
#endif
  }
  fDraw = FALSE;
  ReleaseCapture();
}

void tool_circle_init()
{
  g_tool_circle.label         = L"Circle";
  g_tool_circle.img           = 2;
  g_tool_circle.onlbuttonup   = tool_circle_onlbuttonup;
  g_tool_circle.onlbuttondown = tool_circle_onlbuttondown;
}

void tool_line_onlbuttondown(MOUSEEVENT mEvt)
{
  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  prev.x = LOWORD(mEvt.lParam);
  prev.y = HIWORD(mEvt.lParam);
}

void tool_line_onlbuttonup(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
    canvas_t* canvas = g_viewport.document->canvas;

    rect_t line_rect = {0};
    line_rect.x0     = prev.x;
    line_rect.y0     = prev.y;
    line_rect.x1     = x;
    line_rect.y1     = y;

    draw_line(canvas, line_rect);
  }
  fDraw = FALSE;
  ReleaseCapture();
}

void tool_line_init()
{
  g_tool_line.label         = L"Line";
  g_tool_line.img           = 3;
  g_tool_line.onlbuttonup   = tool_line_onlbuttonup;
  g_tool_line.onlbuttondown = tool_line_onlbuttondown;
}

void tool_rectangle_onlbuttonup(MOUSEEVENT mEvt)
{
  fDraw = TRUE;
  SetCapture(mEvt.hwnd);
  prev.x = LOWORD(mEvt.lParam);
  prev.y = HIWORD(mEvt.lParam);
}

void tool_rectangle_onlbuttondown(MOUSEEVENT mEvt)
{
  signed short x = LOWORD(mEvt.lParam);
  signed short y = HIWORD(mEvt.lParam);

  if (fDraw) {
    canvas_t* canvas = g_viewport.document->canvas;

    rect_t rc = {0};
    rc.x0     = prev.x;
    rc.y0     = prev.y;
    rc.x1     = x;
    rc.y1     = y;
    draw_rectangle(canvas, rc);
  }
  fDraw = FALSE;
  ReleaseCapture();
}

void tool_rectangle_init()
{
  g_tool_rectangle.label         = L"Rectangle";
  g_tool_rectangle.img           = 4;
  g_tool_rectangle.onlbuttonup   = tool_rectangle_onlbuttonup;
  g_tool_rectangle.onlbuttondown = tool_rectangle_onlbuttondown;
}
