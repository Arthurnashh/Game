#include <El/elementpch.hpp>
#include <Es/Common/win.h>
#include "netfw.h"

HRESULT WindowsFirewallInitialize(OUT INetFwProfile** fwProfile)
{
  HRESULT hr = S_OK;
  INetFwMgr* fwMgr = NULL;
  INetFwPolicy* fwPolicy = NULL;

  DoAssert(fwProfile != NULL);

  *fwProfile = NULL;

  // Create an instance of the firewall settings manager.
  hr = CoCreateInstance(
    __uuidof(NetFwMgr),
    NULL,
    CLSCTX_INPROC_SERVER,
    __uuidof(INetFwMgr),
    (void**)&fwMgr
    );
  if (FAILED(hr))
  {
    LogF("CoCreateInstance failed: 0x%08lx\n", hr);
    goto error;
  }

  // Retrieve the local firewall policy.
  hr = fwMgr->get_LocalPolicy(&fwPolicy);
  if (FAILED(hr))
  {
    LogF("get_LocalPolicy failed: 0x%08lx\n", hr);
    goto error;
  }

  // Retrieve the firewall profile currently in effect.
  hr = fwPolicy->get_CurrentProfile(fwProfile);
  if (FAILED(hr))
  {
    LogF("get_CurrentProfile failed: 0x%08lx\n", hr);
    goto error;
  }

error:

  // Release the local firewall policy.
  if (fwPolicy != NULL)
  {
    fwPolicy->Release();
  }

  // Release the firewall settings manager.
  if (fwMgr != NULL)
  {
    fwMgr->Release();
  }

  return hr;
}

void WindowsFirewallCleanup(IN INetFwProfile* fwProfile)
{
  // Release the firewall profile.
  if (fwProfile != NULL)
  {
    fwProfile->Release();
  }
}

HRESULT WindowsFirewallAppIsEnabled(
                                    IN INetFwProfile* fwProfile,
                                    IN const wchar_t* fwProcessImageFileName,
                                    OUT BOOL* fwAppEnabled
                                    )
{
  HRESULT hr = S_OK;
  BSTR fwBstrProcessImageFileName = NULL;
  VARIANT_BOOL fwEnabled;
  INetFwAuthorizedApplication* fwApp = NULL;
  INetFwAuthorizedApplications* fwApps = NULL;

  DoAssert(fwProfile != NULL);
  DoAssert(fwProcessImageFileName != NULL);
  DoAssert(fwAppEnabled != NULL);

  *fwAppEnabled = FALSE;

  // Retrieve the authorized application collection.
  hr = fwProfile->get_AuthorizedApplications(&fwApps);
  if (FAILED(hr))
  {
    LogF("get_AuthorizedApplications failed: 0x%08lx\n", hr);
    goto error;
  }

  // Allocate a BSTR for the process image file name.
  fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
  if (fwBstrProcessImageFileName == NULL)
  {
    hr = E_OUTOFMEMORY;
    LogF("SysAllocString failed: 0x%08lx\n", hr);
    goto error;
  }

  // Attempt to retrieve the authorized application.
  hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);
  if (SUCCEEDED(hr))
  {
    // Find out if the authorized application is enabled.
    hr = fwApp->get_Enabled(&fwEnabled);
    if (FAILED(hr))
    {
      LogF("get_Enabled failed: 0x%08lx\n", hr);
      goto error;
    }

    if (fwEnabled != VARIANT_FALSE)
    {
      // The authorized application is enabled.
      *fwAppEnabled = TRUE;

      LogF(
        "Authorized application %lS is enabled in the firewall.\n",
        fwProcessImageFileName
        );
    }
    else
    {
      LogF(
        "Authorized application %lS is disabled in the firewall.\n",
        fwProcessImageFileName
        );
    }
  }
  else
  {
    // The authorized application was not in the collection.
    hr = S_OK;

    LogF(
      "Authorized application %lS is disabled in the firewall.\n",
      fwProcessImageFileName
      );
  }

