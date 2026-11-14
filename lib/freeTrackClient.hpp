#ifndef _FREETRACKCLIENT_HPP
#define _FREETRACKCLIENT_HPP

#include <Es/Common/win.h>
#include <Es/Types/pointers.hpp>
#include <Es/Strings/rString.hpp>

/// Structure to pass the FreeTrack data to the caller layer
struct FreeTrackData {
  unsigned int dataID;
  float        yaw;
  float        pitch;
  float        roll;
  float        x;
  float        y;
  float        z;
};

/// FreeTrackClient to get data from FreeTrackClient.dll
class FreeTrackClient
{
  struct FreeTrackDataInt
  {
    unsigned int DataID;
    int CamWidth;
    int CamHeight;
    // virtual pose
    float Yaw;   // positive yaw to the left
    float Pitch; // positive pitch up
    float Roll;  // positive roll to the left
    float X;
    float Y;
    float Z;
    // raw pose with no smoothing, sensitivity, response curve etc. 
    float RawYaw;
    float RawPitch;
    float RawRoll;
    float RawX;
    float RawY;
    float RawZ;
    // raw points, sorted by Y, origin top left corner
    float X1;
    float Y1;
    float X2;
    float Y2;
    float X3;
    float Y3;
    float X4;
    float Y4;
  };

  // DLL function signatures
  // These match those given in FTTypes.pas
  // WINAPI is macro for __stdcall defined somewhere in the depths of windows.h
  typedef bool (WINAPI *importGetData)(FreeTrackDataInt *data);
  typedef char *(WINAPI *importGetDllVersion)(void);
  typedef void (WINAPI *importReportName)(int name);
  typedef char *(WINAPI *importProvider)(void);

  //declare imported function pointers
  importGetData getData;
  importGetDllVersion getDllVersion;
  importReportName	reportName;
  importProvider provider;

  // create variables for exchanging data with the dll
  FreeTrackDataInt data;
  char *pDllVersion;
  char *pProvider;
  const static int name = 453;
  HINSTANCE hinstLib;

public:

  FreeTrackClient();
  ~FreeTrackClient();
  bool Init( RString dllPath );
  bool GetData(struct FreeTrackData &out);
};

#endif
