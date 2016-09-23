// LoxDQDll.cpp

#pragma warning(disable:4100)
//#pragma warning(disable:4189)
//#pragma warning(disable:4127)
#pragma warning(disable:4996)

#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <tlhelp32.h>

//#define DEBUG

#ifdef DEBUG
#include "DBG.h"
#else
#define DBG __noop
#endif

#ifndef BASENAME
#define BASENAME "LoXDQ"
#endif
const TCHAR ExeBase[] = BASENAME;
const TCHAR DllFile[] = BASENAME ".dll";
const TCHAR IniPath[] = ".\\" BASENAME ".ini";

#define CHATLOG_MAX 400
#define CHATLOG_SIZE 0x640
#define SYSLOG_MAX -1
#define SYSLOG_SIZE 0xe07b

BOOL WINAPI LoInitialize(DWORD dwProcessID);
BOOL WINAPI LoCheck();
INT WINAPI LoRead(CHAR * buffer);

INT WINAPI ReadSysLog(CHAR * buffer);
INT WINAPI ReadChatLog(CHAR * buffer);
UINT WINAPI SearchAddress(DWORD pid);
VOID WINAPI Parse(UINT_PTR address, UINT_PTR size, CHAR* buffer);
VOID WINAPI DecodeID(DWORD id, CHAR* buf);

//
HANDLE hP = NULL;

UINT_PTR BaseAddress = 0;
UINT_PTR SystemOffset = 0;
UINT_PTR ChatOffset = 0;

UINT_PTR LogBaseP = 0;

UINT_PTR SysLogBase = 0;
UINT_PTR ChatLogBase = 0;

UINT_PTR SysLogLast = 0;
UINT ChatLogLast = CHATLOG_MAX;

BOOL GettingLog = FALSE;

//

BOOL WINAPI LoInitialize(DWORD dwProcessID)
{
  //  DBG("LoInitialize dwProcessID=%x\n", dwdwProcessID);

  UINT_PTR mod_base_addr = SearchAddress(dwProcessID);
  LogBaseP = mod_base_addr + BaseAddress;

  //  DBG("LoInitialize %x\n", GettingLog);

  if ((dwProcessID == 0) || (BaseAddress == 0) || (mod_base_addr == 0)) {
    //    DBG("LoInitialize:0\n");
    GettingLog = false;
    return FALSE;
  }
  //  DBG("LoInitialize:1\n");

  if (hP != NULL) {
    CloseHandle(hP);
  }    
  //    DBG("LoInitialize:11\n");
  hP = OpenProcess(PROCESS_VM_READ, TRUE, dwProcessID);
  if (hP == NULL) {
    //      DBG("LoInitialize:12\n");
    GettingLog = FALSE;
    return FALSE;
  }

  // LoCheck()
  UINT_PTR log_base = 0;
  //    DBG("LoInitialize:2\n");
  if (!ReadProcessMemory(hP, (LPCVOID)LogBaseP,
                         (LPVOID)&log_base, sizeof(UINT_PTR), 0)) {
    //    DBG("LoInitialize:21\n");
    GettingLog = FALSE;
    return FALSE;
  }

  //  DBG("LoInitialize:3\n");

  if (log_base == 0) {
    //    DBG("LoInitialize:4\n");
    GettingLog = FALSE;
    return FALSE;
  }

  GettingLog = TRUE;

  //    DBG("LoInitialize:5\n");
  SysLogBase = log_base + SystemOffset;
  ChatLogBase = log_base + ChatOffset;

  DBG("InitilaizeX SysLogBase=%x\n", SysLogBase);
  DBG("InitilaizeX ChatLogBase=%x\n", ChatLogBase);

  SysLogLast = 0;
  ChatLogLast = CHATLOG_MAX;

  return TRUE;
}

//

