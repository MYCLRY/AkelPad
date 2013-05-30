#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include "StrFunc.h"
#include "WideFunc.h"
#include "AkelEdit.h"
#include "AkelDLL.h"
#include "Resources\Resource.h"


//Include AEC functions
#define AEC_FUNCTIONS
#include "AkelEdit.h"

//Include string functions
#define xmemcpy
#define xmemset
#define xstrlenW
#define xstrcpyW
#define xstrcpynW
#define xatoiW
#define xitoaW
#define xuitoaW
#define dec2hexW
#define hex2decA
#define hex2decW
#define xprintfW
#include "StrFunc.h"

//Include wide functions
#define CreateFontIndirectWide
#define DialogBoxWide
#define GetWindowTextWide
#define ListBox_AddStringWide
#define SetDlgItemTextWide
#define SetWindowTextWide
#include "WideFunc.h"

//Defines
#define STRID_SPACE       1
#define STRID_TAB         2
#define STRID_NEWLINE     3
#define STRID_VERTICALTAB 4
#define STRID_FORMFEED    5
#define STRID_NULL        6
#define STRID_WRAP        7
#define STRID_INDENTLINE  8
#define STRID_PLUGIN      9
#define STRID_COLOR       10
#define STRID_SELCOLOR    11
#define STRID_DOTTEDLINE  12
#define STRID_SYMBOL      13
#define STRID_OK          14
#define STRID_CANCEL      15

#define DLLA_SPECIALCHAR_SET 1
#define DLLA_SPECIALCHAR_GET 2

#define OF_SETTINGS       0x1

typedef struct _SPECIALCOLOR {
  DWORD dwColor;
  DWORD dwSelColor;
  BOOL bColorEnable;
  BOOL bSelColorEnable;
} SPECIALCOLOR;

typedef struct _SPECIALCHAR {
  int nReserved;
  wchar_t wszNewChar[2];
  wchar_t wszOldChar[2];
  SPECIALCOLOR sclr;
} SPECIALCHAR;

#define SC_SPACE        0
#define SC_TAB          1
#define SC_NEWLINE      2
#define SC_VERTICALTAB  3
#define SC_FORMFEED     4
#define SC_NULL         5
#define SC_WRAP         6
#define SC_INDENTLINE   7
#define SC_MAX          8

//UpdateEdit flags
#define UE_ERASE        0x01
#define UE_ALLRECT      0x02
#define UE_DRAWRECT     0x04
#define UE_FIRSTPIXEL   0x08
#define UE_UPDATEWINDOW 0x10

#ifndef SPI_GETCLEARTYPE
  #define SPI_GETCLEARTYPE 0x1048
#endif

//Functions prototypes
BOOL CALLBACK SetupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditParentMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL GetLineSpaces(AECHARINDEX *ciMinDraw, int nTabStopSize, INT_PTR *lpnLineSpaces);
BOOL GetCharColor(HWND hWndEdit, INT_PTR nCharOffset, AECHARCOLORS *aecc);
COLORREF GetColorValueFromStrA(char *pColor);
COLORREF GetColorValueFromStrW(wchar_t *wpColor);
char* GetColorStrFromValueA(COLORREF crColor, char *szColor);
wchar_t* GetColorStrFromValueW(COLORREF crColor, wchar_t *wszColor);
int GetNewLineString(int nNewLine, const wchar_t **wpNewLine);
void UpdateEdit(HWND hWnd, DWORD dwFlags);
void UpdateEditAll(DWORD dwFlags);

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

//Global variables
wchar_t wszPluginName[MAX_PATH];
wchar_t wszPluginTitle[MAX_PATH];
HINSTANCE hInstanceDLL;
HWND hMainWnd;
HWND hMdiClient;
BOOL bOldWindows;
BOOL bOldRichEdit;
BOOL bAkelEdit;
int nMDI;
LANGID wLangModule;
BOOL bInitCommon=FALSE;
BOOL bInitMain=FALSE;
DWORD dwSaveFlags=0;
SPECIALCHAR schArray[SC_MAX]={0};
DWORD dwPaintOptions=0;
BOOL bDottedLine=TRUE;
WNDPROCDATA *NewMainProcData=NULL;
WNDPROCDATA *NewFrameProcData=NULL;
WNDPROCDATA *NewEditProcData=NULL;


//Identification
void __declspec(dllexport) DllAkelPadID(PLUGINVERSION *pv)
{
  pv->dwAkelDllVersion=AKELDLL;
  pv->dwExeMinVersion3x=MAKE_IDENTIFIER(-1, -1, -1, -1);
  pv->dwExeMinVersion4x=MAKE_IDENTIFIER(4, 7, 9, 0);
  pv->pPluginName="SpecialChar";
}

