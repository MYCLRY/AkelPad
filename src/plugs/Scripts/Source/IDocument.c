#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <activscp.h>
#include <richedit.h>
#include "Scripts.h"


//Global variables
CALLBACKSTACK g_hSubclassCallbackStack={0};
CALLBACKSTACK g_hHookCallbackStack={0};
ITypeInfo *g_DocumentTypeInfo=NULL;
WNDPROCDATA *g_lpSubclassMainProc=NULL;
WNDPROCDATA *g_lpSubclassEditProc=NULL;
WNDPROCDATA *g_lpSubclassFrameProc=NULL;

const IDocumentVtbl MyIDocumentVtbl={
  Document_QueryInterface,
  Document_AddRef,
  Document_Release,
  Document_GetTypeInfoCount,
  Document_GetTypeInfo,
  Document_GetIDsOfNames,
  Document_Invoke,
  Document_Constants,
  Document_GetMainWnd,
  Document_GetAkelDir,
  Document_GetInstanceExe,
  Document_GetInstanceDll,
  Document_GetLangId,
  Document_IsOldWindows,
  Document_IsOldRichEdit,
  Document_IsOldComctl32,
  Document_IsAkelEdit,
  Document_IsMDI,
  Document_GetEditWnd,
  Document_SetEditWnd,
  Document_GetEditDoc,
  Document_GetEditFile,
  Document_GetEditCodePage,
  Document_GetEditBOM,
  Document_GetEditNewLine,
  Document_GetEditModified,
  Document_GetEditReadOnly,
  Document_SetFrameInfo,
  Document_SendMessage,
  Document_MessageBox,
  Document_InputBox,
  Document_GetSelStart,
  Document_GetSelEnd,
  Document_SetSel,
  Document_GetSelText,
  Document_GetTextRange,
  Document_ReplaceSel,
  Document_TextFind,
  Document_TextReplace,
  Document_GetClipboardText,
  Document_SetClipboardText,
  Document_IsPluginRunning,
  Document_Call,
  Document_CallA,
  Document_CallW,
  Document_CallEx,
  Document_CallExA,
  Document_CallExW,
  Document_Exec,
  Document_Command,
  Document_Font,
  Document_Recode,
  Document_Include,
  Document_IsInclude,
  Document_OpenFile,
  Document_ReadFile,
  Document_SaveFile,
  Document_ScriptSettings,
  Document_SystemFunction,
  Document_MemAlloc,
  Document_MemCopy,
  Document_MemRead,
  Document_MemStrPtr,
  Document_MemFree,
  Document_Debug,
  Document_VarType,
  Document_GetArgLine,
  Document_GetArgValue,
  Document_WindowRegisterClass,
  Document_WindowUnregisterClass,
  Document_WindowRegisterDialog,
  Document_WindowUnregisterDialog,
  Document_WindowGetMessage,
  Document_WindowSubClass,
  Document_WindowNextProc,
  Document_WindowNoNextProc,
  Document_WindowUnsubClass,
  Document_ThreadHook,
  Document_ThreadUnhook,
  Document_ScriptNoMutex,
  Document_ScriptHandle
};


//// IDocument

