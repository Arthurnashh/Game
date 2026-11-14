/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include "Physics/smVec2r.h"
#include "Physics/smVec2d.h"
#include "Physics/smVec3r.h"
#include "Physics/smVec3d.h"
#include "Physics/smVec4r.h"
#include "Physics/smVec4d.h"

/**
* Some type definitions used to make life easier
*/
/*typedef smVec2<int>     smVec2i;
typedef smVec2<smReal>  smVec2r;
typedef smVec2<smDouble>	smVec2d;
typedef smVec3<int>     smVec3i;
typedef smVec3<smReal>  smVec3r;
typedef smVec3<smDouble>	smVec3d;
typedef smVec4<int>     smVec4i;
typedef smVec4<smReal>  smVec4r;
typedef smVec4<smDouble>	smVec4d;*/

typedef smVec3r smAccelVec ;
typedef smVec3r smForceVec ;
typedef smVec3r smPositionVec ;
typedef smVec3r smSpeedVec ;
typedef smVec3r smAngleDVec ;
typedef smVec3r smTorqueVec ;
typedef smVec3r smDimensionVec ;
typedef smVec3r smMomentumVec ;
typedef smVec3r smLinearMomentumVec ;
typedef smVec3r smAngularMomentumVec ;
typedef smVec3r smAngularRateVec ;
typedef smVec3r smAngularAccelVec ;

smVec3d smVec3rToVec3d( const smVec3r& vec );
smVec3r smVec3dToVec3r( const smVec3d& vec );

std::ostream& operator << (std::ostream& os, const smVec2r& x);
std::ostream& operator << (std::ostream& os, const smVec3r& x);
std::ostream& operator << (std::ostream& os, const smVec4r& x);
std::ostream& operator << (std::ostream& os, const smVec2d& x);
std::ostream& operator << (std::ostream& os, const smVec3d& x);
std::ostream& operator << (std::ostream& os, const smVec4d& x);

std::istream& operator >> (std::istream& is, smVec2r& x);
std::istream& operator >> (std::istream& is, smVec3r& x);
std::istream& operator >> (std::istream& is, smVec4r& x);
std::istream& operator >> (std::istream& is, smVec2d& x);
std::istream& operator >> (std::istream& is, smVec3d& x);
std::istream& operator >> (std::istream& is, smVec4d& x);