//Plugin extern function
void __declspec(dllexport) Main(PLUGINDATA *pd)
{
  pd->dwSupport|=PDS_SUPPORTALL;
  if (pd->dwSupport & PDS_GETSUPPORT)
    return;

  if (!bInitCommon) InitCommon(pd);

  //Is plugin already loaded?
  if (bInitMain)
  {
    UninitMain();
    UpdateEditAll(UE_DRAWRECT);
  }
  else
  {
    InitMain();
    if (!pd->bOnStart)
      UpdateEditAll(UE_DRAWRECT);

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

    if (nAction == DLLA_SPECIALCHAR_SET)
    {
      wchar_t wszChars[MAX_PATH];
      unsigned char *pSpecialChar=NULL;
      unsigned char *pColor=NULL;
      unsigned char *pSelColor=NULL;
      BOOL bColorEnable=-2;
      BOOL bSelColorEnable=-2;
      unsigned char *pNewChar=NULL;
      const wchar_t *wpNextChar=wszChars;
      int nSpecialChar;
      SPECIALCHAR schFirst;
      BOOL bUpdate=FALSE;

      if (IsExtCallParamValid(pd->lParam, 2))
        pSpecialChar=(unsigned char *)GetExtCallParam(pd->lParam, 2);
      if (IsExtCallParamValid(pd->lParam, 3))
        pColor=(unsigned char *)GetExtCallParam(pd->lParam, 3);
      if (IsExtCallParamValid(pd->lParam, 4))
        pSelColor=(unsigned char *)GetExtCallParam(pd->lParam, 4);
      if (IsExtCallParamValid(pd->lParam, 5))
        bColorEnable=(BOOL)GetExtCallParam(pd->lParam, 5);
      if (IsExtCallParamValid(pd->lParam, 6))
        bSelColorEnable=(BOOL)GetExtCallParam(pd->lParam, 6);
      if (IsExtCallParamValid(pd->lParam, 7))
        pNewChar=(unsigned char *)GetExtCallParam(pd->lParam, 7);

      schFirst.sclr.dwColor=(DWORD)-1;

      if (pd->dwSupport & PDS_STRANSI)
        MultiByteToWideChar(CP_ACP, 0, (char *)pSpecialChar, -1, wszChars, MAX_PATH);
      else
        xstrcpynW(wszChars, (wchar_t *)pSpecialChar, MAX_PATH);

      while (nSpecialChar=(int)xatoiW(wpNextChar, &wpNextChar))
      {
        //Zero-based
        --nSpecialChar;

        if (nSpecialChar >= SC_SPACE && nSpecialChar < SC_MAX)
        {
          if (schFirst.sclr.dwColor == (DWORD)-1)
            xmemcpy(&schFirst, &schArray[nSpecialChar], sizeof(SPECIALCHAR));

          if (pd->dwSupport & PDS_STRANSI)
          {
            if (pColor && *(char *)pColor == '#')
              schArray[nSpecialChar].sclr.dwColor=GetColorValueFromStrA((char *)pColor + 1);
            if (pSelColor && *(char *)pSelColor == '#')
              schArray[nSpecialChar].sclr.dwSelColor=GetColorValueFromStrA((char *)pSelColor + 1);
          }
          else
          {
            if (pColor && *(wchar_t *)pColor == '#')
              schArray[nSpecialChar].sclr.dwColor=GetColorValueFromStrW((wchar_t *)pColor + 1);
            if (pSelColor && *(wchar_t *)pSelColor == '#')
              schArray[nSpecialChar].sclr.dwSelColor=GetColorValueFromStrW((wchar_t *)pSelColor + 1);
          }

          if (bColorEnable != -2)
          {
            if (bColorEnable == -1)
              schArray[nSpecialChar].sclr.bColorEnable=!schFirst.sclr.bColorEnable;
            else
              schArray[nSpecialChar].sclr.bColorEnable=bColorEnable;
          }
          if (bSelColorEnable != -2)
          {
            if (bSelColorEnable == -1)
              schArray[nSpecialChar].sclr.bSelColorEnable=!schFirst.sclr.bSelColorEnable;
            else
              schArray[nSpecialChar].sclr.bSelColorEnable=bSelColorEnable;
          }

          if (pNewChar)
          {
            if (pd->dwSupport & PDS_STRANSI)
              MultiByteToWideChar(CP_ACP, 0, (char *)pNewChar, 1, schArray[nSpecialChar].wszNewChar, 1);
            else
              xstrcpynW(schArray[nSpecialChar].wszNewChar, (wchar_t *)pNewChar, 2);
          }
          bUpdate=TRUE;
        }
        if (*wpNextChar == L',')
          ++wpNextChar;
        else
          break;
      }

      if (bUpdate)
      {
        UpdateEditAll(UE_DRAWRECT);
        dwSaveFlags|=OF_SETTINGS;
      }
    }
    else if (nAction == DLLA_SPECIALCHAR_GET)
    {
      wchar_t wszChars[MAX_PATH];
      unsigned char *pSpecialChar=NULL;
      COLORREF *lpcrColor=NULL;
      COLORREF *lpcrSelColor=NULL;
      BOOL *lpbColorEnable=NULL;
      BOOL *lpbSelColorEnable=NULL;
      unsigned char *pNewChar=NULL;
      const wchar_t *wpNextChar=wszChars;
      int nSpecialChar;

      if (IsExtCallParamValid(pd->lParam, 2))
        pSpecialChar=(unsigned char *)GetExtCallParam(pd->lParam, 2);
      if (IsExtCallParamValid(pd->lParam, 3))
        lpcrColor=(COLORREF *)GetExtCallParam(pd->lParam, 3);
      if (IsExtCallParamValid(pd->lParam, 4))
        lpcrSelColor=(COLORREF *)GetExtCallParam(pd->lParam, 4);
      if (IsExtCallParamValid(pd->lParam, 5))
        lpbColorEnable=(BOOL *)GetExtCallParam(pd->lParam, 5);
      if (IsExtCallParamValid(pd->lParam, 6))
        lpbSelColorEnable=(BOOL *)GetExtCallParam(pd->lParam, 6);
      if (IsExtCallParamValid(pd->lParam, 7))
        pNewChar=(unsigned char *)GetExtCallParam(pd->lParam, 7);

      if (pd->dwSupport & PDS_STRANSI)
        MultiByteToWideChar(CP_ACP, 0, (char *)pSpecialChar, -1, wszChars, MAX_PATH);
      else
        xstrcpynW(wszChars, (wchar_t *)pSpecialChar, MAX_PATH);

      while (nSpecialChar=(int)xatoiW(wpNextChar, &wpNextChar))
      {
        //Zero-based
        --nSpecialChar;

        if (nSpecialChar >= SC_SPACE && nSpecialChar < SC_MAX)
        {
          if (lpcrColor)
            *lpcrColor=schArray[nSpecialChar].sclr.dwColor;
          if (lpcrSelColor)
            *lpcrSelColor=schArray[nSpecialChar].sclr.dwSelColor;
          if (lpbColorEnable)
            *lpbColorEnable=schArray[nSpecialChar].sclr.bColorEnable;
          if (lpbSelColorEnable)
            *lpbSelColorEnable=schArray[nSpecialChar].sclr.bSelColorEnable;
          if (pNewChar)
            xstrcpynW(schArray[nSpecialChar].wszNewChar, (wchar_t *)pNewChar, 2);

          //Check only first special char
          break;
        }
        if (*wpNextChar == L',')
          ++wpNextChar;
        else
          break;
      }
    }

    //If plugin already loaded, stay in memory and don't change active status
    if (pd->bInMemory) pd->nUnload=UD_NONUNLOAD_UNCHANGE;
    return;
  }
  DialogBoxWide(hInstanceDLL, MAKEINTRESOURCEW(IDD_SETUP), hMainWnd, (DLGPROC)SetupDlgProc);

  //If plugin already loaded, stay in memory, but show as non-active
  if (pd->bInMemory) pd->nUnload=UD_NONUNLOAD_NONACTIVE;
}

