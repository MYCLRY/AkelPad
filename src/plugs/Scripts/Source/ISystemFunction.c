#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <activscp.h>
#include "Scripts.h"


//Global variables
ITypeInfo *g_SystemFunctionTypeInfo=NULL;
CALLBACKSTACK g_hSysCallbackStack={0};

const ISystemFunctionVtbl MyISystemFunctionVtbl={
  SystemFunction_QueryInterface,
  SystemFunction_AddRef,
  SystemFunction_Release,
  SystemFunction_GetTypeInfoCount,
  SystemFunction_GetTypeInfo,
  SystemFunction_GetIDsOfNames,
  SystemFunction_Invoke,
  SystemFunction_AddParameter,
  SystemFunction_Call,
  SystemFunction_GetLastError,
  SystemFunction_RegisterCallback,
  SystemFunction_UnregisterCallback
};


//// ISystemFunction

HRESULT STDMETHODCALLTYPE SystemFunction_QueryInterface(ISystemFunction *this, REFIID vTableGuid, void **ppv)
{
  if (!ppv) return E_POINTER;

  if (AKD_IsEqualIID(vTableGuid, &IID_IUnknown) || AKD_IsEqualIID(vTableGuid, &IID_IDispatch))
  {
    *ppv=this;
    this->lpVtbl->AddRef(this);
    return NOERROR;
  }
  *ppv=NULL;
  return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE SystemFunction_AddRef(ISystemFunction *this)
{
  return ++((IRealSystemFunction *)this)->dwCount;
}

ULONG STDMETHODCALLTYPE SystemFunction_Release(ISystemFunction *this)
{
  if (--((IRealSystemFunction *)this)->dwCount == 0)
  {
    GlobalFree(this);
    InterlockedDecrement(&g_nObjs);
    return 0;
  }
  return ((IRealSystemFunction *)this)->dwCount;
}


//// IDispatch

HRESULT STDMETHODCALLTYPE SystemFunction_GetTypeInfoCount(ISystemFunction *this, UINT *pCount)
{
  *pCount=1;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE SystemFunction_GetTypeInfo(ISystemFunction *this, UINT itinfo, LCID lcid, ITypeInfo **pTypeInfo)
{
  HRESULT hr;

  *pTypeInfo=NULL;

  if (itinfo)
  {
    hr=ResultFromScode(DISP_E_BADINDEX);
  }
  else if (g_SystemFunctionTypeInfo)
  {
    g_SystemFunctionTypeInfo->lpVtbl->AddRef(g_SystemFunctionTypeInfo);
    hr=S_OK;
  }
  else
  {
    hr=LoadTypeInfoFromFile(NULL, NULL);
  }
  if (hr == S_OK) *pTypeInfo=g_SystemFunctionTypeInfo;

  return hr;
}

HRESULT STDMETHODCALLTYPE SystemFunction_GetIDsOfNames(ISystemFunction *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
  if (!g_SystemFunctionTypeInfo)
  {
    HRESULT hr;

    if ((hr=LoadTypeInfoFromFile(NULL, NULL)) != S_OK)
      return hr;
  }
  return DispGetIDsOfNames(g_SystemFunctionTypeInfo, rgszNames, cNames, rgdispid);
}

HRESULT STDMETHODCALLTYPE SystemFunction_Invoke(ISystemFunction *this, DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
  if (!AKD_IsEqualIID(riid, &IID_NULL))
    return DISP_E_UNKNOWNINTERFACE;

  if (!g_SystemFunctionTypeInfo)
  {
    HRESULT hr;

    if ((hr=LoadTypeInfoFromFile(NULL, NULL)) != S_OK)
      return hr;
  }
  return DispInvoke(this, g_SystemFunctionTypeInfo, dispid, wFlags, params, result, pexcepinfo, puArgErr);
}


//// ISystemFunction methods

HRESULT STDMETHODCALLTYPE SystemFunction_AddParameter(ISystemFunction *this, VARIANT vtParameter)
{
  SYSPARAMSTACK *hStack=&((IRealSystemFunction *)this)->sf.hSysParamStack;
  SYSPARAMITEM *lpSysParam;
  CALLBACKITEM *lpSysCallback;
  VARIANT *pvtParameter=&vtParameter;
  int nUniLen;
  int nAnsiLen;

  if (lpSysParam=StackInsertSysParam(hStack))
  {
    if (pvtParameter->vt == (VT_VARIANT|VT_BYREF))
      pvtParameter=pvtParameter->pvarVal;

    if (pvtParameter->vt == VT_BSTR)
    {
      if (bOldWindows)
      {
        nUniLen=SysStringLen(pvtParameter->bstrVal);
        nAnsiLen=WideCharToMultiByte(CP_ACP, 0, pvtParameter->bstrVal, nUniLen, NULL, 0, NULL, NULL);
        if (lpSysParam->dwValue=(UINT_PTR)GlobalAlloc(GPTR, nAnsiLen + 1))
          WideCharToMultiByte(CP_ACP, 0, pvtParameter->bstrVal, nUniLen + 1, (char *)lpSysParam->dwValue, nAnsiLen + 1, NULL, NULL);
      }
      else lpSysParam->dwValue=(UINT_PTR)pvtParameter->bstrVal;
    }
    else if (pvtParameter->vt == VT_DISPATCH)
    {
      if (lpSysCallback=StackGetCallbackByObject(&g_hSysCallbackStack, pvtParameter->pdispVal))
        lpSysParam->dwValue=(UINT_PTR)lpSysCallback->hHandle;
      else
        lpSysParam->dwValue=(UINT_PTR)pvtParameter->pdispVal;
    }
    else
    {
      if (pvtParameter->vt == VT_BOOL)
        lpSysParam->dwValue=pvtParameter->boolVal?TRUE:FALSE;
      else
        lpSysParam->dwValue=GetVariantInt(pvtParameter);
    }
    lpSysParam->dwType=pvtParameter->vt;
  }
  else return E_OUTOFMEMORY;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE SystemFunction_Call(ISystemFunction *this, BSTR wpDllFunction, SAFEARRAY **psa, INT_PTR *nResult)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealSystemFunction *)this)->lpScriptThread;
  SYSTEMFUNCTION *sf;
  HMODULE hModule=NULL;
  FARPROC lpProcedure=NULL;
  char szFunction[MAX_PATH];
  wchar_t wszFunction[MAX_PATH];
  wchar_t wszDll[MAX_PATH];
  BOOL bLoadLibrary=FALSE;
  int i;

  //Fill call stack
  {
    VARIANT *pvtParameter;
    unsigned char *lpData;
    DWORD dwElement;
    DWORD dwElementSum;

    lpData=(unsigned char *)((*psa)->pvData);
    dwElementSum=(*psa)->rgsabound[0].cElements;

    if (dwElementSum)
    {
      for (dwElement=0; dwElement < dwElementSum; ++dwElement)
      {
        pvtParameter=(VARIANT *)(lpData + dwElement * sizeof(VARIANT));
        SystemFunction_AddParameter(this, *pvtParameter);
      }
    }
  }

  *nResult=0;
  sf=&((IRealSystemFunction *)this)->sf;

  if (*wpDllFunction)
  {
    wszFunction[0]=L'\0';
    wszDll[0]=L'\0';

    for (i=0; wpDllFunction[i]; ++i)
    {
      if (wpDllFunction[i] == ':' && wpDllFunction[i + 1] == ':')
      {
        xstrcpynW(wszDll, wpDllFunction, i + 1);
        xstrcpynW(wszFunction, wpDllFunction + i + 2, MAX_PATH);
        break;
      }
    }

    if (!(hModule=GetModuleHandleWide(wszDll)))
    {
      if (hModule=LoadLibraryWide(wszDll))
      {
        bLoadLibrary=TRUE;
      }
    }
    WideCharToMultiByte(CP_ACP, 0, wszFunction, -1, szFunction, MAX_PATH, NULL, NULL);

    if (hModule)
    {
      if (lpProcedure=GetProcAddress(hModule, szFunction))
      {
        //Call function
        *nResult=AsmCallSysFunc(&sf->hSysParamStack, &sf->hSaveStack, lpProcedure);

        //Get last error
        sf->dwLastError=GetLastError();

        //Free call parameters
        StackFreeSysParams(&sf->hSaveStack);
      }
      if (bLoadLibrary)
      {
        FreeLibrary(hModule);
      }
    }
  }

  if (lpScriptThread->dwDebug & DBG_SYSCALL)
  {
    if (!hModule)
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_SYSCALLDLL), wszDll);
      return E_POINTER;
    }
    if (!lpProcedure)
    {
      xprintfW(wszErrorMsg, GetLangStringW(wLangModule, STRID_DEBUG_SYSCALLFUNCTION), wszFunction, wszDll);
      return E_POINTER;
    }
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE SystemFunction_GetLastError(ISystemFunction *this, DWORD *dwLastError)
{
  *dwLastError=((IRealSystemFunction *)this)->sf.dwLastError;

  return NOERROR;
}

