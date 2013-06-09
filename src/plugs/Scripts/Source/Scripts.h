#ifndef _SCRIPTS_H_
#define _SCRIPTS_H_


//Includes
#include "StackFunc.h"
#include "StrFunc.h"
#include "WideFunc.h"

//Include AEC functions
#define AEC_FUNCTIONS
#include "AkelEdit.h"

#include "AkelDLL.h"
#include "Resources\Resource.h"

#include "Scripts_h.h"
#include "IActiveScriptSite.h"
#include "IConstants.h"
#include "IDocument.h"
#include "IScriptSettings.h"
#include "ISystemFunction.h"
#include "IWArguments.h"
#include "IWScript.h"
#include "IServer.h"

//Defines
#define DLLA_SCRIPTS_EXEC      1
#define DLLA_SCRIPTS_EXECWAIT  2
#define DLLA_SCRIPTS_EDIT      3

#define STRID_ARCHITECTURE_MISMATCH 1
#define STRID_HOTKEY_EXISTS         2
#define STRID_EXECUTE_ERROR         3
#define STRID_ENGINE_ERROR          4
#define STRID_READFILE_ERROR        5
#define STRID_SCRIPTERROR           6
#define STRID_STOP                  7
#define STRID_CONTINUE              8
#define STRID_DEBUG                 9
#define STRID_CODE                  10
#define STRID_DEBUG_MEMLOCATE       11
#define STRID_DEBUG_MEMREAD         12
#define STRID_DEBUG_MEMWRITE        13
#define STRID_DEBUG_MEMFREE         14
#define STRID_DEBUG_MEMLEAK         15
#define STRID_DEBUG_SYSCALLDLL      16
#define STRID_DEBUG_SYSCALLFUNCTION 17
#define STRID_SCRIPT                18
#define STRID_HOTKEY                19
#define STRID_STATUS                20
#define STRID_RUNNING               21
#define STRID_WAITING               22
#define STRID_EXEC                  23
#define STRID_EDIT                  24
#define STRID_ASSIGN                25
#define STRID_PLUGIN                26
#define STRID_OK                    27
#define STRID_CANCEL                28
#define STRID_CLOSE                 29

#define OF_RECT        0x1
#define OF_LASTSCRIPT  0x2
#define OF_HOTKEYS     0x4
#define OF_DEBUG       0x8

#define LVI_SCRIPT_FILE         0
#define LVI_SCRIPT_HOTKEY       1
#define LVI_SCRIPT_STATUS       2

#define BUFFER_SIZE             1024

typedef struct {
  wchar_t *wpScript;
  wchar_t *wpArguments;
  int nArgumentsLen;
  HANDLE hInitMutex;
  BOOL bWaitForScriptSignal;
} EXECSCRIPT;

typedef struct _SCRIPTARG {
  struct _SCRIPTARG *next;
  struct _SCRIPTARG *prev;
  wchar_t *wpArgument;
  int nArgumentLen;
  wchar_t *wpArgName;
  int nArgNameLen;
  wchar_t *wpArgValue;
  int nArgValueLen;
} SCRIPTARG;

typedef struct {
  SCRIPTARG *first;
  SCRIPTARG *last;
  int nElements;
} HARGSTACK;

typedef struct _SCRIPTTHREAD {
  struct _SCRIPTTHREAD *next;
  struct _SCRIPTTHREAD *prev;
  HANDLE hInitMutex;
  HANDLE hExecMutex;
  BOOL bSingleCopy;
  BOOL bWaiting;
  HANDLE hThread;
  DWORD dwThreadID;
  IActiveScript *objActiveScript;
  IActiveScriptParse *objActiveScriptParse;
  wchar_t wszScriptName[MAX_PATH];
  wchar_t wszScriptFile[MAX_PATH];
  wchar_t wszScriptInclude[MAX_PATH];
  HARGSTACK hArgStack;
  wchar_t *wszArguments;
  int nArgumentsLen;

  //IDocument
  CALLBACKSTACK hDialogCallbackStack;
  HWND hWndScriptsThreadDummy;
  BOOL bMessageLoop;
  BOOL bBusy;

  INCLUDESTACK hIncludesStack;
  POINTERSTACK hPointersStack;
  DWORD dwDebug;
  BOOL bStopped;

  HWND hWndPluginEdit;
} SCRIPTTHREAD;

typedef struct {
  SCRIPTTHREAD *first;
  SCRIPTTHREAD *last;
  int nElements;
} HTHREADSTACK;

#ifndef LVM_SETEXTENDEDLISTVIEWSTYLE
  #define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#endif
#ifndef LVS_EX_GRIDLINES
  #define LVS_EX_GRIDLINES 0x00000001
#endif
#ifndef LVS_EX_FULLROWSELECT
  #define LVS_EX_FULLROWSELECT 0x00000020
#endif