BOOL CALLBACK SetupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static HICON hPluginIcon;
  static SPECIALCHAR schArrayDlg[sizeof(schArray) / sizeof(SPECIALCHAR)];
  static HWND hWndList;
  static HWND hWndColorCheck;
  static HWND hWndColorButton;
  static HWND hWndColorLabel;
  static HWND hWndSelColorCheck;
  static HWND hWndSelColorButton;
  static HWND hWndSelColorLabel;
  static HWND hWndDottedLineCheck;
  static HWND hWndCharEdit;
  static HWND hWndCharLabel;
  static int nCurItem=0;

  if (uMsg == WM_INITDIALOG)
  {
    //Load plugin icon
    hPluginIcon=LoadIconA(hInstanceDLL, MAKEINTRESOURCEA(IDI_ICON_PLUGIN));
    SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hPluginIcon);

    hWndList=GetDlgItem(hDlg, IDC_LIST);
    hWndColorCheck=GetDlgItem(hDlg, IDC_COLOR_CHECK);
    hWndColorButton=GetDlgItem(hDlg, IDC_COLOR_BUTTON);
    hWndColorLabel=GetDlgItem(hDlg, IDC_COLOR_LABEL);
    hWndSelColorCheck=GetDlgItem(hDlg, IDC_SELCOLOR_CHECK);
    hWndSelColorButton=GetDlgItem(hDlg, IDC_SELCOLOR_BUTTON);
    hWndSelColorLabel=GetDlgItem(hDlg, IDC_SELCOLOR_LABEL);
    hWndDottedLineCheck=GetDlgItem(hDlg, IDC_DOTTEDLINE_CHECK);
    hWndCharEdit=GetDlgItem(hDlg, IDC_CHAR_EDIT);
    hWndCharLabel=GetDlgItem(hDlg, IDC_CHAR_LABEL);

    SetWindowTextWide(hDlg, wszPluginTitle);
    SetDlgItemTextWide(hDlg, IDC_COLOR_LABEL, GetLangStringW(wLangModule, STRID_COLOR));
    SetDlgItemTextWide(hDlg, IDC_SELCOLOR_LABEL, GetLangStringW(wLangModule, STRID_SELCOLOR));
    SetDlgItemTextWide(hDlg, IDC_DOTTEDLINE_CHECK, GetLangStringW(wLangModule, STRID_DOTTEDLINE));
    SetDlgItemTextWide(hDlg, IDC_CHAR_LABEL, GetLangStringW(wLangModule, STRID_SYMBOL));
    SetDlgItemTextWide(hDlg, IDOK, GetLangStringW(wLangModule, STRID_OK));
    SetDlgItemTextWide(hDlg, IDCANCEL, GetLangStringW(wLangModule, STRID_CANCEL));

    SendMessage(hWndCharEdit, EM_LIMITTEXT, 1, 0);
    xmemcpy(&schArrayDlg, &schArray, sizeof(schArray));

    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_SPACE));
    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_TAB));
    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_NEWLINE));
    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_VERTICALTAB));
    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_FORMFEED));
    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_NULL));
    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_WRAP));
    ListBox_AddStringWide(hWndList, GetLangStringW(wLangModule, STRID_INDENTLINE));

    if (bDottedLine) SendMessage(hWndDottedLineCheck, BM_SETCHECK, BST_CHECKED, 0);

    SendMessage(hWndList, LB_SETCURSEL, (WPARAM)nCurItem, 0);
    SendMessage(hDlg, WM_COMMAND, MAKELONG(IDC_LIST, LBN_SELCHANGE), (LPARAM)hWndList);
  }
  else if (uMsg == WM_DRAWITEM)
  {
    DRAWITEMSTRUCT *dis=(DRAWITEMSTRUCT *)lParam;
    HBRUSH hBrush;
    DWORD dwColor=GetSysColor(COLOR_BTNFACE);

    if (wParam == IDC_COLOR_BUTTON)
    {
      if (schArrayDlg[nCurItem].sclr.bColorEnable)
        dwColor=schArrayDlg[nCurItem].sclr.dwColor;
    }
    else if (wParam == IDC_SELCOLOR_BUTTON)
    {
      if (schArrayDlg[nCurItem].sclr.bSelColorEnable)
        dwColor=schArrayDlg[nCurItem].sclr.dwSelColor;
    }

    if (hBrush=CreateSolidBrush(dwColor))
    {
      FillRect(dis->hDC, &dis->rcItem, hBrush);
      if (dis->hwndItem == GetFocus())
        DrawFocusRect(dis->hDC, &dis->rcItem);
      DeleteObject(hBrush);
    }
    return TRUE;
  }
  else if (uMsg == WM_COMMAND)
  {
    if (LOWORD(wParam) == IDC_LIST)
    {
      if (HIWORD(wParam) == LBN_SELCHANGE)
      {
        nCurItem=(int)SendMessage(hWndList, LB_GETCURSEL, 0, 0);

        SetWindowTextWide(hWndCharEdit, schArrayDlg[nCurItem].wszNewChar);

        SendMessage(hWndColorCheck, BM_SETCHECK, schArrayDlg[nCurItem].sclr.bColorEnable, 0);
        EnableWindow(hWndColorButton, schArrayDlg[nCurItem].sclr.bColorEnable);
        EnableWindow(hWndColorLabel, schArrayDlg[nCurItem].sclr.bColorEnable);
        InvalidateRect(hWndColorButton, NULL, FALSE);

        SendMessage(hWndSelColorCheck, BM_SETCHECK, schArrayDlg[nCurItem].sclr.bSelColorEnable, 0);
        EnableWindow(hWndSelColorButton, schArrayDlg[nCurItem].sclr.bSelColorEnable);
        EnableWindow(hWndSelColorLabel, schArrayDlg[nCurItem].sclr.bSelColorEnable);
        InvalidateRect(hWndSelColorButton, NULL, FALSE);

        ShowWindow(hWndCharEdit, nCurItem == SC_INDENTLINE?SW_HIDE:SW_SHOW);
        ShowWindow(hWndCharLabel, nCurItem == SC_INDENTLINE?SW_HIDE:SW_SHOW);
        ShowWindow(hWndDottedLineCheck, nCurItem == SC_INDENTLINE?SW_SHOW:SW_HIDE);
      }
    }
    else if (LOWORD(wParam) == IDC_CHAR_EDIT)
    {
      GetWindowTextWide(hWndCharEdit, schArrayDlg[nCurItem].wszNewChar, 2);
    }
    else if (LOWORD(wParam) == IDC_COLOR_CHECK)
    {
      schArrayDlg[nCurItem].sclr.bColorEnable=(BOOL)SendMessage(hWndColorCheck, BM_GETCHECK, 0, 0);
      EnableWindow(hWndColorButton, schArrayDlg[nCurItem].sclr.bColorEnable);
      EnableWindow(hWndColorLabel, schArrayDlg[nCurItem].sclr.bColorEnable);
      InvalidateRect(hWndColorButton, NULL, FALSE);
    }
    else if (LOWORD(wParam) == IDC_SELCOLOR_CHECK)
    {
      schArrayDlg[nCurItem].sclr.bSelColorEnable=(BOOL)SendMessage(hWndSelColorCheck, BM_GETCHECK, 0, 0);
      EnableWindow(hWndSelColorButton, schArrayDlg[nCurItem].sclr.bSelColorEnable);
      EnableWindow(hWndSelColorLabel, schArrayDlg[nCurItem].sclr.bSelColorEnable);
      InvalidateRect(hWndSelColorButton, NULL, FALSE);
    }
    else if (LOWORD(wParam) == IDC_COLOR_BUTTON ||
             LOWORD(wParam) == IDC_SELCOLOR_BUTTON)
    {
      static COLORREF crCustColors[16];
      COLORREF *lpcrColor=NULL;
      HWND hWndButton=NULL;

      if (LOWORD(wParam) == IDC_COLOR_BUTTON)
      {
        lpcrColor=&schArrayDlg[nCurItem].sclr.dwColor;
        hWndButton=hWndColorButton;
      }
      else if (LOWORD(wParam) == IDC_SELCOLOR_BUTTON)
      {
        lpcrColor=&schArrayDlg[nCurItem].sclr.dwSelColor;
        hWndButton=hWndSelColorButton;
      }

      if (bOldWindows)
      {
        CHOOSECOLORA ccA;

        xmemset(&ccA, 0, sizeof(CHOOSECOLORA));
        ccA.lStructSize  =sizeof(CHOOSECOLORA);
        ccA.hwndOwner    =hDlg;
        ccA.lpCustColors =crCustColors;
        ccA.Flags        =CC_FULLOPEN|CC_RGBINIT;
        ccA.rgbResult    =*lpcrColor;

        if (ChooseColorA(&ccA))
        {
          *lpcrColor=ccA.rgbResult;
          InvalidateRect(hWndButton, NULL, FALSE);
        }
      }
      else
      {
        CHOOSECOLORW ccW;

        xmemset(&ccW, 0, sizeof(CHOOSECOLORW));
        ccW.lStructSize  =sizeof(CHOOSECOLORW);
        ccW.hwndOwner    =hDlg;
        ccW.lpCustColors =crCustColors;
        ccW.Flags        =CC_FULLOPEN|CC_RGBINIT;
        ccW.rgbResult    =*lpcrColor;

        if (ChooseColorW(&ccW))
        {
          *lpcrColor=ccW.rgbResult;
          InvalidateRect(hWndButton, NULL, FALSE);
        }
      }
    }
    else if (LOWORD(wParam) == IDOK)
    {
      xmemcpy(&schArray, &schArrayDlg, sizeof(schArray));
      bDottedLine=(BOOL)SendMessage(hWndDottedLineCheck, BM_GETCHECK, 0, 0);

      SaveOptions(0);
      EndDialog(hDlg, 0);
      UpdateEditAll(UE_DRAWRECT);
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
  else if (uMsg == WM_DESTROY)
  {
    //Destroy plugin icon
    DestroyIcon(hPluginIcon);
  }
  return FALSE;
}

LRESULT CALLBACK NewMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == AKDN_MAIN_ONFINISH)
  {
    NewMainProcData->NextProc(hWnd, uMsg, wParam, lParam);

    UninitMain();
    return FALSE;
  }

  //Special messages
  {
    LRESULT lResult;

    if (lResult=EditParentMessages(hWnd, uMsg, wParam, lParam))
      return lResult;
  }

  //Call next procedure
  return NewMainProcData->NextProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK NewFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  //Special messages
  {
    LRESULT lResult;

    if (lResult=EditParentMessages(hWnd, uMsg, wParam, lParam))
      return lResult;
  }

  //Call next procedure
  return NewFrameProcData->NextProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK NewEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_PAINT)
  {
    DWORD dwEventMask;

    //Enable AEN_PAINT
    dwEventMask=(DWORD)SendMessage(hWnd, AEM_GETEVENTMASK, 0, 0);
    if (!(dwEventMask & AENM_PAINT))
      SendMessage(hWnd, AEM_SETEVENTMASK, 0, dwEventMask|AENM_PAINT);
  }

  //Call next procedure
  return NewEditProcData->NextProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK EditParentMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_NOTIFY)
  {
    if (wParam == ID_EDIT)
    {
      if (((NMHDR *)lParam)->code == AEN_PAINT)
      {
        AENPAINT *pnt=(AENPAINT *)lParam;
        static AECOLORS aec;
        static AESELECTION aes;
        static HBRUSH hbrSelBk;
        static INT_PTR nPrevLineSpaces;
        static INT_PTR nNextLineSpaces;
        static int nInitTopStartLine;
        static int nInitTopEndLine;
        static int nPrevLine;
        static int nNextLine;
        static int nCharHeight;
        static int nCharHeightNoGap;
        static int nAveCharWidth;
        static int nSpaceWidth;
        static int nTabStopSize;
        static BOOL bIntCall;
        static BOOL bClearType;

        if (pnt->dwType == AEPNT_BEGIN)
        {
          aec.dwFlags=AECLR_ALL;
          SendMessage(pnt->hdr.hwndFrom, AEM_GETCOLORS, 0, (LPARAM)&aec);
          hbrSelBk=CreateSolidBrush(aec.crSelBk);
          SystemParametersInfoA(SPI_GETCLEARTYPE, 0, &bClearType, 0);

          SendMessage(pnt->hdr.hwndFrom, AEM_GETSEL, (WPARAM)NULL, (LPARAM)&aes);
          nCharHeight=(int)SendMessage(pnt->hdr.hwndFrom, AEM_GETCHARSIZE, AECS_HEIGHT, 0);
          nCharHeightNoGap=nCharHeight - (int)SendMessage(pnt->hdr.hwndFrom, AEM_GETLINEGAP, 0, 0);
          nAveCharWidth=(int)SendMessage(pnt->hdr.hwndFrom, AEM_GETCHARSIZE, AECS_AVEWIDTH, 0);
          nSpaceWidth=(int)SendMessage(pnt->hdr.hwndFrom, AEM_GETCHARSIZE, AECS_SPACEWIDTH, 0);
          nTabStopSize=(int)SendMessage(pnt->hdr.hwndFrom, AEM_GETTABSTOP, 0, 0);
          nPrevLineSpaces=-1;
          nNextLineSpaces=-1;
          nInitTopStartLine=-1;
          nInitTopEndLine=-1;
          nPrevLine=-1;
          nNextLine=-1;
        }
        else if (pnt->dwType == AEPNT_DRAWLINE)
        {
          AECHARINDEX ciCount=pnt->ciMinDraw;
          SPECIALCHAR *pscChar;
          SPECIALCHAR *pscIndent;
          COLORREF crCharColor;
          COLORREF crIndentColor;
          COLORREF crTextColorPrev;
          POINT pt;
          INT_PTR nOffset=pnt->nMinDrawOffset;
          INT_PTR nLineSpaces=0;
          INT_PTR nMaxLineSpaces=0;
          int nChar;
          int nBkModePrev;
          int i;
          BOOL bIndentFound=FALSE;
          BOOL bCharInSel;

          nBkModePrev=SetBkMode(pnt->hDC, TRANSPARENT);

          if (schArray[SC_INDENTLINE].sclr.bColorEnable || schArray[SC_INDENTLINE].sclr.bSelColorEnable)
          {
            if (!pnt->ciMinDraw.lpLine->prev || pnt->ciMinDraw.lpLine->prev->nLineBreak != AELB_WRAP)
            {
              bIndentFound=GetLineSpaces(&pnt->ciMinDraw, nTabStopSize, &nLineSpaces);
            }

            if (bIndentFound || (!bIntCall && nInitTopStartLine == -1))
            {
              if (nPrevLineSpaces == -1)
              {
                ciCount=pnt->ciMinDraw;
                nMaxLineSpaces=0;

                while (AEC_PrevLine(&ciCount) && pnt->ciMinDraw.nLine - ciCount.nLine <= 50)
                {
                  ciCount.nCharInLine=ciCount.lpLine->nLineLen;
                  if (!GetLineSpaces(&ciCount, nTabStopSize, &nPrevLineSpaces))
                    break;
                  nMaxLineSpaces=max(nPrevLineSpaces, nMaxLineSpaces);
                }
                nPrevLineSpaces=max(nPrevLineSpaces, nMaxLineSpaces);
                nPrevLine=ciCount.nLine;
              }
              if (nNextLineSpaces == -1)
              {
                ciCount=pnt->ciMinDraw;
                nMaxLineSpaces=0;

                while (AEC_NextLine(&ciCount) && ciCount.nLine - pnt->ciMinDraw.nLine <= 50)
                {
                  ciCount.nCharInLine=ciCount.lpLine->nLineLen;
                  if (!GetLineSpaces(&ciCount, nTabStopSize, &nNextLineSpaces))
                    break;
                  nMaxLineSpaces=max(nNextLineSpaces, nMaxLineSpaces);
                }
                nNextLineSpaces=max(nNextLineSpaces, nMaxLineSpaces);
                nNextLine=ciCount.nLine;
              }
              if (nInitTopStartLine == -1)
              {
                nInitTopStartLine=nPrevLine + 1;
                nInitTopEndLine=pnt->ciMinDraw.nLine - 1;
              }
              nMaxLineSpaces=max(nPrevLineSpaces, nNextLineSpaces);
            }
          }

          ciCount=pnt->ciMinDraw;

          do
          {
            pscChar=NULL;
            pscIndent=NULL;

            if (ciCount.lpLine->nLineBreak == AELB_WRAP &&
                ciCount.nCharInLine >= ciCount.lpLine->nLineLen)
            {
              nChar=-AELB_WRAP;
            }
            else nChar=AEC_CharAtIndex(&ciCount);

            for (i=0; i < SC_MAX; ++i)
            {
              if ((nChar == -AELB_EOF && schArray[i].wszOldChar[0] == L'\0') ||
                  (nChar == -AELB_WRAP && schArray[i].wszOldChar[0] == L'\n') ||
                  (nChar < -AELB_EOF && nChar > -AELB_WRAP && schArray[i].wszOldChar[0] == L'\r') ||
                  nChar == schArray[i].wszOldChar[0])
              {
                pscChar=&schArray[i];
                break;
              }
            }
            if (bIndentFound)
            {
              if (nChar >= 0)
              {
                if (nChar == L' ')
                  ++nLineSpaces;
                else if (nChar != L'\t')
                  bIndentFound=FALSE;

                if (bIndentFound && ciCount.nCharInLine &&
                    (nTabStopSize == 1 ||
                     (nChar == L' ' && (nLineSpaces % nTabStopSize) == 1) ||
                     (nChar == L'\t' && (nLineSpaces % nTabStopSize) == 0)))
                  pscIndent=&schArray[SC_INDENTLINE];

                if (nChar == L'\t')
                  nLineSpaces+=nTabStopSize - nLineSpaces % nTabStopSize;
              }
              else pscIndent=&schArray[SC_INDENTLINE];
            }

            if (pscChar || pscIndent)
            {
              //Get draw char color
              crCharColor=(DWORD)-1;
              crIndentColor=(DWORD)-1;

              if ((aes.dwFlags & AESELT_COLUMNON) ?
                    AEC_IsCharInSelection(&ciCount) :
                    (AEC_IndexCompare(&ciCount, &aes.crSel.ciMin) >= 0 &&
                     AEC_IndexCompare(&ciCount, &aes.crSel.ciMax) < 0))
              {
                if (pscChar && pscChar->sclr.bSelColorEnable)
                  crCharColor=pscChar->sclr.dwSelColor;
                if (pscIndent && pscIndent->sclr.bSelColorEnable)
                  crIndentColor=pscIndent->sclr.dwSelColor;
                bCharInSel=TRUE;
              }
              else
              {
                if (pscChar && pscChar->sclr.bColorEnable)
                  crCharColor=pscChar->sclr.dwColor;
                if (pscIndent && pscIndent->sclr.bColorEnable)
                  crIndentColor=pscIndent->sclr.dwColor;
                bCharInSel=FALSE;
              }

              if (crCharColor != (DWORD)-1 || crIndentColor != (DWORD)-1)
              {
                SendMessage(pnt->hdr.hwndFrom, AEM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)&ciCount);

                if (pscIndent && crIndentColor != (DWORD)-1)
                {
                  //Draw indent line
                  HPEN hPen;
                  HPEN hPenOld;
                  POINT64 ptGlobal;
                  int nTop;
                  int nBottom;
                  int nLeft;
                  int i;

                  hPen=CreatePen(PS_SOLID, 0, crIndentColor);
                  hPenOld=(HPEN)SelectObject(pnt->hDC, hPen);

                  if (bDottedLine)
                  {
                    ptGlobal.x=pt.x;
                    ptGlobal.y=pt.y;
                    SendMessage(pnt->hdr.hwndFrom, AEM_CONVERTPOINT, AECPT_CLIENTTOGLOBAL, (LPARAM)&ptGlobal);

                    nTop=pt.y + !(ptGlobal.y % 2);
                  }
                  else
                    nTop=pt.y;
                  nBottom=pt.y + nCharHeight;

                  if (nChar >= 0)
                  {
                    //Inside line
                    nLeft=pt.x;

                    if (bDottedLine)
                    {
                      for (i=nTop; i < nBottom; i+=2)
                      {
                        //Draw dot
                        MoveToEx(pnt->hDC, nLeft, i, NULL);
                        LineTo(pnt->hDC, nLeft + 1, i + 1);
                      }
                    }
                    else
                    {
                      MoveToEx(pnt->hDC, nLeft, nTop, NULL);
                      LineTo(pnt->hDC, nLeft, nBottom);
                    }
                  }
                  else
                  {
                    //Outside line
                    if (nLineSpaces % nTabStopSize)
                      nLineSpaces+=nTabStopSize - nLineSpaces % nTabStopSize;

                    while (nLineSpaces < nMaxLineSpaces)
                    {
                      if (nLineSpaces)
                      {
                        nLeft=(int)((pt.x - ciCount.lpLine->nLineWidth) + nLineSpaces * nSpaceWidth);

                        if (bDottedLine)
                        {
                          for (i=nTop; i < nBottom; i+=2)
                          {
                            //Draw dot
                            MoveToEx(pnt->hDC, nLeft, i, NULL);
                            LineTo(pnt->hDC, nLeft + 1, i + 1);
                          }
                        }
                        else
                        {
                          MoveToEx(pnt->hDC, nLeft, nTop, NULL);
                          LineTo(pnt->hDC, nLeft, nBottom);
                        }
                      }
                      nLineSpaces+=nTabStopSize;
                    }
                  }
                  if (hPenOld) SelectObject(pnt->hDC, hPenOld);
                  if (hPen) DeleteObject(hPen);
                }
                if (pscChar && crCharColor != (DWORD)-1)
                {
                  if (pscChar->wszOldChar[0] == L'\t' && pscChar->wszNewChar[0] == L'\0')
                  {
                    //Draw tabulation arrow
                    HPEN hPen;
                    HPEN hPenOld;
                    int nTabWidth;
                    int nMargin=1;

                    hPen=CreatePen(PS_SOLID, 0, crCharColor);
                    hPenOld=(HPEN)SelectObject(pnt->hDC, hPen);
                    if (nTabWidth=(int)SendMessage(pnt->hdr.hwndFrom, AEM_GETCHARSIZE, AECS_INDEXWIDTH, (LPARAM)&ciCount))
                      nTabWidth-=nMargin * 2;

                    MoveToEx(pnt->hDC, pt.x + nMargin, pt.y + nCharHeightNoGap / 2, NULL);
                    LineTo(pnt->hDC, pt.x + nTabWidth, pt.y + nCharHeightNoGap / 2);
                    LineTo(pnt->hDC, pt.x + nTabWidth - nCharHeightNoGap / 4, (pt.y + nCharHeightNoGap / 2) - nCharHeightNoGap / 4);
                    MoveToEx(pnt->hDC, pt.x + nTabWidth, pt.y + nCharHeightNoGap / 2, NULL);
                    LineTo(pnt->hDC, pt.x + nTabWidth - nCharHeightNoGap / 4, (pt.y + nCharHeightNoGap / 2) + nCharHeightNoGap / 4);

                    if (hPenOld) SelectObject(pnt->hDC, hPenOld);
                    if (hPen) DeleteObject(hPen);
                  }
                  else if (pscChar->wszOldChar[0] == L'\r' && pscChar->wszNewChar[0] == L'\0')
                  {
                    //Draw new line identificator
                    HBRUSH hBrush;
                    HBRUSH hBrushOld;
                    HPEN hPen;
                    HPEN hPenOld;
                    const wchar_t *wpNewLine;
                    int nNewLineLen;
                    RECT rcBrush;
                    SIZE sizeNewLine;
                    int nStrMargin=1;

                    //New line string: "r", "n", "rn", "rrn".
                    nNewLineLen=GetNewLineString(ciCount.lpLine->nLineBreak, &wpNewLine);
                    GetTextExtentPoint32W(pnt->hDC, wpNewLine, nNewLineLen, &sizeNewLine);
                    pt.x+=nStrMargin;
                    sizeNewLine.cx+=nStrMargin * 2;

                    if (!(dwPaintOptions & PAINT_NONEWLINEDRAW) && bCharInSel)
                    {
                      //Selection rectangle.
                      rcBrush.left=pt.x;
                      rcBrush.top=pt.y;
                      rcBrush.right=pt.x + sizeNewLine.cx + nStrMargin;
                      rcBrush.bottom=pt.y + nCharHeight;
                      FillRect(pnt->hDC, &rcBrush, hbrSelBk);
                    }

                    //Draw rounded rectangle.
                    hPen=CreatePen(PS_SOLID, 0, crCharColor);
                    hPenOld=(HPEN)SelectObject(pnt->hDC, hPen);
                    hBrush=(HBRUSH)GetStockObject(HOLLOW_BRUSH);
                    hBrushOld=(HBRUSH)SelectObject(pnt->hDC, hBrush);
                    RoundRect(pnt->hDC, pt.x, pt.y, pt.x + sizeNewLine.cx, pt.y + nCharHeightNoGap, sizeNewLine.cx / 3, nCharHeightNoGap / 3);
                    if (hBrushOld) SelectObject(pnt->hDC, hBrushOld);
                    if (hPenOld) SelectObject(pnt->hDC, hPenOld);
                    if (hPen) DeleteObject(hPen);

                    //Draw new line string.
                    crTextColorPrev=SetTextColor(pnt->hDC, crCharColor);
                    TextOutW(pnt->hDC, pt.x + nStrMargin, pt.y, wpNewLine, nNewLineLen);
                    SetTextColor(pnt->hDC, crTextColorPrev);
                  }
                  else if (pscChar->wszOldChar[0] == L'\n' && pscChar->wszNewChar[0] == L'\0')
                  {
                    //Draw wrap line arrow
                    HPEN hPen;
                    HPEN hPenOld;
                    int nMarginX=1;
                    int nMarginY=3;

                    hPen=CreatePen(PS_SOLID, 0, crCharColor);
                    hPenOld=(HPEN)SelectObject(pnt->hDC, hPen);

                    if (!ciCount.lpLine->prev || ciCount.lpLine->prev->nLineBreak != AELB_WRAP)
                    {
                      //First wrap line
                      MoveToEx(pnt->hDC, (pt.x + nMarginX + 1) + nAveCharWidth / 2 - nCharHeightNoGap / 4, pt.y + nCharHeightNoGap / 4, NULL);
                      LineTo(pnt->hDC, (pt.x + nMarginX) + nAveCharWidth / 2, pt.y + nCharHeightNoGap / 4);
                      LineTo(pnt->hDC, (pt.x + nMarginX) + nAveCharWidth / 2, pt.y + nCharHeightNoGap - nMarginY);
                    }
                    else
                    {
                      MoveToEx(pnt->hDC, (pt.x + nMarginX) + nAveCharWidth / 2, pt.y + nMarginY, NULL);
                      LineTo(pnt->hDC, (pt.x + nMarginX) + nAveCharWidth / 2, pt.y + nCharHeightNoGap - nMarginY);
                    }
                    LineTo(pnt->hDC, (pt.x + nMarginX) + nAveCharWidth / 2 - nCharHeightNoGap / 4, (pt.y + nCharHeightNoGap - nMarginY) - nCharHeightNoGap / 4);
                    MoveToEx(pnt->hDC, (pt.x + nMarginX) + nAveCharWidth / 2, pt.y + nCharHeightNoGap - nMarginY, NULL);
                    LineTo(pnt->hDC, (pt.x + nMarginX) + nAveCharWidth / 2 + nCharHeightNoGap / 4, (pt.y + nCharHeightNoGap - nMarginY) - nCharHeightNoGap / 4);

                    if (hPenOld) SelectObject(pnt->hDC, hPenOld);
                    if (hPen) DeleteObject(hPen);
                  }
                  else
                  {
                    //Erase old
                    if (nChar >= 0 &&
                        nChar != L' ' &&
                        nChar != L'\t')
                    {
                      AECHARCOLORS aecc;
                      COLORREF crOldBk=0;
                      HFONT hCharFont;
                      HFONT hOldFont;

                      aecc.dwFlags=0;
                      GetCharColor(pnt->hdr.hwndFrom, nOffset, &aecc);

                      if (bClearType)
                      {
                        SetBkMode(pnt->hDC, nBkModePrev);
                        crOldBk=SetBkColor(pnt->hDC, aecc.crBk);
                      }
                      crTextColorPrev=SetTextColor(pnt->hDC, aecc.crBk);
                      hCharFont=(HFONT)SendMessage(pnt->hdr.hwndFrom, AEM_GETFONT, aecc.dwFontStyle, 0);
                      hOldFont=(HFONT)SelectObject(pnt->hDC, hCharFont);

                      TextOutW(pnt->hDC, pt.x, pt.y, pscChar->wszOldChar, 1);

                      SelectObject(pnt->hDC, hOldFont);
                      SetTextColor(pnt->hDC, crTextColorPrev);
                      if (bClearType)
                      {
                        SetBkColor(pnt->hDC, crOldBk);
                        SetBkMode(pnt->hDC, TRANSPARENT);
                      }
                    }

                    //Draw new
                    crTextColorPrev=SetTextColor(pnt->hDC, crCharColor);
                    TextOutW(pnt->hDC, pt.x, pt.y, pscChar->wszNewChar, 1);
                    SetTextColor(pnt->hDC, crTextColorPrev);
                  }
                }
              }
            }

            //Next char
            AEC_IndexInc(&ciCount);
            if (ciCount.nCharInLine > ciCount.lpLine->nLineLen)
              if (!AEC_NextLine(&ciCount))
                break;

            ++nOffset;
          }
          while (AEC_IndexCompare(&ciCount, &pnt->ciMaxDraw) <= 0);

          if (nNextLine <= pnt->ciMaxDraw.nLine)
            nNextLineSpaces=-1;
          nPrevLineSpaces=nLineSpaces;
          SetBkMode(pnt->hDC, nBkModePrev);
        }
        else if (pnt->dwType == AEPNT_END)
        {
          int nInitBottomStartLine=pnt->ciMaxDraw.nLine + 1;
          int nInitBottomEndLine=nNextLine - 1;

          if (hbrSelBk) DeleteObject(hbrSelBk);

          if (!bIntCall && ((nInitTopEndLine >= 0 && nInitTopStartLine <= nInitTopEndLine) ||
                            (nInitBottomEndLine >= 0 && nInitBottomStartLine <= nInitBottomEndLine)))
          {
            bIntCall=TRUE;

            if (nInitTopEndLine >= 0 && nInitTopStartLine <= nInitTopEndLine)
              SendMessage(pnt->hdr.hwndFrom, AEM_REDRAWLINERANGE, (WPARAM)nInitTopStartLine, (LPARAM)nInitTopEndLine);
            if (nInitBottomEndLine >= 0 && nInitBottomStartLine <= nInitBottomEndLine)
              SendMessage(pnt->hdr.hwndFrom, AEM_REDRAWLINERANGE, (WPARAM)nInitBottomStartLine, (LPARAM)nInitBottomEndLine);
            UpdateWindow(pnt->hdr.hwndFrom);
          }
          bIntCall=FALSE;
        }
      }
    }
  }
  return 0;
}

