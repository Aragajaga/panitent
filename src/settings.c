#include "precomp.h"

#include <commctrl.h>
#include <windowsx.h>
#include <knownfolders.h>

#include "panitent.h"
#include "settings.h"

#define IDT_SETTINGS_PAGE_MAIN  0
#define IDT_SETTINGS_PAGE_DEBUG 1

struct rlist {
  size_t size;
  size_t capacity;
  HWND* array;
};

struct callback_entry {
  int id;
  void (*callback)(WPARAM, LPARAM);
};

struct event_handlers {
  size_t size;
  size_t capacity;
  struct callback_entry** entries;
} g_handlers;

static WCHAR szSettingsWndClass[] = L"Win32Ctl_SettingsWindow";

HWND hTabControl;
HWND hButtonCancel;
HWND hButtonOk;

HWND hTPMain;
HWND hTPDebug;

HWND g_hTabActive;
HWND g_hSettingsWindow;

static LRESULT CALLBACK Settings_WndProc(HWND, UINT, WPARAM, LPARAM);
static void OnCreate(HWND, WPARAM, LPARAM);
static void OnSize(HWND, WPARAM, LPARAM);
static void OnDestroy(HWND, WPARAM, LPARAM);
static void OnNotify(HWND, WPARAM, LPARAM);
static void OnCommand(HWND, WPARAM, LPARAM);
static void OnGetMinMaxInfo(HWND, WPARAM, LPARAM);

static void InitSettingsWindow(HWND);
static void SwitchTab(HWND);
static void HideActiveTab();
HWND CreateCheckbox(HWND, const wchar_t*, unsigned int, unsigned int);

static BOOL RegisterSettingsTabPageMain(HINSTANCE);
static BOOL RegisterSettingsTabPageDebug(HINSTANCE);

int new_control_id();
void alloc_handlers_vault();
void register_event_handler(int, void (*)(WPARAM, LPARAM));
void process_event(WPARAM, LPARAM);

/* rlist */
void rlist_create(struct rlist*, size_t);
void rlist_add(struct rlist*, HWND);
void rlist_destroy(struct rlist*);

/* mock callback */
void shima(WPARAM, LPARAM);

static LRESULT CALLBACK Settings_WndProc(HWND hWnd, UINT message, WPARAM wParam,
    LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE:
      OnCreate(hWnd, wParam, lParam);
      return 0;

    case WM_SIZE:
      OnSize(hWnd, wParam, lParam);
      return 0;

    case WM_DESTROY:
      OnDestroy(hWnd, wParam, lParam);
      return 0;

    case WM_NOTIFY:
      OnNotify(hWnd, wParam, lParam);
      return 0;

    case WM_COMMAND:
      OnCommand(hWnd, wParam, lParam);
      return 0;

    case WM_GETMINMAXINFO:
      OnGetMinMaxInfo(hWnd, wParam, lParam);
      return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL SettingsWindow_Register(HINSTANCE hInstance)
{
  RegisterSettingsTabPageMain(hInstance);
  RegisterSettingsTabPageDebug(hInstance);

  WNDCLASS wc      = {0};
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)Settings_WndProc;
  wc.hInstance     = hInstance;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = szSettingsWndClass;

  return RegisterClass(&wc);
}

static void OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  g_hSettingsWindow = hWnd;

  InitSettingsWindow(hWnd);
}

static void OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  UINT width  = GET_X_LPARAM(lParam);
  UINT height = GET_Y_LPARAM(lParam);

  SetWindowPos(hTabControl, HWND_TOP, 10, 10, width - 20, height - 60, 0);
  SetWindowPos(hButtonCancel, HWND_TOP, width - 110, height - 40, 100, 30, 0);
  SetWindowPos(hButtonOk, HWND_TOP, width - 220, height - 40, 100, 30, 0);

  RECT vrc;
  GetClientRect(hTabControl, &vrc);
  vrc.left += 10;
  vrc.top += 10;
  vrc.right += 10;
  vrc.bottom += 10;
  TabCtrl_AdjustRect(hTabControl, FALSE, &vrc);

  SetWindowPos(hTPMain, HWND_TOP,
      vrc.left,
      vrc.top,
      vrc.right - vrc.left,
      vrc.bottom - vrc.top,
      0);

  SetWindowPos(hTPDebug, HWND_TOP,
      vrc.left,
      vrc.top,
      vrc.right - vrc.left,
      vrc.bottom - vrc.top,
      0);
}

static void OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  g_hSettingsWindow = NULL;
}

