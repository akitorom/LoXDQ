// LoXDQ.cpp
//
//

#pragma warning(disable:4100)
//#pragma warning(disable:4189)
#pragma warning(disable:4127)
#pragma warning(disable:4996)

#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <time.h>
#include <tlhelp32.h>
#include <Shlwapi.h>

//#define DEBUG
//#define DLL

// LoXDQ

#ifndef BASENAME
#define BASENAME "LoXDQ"
#endif
const TCHAR ExeBase[] = BASENAME;
const TCHAR DllFile[] = BASENAME ".dll";
const TCHAR IniPath[] = ".\\" BASENAME ".ini";

const TCHAR ClassName[] = "SQEX.CDev.Engine.Framework.MainWindow";

// Font

const TCHAR FontName[] = "Consolas";
const UINT FontSize = 18;
HFONT hfFont;

// Window

HWND hwMain = NULL;
HDC hdcMain;

// Process

DWORD Pid = 0;

BOOL Watching = FALSE;
UINT CheckPeriod = 100;
UINT ProcessPeriod = 10000;
UINT DispPerCheck = 10;

// Log

TCHAR LogPath[MAX_PATH+1] = "";
BOOL CloseLog = TRUE;

// Function

VOID WINAPI WriteState(char *s);
VOID WINAPI WriteLog(HANDLE h, char *s);
VOID WINAPI TimerProc();

// DLL
#ifdef DLL
BOOL(WINAPI *LoInitialize)(DWORD dwDQXProcessID);
INT(WINAPI *LoRead)(CHAR *buffer);
BOOL(WINAPI *LoCheck)();
#else
BOOL WINAPI LoInitialize(DWORD dwProcessID);
BOOL WINAPI LoCheck();
INT WINAPI LoRead(CHAR * buffer);
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved);
#endif

//

#ifdef DEBUG
#include "DBG.h"
#else
#define DBG __noop
#endif

////

VOID WINAPI WriteLog(HANDLE h, CHAR* s) {
  //  DBG("WriteLog: %s", s);

  DWORD b;
  WriteFile(h, s, strlen(s), &b, NULL);
}


//
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
  if (IsIconic(hWnd)) {
    return TRUE;
  }

  TCHAR sClassName[256];
  GetClassName(hWnd, sClassName, sizeof(sClassName));

  if (!lstrcmp(sClassName, ClassName)) {
    PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*)lParam;
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    Pid = pid;
    return FALSE;
  }

  return TRUE;
}

//

DWORD WINAPI SearchProcess() {
  Pid = 0;
  EnumWindows(EnumWindowsProc, 0);
  return (DWORD)Pid;
}

////

VOID WINAPI TimerProc() {
  static HANDLE h = NULL;
  static BOOL initialized = FALSE;
  static UINT dpc = 1;

  static UINT line = 0;
  static UINT _line = 1;

  //  static BOOL Initialized = NULL;

  //  DBG("TimeProc: %x %x %x\n", initialized, Pid, Base);

  if (Pid == 0) {
    SearchProcess();
    if (Pid != 0) {
      TCHAR lf[MAX_PATH+1] = "";
      time_t ti;
      struct tm *tp;
      GetPrivateProfileString(ExeBase, "LogFile", ".\\dqx.log", lf, MAX_PATH, IniPath);
      time(&ti);
      tp = localtime(&ti);
      strftime(LogPath, MAX_PATH, lf, tp);
    }
    return;
  } else {
    //    WriteState("Waiting Character...");
  }

  if (!initialized) {
    SearchProcess();
    if (Pid == 0) {
      SetTimer(hwMain, 14, 1000, NULL);
      Pid = 0;
      initialized = FALSE;
      line = 0;
      _line = 1;
      WriteState("Waiting Process...  ");
      return;
    }
    WriteState("Waiting Character...");
    initialized = LoInitialize(Pid);
    return;
  } else {
    SetTimer(hwMain, 14, CheckPeriod, NULL);
  }

  BOOL logined = LoCheck();
  if (!logined) {
    SetTimer(hwMain, 14, 1000, NULL);
    Pid = 0;
    initialized = FALSE;
    line = 0;
    _line = 1;
    WriteState("Waiting Process...  ");
    return;
  }

  dpc = dpc + 1;

  //

  static CHAR* s = NULL;
  if (!s) {
    s = (CHAR *)malloc(131072);
  }

  if (!LoRead(s)) {
    return;
  }

  if (dpc > DispPerCheck) {
    //  char* p = (char *)c + 20;
    line = atoi((char *)s + 20);
    if (line != _line) {
      char ws[21];
      snprintf(ws, 21, "Line: %d             ", line);
      WriteState(ws);
      _line = line;
      dpc = 0;
    }
    dpc = 1;
  }

  if (h == NULL) {
    h = CreateFile(LogPath,
                   GENERIC_WRITE,
                   FILE_SHARE_READ,
                   NULL,
                   OPEN_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);
    if (h == INVALID_HANDLE_VALUE) {
      MessageBox(NULL, "ERROR: CANNOT OPEN LOGFILE", ExeBase, MB_OK);
      exit(-1);
    }
    SetFilePointer(h, 0, NULL, FILE_END);
  }

  if (strlen(s) != 0) {
    DBG("S: %d\n", strlen(s));
    DBG("S: %s\n", s);
    WriteLog(h, (char *)s);
  }

  if (CloseLog) {
    CloseHandle(h);
    h = NULL;
  } else {
    FlushFileBuffers(h);
  }

  return;
}