HRESULT STDMETHODCALLTYPE SystemFunction_RegisterCallback(ISystemFunction *this, BSTR wpCallbackName, IDispatch *objCallback, int nArgCount, VARIANT *vtFunction)
{
  SCRIPTTHREAD *lpScriptThread=(SCRIPTTHREAD *)((IRealSystemFunction *)this)->lpScriptThread;
  IDispatch *objScript;
  ITypeInfo *pTypeInfo;
  TYPEATTR *pAttr;
  FUNCDESC *pFuncDesc;
  DISPPARAMS dispp;
  VARIANT vtResult;
  DISPID dispidCallbackName;
  int i;
  HRESULT hr=NOERROR;

  if ((!objCallback || nArgCount == -1) && lpScriptThread->objActiveScript)
  {
    if ((hr=lpScriptThread->objActiveScript->lpVtbl->GetScriptDispatch(lpScriptThread->objActiveScript, 0, &objScript)) == S_OK)
    {
      if ((hr=objScript->lpVtbl->GetIDsOfNames(objScript, &IID_NULL, &wpCallbackName, 1, LOCALE_USER_DEFAULT, &dispidCallbackName)) == S_OK)
      {
        //Get function IDispatch
        if (!objCallback)
        {
          dispp.rgvarg=NULL;
          dispp.rgdispidNamedArgs=NULL;
          dispp.cArgs=0;
          dispp.cNamedArgs=0;

          if ((hr=objScript->lpVtbl->Invoke(objScript, dispidCallbackName, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispp, &vtResult, 0, 0)) == S_OK)
            objCallback=vtResult.pdispVal;
        }

        //Get function arguments count
        if (objCallback && nArgCount == -1)
        {
          if ((hr=objScript->lpVtbl->GetTypeInfo(objScript, 0, LOCALE_USER_DEFAULT, &pTypeInfo)) == S_OK)
          {
            if ((hr=pTypeInfo->lpVtbl->GetTypeAttr(pTypeInfo, &pAttr)) == S_OK)
            {
              for (i=0; i < pAttr->cFuncs; ++i)
              {
                if ((hr=pTypeInfo->lpVtbl->GetFuncDesc(pTypeInfo, i, &pFuncDesc)) == S_OK)
                {
                  if (pFuncDesc->memid == dispidCallbackName)
                  {
                    nArgCount=pFuncDesc->cParams;
                    pTypeInfo->lpVtbl->ReleaseFuncDesc(pTypeInfo, pFuncDesc);
                    break;
                  }
                  pTypeInfo->lpVtbl->ReleaseFuncDesc(pTypeInfo, pFuncDesc);
                }
                else break;
              }
              pTypeInfo->lpVtbl->ReleaseTypeAttr(pTypeInfo, pAttr);
            }
            pTypeInfo->lpVtbl->Release(pTypeInfo);
          }
        }
      }
      objScript->lpVtbl->Release(objScript);
    }
  }

  if (objCallback && nArgCount >= 0)
  {
    CALLBACKITEM *lpCallback;
    SYSCALLBACK lpCallbackProc;
    int nIndex;

    //Find unhooked element if any
    if (lpCallback=StackGetCallbackByHandle(&g_hSysCallbackStack, NULL))
      nIndex=lpCallback->nStaticIndex - 1;
    else
      nIndex=g_hSysCallbackStack.nElements;

    //We support limited number of callbacks because with one callback we couldn't know who is called it
    if (nIndex == 0)
      lpCallbackProc=AsmCallback1Proc;
    else if (nIndex == 1)
      lpCallbackProc=AsmCallback2Proc;
    else if (nIndex == 2)
      lpCallbackProc=AsmCallback3Proc;
    else if (nIndex == 3)
      lpCallbackProc=AsmCallback4Proc;

    if (!lpCallback)
      lpCallback=StackInsertCallback(&g_hSysCallbackStack);

    if (lpCallback)
    {
      objCallback->lpVtbl->AddRef(objCallback);
      lpCallback->hHandle=(HANDLE)(INT_PTR)lpCallbackProc;
      lpCallback->objFunction=objCallback;
      lpCallback->dwData=nArgCount;
      lpCallback->nCallbackType=CIT_SYSCALLBACK;
      lpCallback->lpScriptThread=(void *)lpScriptThread;

      if (!lpScriptThread->hWndScriptsThreadDummy)
      {
        lpScriptThread->hWndScriptsThreadDummy=CreateScriptsThreadDummyWindow();
      }
    }
  }
  else if (hr == NOERROR)
  {
    hr=DISP_E_BADPARAMCOUNT;
  }

  objCallback->lpVtbl->AddRef(objCallback);
  VariantInit(vtFunction);
  vtFunction->vt=VT_DISPATCH;
  vtFunction->pdispVal=objCallback;

  return hr;
}

