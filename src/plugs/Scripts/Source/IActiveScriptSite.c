#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <activscp.h>
#include <stddef.h>
#include "Scripts.h"


//Global variables
const IActiveScriptSiteVtbl SiteTable={
  QueryInterface,
  AddRef,
  Release,
  GetLCID,
  GetItemInfo,
  GetDocVersionString,
  OnScriptTerminate,
  OnStateChange,
  OnScriptError,
  OnEnterScript,
  OnLeaveScript
};

const IActiveScriptSiteWindowVtbl SiteWindowTable={
  siteWnd_QueryInterface,
  siteWnd_AddRef,
  siteWnd_Release,
  GetSiteWindow,
  EnableModeless
};

MyRealIActiveScriptSite MyActiveScriptSite;
HHOOK hHookMessageBox;


//// Script text execution

HRESULT ExecScriptText(void *lpScriptThread, GUID *guidEngine, const wchar_t *wpScriptText)
{
  SCRIPTTHREAD *st=(SCRIPTTHREAD *)lpScriptThread;
  HRESULT nCoInit;
  HRESULT nResult=E_FAIL;

  nCoInit=CoInitialize(0);

  if (CoCreateInstance(guidEngine, 0, CLSCTX_INPROC_SERVER, &IID_IActiveScript, (void **)&st->objActiveScript) == S_OK)
  {
    if (st->objActiveScript->lpVtbl->QueryInterface(st->objActiveScript, &IID_IActiveScriptParse, (void **)&st->objActiveScriptParse) == S_OK)
    {
      if (st->objActiveScriptParse->lpVtbl->InitNew(st->objActiveScriptParse) == S_OK)
      {
        if (st->objActiveScript->lpVtbl->SetScriptSite(st->objActiveScript, (IActiveScriptSite *)&MyActiveScriptSite) == S_OK)
        {
          if (st->objActiveScript->lpVtbl->AddNamedItem(st->objActiveScript, L"WScript", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE) == S_OK &&
              st->objActiveScript->lpVtbl->AddNamedItem(st->objActiveScript, L"AkelPad", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE) == S_OK &&
              st->objActiveScript->lpVtbl->AddNamedItem(st->objActiveScript, L"Constants", SCRIPTITEM_GLOBALMEMBERS|SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE) == S_OK)
          {
            if (st->objActiveScriptParse->lpVtbl->ParseScriptText(st->objActiveScriptParse, wpScriptText, NULL, NULL, NULL, 0, 0, 0, NULL, NULL) == S_OK)
            {
              st->objActiveScript->lpVtbl->SetScriptState(st->objActiveScript, SCRIPTSTATE_CONNECTED);
              nResult=S_OK;
            }
          }
        }
      }
      st->objActiveScriptParse->lpVtbl->Release(st->objActiveScriptParse);
    }
    st->objActiveScript->lpVtbl->Close(st->objActiveScript);
    st->objActiveScript->lpVtbl->Release(st->objActiveScript);
  }
  if (nCoInit == S_OK)
    CoUninitialize();

  return nResult;
}

