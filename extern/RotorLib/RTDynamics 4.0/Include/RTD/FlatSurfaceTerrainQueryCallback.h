/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:		RTDynamics
Subsystem:		RTDynamics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/Plane.h"
#include "RTD/Common.h"

namespace RTD
{
	/**	FlatSurfaceTerrainQueryCallback implements a TerrainQueryCallback and a virtual planar terrain model at the same time.
	*	Use this class to easily add a TerrainQueryCallback to a vehicle.
	*/
	class RTDYNAMICS_EXPORT FlatSurfaceTerrainQueryCallback : public RTD::TerrainQueryCallback
	{
		Plane3d m_plane;

	public:
		FlatSurfaceTerrainQueryCallback( RTD::World* world, const Plane3d& p = Plane3d( smVec3d(0,0,0), smVec3d(0,0,1) ) );

		virtual void setPlane( const Plane3d& p );
		virtual Plane3d& getPlane( ) { return m_plane; } 

		virtual bool query( const smVec3d& start, const smVec3d& end , smVec3d& hitXYZ , smVec3d& hitNormal, smVec3d& hitVelocity );
	};
}