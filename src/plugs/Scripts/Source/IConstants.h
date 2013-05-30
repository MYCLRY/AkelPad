#ifndef _ICONSTANTS_H_
#define _ICONSTANTS_H_

//Defines
typedef struct {
  IConstantsVtbl *lpVtbl;
  DWORD dwCount;
  void *lpScriptThread;
} IRealConstants;

//Global variables
extern ITypeInfo *g_ConstantsTypeInfo;
extern const IConstantsVtbl MyIConstantsVtbl;

//Functions prototypes
HRESULT STDMETHODCALLTYPE Constants_QueryInterface(IConstants *this, REFIID vTableGuid, void **ppv);
ULONG STDMETHODCALLTYPE Constants_AddRef(IConstants *this);
ULONG STDMETHODCALLTYPE Constants_Release(IConstants *this);

HRESULT STDMETHODCALLTYPE Constants_GetTypeInfoCount(IConstants *this, UINT *pCount);
HRESULT STDMETHODCALLTYPE Constants_GetTypeInfo(IConstants *this, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo);
HRESULT STDMETHODCALLTYPE Constants_GetIDsOfNames(IConstants *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
HRESULT STDMETHODCALLTYPE Constants_Invoke(IConstants *this, DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr);

HRESULT STDMETHODCALLTYPE Constants_TCHAR(IConstants *this, BSTR *wpChar);
HRESULT STDMETHODCALLTYPE Constants_vbTCHAR(IConstants *this, BSTR *wpChar);
HRESULT STDMETHODCALLTYPE Constants_TSTR(IConstants *this, int *nStr);
HRESULT STDMETHODCALLTYPE Constants_vbTSTR(IConstants *this, int *nStr);
HRESULT STDMETHODCALLTYPE Constants_TSIZE(IConstants *this, int *nSize);
HRESULT STDMETHODCALLTYPE Constants_vbTSIZE(IConstants *this, int *nSize);
HRESULT STDMETHODCALLTYPE Constants_X64(IConstants *this, BOOL *b64);
HRESULT STDMETHODCALLTYPE Constants_vbX64(IConstants *this, BOOL *b64);

#endif
