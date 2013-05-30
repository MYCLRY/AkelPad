#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "StrFunc.h"
#include "WideFunc.h"
#include "AkelEdit.h"
#include "AkelDLL.h"
#include "Resources\Resource.h"


//Include string functions
#define xmemcpy
#define xmemset
#define xstrlenW
#define xstrcpynW
#define xatoiW
#define xitoaW
#define xuitoaW
#define dec2hexW
#define xprintfW
#include "StrFunc.h"

//Include wide functions
#define DialogBoxWide
#define GetWindowLongPtrWide
#define SetDlgItemTextWide
#define SetWindowTextWide
#include "WideFunc.h"

//Defines
#define DLLA_SCROLL_HSCROLL      1
#define DLLA_SCROLL_VSCROLL      2
#define DLLA_SCROLL_LINEHSCROLL  3
#define DLLA_SCROLL_LINEVSCROLL  4
#define DLLA_SCROLL_ENTERSCROLL  5

#define STRID_AUTOSTEP          1
#define STRID_ENABLE            2
#define STRID_STEPTIME          3
#define STRID_STEPWIDTH         4
#define STRID_HSYNCHRONIZATION  5
#define STRID_VSYNCHRONIZATION  6
#define STRID_FRAMEMDI          7
#define STRID_SPLITPANE         8
#define STRID_NOSCROLL          9
#define STRID_UNDO              10
#define STRID_REDO              11
#define STRID_SELECTALL         12
#define STRID_AUTOFOCUS         13
#define STRID_FOCUSBACKGROUND   14
#define STRID_MOVESCROLLBAR     15
#define STRID_MOVEWITHSHIFT     16
#define STRID_SWITCHTAB         17
#define STRID_WITHSPIN          18
#define STRID_INVERT            19
#define STRID_PLUGIN            20
#define STRID_OK                21
#define STRID_CANCEL            22

//Move caret
#define MC_CARETNOMOVE      0
#define MC_CARETMOVEINSIDE  1
#define MC_CARETMOVEOUTSIDE 2

//No scroll
#define NS_UNDO      0x1
#define NS_REDO      0x2
#define NS_SELECTALL 0x4

//Synchronization
#define SNC_MDI         0x00000001  //Synchronize scroll of MDI windows.
#define SNC_CLONE       0x00000002  //Synchronize scroll of split windows.

//Auto focus
#define AF_FOCUSBACKGROUND     0x00000001
#define AF_MOVESCROLLBAR       0x00000010
#define AF_MOVESCROLLBARINVERT 0x00000020
#define AF_MOVEWITHSHIFT       0x00000100
#define AF_MOVEWITHSHIFTINVERT 0x00000200
#define AF_SWITCHTAB           0x00001000
#define AF_SWITCHTABINVERT     0x00002000
#define AF_SWITCHTABWITHSPIN   0x00004000

#ifndef WM_MOUSEHWHEEL
  #define WM_MOUSEHWHEEL  0x020E
#endif
#ifndef SPI_GETWHEELSCROLLCHARS
  #define SPI_GETWHEELSCROLLCHARS  0x006C
#endif

//Hook information
#define HT_MAX  10

typedef struct {
  HHOOK hHook;
  DWORD dwThreadId;
  LRESULT (CALLBACK *GetMsgProc)(int, WPARAM, LPARAM);
} HOOKTHREAD;

