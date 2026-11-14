// *******************************************************************************
// *
// * Module Name:
// *   NPClientWraps.h
// *
// * Software Engineer:
// *   Doyle Nickless - GoFlight Inc., for Eye Control Technology.
// *
// * Abstract:
// *   Header file for NPClientWraps.cpp module.
// *
// * Environment:
// *   User mode
// *
// *******************************************************************************
//
#ifndef _NPCLIENTWRAPS_H_DEFINED_
#define _NPCLIENTWRAPS_H_DEFINED_

#include "npClient.h"
#include <Es/Strings/rString.hpp>

/////////////
// Defines ///////////////////////////////////////////////////////////////////////
/////////////
//

/////////////////////////
// Function Prototypes ///////////////////////////////////////////////////////////
/////////////////////////
//
NPRESULT NPClient_Init( RString );

#endif // #ifdef _NPCLIENTWRAPS_H_DEFINED_
