#ifdef _WIN32

#include "freeTrackClient.hpp"

bool FreeTrackClient::Init( RString dllPath )
{
  // Load DLL file
  if (dllPath.GetLength()>0)
  { //terminate by '\\'
    if (dllPath[dllPath.GetLength()-1]!='\\') dllPath = dllPath + RString("\\");
  }
  RString fullDLLPath = dllPath + RString("FreeTrackClient.dll");
  hinstLib = LoadLibrary(fullDLLPath);
  if (hinstLib == NULL) {
    LogF("ERROR: unable to load FreeTrackClient.dll");
    return false;
  }

  // Get function pointers
  getData = (importGetData)GetProcAddress(hinstLib, "FTGetData");
  getDllVersion = (importGetDllVersion)GetProcAddress(hinstLib, "FTGetDllVersion");
  reportName = (importReportName)GetProcAddress(hinstLib, "FTReportName");
  provider = (importProvider)GetProcAddress(hinstLib, "FTProvider");

  // Check they are valid
  if (getData == NULL) {
    LogF("ERROR: unable to find 'FTGetData' function");
    FreeLibrary(hinstLib);
    return false;
  }
  if (getDllVersion == NULL){
    LogF("ERROR: unable to find 'FTGetDllVersion' function");
    FreeLibrary(hinstLib);
    return false;
  }
  if (reportName == NULL){
    LogF("ERROR: unable to find 'FTReportName' function");
    FreeLibrary(hinstLib);
    return false;
  }
  if (provider == NULL){
    LogF("ERROR: unable to find 'FTProvider' function");
    FreeLibrary(hinstLib);
    return false;
  }

  //	Call each function and display result
  pDllVersion = getDllVersion();
  LogF("FreeTrack Dll Version: %s", pDllVersion);
  pProvider = provider();
  LogF("Provider: %s", pProvider);

  reportName(name);	//not sure what this does - I guess it tells the dll that I am using it.

  return true;
}

FreeTrackClient::FreeTrackClient()
:hinstLib(NULL)
{

}

FreeTrackClient::~FreeTrackClient()
{
  // Unload DLL file
  FreeLibrary(hinstLib);
}

bool FreeTrackClient::GetData(struct FreeTrackData &out)
{
  if ( getData(&data) )
  {
    out.dataID = data.DataID;
    out.yaw = data.Yaw;
    out.pitch = data.Pitch;
    out.roll = data.Roll;
    out.x = data.X;
    out.y =  data.Y;
    out.z = data.Z;
    return true;
  }
  else
  {
    // Nothing returned from getData
    out.dataID = 0;
    out.yaw = out.pitch = out.roll = out.x = out.y = out.z = 0;
  }
  return false;
}

#endif