HRESULT STDMETHODCALLTYPE SystemFunction_UnregisterCallback(ISystemFunction *this, IDispatch *objFunction)
{
  CALLBACKITEM *lpCallback;

  if (lpCallback=StackGetCallbackByObject(&g_hSysCallbackStack, objFunction))
  {
    lpCallback->objFunction->lpVtbl->Release(lpCallback->objFunction);

    //We don't use StackDeleteCallback, because stack elements is linked to static procedure addresses.
    lpCallback->hHandle=NULL;
    lpCallback->objFunction=NULL;
    lpCallback->dwData=0;
    lpCallback->lpScriptThread=NULL;
  }
  return NOERROR;
}

SYSPARAMITEM* StackInsertSysParam(SYSPARAMSTACK *hStack)
{
  SYSPARAMITEM *lpSysParam;

  if (!StackInsertIndex((stack **)&hStack->first, (stack **)&hStack->last, (stack **)&lpSysParam, 1, sizeof(SYSPARAMITEM)))
    ++hStack->nElements;

  return lpSysParam;
}

void StackSaveSysParam(SYSPARAMSTACK *hFromStack, SYSPARAMSTACK *hToStack)
{
  StackJoin((stack **)&hToStack->first, (stack **)&hToStack->last, NULL, (stack *)hFromStack->first, (stack *)hFromStack->last);
  hFromStack->first=0;
  hFromStack->last=0;
  hFromStack->nElements=0;
}