//Functions prototypes
LRESULT CALLBACK NewMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditParentMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SetupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetScrollTimer();
void KillScrollTimer();
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void GetMsgProcCommon(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc1(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc2(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc3(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc4(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc5(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc6(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc7(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc8(int code, WPARAM wParam, LPARAM lParam);
HWND GetCurEdit();
AEHDOC GetCurDoc();

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
void InitAutoScroll();
void UninitAutoScroll();
void InitSyncHorz();
void UninitSyncHorz();
void InitSyncVert();
void UninitSyncVert();
void InitNoScroll();
void UninitNoScroll();
void InitAutoFocus();
void UninitAutoFocus();
WNDPROCDATA *NewMainProcData=NULL;
WNDPROCDATA *NewFrameProcData=NULL;

//Global variables
char szClassName[MAX_PATH];
wchar_t wszPluginName[MAX_PATH];
wchar_t wszPluginTitle[MAX_PATH];
HINSTANCE hInstanceDLL;
HWND hMainWnd;
HWND hMdiClient;
HICON hMainIcon;
int nMDI;
BOOL bAkelEdit;
LANGID wLangModule;
BOOL bInitCommon=FALSE;
int nInitMain=0;
BOOL bInitAutoScroll=FALSE;
BOOL bInitSyncHorz=FALSE;
BOOL bInitSyncVert=FALSE;
BOOL bInitNoScroll=FALSE;
BOOL bInitAutoFocus=FALSE;
HWND hWndAutoScroll=NULL;
AEHDOC hDocAutoScroll=NULL;
UINT_PTR dwAutoScrollTimer=0;
int nAutoScrollStepTime=50;
int nAutoScrollStepWidth=1;
DWORD dwSyncHorz=SNC_MDI|SNC_CLONE;
DWORD dwSyncVert=SNC_MDI|SNC_CLONE;
DWORD dwNoScroll=NS_UNDO|NS_REDO|NS_SELECTALL;
DWORD dwAutoFocus=AF_FOCUSBACKGROUND|AF_MOVESCROLLBAR|AF_MOVEWITHSHIFT|AF_SWITCHTAB;
HOOKTHREAD ht[HT_MAX];

//Identification
void __declspec(dllexport) DllAkelPadID(PLUGINVERSION *pv)
{
  pv->dwAkelDllVersion=AKELDLL;
  pv->dwExeMinVersion3x=MAKE_IDENTIFIER(-1, -1, -1, -1);
  pv->dwExeMinVersion4x=MAKE_IDENTIFIER(4, 7, 7, 0);
  pv->pPluginName="Scroll";
}

//Plugin extern function
void __declspec(dllexport) AutoScroll(PLUGINDATA *pd)
{
  pd->dwSupport|=PDS_SUPPORTALL;
  if (pd->dwSupport & PDS_GETSUPPORT)
    return;

  if (!bInitCommon) InitCommon(pd);

  if (bInitAutoScroll)
  {
    UninitMain();
    UninitAutoScroll();

    //If any function still loaded, stay in memory and show as non-active
    if (nInitMain) pd->nUnload=UD_NONUNLOAD_NONACTIVE;
  }
  else
  {
    InitMain();
    InitAutoScroll();

    //Stay in memory, and show as active
    pd->nUnload=UD_NONUNLOAD_ACTIVE;
  }
}

void __declspec(dllexport) SyncHorz(PLUGINDATA *pd)
{
  pd->dwSupport|=PDS_SUPPORTALL;
  if (pd->dwSupport & PDS_GETSUPPORT)
    return;

  if (!bInitCommon) InitCommon(pd);

  if (bInitSyncHorz)
  {
    UninitMain();
    UninitSyncHorz();

    //If any function still loaded, stay in memory and show as non-active
    if (nInitMain) pd->nUnload=UD_NONUNLOAD_NONACTIVE;
  }
  else
  {
    InitMain();
    InitSyncHorz();

    //Stay in memory, and show as active
    pd->nUnload=UD_NONUNLOAD_ACTIVE;
  }
}

void __declspec(dllexport) SyncVert(PLUGINDATA *pd)
{
  pd->dwSupport|=PDS_SUPPORTALL;
  if (pd->dwSupport & PDS_GETSUPPORT)
    return;

  if (!bInitCommon) InitCommon(pd);

  if (bInitSyncVert)
  {
    UninitMain();
    UninitSyncVert();

    //If any function still loaded, stay in memory and show as non-active
    if (nInitMain) pd->nUnload=UD_NONUNLOAD_NONACTIVE;
  }
  else
  {
    InitMain();
    InitSyncVert();

    //Stay in memory, and show as active
    pd->nUnload=UD_NONUNLOAD_ACTIVE;
  }
}

void __declspec(dllexport) NoScroll(PLUGINDATA *pd)
{
  pd->dwSupport|=PDS_SUPPORTALL;
  if (pd->dwSupport & PDS_GETSUPPORT)
    return;

  if (!bInitCommon) InitCommon(pd);

  if (bInitNoScroll)
  {
    UninitMain();
    UninitNoScroll();

    //If any function still loaded, stay in memory and show as non-active
    if (nInitMain) pd->nUnload=UD_NONUNLOAD_NONACTIVE;
  }
  else
  {
    InitMain();
    InitNoScroll();

    //Stay in memory, and show as active
    pd->nUnload=UD_NONUNLOAD_ACTIVE;
  }
}

void __declspec(dllexport) AutoFocus(PLUGINDATA *pd)
{
  pd->dwSupport|=PDS_SUPPORTALL;
  if (pd->dwSupport & PDS_GETSUPPORT)
    return;

  if (!bInitCommon) InitCommon(pd);

  if (bInitAutoFocus)
  {
    UninitMain();
    UninitAutoFocus();

    //If any function still loaded, stay in memory and show as non-active
    if (nInitMain) pd->nUnload=UD_NONUNLOAD_NONACTIVE;
  }
  else
  {
    InitMain();
    InitAutoFocus();

    //Stay in memory, and show as active
    pd->nUnload=UD_NONUNLOAD_ACTIVE;
  }
}

void __declspec(dllexport) Settings(PLUGINDATA *pd)
{
  pd->dwSupport|=PDS_NOAUTOLOAD;
  if (pd->dwSupport & PDS_GETSUPPORT)
    return;

  if (!bInitCommon) InitCommon(pd);

  if (pd->lParam)
  {
    INT_PTR nAction=GetExtCallParam(pd->lParam, 1);

    if (pd->bAkelEdit)
    {
      if (nAction == DLLA_SCROLL_HSCROLL)
      {
        int nValue=-1;

        if (IsExtCallParamValid(pd->lParam, 2))
          nValue=(int)GetExtCallParam(pd->lParam, 2);

        if (nValue >= 0)
        {
          SendMessage(pd->hWndEdit, AEM_SCROLL, AESB_HORZ, nValue);
        }
      }
      else if (nAction == DLLA_SCROLL_VSCROLL)
      {
        int nValue=-1;

        if (IsExtCallParamValid(pd->lParam, 2))
          nValue=(int)GetExtCallParam(pd->lParam, 2);

        if (nValue >= 0)
        {
          SendMessage(pd->hWndEdit, AEM_SCROLL, AESB_VERT, nValue);
        }
      }
      else if (nAction == DLLA_SCROLL_LINEHSCROLL)
      {
        AESELECTION aes;
        AECHARINDEX ciCaret;
        AESCROLLTOPOINT stp;
        DWORD dwScrollResult;
        int nValue1=0;
        int nValue2=0;

        if (IsExtCallParamValid(pd->lParam, 2))
          nValue1=(int)GetExtCallParam(pd->lParam, 2);
        if (IsExtCallParamValid(pd->lParam, 3))
          nValue2=(int)GetExtCallParam(pd->lParam, 3);

        if (nValue1)
        {
          if (nValue2 == MC_CARETNOMOVE)
          {
            SendMessage(pd->hWndEdit, AEM_LINESCROLL, AESB_HORZ, nValue1);
          }
          else if (nValue2 == MC_CARETMOVEINSIDE)
          {
            //Test scroll to caret
            stp.dwFlags=AESC_TEST|AESC_POINTCARET|AESC_OFFSETCHARX|AESC_OFFSETCHARY;
            stp.nOffsetX=0;
            stp.nOffsetY=0;
            dwScrollResult=(DWORD)SendMessage(pd->hWndEdit, AEM_SCROLLTOPOINT, 0, (LPARAM)&stp);

            if (SendMessage(pd->hWndEdit, AEM_LINESCROLL, AESB_HORZ, nValue1))
            {
              if (!(dwScrollResult & AECSE_SCROLLEDX) && !(dwScrollResult & AECSE_SCROLLEDY))
              {
                SendMessage(pd->hWndEdit, AEM_GETINDEX, AEGI_CARETCHAR, (LPARAM)&ciCaret);
                ciCaret.nCharInLine=max(ciCaret.nCharInLine + nValue1, 0);

                //Set new caret position
                aes.crSel.ciMin=ciCaret;
                aes.crSel.ciMax=ciCaret;
                aes.dwFlags=AESELT_LOCKSCROLL;
                SendMessage(pd->hWndEdit, AEM_SETSEL, (WPARAM)NULL, (LPARAM)&aes);
              }
            }
          }
          else if (nValue2 == MC_CARETMOVEOUTSIDE)
          {
            SendMessage(pd->hWndEdit, AEM_LINESCROLL, AESB_HORZ, nValue1);

            //Test scroll to caret
            stp.dwFlags=AESC_TEST|AESC_POINTCARET|AESC_OFFSETCHARX|AESC_OFFSETCHARY;
            stp.nOffsetX=0;
            stp.nOffsetY=0;
            dwScrollResult=(DWORD)SendMessage(pd->hWndEdit, AEM_SCROLLTOPOINT, 0, (LPARAM)&stp);

            if (dwScrollResult & AECSE_SCROLLEDX)
            {
              SendMessage(pd->hWndEdit, AEM_GETINDEX, AEGI_CARETCHAR, (LPARAM)&ciCaret);
              ciCaret.nCharInLine=max(ciCaret.nCharInLine + nValue1, 0);

              //Set new caret position
              aes.crSel.ciMin=ciCaret;
              aes.crSel.ciMax=ciCaret;
              aes.dwFlags=AESELT_LOCKSCROLL;
              SendMessage(pd->hWndEdit, AEM_SETSEL, (WPARAM)NULL, (LPARAM)&aes);

              //Scroll to caret
              stp.dwFlags=AESC_POINTCARET|AESC_OFFSETCHARX|AESC_OFFSETCHARY;
              stp.nOffsetX=0;
              stp.nOffsetY=0;
              SendMessage(pd->hWndEdit, AEM_SCROLLTOPOINT, 0, (LPARAM)&stp);
            }
            else if (dwScrollResult & AECSE_SCROLLEDY)
            {
              //Scroll to caret
              stp.dwFlags=AESC_POINTCARET|AESC_OFFSETRECTDIVY;
              stp.nOffsetX=0;
              stp.nOffsetY=2;
              SendMessage(pd->hWndEdit, AEM_SCROLLTOPOINT, 0, (LPARAM)&stp);
            }
          }
        }
      }
      else if (nAction == DLLA_SCROLL_LINEVSCROLL)
      {
        AESELECTION aes;
        AECHARINDEX ciCaret;
        AESCROLLTOPOINT stp;
        POINT64 ptGlobal;
        DWORD dwAlign=0;
        DWORD dwScrollResult;
        int nValue1=0;
        int nValue2=0;

        if (IsExtCallParamValid(pd->lParam, 2))
          nValue1=(int)GetExtCallParam(pd->lParam, 2);
        if (IsExtCallParamValid(pd->lParam, 3))
          nValue2=(int)GetExtCallParam(pd->lParam, 3);

        if (nValue1)
        {
          if (nValue1 > 0)
            dwAlign=AESB_ALIGNTOP;
          else if (nValue1 < 0)
            dwAlign=AESB_ALIGNBOTTOM;

          if (nValue2 == MC_CARETNOMOVE)
          {
            SendMessage(pd->hWndEdit, AEM_LINESCROLL, AESB_VERT|dwAlign, nValue1);
          }
          else if (nValue2 == MC_CARETMOVEINSIDE)
          {
            //Test scroll to caret
            stp.dwFlags=AESC_TEST|AESC_POINTCARET|AESC_OFFSETCHARX|AESC_OFFSETCHARY;
            stp.nOffsetX=0;
            stp.nOffsetY=0;
            dwScrollResult=(DWORD)SendMessage(pd->hWndEdit, AEM_SCROLLTOPOINT, 0, (LPARAM)&stp);

            if (SendMessage(pd->hWndEdit, AEM_LINESCROLL, AESB_VERT|dwAlign, nValue1))
            {
              if (!(dwScrollResult & AECSE_SCROLLEDY))
              {
                SendMessage(pd->hWndEdit, AEM_GETCARETPOS, 0, (WPARAM)&ptGlobal);
                ptGlobal.x=SendMessage(pd->hWndEdit, AEM_GETCARETHORZINDENT, 0, 0);
                ptGlobal.y+=nValue1 * SendMessage(pd->hWndEdit, AEM_GETCHARSIZE, AECS_HEIGHT, 0);
                SendMessage(pd->hWndEdit, AEM_CHARFROMGLOBALPOS, (WPARAM)&ptGlobal, (LPARAM)&ciCaret);

                //Set new caret position
                aes.crSel.ciMin=ciCaret;
                aes.crSel.ciMax=ciCaret;
                aes.dwFlags=AESELT_LOCKSCROLL|AESELT_NOCARETHORZINDENT;
                SendMessage(pd->hWndEdit, AEM_SETSEL, (WPARAM)NULL, (LPARAM)&aes);
              }
            }
          }
          else if (nValue2 == MC_CARETMOVEOUTSIDE)
          {
            SendMessage(pd->hWndEdit, AEM_LINESCROLL, AESB_VERT|dwAlign, nValue1);

            //Test scroll to caret
            stp.dwFlags=AESC_TEST|AESC_POINTCARET|AESC_OFFSETCHARX|AESC_OFFSETCHARY;
            stp.nOffsetX=0;
            stp.nOffsetY=0;
            dwScrollResult=(DWORD)SendMessage(pd->hWndEdit, AEM_SCROLLTOPOINT, 0, (LPARAM)&stp);

            if (dwScrollResult & AECSE_SCROLLEDY)
            {
              SendMessage(pd->hWndEdit, AEM_GETCARETPOS, 0, (WPARAM)&ptGlobal);
              ptGlobal.x=SendMessage(pd->hWndEdit, AEM_GETCARETHORZINDENT, 0, 0);
              ptGlobal.y+=nValue1 * SendMessage(pd->hWndEdit, AEM_GETCHARSIZE, AECS_HEIGHT, 0);
              SendMessage(pd->hWndEdit, AEM_CHARFROMGLOBALPOS, (WPARAM)&ptGlobal, (LPARAM)&ciCaret);

              //Set new caret position
              aes.crSel.ciMin=ciCaret;
              aes.crSel.ciMax=ciCaret;
              aes.dwFlags=AESELT_NOCARETHORZINDENT;
              SendMessage(pd->hWndEdit, AEM_SETSEL, (WPARAM)NULL, (LPARAM)&aes);
            }
            else if (dwScrollResult & AECSE_SCROLLEDX)
            {
              //Scroll to caret
              stp.dwFlags=AESC_POINTCARET|AESC_OFFSETRECTDIVX;
              stp.nOffsetX=3;
              stp.nOffsetY=0;
              SendMessage(pd->hWndEdit, AEM_SCROLLTOPOINT, 0, (LPARAM)&stp);
            }
          }
        }
      }
      else if (nAction == DLLA_SCROLL_ENTERSCROLL)
      {
        if (pd->hWndEdit != GetFocus())
        {
          pd->nUnload=UD_HOTKEY_DODEFAULT;
          if (pd->bInMemory) pd->nUnload|=UD_NONUNLOAD_NONACTIVE;
          return;
        }
        SendMessage(pd->hWndEdit, AEM_LOCKUPDATE, AELU_CARET, TRUE);
        SendMessage(pd->hWndEdit, WM_CHAR, VK_RETURN, 0);
        SendMessage(pd->hWndEdit, AEM_SCROLL, AESB_VERT, SB_LINEDOWN);
        SendMessage(pd->hWndEdit, AEM_LOCKUPDATE, AELU_CARET, FALSE);
        SendMessage(pd->hWndEdit, AEM_UPDATECARET, 0, 0);
      }
    }
  }
  else
  {
    DialogBoxWide(hInstanceDLL, MAKEINTRESOURCEW(IDD_SETUP), hMainWnd, (DLGPROC)SetupDlgProc);
  }

  //If plugin already loaded, stay in memory, but show as non-active
  if (pd->bInMemory) pd->nUnload=UD_NONUNLOAD_NONACTIVE;
}

LRESULT CALLBACK NewMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LRESULT lResult;

  if (uMsg == AKDN_EDIT_ONFINISH ||
      uMsg == AKDN_EDIT_ONCLOSE)
  {
    if (bInitAutoScroll)
    {
      if (hDocAutoScroll == (AEHDOC)lParam)
      {
        KillScrollTimer();
        hWndAutoScroll=NULL;
        hDocAutoScroll=NULL;
      }
    }
  }
  else if (uMsg == WM_COMMAND)
  {
    if (bInitNoScroll)
    {
      if (LOWORD(wParam) == IDM_EDIT_SELECTALL && (dwNoScroll & NS_SELECTALL))
      {
        HWND hWndEdit;
        LRESULT lResult;
        int nLockScroll;

        if (bAkelEdit)
        {
          if (hWndEdit=GetCurEdit())
          {
            if ((nLockScroll=(int)SendMessage(hWndEdit, AEM_LOCKSCROLL, (WPARAM)-1, 0)) == -1)
              SendMessage(hWndEdit, AEM_LOCKSCROLL, SB_BOTH, TRUE);
            lResult=NewMainProcData->NextProc(hWnd, uMsg, wParam, lParam);
            if (nLockScroll == -1)
              SendMessage(hWndEdit, AEM_LOCKSCROLL, SB_BOTH, FALSE);
            return lResult;
          }
        }
      }
    }
  }
  if (lResult=EditParentMessages(hWnd, uMsg, wParam, lParam))
    return lResult;

  //Call next procedure
  return NewMainProcData->NextProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK NewFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LRESULT lResult;

  if (uMsg == WM_NOTIFY)
  {
    if (wParam == ID_EDIT)
    {
      if ((bInitSyncHorz && ((NMHDR *)lParam)->code == AEN_HSCROLL && (dwSyncHorz & SNC_MDI)) ||
          (bInitSyncVert && ((NMHDR *)lParam)->code == AEN_VSCROLL && (dwSyncVert & SNC_MDI)))
      {
        AENSCROLL *aens=(AENSCROLL *)lParam;
        HWND hWndChildFrame;
        HWND hWndChildEdit;
        POINT64 ptGlobal;
        static BOOL bLock=FALSE;
        BOOL bMdiMaximize;

        if (nMDI == WMD_MDI && SendMessage(hMdiClient, WM_MDIGETACTIVE, 0, (LPARAM)&bMdiMaximize) && !bMdiMaximize)
        {
          if (!bLock)
          {
            bLock=TRUE;
            hWndChildFrame=GetWindow(hMdiClient, GW_CHILD);

            while (hWndChildFrame)
            {
              if (hWndChildFrame != hWnd)
              {
                hWndChildEdit=GetDlgItem(hWndChildFrame, ID_EDIT);

                SendMessage(hWndChildEdit, AEM_GETSCROLLPOS, 0, (LPARAM)&ptGlobal);
                if (((NMHDR *)lParam)->code == AEN_HSCROLL)
                  ptGlobal.x+=aens->nPosNew - aens->nPosOld;
                else
                  ptGlobal.y+=aens->nPosNew - aens->nPosOld;
                SendMessage(hWndChildEdit, AEM_SETSCROLLPOS, 0, (LPARAM)&ptGlobal);
              }
              hWndChildFrame=GetNextWindow(hWndChildFrame, GW_HWNDNEXT);
            }
            bLock=FALSE;
          }
        }
      }
    }
  }
  if (lResult=EditParentMessages(hWnd, uMsg, wParam, lParam))
    return lResult;

  //Call next procedure
  return NewFrameProcData->NextProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK EditParentMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_NOTIFY)
  {
    if (wParam == ID_EDIT)
    {
      if ((bInitSyncHorz && ((NMHDR *)lParam)->code == AEN_HSCROLL && (dwSyncHorz & SNC_CLONE)) ||
          (bInitSyncVert && ((NMHDR *)lParam)->code == AEN_VSCROLL && (dwSyncVert & SNC_CLONE)))
      {
        AENSCROLL *aens=(AENSCROLL *)lParam;
        HWND hWndMaster;
        HWND hWndChildEdit;
        POINT64 ptGlobal;
        int nClone=0;
        static BOOL bLock=FALSE;

        if (!bLock)
        {
          bLock=TRUE;

          if (hWndMaster=(HWND)SendMessage(aens->hdr.hwndFrom, AEM_GETMASTER, 0, 0))
          {
            hWndChildEdit=hWndMaster;

            while (hWndChildEdit)
            {
              if (hWndChildEdit != aens->hdr.hwndFrom)
              {
                SendMessage(hWndChildEdit, AEM_GETSCROLLPOS, 0, (LPARAM)&ptGlobal);
                if (((NMHDR *)lParam)->code == AEN_HSCROLL)
                  ptGlobal.x+=aens->nPosNew - aens->nPosOld;
                else
                  ptGlobal.y+=aens->nPosNew - aens->nPosOld;
                SendMessage(hWndChildEdit, AEM_SETSCROLLPOS, 0, (LPARAM)&ptGlobal);
              }
              hWndChildEdit=(HWND)SendMessage(hWndMaster, AEM_GETCLONE, nClone++, 0);
            }
          }
          bLock=FALSE;
        }
      }
      else if (((NMHDR *)lParam)->code == AEN_TEXTCHANGING)
      {
        AENTEXTCHANGE *aentc=(AENTEXTCHANGE *)lParam;
        DWORD dwOptions;

        if ((bInitNoScroll && (aentc->dwType & AETCT_UNDO) && (dwNoScroll & NS_UNDO)) ||
            (bInitNoScroll && (aentc->dwType & AETCT_REDO) && (dwNoScroll & NS_REDO)))
        {
          dwOptions=(DWORD)SendMessage(aentc->hdr.hwndFrom, AEM_GETOPTIONS, 0, 0);
          if (!(dwOptions & AECO_NOSCROLLDELETEALL))
            SendMessage(aentc->hdr.hwndFrom, AEM_SETOPTIONS, AECOOP_OR, AECO_NOSCROLLDELETEALL);
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK SetupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static HWND hWndAutoScrollTitle;
  static HWND hWndStepTime;
  static HWND hWndStepWidth;
  static HWND hWndNoScrollTitle;
  static HWND hWndUndo;
  static HWND hWndRedo;
  static HWND hWndSelectAll;
  static HWND hWndSyncHorzTitle;
  static HWND hWndSyncHorzMDI;
  static HWND hWndSyncHorzClone;
  static HWND hWndSyncVertTitle;
  static HWND hWndSyncVertMDI;
  static HWND hWndSyncVertClone;
  static HWND hWndAutoFocusTitle;
  static HWND hWndFocusBackground;
  static HWND hWndMoveScrollbar;
  static HWND hWndMoveScrollbarInvert;
  static HWND hWndMoveWithShift;
  static HWND hWndMoveWithShiftInvert;
  static HWND hWndSwitchTab;
  static HWND hWndSwitchTabInvert;
  static HWND hWndSwitchTabWithSpin;
  BOOL bState;

  if (uMsg == WM_INITDIALOG)
  {
    SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hMainIcon);
    hWndAutoScrollTitle=GetDlgItem(hDlg, IDC_AUTOSCROLL_TITLE);
    hWndStepTime=GetDlgItem(hDlg, IDC_AUTOSCROLL_STEPTIME);
    hWndStepWidth=GetDlgItem(hDlg, IDC_AUTOSCROLL_STEPWIDTH);
    hWndNoScrollTitle=GetDlgItem(hDlg, IDC_NOSCROLL_TITLE);
    hWndUndo=GetDlgItem(hDlg, IDC_NOSCROLL_UNDO);
    hWndRedo=GetDlgItem(hDlg, IDC_NOSCROLL_REDO);
    hWndSelectAll=GetDlgItem(hDlg, IDC_NOSCROLL_SELECTALL);
    hWndSyncHorzTitle=GetDlgItem(hDlg, IDC_SYNCHORZ_TITLE);
    hWndSyncHorzMDI=GetDlgItem(hDlg, IDC_SYNCHORZ_MDI);
    hWndSyncHorzClone=GetDlgItem(hDlg, IDC_SYNCHORZ_CLONE);
    hWndSyncVertTitle=GetDlgItem(hDlg, IDC_SYNCVERT_TITLE);
    hWndSyncVertMDI=GetDlgItem(hDlg, IDC_SYNCVERT_MDI);
    hWndSyncVertClone=GetDlgItem(hDlg, IDC_SYNCVERT_CLONE);
    hWndAutoFocusTitle=GetDlgItem(hDlg, IDC_AUTOFOCUS_TITLE);
    hWndFocusBackground=GetDlgItem(hDlg, IDC_AUTOFOCUS_FOCUSBACKGROUND);
    hWndMoveScrollbar=GetDlgItem(hDlg, IDC_AUTOFOCUS_MOVESCROLLBAR);
    hWndMoveScrollbarInvert=GetDlgItem(hDlg, IDC_AUTOFOCUS_MOVESCROLLBAR_INVERT);
    hWndMoveWithShift=GetDlgItem(hDlg, IDC_AUTOFOCUS_MOVEWITHSHIFT);
    hWndMoveWithShiftInvert=GetDlgItem(hDlg, IDC_AUTOFOCUS_MOVEWITHSHIFT_INVERT);
    hWndSwitchTab=GetDlgItem(hDlg, IDC_AUTOFOCUS_SWITCHTAB);
    hWndSwitchTabInvert=GetDlgItem(hDlg, IDC_AUTOFOCUS_SWITCHTAB_INVERT);
    hWndSwitchTabWithSpin=GetDlgItem(hDlg, IDC_AUTOFOCUS_SWITCHTAB_WITHSPIN);

    SetWindowTextWide(hDlg, wszPluginTitle);
    SetDlgItemTextWide(hDlg, IDC_AUTOSCROLL_AUTOSTEP_GROUP, GetLangStringW(wLangModule, STRID_AUTOSTEP));
    SetDlgItemTextWide(hDlg, IDC_AUTOSCROLL_STEPTIME_LABEL, GetLangStringW(wLangModule, STRID_STEPTIME));
    SetDlgItemTextWide(hDlg, IDC_AUTOSCROLL_STEPWIDTH_LABEL, GetLangStringW(wLangModule, STRID_STEPWIDTH));
    SetDlgItemTextWide(hDlg, IDC_SYNCHORZ_GROUP, GetLangStringW(wLangModule, STRID_HSYNCHRONIZATION));
    SetDlgItemTextWide(hDlg, IDC_SYNCHORZ_MDI, GetLangStringW(wLangModule, STRID_FRAMEMDI));
    SetDlgItemTextWide(hDlg, IDC_SYNCHORZ_CLONE, GetLangStringW(wLangModule, STRID_SPLITPANE));
    SetDlgItemTextWide(hDlg, IDC_SYNCVERT_GROUP, GetLangStringW(wLangModule, STRID_VSYNCHRONIZATION));
    SetDlgItemTextWide(hDlg, IDC_SYNCVERT_MDI, GetLangStringW(wLangModule, STRID_FRAMEMDI));
    SetDlgItemTextWide(hDlg, IDC_SYNCVERT_CLONE, GetLangStringW(wLangModule, STRID_SPLITPANE));
    SetDlgItemTextWide(hDlg, IDC_NOSCROLL_GROUP, GetLangStringW(wLangModule, STRID_NOSCROLL));
    SetDlgItemTextWide(hDlg, IDC_NOSCROLL_UNDO, GetLangStringW(wLangModule, STRID_UNDO));
    SetDlgItemTextWide(hDlg, IDC_NOSCROLL_REDO, GetLangStringW(wLangModule, STRID_REDO));
    SetDlgItemTextWide(hDlg, IDC_NOSCROLL_SELECTALL, GetLangStringW(wLangModule, STRID_SELECTALL));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_GROUP, GetLangStringW(wLangModule, STRID_AUTOFOCUS));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_FOCUSBACKGROUND, GetLangStringW(wLangModule, STRID_FOCUSBACKGROUND));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_MOVESCROLLBAR, GetLangStringW(wLangModule, STRID_MOVESCROLLBAR));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_MOVESCROLLBAR_INVERT, GetLangStringW(wLangModule, STRID_INVERT));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_MOVEWITHSHIFT, GetLangStringW(wLangModule, STRID_MOVEWITHSHIFT));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_MOVEWITHSHIFT_INVERT, GetLangStringW(wLangModule, STRID_INVERT));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_SWITCHTAB, GetLangStringW(wLangModule, STRID_SWITCHTAB));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_SWITCHTAB_INVERT, GetLangStringW(wLangModule, STRID_INVERT));
    SetDlgItemTextWide(hDlg, IDC_AUTOFOCUS_SWITCHTAB_WITHSPIN, GetLangStringW(wLangModule, STRID_WITHSPIN));
    SetDlgItemTextWide(hDlg, IDOK, GetLangStringW(wLangModule, STRID_OK));
    SetDlgItemTextWide(hDlg, IDCANCEL, GetLangStringW(wLangModule, STRID_CANCEL));

    if (!bInitAutoScroll) EnableWindow(hWndAutoScrollTitle, FALSE);
    if (!bInitNoScroll) EnableWindow(hWndNoScrollTitle, FALSE);
    if (!bInitSyncHorz) EnableWindow(hWndSyncHorzTitle, FALSE);
    if (!bInitSyncVert) EnableWindow(hWndSyncVertTitle, FALSE);
    if (!bInitAutoFocus) EnableWindow(hWndAutoFocusTitle, FALSE);
    SendMessage(hWndStepTime, EM_LIMITTEXT, 5, 0);
    SendMessage(hWndStepWidth, EM_LIMITTEXT, 5, 0);
    SetDlgItemInt(hDlg, IDC_AUTOSCROLL_STEPTIME, nAutoScrollStepTime, TRUE);
    SetDlgItemInt(hDlg, IDC_AUTOSCROLL_STEPWIDTH, nAutoScrollStepWidth, TRUE);

    if (dwNoScroll & NS_UNDO)
      SendMessage(hWndUndo, BM_SETCHECK, BST_CHECKED, 0);
    if (dwNoScroll & NS_REDO)
      SendMessage(hWndRedo, BM_SETCHECK, BST_CHECKED, 0);
    if (dwNoScroll & NS_SELECTALL)
      SendMessage(hWndSelectAll, BM_SETCHECK, BST_CHECKED, 0);

    if (dwSyncHorz & SNC_MDI)
      SendMessage(hWndSyncHorzMDI, BM_SETCHECK, BST_CHECKED, 0);
    if (dwSyncHorz & SNC_CLONE)
      SendMessage(hWndSyncHorzClone, BM_SETCHECK, BST_CHECKED, 0);
    if (dwSyncVert & SNC_MDI)
      SendMessage(hWndSyncVertMDI, BM_SETCHECK, BST_CHECKED, 0);
    if (dwSyncVert & SNC_CLONE)
      SendMessage(hWndSyncVertClone, BM_SETCHECK, BST_CHECKED, 0);

    if (dwAutoFocus & AF_FOCUSBACKGROUND)
      SendMessage(hWndFocusBackground, BM_SETCHECK, BST_CHECKED, 0);
    if (dwAutoFocus & AF_MOVESCROLLBAR)
      SendMessage(hWndMoveScrollbar, BM_SETCHECK, BST_CHECKED, 0);
    if (dwAutoFocus & AF_MOVESCROLLBARINVERT)
      SendMessage(hWndMoveScrollbarInvert, BM_SETCHECK, BST_CHECKED, 0);
    if (dwAutoFocus & AF_MOVEWITHSHIFT)
      SendMessage(hWndMoveWithShift, BM_SETCHECK, BST_CHECKED, 0);
    if (dwAutoFocus & AF_MOVEWITHSHIFTINVERT)
      SendMessage(hWndMoveWithShiftInvert, BM_SETCHECK, BST_CHECKED, 0);
    if (dwAutoFocus & AF_SWITCHTAB)
      SendMessage(hWndSwitchTab, BM_SETCHECK, BST_CHECKED, 0);
    if (dwAutoFocus & AF_SWITCHTABINVERT)
      SendMessage(hWndSwitchTabInvert, BM_SETCHECK, BST_CHECKED, 0);
    if (dwAutoFocus & AF_SWITCHTABWITHSPIN)
      SendMessage(hWndSwitchTabWithSpin, BM_SETCHECK, BST_CHECKED, 0);

    if (!bAkelEdit || nMDI == WMD_SDI || nMDI == WMD_PMDI)
    {
      EnableWindow(hWndSyncHorzMDI, FALSE);
      EnableWindow(hWndSyncVertMDI, FALSE);
    }
    if (!bAkelEdit)
    {
      EnableWindow(hWndSyncHorzClone, FALSE);
      EnableWindow(hWndSyncVertClone, FALSE);

      EnableWindow(hWndUndo, FALSE);
      EnableWindow(hWndRedo, FALSE);
      EnableWindow(hWndSelectAll, FALSE);
    }
    SendMessage(hDlg, WM_COMMAND, IDC_AUTOFOCUS_MOVESCROLLBAR, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_AUTOFOCUS_MOVEWITHSHIFT, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_AUTOFOCUS_SWITCHTAB, 0);
  }
  else if (uMsg == WM_COMMAND)
  {
    if (LOWORD(wParam) == IDC_AUTOFOCUS_MOVESCROLLBAR)
    {
      bState=(BOOL)SendMessage(hWndMoveScrollbar, BM_GETCHECK, 0, 0);
      EnableWindow(hWndMoveScrollbarInvert, bState);
    }
    else if (LOWORD(wParam) == IDC_AUTOFOCUS_MOVEWITHSHIFT)
    {
      bState=(BOOL)SendMessage(hWndMoveWithShift, BM_GETCHECK, 0, 0);
      EnableWindow(hWndMoveWithShiftInvert, bState);
    }
    else if (LOWORD(wParam) == IDC_AUTOFOCUS_SWITCHTAB)
    {
      bState=(BOOL)SendMessage(hWndSwitchTab, BM_GETCHECK, 0, 0);
      EnableWindow(hWndSwitchTabInvert, bState);
      EnableWindow(hWndSwitchTabWithSpin, bState);
    }
    else if (LOWORD(wParam) == IDOK)
    {
      nAutoScrollStepTime=GetDlgItemInt(hDlg, IDC_AUTOSCROLL_STEPTIME, NULL, TRUE);
      nAutoScrollStepWidth=GetDlgItemInt(hDlg, IDC_AUTOSCROLL_STEPWIDTH, NULL, TRUE);

      dwNoScroll=0;
      if (SendMessage(hWndUndo, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwNoScroll|=NS_UNDO;
      if (SendMessage(hWndRedo, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwNoScroll|=NS_REDO;
      if (SendMessage(hWndSelectAll, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwNoScroll|=NS_SELECTALL;

      dwSyncHorz=0;
      if (SendMessage(hWndSyncHorzMDI, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwSyncHorz|=SNC_MDI;
      if (SendMessage(hWndSyncHorzClone, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwSyncHorz|=SNC_CLONE;

      dwSyncVert=0;
      if (SendMessage(hWndSyncVertMDI, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwSyncVert|=SNC_MDI;
      if (SendMessage(hWndSyncVertClone, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwSyncVert|=SNC_CLONE;

      dwAutoFocus=0;
      if (SendMessage(hWndFocusBackground, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_FOCUSBACKGROUND;
      if (SendMessage(hWndMoveScrollbar, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_MOVESCROLLBAR;
      if (SendMessage(hWndMoveScrollbarInvert, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_MOVESCROLLBARINVERT;
      if (SendMessage(hWndMoveWithShift, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_MOVEWITHSHIFT;
      if (SendMessage(hWndMoveWithShiftInvert, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_MOVEWITHSHIFTINVERT;
      if (SendMessage(hWndSwitchTab, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_SWITCHTAB;
      if (SendMessage(hWndSwitchTabInvert, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_SWITCHTABINVERT;
      if (SendMessage(hWndSwitchTabWithSpin, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwAutoFocus|=AF_SWITCHTABWITHSPIN;

      SaveOptions(0);
      EndDialog(hDlg, 0);

      if (nInitMain)
      {
        if (bInitAutoScroll)
        {
          UninitAutoScroll();
          InitAutoScroll();
        }
        if (bInitNoScroll)
        {
          UninitNoScroll();
          InitNoScroll();
        }
        if (bInitSyncHorz)
        {
          UninitSyncHorz();
          InitSyncHorz();
        }
        if (bInitSyncVert)
        {
          UninitSyncVert();
          InitSyncVert();
        }
      }
      return TRUE;
    }
    else if (LOWORD(wParam) == IDCANCEL)
    {
      EndDialog(hDlg, 0);
      return TRUE;
    }
  }
  else if (uMsg == WM_CLOSE)
  {
    PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
    return TRUE;
  }
  return FALSE;
}

void SetScrollTimer()
{
  if (!dwAutoScrollTimer)
  {
    //if (hWndAutoScroll) HideCaret(hWndAutoScroll);
    dwAutoScrollTimer=SetTimer(NULL, 0, nAutoScrollStepTime, (TIMERPROC)TimerProc);
  }
}

void KillScrollTimer()
{
  if (dwAutoScrollTimer)
  {
    KillTimer(NULL, dwAutoScrollTimer);
    dwAutoScrollTimer=0;
    //if (hWndAutoScroll) ShowCaret(hWndAutoScroll);
  }
}

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  if (hDocAutoScroll)
  {
    if (hDocAutoScroll == GetCurDoc())
    {
      SCROLLINFO si;
      POINT64 ptGlobal;

      si.cbSize=sizeof(SCROLLINFO);
      si.fMask=SIF_PAGE|SIF_POS|SIF_RANGE;
      GetScrollInfo(hWndAutoScroll, SB_VERT, &si);

      if (si.nPage > 0 && si.nPos + (int)si.nPage < si.nMax)
      {
        SendMessage(hWndAutoScroll, AEM_GETSCROLLPOS, 0, (LPARAM)&ptGlobal);
        ptGlobal.y+=nAutoScrollStepWidth;
        SendMessage(hWndAutoScroll, AEM_SETSCROLLPOS, 0, (LPARAM)&ptGlobal);
      }
    }
  }
  else
  {
    hWndAutoScroll=GetCurEdit();
    hDocAutoScroll=GetCurDoc();
  }
}

void GetMsgProcCommon(int code, WPARAM wParam, LPARAM lParam)
{
  if (code >= 0)
  {
    MSG *msg=(MSG *)lParam;

    if (msg->message == WM_MOUSEWHEEL ||
        msg->message == WM_MOUSEHWHEEL)
    {
      HWND hWndPoint;
      POINT ptPos;

      GetCursorPos(&ptPos);

      if ((hWndPoint=WindowFromPoint(ptPos)))
      {
        if (GetClassNameA(hWndPoint, szClassName, MAX_PATH))
        {
          if (!lstrcmpiA(szClassName, "SysTabControl32"))
          {
            if (dwAutoFocus & AF_SWITCHTAB)
            {
              HWND hTabSpin=NULL;
              NMHDR nmhdr;
              int nTabCurIndex;
              int nTabNewIndex;

              if (!(dwAutoFocus & AF_SWITCHTABWITHSPIN) || ((hTabSpin=GetWindow(hWndPoint, GW_CHILD)) && IsWindowVisible(hTabSpin)))
              {
                if (dwAutoFocus & AF_SWITCHTABWITHSPIN)
                {
                  nTabCurIndex=LOWORD(SendMessage(hTabSpin, UDM_GETPOS, 0, 0));
                  nTabNewIndex=nTabCurIndex;
                }
                else
                {
                  nmhdr.hwndFrom=hWndPoint;
                  nmhdr.idFrom=GetDlgCtrlID(hWndPoint);
                  nmhdr.code=TCN_SELCHANGE;

                  nTabCurIndex=(int)SendMessage(hWndPoint, TCM_GETCURSEL, 0, 0);
                  nTabNewIndex=nTabCurIndex;
                }

                if ((!(dwAutoFocus & AF_SWITCHTABINVERT) && (short)HIWORD(msg->wParam) < 0) ||
                    ((dwAutoFocus & AF_SWITCHTABINVERT) && (short)HIWORD(msg->wParam) >= 0))
                {
                  if (nTabCurIndex > 0)
                    --nTabNewIndex;
                }
                else
                {
                  if (nTabCurIndex < (int)(SendMessage(hWndPoint, TCM_GETITEMCOUNT, 0, 0) - 1))
                    ++nTabNewIndex;
                }
                if (nTabNewIndex != nTabCurIndex)
                {
                  if (dwAutoFocus & AF_SWITCHTABWITHSPIN)
                  {
                    SendMessage(hWndPoint, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, nTabNewIndex), 0);
                    SendMessage(hWndPoint, WM_HSCROLL, MAKELONG(SB_ENDSCROLL, 0), 0);
                  }
                  else
                  {
                    if (SendMessage(hWndPoint, TCM_SETCURSEL, (WPARAM)nTabNewIndex, 0) != -1)
                      SendMessage(GetParent(hWndPoint), WM_NOTIFY, (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
                  }
                }
                msg->message=WM_NULL;
              }
            }
          }
          else if (!lstrcmpiA(szClassName, "AkelEditA") ||
                   !lstrcmpiA(szClassName, "AkelEditW") ||
                   !lstrcmpiA(szClassName, "RichEdit20A") ||
                   !lstrcmpiA(szClassName, "RichEdit20W") ||
                   !lstrcmpiA(szClassName, "SysListView32") ||
                   !lstrcmpiA(szClassName, "SysTreeView32") ||
                   !lstrcmpiA(szClassName, "ListBox") ||
                   !lstrcmpiA(szClassName, "ComboBox") ||
                   !lstrcmpiA(szClassName, "Edit"))
          {
            UINT uMsg=msg->message;
            DWORD dwChars=0;
            int nHitTest=0;
            BOOL bInvert=FALSE;

            if (dwAutoFocus & AF_FOCUSBACKGROUND)
            {
              if (hWndPoint != msg->hwnd)
              {
                SetFocus(hWndPoint);
                msg->message=WM_NULL;
              }
            }

            if (msg->message == WM_MOUSEWHEEL && (((dwAutoFocus & AF_MOVESCROLLBAR) && (nHitTest=(int)SendMessage(hWndPoint, WM_NCHITTEST, 0, MAKELONG(ptPos.x, ptPos.y))) == HTHSCROLL) ||
                                                  ((dwAutoFocus & AF_MOVEWITHSHIFT) && (GetKeyState(VK_SHIFT) & 0x80))))
            {
              SystemParametersInfoA(SPI_GETWHEELSCROLLCHARS, 0, &dwChars, 0);
              if (!dwChars) dwChars=3;

              if (nHitTest == HTHSCROLL)
              {
                if (dwAutoFocus & AF_MOVESCROLLBARINVERT)
                  bInvert=TRUE;
              }
              else
              {
                if (dwAutoFocus & AF_MOVEWITHSHIFTINVERT)
                  bInvert=TRUE;
              }

              if ((!bInvert && (short)HIWORD(msg->wParam) < 0) ||
                  (bInvert && (short)HIWORD(msg->wParam) >= 0))
              {
                while (dwChars--) SendMessage(hWndPoint, WM_HSCROLL, SB_LINELEFT, (LPARAM)NULL);
              }
              else
              {
                while (dwChars--) SendMessage(hWndPoint, WM_HSCROLL, SB_LINERIGHT, (LPARAM)NULL);
              }
              msg->message=WM_NULL;
            }
            else if (!(dwAutoFocus & AF_FOCUSBACKGROUND))
            {
              if (hWndPoint != msg->hwnd)
              {
                SendMessage(hWndPoint, uMsg, msg->wParam, msg->lParam);
                msg->message=WM_NULL;
              }
            }
          }
        }
      }
    }
  }
}

LRESULT CALLBACK GetMsgProc1(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[0].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc2(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[1].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc3(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[2].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc4(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[3].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc5(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[4].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc6(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[5].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc7(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[6].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc8(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[7].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc9(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[8].hHook, code, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc10(int code, WPARAM wParam, LPARAM lParam)
{
  GetMsgProcCommon(code, wParam, lParam);
  return CallNextHookEx(ht[9].hHook, code, wParam, lParam);
}

HWND GetCurEdit()
{
  return (HWND)SendMessage(hMainWnd, AKD_GETFRAMEINFO, FI_WNDEDIT, (LPARAM)NULL);
}

AEHDOC GetCurDoc()
{
  return (AEHDOC)SendMessage(hMainWnd, AKD_GETFRAMEINFO, FI_DOCEDIT, (LPARAM)NULL);
}

//// Options

INT_PTR WideOption(HANDLE hOptions, const wchar_t *pOptionName, DWORD dwType, BYTE *lpData, DWORD dwData)
{
  PLUGINOPTIONW po;

  po.pOptionName=pOptionName;
  po.dwType=dwType;
  po.lpData=lpData;
  po.dwData=dwData;
  return SendMessage(hMainWnd, AKD_OPTIONW, (WPARAM)hOptions, (LPARAM)&po);
}

void ReadOptions(DWORD dwFlags)
{
  HANDLE hOptions;

  if (hOptions=(HANDLE)SendMessage(hMainWnd, AKD_BEGINOPTIONSW, POB_READ, (LPARAM)wszPluginName))
  {
    WideOption(hOptions, L"StepTime", PO_DWORD, (LPBYTE)&nAutoScrollStepTime, sizeof(DWORD));
    WideOption(hOptions, L"StepWidth", PO_DWORD, (LPBYTE)&nAutoScrollStepWidth, sizeof(DWORD));
    WideOption(hOptions, L"NoScroll", PO_DWORD, (LPBYTE)&dwNoScroll, sizeof(DWORD));
    WideOption(hOptions, L"SyncHorz", PO_DWORD, (LPBYTE)&dwSyncHorz, sizeof(DWORD));
    WideOption(hOptions, L"SyncVert", PO_DWORD, (LPBYTE)&dwSyncVert, sizeof(DWORD));
    WideOption(hOptions, L"AutoFocus", PO_DWORD, (LPBYTE)&dwAutoFocus, sizeof(DWORD));

    SendMessage(hMainWnd, AKD_ENDOPTIONS, (WPARAM)hOptions, 0);
  }
}

void SaveOptions(DWORD dwFlags)
{
  HANDLE hOptions;

  if (hOptions=(HANDLE)SendMessage(hMainWnd, AKD_BEGINOPTIONSW, POB_SAVE, (LPARAM)wszPluginName))
  {
    WideOption(hOptions, L"StepTime", PO_DWORD, (LPBYTE)&nAutoScrollStepTime, sizeof(DWORD));
    WideOption(hOptions, L"StepWidth", PO_DWORD, (LPBYTE)&nAutoScrollStepWidth, sizeof(DWORD));
    WideOption(hOptions, L"NoScroll", PO_DWORD, (LPBYTE)&dwNoScroll, sizeof(DWORD));
    WideOption(hOptions, L"SyncHorz", PO_DWORD, (LPBYTE)&dwSyncHorz, sizeof(DWORD));
    WideOption(hOptions, L"SyncVert", PO_DWORD, (LPBYTE)&dwSyncVert, sizeof(DWORD));
    WideOption(hOptions, L"AutoFocus", PO_DWORD, (LPBYTE)&dwAutoFocus, sizeof(DWORD));

    SendMessage(hMainWnd, AKD_ENDOPTIONS, (WPARAM)hOptions, 0);
  }
}

const char* GetLangStringA(LANGID wLangID, int nStringID)
{
  static char szStringBuf[MAX_PATH];

  WideCharToMultiByte(CP_ACP, 0, GetLangStringW(wLangID, nStringID), -1, szStringBuf, MAX_PATH, NULL, NULL);
  return szStringBuf;
}

const wchar_t* GetLangStringW(LANGID wLangID, int nStringID)
{
  if (wLangID == LANG_RUSSIAN)
  {
    if (nStringID == STRID_AUTOSTEP)
      return L"\x0410\x0432\x0442\x043E\x043C\x0430\x0442\x0438\x0447\x0435\x0441\x043A\x0430\x044F\x0020\x043F\x0440\x043E\x043A\x0440\x0443\x0442\x043A\x0430";
    if (nStringID == STRID_ENABLE)
      return L"\x0412\x043A\x043B\x044E\x0447\x0435\x043D\x043E";
    if (nStringID == STRID_STEPTIME)
      return L"\x0412\x0440\x0435\x043C\x044F\x0020\x0448\x0430\x0433\x0430";
    if (nStringID == STRID_STEPWIDTH)
      return L"\x0428\x0438\x0440\x0438\x043D\x0430\x0020\x0448\x0430\x0433\x0430";
    if (nStringID == STRID_HSYNCHRONIZATION)
      return L"\x0413\x043E\x0440\x0438\x0437\x043E\x043D\x0442\x0430\x043B\x044C\x043D\x0430\x044F\x0020\x0441\x0438\x043D\x0445\x0440\x043E\x043D\x0438\x0437\x0430\x0446\x0438\x044F";
    if (nStringID == STRID_VSYNCHRONIZATION)
      return L"\x0412\x0435\x0440\x0442\x0438\x043A\x0430\x043B\x044C\x043D\x0430\x044F\x0020\x0441\x0438\x043D\x0445\x0440\x043E\x043D\x0438\x0437\x0430\x0446\x0438\x044F";
    if (nStringID == STRID_FRAMEMDI)
      return L"\x0412\x043A\x043B\x0430\x0434\x043A\x0438\x0020\x0028\x004D\x0044\x0049\x0029";
    if (nStringID == STRID_SPLITPANE)
      return L"\x0420\x0430\x0437\x0434\x0435\x043B\x0435\x043D\x043D\x044B\x0435\x0020\x043E\x043A\x043D\x0430";
    if (nStringID == STRID_NOSCROLL)
      return L"\x041D\x0435\x0020\x043F\x0440\x043E\x043A\x0440\x0443\x0447\x0438\x0432\x0430\x0442\x044C\x0020\x043E\x043F\x0435\x0440\x0430\x0446\x0438\x0438";
    if (nStringID == STRID_UNDO)
      return L"\x041E\x0442\x043C\x0435\x043D\x0438\x0442\x044C\x0020\x0028\x0432\x0435\x0441\x044C\x0020\x0442\x0435\x043A\x0441\x0442\x0029";
    if (nStringID == STRID_REDO)
      return L"\x041F\x043E\x0432\x0442\x043E\x0440\x0438\x0442\x044C\x0020\x0028\x0432\x0435\x0441\x044C\x0020\x0442\x0435\x043A\x0441\x0442\x0029";
    if (nStringID == STRID_SELECTALL)
      return L"\x0412\x044B\x0434\x0435\x043B\x0438\x0442\x044C\x0020\x0432\x0441\x0435";
    if (nStringID == STRID_AUTOFOCUS)
      return L"\x0410\x0432\x0442\x043E\x043C\x0430\x0442\x0438\x0447\x0435\x0441\x043A\x0438\x0439\x0020\x0444\x043E\x043A\x0443\x0441";
    if (nStringID == STRID_FOCUSBACKGROUND)
      return L"\x041F\x0435\x0440\x0435\x0434\x0430\x0447\x0430\x0020\x0444\x043E\x043A\x0443\x0441\x0430";
    if (nStringID == STRID_MOVESCROLLBAR)
      return L"\x041F\x0440\x043E\x043A\x0440\x0443\x0442\x043A\x0430\x0020\x043F\x043E\x043B\x0437\x0443\x043D\x043A\x043E\x043C";
    if (nStringID == STRID_MOVEWITHSHIFT)
      return L"\x041F\x0440\x043E\x043A\x0440\x0443\x0442\x043A\x0430\x0020\x0441\x0020\x043A\x043B\x0430\x0432\x0438\x0448\x0435\x0439 Shift";
    if (nStringID == STRID_SWITCHTAB)
      return L"\x041F\x0435\x0440\x0435\x043A\x043B\x044E\x0447\x0435\x043D\x0438\x0435\x0020\x043C\x0435\x0436\x0434\x0443\x0020\x0432\x043A\x043B\x0430\x0434\x043A\x0430\x043C\x0438";
    if (nStringID == STRID_WITHSPIN)
      return L"\x041A\x043E\x0433\x0434\x0430\x0020\x043A\x043D\x043E\x043F\x043A\x0438\x0020\x043F\x0440\x043E\x043A\x0440\x0443\x0442\x043A\x0438\x0020\x0432\x0438\x0434\x0438\x043C\x044B";
    if (nStringID == STRID_INVERT)
      return L"\x0418\x043D\x0432\x0435\x0440\x0442\x0438\x0440\x043E\x0432\x0430\x0442\x044C";
    if (nStringID == STRID_PLUGIN)
      return L"%s \x043F\x043B\x0430\x0433\x0438\x043D";
    if (nStringID == STRID_OK)
      return L"\x004F\x004B";
    if (nStringID == STRID_CANCEL)
      return L"\x041E\x0442\x043C\x0435\x043D\x0430";
  }
  else
  {
    if (nStringID == STRID_AUTOSTEP)
      return L"Automatic scrolling";
    if (nStringID == STRID_ENABLE)
      return L"Enable";
    if (nStringID == STRID_STEPTIME)
      return L"Step time";
    if (nStringID == STRID_STEPWIDTH)
      return L"Step width";
    if (nStringID == STRID_HSYNCHRONIZATION)
      return L"Horizontal synchronization";
    if (nStringID == STRID_VSYNCHRONIZATION)
      return L"Vertical synchronization";
    if (nStringID == STRID_FRAMEMDI)
      return L"Tabs (MDI)";
    if (nStringID == STRID_SPLITPANE)
      return L"Split panes";
    if (nStringID == STRID_NOSCROLL)
      return L"No scroll operations";
    if (nStringID == STRID_UNDO)
      return L"Undo (all text)";
    if (nStringID == STRID_REDO)
      return L"Redo (all text)";
    if (nStringID == STRID_SELECTALL)
      return L"Select all";
    if (nStringID == STRID_AUTOFOCUS)
      return L"Automatic focus";
    if (nStringID == STRID_FOCUSBACKGROUND)
      return L"Transfer focus";
    if (nStringID == STRID_MOVESCROLLBAR)
      return L"Scrolling with slider";
    if (nStringID == STRID_MOVEWITHSHIFT)
      return L"Scrolling with Shift key";
    if (nStringID == STRID_SWITCHTAB)
      return L"Switching between tabs";
    if (nStringID == STRID_WITHSPIN)
      return L"When scroll buttons visible";
    if (nStringID == STRID_INVERT)
      return L"Invert";
    if (nStringID == STRID_PLUGIN)
      return L"%s plugin";
    if (nStringID == STRID_OK)
      return L"OK";
    if (nStringID == STRID_CANCEL)
      return L"Cancel";
  }
  return L"";
}

BOOL IsExtCallParamValid(LPARAM lParam, int nIndex)
{
  if (*((INT_PTR *)lParam) >= (INT_PTR)((nIndex + 1) * sizeof(INT_PTR)))
    return TRUE;
  return FALSE;
}

INT_PTR GetExtCallParam(LPARAM lParam, int nIndex)
{
  if (*((INT_PTR *)lParam) >= (INT_PTR)((nIndex + 1) * sizeof(INT_PTR)))
    return *(((INT_PTR *)lParam) + nIndex);
  return 0;
}

void InitCommon(PLUGINDATA *pd)
{
  bInitCommon=TRUE;
  hInstanceDLL=pd->hInstanceDLL;
  hMainWnd=pd->hMainWnd;
  hMdiClient=pd->hMdiClient;
  hMainIcon=pd->hMainIcon;
  nMDI=pd->nMDI;
  bAkelEdit=pd->bAkelEdit;
  wLangModule=PRIMARYLANGID(pd->wLangModule);

  //Initialize WideFunc.h header
  WideInitialize();

  //Plugin name
  {
    int i;

    for (i=0; pd->wszFunction[i] != ':'; ++i)
      wszPluginName[i]=pd->wszFunction[i];
    wszPluginName[i]='\0';
  }
  xprintfW(wszPluginTitle, GetLangStringW(wLangModule, STRID_PLUGIN), wszPluginName);
  ReadOptions(0);
}

void InitMain()
{
  if (!nInitMain++)
  {
    //Subclass
    NewMainProcData=NULL;
    SendMessage(hMainWnd, AKD_SETMAINPROC, (WPARAM)NewMainProc, (LPARAM)&NewMainProcData);

    if (nMDI == WMD_MDI)
    {
      NewFrameProcData=NULL;
      SendMessage(hMainWnd, AKD_SETFRAMEPROC, (WPARAM)NewFrameProc, (LPARAM)&NewFrameProcData);
    }
  }
}

void UninitMain()
{
  if (!--nInitMain)
  {
    //Remove subclass
    if (NewMainProcData)
    {
      SendMessage(hMainWnd, AKD_SETMAINPROC, (WPARAM)NULL, (LPARAM)&NewMainProcData);
      NewMainProcData=NULL;
    }
    if (NewFrameProcData)
    {
      SendMessage(hMainWnd, AKD_SETFRAMEPROC, (WPARAM)NULL, (LPARAM)&NewFrameProcData);
      NewFrameProcData=NULL;
    }
  }
}

void InitAutoScroll()
{
  bInitAutoScroll=TRUE;

  hWndAutoScroll=GetCurEdit();
  hDocAutoScroll=GetCurDoc();
  SetScrollTimer();
}

void UninitAutoScroll()
{
  bInitAutoScroll=FALSE;

  KillScrollTimer();
  hWndAutoScroll=NULL;
  hDocAutoScroll=NULL;
}

void InitSyncHorz()
{
  bInitSyncHorz=TRUE;
}

void UninitSyncHorz()
{
  bInitSyncHorz=FALSE;
}

void InitSyncVert()
{
  bInitSyncVert=TRUE;
}

void UninitSyncVert()
{
  bInitSyncVert=FALSE;
}

void InitNoScroll()
{
  bInitNoScroll=TRUE;
}

void UninitNoScroll()
{
  bInitNoScroll=FALSE;
}

void InitAutoFocus()
{
  bInitAutoFocus=TRUE;

  xmemset(&ht, 0, sizeof(ht));
  ht[0].GetMsgProc=GetMsgProc1;
  ht[1].GetMsgProc=GetMsgProc2;
  ht[2].GetMsgProc=GetMsgProc3;
  ht[3].GetMsgProc=GetMsgProc4;
  ht[4].GetMsgProc=GetMsgProc5;
  ht[5].GetMsgProc=GetMsgProc6;
  ht[6].GetMsgProc=GetMsgProc7;
  ht[7].GetMsgProc=GetMsgProc8;
  ht[8].GetMsgProc=GetMsgProc9;
  ht[9].GetMsgProc=GetMsgProc10;

  ht[0].dwThreadId=GetCurrentThreadId();
  ht[0].hHook=SetWindowsHookExA(WH_GETMESSAGE, ht[0].GetMsgProc, 0, ht[0].dwThreadId);
}

void UninitAutoFocus()
{
  int i;

  bInitAutoFocus=FALSE;

  //Free all threads
  for (i=0; i < HT_MAX; ++i)
  {
    if (ht[i].dwThreadId)
    {
      UnhookWindowsHookEx(ht[i].hHook);
      ht[i].hHook=NULL;
      ht[i].dwThreadId=0;
    }
  }
}


//Entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
  {
  }
  else if (fdwReason == DLL_THREAD_ATTACH)
  {
    //Hook current thread
    if (bInitAutoFocus)
    {
      DWORD dwThreadId=GetCurrentThreadId();
      int i;

      for (i=0; i < HT_MAX; ++i)
      {
        if (dwThreadId == ht[i].dwThreadId)
          return TRUE;
      }

      for (i=0; i < HT_MAX; ++i)
      {
        if (!ht[i].dwThreadId)
        {
          ht[i].dwThreadId=dwThreadId;
          ht[i].hHook=SetWindowsHookExA(WH_GETMESSAGE, ht[i].GetMsgProc, 0, ht[i].dwThreadId);
          break;
        }
      }
    }
  }
  else if (fdwReason == DLL_THREAD_DETACH)
  {
    //Unhook current thread
    if (bInitAutoFocus)
    {
      DWORD dwThreadId=GetCurrentThreadId();
      int i;

      for (i=0; i < HT_MAX; ++i)
      {
        if (ht[i].dwThreadId)
        {
          if (ht[i].dwThreadId == dwThreadId)
          {
            UnhookWindowsHookEx(ht[i].hHook);
            ht[i].hHook=NULL;
            ht[i].dwThreadId=0;
            break;
          }
        }
      }
    }
  }
  else if (fdwReason == DLL_PROCESS_DETACH)
  {
    if (bInitAutoScroll)
    {
      UninitMain();
      UninitAutoScroll();
    }
    if (bInitSyncHorz)
    {
      UninitMain();
      UninitSyncHorz();
    }
    if (bInitSyncVert)
    {
      UninitMain();
      UninitSyncVert();
    }
    if (bInitNoScroll)
    {
      UninitMain();
      UninitNoScroll();
    }
    if (bInitAutoFocus)
    {
      UninitMain();
      UninitAutoFocus();
    }
  }
  return TRUE;
}
