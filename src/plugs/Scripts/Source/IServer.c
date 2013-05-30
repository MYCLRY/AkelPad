#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <activscp.h>
#include "Scripts.h"


#include <initguid.h>
#include "Scripts_i.c"

//Global variables
LONG g_nObjs=0;
LONG g_nLocks=0;
IClassFactory MyIClassFactoryObj;

const IClassFactoryVtbl MyIClassFactoryVtbl={
  Class_QueryInterface,
  Class_AddRef,
  Class_Release,
  Class_CreateInstance,
  Class_LockServer
};


//// Common functions

HRESULT LoadTypeInfoFromFile(const GUID *guid, ITypeInfo **ppTypeInfo)
{
  wchar_t wszDLL[MAX_PATH];
  LPTYPELIB pTypeLib;
  HRESULT hr;

  GetModuleFileNameWide(hInstanceDLL, wszDLL, MAX_PATH);

  if ((hr=LoadTypeLib(wszDLL, &pTypeLib)) == S_OK)
  {
    if (guid)
    {
      //Get TypeInfo for concrete object
      if ((hr=pTypeLib->lpVtbl->GetTypeInfoOfGuid(pTypeLib, guid, ppTypeInfo)) == S_OK)
      {
        (*ppTypeInfo)->lpVtbl->AddRef(*ppTypeInfo);
      }
    }
    else
    {
      //Get TypeInfo for all objects
      GUIDTYPEINFO gti[]={{&IID_IWScript,        &g_WScriptTypeInfo},
                          {&IID_IWArguments,     &g_WArgumentsTypeInfo},
                          {&IID_IDocument,       &g_DocumentTypeInfo},
                          {&IID_IScriptSettings, &g_ScriptSettingsTypeInfo},
                          {&IID_ISystemFunction, &g_SystemFunctionTypeInfo},
                          {&IID_IConstants,      &g_ConstantsTypeInfo},
                          {0, 0}};
      int i;

      for (i=0; gti[i].guid; ++i)
      {
        if (!*gti[i].ppTypeInfo)
        {
          if ((hr=pTypeLib->lpVtbl->GetTypeInfoOfGuid(pTypeLib, gti[i].guid, gti[i].ppTypeInfo)) == S_OK)
          {
            (*gti[i].ppTypeInfo)->lpVtbl->AddRef(*gti[i].ppTypeInfo);
          }
        }
      }
    }
    pTypeLib->lpVtbl->Release(pTypeLib);
  }
  return hr;
}

//// IClassFactory