HRESULT STDMETHODCALLTYPE Document_QueryInterface(IDocument *this, REFIID vTableGuid, void **ppv)
{
  if (AKD_IsEqualIID(vTableGuid, &IID_IUnknown) || AKD_IsEqualIID(vTableGuid, &IID_IDocument) || AKD_IsEqualIID(vTableGuid, &IID_IDispatch))
  {
    *ppv=this;
    this->lpVtbl->AddRef(this);
    return NOERROR;
  }
  *ppv=NULL;
  return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE Document_AddRef(IDocument *this)
{
  return ++((IRealDocument *)this)->dwCount;
}

ULONG STDMETHODCALLTYPE Document_Release(IDocument *this)
{
  if (--((IRealDocument *)this)->dwCount == 0)
  {
    GlobalFree(this);
    InterlockedDecrement(&g_nObjs);
    return 0;
  }
  return ((IRealDocument *)this)->dwCount;
}


//// IDispatch

HRESULT STDMETHODCALLTYPE Document_GetTypeInfoCount(IDocument *this, UINT *pCount)
{
  *pCount=1;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE Document_GetTypeInfo(IDocument *this, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo)
{
  HRESULT hr;

  *pTypeInfo=NULL;

  if (itinfo)
  {
    hr=ResultFromScode(DISP_E_BADINDEX);
  }
  else if (g_DocumentTypeInfo)
  {
    g_DocumentTypeInfo->lpVtbl->AddRef(g_DocumentTypeInfo);
    hr=S_OK;
  }
  else
  {
    hr=LoadTypeInfoFromFile(NULL, NULL);
  }
  if (hr == S_OK) *pTypeInfo=g_DocumentTypeInfo;

  return hr;
}

HRESULT STDMETHODCALLTYPE Document_GetIDsOfNames(IDocument *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
  if (!g_DocumentTypeInfo)
  {
    HRESULT hr;

    if ((hr=LoadTypeInfoFromFile(NULL, NULL)) != S_OK)
      return hr;
  }
  return DispGetIDsOfNames(g_DocumentTypeInfo, rgszNames, cNames, rgdispid);
}

HRESULT STDMETHODCALLTYPE Document_Invoke(IDocument *this, DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
  if (!AKD_IsEqualIID(riid, &IID_NULL))
    return DISP_E_UNKNOWNINTERFACE;

  if (!g_DocumentTypeInfo)
  {
    HRESULT hr;

    if ((hr=LoadTypeInfoFromFile(NULL, NULL)) != S_OK)
      return hr;
  }
  return DispInvoke(this, g_DocumentTypeInfo, dispid, wFlags, params, result, pexcepinfo, puArgErr);
}


//// IDocument methods

HRESULT STDMETHODCALLTYPE Document_Constants(IDocument *this, IDispatch **objConstants)
{
  return Class_CreateInstance(NULL, NULL, &IID_IConstants, (void **)objConstants);
}

HRESULT STDMETHODCALLTYPE Document_GetMainWnd(IDocument *this, HWND *hWnd)
{
  *hWnd=hMainWnd;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetAkelDir(IDocument *this, int nSubDir, BSTR *wpDir)
{
  HRESULT hr=NOERROR;

  if (nSubDir == ADTYPE_ROOT)
    xprintfW(wszBuffer, L"%s", wszAkelPadDir);
  else if (nSubDir == ADTYPE_AKELFILES)
    xprintfW(wszBuffer, L"%s\\AkelFiles", wszAkelPadDir);
  else if (nSubDir == ADTYPE_DOCS)
    xprintfW(wszBuffer, L"%s\\AkelFiles\\Docs", wszAkelPadDir);
  else if (nSubDir == ADTYPE_LANGS)
    xprintfW(wszBuffer, L"%s\\AkelFiles\\Langs", wszAkelPadDir);
  else if (nSubDir == ADTYPE_PLUGS)
    xprintfW(wszBuffer, L"%s\\AkelFiles\\Plugs", wszAkelPadDir);
  else if (nSubDir == ADTYPE_SCRIPTS)
    xprintfW(wszBuffer, L"%s\\AkelFiles\\Plugs\\Scripts", wszAkelPadDir);
  else if (nSubDir == ADTYPE_INCLUDE)
    xprintfW(wszBuffer, L"%s\\AkelFiles\\Plugs\\Scripts\\Include", wszAkelPadDir);
  else
    wszBuffer[0]=L'\0';

  if (!(*wpDir=SysAllocString(wszBuffer)))
    hr=E_OUTOFMEMORY;

  return hr;
}

HRESULT STDMETHODCALLTYPE Document_GetInstanceExe(IDocument *this, INT_PTR *hInstance)
{
  *hInstance=(INT_PTR)hInstanceEXE;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetInstanceDll(IDocument *this, INT_PTR *hInstance)
{
  *hInstance=(INT_PTR)hInstanceDLL;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetLangId(IDocument *this, int nType, int *nLangModule)
{
  if (nType == LANGID_FULL)
    *nLangModule=(int)wLangModule;
  else if (nType == LANGID_PRIMARY)
    *nLangModule=(int)PRIMARYLANGID(wLangModule);
  else if (nType == LANGID_SUB)
    *nLangModule=(int)SUBLANGID(wLangModule);
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_IsOldWindows(IDocument *this, BOOL *bIsOld)
{
  *bIsOld=bOldWindows;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_IsOldRichEdit(IDocument *this, BOOL *bIsOld)
{
  *bIsOld=bOldRichEdit;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_IsOldComctl32(IDocument *this, BOOL *bIsOld)
{
  *bIsOld=bOldComctl32;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_IsAkelEdit(IDocument *this, HWND hWnd, int *nIsAkelEdit)
{
  if (hWnd)
  {
    char szName[MAX_PATH];

    if (GetClassNameA(hWnd, szName, MAX_PATH) &&
        (xstrcmpinA("AkelEdit", szName, (DWORD)-1) ||
         xstrcmpinA("RichEdit20", szName, (DWORD)-1)))
    {
      if (SendMessage(hMainWnd, AKD_FRAMEFIND, FWF_BYEDITWINDOW, (LPARAM)hWnd))
        *nIsAkelEdit=ISAEW_PROGRAM;
      else
        *nIsAkelEdit=ISAEW_PLUGIN;
    }
    else *nIsAkelEdit=ISAEW_NONE;
  }
  else *nIsAkelEdit=bAkelEdit;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_IsMDI(IDocument *this, int *nIsMDI)
{
  *nIsMDI=nMDI;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetEditWnd(IDocument *this, HWND *hWnd)
{
  *hWnd=GetCurEdit(this);
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_SetEditWnd(IDocument *this, HWND hWnd, HWND *hWndResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  int nIsAkelEdit;

  Document_IsAkelEdit(this, hWnd, &nIsAkelEdit);

  if (nIsAkelEdit == ISAEW_PLUGIN)
    lpScriptThread->hWndPluginEdit=hWnd;
  else
    lpScriptThread->hWndPluginEdit=NULL;
  *hWndResult=lpScriptThread->hWndPluginEdit;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetEditDoc(IDocument *this, INT_PTR *hDoc)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;

  if (lpScriptThread->hWndPluginEdit)
    *hDoc=SendMessage(lpScriptThread->hWndPluginEdit, AEM_GETDOCUMENT, 0, 0);
  else
    *hDoc=SendMessage(hMainWnd, AKD_GETFRAMEINFO, FI_DOCEDIT, (LPARAM)NULL);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetEditFile(IDocument *this, HWND hWnd, BSTR *wpFile)
{
  EDITINFO ei;
  HRESULT hr=NOERROR;

  *wpFile=NULL;
  if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)hWnd, (LPARAM)&ei))
  {
    if (!(*wpFile=SysAllocString(ei.wszFile)))
      hr=E_OUTOFMEMORY;
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_GetEditCodePage(IDocument *this, HWND hWnd, int *nCodePage)
{
  EDITINFO ei;

  if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)hWnd, (LPARAM)&ei))
    *nCodePage=ei.nCodePage;
  else
    *nCodePage=0;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetEditBOM(IDocument *this, HWND hWnd, BOOL *bBOM)
{
  EDITINFO ei;

  if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)hWnd, (LPARAM)&ei))
    *bBOM=ei.bBOM;
  else
    *bBOM=0;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetEditNewLine(IDocument *this, HWND hWnd, int *nNewLine)
{
  EDITINFO ei;

  if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)hWnd, (LPARAM)&ei))
    *nNewLine=ei.nNewLine;
  else
    *nNewLine=0;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetEditModified(IDocument *this, HWND hWnd, BOOL *bModified)
{
  EDITINFO ei;

  if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)hWnd, (LPARAM)&ei))
    *bModified=ei.bModified;
  else
    *bModified=0;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetEditReadOnly(IDocument *this, HWND hWnd, BOOL *bReadOnly)
{
  EDITINFO ei;

  if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)hWnd, (LPARAM)&ei))
    *bReadOnly=ei.bReadOnly;
  else
    *bReadOnly=0;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_SetFrameInfo(IDocument *this, FRAMEDATA *lpFrame, int nType, UINT_PTR dwData, BOOL *bResult)
{
  FRAMEINFO fi;

  fi.nType=nType;
  fi.dwData=dwData;
  *bResult=(BOOL)SendMessage(hMainWnd, AKD_SETFRAMEINFO, (WPARAM)&fi, (LPARAM)lpFrame);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_SendMessage(IDocument *this, HWND hWnd, UINT uMsg, VARIANT vtwParam, VARIANT vtlParam, INT_PTR *nResult)
{
  VARIANT *pvtwParam=&vtwParam;
  VARIANT *pvtlParam=&vtlParam;
  WPARAM wParam;
  LPARAM lParam;

  if (pvtwParam->vt == (VT_VARIANT|VT_BYREF))
    pvtwParam=pvtwParam->pvarVal;
  wParam=GetVariantValue(pvtwParam, bOldWindows);

  if (pvtlParam->vt == (VT_VARIANT|VT_BYREF))
    pvtlParam=pvtlParam->pvarVal;
  lParam=GetVariantValue(pvtlParam, bOldWindows);

  if (bOldWindows)
  {
    *nResult=SendMessageA(hWnd, uMsg, wParam, lParam);
    if (pvtwParam->vt == VT_BSTR) GlobalFree((HGLOBAL)wParam);
    if (pvtlParam->vt == VT_BSTR) GlobalFree((HGLOBAL)lParam);
  }
  else *nResult=SendMessageW(hWnd, uMsg, wParam, lParam);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_MessageBox(IDocument *this, HWND hWnd, BSTR pText, BSTR pCaption, UINT uType, SAFEARRAY **psa, int *nResult)
{
  DIALOGMESSAGEBOX dmb;
  BUTTONMESSAGEBOX *bmb;

  if (bmb=FillButtonsArray(*psa, &dmb.hIcon))
  {
    dmb.hWndParent=hWnd;
    dmb.wpText=pText;
    dmb.wpCaption=pCaption;
    dmb.uType=uType;
    dmb.bmb=bmb;
    *nResult=(int)SendMessage(hMainWnd, AKD_MESSAGEBOX, 0, (LPARAM)&dmb);

    GlobalFree((HGLOBAL)bmb);
  }
  else *nResult=MessageBoxW(hWnd, pText, pCaption, uType);

  return NOERROR;
}

BUTTONMESSAGEBOX* FillButtonsArray(SAFEARRAY *psa, HICON *hIcon)
{
  BUTTONMESSAGEBOX *bmb=NULL;
  BUTTONMESSAGEBOX *bmbNext;
  VARIANT *pvtParameter;
  unsigned char *lpData;
  DWORD dwElement;
  DWORD dwElementSum;

  lpData=(unsigned char *)(psa->pvData);
  dwElementSum=psa->rgsabound[0].cElements;
  if (!dwElementSum)
    return 0;

  //DIALOGMESSAGEBOX.hIcon
  pvtParameter=(VARIANT *)lpData;
  if (pvtParameter->vt == (VT_VARIANT|VT_BYREF))
    pvtParameter=pvtParameter->pvarVal;
  *hIcon=(HICON)GetVariantInt(pvtParameter);
  lpData+=sizeof(VARIANT);
  --dwElementSum;

  //BUTTONMESSAGEBOX array
  dwElementSum=(dwElementSum / 3) * 3;
  if (!dwElementSum)
    return 0;

  if (bmb=GlobalAlloc(GPTR, (dwElementSum / 3 + 1) * sizeof(BUTTONMESSAGEBOX)))
  {
    bmbNext=bmb;
    dwElement=0;

    while (dwElement < dwElementSum)
    {
      //BUTTONMESSAGEBOX.nButtonControlID
      pvtParameter=(VARIANT *)(lpData + dwElement * sizeof(VARIANT));
      if (pvtParameter->vt == (VT_VARIANT|VT_BYREF))
        pvtParameter=pvtParameter->pvarVal;
      bmbNext->nButtonControlID=(int)GetVariantInt(pvtParameter);
      ++dwElement;

      //BUTTONMESSAGEBOX.wpButtonStr
      pvtParameter=(VARIANT *)(lpData + dwElement * sizeof(VARIANT));
      if (pvtParameter->vt == (VT_VARIANT|VT_BYREF))
        pvtParameter=pvtParameter->pvarVal;
      if (pvtParameter->vt == VT_BSTR)
        bmbNext->wpButtonStr=pvtParameter->bstrVal;
      else
        bmbNext->wpButtonStr=(wchar_t *)GetVariantInt(pvtParameter);
      ++dwElement;

      //BUTTONMESSAGEBOX.dwFlags
      pvtParameter=(VARIANT *)(lpData + dwElement * sizeof(VARIANT));
      if (pvtParameter->vt == (VT_VARIANT|VT_BYREF))
        pvtParameter=pvtParameter->pvarVal;
      bmbNext->dwFlags=(DWORD)GetVariantInt(pvtParameter);
      ++dwElement;

      ++bmbNext;
    }
  }
  return bmb;
}

HRESULT STDMETHODCALLTYPE Document_InputBox(IDocument *this, HWND hWnd, BSTR wpCaption, BSTR wpLabel, BSTR wpEdit, VARIANT *vtResult)
{
  INPUTBOX db;

  VariantInit(vtResult);

  db.wpCaption=wpCaption;
  db.wpLabel=wpLabel;
  db.wpEdit=wpEdit;
  db.vtResult=vtResult;
  db.hr=NOERROR;

  DialogBoxParamWide(hInstanceDLL, MAKEINTRESOURCEW(IDD_INPUTBOX), hWnd, (DLGPROC)InputBoxProc, (LPARAM)&db);

  return db.hr;
}

BOOL CALLBACK InputBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static HWND hWndLabel;
  static HWND hWndEditBox;
  static HWND hWndOK;
  static HWND hWndCancel;
  static INPUTBOX *db;

  if (uMsg == WM_INITDIALOG)
  {
    db=(INPUTBOX *)lParam;

    SendMessage(hDlg, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)g_hPluginIcon);
    hWndLabel=GetDlgItem(hDlg, IDC_INPUTBOX_LABEL);
    hWndEditBox=GetDlgItem(hDlg, IDC_INPUTBOX_EDIT);
    hWndOK=GetDlgItem(hDlg, IDOK);
    hWndCancel=GetDlgItem(hDlg, IDCANCEL);

    SetDlgItemTextWide(hDlg, IDC_INPUTBOX_LABEL, db->wpLabel);
    SetDlgItemTextWide(hDlg, IDOK, GetLangStringW(wLangModule, STRID_OK));
    SetDlgItemTextWide(hDlg, IDCANCEL, GetLangStringW(wLangModule, STRID_CANCEL));
    SetWindowTextWide(hDlg, db->wpCaption);
    SetWindowTextWide(hWndEditBox, db->wpEdit);

    //Multiline label
    {
      RECT rc;
      wchar_t *wpCount;
      int nHeight=0;
      int nLines=0;

      for (wpCount=db->wpLabel; *wpCount; ++wpCount)
      {
        if (*wpCount == L'\n')
          ++nLines;
      }
      if (nLines > 0)
      {
        GetWindowPos(hWndLabel, hDlg, &rc);
        nHeight=rc.bottom * nLines;
        SetWindowPos(hWndLabel, 0, 0, 0, rc.right, rc.bottom + nHeight, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

        GetWindowPos(hDlg, NULL, &rc);
        SetWindowPos(hDlg, 0, 0, 0, rc.right, rc.bottom + nHeight, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

        GetWindowPos(hWndEditBox, hDlg, &rc);
        SetWindowPos(hWndEditBox, 0, rc.left, rc.top + nHeight, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);

        GetWindowPos(hWndOK, hDlg, &rc);
        SetWindowPos(hWndOK, 0, rc.left, rc.top + nHeight, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);

        GetWindowPos(hWndCancel, hDlg, &rc);
        SetWindowPos(hWndCancel, 0, rc.left, rc.top + nHeight, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
      }
    }
  }
  else if (uMsg == WM_COMMAND)
  {
    if (LOWORD(wParam) == IDOK)
    {
      wchar_t *wszText;
      int nTextLen;

      nTextLen=GetWindowTextLengthA(hWndEditBox);

      if (wszText=(wchar_t *)GlobalAlloc(GPTR, (nTextLen + 1) * sizeof(wchar_t)))
      {
        GetWindowTextWide(hWndEditBox, wszText, nTextLen + 1);

        db->vtResult->vt=VT_BSTR;
        if (!(db->vtResult->bstrVal=SysAllocString(wszText)))
          db->hr=E_OUTOFMEMORY;

        GlobalFree((HGLOBAL)wszText);
      }
      EndDialog(hDlg, 0);
      return TRUE;
    }
    else if (LOWORD(wParam) == IDCANCEL)
    {
      EndDialog(hDlg, 0);
      return TRUE;
    }
  }
  return FALSE;
}

HRESULT STDMETHODCALLTYPE Document_GetSelStart(IDocument *this, INT_PTR *nSelStart)
{
  HWND hWndCurEdit;
  CHARRANGE64 cr={0};

  if (hWndCurEdit=GetCurEdit(this))
  {
    SendMessage(hWndCurEdit, EM_EXGETSEL64, 0, (LPARAM)&cr);
    *nSelStart=cr.cpMin;
  }
  else *nSelStart=0;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetSelEnd(IDocument *this, INT_PTR *nSelEnd)
{
  HWND hWndCurEdit;
  CHARRANGE64 cr={0};

  if (hWndCurEdit=GetCurEdit(this))
  {
    SendMessage(hWndCurEdit, EM_EXGETSEL64, 0, (LPARAM)&cr);
    *nSelEnd=cr.cpMax;
  }
  else *nSelEnd=0;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_SetSel(IDocument *this, INT_PTR nSelStart, INT_PTR nSelEnd, DWORD dwFlags)
{
  HWND hWndCurEdit;

  if (hWndCurEdit=GetCurEdit(this))
  {
    if (dwFlags)
    {
      AESELECTION aes;
      AECHARINDEX *ciCaret;

      SendMessage(hWndCurEdit, AEM_RICHOFFSETTOINDEX, (WPARAM)nSelStart, (LPARAM)&aes.crSel.ciMin);
      SendMessage(hWndCurEdit, AEM_RICHOFFSETTOINDEX, (WPARAM)nSelEnd, (LPARAM)&aes.crSel.ciMax);
      aes.dwFlags=dwFlags;
      if (nSelStart > nSelEnd)
        ciCaret=&aes.crSel.ciMin;
      else
        ciCaret=&aes.crSel.ciMax;
      SendMessage(hWndCurEdit, AEM_SETSEL, (WPARAM)ciCaret, (LPARAM)&aes);
    }
    else
    {
      CHARRANGE64 cr;

      cr.cpMin=nSelStart;
      cr.cpMax=nSelEnd;
      SendMessage(hWndCurEdit, EM_EXSETSEL64, 0, (LPARAM)&cr);
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetSelText(IDocument *this, int nNewLine, BSTR *wpText)
{
  HWND hWndCurEdit;
  CHARRANGE64 cr;
  HRESULT hr=NOERROR;

  *wpText=NULL;
  if (hWndCurEdit=GetCurEdit(this))
  {
    SendMessage(hWndCurEdit, EM_EXGETSEL64, 0, (LPARAM)&cr);
    hr=GetTextRange(hWndCurEdit, cr.cpMin, cr.cpMax, nNewLine, -1, wpText);
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_GetTextRange(IDocument *this, INT_PTR nRangeStart, INT_PTR nRangeEnd, int nNewLine, BSTR *wpText)
{
  HWND hWndCurEdit;
  HRESULT hr=NOERROR;

  *wpText=NULL;
  if (hWndCurEdit=GetCurEdit(this))
  {
    hr=GetTextRange(hWndCurEdit, nRangeStart, nRangeEnd, nNewLine, FALSE, wpText);
  }
  return hr;
}

HRESULT GetTextRange(HWND hWnd, INT_PTR nRangeStart, INT_PTR nRangeEnd, int nNewLine, BOOL bColumnSel, BSTR *wpText)
{
  AETEXTRANGEW tr;
  INT_PTR nTextLen=0;
  HRESULT hr=NOERROR;

  *wpText=NULL;

  SendMessage(hWnd, AEM_RICHOFFSETTOINDEX, (WPARAM)nRangeStart, (LPARAM)&tr.cr.ciMin);
  SendMessage(hWnd, AEM_RICHOFFSETTOINDEX, (WPARAM)nRangeEnd, (LPARAM)&tr.cr.ciMax);
  tr.bColumnSel=bColumnSel;
  tr.pBuffer=NULL;
  tr.dwBufferMax=(UINT_PTR)-1;
  if (nNewLine == 1)
    tr.nNewLine=AELB_R;
  else if (nNewLine == 2)
    tr.nNewLine=AELB_N;
  else if (nNewLine == 3)
    tr.nNewLine=AELB_RN;
  else
    tr.nNewLine=AELB_ASIS;
  tr.bFillSpaces=TRUE;

  if (nTextLen=SendMessage(hWnd, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr))
  {
    if (tr.pBuffer=(wchar_t *)GlobalAlloc(GMEM_FIXED, nTextLen * sizeof(wchar_t)))
    {
      if (nTextLen=SendMessage(hWnd, AEM_GETTEXTRANGEW, 0, (LPARAM)&tr))
      {
        if (!(*wpText=SysAllocStringLen((wchar_t *)tr.pBuffer, (UINT)nTextLen)))
          hr=E_OUTOFMEMORY;
      }
      GlobalFree((HGLOBAL)tr.pBuffer);
    }
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_ReplaceSel(IDocument *this, BSTR wpText, BOOL bSelect)
{
  HWND hWndCurEdit;

  if (hWndCurEdit=GetCurEdit(this))
  {
    if (!(SendMessage(hWndCurEdit, AEM_GETOPTIONS, 0, 0) & AECO_READONLY))
    {
      AEREPLACESELW rs;
      AESELECTION aesInitial;
      AESELECTION aesNew;
      AECHARINDEX ciInitialCaret;

      SendMessage(hWndCurEdit, AEM_GETSEL, (WPARAM)&ciInitialCaret, (LPARAM)&aesInitial);
      rs.pText=wpText;
      rs.dwTextLen=SysStringLen(wpText);
      rs.nNewLine=AELB_ASINPUT;
      rs.dwFlags=AEREPT_COLUMNASIS;
      rs.ciInsertStart=&aesNew.crSel.ciMin;
      rs.ciInsertEnd=&aesNew.crSel.ciMax;
      SendMessage(hWndCurEdit, AEM_REPLACESELW, 0, (LPARAM)&rs);

      if (bSelect)
      {
        aesNew.dwFlags=aesInitial.dwFlags;
        if (!AEC_IndexCompare(&aesInitial.crSel.ciMin, &ciInitialCaret))
          SendMessage(hWndCurEdit, AEM_SETSEL, (WPARAM)&aesNew.crSel.ciMin, (LPARAM)&aesNew);
        else
          SendMessage(hWndCurEdit, AEM_SETSEL, (WPARAM)&aesNew.crSel.ciMax, (LPARAM)&aesNew);
      }
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_TextFind(IDocument *this, HWND hWnd, BSTR wpFindIt, DWORD dwFlags, INT_PTR *nResult)
{
  TEXTFINDW tf;

  tf.dwFlags=dwFlags;
  tf.pFindIt=wpFindIt;
  tf.nFindItLen=SysStringLen(wpFindIt);
  *nResult=SendMessage(hMainWnd, AKD_TEXTFINDW, (WPARAM)hWnd, (LPARAM)&tf);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_TextReplace(IDocument *this, HWND hWnd, BSTR wpFindIt, BSTR wpReplaceWith, DWORD dwFlags, BOOL bAll, INT_PTR *nResult)
{
  TEXTREPLACEW tr;

  tr.dwFlags=dwFlags;
  tr.pFindIt=wpFindIt;
  tr.nFindItLen=SysStringLen(wpFindIt);
  tr.pReplaceWith=wpReplaceWith;
  tr.nReplaceWithLen=SysStringLen(wpReplaceWith);
  tr.bAll=bAll;
  *nResult=SendMessage(hMainWnd, AKD_TEXTREPLACEW, (WPARAM)hWnd, (LPARAM)&tr);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetClipboardText(IDocument *this, BOOL bAnsi, BSTR *wpText)
{
  HRESULT hr=NOERROR;
  HGLOBAL hData;
  LPVOID pData;

  *wpText=NULL;

  if (OpenClipboard(NULL))
  {
    if (!bAnsi && (hData=GetClipboardData(CF_UNICODETEXT)))
    {
      if (pData=GlobalLock(hData))
      {
        if (!(*wpText=SysAllocString((wchar_t *)pData)))
          hr=E_OUTOFMEMORY;
        GlobalUnlock(hData);
      }
    }
    else if (hData=GetClipboardData(CF_TEXT))
    {
      if (pData=GlobalLock(hData))
      {
        wchar_t *wszBuffer;
        int nUnicodeLen;

        nUnicodeLen=MultiByteToWideChar(CP_ACP, 0, (char *)pData, -1, NULL, 0);

        if (wszBuffer=(wchar_t *)GlobalAlloc(GPTR, nUnicodeLen * sizeof(wchar_t)))
        {
          MultiByteToWideChar(CP_ACP, 0, (char *)pData, -1, wszBuffer, nUnicodeLen);
          if (!(*wpText=SysAllocString(wszBuffer)))
            hr=E_OUTOFMEMORY;
          GlobalFree((HGLOBAL)wszBuffer);
        }
        else hr=E_OUTOFMEMORY;

        GlobalUnlock(hData);
      }
    }
    CloseClipboard();
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_SetClipboardText(IDocument *this, BSTR wpText)
{
  HRESULT hr=NOERROR;
  HGLOBAL hDataA=NULL;
  HGLOBAL hDataW=NULL;
  LPVOID pData;
  int nUnicodeLen;
  int nAnsiLen;

  if (!wpText) wpText=L"";

  if (OpenClipboard(NULL))
  {
    //Unicode
    nUnicodeLen=SysStringLen(wpText) + 1;

    if (hDataW=GlobalAlloc(GMEM_MOVEABLE, nUnicodeLen * sizeof(wchar_t)))
    {
      if (pData=GlobalLock(hDataW))
      {
        xmemcpy(pData, wpText, nUnicodeLen * sizeof(wchar_t));
        GlobalUnlock(hDataW);
      }
    }

    //ANSI
    nAnsiLen=WideCharToMultiByte(CP_ACP, 0, wpText, nUnicodeLen, NULL, 0, NULL, NULL);

    if (hDataA=GlobalAlloc(GMEM_MOVEABLE, nAnsiLen))
    {
      if (pData=GlobalLock(hDataA))
      {
        WideCharToMultiByte(CP_ACP, 0, wpText, nUnicodeLen, (char *)pData, nAnsiLen, NULL, NULL);
        GlobalUnlock(hDataA);
      }
    }
    EmptyClipboard();
    if (hDataW) SetClipboardData(CF_UNICODETEXT, hDataW);
    if (hDataA) SetClipboardData(CF_TEXT, hDataA);
    CloseClipboard();
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_IsPluginRunning(IDocument *this, BSTR wpFunction, BOOL *bRunning)
{
  PLUGINFUNCTION *pf;

  *bRunning=FALSE;

  if (pf=(PLUGINFUNCTION *)SendMessage(hMainWnd, AKD_DLLFINDW, (WPARAM)wpFunction, 0))
    *bRunning=pf->bRunning;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_Call(IDocument *this, BSTR wpFunction, SAFEARRAY **psa, int *nResult)
{
  return CallPlugin(0, 0, wpFunction, psa, nResult);
}

HRESULT STDMETHODCALLTYPE Document_CallA(IDocument *this, BSTR wpFunction, SAFEARRAY **psa, int *nResult)
{
  return CallPlugin(0, PDS_STRANSI, wpFunction, psa, nResult);
}

HRESULT STDMETHODCALLTYPE Document_CallW(IDocument *this, BSTR wpFunction, SAFEARRAY **psa, int *nResult)
{
  return CallPlugin(0, PDS_STRWIDE, wpFunction, psa, nResult);
}

HRESULT STDMETHODCALLTYPE Document_CallEx(IDocument *this, DWORD dwFlags, BSTR wpFunction, SAFEARRAY **psa, int *nResult)
{
  return CallPlugin(dwFlags, 0, wpFunction, psa, nResult);
}

HRESULT STDMETHODCALLTYPE Document_CallExA(IDocument *this, DWORD dwFlags, BSTR wpFunction, SAFEARRAY **psa, int *nResult)
{
  return CallPlugin(dwFlags, PDS_STRANSI, wpFunction, psa, nResult);
}

HRESULT STDMETHODCALLTYPE Document_CallExW(IDocument *this, DWORD dwFlags, BSTR wpFunction, SAFEARRAY **psa, int *nResult)
{
  return CallPlugin(dwFlags, PDS_STRWIDE, wpFunction, psa, nResult);
}

HRESULT CallPlugin(DWORD dwFlags, DWORD dwSupport, BSTR wpFunction, SAFEARRAY **psa, int *nResult)
{
  POINTERSTACK hStringsStack={0};
  POINTERITEM *lpElement=NULL;
  VARIANT *pvtParameter;
  unsigned char *lpData;
  unsigned char *lpStructure=NULL;
  UINT_PTR dwParameter;
  DWORD dwElement;
  DWORD dwElementSum;
  DWORD dwStructSize;
  DWORD dwOffset;
  HRESULT hr=NOERROR;

  lpData=(unsigned char *)((*psa)->pvData);
  dwElementSum=(*psa)->rgsabound[0].cElements;
  dwStructSize=(dwElementSum + 1) * sizeof(UINT_PTR);
  dwOffset=0;

  if (dwElementSum)
  {
    if (lpStructure=(unsigned char *)GlobalAlloc(GPTR, dwStructSize))
    {
      //Set structure size in first parameter
      *(UINT_PTR *)lpStructure=dwStructSize;
      dwOffset+=sizeof(UINT_PTR);

      for (dwElement=0; dwElement < dwElementSum; ++dwElement)
      {
        pvtParameter=(VARIANT *)(lpData + dwElement * sizeof(VARIANT));
        dwParameter=0;

        if (pvtParameter->vt == (VT_VARIANT|VT_BYREF))
          pvtParameter=pvtParameter->pvarVal;

        if (pvtParameter->vt == VT_BSTR)
        {
          if ((dwSupport & PDS_STRANSI) || (bOldWindows && !(dwSupport & PDS_STRWIDE)))
          {
            char *szString;
            int nStringLenA;
            int nStringLenW;

            nStringLenW=SysStringLen(pvtParameter->bstrVal) + 1;
            nStringLenA=WideCharToMultiByte(CP_ACP, 0, pvtParameter->bstrVal, nStringLenW, NULL, 0, NULL, NULL);

            if (szString=(char *)GlobalAlloc(GPTR, nStringLenA))
            {
              WideCharToMultiByte(CP_ACP, 0, pvtParameter->bstrVal, nStringLenW, szString, nStringLenA, NULL, NULL);

              if (lpElement=StackInsertPointer(&hStringsStack))
              {
                lpElement->lpData=szString;
                lpElement->dwSize=nStringLenA;
              }
            }
            dwParameter=(UINT_PTR)szString;
          }
          else dwParameter=(UINT_PTR)pvtParameter->bstrVal;
        }
        else if (pvtParameter->vt == VT_BOOL)
        {
          dwParameter=pvtParameter->boolVal?TRUE:FALSE;
        }
        else
        {
          dwParameter=GetVariantInt(pvtParameter);
        }
        *(UINT_PTR *)(lpStructure + dwOffset)=dwParameter;
        dwOffset+=sizeof(UINT_PTR);
      }
    }
  }

  //Call plugin
  {
    PLUGINCALLSENDW pcs;

    pcs.pFunction=wpFunction;
    pcs.lParam=(LPARAM)lpStructure;
    pcs.dwSupport=dwSupport;
    *nResult=(int)SendMessage(hMainWnd, AKD_DLLCALLW, (WPARAM)dwFlags, (LPARAM)&pcs);
  }

  //Free structure
  if (lpStructure) GlobalFree((HGLOBAL)lpStructure);

  //Free strings stack
  StackFreePointers(&hStringsStack);

  return hr;
}

HRESULT STDMETHODCALLTYPE Document_Exec(IDocument *this, BSTR wpCmdLine, BSTR wpWorkDir, int nWait, DWORD *dwExit)
{
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  wchar_t *wszExecuteCommandExp;
  wchar_t *wszExecuteDirectoryExp;
  int nCommandLen;
  int nWorkDirLen;

  *dwExit=0;

  if (wpCmdLine)
  {
    nCommandLen=TranslateFileString(wpCmdLine, NULL, 0);
    nWorkDirLen=TranslateFileString(wpWorkDir, NULL, 0);

    if (wszExecuteCommandExp=(wchar_t *)GlobalAlloc(GMEM_FIXED, (nCommandLen + 1) * sizeof(wchar_t)))
    {
      if (wszExecuteDirectoryExp=(wchar_t *)GlobalAlloc(GMEM_FIXED, (nWorkDirLen + 1) * sizeof(wchar_t)))
      {
        TranslateFileString(wpCmdLine, wszExecuteCommandExp, nCommandLen + 1);
        TranslateFileString(wpWorkDir, wszExecuteDirectoryExp, nWorkDirLen + 1);

        if (*wszExecuteCommandExp)
        {
          xmemset(&si, 0, sizeof(STARTUPINFOW));
          si.cb=sizeof(STARTUPINFOW);
          if (CreateProcessWide(NULL, wszExecuteCommandExp, NULL, NULL, FALSE, 0, NULL, (wszExecuteDirectoryExp && *wszExecuteDirectoryExp)?wszExecuteDirectoryExp:NULL, &si, &pi))
          {
            if (nWait == 1)
            {
              WaitForSingleObject(pi.hProcess, INFINITE);
              GetExitCodeProcess(pi.hProcess, dwExit);
            }
            else if (nWait == 2)
            {
              WaitForInputIdle(pi.hProcess, INFINITE);
            }
          }
        }
        GlobalFree((HGLOBAL)wszExecuteDirectoryExp);
      }
      GlobalFree((HGLOBAL)wszExecuteCommandExp);
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_Command(IDocument *this, int nCommand, LPARAM lParam, INT_PTR *nResult)
{
  *nResult=SendMessage(hMainWnd, WM_COMMAND, (WPARAM)nCommand, lParam);
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_Font(IDocument *this, BSTR wpFaceName, DWORD dwFontStyle, int nPointSize)
{
  HWND hWndCurEdit;
  LOGFONTW lfNew;
  HDC hDC;

  if (!wpFaceName) wpFaceName=L"";

  if (hWndCurEdit=(HWND)SendMessage(hMainWnd, AKD_GETFRAMEINFO, FI_WNDEDIT, (LPARAM)NULL))
  {
    if (SendMessage(hMainWnd, AKD_GETFONTW, (WPARAM)hWndCurEdit, (LPARAM)&lfNew))
    {
      if (nPointSize)
      {
        if (hDC=GetDC(hWndCurEdit))
        {
          lfNew.lfHeight=-MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
          ReleaseDC(hWndCurEdit, hDC);
        }
      }
      if (dwFontStyle != FS_NONE)
      {
        lfNew.lfWeight=(dwFontStyle == FS_FONTBOLD || dwFontStyle == FS_FONTBOLDITALIC)?FW_BOLD:FW_NORMAL;
        lfNew.lfItalic=(dwFontStyle == FS_FONTITALIC || dwFontStyle == FS_FONTBOLDITALIC)?TRUE:FALSE;
      }
      if (*wpFaceName != '\0')
      {
        xstrcpynW(lfNew.lfFaceName, wpFaceName, LF_FACESIZE);
      }
      SendMessage(hMainWnd, AKD_SETFONTW, (WPARAM)hWndCurEdit, (LPARAM)&lfNew);
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_Recode(IDocument *this, int nCodePageFrom, int nCodePageTo)
{
  HWND hWndCurEdit;
  TEXTRECODE tr;

  if (hWndCurEdit=(HWND)SendMessage(hMainWnd, AKD_GETFRAMEINFO, FI_WNDEDIT, (LPARAM)NULL))
  {
    tr.nCodePageFrom=nCodePageFrom;
    tr.nCodePageTo=nCodePageTo;
    tr.dwFlags=0;
    SendMessage(hMainWnd, AKD_RECODESEL, (WPARAM)hWndCurEdit, (LPARAM)&tr);
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_Include(IDocument *this, BSTR wpFileName, BOOL *bResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  INCLUDEITEM *lpIncludeItem;
  wchar_t wszScriptInclude[MAX_PATH];
  wchar_t *wpContent=NULL;
  HRESULT hr=NOERROR;

  *bResult=FALSE;

  if (lpIncludeItem=StackInsertInclude(&lpScriptThread->hIncludesStack))
  {
    xstrcpynW(wszScriptInclude, lpScriptThread->wszScriptInclude, MAX_PATH);
    xprintfW(lpScriptThread->wszScriptInclude, L"%s\\AkelFiles\\Plugs\\Scripts\\Include\\%s", wszAkelPadDir, wpFileName);
    xstrcpynW(lpIncludeItem->wszInclude, lpScriptThread->wszScriptInclude, MAX_PATH);

    if (ReadFileContent(lpScriptThread->wszScriptInclude, OD_ADT_BINARY_ERROR|OD_ADT_DETECT_CODEPAGE|OD_ADT_DETECT_BOM, 0, 0, &wpContent, (UINT_PTR)-1))
    {
      lpScriptThread->objActiveScript->lpVtbl->SetScriptState(lpScriptThread->objActiveScript, SCRIPTSTATE_DISCONNECTED);
      if ((hr=lpScriptThread->objActiveScriptParse->lpVtbl->ParseScriptText(lpScriptThread->objActiveScriptParse, wpContent, NULL, NULL, NULL, lpScriptThread->hIncludesStack.nElements, 0, 0, NULL, NULL)) == S_OK)
        *bResult=TRUE;
      lpScriptThread->objActiveScript->lpVtbl->SetScriptState(lpScriptThread->objActiveScript, SCRIPTSTATE_CONNECTED);

      SendMessage(hMainWnd, AKD_FREETEXT, 0, (LPARAM)wpContent);
    }
    xstrcpynW(lpScriptThread->wszScriptInclude, wszScriptInclude, MAX_PATH);
  }

  return hr;
}

HRESULT STDMETHODCALLTYPE Document_IsInclude(IDocument *this, BSTR *wpFileName)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  HRESULT hr=NOERROR;

  if (lpScriptThread->wszScriptInclude)
  {
    if (!(*wpFileName=SysAllocString(lpScriptThread->wszScriptInclude)))
      hr=E_OUTOFMEMORY;
  }
  else *wpFileName=NULL;

  return hr;
}

HRESULT STDMETHODCALLTYPE Document_OpenFile(IDocument *this, BSTR wpFile, DWORD dwFlags, int nCodePage, BOOL bBOM, int *nResult)
{
  OPENDOCUMENTW od;

  od.pFile=wpFile;
  od.pWorkDir=NULL;
  od.dwFlags=dwFlags;
  od.nCodePage=nCodePage;
  od.bBOM=bBOM;
  *nResult=(int)SendMessage(hMainWnd, AKD_OPENDOCUMENTW, (WPARAM)NULL, (LPARAM)&od);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_ReadFile(IDocument *this, BSTR wpFile, DWORD dwFlags, int nCodePage, BOOL bBOM, INT_PTR nBytesMax, BSTR *wpText)
{
  wchar_t *wpContent=NULL;
  HRESULT hr=NOERROR;
  INT_PTR nContentLen;

  *wpText=NULL;

  nContentLen=ReadFileContent(wpFile, dwFlags, nCodePage, bBOM, &wpContent, (UINT_PTR)nBytesMax);

  if (wpContent)
  {
    if (!(*wpText=SysAllocStringLen(wpContent, (DWORD)nContentLen)))
      hr=E_OUTOFMEMORY;
    SendMessage(hMainWnd, AKD_FREETEXT, 0, (LPARAM)wpContent);
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_SaveFile(IDocument *this, HWND hWnd, BSTR wpFile, int nCodePage, BOOL bBOM, DWORD dwFlags, int *nResult)
{
  SAVEDOCUMENTW sd;
  EDITINFO ei;

  if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)hWnd, (LPARAM)&ei))
  {
    if (nCodePage == -1)
      nCodePage=ei.nCodePage;
    if (bBOM == -1)
      bBOM=ei.bBOM;
    sd.pFile=wpFile;
    sd.nCodePage=nCodePage;
    sd.bBOM=bBOM;
    sd.dwFlags=dwFlags;
    *nResult=(int)SendMessage(hMainWnd, AKD_SAVEDOCUMENTW, (WPARAM)hWnd, (LPARAM)&sd);
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_ScriptSettings(IDocument *this, IDispatch **objSet)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  IRealScriptSettings *lpSet;

  if (!objSet) return E_POINTER;

  if ((lpSet=(IRealScriptSettings *)GlobalAlloc(GPTR, sizeof(IRealScriptSettings))))
  {
    lpSet->lpVtbl=(IScriptSettingsVtbl *)&MyIScriptSettingsVtbl;
    lpSet->dwCount=1;
    lpSet->lpScriptThread=(void *)lpScriptThread;

    InterlockedIncrement(&g_nObjs);
    *objSet=(IDispatch *)lpSet;
  }
  else return E_OUTOFMEMORY;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_SystemFunction(IDocument *this, IDispatch **objSys)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  IRealSystemFunction *lpSys;

  if (!objSys) return E_POINTER;

  if ((lpSys=(IRealSystemFunction *)GlobalAlloc(GPTR, sizeof(IRealSystemFunction))))
  {
    lpSys->lpVtbl=(ISystemFunctionVtbl *)&MyISystemFunctionVtbl;
    lpSys->dwCount=1;
    lpSys->lpScriptThread=(void *)lpScriptThread;

    InterlockedIncrement(&g_nObjs);
    *objSys=(IDispatch *)lpSys;
  }
  else return E_OUTOFMEMORY;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_MemAlloc(IDocument *this, DWORD dwSize, INT_PTR *nPointer)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  POINTERITEM *lpElement=NULL;

  if (*nPointer=(INT_PTR)GlobalAlloc(GPTR, dwSize))
  {
    if (lpElement=StackInsertPointer(&lpScriptThread->hPointersStack))
    {
      lpElement->lpData=(void *)*nPointer;
      lpElement->dwSize=dwSize;
    }
    return NOERROR;
  }
  return E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE Document_MemCopy(IDocument *this, INT_PTR nPointer, VARIANT vtData, DWORD dwType, int nDataLen, int *nBytes)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  CALLBACKITEM *lpSysCallback;
  POINTERITEM *lpElement=NULL;
  VARIANT *pvtData=&vtData;
  unsigned char *lpString=NULL;
  UINT_PTR dwNumber=0;
  int nStringLen;
  HRESULT hr=NOERROR;

  *nBytes=0;

  if (pvtData->vt == (VT_VARIANT|VT_BYREF))
    pvtData=pvtData->pvarVal;

  if (pvtData->vt == VT_DISPATCH)
  {
    if (lpSysCallback=StackGetCallbackByObject(&g_hSysCallbackStack, pvtData->pdispVal))
      dwNumber=(UINT_PTR)lpSysCallback->hHandle;
    else
      dwNumber=(UINT_PTR)pvtData->pdispVal;
  }
  else if (pvtData->vt == VT_BSTR)
  {
    lpString=(unsigned char *)pvtData->bstrVal;
    if (nDataLen == -1)
      nStringLen=SysStringLen(pvtData->bstrVal);
    else
      nStringLen=nDataLen;
  }
  else if (pvtData->vt == VT_BOOL)
    dwNumber=pvtData->boolVal?TRUE:FALSE;
  else
    dwNumber=GetVariantInt(pvtData);

  if (nPointer && (lpScriptThread->dwDebug & DBG_MEMWRITE))
  {
    if (!(lpElement=StackGetPointer(&lpScriptThread->hPointersStack, (void *)nPointer, 1)))
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMLOCATE), nPointer);
      return E_POINTER;
    }
  }

  if (dwType == DT_ANSI)
  {
    if (lpString)
    {
      *nBytes=WideCharToMultiByte(CP_ACP, 0, (wchar_t *)lpString, nStringLen, NULL, 0, NULL, NULL);

      if (nPointer)
      {
        if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + *nBytes + sizeof(char))
        {
          xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMWRITE));
          return E_POINTER;
        }
        WideCharToMultiByte(CP_ACP, 0, (wchar_t *)lpString, nStringLen, (char *)nPointer, *nBytes, NULL, NULL);
      }
      if (nDataLen == -1)
      {
        if (nPointer) ((char *)nPointer)[*nBytes]='\0';
        *nBytes+=sizeof(char);
      }
    }
  }
  else if (dwType == DT_UNICODE)
  {
    if (lpString)
    {
      *nBytes=nStringLen * sizeof(wchar_t);

      if (nPointer)
      {
        if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + *nBytes + sizeof(wchar_t))
        {
          xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMWRITE));
          return E_POINTER;
        }
        xmemcpy((void *)nPointer, lpString, *nBytes);
      }
      if (nDataLen == -1)
      {
        if (nPointer) ((wchar_t *)nPointer)[nStringLen]=L'\0';
        *nBytes+=sizeof(wchar_t);
      }
    }
  }
  else if (dwType == DT_QWORD)
  {
    *nBytes=sizeof(UINT_PTR);

    if (nPointer)
    {
      if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + *nBytes)
      {
        xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMWRITE));
        return E_POINTER;
      }
      xmemcpy((void *)nPointer, &dwNumber, *nBytes);
    }
  }
  else if (dwType == DT_DWORD)
  {
    *nBytes=sizeof(DWORD);

    if (nPointer)
    {
      if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + *nBytes)
      {
        xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMWRITE));
        return E_POINTER;
      }
      xmemcpy((void *)nPointer, &dwNumber, *nBytes);
    }
  }
  else if (dwType == DT_WORD)
  {
    *nBytes=sizeof(WORD);

    if (nPointer)
    {
      if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + *nBytes)
      {
        xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMWRITE));
        return E_POINTER;
      }
      dwNumber=LOWORD((DWORD)dwNumber);
      xmemcpy((void *)nPointer, &dwNumber, *nBytes);
    }
  }
  else if (dwType == DT_BYTE)
  {
    *nBytes=sizeof(BYTE);

    if (nPointer)
    {
      if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + *nBytes)
      {
        xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMWRITE));
        return E_POINTER;
      }
      dwNumber=LOBYTE((DWORD)dwNumber);
      xmemcpy((void *)nPointer, &dwNumber, *nBytes);
    }
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_MemRead(IDocument *this, INT_PTR nPointer, DWORD dwType, int nDataLen, VARIANT *vtData)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  POINTERITEM *lpElement=NULL;
  wchar_t *wszString;
  int nStringLen;
  HRESULT hr=NOERROR;

  if (lpScriptThread->dwDebug & DBG_MEMREAD)
  {
    if (!(lpElement=StackGetPointer(&lpScriptThread->hPointersStack, (void *)nPointer, 1)))
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMLOCATE), nPointer);
      return E_POINTER;
    }
  }
  VariantInit(vtData);

  if (dwType == DT_ANSI)
  {
    if (nDataLen == -1)
      nStringLen=lstrlenA((char *)nPointer);
    else
      nStringLen=nDataLen;

    if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + nStringLen)
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMREAD));
      return E_POINTER;
    }
    if (wszString=(wchar_t *)GlobalAlloc(GPTR, (nStringLen + 1) * sizeof(wchar_t)))
    {
      nStringLen=MultiByteToWideChar(CP_ACP, 0, (char *)nPointer, nStringLen, wszString, nStringLen);
      vtData->vt=VT_BSTR;
      if (!(vtData->bstrVal=SysAllocStringLen(wszString, nStringLen)))
        hr=E_OUTOFMEMORY;
      GlobalFree((HGLOBAL)wszString);
    }
    else hr=E_OUTOFMEMORY;
  }
  else if (dwType == DT_UNICODE)
  {
    if (nDataLen == -1)
      nStringLen=lstrlenW((wchar_t *)nPointer);
    else
      nStringLen=nDataLen;

    if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)((wchar_t *)nPointer + nStringLen))
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMREAD));
      return E_POINTER;
    }
    vtData->vt=VT_BSTR;
    if (!(vtData->bstrVal=SysAllocStringLen((wchar_t *)nPointer, nStringLen)))
      hr=E_OUTOFMEMORY;
  }
  else if (dwType == DT_QWORD)
  {
    if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + sizeof(UINT_PTR))
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMREAD));
      return E_POINTER;
    }
    #ifdef _WIN64
      vtData->vt=VT_I8;
      vtData->llVal=*(INT_PTR *)nPointer;
    #else
      vtData->vt=VT_I4;
      vtData->lVal=*(INT_PTR *)nPointer;
    #endif
  }
  else if (dwType == DT_DWORD)
  {
    if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + sizeof(DWORD))
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMREAD));
      return E_POINTER;
    }
    vtData->vt=VT_I4;
    vtData->lVal=(DWORD)(*(INT_PTR *)nPointer);
  }
  else if (dwType == DT_WORD)
  {
    if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + sizeof(WORD))
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMREAD));
      return E_POINTER;
    }
    vtData->vt=VT_UI2;
    vtData->uiVal=LOWORD(*(UINT_PTR *)nPointer);
  }
  else if (dwType == DT_BYTE)
  {
    if (lpElement && (LPBYTE)lpElement->lpData + lpElement->dwSize < (LPBYTE)nPointer + sizeof(BYTE))
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMREAD));
      return E_POINTER;
    }
    vtData->vt=VT_UI1;
    vtData->bVal=LOBYTE(*(UINT_PTR *)nPointer);
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_MemStrPtr(IDocument *this, BSTR wpString, INT_PTR *nPointer)
{
  *nPointer=(INT_PTR)wpString;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_MemFree(IDocument *this, INT_PTR nPointer)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  POINTERITEM *lpElement=NULL;

  if (!(lpElement=StackGetPointer(&lpScriptThread->hPointersStack, (void *)nPointer, 1)))
  {
    if (lpScriptThread->dwDebug & DBG_MEMFREE)
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_MEMFREE), nPointer);
      return E_POINTER;
    }
  }
  else StackDeletePointer(&lpScriptThread->hPointersStack, lpElement);

  GlobalFree((HGLOBAL)nPointer);
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_Debug(IDocument *this, DWORD dwDebug, DWORD *dwResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;

  *dwResult=lpScriptThread->dwDebug;
  lpScriptThread->dwDebug=dwDebug;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_VarType(IDocument *this, VARIANT vtData, int *nType)
{
  VARIANT *pvtData=&vtData;

  if (pvtData->vt == (VT_VARIANT|VT_BYREF))
    pvtData=pvtData->pvarVal;

  *nType=pvtData->vt;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_GetArgLine(IDocument *this, BOOL bNoEncloseQuotes, BSTR *wpArgLine)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  HRESULT hr=NOERROR;
  wchar_t *wpNoQuote=lpScriptThread->wszArguments;
  int nNoQuoteLen=lpScriptThread->nArgumentsLen;

  if (lpScriptThread->wszArguments && *lpScriptThread->wszArguments)
  {
    if (bNoEncloseQuotes)
    {
      while ((wpNoQuote[0] == L'\"' || wpNoQuote[0] == L'\'' || wpNoQuote[0] == L'`') && nNoQuoteLen >= 2 && wpNoQuote[0] == wpNoQuote[nNoQuoteLen - 1])
      {
        ++wpNoQuote;
        nNoQuoteLen-=2;
      }
    }
    if (!(*wpArgLine=SysAllocStringLen(wpNoQuote, nNoQuoteLen)))
      hr=E_OUTOFMEMORY;
  }
  else *wpArgLine=NULL;

  return hr;
}

HRESULT STDMETHODCALLTYPE Document_GetArgValue(IDocument *this, BSTR wpArgName, VARIANT vtDefault, VARIANT *vtResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  SCRIPTARG *lpScriptArg;
  wchar_t *wszArgValue;
  HRESULT hr=NOERROR;

  VariantInit(vtResult);

  if (lpScriptArg=StackGetArgumentByName(&lpScriptThread->hArgStack, wpArgName, SysStringLen(wpArgName)))
  {
    if (vtDefault.vt == VT_BSTR)
    {
      vtResult->vt=VT_BSTR;
      vtResult->bstrVal=SysAllocStringLen(lpScriptArg->wpArgValue, lpScriptArg->nArgValueLen);
    }
    else
    {
      if (wszArgValue=(wchar_t *)GlobalAlloc(GPTR, (lpScriptArg->nArgValueLen + 1) * sizeof(wchar_t)))
      {
        xstrcpynW(wszArgValue, lpScriptArg->wpArgValue, lpScriptArg->nArgValueLen + 1);
        hr=lpScriptThread->objActiveScriptParse->lpVtbl->ParseScriptText(lpScriptThread->objActiveScriptParse, wszArgValue, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, vtResult, NULL);
        GlobalFree((HGLOBAL)wszArgValue);
      }
      else hr=E_OUTOFMEMORY;
    }
  }
  else
  {
    if (vtDefault.vt != VT_ERROR)
      hr=VariantCopy(vtResult, &vtDefault);
  }
  return hr;
}

HRESULT STDMETHODCALLTYPE Document_WindowRegisterClass(IDocument *this, BSTR wpClassName, WORD *wAtom)
{
  WNDCLASSW wndclass;

  wndclass.style        =CS_HREDRAW|CS_VREDRAW;
  wndclass.lpfnWndProc  =DialogCallbackProc;
  wndclass.cbClsExtra   =0;
  wndclass.cbWndExtra   =DLGWINDOWEXTRA;
  wndclass.hInstance    =hInstanceDLL;
  wndclass.hIcon        =NULL;
  wndclass.hCursor      =LoadCursor(NULL, IDC_ARROW);
  wndclass.hbrBackground=(HBRUSH)(UINT_PTR)(COLOR_BTNFACE + 1);
  wndclass.lpszMenuName =NULL;
  wndclass.lpszClassName=wpClassName;
  *wAtom=RegisterClassWide(&wndclass);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowUnregisterClass(IDocument *this, BSTR wpClassName, BOOL *bResult)
{
  *bResult=UnregisterClassWide(wpClassName, hInstanceDLL);

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowRegisterDialog(IDocument *this, HWND hDlg, BOOL *bResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  CALLBACKITEM *lpCallback;

  *bResult=FALSE;

  if (!StackGetCallbackByHandle(&lpScriptThread->hDialogCallbackStack, hDlg))
  {
    if (lpCallback=StackInsertCallback(&lpScriptThread->hDialogCallbackStack))
    {
      lpCallback->hHandle=(HANDLE)hDlg;
      lpCallback->objFunction=NULL;
      lpCallback->lpScriptThread=(void *)lpScriptThread;
      lpCallback->nCallbackType=CIT_DIALOG;
      *bResult=TRUE;
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowUnregisterDialog(IDocument *this, HWND hDlg, BOOL *bResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  CALLBACKITEM *lpCallback;

  *bResult=FALSE;

  if (lpCallback=StackGetCallbackByHandle(&lpScriptThread->hDialogCallbackStack, hDlg))
  {
    StackDeleteCallback(&lpScriptThread->hDialogCallbackStack, lpCallback);
    *bResult=TRUE;
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowGetMessage(IDocument *this)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  CALLBACKITEM *lpCallback;
  MSG msg;

  lpScriptThread->bMessageLoop=TRUE;

  while (GetMessageWide(&msg, NULL, 0, 0) > 0)
  {
    //Dialog message
    if (TranslateAcceleratorWide(hMainWnd, hGlobalAccel, &msg))
      continue;

    for (lpCallback=lpScriptThread->hDialogCallbackStack.first; lpCallback; lpCallback=lpCallback->next)
    {
      if (lpCallback->hHandle && IsDialogMessageWide((HWND)lpCallback->hHandle, &msg))
      {
        if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
        {
          SendMessage((HWND)lpCallback->hHandle, msg.message, msg.wParam, msg.lParam);
        }
        break;
      }
    }

    //Regular message
    if (!lpCallback)
    {
      TranslateMessage(&msg);
      DispatchMessageWide(&msg);
    }
  }
  lpScriptThread->bMessageLoop=FALSE;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowSubClass(IDocument *this, HWND hWnd, IDispatch *objFunction, SAFEARRAY **psa, INT_PTR *lpCallbackItem)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  CALLBACKITEM *lpCallback=NULL;
  WNDPROC lpOldWndProc;
  UINT_PTR dwData;
  int nCallbackType=0;

  if (hWnd)
  {
    if ((INT_PTR)hWnd == WSC_MAINPROC)
    {
      if (!g_lpSubclassMainProc)
      {
        SendMessage(hMainWnd, AKD_SETMAINPROC, (WPARAM)SubclassMainProc, (LPARAM)&g_lpSubclassMainProc);
        dwData=(UINT_PTR)g_lpSubclassMainProc;
        nCallbackType=CIT_MAINPROC;
      }
    }
    else if ((INT_PTR)hWnd == WSC_EDITPROC)
    {
      if (!g_lpSubclassEditProc)
      {
        SendMessage(hMainWnd, AKD_SETEDITPROC, (WPARAM)SubclassEditProc, (LPARAM)&g_lpSubclassEditProc);
        dwData=(UINT_PTR)g_lpSubclassEditProc;
        nCallbackType=CIT_EDITPROC;
      }
    }
    else if ((INT_PTR)hWnd == WSC_FRAMEPROC)
    {
      if (!g_lpSubclassFrameProc && nMDI == WMD_MDI)
      {
        SendMessage(hMainWnd, AKD_SETFRAMEPROC, (WPARAM)SubclassFrameProc, (LPARAM)&g_lpSubclassFrameProc);
        dwData=(UINT_PTR)g_lpSubclassFrameProc;
        nCallbackType=CIT_FRAMEPROC;
      }
    }
    else
    {
      if (lpOldWndProc=(WNDPROC)GetWindowLongPtrWide(hWnd, GWLP_WNDPROC))
      {
        SetWindowLongPtrWide(hWnd, GWLP_WNDPROC, (UINT_PTR)SubclassCallbackProc);
        dwData=(UINT_PTR)lpOldWndProc;
        nCallbackType=CIT_SUBCLASS;
      }
    }

    if (nCallbackType)
    {
      if (lpCallback=StackInsertCallback(&g_hSubclassCallbackStack))
      {
        lpCallback->hHandle=(HANDLE)hWnd;
        lpCallback->objFunction=objFunction;
        lpCallback->dwData=dwData;
        lpCallback->nCallbackType=nCallbackType;
        lpCallback->lpScriptThread=(void *)lpScriptThread;

        if (!lpScriptThread->hWndScriptsThreadDummy)
        {
          lpScriptThread->hWndScriptsThreadDummy=CreateScriptsThreadDummyWindow();
        }
        StackFillMessages(&lpCallback->hMsgIntStack, *psa);
      }
    }
  }
  *lpCallbackItem=(INT_PTR)lpCallback;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowNextProc(IDocument *this, INT_PTR *lpCallbackItem, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
  CALLBACKITEM *lpCallback=(CALLBACKITEM *)lpCallbackItem;

  if (lpCallback->hHandle)
  {
    if ((INT_PTR)lpCallback->hHandle == WSC_MAINPROC)
    {
      *lResult=g_lpSubclassMainProc->NextProc(hWnd, uMsg, wParam, lParam);
    }
    else if ((INT_PTR)lpCallback->hHandle == WSC_EDITPROC)
    {
      *lResult=g_lpSubclassEditProc->NextProc(hWnd, uMsg, wParam, lParam);
    }
    else if ((INT_PTR)lpCallback->hHandle == WSC_FRAMEPROC)
    {
      *lResult=g_lpSubclassFrameProc->NextProc(hWnd, uMsg, wParam, lParam);
    }
    else
    {
      if (bOldWindows)
        *lResult=CallWindowProcA((WNDPROC)lpCallback->dwData, hWnd, uMsg, wParam, lParam);
      else
        *lResult=CallWindowProcW((WNDPROC)lpCallback->dwData, hWnd, uMsg, wParam, lParam);
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowNoNextProc(IDocument *this, INT_PTR *lpCallbackItem)
{
  CALLBACKITEM *lpCallback=(CALLBACKITEM *)lpCallbackItem;

  lpCallback->bNoNextProc=TRUE;
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_WindowUnsubClass(IDocument *this, HWND hWnd)
{
  CALLBACKITEM *lpCallback;

  if (hWnd)
  {
    if ((INT_PTR)hWnd == WSC_MAINPROC)
    {
      if (g_lpSubclassMainProc)
      {
        lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, (HANDLE)(INT_PTR)WSC_MAINPROC);
        SendMessage(hMainWnd, AKD_SETMAINPROC, (WPARAM)NULL, (LPARAM)&g_lpSubclassMainProc);
        g_lpSubclassMainProc=NULL;
      }
    }
    else if ((INT_PTR)hWnd == WSC_EDITPROC)
    {
      if (g_lpSubclassEditProc)
      {
        lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, (HANDLE)(INT_PTR)WSC_EDITPROC);
        SendMessage(hMainWnd, AKD_SETEDITPROC, (WPARAM)NULL, (LPARAM)&g_lpSubclassEditProc);
        g_lpSubclassEditProc=NULL;
      }
    }
    else if ((INT_PTR)hWnd == WSC_FRAMEPROC)
    {
      if (g_lpSubclassFrameProc)
      {
        lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, (HANDLE)(INT_PTR)WSC_FRAMEPROC);
        SendMessage(hMainWnd, AKD_SETFRAMEPROC, (WPARAM)NULL, (LPARAM)&g_lpSubclassFrameProc);
        g_lpSubclassFrameProc=NULL;
      }
    }
    else
    {
      if (lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, hWnd))
        SetWindowLongPtrWide(hWnd, GWLP_WNDPROC, lpCallback->dwData);
    }
    if (lpCallback) StackDeleteCallback(&g_hSubclassCallbackStack, lpCallback);
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_ThreadHook(IDocument *this, int idHook, IDispatch *objFunction, DWORD dwThreadId, HHOOK *hHook)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;
  CALLBACKITEM *lpCallback;
  HOOKPROC lpHookProc=NULL;
  int nIndex;

  *hHook=NULL;

  if (objFunction)
  {
    //Find unhooked element if any
    if (lpCallback=StackGetCallbackByHandle(&g_hHookCallbackStack, NULL))
      nIndex=lpCallback->nStaticIndex - 1;
    else
      nIndex=g_hHookCallbackStack.nElements;

    //We support limited number of callbacks because with one callback we couldn't know what hook is called callback
    if (nIndex == 0)
      lpHookProc=Hook1CallbackProc;
    else if (nIndex == 1)
      lpHookProc=Hook2CallbackProc;
    else if (nIndex == 2)
      lpHookProc=Hook3CallbackProc;
    else if (nIndex == 3)
      lpHookProc=Hook4CallbackProc;

    if (lpHookProc)
    {
      if (*hHook=SetWindowsHookEx(idHook, lpHookProc, NULL, dwThreadId))
      {
        if (!lpCallback)
          lpCallback=StackInsertCallback(&g_hHookCallbackStack);

        if (lpCallback)
        {
          lpCallback->hHandle=(HANDLE)*hHook;
          lpCallback->objFunction=objFunction;
          lpCallback->dwData=0;
          lpCallback->nCallbackType=CIT_HOOKCALLBACK;
          lpCallback->lpScriptThread=(void *)lpScriptThread;

          if (!lpScriptThread->hWndScriptsThreadDummy)
          {
            lpScriptThread->hWndScriptsThreadDummy=CreateScriptsThreadDummyWindow();
          }
        }
      }
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_ThreadUnhook(IDocument *this, HHOOK hHook, BOOL *bResult)
{
  CALLBACKITEM *lpCallback;

  if (lpCallback=StackGetCallbackByHandle(&g_hHookCallbackStack, (HANDLE)hHook))
  {
    if (*bResult=UnhookWindowsHookEx(hHook))
    {
      //We don't use StackDeleteCallback, because stack elements is linked to static procedure addresses.
      lpCallback->hHandle=NULL;
      lpCallback->objFunction=NULL;
      lpCallback->dwData=0;
      lpCallback->lpScriptThread=NULL;
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_ScriptNoMutex(IDocument *this, DWORD dwUnlockType, DWORD *dwResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;

  *dwResult=0;

  if (dwUnlockType & ULT_LOCKMULTICOPY)
  {
    lpScriptThread->bSingleCopy=TRUE;
    *dwResult|=ULT_LOCKMULTICOPY;
  }
  if (dwUnlockType & ULT_UNLOCKMULTICOPY)
  {
    lpScriptThread->bSingleCopy=FALSE;
    *dwResult|=ULT_UNLOCKMULTICOPY;
  }

  if (dwUnlockType & ULT_UNLOCKSCRIPTSQUEUE)
  {
    if (lpScriptThread->hExecMutex)
    {
      SetEvent(lpScriptThread->hExecMutex);
      CloseHandle(lpScriptThread->hExecMutex);
      lpScriptThread->hExecMutex=NULL;
      *dwResult|=ULT_UNLOCKSCRIPTSQUEUE;
    }
  }
  if (dwUnlockType & ULT_UNLOCKPROGRAMTHREAD)
  {
    if (lpScriptThread->hInitMutex)
    {
      SetEvent(lpScriptThread->hInitMutex);
      lpScriptThread->hInitMutex=NULL;
      *dwResult|=ULT_UNLOCKPROGRAMTHREAD;
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Document_ScriptHandle(IDocument *this, VARIANT vtData, int nOperation, VARIANT *vtResult)
{
  INT_PTR nResult=0;
  HRESULT hr=NOERROR;
  VARIANT *pvtData=&vtData;

  if (pvtData->vt == (VT_VARIANT|VT_BYREF))
    pvtData=pvtData->pvarVal;
  VariantInit(vtResult);

  if (nOperation == SH_FIRSTSCRIPT)
  {
    nResult=(INT_PTR)hThreadStack.first;
  }
  else if (nOperation == SH_THISSCRIPT)
  {
    nResult=(INT_PTR)((IRealDocument *)this)->lpScriptThread;
  }
  else if (nOperation == SH_FINDSCRIPT)
  {
    if (pvtData->vt == VT_BSTR)
    {
      const wchar_t *wpScriptName=(const wchar_t *)pvtData->bstrVal;

      if (wpScriptName && *wpScriptName)
        nResult=(INT_PTR)StackGetScriptThreadByName(&hThreadStack, wpScriptName);
    }
  }
  else
  {
    SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)GetVariantInt(pvtData);

    if (lpScriptThread)
    {
      if (nOperation == SH_GETTHREADHANDLE)
        nResult=(INT_PTR)lpScriptThread->hThread;
      else if (nOperation == SH_GETTHREADID)
        nResult=lpScriptThread->dwThreadID;
      else if (nOperation == SH_GETMESSAGELOOP)
        nResult=lpScriptThread->bMessageLoop;
      else if (nOperation == SH_GETNAME)
      {
        vtResult->vt=VT_BSTR;
        if (!(vtResult->bstrVal=SysAllocString(lpScriptThread->wszScriptName)))
          hr=E_OUTOFMEMORY;
        return hr;
      }
      else if (nOperation == SH_GETFILE)
      {
        vtResult->vt=VT_BSTR;
        if (!(vtResult->bstrVal=SysAllocString(lpScriptThread->wszScriptFile)))
          hr=E_OUTOFMEMORY;
        return hr;
      }
      else if (nOperation == SH_GETNCLUDE)
      {
        vtResult->vt=VT_BSTR;
        if (!(vtResult->bstrVal=SysAllocString(lpScriptThread->wszScriptInclude)))
          hr=E_OUTOFMEMORY;
        return hr;
      }
      else if (nOperation == SH_GETARGUMENTS)
      {
        vtResult->vt=VT_BSTR;
        if (!(vtResult->bstrVal=SysAllocStringLen(lpScriptThread->wszArguments, lpScriptThread->nArgumentsLen)))
          hr=E_OUTOFMEMORY;
        return hr;
      }
      else if (nOperation == SH_NEXTSCRIPT)
      {
        nResult=(INT_PTR)lpScriptThread->next;
      }
      else if (nOperation == SH_NEXTSAMESCRIPT)
      {
        SCRIPTTHREAD *lpScriptThreadNext;

        for (lpScriptThreadNext=lpScriptThread->next; lpScriptThreadNext; lpScriptThreadNext=lpScriptThreadNext->next)
        {
          if (!xstrcmpiW(lpScriptThreadNext->wszScriptName, lpScriptThread->wszScriptName))
          {
            nResult=(INT_PTR)lpScriptThreadNext;
            break;
          }
        }
      }
      else if (nOperation == SH_CLOSESCRIPT)
      {
        if (lpScriptThread->hWndScriptsThreadDummy && lpScriptThread->bMessageLoop)
          SendMessage(lpScriptThread->hWndScriptsThreadDummy, AKDLL_POSTQUIT, 0, 0);
      }
    }
  }

  #ifdef _WIN64
    vtResult->vt=VT_I8;
    vtResult->llVal=nResult;
  #else
    vtResult->vt=VT_I4;
    vtResult->lVal=nResult;
  #endif
  return hr;
}

HWND GetCurEdit(IDocument *this)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealDocument *)this)->lpScriptThread;

  if (lpScriptThread->hWndPluginEdit)
    return lpScriptThread->hWndPluginEdit;
  return (HWND)SendMessage(hMainWnd, AKD_GETFRAMEINFO, FI_WNDEDIT, (LPARAM)NULL);
}

int TranslateFileString(const wchar_t *wpString, wchar_t *wszBuffer, int nBufferSize)
{
  //%a -AkelPad directory, %% -%
  wchar_t *wpExeDir=wszAkelPadDir;
  wchar_t *wszSource;
  wchar_t *wpSource;
  wchar_t *wpTarget=wszBuffer;
  wchar_t *wpTargetMax=wszBuffer + (wszBuffer?nBufferSize:0x7FFFFFFF);
  int nStringLen;

  //Expand environment strings
  nStringLen=ExpandEnvironmentStringsWide(wpString, NULL, 0);

  if (wszSource=(wchar_t *)GlobalAlloc(GPTR, nStringLen * sizeof(wchar_t)))
  {
    ExpandEnvironmentStringsWide(wpString, wszSource, nStringLen);

    //Expand plugin variables
    for (wpSource=wszSource; *wpSource && wpTarget < wpTargetMax;)
    {
      if (*wpSource == '%')
      {
        if (*++wpSource == '%')
        {
          ++wpSource;
          if (wszBuffer) *wpTarget='%';
          ++wpTarget;
        }
        else if (*wpSource == 'a' || *wpSource == 'A')
        {
          ++wpSource;
          wpTarget+=xstrcpynW(wszBuffer?wpTarget:NULL, wpExeDir, wpTargetMax - wpTarget) - !wszBuffer;
        }
      }
      else
      {
        if (wszBuffer) *wpTarget=*wpSource;
        ++wpTarget;
        ++wpSource;
      }
    }
    if (wpTarget < wpTargetMax)
    {
      if (wszBuffer)
        *wpTarget='\0';
      else
        ++wpTarget;
    }
    GlobalFree((HGLOBAL)wszSource);
  }
  return (int)(wpTarget - wszBuffer);
}

INCLUDEITEM* StackInsertInclude(INCLUDESTACK *hStack)
{
  INCLUDEITEM *lpElement=NULL;

  if (!StackInsertIndex((stack **)&hStack->first, (stack **)&hStack->last, (stack **)&lpElement, -1, sizeof(INCLUDEITEM)))
    ++hStack->nElements;

  return lpElement;
}

INCLUDEITEM* StackGetInclude(INCLUDESTACK *hStack, DWORD dwIndex)
{
  INCLUDEITEM *lpElement;
  DWORD dwCount=0;

  for (lpElement=hStack->first; lpElement; lpElement=lpElement->next)
  {
    if (++dwCount == dwIndex)
      break;
  }
  return lpElement;
}

void StackFreeIncludes(INCLUDESTACK *hStack)
{
  StackClear((stack **)&hStack->first, (stack **)&hStack->last);
  hStack->nElements=0;
}

POINTERITEM* StackInsertPointer(POINTERSTACK *hStack)
{
  POINTERITEM *lpElement=NULL;

  if (!StackInsertIndex((stack **)&hStack->first, (stack **)&hStack->last, (stack **)&lpElement, -1, sizeof(POINTERITEM)))
    ++hStack->nElements;

  return lpElement;
}

POINTERITEM* StackGetPointer(POINTERSTACK *hStack, void *lpData, INT_PTR nRange)
{
  POINTERITEM *lpElement;

  for (lpElement=hStack->first; lpElement; lpElement=lpElement->next)
  {
    if ((LPBYTE)lpData >= (LPBYTE)lpElement->lpData &&
        (LPBYTE)lpData + nRange <= (LPBYTE)lpElement->lpData + lpElement->dwSize)
    {
      break;
    }
  }
  return lpElement;
}

void StackDeletePointer(POINTERSTACK *hStack, POINTERITEM *lpPointer)
{
  if (!StackDelete((stack **)&hStack->first, (stack **)&hStack->last, (stack *)lpPointer))
    --hStack->nElements;
}

void StackFreePointers(POINTERSTACK *hStack)
{
  POINTERITEM *lpElement=(POINTERITEM *)hStack->first;

  while (lpElement)
  {
    if (lpElement->lpData) GlobalFree((HGLOBAL)lpElement->lpData);

    lpElement=lpElement->next;
  }
  StackClear((stack **)&hStack->first, (stack **)&hStack->last);
  hStack->nElements=0;
}

MSGINT* StackInsertMessage(MSGINTSTACK *hStack, UINT uMsg)
{
  MSGINT *lpElement=NULL;

  if (!StackInsertIndex((stack **)&hStack->first, (stack **)&hStack->last, (stack **)&lpElement, -1, sizeof(MSGINT)))
  {
    lpElement->uMsg=uMsg;
    ++hStack->nElements;
  }
  return lpElement;
}

MSGINT* StackGetMessage(MSGINTSTACK *hStack, UINT uMsg)
{
  MSGINT *lpElement;

  for (lpElement=hStack->first; lpElement; lpElement=lpElement->next)
  {
    if (lpElement->uMsg == uMsg)
      return lpElement;
  }
  return NULL;
}

void StackDeleteMessage(MSGINTSTACK *hStack, MSGINT *lpMessage)
{
  if (!StackDelete((stack **)&hStack->first, (stack **)&hStack->last, (stack *)lpMessage))
    --hStack->nElements;
}

void StackFreeMessages(MSGINTSTACK *hStack)
{
  StackClear((stack **)&hStack->first, (stack **)&hStack->last);
  hStack->nElements=0;
}

void StackFillMessages(MSGINTSTACK *hStack, SAFEARRAY *psa)
{
  VARIANT *pvtParameter;
  unsigned char *lpData;
  UINT_PTR dwParameter;
  DWORD dwElement;
  DWORD dwElementSum;

  lpData=(unsigned char *)(psa->pvData);
  dwElementSum=psa->rgsabound[0].cElements;

  if (dwElementSum)
  {
    for (dwElement=0; dwElement < dwElementSum; ++dwElement)
    {
      pvtParameter=(VARIANT *)(lpData + dwElement * sizeof(VARIANT));
      dwParameter=0;

      if (pvtParameter->vt == (VT_VARIANT|VT_BYREF))
        pvtParameter=pvtParameter->pvarVal;

      if (pvtParameter->vt == VT_BOOL)
        dwParameter=pvtParameter->boolVal?TRUE:FALSE;
      else
        dwParameter=GetVariantInt(pvtParameter);

      StackInsertMessage(hStack, (UINT)dwParameter);
    }
  }
}

CALLBACKITEM* StackInsertCallback(CALLBACKSTACK *hStack)
{
  CALLBACKITEM *lpElement;

  if (!StackInsertIndex((stack **)&hStack->first, (stack **)&hStack->last, (stack **)&lpElement, -1, sizeof(CALLBACKITEM)))
    lpElement->nStaticIndex=++hStack->nElements;

  return lpElement;
}

CALLBACKITEM* StackGetCallbackByHandle(CALLBACKSTACK *hStack, HANDLE hHandle)
{
  CALLBACKITEM *lpElement;

  for (lpElement=hStack->first; lpElement; lpElement=lpElement->next)
  {
    if (lpElement->hHandle == hHandle)
      return lpElement;
  }
  return NULL;
}

CALLBACKITEM* StackGetCallbackByObject(CALLBACKSTACK *hStack, IDispatch *objFunction)
{
  CALLBACKITEM *lpElement;

  for (lpElement=hStack->first; lpElement; lpElement=lpElement->next)
  {
    if (lpElement->objFunction == objFunction)
      return lpElement;
  }
  return NULL;
}

CALLBACKITEM* StackGetCallbackByIndex(CALLBACKSTACK *hStack, int nIndex)
{
  CALLBACKITEM *lpElement;
  int nCount=0;

  for (lpElement=hStack->first; lpElement; lpElement=lpElement->next)
  {
    if (nIndex == ++nCount)
      return lpElement;
  }
  return NULL;
}

void StackDeleteCallback(CALLBACKSTACK *hStack, CALLBACKITEM *lpElement)
{
  StackFreeMessages(&lpElement->hMsgIntStack);
  if (!StackDelete((stack **)&hStack->first, (stack **)&hStack->last, (stack *)lpElement))
    --hStack->nElements;
}

void StackFreeCallbacks(CALLBACKSTACK *hStack)
{
  CALLBACKITEM *lpElement;
  CALLBACKITEM *lpNextElement;

  for (lpElement=hStack->first; lpElement; lpElement=lpNextElement)
  {
    lpNextElement=lpElement->next;

    if (lpElement->nCallbackType == CIT_MAINPROC ||
        lpElement->nCallbackType == CIT_EDITPROC ||
        lpElement->nCallbackType == CIT_FRAMEPROC)
    {
      Document_WindowUnsubClass(NULL, lpElement->hHandle);
    }
    StackFreeMessages(&lpElement->hMsgIntStack);
  }
  StackClear((stack **)&hStack->first, (stack **)&hStack->last);
  hStack->nElements=0;
}

LRESULT CALLBACK DialogCallbackProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  SCRIPTTHREAD *lpScriptThread=StackGetScriptThreadCurrent();
  CALLBACKITEM *lpCallback=NULL;
  LRESULT lResult=0;

  if (uMsg == WM_CREATE)
  {
    if (lParam)
    {
      CREATESTRUCTA *cs=(CREATESTRUCTA *)lParam;

      if (lpCallback=StackInsertCallback(&lpScriptThread->hDialogCallbackStack))
      {
        lpCallback->hHandle=(HANDLE)hWnd;
        lpCallback->objFunction=(IDispatch *)cs->lpCreateParams;
        lpCallback->lpScriptThread=(void *)lpScriptThread;
        lpCallback->nCallbackType=CIT_DIALOG;
      }
    }
    SendMessage(hWnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)g_hPluginIcon);
  }
  else
  {
    if (lpCallback=StackGetCallbackByHandle(&lpScriptThread->hDialogCallbackStack, hWnd))
    {
      if (uMsg == WM_DESTROY)
        lpCallback->hHandle=NULL;
    }
  }

  if (lpCallback)
  {
    if (lpCallback->objFunction)
    {
      //Message procedure in script
      lpScriptThread->bBusy=TRUE;
      CallScriptProc(lpCallback->objFunction, hWnd, uMsg, wParam, lParam, &lResult);
      lpScriptThread->bBusy=FALSE;
    }
    if (uMsg == WM_DESTROY)
      StackDeleteCallback(&lpScriptThread->hDialogCallbackStack, lpCallback);
    if (lResult)
      return lResult;
  }

  return DefWindowProcWide(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK SubclassCallbackProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;
  LRESULT lResult=0;

  if (lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, hWnd))
  {
    lResult=SubclassSend(lpCallback, hWnd, uMsg, wParam, lParam);

    if (lpCallback->bNoNextProc)
      lpCallback->bNoNextProc=FALSE;
    else
    {
      if (bOldWindows)
        lResult=CallWindowProcA((WNDPROC)lpCallback->dwData, hWnd, uMsg, wParam, lParam);
      else
        lResult=CallWindowProcW((WNDPROC)lpCallback->dwData, hWnd, uMsg, wParam, lParam);
    }
  }
  return lResult;
}

LRESULT CALLBACK SubclassMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;
  LRESULT lResult;

  if (lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, (HANDLE)(INT_PTR)WSC_MAINPROC))
  {
    lResult=SubclassSend(lpCallback, hWnd, uMsg, wParam, lParam);

    if (lpCallback->bNoNextProc)
      lpCallback->bNoNextProc=FALSE;
    else
      lResult=g_lpSubclassMainProc->NextProc(hWnd, uMsg, wParam, lParam);
  }
  return lResult;
}

LRESULT CALLBACK SubclassEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;
  LRESULT lResult;

  if (lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, (HANDLE)(INT_PTR)WSC_EDITPROC))
  {
    lResult=SubclassSend(lpCallback, hWnd, uMsg, wParam, lParam);

    if (lpCallback->bNoNextProc)
      lpCallback->bNoNextProc=FALSE;
    else
      lResult=g_lpSubclassEditProc->NextProc(hWnd, uMsg, wParam, lParam);
  }
  return lResult;
}

LRESULT CALLBACK SubclassFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;
  LRESULT lResult;

  if (lpCallback=StackGetCallbackByHandle(&g_hSubclassCallbackStack, (HANDLE)(INT_PTR)WSC_FRAMEPROC))
  {
    lResult=SubclassSend(lpCallback, hWnd, uMsg, wParam, lParam);

    if (lpCallback->bNoNextProc)
      lpCallback->bNoNextProc=FALSE;
    else
      lResult=g_lpSubclassFrameProc->NextProc(hWnd, uMsg, wParam, lParam);
  }
  return lResult;
}

LRESULT CALLBACK SubclassSend(CALLBACKITEM *lpCallback, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)lpCallback->lpScriptThread;
  MSGSEND msgs;

  //Because objFunction->lpVtbl->Invoke cause error for different thread, we send message from this thread to hWndScriptsThreadDummy.
  if (lpScriptThread->bMessageLoop)
  {
    if (!lpCallback->hMsgIntStack.nElements || StackGetMessage(&lpCallback->hMsgIntStack, uMsg))
    {
      msgs.lpCallback=lpCallback;
      msgs.hWnd=hWnd;
      msgs.uMsg=uMsg;
      msgs.wParam=wParam;
      msgs.lParam=lParam;
      return SendMessage(lpScriptThread->hWndScriptsThreadDummy, AKDLL_SUBCLASSSEND, 0, (LPARAM)&msgs);
    }
  }
  return 0;
}

LRESULT CALLBACK Hook1CallbackProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;

  lpCallback=StackGetCallbackByIndex(&g_hHookCallbackStack, 1);
  return HookCommonCallbackProc(lpCallback, nCode, wParam, lParam);
}

LRESULT CALLBACK Hook2CallbackProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;

  lpCallback=StackGetCallbackByIndex(&g_hHookCallbackStack, 2);
  return HookCommonCallbackProc(lpCallback, nCode, wParam, lParam);
}

LRESULT CALLBACK Hook3CallbackProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;

  lpCallback=StackGetCallbackByIndex(&g_hHookCallbackStack, 3);
  return HookCommonCallbackProc(lpCallback, nCode, wParam, lParam);
}

LRESULT CALLBACK Hook4CallbackProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  CALLBACKITEM *lpCallback;

  lpCallback=StackGetCallbackByIndex(&g_hHookCallbackStack, 4);
  return HookCommonCallbackProc(lpCallback, nCode, wParam, lParam);
}

LRESULT CALLBACK HookCommonCallbackProc(CALLBACKITEM *lpCallback, int nCode, WPARAM wParam, LPARAM lParam)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)lpCallback->lpScriptThread;

  if (lpCallback)
  {
    if (nCode >= 0)
    {
      MSGSEND msgs;

      //Because objFunction->lpVtbl->Invoke cause error for different thread, we send message from this thread to hWndScriptsThreadDummy.
      if (lpScriptThread->bMessageLoop)
      {
        msgs.lpCallback=lpCallback;
        msgs.hWnd=NULL;
        msgs.uMsg=nCode;
        msgs.wParam=wParam;
        msgs.lParam=lParam;
        SendMessage(lpScriptThread->hWndScriptsThreadDummy, AKDLL_HOOKSEND, 0, (LPARAM)&msgs);
      }
    }
    return CallNextHookEx((HHOOK)lpCallback->hHandle, nCode, wParam, lParam);
  }
  return 0;
}

