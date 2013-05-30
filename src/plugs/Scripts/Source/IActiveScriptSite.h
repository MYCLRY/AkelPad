#ifndef _IACTIVESCRIPTSITE_H_
#define _IACTIVESCRIPTSITE_H_

//Defines
typedef struct {
  IActiveScriptSite site;
  IActiveScriptSiteWindow siteWnd;
} MyRealIActiveScriptSite;

//Global variables
extern MyRealIActiveScriptSite MyActiveScriptSite;

//Functions prototypes
HRESULT ExecScriptText(void *lpScriptThread, GUID *guidEngine, const wchar_t *wpScriptText);
HRESULT GetScriptEngineA(char *szExt, GUID *guidEngine);
int CBTMessageBox(HWND hWnd, const wchar_t *wpText, const wchar_t *wpCaption, UINT uType);
LRESULT CALLBACK CBTMessageBoxProc(INT nCode, WPARAM wParam, LPARAM lParam);
void InitIActiveScriptSiteObject(void);

STDMETHODIMP QueryInterface(IActiveScriptSite *, REFIID, void **);
STDMETHODIMP_(ULONG) AddRef(IActiveScriptSite *);
STDMETHODIMP_(ULONG) Release(IActiveScriptSite *);
STDMETHODIMP GetLCID(IActiveScriptSite *, LCID *);
STDMETHODIMP GetItemInfo(IActiveScriptSite *, LPCOLESTR, DWORD, IUnknown **, ITypeInfo **);
STDMETHODIMP GetDocVersionString(IActiveScriptSite *, BSTR *);
STDMETHODIMP OnScriptTerminate(IActiveScriptSite *, const VARIANT *, const EXCEPINFO *);
STDMETHODIMP OnStateChange(IActiveScriptSite *, SCRIPTSTATE);
STDMETHODIMP OnScriptError(IActiveScriptSite *, IActiveScriptError *);
STDMETHODIMP OnEnterScript(IActiveScriptSite *);
STDMETHODIMP OnLeaveScript(IActiveScriptSite *);

STDMETHODIMP siteWnd_QueryInterface(IActiveScriptSiteWindow *, REFIID, void **);
STDMETHODIMP_(ULONG) siteWnd_AddRef(IActiveScriptSiteWindow *);
STDMETHODIMP_(ULONG) siteWnd_Release(IActiveScriptSiteWindow *);
STDMETHODIMP GetSiteWindow(IActiveScriptSiteWindow *, HWND *);
STDMETHODIMP EnableModeless(IActiveScriptSiteWindow *, BOOL);

#endif
