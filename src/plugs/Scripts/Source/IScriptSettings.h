#ifndef _ISCRIPTSETTINGS_H_
#define _ISCRIPTSETTINGS_H_

//Defines
typedef struct {
  const IScriptSettingsVtbl *lpVtbl;
  DWORD dwCount;
  void *lpScriptThread;
  HANDLE hOptions;
} IRealScriptSettings;

//Global variables
extern ITypeInfo *g_ScriptSettingsTypeInfo;
extern const IScriptSettingsVtbl MyIScriptSettingsVtbl;

//Functions prototypes
HRESULT STDMETHODCALLTYPE ScriptSettings_QueryInterface(IScriptSettings *this, REFIID vTableGuid, void **ppv);
ULONG STDMETHODCALLTYPE ScriptSettings_AddRef(IScriptSettings *this);
ULONG STDMETHODCALLTYPE ScriptSettings_Release(IScriptSettings *this);

HRESULT STDMETHODCALLTYPE ScriptSettings_GetTypeInfoCount(IScriptSettings *this, UINT *pCount);
HRESULT STDMETHODCALLTYPE ScriptSettings_GetTypeInfo(IScriptSettings *this, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo);
HRESULT STDMETHODCALLTYPE ScriptSettings_GetIDsOfNames(IScriptSettings *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
HRESULT STDMETHODCALLTYPE ScriptSettings_Invoke(IScriptSettings *this, DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr);

HRESULT STDMETHODCALLTYPE ScriptSettings_Begin(IScriptSettings *this, BSTR wpScriptName, DWORD dwFlags, HANDLE *hOptions);
HRESULT STDMETHODCALLTYPE ScriptSettings_Read(IScriptSettings *this, BSTR wpOptionName, DWORD dwType, VARIANT vtDefault, VARIANT *vtData);
HRESULT STDMETHODCALLTYPE ScriptSettings_Write(IScriptSettings *this, BSTR wpOptionName, DWORD dwType, VARIANT vtData, int nDataSize, int *nResult);
HRESULT STDMETHODCALLTYPE ScriptSettings_Delete(IScriptSettings *this, BSTR wpOptionName, BOOL *bResult);
HRESULT STDMETHODCALLTYPE ScriptSettings_End(IScriptSettings *this, BOOL *bResult);

#endif