BOOL WINAPI LoCheck()
{
  //  DBG("LoCheck: %x %x %x\n", GettingLog, DQXProcess, LogBaseP);

  UINT_PTR log_base = 0;
  if (!ReadProcessMemory(hP, (LPCVOID)LogBaseP,
                         (LPVOID)&log_base, sizeof(UINT_PTR), 0)) {
    GettingLog = FALSE;
    //    DBG("LoCheck:1 %x\n", GettingLog);
    return FALSE;
  }
  if (log_base == 0) {
    GettingLog = FALSE;
    //    DBG("LoCheck:2 %x\n", GettingLog);
    return FALSE;
  }

  //  DBG("LoCheck:4 %x\n", GettingLog);
  GettingLog = TRUE;
  return TRUE;
}

//

INT WINAPI LoRead(CHAR* buffer)
{
  if (!GettingLog) {
    return 0;
  }
  if (buffer == NULL) {
    return 0;
  }
  *buffer = '\0';

  INT length;
  length = ReadSysLog(buffer);
  length += ReadChatLog(buffer);
  return length;
}

//////////////////////////////////////////////////////////////////////

UINT_PTR WINAPI SearchAddress(DWORD pid) {
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

  MODULEENTRY32 me32;
  me32.dwSize = sizeof(MODULEENTRY32);
  BOOL bModuleResult = Module32First(hSnapshot, &me32);
  if (!bModuleResult) {
    return NULL;
  }
  UINT_PTR base = (UINT_PTR)me32.modBaseAddr;
  CloseHandle(hSnapshot);
  return base;
}

//

VOID WINAPI Parse(UINT_PTR address, UINT_PTR size, CHAR* buffer)
{
  DBG("Parse: %x %x\n", address, size);

  static BYTE mem[SYSLOG_SIZE+1];

  UINT_PTR current = 0;
  UINT_PTR length = 0;

  UINT number = 0;
  UINT style = 0;
  CHAR chat[1024];

  time_t now = time(NULL);
  struct tm t;
  localtime_s(&t, &now);

  if (!ReadProcessMemory(hP, (LPCVOID)address, (LPVOID)mem, size, 0)) {
    return;
  }

  while (current < size) {
    number = *((UINT *)(&mem[current]));
    if (number == 0) {
      break;
    }
    if ((INT)number < 0) {
      *buffer = '\0';
      return;
    }
    style = mem[current + 4];
    length = mem[current + 5];
    if (length == 0) {
      break;
    }

    UINT_PTR id[4];
    UINT idn = 0;
    UINT_PTR j = 0;
    for (UINT_PTR i = current + 6; i < current + length; i++) {
      if (mem[i] == '\\') {
        if (mem[i + 1] == 's') {
          i++;
          continue;
        }
        if (mem[i + 1] == 'm') {
          id[idn] = i + 2;
          mem[i + 11] = '\0';
          i += 12;
          idn++;
        }
      } else {
        chat[j] = mem[i];
        if (mem[i] == '\0') {
          break;
        }
        j++;
      }
    }

    CHAR line[1024];
    sprintf_s(line, _countof(line),
              "%02d-%02d-%02d %02d:%02d:%02d\t%d\t0x%08x\t%s\t%s\t%s\t%s\t%s%s\n",
              t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
              t.tm_hour, t.tm_min, t.tm_sec,
              number, style,
              (idn > 0) ? (CHAR *)&mem[id[0]] : "",
              (idn > 1) ? (CHAR *)&mem[id[1]] : "",
              "", "",
              chat, "");
    strcat(buffer, line);

    current = current + length;
  }

  return;
}

//

INT WINAPI ReadSysLog(CHAR* buffer)
{
  UINT_PTR next = 0;

  if (!ReadProcessMemory(hP, (LPCVOID)(SysLogBase - 4),
                         (LPVOID)&next, 4, 0)) {
    return 0;
  }

  DBG("\n$ next=%d SysLogLast=%d\n", next, SysLogLast);

  if (SysLogLast == next) {
    return 0;
  }

  UINT_PTR start;
  UINT_PTR size;
  BOOL loop = FALSE;

  start = SysLogBase + SysLogLast;
  if (SysLogLast > next) {
    size = SYSLOG_SIZE - SysLogLast;
    loop = TRUE;
  } else {
    size = next - SysLogLast;
  }
  Parse(start, size, buffer);

  if (loop) {
    start = SysLogBase;
    size = next;
    Parse(start, size, buffer);
  }

  SysLogLast = next;

  return strlen(buffer);
}

