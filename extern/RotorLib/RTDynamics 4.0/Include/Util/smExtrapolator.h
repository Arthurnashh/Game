/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#include "Physics/smTypes.h"
#include "Physics/smVecTypes.h"

/**
*	smExtrapolator is the container class for all kinds of function based
*	extrapolators.
*/
class smExtrapolator
{
public:
	/**
	*	The simples extrapolation of all, the linear extrapolation.
	*/
	static void linear
		(
		const smVec3r&		vec,
		const smVec3r&		vecDot,
		const smTime&		deltaTime,
		smVec3r&			outVec
		)
	{
		outVec[0] = vec[0] + deltaTime * vecDot[0] ;
		outVec[1] = vec[1] + deltaTime * vecDot[1] ;
		outVec[2] = vec[2] + deltaTime * vecDot[2] ;
	}
};