HRESULT GetScriptEngineA(char *szExt, GUID *guidEngine)
{
  char szValue[MAX_PATH];
  wchar_t wszValue[MAX_PATH];
  HKEY hKey;
  HKEY hSubKey;
  DWORD dwType;
  DWORD dwSize;
  int nQuery;
  HRESULT nResult=E_FAIL;

  if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szExt, 0, KEY_QUERY_VALUE|KEY_READ, &hKey) == ERROR_SUCCESS)
  {
    dwSize=sizeof(szValue);
    nQuery=RegQueryValueExA(hKey, NULL, 0, &dwType, (LPBYTE)szValue, &dwSize);
    RegCloseKey(hKey);

    if (nQuery == ERROR_SUCCESS)
    {
      Loop:

      if (RegOpenKeyExA(HKEY_CLASSES_ROOT, (LPCSTR)szValue, 0, KEY_QUERY_VALUE|KEY_READ, &hKey) == ERROR_SUCCESS)
      {
        if (RegOpenKeyExA(hKey, "CLSID", 0, KEY_QUERY_VALUE|KEY_READ, &hSubKey) == ERROR_SUCCESS)
        {
          dwSize=sizeof(szValue);
          nQuery=RegQueryValueExA(hSubKey, 0, 0, &dwType, (LPBYTE)szValue, &dwSize);
          RegCloseKey(hSubKey);

          MultiByteToWideChar(CP_ACP, 0, szValue, -1, wszValue, MAX_PATH);
          if (CLSIDFromString(wszValue, guidEngine) == NOERROR)
            nResult=S_OK;
        }
        else if (szExt)
        {
          if (RegOpenKeyExA(hKey, "ScriptEngine", 0, KEY_QUERY_VALUE|KEY_READ, &hSubKey) == ERROR_SUCCESS)
          {
            dwSize=sizeof(szValue);
            nQuery=RegQueryValueExA(hSubKey, 0, 0, &dwType, (LPBYTE)szValue, &dwSize);
            RegCloseKey(hSubKey);

            if (nQuery == ERROR_SUCCESS)
            {
              RegCloseKey(hKey);
              szExt=NULL;
              goto Loop;
            }
          }
        }
      }
      RegCloseKey(hKey);
    }
  }
  return nResult;
}

//MessageBox with custom button text
int CBTMessageBox(HWND hWnd, const wchar_t *wpText, const wchar_t *wpCaption, UINT uType)
{
  hHookMessageBox=SetWindowsHookEx(WH_CBT, &CBTMessageBoxProc, 0, GetCurrentThreadId());
  return MessageBoxW(hWnd, wpText, wpCaption, uType);
}

LRESULT CALLBACK CBTMessageBoxProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode == HCBT_ACTIVATE)
  {
    if (GetDlgItem((HWND)wParam, IDYES))
      SetDlgItemTextWide((HWND)wParam, IDYES, GetLangStringW(wLangModule, STRID_STOP));
    if (GetDlgItem((HWND)wParam, IDNO))
      SetDlgItemTextWide((HWND)wParam, IDNO, GetLangStringW(wLangModule, STRID_EDIT));
    if (GetDlgItem((HWND)wParam, IDCANCEL))
      SetDlgItemTextWide((HWND)wParam, IDCANCEL, GetLangStringW(wLangModule, STRID_CONTINUE));
    UnhookWindowsHookEx(hHookMessageBox);
    hHookMessageBox=NULL;
    return 0;
  }
  else return CallNextHookEx(hHookMessageBox, nCode, wParam, lParam);
}

// Initialize ActiveScriptSite object
void InitIActiveScriptSiteObject(void)
{
  MyActiveScriptSite.site.lpVtbl=(IActiveScriptSiteVtbl *)&SiteTable;
  MyActiveScriptSite.siteWnd.lpVtbl=(IActiveScriptSiteWindowVtbl *)&SiteWindowTable;
}


//// IActiveScriptSite implementation

STDMETHODIMP QueryInterface(IActiveScriptSite *this, REFIID riid, void **ppv)
{
  if (AKD_IsEqualIID(riid, &IID_IUnknown) || AKD_IsEqualIID(riid, &IID_IActiveScriptSite))
    *ppv=this;
  else if (AKD_IsEqualIID(riid, &IID_IActiveScriptSiteWindow))
    *ppv=((unsigned char *)this + offsetof(MyRealIActiveScriptSite, siteWnd));
  else
  {
    *ppv=0;
    return(E_NOINTERFACE);
  }

  AddRef(this);
  return(S_OK);
}

STDMETHODIMP_(ULONG) AddRef(IActiveScriptSite *this)
{
  return(1);
}

STDMETHODIMP_(ULONG) Release(IActiveScriptSite *this)
{
  return(1);
}