error:

  // Free the BSTR.
  SysFreeString(fwBstrProcessImageFileName);

  // Release the authorized application instance.
  if (fwApp != NULL)
  {
    fwApp->Release();
  }

  // Release the authorized application collection.
  if (fwApps != NULL)
  {
    fwApps->Release();
  }

  return hr;
}

HRESULT WindowsFirewallAddApp(
                              IN INetFwProfile* fwProfile,
                              IN const wchar_t* fwProcessImageFileName,
                              IN const wchar_t* fwName
                              )
{
  HRESULT hr = S_OK;
  BOOL fwAppEnabled;
  BSTR fwBstrName = NULL;
  BSTR fwBstrProcessImageFileName = NULL;
  INetFwAuthorizedApplication* fwApp = NULL;
  INetFwAuthorizedApplications* fwApps = NULL;

  DoAssert(fwProfile != NULL);
  DoAssert(fwProcessImageFileName != NULL);
  DoAssert(fwName != NULL);

  // First check to see if the application is already authorized.
  hr = WindowsFirewallAppIsEnabled(
    fwProfile,
    fwProcessImageFileName,
    &fwAppEnabled
    );
  if (FAILED(hr))
  {
    LogF("WindowsFirewallAppIsEnabled failed: 0x%08lx\n", hr);
    goto error;
  }

  // Only add the application if it isn't already authorized.
  if (!fwAppEnabled)
  {
    // Retrieve the authorized application collection.
    hr = fwProfile->get_AuthorizedApplications(&fwApps);
    if (FAILED(hr))
    {
      LogF("get_AuthorizedApplications failed: 0x%08lx\n", hr);
      goto error;
    }

    // Create an instance of an authorized application.
    hr = CoCreateInstance(
      __uuidof(NetFwAuthorizedApplication),
      NULL,
      CLSCTX_INPROC_SERVER,
      __uuidof(INetFwAuthorizedApplication),
      (void**)&fwApp
      );
    if (FAILED(hr))
    {
      LogF("CoCreateInstance failed: 0x%08lx\n", hr);
      goto error;
    }

    // Allocate a BSTR for the process image file name.
    fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
    if (fwBstrProcessImageFileName == NULL)
    {
      hr = E_OUTOFMEMORY;
      LogF("SysAllocString failed: 0x%08lx\n", hr);
      goto error;
    }

    // Set the process image file name.
    hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
    if (FAILED(hr))
    {
      LogF("put_ProcessImageFileName failed: 0x%08lx\n", hr);
      goto error;
    }

    // Allocate a BSTR for the application friendly name.
    fwBstrName = SysAllocString(fwName);
    if (SysStringLen(fwBstrName) == 0)
    {
      hr = E_OUTOFMEMORY;
      LogF("SysAllocString failed: 0x%08lx\n", hr);
      goto error;
    }

    // Set the application friendly name.
    hr = fwApp->put_Name(fwBstrName);
    if (FAILED(hr))
    {
      LogF("put_Name failed: 0x%08lx\n", hr);
      goto error;
    }

    // Add the application to the collection.
    hr = fwApps->Add(fwApp);
    if (FAILED(hr))
    {
      LogF("Add failed: 0x%08lx\n", hr);
      goto error;
    }

    LogF(
      "Authorized application %lS is now enabled in the firewall.\n",
      fwProcessImageFileName
      );
  }

error:

  // Free the BSTRs.
  SysFreeString(fwBstrName);
  SysFreeString(fwBstrProcessImageFileName);

  // Release the authorized application instance.
  if (fwApp != NULL)
  {
    fwApp->Release();
  }

  // Release the authorized application collection.
  if (fwApps != NULL)
  {
    fwApps->Release();
  }

  return hr;
}