void StackFreeSysParams(SYSPARAMSTACK *hStack)
{
  SYSPARAMITEM *lpSysParam;

  if (bOldWindows)
  {
    for (lpSysParam=hStack->first; lpSysParam; lpSysParam=lpSysParam->next)
    {
      if (lpSysParam->dwType == VT_BSTR)
      {
        if (lpSysParam->dwValue)
          GlobalFree((HGLOBAL)lpSysParam->dwValue);
      }
    }
  }
  StackClear((stack **)&hStack->first, (stack **)&hStack->last);
  hStack->nElements=0;
}

#ifndef _WIN64
int __declspec(naked) AsmCallSysFunc(SYSPARAMSTACK *hCurStack, SYSPARAMSTACK *hSaveStack, FARPROC lpProcedure)
{
  SYSPARAMITEM *lpSysParam;
  DWORD dwValue;

  __asm
  {
    //Prolog
    push  ebp
    mov   ebp, esp
    sub   esp, __LOCAL_SIZE
  }

  //Push parameters to stack
  for (lpSysParam=hCurStack->first; lpSysParam; lpSysParam=lpSysParam->next)
  {
    dwValue=lpSysParam->dwValue;

    __asm
    {
      push  [dwValue]
    }
  }

  //Move arguments from hCurStack to hSaveStack stack to support recursive calls
  StackSaveSysParam(hCurStack, hSaveStack);

  __asm
  {
    //Call
    call  [lpProcedure]
  }

  __asm
  {
    //Epilog
    mov  esp, ebp
    pop  ebp
    ret
  }
}