BOOL GetLineSpaces(AECHARINDEX *ciMinDraw, int nTabStopSize, INT_PTR *lpnLineSpaces)
{
  AECHARINDEX ciCount;
  INT_PTR nLineSpaces=0;

  ciCount.nLine=ciMinDraw->nLine;
  ciCount.lpLine=ciMinDraw->lpLine;
  ciCount.nCharInLine=0;

  while (ciCount.nCharInLine < ciMinDraw->nCharInLine)
  {
    if (ciCount.lpLine->wpLine[ciCount.nCharInLine] == L' ')
      ++nLineSpaces;
    else if (ciCount.lpLine->wpLine[ciCount.nCharInLine] == L'\t')
      nLineSpaces+=nTabStopSize - nLineSpaces % nTabStopSize;
    else
    {
      *lpnLineSpaces=nLineSpaces;
      return FALSE;
    }
    AEC_IndexInc(&ciCount);
  }
  *lpnLineSpaces=nLineSpaces;
  return TRUE;
}

BOOL GetCharColor(HWND hWndEdit, INT_PTR nCharOffset, AECHARCOLORS *aecc)
{
  AECHARINDEX ciCharIndex;

  SendMessage(hWndEdit, AEM_RICHOFFSETTOINDEX, (WPARAM)nCharOffset, (LPARAM)&ciCharIndex);
  return (BOOL)SendMessage(hWndEdit, AEM_GETCHARCOLORS, (WPARAM)&ciCharIndex, (LPARAM)aecc);
}