HWND CreateScriptsThreadDummyWindow()
{
  WNDCLASSW wndclassW;

  wndclassW.style        =0;
  wndclassW.lpfnWndProc  =ScriptsThreadProc;
  wndclassW.cbClsExtra   =0;
  wndclassW.cbWndExtra   =DLGWINDOWEXTRA;
  wndclassW.hInstance    =hInstanceDLL;
  wndclassW.hIcon        =NULL;
  wndclassW.hCursor      =NULL;
  wndclassW.hbrBackground=NULL;
  wndclassW.lpszMenuName =NULL;
  wndclassW.lpszClassName=L"ScriptsThread";
  RegisterClassWide(&wndclassW);

  return CreateWindowExWide(0, L"ScriptsThread", L"", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstanceDLL, NULL);
}

LRESULT CALLBACK ScriptsThreadProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == AKDLL_SUBCLASSSEND)
  {
    MSGSEND *msgs=(MSGSEND *)lParam;
    SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)msgs->lpCallback->lpScriptThread;
    LRESULT lResult=0;

    //Now we in plugin thread - call procedure in script
    if (lpScriptThread && lpScriptThread->bMessageLoop)
    {
      lpScriptThread->bBusy=TRUE;
      CallScriptProc(msgs->lpCallback->objFunction, (HWND)msgs->hWnd, msgs->uMsg, msgs->wParam, msgs->lParam, &lResult);
      lpScriptThread->bBusy=FALSE;
    }
    return lResult;
  }
  else if (uMsg == AKDLL_HOOKSEND)
  {
    MSGSEND *msgs=(MSGSEND *)lParam;
    SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)msgs->lpCallback->lpScriptThread;
    LRESULT lResult=0;

    //Now we in plugin thread - call procedure in script
    if (lpScriptThread && lpScriptThread->bMessageLoop)
    {
      lpScriptThread->bBusy=TRUE;
      CallScriptProc(msgs->lpCallback->objFunction, NULL, msgs->uMsg, msgs->wParam, msgs->lParam, &lResult);
      lpScriptThread->bBusy=FALSE;
    }
    return lResult;
  }
  else if (uMsg == AKDLL_CALLBACKSEND)
  {
    MSGSEND *msgs=(MSGSEND *)lParam;
    SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)msgs->lpCallback->lpScriptThread;
    DISPPARAMS *dispp=(DISPPARAMS *)msgs->lParam;
    VARIANT vtInvoke;
    LRESULT lResult=0;

    //Now we in plugin thread - call procedure in script
    lpScriptThread->bBusy=TRUE;

    if (msgs->lpCallback->objFunction->lpVtbl->Invoke(msgs->lpCallback->objFunction, DISPID_VALUE, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, dispp, &vtInvoke, 0, 0) == S_OK)
    {
      if (vtInvoke.vt == VT_BOOL)
        lResult=vtInvoke.boolVal?TRUE:FALSE;
      else
        lResult=(int)GetVariantInt(&vtInvoke);
    }

    lpScriptThread->bBusy=FALSE;
    return lResult;
  }
  else if (uMsg == AKDLL_POSTQUIT)
  {
    PostQuitMessage(0);
  }

  return DefWindowProcWide(hWnd, uMsg, wParam, lParam);
}