HRESULT WindowsFirewallRemoveApp(
  IN INetFwProfile* fwProfile,
  IN const wchar_t* fwProcessImageFileName
 )
{
  HRESULT hr = S_OK;
  BSTR fwBstrProcessImageFileName = NULL;
  INetFwAuthorizedApplications* fwApps = NULL;

  DoAssert(fwProfile != NULL);
  DoAssert(fwProcessImageFileName != NULL);
  
  // Retrieve the authorized application collection.
  hr = fwProfile->get_AuthorizedApplications(&fwApps);
  if (FAILED(hr))
  {
    LogF("get_AuthorizedApplications failed: 0x%08lx\n", hr);
    goto error;
  }

  // Allocate a BSTR for the process image file name.
  fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
  if (fwBstrProcessImageFileName == NULL)
  {
    hr = E_OUTOFMEMORY;
    LogF("SysAllocString failed: 0x%08lx\n", hr);
    goto error;
  }

  // Remove the authorized application.
  hr = fwApps->Remove(fwBstrProcessImageFileName);

error:

  // Free the BSTR.
  SysFreeString(fwBstrProcessImageFileName);

  // Release the authorized application collection.
  if (fwApps != NULL)
  {
    fwApps->Release();
  }

  return hr;
}

bool WindowsFirewallAddApplication(const wchar_t *filename, const wchar_t *appname)
{
  HRESULT hr = S_OK;
  INetFwProfile* fwProfile = NULL;

  // Initialize COM.
  HRESULT comInit = CoInitializeEx(
    0,
    COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE
    );

  // Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
  // initialized with a different mode. Since we don't care what the mode is,
  // we'll just use the existing mode.
  if (comInit != RPC_E_CHANGED_MODE)
  {
    hr = comInit;
    if (FAILED(hr))
    {
      LogF("CoInitializeEx failed: 0x%08lx\n", hr);
      goto error;
    }
  }

  // Retrieve the firewall profile currently in effect.
  hr = WindowsFirewallInitialize(&fwProfile);
  if (FAILED(hr))
  {
    LogF("WindowsFirewallInitialize failed: 0x%08lx\n", hr);
    goto error;
  }

  // Add Windows Messenger to the authorized application collection.
  hr = WindowsFirewallAddApp(
    fwProfile,
    filename,
    appname
    );
  if (FAILED(hr))
  {
    LogF("WindowsFirewallAddApp failed: 0x%08lx\n", hr);
    goto error;
  }

error:

  // Release the firewall profile.
  WindowsFirewallCleanup(fwProfile);

  // Uninitialize COM.
  if (SUCCEEDED(comInit))
  {
    CoUninitialize();
  }

  return SUCCEEDED(hr);
}

bool WindowsFirewallRemoveApplication(const wchar_t *filename)
{
  HRESULT hr = S_OK;
  INetFwProfile* fwProfile = NULL;

  // Initialize COM.
  HRESULT comInit = CoInitializeEx(
    0,
    COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE
    );

  // Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
  // initialized with a different mode. Since we don't care what the mode is,
  // we'll just use the existing mode.
  if (comInit != RPC_E_CHANGED_MODE)
  {
    hr = comInit;
    if (FAILED(hr))
    {
      LogF("CoInitializeEx failed: 0x%08lx\n", hr);
      goto error;
    }
  }

  // Retrieve the firewall profile currently in effect.
  hr = WindowsFirewallInitialize(&fwProfile);
  if (FAILED(hr))
  {
    LogF("WindowsFirewallInitialize failed: 0x%08lx\n", hr);
    goto error;
  }

  // Add Windows Messenger to the authorized application collection.
  hr = WindowsFirewallRemoveApp(
    fwProfile,
    filename
    );
  if (FAILED(hr))
  {
    LogF("WindowsFirewallRemoveApp failed: 0x%08lx\n", hr);
    goto error;
  }

error:

  // Release the firewall profile.
  WindowsFirewallCleanup(fwProfile);

  // Uninitialize COM.
  if (SUCCEEDED(comInit))
  {
    CoUninitialize();
  }

  return SUCCEEDED(hr);
}
