#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <stdio.h>
#include <stdio.h>
#include <cassert>
#include <algorithm>
#include <string>
#include <set>
#include "RTD/Base.h"
#include "RTD/Common.h"
#include "RTD/BotBase.h"
#include "RTD/VehicleBase.h"
#include "RTD/Math.h"
#include "RTD/RTD.h"
#include "RTD/ParamDb.h"
#include "RTD/Terrain.h"
#include "RTD/FuelConsumptionModelBase.h"
#include "FixedWingLibCGF/SimplifiedAirplane.h"
#include "FixedWingLibCGF/FollowRouteController.h"
#include "FixedWingLibCGF/BasicFlightController.h"
#include "FixedWingLibCGF/BasicLowLevelController.h"
#include "FixedWingLibCGF/VelocityController.h"
#include "FixedWingLibCGF/FlightPathController.h"
#include "FixedWingLibCGF/PositionController.h"
#include "FixedWingLibCGF/DiveBombingController.h"
#include "FixedWingLibCGF/GroundAvoidanceController.h"
#include "FixedWingLibCGF/HardTurnController.h"
#include "FixedWingLibCGF/LoiterController.h"
#include "FixedWingLibCGF/ImmelmannController.h"
#include "FixedWingLibCGF/STurnController.h"
#include "FixedWingLibCGF/FollowEntityController.h"
#include "FixedWingLibCGF/SplitSController.h"
#include "FixedWingLibCGF/PursuitController.h"
#include "FixedWingLibCGF/ExtensionController.h"
#include "FixedWingLibCGF/BankLeftRightController.h"
#include "FixedWingLibCGF/HighYoyoController.h"
#include "FixedWingLibCGF/LowYoyoController.h"
#include "FixedWingLibCGF/LiftVectorTurnController.h"
#include "FixedWingLibCGF/BreakTurnController.h"
#include "FixedWingLibCGF/HeadOnController.h"
#include "FixedWingLibCGF/Degree90Controller.h"
#include "FixedWingLibCGF/HighSpeedGunJinkController.h"
#include "FixedWingLibCGF/LowSpeedGunJinkController.h"
#include "FixedWingLibCGF/ReversalController.h"
#include "FixedWingLibCGF/BarrelRollController.h"
#include "FixedWingLibCGF/ScissorController.h"
#include "FixedWingLibCGF/RaceTrackController.h"
#include "FixedWingLibCGF/DisengEgressController.h"
#include "RotorLibFDM/FCSHorizontalStabilizerController.h"
#include "RotorLibFDM/FCSHelicopterAttitudeController.h"
#include "RotorLibFDM/FCSHelicopterVerticalSpeedController.h"
#include "RotorLibFDM/FCSHelicopterVelocityController.h"
#include "RotorLibFDM/FCSHelicopterHoverController.h"