static void OnNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  LPNMHDR pNm = (LPNMHDR)lParam;

  /* At now we expect only tab changing messages */
  if (pNm->hwndFrom == hTabControl &&
      pNm->code == TCN_SELCHANGE)
  {
    return;
  }

  UINT tabId = TabCtrl_GetCurSel(hTabControl);

  HWND hNextTab = NULL;
  switch (tabId) {
    case IDT_SETTINGS_PAGE_MAIN:
      hNextTab = hTPMain;
      break;
    case IDT_SETTINGS_PAGE_DEBUG:
      hNextTab = hTPDebug;
      break;
  }

  SwitchTab(hNextTab);
}

static void OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  if (LOWORD(wParam) == IDOK &&
      HIWORD(wParam) == BN_CLICKED)
  {
    DestroyWindow(hWnd);
  }
}

static void OnGetMinMaxInfo(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  RECT rc   = {0};
  rc.right  = 640;
  rc.bottom = 480;
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

  LPMINMAXINFO lpMMI      = (LPMINMAXINFO)lParam;
  lpMMI->ptMinTrackSize.x = rc.right - rc.left;
  lpMMI->ptMinTrackSize.y = rc.bottom - rc.top;
}


int ShowSettingsWindow(HWND hParent)
{
  if (g_hSettingsWindow)
  {
    ShowWindow(g_hSettingsWindow, SW_SHOW);
    return 0;
  }

  RECT rc   = {0};
  rc.right  = 640;
  rc.bottom = 480;
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

  g_hSettingsWindow = CreateWindow(szSettingsWndClass, L"Settings",
      WS_VISIBLE | WS_OVERLAPPEDWINDOW,
      (GetSystemMetrics(SM_CXSCREEN) - rc.right - rc.left) / 2,
      (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2,
      rc.right - rc.left,
      rc.bottom - rc.top,
      hParent,
      NULL,
      (HINSTANCE)GetWindowLongPtr(hParent, GWLP_HINSTANCE),
      NULL);

  return 0;
}

static void InitSettingsWindow(HWND hwnd)
{
  hTabControl = CreateWindowEx(0, WC_TABCONTROL, NULL, WS_CHILD | WS_VISIBLE,
      10, 10, 620, 180, hwnd, NULL,
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  SetGuiFont(hTabControl);

  TCITEM tab_main  = {0};
  tab_main.mask    = TCIF_TEXT;
  tab_main.pszText = L"Main";
  tab_main.iImage  = -1;
  TabCtrl_InsertItem(hTabControl, IDT_SETTINGS_PAGE_MAIN, &tab_main);

  TCITEM tab_debug  = {0};
  tab_debug.mask    = TCIF_TEXT;
  tab_debug.pszText = L"Debug";
  tab_debug.iImage  = -1;
  TabCtrl_InsertItem(hTabControl, IDT_SETTINGS_PAGE_DEBUG, &tab_debug);

  RECT tab_view_rc   = {0};
  tab_view_rc.left   = 10;
  tab_view_rc.top    = 10;
  tab_view_rc.right  = 640;
  tab_view_rc.bottom = 200;
  TabCtrl_AdjustRect(hTabControl, FALSE, &tab_view_rc);

  hTPMain = CreateWindow(L"SettingsTabPageMain", NULL, WS_CHILD | WS_VISIBLE,
      tab_view_rc.left,
      tab_view_rc.top,
      tab_view_rc.right - tab_view_rc.left,
      tab_view_rc.bottom - tab_view_rc.top,
      hwnd, NULL, GetModuleHandle(NULL), NULL);

  hTPDebug = CreateWindow(L"SettingsTabPageDebug", NULL, WS_CHILD,
      tab_view_rc.left,
      tab_view_rc.top,
      tab_view_rc.right - tab_view_rc.left,
      tab_view_rc.bottom - tab_view_rc.top,
      hwnd, NULL, GetModuleHandle(NULL), NULL);

  hButtonCancel = CreateWindow(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE,
      420, 440, 100, 30, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);

  SetGuiFont(hButtonCancel);

  hButtonOk = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE,
      530, 440, 100, 30,
      hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);

  SetGuiFont(hButtonOk);
}

static void HideActiveTab()
{
  if (g_hTabActive == NULL)
    return;

  ShowWindow(g_hTabActive, SW_HIDE);
}

static void SwitchTab(HWND hTab)
{
  if (hTab == g_hTabActive)
    return;

  HideActiveTab();

  if (hTab == NULL)
    return;

  g_hTabActive = hTab;
  ShowWindow(g_hTabActive, SW_SHOW);
}

int new_control_id()
{
  /* TODO: Reuse freed ids */
  static int i = 1201;
  return i++;
}

void shima(__attribute__((unused)) WPARAM wParam, LPARAM lParam)
{
  DWORD bCheck = Button_GetCheck((HWND)lParam);

  if (bCheck == BST_UNCHECKED) {
    int result = MessageBox(g_hSettingsWindow,
                            L"Are you sure?",
                            L"Warning",
                            MB_YESNO | MB_ICONEXCLAMATION);
    if (result == IDNO) {
      return;
    }
  }

  Button_SetCheck((HWND)lParam,
                  bCheck == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
}

HWND CreateCheckbox(HWND parent, const wchar_t* label, unsigned int posX,
    unsigned int posY)
{
  size_t id = new_control_id();
  register_event_handler(id, &shima);

  HWND hWnd = CreateWindow(L"BUTTON", label, WS_CHILD | WS_VISIBLE |
      BS_CHECKBOX, posX, posY, 250, 20, parent, (HMENU)id,
      GetModuleHandle(NULL), NULL);
  SetGuiFont(hWnd);

  return hWnd;
}

void register_event_handler(int id, void (*callback)(WPARAM wParam,
      LPARAM lParam))
{
  size_t* cap = &g_handlers.capacity;

  if (*cap == 0)
    return;

  /*
   * if modulo 16 of capacity is zero, that means there no more space
   * left for the new entry.
   */
  if (g_handlers.size >= *cap) {
    size_t new_cap     = (*cap / 16 + 1) * 16;
    g_handlers.entries = realloc(g_handlers.entries, new_cap);
    *cap               = new_cap;
  }

  struct callback_entry* e = calloc(1, sizeof(struct callback_entry));
  e->id                    = id;
  e->callback              = callback;
  g_handlers.entries[g_handlers.size++] = e;
}

void process_event(WPARAM wParam, LPARAM lParam)
{
  for (size_t i = 0; i < g_handlers.size; i++) {
    if (LOWORD(wParam) == g_handlers.entries[i]->id &&
        g_handlers.entries[i]->callback != NULL) {
      g_handlers.entries[i]->callback(wParam, lParam);
      break;
    }
  }
}

void rlist_create(struct rlist* list, size_t capacity)
{
  list->array    = calloc(capacity, sizeof(HWND));
  list->capacity = capacity;
  list->size     = 0;
}

void rlist_add(struct rlist* list, HWND hWnd)
{
  size_t* cap = &list->capacity;

  if (list->size >= *cap) {
    size_t new_cap = (*cap / 16 + 1) * 16;
    list->array    = realloc(list->array, new_cap);
    *cap           = new_cap;
  }

  SetWindowPos(hWnd, 0, 10, 10 + 20 * list->size++, 0, 0, SWP_NOSIZE);
}

void alloc_handlers_vault()
{
  g_handlers.entries  = calloc(2, sizeof(struct callback_entry));
  g_handlers.capacity = 2;
  g_handlers.size     = 0;
}

void rlist_destroy(struct rlist* list)
{
  free(list->array);
  list->array    = NULL;
  list->capacity = list->size = 0;
}

struct rlist r = {0};

LRESULT CALLBACK SettingsTabPageMainProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
  switch (msg) {
    case WM_CREATE: {
      alloc_handlers_vault();
      rlist_create(&r, 2);
      rlist_add(&r, CreateCheckbox(hwnd, L"Show border", 0, 0));
    } break;
    case WM_DESTROY:
      rlist_destroy(&r);
      break;
    case WM_COMMAND:
      process_event(wParam, lParam);
      break;

    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

BOOL RegisterSettingsTabPageMain(HINSTANCE hInstance)
{
  WNDCLASS wc      = {0};
  wc.lpfnWndProc   = (WNDPROC)SettingsTabPageMainProc;
  wc.hInstance     = hInstance;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"SettingsTabPageMain";

  return RegisterClass(&wc);
}

LRESULT CALLBACK SettingsTabPageDebugProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
  switch (msg) {
    case WM_CREATE:
    {
      HWND hCheckBox = CreateWindow(L"BUTTON", L"Log actions",
          WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
          10, 10, 100, 20,
          hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
      SetGuiFont(hCheckBox);
    } break;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}

BOOL RegisterSettingsTabPageDebug(HINSTANCE hInstance)
{
  WNDCLASS wc      = {0};
  wc.lpfnWndProc   = (WNDPROC)SettingsTabPageDebugProc;
  wc.hInstance     = hInstance;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = L"SettingsTabPageDebug";

  return RegisterClass(&wc);
}