//Global variables
extern char szBuffer[BUFFER_SIZE];
extern wchar_t wszBuffer[BUFFER_SIZE];
extern wchar_t wszPluginName[MAX_PATH];
extern wchar_t wszPluginTitle[MAX_PATH];
extern HINSTANCE hInstanceEXE;
extern HINSTANCE hInstanceDLL;
extern HWND hMainWnd;
extern HACCEL hGlobalAccel;
extern BOOL bOldWindows;
extern BOOL bOldWindows;
extern BOOL bOldRichEdit;
extern BOOL bOldComctl32;
extern BOOL bAkelEdit;
extern int nMDI;
extern LANGID wLangModule;
extern HICON g_hPluginIcon;
extern BOOL g_MainMessageBox;
extern BOOL bInitCommon;
extern char szScriptsDir[MAX_PATH];
extern wchar_t wszScriptsDir[MAX_PATH];
extern char szAkelPadDir[MAX_PATH];
extern wchar_t wszAkelPadDir[MAX_PATH];
extern wchar_t wszErrorMsg[BUFFER_SIZE];
extern HTHREADSTACK hThreadStack;
extern SCRIPTTHREAD *lpScriptThreadActiveX;

//Functions prototypes
BOOL CALLBACK MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL RegisterHotkey(wchar_t *wszScriptName, WORD wHotkey);
void FillScriptList(HWND hWnd, const wchar_t *wpFilter);

LRESULT CALLBACK NewMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
BOOL CALLBACK HotkeyProc(void *lpParameter, LPARAM lParam, DWORD dwSupport);
int EditScript(wchar_t *wpScript);
void ExecScript(wchar_t *wpScript, wchar_t *wszArguments, BOOL bWaitExec);
DWORD WINAPI ExecThreadProc(LPVOID lpParameter);
SCRIPTTHREAD* StackInsertScriptThread(HTHREADSTACK *hStack);
SCRIPTTHREAD* StackGetScriptThreadCurrent();
SCRIPTTHREAD* StackGetScriptThreadByID(HTHREADSTACK *hStack, DWORD dwThreadID);
SCRIPTTHREAD* StackGetScriptThreadByName(HTHREADSTACK *hStack, const wchar_t *wpScriptName);
void StackDeleteScriptThread(HTHREADSTACK *hStack, SCRIPTTHREAD *lpScriptThread);
void StackFreeScriptThread(HTHREADSTACK *hStack);
void FreeScriptResources(SCRIPTTHREAD *lpScriptThread, BOOL bDebug);
BOOL CloseScriptWindows(SCRIPTTHREAD *lpScriptThread);
void CloseAllScriptWindows(int *nCloseOK, int *nCloseERR);
void PostQuitAllScriptWindows();
int CountAllScriptWindows();
void ShowScriptWindows(SCRIPTTHREAD *lpScriptThread, BOOL bShow);
void ShowAllScriptWindows(BOOL bShow);
int GetCommandLineArg(const wchar_t *wpCmdLine, wchar_t *wszArg, int nArgMax, const wchar_t **wpNextArg);
SCRIPTARG* StackInsertArgument(HARGSTACK *hStack);
SCRIPTARG* StackGetArgumentByIndex(HARGSTACK *hStack, int nIndex);
SCRIPTARG* StackGetArgumentByName(HARGSTACK *hStack, const wchar_t *wpArgName, int nArgNameLen);
void StackFreeArguments(HARGSTACK *hStack);
UINT_PTR GetVariantInt(VARIANT *vt);
int GetHotkeyString(WORD wHotkey, wchar_t *wszString);
INT_PTR ReadFileContent(wchar_t *wpFile, DWORD dwFlags, int nCodePage, BOOL bBOM, wchar_t **wpContent, UINT_PTR dwBytesMax);
const wchar_t* GetFileExt(const wchar_t *wpFile, int nFileLen);
const char* GetFileNameAnsi(const char *pFile, int nFileLen);
const wchar_t* GetFileNameWide(const wchar_t *wpFile, int nFileLen);
int GetBaseName(const wchar_t *wpFile, wchar_t *wszBaseName, int nBaseNameMaxLen);
DWORD ScrollCaret(HWND hWnd);
BOOL GetWindowPos(HWND hWnd, HWND hWndOwner, RECT *rc);

INT_PTR WideOption(HANDLE hOptions, const wchar_t *pOptionName, DWORD dwType, BYTE *lpData, DWORD dwData);
void ReadOptions(DWORD dwFlags);
void SaveOptions(DWORD dwFlags);
const char* GetLangStringA(LANGID wLangID, int nStringID);
const wchar_t* GetLangStringW(LANGID wLangID, int nStringID);
BOOL IsExtCallParamValid(LPARAM lParam, int nIndex);
INT_PTR GetExtCallParam(LPARAM lParam, int nIndex);
void InitCommon(PLUGINDATA *pd);
void InitMain();
void UninitMain();

#endif
