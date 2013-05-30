#ifndef _IWARGUMENTS_H_
#define _IWARGUMENTS_H_

//Defines
typedef struct {
  IWArgumentsVtbl *lpVtbl;
  DWORD dwCount;
  void *lpScriptThread;
} IRealWArguments;

//Global variables
extern ITypeInfo *g_WArgumentsTypeInfo;
extern const IWArgumentsVtbl MyIWArgumentsVtbl;

//Functions prototypes
HRESULT STDMETHODCALLTYPE WArguments_QueryInterface(IWArguments *this, REFIID vTableGuid, void **ppv);
ULONG STDMETHODCALLTYPE WArguments_AddRef(IWArguments *this);
ULONG STDMETHODCALLTYPE WArguments_Release(IWArguments *this);

HRESULT STDMETHODCALLTYPE WArguments_GetTypeInfoCount(IWArguments *this, UINT *pCount);
HRESULT STDMETHODCALLTYPE WArguments_GetTypeInfo(IWArguments *this, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo);
HRESULT STDMETHODCALLTYPE WArguments_GetIDsOfNames(IWArguments *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
HRESULT STDMETHODCALLTYPE WArguments_Invoke(IWArguments *this, DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr);

HRESULT STDMETHODCALLTYPE WArguments_Item(IWArguments *this, int nIndex, BSTR *wpItem);
HRESULT STDMETHODCALLTYPE WArguments_Length(IWArguments *this, int *nItems);
HRESULT STDMETHODCALLTYPE WArguments_Count(IWArguments *this, int *nItems);

#endif