HRESULT STDMETHODCALLTYPE Class_QueryInterface(IClassFactory *this, REFIID factoryGuid, void **ppv)
{
  if (AKD_IsEqualIID(factoryGuid, &IID_IUnknown) || AKD_IsEqualIID(factoryGuid, &IID_IClassFactory))
  {
    this->lpVtbl->AddRef(this);
    *ppv=this;
    return NOERROR;
  }
  *ppv=NULL;
  return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE Class_AddRef(IClassFactory *this)
{
  InterlockedIncrement(&g_nObjs);
  return 1;
}

ULONG STDMETHODCALLTYPE Class_Release(IClassFactory *this)
{
  return InterlockedDecrement(&g_nObjs);
}

HRESULT STDMETHODCALLTYPE Class_CreateInstance(IClassFactory *this, IUnknown *punkOuter, REFIID vTableGuid, void **objHandle)
{
  IRealDocument *objIDocument;
  IRealWScript *objIWScript;
  IRealConstants *objIConstants;

  if (!objHandle) return E_POINTER;
  *objHandle=NULL;
  if (punkOuter) return CLASS_E_NOAGGREGATION;

  if (AKD_IsEqualIID(vTableGuid, &IID_IUnknown) || AKD_IsEqualIID(vTableGuid, &IID_IDocument))
  {
    //IDocument
    if ((objIDocument=(IRealDocument *)GlobalAlloc(GPTR, sizeof(IRealDocument))))
    {
      objIDocument->lpVtbl=(IDocumentVtbl *)&MyIDocumentVtbl;
      objIDocument->dwCount=1;
      objIDocument->lpScriptThread=StackGetScriptThreadCurrent();

      InterlockedIncrement(&g_nObjs);
      *objHandle=objIDocument;
    }
    else return E_OUTOFMEMORY;
  }
  else if (AKD_IsEqualIID(vTableGuid, &IID_IWScript))
  {
    //IWScript
    if ((objIWScript=(IRealWScript *)GlobalAlloc(GPTR, sizeof(IRealWScript))))
    {
      objIWScript->lpVtbl=(IWScriptVtbl *)&MyIWScriptVtbl;
      objIWScript->dwCount=1;
      objIWScript->lpScriptThread=StackGetScriptThreadCurrent();

      InterlockedIncrement(&g_nObjs);
      *objHandle=objIWScript;
    }
    else return E_OUTOFMEMORY;
  }
  else if (AKD_IsEqualIID(vTableGuid, &IID_IConstants))
  {
    //IConstants
    if ((objIConstants=(IRealConstants *)GlobalAlloc(GPTR, sizeof(IRealConstants))))
    {
      objIConstants->lpVtbl=(IConstantsVtbl *)&MyIConstantsVtbl;
      objIConstants->dwCount=1;
      objIConstants->lpScriptThread=StackGetScriptThreadCurrent();

      InterlockedIncrement(&g_nObjs);
      *objHandle=objIConstants;
    }
    else return E_OUTOFMEMORY;
  }
  return NOERROR;
}

HRESULT STDMETHODCALLTYPE Class_LockServer(IClassFactory *this, BOOL flock)
{
  if (flock)
    InterlockedIncrement(&g_nLocks);
  else
    InterlockedDecrement(&g_nLocks);
  return NOERROR;
}


//// Helper functions

BOOL AKD_IsEqualIID(const GUID *rguid1, const GUID *rguid2)
{
  return !xmemcmp(rguid1, rguid2, sizeof(GUID));
}


//// Extern functions

HRESULT WINAPI DllGetClassObject(REFCLSID objGuid, REFIID factoryGuid, void **factoryHandle)
{
  if (AKD_IsEqualIID(objGuid, &CLSID_Document))
  {
    //ActiveXObject
    if (!bInitCommon && !lpScriptThreadActiveX)
    {
      //Initialize WideFunc.h header
      WideInitialize();

      bOldWindows=WideGlobal_bOldWindows;
      wLangModule=GetUserDefaultLangID();
      MyIClassFactoryObj.lpVtbl=(IClassFactoryVtbl *)&MyIClassFactoryVtbl;

      if (lpScriptThreadActiveX=StackInsertScriptThread(&hThreadStack))
      {
        lpScriptThreadActiveX->dwThreadID=GetCurrentThreadId();
      }
    }

    return Class_QueryInterface(&MyIClassFactoryObj, factoryGuid, factoryHandle);
  }
  *factoryHandle=NULL;
  return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
 if (g_nObjs || g_nLocks)
   return S_FALSE;
 else
   return S_OK;
}

HRESULT WINAPI DllRegisterServer()
{
  const char szNameTypeLib[]="Scripts.dll";
  const char szNameProgID[]="AkelPad.document";
  const char szObjectDescription[]="AkelPad COM component";

  return RegisterServerA(hInstanceDLL, (REFCLSID)&CLSID_Document, szNameTypeLib, szNameProgID, szObjectDescription);
}

HRESULT WINAPI DllUnregisterServer()
{
  const char szNameProgID[]="AkelPad.document";

  return UnregisterServerA((REFCLSID)&CLSID_Document, (REFGUID)&LIBID_Scripts, szNameProgID);
}

HRESULT IsServerRegisteredA(HINSTANCE hInstance, REFCLSID clsid, const char *pNameTypeLib, const char *pNameProgID, const char *pObjectDescription, char *szDllFile, int nDllFileLen)
{
  char szGUID[64];
  char szDLL[MAX_PATH];
  const char *pDllName;
  HKEY hKey;
  DWORD dwSize;
  BSTR bstrDll;
  HRESULT hr=S_OK;

  if (szDllFile) szDllFile[0]='\0';

  //{DB045777-BAFF-416b-AA8E-A154E6A64A88}
  stringFromCLSIDA(szGUID, clsid);

  //Get file name Scripts.dll
  dwSize=GetModuleFileNameA(hInstance, szDLL, MAX_PATH);
  pDllName=GetFileNameAnsi(szDLL, dwSize);

  //HKEY_LOCAL_MACHINE\SOFTWARE\Classes\AkelPad.document
  wsprintfA(szBuffer, "Software\\Classes\\%s", pNameProgID);
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
  {
    dwSize=BUFFER_SIZE;
    RegQueryValueExA(hKey, NULL, NULL, NULL, (BYTE *)szBuffer, &dwSize);
    RegCloseKey(hKey);
  }
  else return S_FALSE;

  if (lstrcmpiA(pObjectDescription, szBuffer))
    return S_FALSE;

  //HKEY_LOCAL_MACHINE\SOFTWARE\Classes\AkelPad.document\CLSID
  wsprintfA(szBuffer, "Software\\Classes\\%s\\CLSID", pNameProgID);
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
  {
    dwSize=BUFFER_SIZE;
    RegQueryValueExA(hKey, NULL, NULL, NULL, (BYTE *)szBuffer, &dwSize);
    RegCloseKey(hKey);
  }
  else return S_FALSE;

  if (lstrcmpiA(szGUID, szBuffer))
    return S_FALSE;

  //HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{DB045777-BAFF-416B-AA8E-A154E6A64A88}
  wsprintfA(szBuffer, "Software\\Classes\\CLSID\\%s", szGUID);
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
  {
    dwSize=BUFFER_SIZE;
    RegQueryValueExA(hKey, NULL, NULL, NULL, (BYTE *)szBuffer, &dwSize);
    RegCloseKey(hKey);
  }
  else return S_FALSE;

  if (lstrcmpiA(pObjectDescription, szBuffer))
    return S_FALSE;

  //HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{DB045777-BAFF-416B-AA8E-A154E6A64A88}\InprocServer32
  wsprintfA(szBuffer, "Software\\Classes\\CLSID\\%s\\InprocServer32", szGUID);
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
  {
    dwSize=BUFFER_SIZE;
    RegQueryValueExA(hKey, NULL, NULL, NULL, (BYTE *)szBuffer, &dwSize);
    RegCloseKey(hKey);
  }
  else return S_FALSE;

  if (lstrcmpiA(pDllName, GetFileNameAnsi(szBuffer, -1)) || !FileExistsAnsi(szBuffer))
    return S_FALSE;

  //HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{DB045777-BAFF-416B-AA8E-A154E6A64A88}\ProgID
  wsprintfA(szBuffer, "Software\\Classes\\CLSID\\%s\\ProgID", szGUID);
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
  {
    dwSize=BUFFER_SIZE;
    RegQueryValueExA(hKey, NULL, NULL, NULL, (BYTE *)szBuffer, &dwSize);
    RegCloseKey(hKey);
  }
  else return S_FALSE;

  if (lstrcmpiA(pNameProgID, szBuffer))
    return S_FALSE;

  if (bstrDll=SysAllocStringLen(NULL, MAX_PATH))
  {
    hr=QueryPathOfRegTypeLib((REFGUID)&LIBID_Scripts, 1, 0, LOCALE_NEUTRAL, &bstrDll);

    if (hr == S_OK)
    {
      WideCharToMultiByte(CP_ACP, 0, bstrDll, -1, szBuffer, MAX_PATH, NULL, NULL);
      if (lstrcmpiA(pDllName, GetFileNameAnsi(szBuffer, -1)) || !FileExistsAnsi(szBuffer))
        hr=S_FALSE;
      if (szDllFile) lstrcpynA(szDllFile, szBuffer, nDllFileLen);
    }
    SysFreeString(bstrDll);
  }
  else return S_FALSE;

  return hr;
}

HRESULT RegisterServerA(HINSTANCE hInstance, REFCLSID clsid, const char *pNameTypeLib, const char *pNameProgID, const char *pObjectDescription)
{
  char szGUID[64];
  char szDLL[MAX_PATH];
  HKEY hRootKey;
  HKEY hKey;
  HKEY hKey2;
  HKEY hKey3;
  HRESULT hr=S_OK;

  GetModuleFileNameA(hInstance, szDLL, MAX_PATH);

  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Classes", 0, KEY_WRITE, &hRootKey) == ERROR_SUCCESS)
  {
    if (RegCreateKeyExA(hRootKey, pNameProgID, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, NULL) == ERROR_SUCCESS)
    {
      RegSetValueEx(hKey, NULL, 0, REG_SZ, (const BYTE *)pObjectDescription, lstrlenA(pObjectDescription) + 1);

      if (RegCreateKeyExA(hKey, "CLSID", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey2, NULL) == ERROR_SUCCESS)
      {
        stringFromCLSIDA(szGUID, clsid);
        RegSetValueEx(hKey2, NULL, 0, REG_SZ, (const BYTE *)szGUID, lstrlenA(szGUID) + 1);
        RegCloseKey(hKey2);
      }
      else hr=S_FALSE;

      RegCloseKey(hKey);
    }
    else hr=S_FALSE;

    if (hr == S_OK && RegOpenKeyExA(hRootKey, "CLSID", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
      if (RegCreateKeyExA(hKey, szGUID, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey2, NULL) == ERROR_SUCCESS)
      {
        RegSetValueEx(hKey2, NULL, 0, REG_SZ, (const BYTE *)pObjectDescription, lstrlenA(pObjectDescription) + 1);

        if (RegCreateKeyExA(hKey2, "InprocServer32", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey3, NULL) == ERROR_SUCCESS)
        {
          RegSetValueEx(hKey3, NULL, 0, REG_SZ, (const BYTE *)szDLL, lstrlenA(szDLL) + 1);
          RegCloseKey(hKey3);
        }
        else hr=S_FALSE;

        if (hr == S_OK && RegCreateKeyExA(hKey2, "ProgID", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey3, NULL) == ERROR_SUCCESS)
        {
          RegSetValueEx(hKey3, NULL, 0, REG_SZ, (const BYTE *)pNameProgID, lstrlenA(pNameProgID) + 1);
          RegCloseKey(hKey3);
        }
        else hr=S_FALSE;

        RegCloseKey(hKey2);
      }
      else hr=S_FALSE;

      RegCloseKey(hKey);
    }
    else hr=S_FALSE;

    RegCloseKey(hRootKey);
  }
  else hr=S_FALSE;

  if (hr == S_OK)
  {
    wchar_t wszGUID[MAX_PATH];
    ITypeLib *pTypeLib;
    int i;

    for (i=lstrlenA(szDLL) - 1; i >= 0; --i)
      if (szDLL[i] == '\\') break;
    lstrcpyA(szDLL + i + 1, pNameTypeLib);

    MultiByteToWideChar(CP_ACP, 0, szDLL, -1, wszGUID, MAX_PATH);
    if ((hr=LoadTypeLib(wszGUID, &pTypeLib)) == S_OK)
    {
      hr=RegisterTypeLib(pTypeLib, wszGUID, NULL);
      pTypeLib->lpVtbl->Release(pTypeLib);
    }
  }
  return hr;
}

HRESULT UnregisterServerA(REFCLSID clsid, REFGUID typelib, const char *pNameProgID)
{
  char szGUID[64];
  HKEY hRootKey;
  HKEY hKey;
  HKEY hKey2;

  stringFromCLSIDA(szGUID, clsid);

  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Classes", 0, KEY_WRITE, &hRootKey) == ERROR_SUCCESS)
  {
    if (RegOpenKeyExA(hRootKey, pNameProgID, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
      RegDeleteKeyA(hKey, "CLSID");
      RegCloseKey(hKey);
      RegDeleteKeyA(hRootKey, pNameProgID);
    }
    if (RegOpenKeyExA(hRootKey, "CLSID", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
      if (RegOpenKeyExA(hKey, szGUID, 0, KEY_ALL_ACCESS, &hKey2) == ERROR_SUCCESS)
      {
        RegDeleteKeyA(hKey2, "InprocServer32");
        RegDeleteKeyA(hKey2, "ProgID");
        RegCloseKey(hKey2);
        RegDeleteKeyA(hKey, szGUID);
      }
      RegCloseKey(hKey);
    }
    RegCloseKey(hRootKey);

    UnRegisterTypeLib(typelib, 1, 0, LOCALE_NEUTRAL, SYS_WIN32);
  }
  return S_OK;
}

void stringFromCLSIDA(char *szString, REFCLSID clsid)
{
  wsprintfA(szString, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
    clsid->Data1, clsid->Data2, clsid->Data3,
    clsid->Data4[0], clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
    clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7]);
}