COLORREF GetColorValueFromStrA(char *pColor)
{
  COLORREF crColor;

  crColor=(COLORREF)hex2decA(pColor, -1);
  crColor=RGB(GetBValue(crColor), GetGValue(crColor), GetRValue(crColor));
  return crColor;
}

COLORREF GetColorValueFromStrW(wchar_t *wpColor)
{
  COLORREF crColor;

  crColor=(COLORREF)hex2decW(wpColor, -1);
  crColor=RGB(GetBValue(crColor), GetGValue(crColor), GetRValue(crColor));
  return crColor;
}

char* GetColorStrFromValueA(COLORREF crColor, char *szColor)
{
  wsprintfA(szColor, "%02X%02X%02X", GetRValue(crColor), GetGValue(crColor), GetBValue(crColor));
  return szColor;
}

wchar_t* GetColorStrFromValueW(COLORREF crColor, wchar_t *wszColor)
{
  xprintfW(wszColor, L"%02X%02X%02X", GetRValue(crColor), GetGValue(crColor), GetBValue(crColor));
  return wszColor;
}

int GetNewLineString(int nNewLine, const wchar_t **wpNewLine)
{
  if (nNewLine == AELB_R)
  {
    if (wpNewLine) *wpNewLine=L"r";
    return 1;
  }
  else if (nNewLine == AELB_N)
  {
    if (wpNewLine) *wpNewLine=L"n";
    return 1;
  }
  else if (nNewLine == AELB_RN)
  {
    if (wpNewLine) *wpNewLine=L"rn";
    return 2;
  }
  else if (nNewLine == AELB_RRN)
  {
    if (wpNewLine) *wpNewLine=L"rrn";
    return 3;
  }
  if (wpNewLine) *wpNewLine=L"rn";
  return 2;
}

