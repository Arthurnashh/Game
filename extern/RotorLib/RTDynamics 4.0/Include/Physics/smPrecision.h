/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RotorLib
Subsystem:		RLDynamics

The content of this work is propriatery and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

/**
* Dina smReal or single precision selection.
* If smReal presicion is required, define SM_DOUBLE_PRECISION
* in the compiler settings and link with the appropriate
* version of library.
*/

#ifndef SM_DOUBLE_PRECISION
typedef	float	smReal;
#define	SM_REAL_MAX	FLT_MAX
#else
typedef	double	smReal;
#define	SM_REAL_MAX	DBL_MAX
#endif

typedef	double	smDouble;