//

INT WINAPI ReadChatLog(CHAR* buffer)
{
  static UINT_PTR address[CHATLOG_MAX];
  UINT number = 0;

  INT start = 0;
  INT end = 0;

  UINT_PTR last = 0;

  DBG("\n! ChatLogLast=%d\n", ChatLogLast);

  if (!ReadProcessMemory(hP, (LPCVOID)(ChatLogBase + CHATLOG_SIZE),
                         (LPVOID)&number, 4, 0)) {
    return 0;
  }

  DBG("! number=%x\n", number);

  if (number == 0) {
    return 0;
  }

  if ((number != CHATLOG_MAX) && (number == ChatLogLast + 1)) {
    return 0;
  }

  if (ChatLogLast != CHATLOG_MAX) {
    last = address[ChatLogLast];
  }

  DBG("! last=%x\n", last);

  if (!ReadProcessMemory(hP, (LPCVOID)ChatLogBase,
                         (LPVOID)&address, CHATLOG_SIZE, 0)) {
    return 0;
  }

  if ((number == CHATLOG_MAX) && (last == address[CHATLOG_MAX - 1])) {
    return 0;
  }

  for (UINT i = 0; i < number; i++) {
    if (last == address[i]) {
      start = i + 1;
    }
  }

  end = number - 1;

  DBG("! start=%d\n", start);
  DBG("! end=%d\n", end);

  time_t now = time(NULL);
  struct tm t;
  localtime_s(&t, &now);

  while (start <= end) {
    INT_PTR tmpAddress = address[start];

    struct s_chat {
      DWORD FromID;
      DWORD ToID;
      DWORD Number; 
      DWORD Unknown1;
      DWORD Style;
      UCHAR From[19];
      UCHAR To[19];
      UCHAR Line1[61];
      UCHAR Line2[61];
    };
    struct s_chat c;

    if (!ReadProcessMemory(hP, (LPCVOID)tmpAddress,
                           (LPVOID)&c, sizeof(s_chat), 0)) {
      return 0;
    }

    CHAR from_id[10];
    CHAR to_id[10];
    DecodeID(c.FromID, (CHAR *)&from_id);
    DecodeID(c.ToID, (CHAR *)&to_id);

    CHAR line[256];
    sprintf_s(line, _countof(line),
              "%02d-%02d-%02d %02d:%02d:%02d\t%d\t0x%08x\t%s\t%s\t%s\t%s\t%s%s\n",
              t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
              t.tm_hour, t.tm_min, t.tm_sec,
              c.Number, c.Style,
              from_id, to_id,
              c.From, c.To,
              c.Line1, c.Line2);
    strcat(buffer, line);

    start = start + 1;
  }

  ChatLogLast = number - 1;

  return strlen(buffer);
}

//

VOID WINAPI DecodeID(DWORD id, CHAR* buf)
{
  if (id == 0) {
    buf[0] = '\0';
  } else {
    UINT a = id % 1000;
    UINT b = ((id - a) % 1000000) / 1000;
    UINT c = id / 1000000;
    sprintf_s(buf, 10, "%c%c%03d-%03d\0",
              (c / 26) + 'A', (c % 26) + 'A', b, a);
  }
  return;
}

//

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
  if (dwReason == DLL_PROCESS_ATTACH) {
    BaseAddress = GetPrivateProfileInt(ExeBase, "BaseAddress", 0, IniPath);
    SystemOffset = GetPrivateProfileInt(ExeBase, "SystemOffset", 0, IniPath);
    ChatOffset = GetPrivateProfileInt(ExeBase, "ChatOffset", 0, IniPath);
    DBG("DLL START %x %x %x\n", BaseAddress, SystemOffset, ChatOffset);
  } else if (dwReason == DLL_PROCESS_DETACH) {
    DBG("DLL END\n");
  }
  return TRUE;
}