void UpdateEdit(HWND hWnd, DWORD dwFlags)
{
  RECT rc;
  BOOL bErase=FALSE;

  if (dwFlags & UE_ERASE)
    bErase=TRUE;

  if (dwFlags & UE_ALLRECT)
  {
    InvalidateRect(hWnd, NULL, bErase);
  }
  else if (dwFlags & UE_DRAWRECT)
  {
    SendMessage(hWnd, EM_GETRECT, 0, (LPARAM)&rc);
    InvalidateRect(hWnd, &rc, bErase);
  }
  else if (dwFlags & UE_FIRSTPIXEL)
  {
    rc.left=0;
    rc.top=0;
    rc.right=1;
    rc.bottom=1;
    InvalidateRect(hWnd, &rc, bErase);
  }
  if (dwFlags & UE_UPDATEWINDOW)
    UpdateWindow(hWnd);
}

void UpdateEditAll(DWORD dwFlags)
{
  EDITINFO ei;
  BOOL bMaximized=TRUE;

  if (nMDI == WMD_MDI)
    SendMessage(hMdiClient, WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized);

  if (bMaximized)
  {
    if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)NULL, (LPARAM)&ei))
    {
      if (ei.hWndMaster)
      {
        UpdateEdit(ei.hWndMaster, dwFlags);
        if (ei.hWndClone1)
          UpdateEdit(ei.hWndClone1, dwFlags);
        if (ei.hWndClone2)
          UpdateEdit(ei.hWndClone2, dwFlags);
        if (ei.hWndClone3)
          UpdateEdit(ei.hWndClone3, dwFlags);
      }
      else UpdateEdit(ei.hWndEdit, dwFlags);
    }
  }
  else
  {
    //nMDI == WMD_MDI in non-maximized state
    FRAMEDATA *lpFrameCount=(FRAMEDATA *)SendMessage(hMainWnd, AKD_FRAMEFIND, FWF_CURRENT, 0);
    FRAMEDATA *lpFrameInit=lpFrameCount;

    while (lpFrameCount)
    {
      if (lpFrameCount->ei.hWndMaster)
      {
        UpdateEdit(lpFrameCount->ei.hWndMaster, dwFlags);
        if (lpFrameCount->ei.hWndClone1)
          UpdateEdit(lpFrameCount->ei.hWndClone1, dwFlags);
        if (lpFrameCount->ei.hWndClone2)
          UpdateEdit(lpFrameCount->ei.hWndClone2, dwFlags);
        if (lpFrameCount->ei.hWndClone3)
          UpdateEdit(lpFrameCount->ei.hWndClone3, dwFlags);
      }
      else UpdateEdit(lpFrameCount->ei.hWndEdit, dwFlags);

      if (lpFrameInit == (lpFrameCount=(FRAMEDATA *)SendMessage(hMainWnd, AKD_FRAMEFIND, FWF_NEXT, (LPARAM)lpFrameCount)))
        break;
    }
  }
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
    WideOption(hOptions, L"SpecialChars", PO_BINARY, (LPBYTE)&schArray, sizeof(schArray));
    WideOption(hOptions, L"DottedLine", PO_DWORD, (LPBYTE)&bDottedLine, sizeof(DWORD));

    SendMessage(hMainWnd, AKD_ENDOPTIONS, (WPARAM)hOptions, 0);
  }
}