LRESULT __declspec(naked) AsmCallback1Proc()
{
  //More info at: http://en.wikibooks.org/wiki/X86_Disassembly/Functions_and_Stack_Frames
  int *lpnFirstArg;
  int nArgSize;
  LRESULT lResult;

  __asm
  {
    //Prolog
    push  ebp
    mov   ebp, esp
    sub   esp, __LOCAL_SIZE

    //Get pointer to first element in stack
    mov   lpnFirstArg, ebp

    //To get first argument pointer we skip first two elements in stack:
    //1-saved ebp in prolog and 2-return address
    add   lpnFirstArg, 8
  }
  lResult=AsmCallbackHelper(lpnFirstArg, 1, &nArgSize);

  __asm
  {
    //Remember nArgSize in edx
    mov  edx, nArgSize

    //Return value
    mov  eax, lResult

    //Epilog
    mov  esp, ebp
    pop  ebp
  }

  //Remove pushed function arguments.
  __asm
  {
    mov  ecx, [esp]  //Save return address in ecx register.
    add  esp, edx    //Correct stack pointer, esp will point on the last function argument.
    mov  [esp], ecx  //Restore return address in place of the last function argument.
    ret
  }
}

LRESULT __declspec(naked) AsmCallback2Proc()
{
  //More info at: http://en.wikibooks.org/wiki/X86_Disassembly/Functions_and_Stack_Frames
  int *lpnFirstArg;
  int nArgSize;
  LRESULT lResult;

  __asm
  {
    //Prolog
    push  ebp
    mov   ebp, esp
    sub   esp, __LOCAL_SIZE

    //Get pointer to first element in stack
    mov   lpnFirstArg, ebp

    //To get first argument pointer we skip first two elements in stack:
    //1-saved ebp in prolog and 2-return address
    add   lpnFirstArg, 8
  }
  lResult=AsmCallbackHelper(lpnFirstArg, 2, &nArgSize);

  __asm
  {
    //Remember nArgSize in edx
    mov  edx, nArgSize

    //Return value
    mov  eax, lResult

    //Epilog
    mov  esp, ebp
    pop  ebp
  }

  //Remove pushed function arguments.
  __asm
  {
    mov  ecx, [esp]  //Save return address in ecx register.
    add  esp, edx    //Correct stack pointer, esp will point on the last function argument.
    mov  [esp], ecx  //Restore return address in place of the last function argument.
    ret
  }
}

LRESULT __declspec(naked) AsmCallback3Proc()
{
  //More info at: http://en.wikibooks.org/wiki/X86_Disassembly/Functions_and_Stack_Frames
  int *lpnFirstArg;
  int nArgSize;
  LRESULT lResult;

  __asm
  {
    //Prolog
    push  ebp
    mov   ebp, esp
    sub   esp, __LOCAL_SIZE

    //Get pointer to first element in stack
    mov   lpnFirstArg, ebp

    //To get first argument pointer we skip first two elements in stack:
    //1-saved ebp in prolog and 2-return address
    add   lpnFirstArg, 8
  }
  lResult=AsmCallbackHelper(lpnFirstArg, 3, &nArgSize);

  __asm
  {
    //Remember nArgSize in edx
    mov  edx, nArgSize

    //Return value
    mov  eax, lResult

    //Epilog
    mov  esp, ebp
    pop  ebp
  }

  //Remove pushed function arguments.
  __asm
  {
    mov  ecx, [esp]  //Save return address in ecx register.
    add  esp, edx    //Correct stack pointer, esp will point on the last function argument.
    mov  [esp], ecx  //Restore return address in place of the last function argument.
    ret
  }
}

