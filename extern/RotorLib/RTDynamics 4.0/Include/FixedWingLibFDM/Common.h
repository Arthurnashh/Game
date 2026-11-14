/*
(c) Copyright 2006, RTDynamics
All Rights Reserved.

Product:		FixedWingLib
Subsystem:		FixedWingLibFDM

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

/**
*	The Common.h includes all common header files and definitions
*	required by the RTD headers.
*/



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
#if __GNUC__ >= 3
#include <ostream>
#endif

#include <vector>

#include "Util/smDateTime.h"
#include "Physics/smMath.h"
#include "Physics/smMatrix.h"
#include "Physics/smQuat.h"
#include "Physics/smTypes.h"
#include "Physics/smVecTypes.h"
#include "Util/smEventHandlerAutoPrint.h"
#include "Util/smManager.h"
#include "Util/smIntegrator.h"
#include "Util/smPrinter.h"


/**
*	\namespace FixedWingLibFDM
*
*	The FixedWingLibFDM namespace includes aircraft dynamics models for simulating
*	fixed wing aircraft.
*/