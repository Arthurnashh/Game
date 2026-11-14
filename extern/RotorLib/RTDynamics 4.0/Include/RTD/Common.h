/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

/**
*   The Common.h includes all common header files and definitions
*   required by the RTD headers.
*/

#ifdef WIN32
#	ifdef RTDYNAMICS_USE_STATIC_LIBRARY
#		define RTDYNAMICS_EXPORT 
#   else
#		ifdef RTDYNAMICS_EXPORTS
#			define RTDYNAMICS_EXPORT _declspec(dllexport)
#		else
#			define RTDYNAMICS_EXPORT _declspec(dllimport)
#		endif
#	endif
#   pragma warning (disable  : 4251 )
#   include <windows.h>
#elif defined(__linux)
#   define RTDYNAMICS_EXPORT
#endif




#if /*[[[military_features]]]*/0/*[[[End]]]*/
#define RTDYNAMICS_EXPORT_MILITARY_FEATURES
#endif

#ifdef WIN32
#include <direct.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cmath>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#if __GNUC__ >= 3
#include <ostream>
#endif

#include <vector>

#include "Physics/smMath.h"
#include "Physics/smMatrix.h"
#include "Physics/smMathMatrixR.h"
#include "Physics/smQuat.h"
#include "Physics/smTypes.h"
#include "Physics/smVecTypes.h"
#include "Util/smEventHandlerAutoPrint.h"
#include "Util/smManager.h"
#include "Util/smIntegrator.h"
#include "Util/smPrinter.h"
#include "Util/smStringTokenIterator.h"


#define RTD_EXPAND_VEC3(v)  v[0],v[1],v[2]
#define RTD_EXPAND_VEC4(v)  v[0],v[1],v[2],v[3]
#define RTD_DELETE_MEMBERS_AND_CLEAR_VECTOR( stdvec )	while ( stdvec.size() > 0 ){ delete stdvec.back() ; stdvec.pop_back(); }