void SaveOptions(DWORD dwFlags)
{
  HANDLE hOptions;

  if (hOptions=(HANDLE)SendMessage(hMainWnd, AKD_BEGINOPTIONSW, POB_SAVE, (LPARAM)wszPluginName))
  {
    WideOption(hOptions, L"SpecialChars", PO_BINARY, (LPBYTE)&schArray, sizeof(schArray));
    WideOption(hOptions, L"DottedLine", PO_DWORD, (LPBYTE)&bDottedLine, sizeof(DWORD));

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
    if (nStringID == STRID_SPACE)
      return L"\x041F\x0440\x043E\x0431\x0435\x043B";
    if (nStringID == STRID_TAB)
      return L"\x0422\x0430\x0431\x0443\x043B\x044F\x0446\x0438\x044F";
    if (nStringID == STRID_NEWLINE)
      return L"\x041D\x043E\x0432\x0430\x044F\x0020\x0441\x0442\x0440\x043E\x043A\x0430";
    if (nStringID == STRID_VERTICALTAB)
      return L"\x0412\x0435\x0440\x0442\x0438\x043A\x0430\x043B\x044C\x043D\x0430\x044F\x0020\x0442\x0430\x0431\x0443\x043B\x044F\x0446\x0438\x044F";
    if (nStringID == STRID_FORMFEED)
      return L"\x041F\x0440\x043E\x0433\x043E\x043D\x0020\x043B\x0438\x0441\x0442\x0430";
    if (nStringID == STRID_NULL)
      return L"\x041D\x0443\x043B\x0435\x0432\x043E\x0439\x0020\x0441\x0438\x043C\x0432\x043E\x043B";
    if (nStringID == STRID_WRAP)
      return L"\x041F\x0435\x0440\x0435\x043D\x043E\x0441";
    if (nStringID == STRID_INDENTLINE)
      return L"\x041B\x0438\x043D\x0438\x044F\x0020\x043E\x0442\x0441\x0442\x0443\x043F\x0430";
    if (nStringID == STRID_COLOR)
      return L"\x0426\x0432\x0435\x0442";
    if (nStringID == STRID_SELCOLOR)
      return L"\x0426\x0432\x0435\x0442\x0020\x0432\x0020\x0432\x044B\x0434\x0435\x043B\x0435\x043D\x0438\x0438";
    if (nStringID == STRID_SYMBOL)
      return L"\x0421\x0438\x043C\x0432\x043E\x043B";
    if (nStringID == STRID_DOTTEDLINE)
      return L"\x041F\x0443\x043D\x043A\x0442\x0438\x0440\x043D\x0430\x044F\x0020\x043B\x0438\x043D\x0438\x044F";
    if (nStringID == STRID_PLUGIN)
      return L"%s \x043F\x043B\x0430\x0433\x0438\x043D";
    if (nStringID == STRID_OK)
      return L"\x004F\x004B";
    if (nStringID == STRID_CANCEL)
      return L"\x041E\x0442\x043C\x0435\x043D\x0430";
  }
  else
  {
    if (nStringID == STRID_SPACE)
      return L"Space";
    if (nStringID == STRID_TAB)
      return L"Tabulation";
    if (nStringID == STRID_NEWLINE)
      return L"New line";
    if (nStringID == STRID_VERTICALTAB)
      return L"Vertical tabulation";
    if (nStringID == STRID_FORMFEED)
      return L"Form-feed";
    if (nStringID == STRID_NULL)
      return L"Null";
    if (nStringID == STRID_WRAP)
      return L"Wrap";
    if (nStringID == STRID_INDENTLINE)
      return L"Indent line";
    if (nStringID == STRID_COLOR)
      return L"Color";
    if (nStringID == STRID_SELCOLOR)
      return L"Color in selection";
    if (nStringID == STRID_SYMBOL)
      return L"Symbol";
    if (nStringID == STRID_DOTTEDLINE)
      return L"Dotted line";
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
  bOldWindows=pd->bOldWindows;
  bOldRichEdit=pd->bOldRichEdit;
  bAkelEdit=pd->bAkelEdit;
  nMDI=pd->nMDI;
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

  dwPaintOptions=(DWORD)SendMessage(hMainWnd, AKD_GETMAININFO, MI_PAINTOPTIONS, 0);

  //Default characters
  {
    SPECIALCOLOR scrDefault;

    //Default color
    scrDefault.dwColor=RGB(0x33, 0x33, 0xCC);
    scrDefault.dwSelColor=GetSysColor(COLOR_HIGHLIGHTTEXT);
    scrDefault.bColorEnable=TRUE;
    scrDefault.bSelColorEnable=TRUE;

    xstrcpyW(schArray[SC_SPACE].wszNewChar, L"\x00B7");
    xstrcpyW(schArray[SC_SPACE].wszOldChar, L" ");
    schArray[SC_SPACE].sclr=scrDefault;

    xstrcpyW(schArray[SC_TAB].wszNewChar, L"\x0000");
    xstrcpyW(schArray[SC_TAB].wszOldChar, L"\t");
    schArray[SC_TAB].sclr=scrDefault;

    xstrcpyW(schArray[SC_NEWLINE].wszNewChar, L"\x0000"); //or \x00B6
    xstrcpyW(schArray[SC_NEWLINE].wszOldChar, L"\r");
    schArray[SC_NEWLINE].sclr=scrDefault;

    xstrcpyW(schArray[SC_VERTICALTAB].wszNewChar, L"\x00A6");
    xstrcpyW(schArray[SC_VERTICALTAB].wszOldChar, L"\v");
    schArray[SC_VERTICALTAB].sclr=scrDefault;

    xstrcpyW(schArray[SC_FORMFEED].wszNewChar, L"\x00A7");
    xstrcpyW(schArray[SC_FORMFEED].wszOldChar, L"\f");
    schArray[SC_FORMFEED].sclr=scrDefault;

    xstrcpyW(schArray[SC_NULL].wszNewChar, L"\x00A4");
    xstrcpyW(schArray[SC_NULL].wszOldChar, L"\0");
    schArray[SC_NULL].sclr=scrDefault;

    xstrcpyW(schArray[SC_WRAP].wszNewChar, L"\x0000"); //or \x002F
    xstrcpyW(schArray[SC_WRAP].wszOldChar, L"\n");
    schArray[SC_WRAP].sclr=scrDefault;
    schArray[SC_WRAP].sclr.dwSelColor=schArray[SC_WRAP].sclr.dwColor;

    xstrcpyW(schArray[SC_INDENTLINE].wszNewChar, L"\x0000");
    xstrcpyW(schArray[SC_INDENTLINE].wszOldChar, L"\0");
    schArray[SC_INDENTLINE].sclr=scrDefault;
  }
  ReadOptions(0);

  ////Make sure that we have null-terminated strings
  //{
  //  int i;
  //
  //  for (i=0; i < SC_MAX; ++i)
  //  {
  //    schArray[i].wszNewChar[1]=L'\0';
  //    schArray[i].wszOldChar[1]=L'\0';
  //  }
  //}
}

void InitMain()
{
  bInitMain=TRUE;

  //SubClass
  NewMainProcData=NULL;
  SendMessage(hMainWnd, AKD_SETMAINPROC, (WPARAM)NewMainProc, (LPARAM)&NewMainProcData);

  if (nMDI == WMD_MDI)
  {
    NewFrameProcData=NULL;
    SendMessage(hMainWnd, AKD_SETFRAMEPROC, (WPARAM)NewFrameProc, (LPARAM)&NewFrameProcData);
  }

  NewEditProcData=NULL;
  SendMessage(hMainWnd, AKD_SETEDITPROC, (WPARAM)NewEditProc, (LPARAM)&NewEditProcData);
}

void UninitMain()
{
  bInitMain=FALSE;

  //Save options
  if (dwSaveFlags)
  {
    SaveOptions(dwSaveFlags);
    dwSaveFlags=0;
  }

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
  if (NewEditProcData)
  {
    SendMessage(hMainWnd, AKD_SETEDITPROC, (WPARAM)NULL, (LPARAM)&NewEditProcData);
    NewEditProcData=NULL;
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
  }
  else if (fdwReason == DLL_THREAD_DETACH)
  {
  }
  else if (fdwReason == DLL_PROCESS_DETACH)
  {
  }
  return TRUE;
}