////

VOID WINAPI WriteState(CHAR *s) {
  TextOut(hdcMain, 5, 5, s, lstrlen(s));
  InvalidateRect(hwMain, NULL, FALSE);
  return;
}

////

LRESULT CALLBACK WinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  RECT rc;
  CHAR s[16];
  static int c = 0;

  switch(Msg) {
  case WM_TIMER:
    TimerProc();
    break;
  case WM_CLOSE:
    GetWindowRect(hWnd, &rc);
    wsprintf(s, "%d", rc.left);
    WritePrivateProfileString(ExeBase, "X", s, IniPath);
    wsprintf(s, "%d", rc.top);
    WritePrivateProfileString(ExeBase, "Y", s, IniPath);
    DestroyWindow(hWnd);
    break;
  case WM_DESTROY:
    ReleaseDC(hwMain, hdcMain);
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, Msg, wParam, lParam);
  }

  return 0L;
}

////

VOID EnablePrivilege(LPTSTR lpPrivilegeName)
{
  HANDLE hToken;
  LUID luid;
  TOKEN_PRIVILEGES tp;
  BOOL r;

  r = OpenProcessToken(GetCurrentProcess(),
                       TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                       &hToken);
  if (!r) {
    return;
  }

  r = LookupPrivilegeValue(NULL, lpPrivilegeName, &luid);
  if (r) {
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tp, 0, 0, 0);
  }

  CloseHandle(hToken);
}

////

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR lpCmdLine, int nCmdShow) {
#ifdef DLL
  HMODULE h = LoadLibraryEx(DllFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (h == NULL) {
    MessageBox(NULL, "ERROR: CANNOT LOAD DLL", ExeBase, MB_OK);
  }

  LoInitialize = (BOOL(WINAPI *)(DWORD))GetProcAddress(h, "LoInitialize");
  LoCheck = (BOOL(WINAPI *)())GetProcAddress(h, "LoCheck");
  LoRead = (INT(WINAPI *)(CHAR *))GetProcAddress(h, "LoRead");
#else
  DllMain(hInstance, DLL_PROCESS_ATTACH, 0);
#endif

  TCHAR lf[MAX_PATH+1] = "";
  time_t ti;
  struct tm *tp;
  GetPrivateProfileString(ExeBase, "LogPath", ".\\dqx.log", lf, MAX_PATH, IniPath);
  time(&ti);
  tp = localtime(&ti);
  strftime(LogPath, MAX_PATH, lf, tp);

  int cps = GetPrivateProfileInt(ExeBase, "CheckPerSecond", 10, IniPath);
  CheckPeriod = (UINT)1000 / cps;
  DispPerCheck = (UINT)1000 / CheckPeriod;

  int X = GetPrivateProfileInt(ExeBase, "X", 50, IniPath);
  int Y = GetPrivateProfileInt(ExeBase, "Y", 50, IniPath);
  int W = GetPrivateProfileInt(ExeBase, "W", 96, IniPath);
  int H = GetPrivateProfileInt(ExeBase, "H", 56, IniPath);

  CloseLog = GetPrivateProfileInt(ExeBase, "CloseLog", 1, IniPath);

  int argc;
  LPWSTR *argv;
  LPTSTR exebase;
  exebase = PathFindFileName(GetCommandLine());
  if (exebase[lstrlen(exebase)-2] == '"') {
    exebase[lstrlen(exebase)-2] = '\0';
  }
  PathRemoveExtension(exebase);

  WNDCLASS wMain;
  wMain.style = CS_HREDRAW | CS_VREDRAW;
  wMain.lpfnWndProc = WinProc;
  wMain.cbClsExtra = 0;
  wMain.cbWndExtra = 0;
  wMain.hInstance =hInstance;
  wMain.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wMain.hCursor = LoadCursor(NULL, IDC_ARROW);
  wMain.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wMain.lpszMenuName = NULL;
  wMain.lpszClassName = exebase;

  if (!RegisterClass(&wMain)) return 0;

  hwMain = CreateWindowEx(WS_EX_LEFT,
                          exebase, exebase,
                          WS_CAPTION | WS_BORDER | WS_SYSMENU |
                          WS_MINIMIZEBOX | WS_VISIBLE,
                          X, Y, W, H,
                          NULL, NULL, hInstance, NULL);
  hdcMain = GetDC(hwMain);
  ShowWindow(hwMain, SW_SHOW);
  UpdateWindow(hwMain);

  LOGFONT lfFont;
  ZeroMemory(&lfFont, sizeof(lfFont));
  lfFont.lfHeight = FontSize;
  lfFont.lfWeight = FW_NORMAL;
  lfFont.lfCharSet = ANSI_CHARSET;
  lfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
  lfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lfFont.lfQuality = DEFAULT_QUALITY;
  lfFont.lfPitchAndFamily = DEFAULT_PITCH;
  lstrcpy(lfFont.lfFaceName, FontName);

  hfFont = CreateFontIndirect(&lfFont);
  SelectObject(hdcMain, hfFont);

  EnablePrivilege(SE_DEBUG_NAME);

  WriteState("Waiting Process...  ");
  SetTimer(hwMain, 14, 1000, NULL);

  MSG msgMain;
  while (GetMessage(&msgMain, NULL, 0L, 0L)) {
    TranslateMessage(&msgMain);
    DispatchMessage(&msgMain);
  }

  return 0;
}
