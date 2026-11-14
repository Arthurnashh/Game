/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once




#include <cmath>
#include <map>
#include <vector>
#include <deque>
#include <ctime>
#include <iostream>
#include <fstream>
#include <float.h>

#ifdef WIN32
#include <direct.h>
#endif

#include "Physics/smTypes.h"
#include "Physics/smMathMatrixR.h"
#include <Physics/smMath.h>
#include <Physics/smTypes.h>

#include "RTD/Base.h"
#include "RTD/BotBase.h"
#include "RTD/Math.h"
#include "RTD/PIDController.h"
#include "RTD/PIDControllerD.h"
#include "RTD/ParamDb.h"

/**
*	\namespace RotorLibCGF
*
*	The RotorLibCGF namespace includes flight dynamics and control classes for autonomous helicopters.
*/