#include "wpch.hpp"

//#include <El/Common/perfProf.hpp>

#if defined(_WIN32) 

#include "trackIR.hpp"
#include "npClient.h"
#include "npClientWraps.h"

#include "keyInput.hpp"
#include "global.hpp"
#include <El/ParamFile/paramFile.hpp>
#include <El/Evaluator/expressImpl.hpp>

TrackIR::TrackIR(int trIRid, int keyHigh, int keyLow) : trackIRid(trIRid), appKeyHigh(keyHigh), appKeyLow(keyLow)
{
  isWorking = false;
  gcsDLLPath = "C:\\NP Client Interface 2\\Game Client DLL\\Debug";
  for (int i=0; i<N_TRACKIR_AXES; i++) 
  {
    axisOld[i] = 0;
  }
  trackIRActive = false;
  enabled = true;
  GameDataNamespace globals(NULL, RString(), false); // save - no global namespace
  extern bool ParseUserParams(ParamFile &cfg, GameDataNamespace *globals);
  ParamFile cfg;
  ParseUserParams(cfg, &globals);
  ParamEntryPtr entry = cfg.FindEntry("trackIR");
  if (entry) enabled=*entry;
}

TrackIR::~TrackIR()
{
  // Request that the TrackIR software stop sending Tracking Data
  NP_StopDataTransmission();
  // Un-register your applications Windows Handle
  NP_UnregisterWindowHandle();
}

void TrackIR::Init(HWND hwnd)
{
  NPRESULT result;

  // Zero TrackIR SDK Related counters
  npFrameSignature = 0;
  npStaleFrames = 0;

  // Locate the TrackIR Enhanced DLL
  gcsDLLPath = GetDllLocation();
  if (gcsDLLPath==RString("")) return;

  // Initialize the the TrackIR Enhanced DLL
  result = NPClient_Init( gcsDLLPath );
  if( NP_OK == result )
    LogF( "NPClient interface -- initialize OK." );
  else {
    LogF( "Error initializing NPClient interface!!" ); return;
  }

  // Register your applications Window Handle 
  //result = NP_RegisterWindowHandle( GetSafeHwnd() );
  result = NP_RegisterWindowHandle( hwnd );
  if( NP_OK == result )
    LogF( "NPClient : Window handle registration successful." );
  else {
    LogF( "NPClient : Error registering window handle!!" ); return;
  }

  // Query for the NaturalPoint TrackIR software version
  unsigned short wNPClientVer;
  result = NP_QueryVersion( &wNPClientVer );
  if( NP_OK == result )
    LogF("NaturalPoint software version is %d.%d", (wNPClientVer >> 8), (wNPClientVer & 0x00FF));
  else {
    LogF( "NPClient : Error querying NaturalPoint software version!!" ); return;
  }

  // Choose the Axes that you want tracking data for
  unsigned int dataFields = 0;

  // Rotation Axes
  dataFields |= NPPitch;
  dataFields |= NPYaw;
  dataFields |= NPRoll;

  // Translation Axes
  dataFields |= NPX;
  dataFields |= NPY;
  dataFields |= NPZ;

  // Register the Axes selection with the TrackIR Enhanced interface
  NP_RequestData(dataFields);

  // It is *required* that your application registers the Developer ID 
  // assigned by NaturalPoint!

  // Your assigned developer ID needs to be inserted below!

//#define NP_DEVELOPER_ID 7501

  NP_RegisterProgramProfileID(trackIRid);

  // Stop the cursor
  result = NP_StopCursor();
  if (result == NP_OK)
    LogF("Cursor stopped");
  else {
    LogF("NPCient : Error stopping cursor"); return;
  }

  // Request that the TrackIR software begins sending Tracking Data
  result = NP_StartDataTransmission();
  if (result == NP_OK)
  {
    LogF("Data transmission started");
    trackIRActive=true;
  }
  else {
    LogF("NPCient : Error starting data transmission"); return;
  }
}

RString TrackIR::GetDllLocation()
{
  //find path to NPClient.dll
  HKEY pKey = NULL;
  //open the registry key 
  if (::RegOpenKeyEx(HKEY_CURRENT_USER,
    "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location",
    0,
    KEY_READ,
    &pKey) != ERROR_SUCCESS)
  {
    //error condition
    LogF("Natural Point - Track IR: DLL Location key not present");
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
      RptF("Natural Point - Track IR: insufficient memory!");
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
      LogF("Natural Point - Track IR: Error reading location key!");
    }
  }
  ::RegCloseKey(pKey);
  return RString("");
}

bool TrackIR::Get(float axis[NAxes])
{
  TRACKIRDATA tid;
  RString csDataRxMsg;
  RString t_str;

  for (int i=0; i<NAxes; i++) axis[i]=axisOld[i];
  if (!trackIRActive || !enabled) return false; //no TrackIR, returned values are all zeros
  // Query the TrackIR Enhanced Interface for the latest data
  NPRESULT result = NP_GetData( &tid, appKeyHigh, appKeyLow);

  // If the call succeeded, then we have data to process
  if( NP_OK == result )
  {
    // Make sure the remote interface is active
    if (tid.wNPStatus == NPSTATUS_REMOTEACTIVE)
    {
      isWorking = true; //set the TrackIR is working and FreeTrack should not process anything!
      // Compare the last frame signature to the current one if 
      // they are not the same then the data is new
      if (npFrameSignature != tid.wPFrameSignature)
      {
        // In your own application, this is where you would utilize
        // the Tracking Data for View Control / etc.
        axis[0]=tid.fNPPitch/TrackIrMaxValue;
        axis[1]=tid.fNPYaw/TrackIrMaxValue;
        axis[2]=tid.fNPRoll/TrackIrMaxValue;
        axis[3]=tid.fNPX/TrackIrMaxValue; 
        axis[4]=tid.fNPY/TrackIrMaxValue; 
        axis[5]=tid.fNPZ/TrackIrMaxValue;
        for (int i=0; i<NAxes; i++) axisOld[i]=axis[i];
        npFrameSignature = tid.wPFrameSignature;
        npStaleFrames = 0;
        return true;
        //
        // All other data fields in TRACKIRDATA can be handled in a similar way.
        //
      }
      else
      {
        // Either there is no tracking data, the user has
        // paused the trackIR, or the call happened before
        // the TrackIR was able to update the interface
        // with new data
        for (int i=0; i<NAxes; i++) axisOld[i]=0;
        return false; //NP_ERR_NO_DATA;
      }
    }
    else
    {
      // The user has set the device out of trackIR Enhanced Mode
      // and into Mouse Emulation mode with the hotkey
      LogF("Natural Point - Track IR: User Disabled");
      return false;
    }
  } 
  return false;
}

void TrackIR::Enable(bool val)
{
  if (enabled!=val)
  {
    enabled = val;
    if (enabled)
      NP_StartDataTransmission();
    else
      NP_StopDataTransmission();
  }
}

#endif

