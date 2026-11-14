/*
 * Copyright (c) 2005 Erwin Coumans http://continuousphysics.com/Bullet/
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erwin Coumans makes no representations about the suitability 
 * of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
*/
#ifndef VEHICLE_RAYCASTER_H
#define VEHICLE_RAYCASTER_H

#include "LinearMath/btVector3.h"

/// btVehicleRaycaster is provides interface for between vehicle simulation and raycasting
struct btVehicleRaycaster
{
	virtual ~btVehicleRaycaster()
	{
	}
	struct btVehicleRaycasterResult
	{
		btVehicleRaycasterResult() :m_distFraction(btScalar(-1.)), m_material(0) {};
		btVector3	m_hitPointInWorld;
		btVector3	m_hitNormalInWorld;
		btScalar	m_distFraction;
		const btMaterial* m_material;
	};
	virtual btScalar GetWaterLevel(btScalar x, btScalar y) const
	{
		return btScalar(0.0f);
	}

	virtual void prepare(btCollisionObject* exclude, const btVector3& mins, const btVector3& maxs)=0;
	virtual btCollisionObject* castRay(btCollisionObject* exclude, const btVector3& from,const btVector3& to, btVehicleRaycasterResult& result) = 0;

};

#endif //VEHICLE_RAYCASTER_H