HRESULT CallScriptProc(IDispatch *objFunction, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
  DISPPARAMS dispp;
  VARIANT vtArg[4];
  VARIANT vtInvoke;
  HRESULT hr;

  //if (g_MainMessageBox)
  //  return E_FAIL;
  if (!objFunction)
    return E_FAIL;

  #ifdef _WIN64
    vtArg[0].llVal=lParam;
    vtArg[0].vt=VT_I8;
    vtArg[1].llVal=(INT_PTR)wParam;
    vtArg[1].vt=VT_I8;
    vtArg[2].lVal=uMsg;
    vtArg[2].vt=VT_I4;
    if (hWnd)
    {
      vtArg[3].llVal=(INT_PTR)hWnd;
      vtArg[3].vt=VT_I8;
    }
  #else
    //Use VT_I4 because VBScript cause error
    vtArg[0].lVal=lParam;
    vtArg[0].vt=VT_I4;
    vtArg[1].lVal=(int)wParam;
    vtArg[1].vt=VT_I4;
    vtArg[2].lVal=uMsg;
    vtArg[2].vt=VT_I4;
    if (hWnd)
    {
      vtArg[3].lVal=(int)hWnd;
      vtArg[3].vt=VT_I4;
    }
  #endif

  xmemset(&dispp, 0, sizeof(DISPPARAMS));
  dispp.cArgs=hWnd?4:3;
  dispp.rgvarg=vtArg;

  if ((hr=objFunction->lpVtbl->Invoke(objFunction, DISPID_VALUE, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispp, &vtInvoke, 0, 0)) == S_OK)
  {
    if (vtInvoke.vt == VT_BOOL)
      *lResult=vtInvoke.boolVal?TRUE:FALSE;
    else
      *lResult=(LRESULT)GetVariantInt(&vtInvoke);
  }
  return hr;
}
