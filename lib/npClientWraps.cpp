// *******************************************************************************
// *
// * Module Name:
// *   NPClientWraps.cpp
// *
// * Software Engineer:
// *   Doyle Nickless - GoFlight Inc., for Eye Control Technology.
// *
// * Abstract:
// *   This module implements the wrapper code for interfacing to the NaturalPoint
// *   Game Client API.  Developers of client apps can include this module into
// *   their projects to simplify communication with the NaturalPoint software.
// *
// *   This is necessary since the NPClient DLL is run-time linked rather than
// *   load-time linked, avoiding the need to link a static library into the
// *   client program (only this module is needed, and can be supplied in source
// *   form.)
// *
// * Environment:
// *   User mode
// *
// *******************************************************************************
//
//#include "stdafx.h"
#ifdef _WIN32

#include "wpch.hpp"
#include <Es/Common/win.h>

#include "npClient.h"
#include "npClientWraps.h"
#include "string.h"

/////////////
// Defines ///////////////////////////////////////////////////////////////////////
/////////////
//

/////////////////
// Global Data ///////////////////////////////////////////////////////////////////
/////////////////
//
PF_NP_REGISTERWINDOWHANDLE       gpfNP_RegisterWindowHandle = NULL;
PF_NP_UNREGISTERWINDOWHANDLE     gpfNP_UnregisterWindowHandle = NULL;
PF_NP_REGISTERPROGRAMPROFILEID   gpfNP_RegisterProgramProfileID = NULL;
PF_NP_QUERYVERSION               gpfNP_QueryVersion = NULL;
PF_NP_REQUESTDATA                gpfNP_RequestData = NULL;
PF_NP_GETSIGNATURE               gpfNP_GetSignature = NULL;
PF_NP_GETDATA                    gpfNP_GetData = NULL;
PF_NP_STARTCURSOR                gpfNP_StartCursor = NULL;
PF_NP_STOPCURSOR                 gpfNP_StopCursor = NULL;
PF_NP_RECENTER	                 gpfNP_ReCenter = NULL;
PF_NP_GETTRACKIRDATA             gpfNP_GetTrackIRData = NULL;
PF_NP_STARTDATATRANSMISSION      gpfNP_StartDataTransmission = NULL;
PF_NP_STOPDATATRANSMISSION       gpfNP_StopDataTransmission = NULL;

HMODULE ghNPClientDLL = (HMODULE)NULL;

NPRESULT PostProcessResult(LPTRACKIRDATA TID, long AppKeyHigh, long AppKeyLow);

////////////////////////////////////////////////////
// NaturalPoint Game Client API function wrappers /////////////////////////////
////////////////////////////////////////////////////
//
NPRESULT __stdcall NP_RegisterWindowHandle( HWND hWnd  )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_RegisterWindowHandle )
		result = (*gpfNP_RegisterWindowHandle)( hWnd );

	return result;
} // NP_RegisterWindowHandle()


NPRESULT __stdcall NP_UnregisterWindowHandle()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_UnregisterWindowHandle )
		result = (*gpfNP_UnregisterWindowHandle)();

	return result;
} // NP_UnregisterWindowHandle()


NPRESULT __stdcall NP_RegisterProgramProfileID( unsigned short wPPID )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_RegisterProgramProfileID )
		result = (*gpfNP_RegisterProgramProfileID)( wPPID );

	return result;
} // NP_RegisterProgramProfileID()


NPRESULT __stdcall NP_QueryVersion( unsigned short* pwVersion )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_QueryVersion )
		result = (*gpfNP_QueryVersion)( pwVersion );

	return result;
} // NP_QueryVersion()


NPRESULT __stdcall NP_RequestData( unsigned short wDataReq )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_RequestData )
		result = (*gpfNP_RequestData)( wDataReq );

	return result;
} // NP_RequestData()

NPRESULT __stdcall NP_GetSignature( LPTRACKIRSIGNATURE pSignature )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_GetSignature )
		result = (*gpfNP_GetSignature)( pSignature );

	return result;
} // NP_GetSignature()


NPRESULT __stdcall NP_GetData( LPTRACKIRDATA pTID, long AppKeyHigh, long AppKeyLow )
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_GetData )
    {
        if(AppKeyHigh==0 && AppKeyLow==0)
            return NP_ERR_INVALID_KEY;

		result = (*gpfNP_GetData)( pTID );
        if(result==NP_OK)
        {
            result = PostProcessResult(pTID, AppKeyHigh, AppKeyLow);
        }
        else
            memset(pTID, 0, sizeof(tagTrackIRData));
    }

    
	return result;
} // NP_GetData()

NPRESULT __stdcall NP_StartCursor()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_StartCursor )
		result = (*gpfNP_StartCursor)();

	return result;
} // NP_StartCursor()


NPRESULT __stdcall NP_StopCursor()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_StopCursor )
		result = (*gpfNP_StopCursor)();

	return result;
} // NP_StopCursor()


NPRESULT __stdcall NP_ReCenter()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_ReCenter )
		result = (*gpfNP_ReCenter)();

	return result;
} // NP_ReCenter()


NPRESULT __stdcall NP_StartDataTransmission()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_StartDataTransmission )
		result = (*gpfNP_StartDataTransmission)();

	return result;
} // NP_StartDataTransmission()


NPRESULT __stdcall NP_StopDataTransmission()
{
	NPRESULT result = NP_ERR_DLL_NOT_FOUND;

	if( NULL != gpfNP_StopDataTransmission )
		result = (*gpfNP_StopDataTransmission)();

	return result;
} // NP_StopDataTransmission()


