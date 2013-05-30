#ifndef _IWSCRIPT_H_
#define _IWSCRIPT_H_

//Defines
typedef struct {
  IWScriptVtbl *lpVtbl;
  DWORD dwCount;
  void *lpScriptThread;
} IRealWScript;


//Global variables
extern ITypeInfo *g_WScriptTypeInfo;
extern const IWScriptVtbl MyIWScriptVtbl;

//Functions prototypes
HRESULT STDMETHODCALLTYPE WScript_QueryInterface(IWScript *this, REFIID vTableGuid, void **ppv);
ULONG STDMETHODCALLTYPE WScript_AddRef(IWScript *this);
ULONG STDMETHODCALLTYPE WScript_Release(IWScript *this);

HRESULT STDMETHODCALLTYPE WScript_GetTypeInfoCount(IWScript *this, UINT *pCount);
HRESULT STDMETHODCALLTYPE WScript_GetTypeInfo(IWScript *this, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo);
HRESULT STDMETHODCALLTYPE WScript_GetIDsOfNames(IWScript *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
HRESULT STDMETHODCALLTYPE WScript_Invoke(IWScript *this, DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr);

HRESULT STDMETHODCALLTYPE WScript_Arguments(IWScript *this, IDispatch **objWArguments);
HRESULT STDMETHODCALLTYPE WScript_ScriptFullName(IWScript *this, BSTR *wpScriptFullName);
HRESULT STDMETHODCALLTYPE WScript_ScriptName(IWScript *this, BSTR *wpScriptName);
HRESULT STDMETHODCALLTYPE WScript_ScriptBaseName(IWScript *this, BSTR *wpScriptBaseName);
HRESULT STDMETHODCALLTYPE WScript_FullName(IWScript *this, BSTR *wpDllFullName);
HRESULT STDMETHODCALLTYPE WScript_Path(IWScript *this, BSTR *wpDllPath);
HRESULT STDMETHODCALLTYPE WScript_Name(IWScript *this, BSTR *wpDllName);
HRESULT STDMETHODCALLTYPE WScript_Echo(IWScript *this, BSTR wpText);
HRESULT STDMETHODCALLTYPE WScript_Sleep(IWScript *this, int nTime);
HRESULT STDMETHODCALLTYPE WScript_Quit(IWScript *this, int nErrorCode);

#endif