LRESULT __declspec(naked) AsmCallback4Proc()
{
  //More info at: http://en.wikibooks.org/wiki/X86_Disassembly/Functions_and_Stack_Frames
  int *lpnFirstArg;
  int nArgSize;
  LRESULT lResult;

  __asm
  {
    //Prolog
    push  ebp
    mov   ebp, esp
    sub   esp, __LOCAL_SIZE

    //Get pointer to first element in stack
    mov   lpnFirstArg, ebp

    //To get first argument pointer we skip first two elements in stack:
    //1-saved ebp in prolog and 2-return address
    add   lpnFirstArg, 8
  }
  lResult=AsmCallbackHelper(lpnFirstArg, 4, &nArgSize);

  __asm
  {
    //Remember nArgSize in edx
    mov  edx, nArgSize

    //Return value
    mov  eax, lResult

    //Epilog
    mov  esp, ebp
    pop  ebp
  }

  //Remove pushed function arguments.
  __asm
  {
    mov  ecx, [esp]  //Save return address in ecx register.
    add  esp, edx    //Correct stack pointer, esp will point on the last function argument.
    mov  [esp], ecx  //Restore return address in place of the last function argument.
    ret
  }
}
#endif

LRESULT AsmCallbackHelper(INT_PTR *lpnFirstArg, int nCallbackIndex, int *lpnArgSize)
{
  CALLBACKITEM *lpSysCallback;
  SCRIPTTHREAD *lpScriptThread;
  MSGSEND msgs;
  DISPPARAMS dispp;
  VARIANT *vtArg=NULL;
  VARIANT *vtCount;
  LRESULT lResult=0;
  int nArgCount=0;
  int i;

  if (lpSysCallback=StackGetCallbackByIndex(&g_hSysCallbackStack, nCallbackIndex))
  {
    lpScriptThread=(SCRIPTTHREAD *)lpSysCallback->lpScriptThread;
    nArgCount=(int)lpSysCallback->dwData;

    if (nArgCount)
    {
      if (vtArg=(VARIANT *)GlobalAlloc(GPTR, sizeof(VARIANT) * nArgCount))
      {
        //Get last VARIANT pointer, because DISPPARAMS filled in reverse order
        vtCount=vtArg + (nArgCount - 1);

        for (i=0; i < nArgCount; ++i)
        {
          VariantInit(vtCount);
          #ifdef _WIN64
            vtCount->llVal=*(lpnFirstArg + i);
            vtCount->vt=VT_I8;
          #else
            vtCount->lVal=*(lpnFirstArg + i);
            vtCount->vt=VT_I4;
          #endif
          --vtCount;
        }
      }
    }
    xmemset(&dispp, 0, sizeof(DISPPARAMS));
    dispp.cArgs=nArgCount;
    dispp.rgvarg=vtArg;

    //Because objFunction->lpVtbl->Invoke cause error for different thread, we send message from this thread to hWndScriptsThreadDummy.
    msgs.lpCallback=lpSysCallback;
    msgs.lParam=(LPARAM)&dispp;
    lResult=SendMessage(lpScriptThread->hWndScriptsThreadDummy, AKDLL_CALLBACKSEND, 0, (LPARAM)&msgs);

    if (vtArg) GlobalFree((HGLOBAL)vtArg);
  }
  if (lpnArgSize) *lpnArgSize=nArgCount * sizeof(INT_PTR);
  return lResult;
}