//////////////////////////////////////////////////////////////////////////////
// NPClientInit() -- Loads the DLL and retrieves pointers to all exports
//
NPRESULT NPClient_Init( RString csDLLPath )
{
    NPRESULT result = NP_OK;
    RString csNPClientDLLFullPath;
    if( 0 != csDLLPath.GetLength() )
      csNPClientDLLFullPath = csDLLPath + "\\";
    csNPClientDLLFullPath = csNPClientDLLFullPath + "NPClient.dll";
    ghNPClientDLL = ::LoadLibrary( csNPClientDLLFullPath );
    if( NULL != ghNPClientDLL )
		{
			// verify the dll signature
			gpfNP_GetSignature             = (PF_NP_GETSIGNATURE)::GetProcAddress( ghNPClientDLL, "NP_GetSignature" );

			SIGNATUREDATA pSignature;
			SIGNATUREDATA verifySignature;
			// init the signatures
			strcpy(verifySignature.DllSignature, "precise head tracking\n put your head into the game\n now go look around\n\n Copyright EyeControl Technologies");
			strcpy(verifySignature.AppSignature, "hardware camera\n software processing data\n track user movement\n\n Copyright EyeControl Technologies");
			// query the dll and compare the results
			NPRESULT vresult = NP_GetSignature( &pSignature );
			if( vresult == NP_OK )
			{
				if ((strcmp(verifySignature.DllSignature,pSignature.DllSignature)==0) 
					&& (strcmp(verifySignature.AppSignature,pSignature.AppSignature)==0))
				{	
					result = NP_OK;	

					// Get addresses of all exported functions
					gpfNP_RegisterWindowHandle     = (PF_NP_REGISTERWINDOWHANDLE)::GetProcAddress( ghNPClientDLL, "NP_RegisterWindowHandle" );
					gpfNP_UnregisterWindowHandle   = (PF_NP_UNREGISTERWINDOWHANDLE)::GetProcAddress( ghNPClientDLL, "NP_UnregisterWindowHandle" );
					gpfNP_RegisterProgramProfileID = (PF_NP_REGISTERPROGRAMPROFILEID)::GetProcAddress( ghNPClientDLL, "NP_RegisterProgramProfileID" );
					gpfNP_QueryVersion             = (PF_NP_QUERYVERSION)::GetProcAddress( ghNPClientDLL, "NP_QueryVersion" );
					gpfNP_RequestData              = (PF_NP_REQUESTDATA)::GetProcAddress( ghNPClientDLL, "NP_RequestData" );
					gpfNP_GetData                  = (PF_NP_GETDATA)::GetProcAddress( ghNPClientDLL, "NP_GetData" );
					gpfNP_StartCursor              = (PF_NP_STARTCURSOR)::GetProcAddress( ghNPClientDLL, "NP_StartCursor" );
					gpfNP_StopCursor               = (PF_NP_STOPCURSOR)::GetProcAddress( ghNPClientDLL, "NP_StopCursor" );
					gpfNP_ReCenter	               = (PF_NP_RECENTER)::GetProcAddress( ghNPClientDLL, "NP_ReCenter" );
					gpfNP_StartDataTransmission    = (PF_NP_STARTDATATRANSMISSION)::GetProcAddress( ghNPClientDLL, "NP_StartDataTransmission" );
					gpfNP_StopDataTransmission     = (PF_NP_STOPDATATRANSMISSION)::GetProcAddress( ghNPClientDLL, "NP_StopDataTransmission" );
				}
				else
				{	
					result = NP_ERR_DLL_NOT_FOUND;	
				}
			}
			else
			{
				result = NP_ERR_DLL_NOT_FOUND;	
			}
		}
		else
		result = NP_ERR_DLL_NOT_FOUND;

	return result;

} // NPClient_Init()



#define uint16_t __int16
#define uint32_t __int32
#define uint64_t __int64

#define get16bits(d) (*((const uint16_t *) (d)))

uint32_t SuperFastHash (const char * data, int len) {
uint32_t hash = len, tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

void NP_TIREnhancedUnHash(char *Buffer, int BufferSize, const char *HashKey, int HashKeyLength)
{
    int keyIndexPointer = 0;

    unsigned char rollingHash = 0x88;

    for(int i=BufferSize-1; i>=0; i--)
    {
        Buffer[i] = Buffer[i] ^ HashKey[keyIndexPointer];
        Buffer[i] = Buffer[i] ^ rollingHash;

        rollingHash += Buffer[i] + i;

        keyIndexPointer = (keyIndexPointer+1)%HashKeyLength;
    }
}

NPRESULT PostProcessResult(LPTRACKIRDATA tir, long AppKeyHigh, long AppKeyLow)
{
    __int32 AppKey[2];

    AppKey[0] = AppKeyHigh;
    AppKey[1] = AppKeyLow;

    NP_TIREnhancedUnHash((char*)tir,sizeof(TRACKIRDATA), (char*)AppKey, 8);

    int checkValue = tir->dwNPIOData;
    tir->dwNPIOData = 0;

    int hashValue = SuperFastHash((const char*)tir,sizeof(TRACKIRDATA));

	tir->fNPRawX = 0;
	tir->fNPRawY = 0;
	tir->fNPRawZ = 0;
	tir->fNPDeltaX = 0;
	tir->fNPDeltaY = 0;
	tir->fNPDeltaZ = 0;
	tir->fNPSmoothX = 0;
	tir->fNPSmoothY = 0;
	tir->fNPSmoothZ = 0;
    
    if(checkValue == hashValue)
        return NP_OK;

    return NP_ERR_FAILED;
}

//////////////////////////////////////////////////////////////////////////////
#endif //_WIN32
