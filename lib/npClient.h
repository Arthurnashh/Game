// *******************************************************************************
// *
// * Module Name:
// *   NPClient.h
// *
// * Doyle Nickless -- 13 Jan, 2003 -- for Eye Control Technology.
// *
// * Abstract:
// *   Header for NaturalPoint Game Client API.
// *
// * Environment:
// *   Microsoft Windows -- User mode
// *
// *******************************************************************************

#ifndef _NPCLIENT_H_DEFINED_
#define _NPCLIENT_H_DEFINED_

#pragma pack( push, npclient_h ) // Save current pack value
#pragma pack(1)

#include "NPCommon.h"
//
// Typedef for pointer to the notify callback function that is implemented within
// the client -- this function receives head tracker reports from the game client API
//
typedef NPRESULT (__stdcall *PF_NOTIFYCALLBACK)( unsigned short, unsigned short );

// Typedefs for game client API functions (useful for declaring pointers to these
// functions within the client for use during GetProcAddress() ops)
//
typedef NPRESULT (__stdcall *PF_NP_REGISTERWINDOWHANDLE)( HWND );
typedef NPRESULT (__stdcall *PF_NP_UNREGISTERWINDOWHANDLE)( void );
typedef NPRESULT (__stdcall *PF_NP_REGISTERPROGRAMPROFILEID)( unsigned short );
typedef NPRESULT (__stdcall *PF_NP_QUERYVERSION)( unsigned short* );
typedef NPRESULT (__stdcall *PF_NP_REQUESTDATA)( unsigned short );
typedef NPRESULT (__stdcall *PF_NP_GETSIGNATURE)( LPTRACKIRSIGNATURE );
typedef NPRESULT (__stdcall *PF_NP_GETDATA)( LPTRACKIRDATA );
typedef NPRESULT (__stdcall *PF_NP_REGISTERNOTIFY)( PF_NOTIFYCALLBACK );
typedef NPRESULT (__stdcall *PF_NP_UNREGISTERNOTIFY)( void );
typedef NPRESULT (__stdcall *PF_NP_STARTCURSOR)( void );
typedef NPRESULT (__stdcall *PF_NP_STOPCURSOR)( void );
typedef NPRESULT (__stdcall *PF_NP_RECENTER)( void );
typedef NPRESULT (__stdcall *PF_NP_GETTRACKIRDATA)( LPTRACKIRDATA );
typedef NPRESULT (__stdcall *PF_NP_STARTDATATRANSMISSION)( void );
typedef NPRESULT (__stdcall *PF_NP_STOPDATATRANSMISSION)( void );

//// Function Prototypes ///////////////////////////////////////////////
//
// Functions exported from game client API DLL ( note __stdcall calling convention
// is used for ease of interface to clients of differing implementations including
// C, C++, Pascal (Delphi) and VB. )
//
NPRESULT __stdcall NP_RegisterWindowHandle( HWND hWnd  );
NPRESULT __stdcall NP_UnregisterWindowHandle( void );
NPRESULT __stdcall NP_RegisterProgramProfileID( unsigned short wPPID );
NPRESULT __stdcall NP_QueryVersion( unsigned short* pwVersion );
NPRESULT __stdcall NP_RequestData( unsigned short wDataReq );
NPRESULT __stdcall NP_GetSignature( LPTRACKIRSIGNATURE pSignature );
NPRESULT __stdcall NP_GetData( LPTRACKIRDATA pTID, long AppKeyHigh, long AppKeyLow);
NPRESULT __stdcall NP_RegisterNotify( PF_NOTIFYCALLBACK pfNotify );
NPRESULT __stdcall NP_UnregisterNotify( void );
NPRESULT __stdcall NP_StartCursor( void );
NPRESULT __stdcall NP_StopCursor( void );
NPRESULT __stdcall NP_ReCenter( void );
NPRESULT __stdcall NP_TrackIR( LPTRACKIRDATA pTID );
NPRESULT __stdcall NP_StartDataTransmission( void );
NPRESULT __stdcall NP_StopDataTransmission( void );

/////////////////////////////////////////////////////////////////////////

#pragma pack( pop, npclient_h ) // Ensure previous pack value is restored

#endif // #ifdef NPCLIENT_H_DEFINED_

//
// *** End of file: NPClient.h ***
//


