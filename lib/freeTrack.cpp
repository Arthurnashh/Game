/************************************************************************
*	freeTrack.cpp
* Read and pass data to the structures used by struct Input.
* 
*	Assumes that a copy of the FreeTrackClient.dll is in the FreeTrack 
*	application folder, detected by windows registry query.
*
************************************************************************/

#include "wpch.hpp"

//#include <El/Common/perfProf.hpp>

#if defined(_WIN32) 

#include "freeTrack.hpp"

#include "keyInput.hpp"
#include "global.hpp"
#include <El/ParamFile/paramFile.hpp>
#include <El/Evaluator/expressImpl.hpp>

FreeTrack::FreeTrack()
{
  dllPath = RString("");
  for (int i=0; i<NAxes; i++) 
  {
    axisOld[i] = 0;
  }
  freeTrackActive = false;
  enabled = true;
  GameDataNamespace globals(NULL, RString(), false); // save - no global namespace
  extern bool ParseUserParams(ParamFile &cfg, GameDataNamespace *globals);
  ParamFile cfg;
  ParseUserParams(cfg, &globals);
  ParamEntryPtr entry = cfg.FindEntry("freeTrack");
  if (entry) enabled=*entry;
}

void FreeTrack::Init()
{
  // Locate the FreeTrack DLL
  dllPath = GetDllLocation();
  if (dllPath==RString("")) return;

  // Initialize the the FreeTrack DLL
  if( freeTrackClient.Init( dllPath ) )
    LogF( "FreeTrack -- initialize OK." );
  else {
    LogF( "FreeTrack -- initialization FAILED." ); return;
  }
  freeTrackActive = true;
}

RString FreeTrack::GetDllLocation()
{
  //find path to FreeTrackClient.dll
  HKEY pKey = NULL;
  //open the registry key 
  if (::RegOpenKeyEx(HKEY_CURRENT_USER,
    "Software\\Freetrack\\FreetrackClient",
    0,
    KEY_READ,
    &pKey) != ERROR_SUCCESS)
  {
    //error condition
    LogF("FreeTrack - DLL Location key not present");
    return RString("");
  }

  //get the value from the key
  DWORD dwSize;
  //first discover the size of the value
  if (RegQueryValueEx(pKey,
    "Path",
    NULL,
    NULL,
    NULL,
    &dwSize) == ERROR_SUCCESS)
  {
    //allocate memory for the buffer for the value
    Temp<unsigned char> szValue(dwSize); 
    if (szValue == NULL)//error
    {
      RptF("FreeTrack - insufficient memory!");
      return RString("");
    }
    //now get the value
    if (RegQueryValueEx(pKey,
      "Path",
      NULL,
      NULL,
      szValue,
      &dwSize) == ERROR_SUCCESS)
    {
      //everything worked
      ::RegCloseKey(pKey);
      return RString(reinterpret_cast<const char *>(szValue.Data()));
    }
    else//error
    {
      LogF("FreeTrack - Error reading location key!");
    }
  }
  ::RegCloseKey(pKey);
  return RString("");

  //suppose the file is located at the same EXE directory
  // return RString("FreeTrackClient.cll");
}

const float FreeTrackMaxTraverseValueInv = 1/50.0f; //360.0f/(2*3.14f);
const float FreeTrackMaxRotValueInv = 2.0f; 
bool FreeTrack::Get(float axis[NAxes])
{
  for (int i=0; i<NAxes; i++) axis[i]=axisOld[i];

  if (freeTrackActive && enabled) 
  {
    // Query the FreeTrackClient for the latest data
    FreeTrackData data;
    if ( freeTrackClient.GetData(data) )
    {
      // In your own application, this is where you would utilize
      // the Tracking Data for View Control / etc.
      axis[0] = -data.pitch * FreeTrackMaxRotValueInv;
      axis[1] =  data.yaw * FreeTrackMaxRotValueInv;
      axis[2] =  data.roll * FreeTrackMaxRotValueInv;
      axis[3] =  data.x * FreeTrackMaxTraverseValueInv;
      axis[4] =  data.y * FreeTrackMaxTraverseValueInv;
      axis[5] =  data.z * FreeTrackMaxTraverseValueInv / Input::TrackIRzfactor; //special Z axis handling not used for FreeTrack (combine inverse TrackIRzfactor to eliminate the effect)
      for (int i=0; i<NAxes; i++) axisOld[i]=axis[i];
      return true;
    }
  }
  // FreeTrack not working?
  for (int i=0; i<NAxes; i++)
  {
    axisOld[i] *= 0.85; // not too fast attenuation to zero position after FreeTrack is not working properly
  }
  return false;
}

void FreeTrack::Enable(bool val)
{
  enabled=val;
  if ( !enabled )
  { //reset TrackIR axis values
    GInput.ResetTrackIRValues();
  }
}

#endif