STDMETHODIMP GetItemInfo(IActiveScriptSite *this, LPCOLESTR objectName, DWORD dwReturnMask, IUnknown **objPtr, ITypeInfo **typeInfo)
{
  HRESULT hr=E_FAIL;

  if (dwReturnMask & SCRIPTINFO_IUNKNOWN) *objPtr=0;
  if (dwReturnMask & SCRIPTINFO_ITYPEINFO) *typeInfo=0;

  if (!xstrcmpiW(objectName, L"WScript"))
  {
    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
      hr=Class_CreateInstance(NULL, NULL, &IID_IWScript, (void **)objPtr);
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
      hr=WScript_GetTypeInfo(NULL, 0, 0, typeInfo);
  }
  else if (!xstrcmpiW(objectName, L"AkelPad"))
  {
    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
      hr=Class_CreateInstance(NULL, NULL, &IID_IDocument, (void **)objPtr);
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
      hr=Document_GetTypeInfo(NULL, 0, 0, typeInfo);
  }
  else if (!xstrcmpiW(objectName, L"Constants"))
  {
    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
      hr=Class_CreateInstance(NULL, NULL, &IID_IConstants, (void **)objPtr);
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
      hr=Constants_GetTypeInfo(NULL, 0, 0, typeInfo);
  }
  return(hr);
}

STDMETHODIMP OnScriptError(IActiveScriptSite *this, IActiveScriptError *scriptError)
{
  SCRIPTTHREAD *lpScriptThread;
  wchar_t wszScriptFile[MAX_PATH];
  INCLUDEITEM *lpIncludeItem=NULL;
  DWORD dwIncludeIndex=0;
  ULONG nLine;
  LONG nChar;
  EXCEPINFO ei;

  if (lpScriptThread=StackGetScriptThreadCurrent())
  {
    //Get message text
    xmemset(&ei, 0, sizeof(EXCEPINFO));
    scriptError->lpVtbl->GetSourcePosition(scriptError, &dwIncludeIndex, &nLine, &nChar);
    scriptError->lpVtbl->GetExceptionInfo(scriptError, &ei);
    if (!*wszErrorMsg)
    {
      if (ei.bstrDescription)
        xstrcpynW(wszErrorMsg, ei.bstrDescription, MAX_PATH);
    }
    if (dwIncludeIndex)
      lpIncludeItem=StackGetInclude(&lpScriptThread->hIncludesStack, dwIncludeIndex);
    if (lpIncludeItem)
      xstrcpynW(wszScriptFile, lpIncludeItem->wszInclude, MAX_PATH);
    else
      xstrcpynW(wszScriptFile, lpScriptThread->wszScriptFile, MAX_PATH);

    xprintfW(wszBuffer, GetLangStringW(wLangModule, STRID_SCRIPTERROR),
                        wszScriptFile,
                        nLine + 1,
                        nChar + 1,
                        wszErrorMsg,
                        ei.scode,
                        ei.bstrSource?ei.bstrSource:L"");

    wszErrorMsg[0]='\0';
    SysFreeString(ei.bstrSource);
    SysFreeString(ei.bstrDescription);
    SysFreeString(ei.bstrHelpFile);

    //Show message
    {
      EDITINFO ei;
      int nChoice;
      int nOpenResult;

      nChoice=CBTMessageBox(IsWindowEnabled(hMainWnd)?hMainWnd:NULL, wszBuffer, wszPluginTitle, (lpScriptThread->hDialogCallbackStack.first?MB_YESNOCANCEL:MB_YESNO)|MB_ICONERROR);

      if (nChoice == IDYES || //"Stop"
          nChoice == IDNO)    //"Edit"
      {
        lpScriptThread->bStopped=TRUE;

        if (!IsWindowEnabled(hMainWnd))
          EnableWindow(hMainWnd, TRUE);

        //Stop script
        //CloseScriptWindows(lpScriptThread);
        if (lpScriptThread->bMessageLoop)
          PostQuitMessage(0);
        lpScriptThread->objActiveScript->lpVtbl->Close(lpScriptThread->objActiveScript);

        if (nChoice == IDNO) //"Edit"
        {
          Document_OpenFile(NULL, wszScriptFile, OD_ADT_BINARY_ERROR|OD_ADT_DETECT_CODEPAGE|OD_ADT_DETECT_BOM, 0, 0, &nOpenResult);

          if (nOpenResult == EOD_SUCCESS || (nMDI != WMD_SDI && nOpenResult == EOD_WINDOW_EXIST))
          {
            if (SendMessage(hMainWnd, AKD_GETEDITINFO, (WPARAM)NULL, (LPARAM)&ei))
            {
              CHARRANGE64 cr;
              int nLockScroll;

              if (bAkelEdit)
              {
                if ((nLockScroll=(int)SendMessage(ei.hWndEdit, AEM_LOCKSCROLL, (WPARAM)-1, 0)) == -1)
                  SendMessage(ei.hWndEdit, AEM_LOCKSCROLL, SB_BOTH, TRUE);
              }
              nLine=(int)SendMessage(ei.hWndEdit, AEM_GETWRAPLINE, (WPARAM)nLine, (LPARAM)NULL);
              cr.cpMin=SendMessage(ei.hWndEdit, EM_LINEINDEX, (WPARAM)nLine, 0) + nChar;
              cr.cpMax=cr.cpMin;
              SendMessage(ei.hWndEdit, EM_EXSETSEL64, 0, (LPARAM)&cr);
              if (bAkelEdit && nLockScroll == -1)
              {
                SendMessage(ei.hWndEdit, AEM_LOCKSCROLL, SB_BOTH, FALSE);
                ScrollCaret(ei.hWndEdit);
              }
            }
          }
        }
        InvalidateRect(hMainWnd, NULL, FALSE);
      }
      else if (nChoice == IDCANCEL) //"Continue"
      {
      }
    }
  }
  return(S_OK);
}

