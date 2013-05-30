#ifndef _ISYSTEMFUNCTION_H_
#define _ISYSTEMFUNCTION_H_

//Defines
typedef struct _SYSPARAMITEM {
  struct _SYSPARAMITEM *next;
  struct _SYSPARAMITEM *prev;
  UINT_PTR dwType;
  UINT_PTR dwValue;
} SYSPARAMITEM;

typedef struct {
  SYSPARAMITEM *first;
  SYSPARAMITEM *last;
  int nElements;
} SYSPARAMSTACK;

typedef struct _SYSTEMFUNCTION {
  SYSPARAMSTACK hSysParamStack;
  SYSPARAMSTACK hSaveStack;
  DWORD dwLastError;
} SYSTEMFUNCTION;

typedef struct {
  ISystemFunctionVtbl *lpVtbl;
  DWORD dwCount;
  void *lpScriptThread;
  SYSTEMFUNCTION sf;
} IRealSystemFunction;

typedef LRESULT (*SYSCALLBACK)();

//Global variables
extern ITypeInfo *g_SystemFunctionTypeInfo;
extern CALLBACKSTACK g_hSysCallbackStack;
extern const ISystemFunctionVtbl MyISystemFunctionVtbl;

//Functions prototypes
HRESULT STDMETHODCALLTYPE SystemFunction_QueryInterface(ISystemFunction *this, REFIID vTableGuid, void **ppv);
ULONG STDMETHODCALLTYPE SystemFunction_AddRef(ISystemFunction *this);
ULONG STDMETHODCALLTYPE SystemFunction_Release(ISystemFunction *this);

HRESULT STDMETHODCALLTYPE SystemFunction_GetTypeInfoCount(ISystemFunction *this, UINT *pCount);
HRESULT STDMETHODCALLTYPE SystemFunction_GetTypeInfo(ISystemFunction *this, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo);
HRESULT STDMETHODCALLTYPE SystemFunction_GetIDsOfNames(ISystemFunction *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
HRESULT STDMETHODCALLTYPE SystemFunction_Invoke(ISystemFunction *this, DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr);

HRESULT STDMETHODCALLTYPE SystemFunction_AddParameter(ISystemFunction *this, VARIANT vtParameter);
HRESULT STDMETHODCALLTYPE SystemFunction_Call(ISystemFunction *this, BSTR wpDllFunction, SAFEARRAY **psa, INT_PTR *nResult);
HRESULT STDMETHODCALLTYPE SystemFunction_GetLastError(ISystemFunction *this, DWORD *dwLastError);
HRESULT STDMETHODCALLTYPE SystemFunction_RegisterCallback(ISystemFunction *this, BSTR wpCallbackName, IDispatch *objCallback, int nArgCount, VARIANT *vtFunction);
HRESULT STDMETHODCALLTYPE SystemFunction_UnregisterCallback(ISystemFunction *this, IDispatch *objFunction);
SYSPARAMITEM* StackInsertSysParam(SYSPARAMSTACK *hStack);
void StackSaveSysParam(SYSPARAMSTACK *hFromStack, SYSPARAMSTACK *hToStack);
void StackFreeSysParams(SYSPARAMSTACK *hStack);
int AsmCallSysFunc(SYSPARAMSTACK *hCurStack, SYSPARAMSTACK *hSaveStack, FARPROC lpProcedure);
LRESULT AsmCallback1Proc();
LRESULT AsmCallback2Proc();
LRESULT AsmCallback3Proc();
LRESULT AsmCallback4Proc();
LRESULT AsmCallbackHelper(INT_PTR *lpnFirstArg, int nCallbackIndex, int *lpnArgSize);

#endif