STDMETHODIMP GetLCID(IActiveScriptSite *this, LCID *lcid)
{
  *lcid=LOCALE_USER_DEFAULT;
  return(S_OK);
}

STDMETHODIMP GetDocVersionString(IActiveScriptSite *this, BSTR *version)
{
  *version=0;
  return(S_OK);
}

STDMETHODIMP OnScriptTerminate(IActiveScriptSite *this, const VARIANT *pvr, const EXCEPINFO *pei)
{
  return(S_OK);
}

STDMETHODIMP OnStateChange(IActiveScriptSite *this, SCRIPTSTATE state)
{
  return(S_OK);
}

STDMETHODIMP OnEnterScript(IActiveScriptSite *this)
{
  return(S_OK);
}

STDMETHODIMP OnLeaveScript(IActiveScriptSite *this)
{
  return(S_OK);
}


//// IActiveScriptSiteWindow implementation

STDMETHODIMP siteWnd_QueryInterface(IActiveScriptSiteWindow *this, REFIID riid, void **ppv)
{
  this=(IActiveScriptSiteWindow *)(((unsigned char *)this - offsetof(MyRealIActiveScriptSite, siteWnd)));
  return(QueryInterface((IActiveScriptSite *)this, riid, ppv));
}

STDMETHODIMP_(ULONG) siteWnd_AddRef(IActiveScriptSiteWindow *this)
{
  this=(IActiveScriptSiteWindow *)(((unsigned char *)this - offsetof(MyRealIActiveScriptSite, siteWnd)));
  return(AddRef((IActiveScriptSite *)this));
}

STDMETHODIMP_(ULONG) siteWnd_Release(IActiveScriptSiteWindow *this)
{
  this=(IActiveScriptSiteWindow *)(((unsigned char *)this - offsetof(MyRealIActiveScriptSite, siteWnd)));
  return(Release((IActiveScriptSite *)this));
}

STDMETHODIMP GetSiteWindow(IActiveScriptSiteWindow *this, HWND *phwnd)
{
  *phwnd=hMainWnd;
  return(S_OK);
}

STDMETHODIMP EnableModeless(IActiveScriptSiteWindow *this, BOOL enable)
{
  return(S_OK);
}
