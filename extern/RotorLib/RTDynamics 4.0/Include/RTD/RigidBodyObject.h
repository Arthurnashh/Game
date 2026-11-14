/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/VehicleBase.h"
#include "RTD/GAVDynamics.h"

namespace RTD
{
	/**
	*	RigidBodyObject serves as a general purpose rigid body object. It is possible
	*	to decorate this object with custom or standard ForceGenerator based classes to
	*	simulate other kind of vehicles or objects such as missiles and rockets.
	*/
	class RigidBodyObject : public VehicleBase
	{
	public:
		friend class GAVDynamics;

		RigidBodyObject( World* world, const std::string& name, const smMass& mass = 1, const smDimensionVec& hullDims = smDimensionVec(1,1,1) ) : VehicleBase( world, name ),
			m_rigidBody( world, name + "Rigidbody")
		{
			m_rigidBody.computeAndStoreInertia( mass, hullDims ) ;

			m_dynEngine = new RTD::GAVDynamics(this) ;

			m_state = new RigidBodyState( name + ".State" );
		}

		virtual void update( smTime timestep )
		{
			if ( timestep != 0 )
				m_dynEngine->update( timestep ) ;
		}

		const ConfigParams* getConfigParams() const     {   return NULL;  }
		ConfigParams*       getConfigParams()           {   return NULL;  }

		virtual void			computeMassProperties() const { m_rigidBody.getMass(); }
		RTD::RigidBody&			getRigidBody()			{	return m_rigidBody;	}
		const RTD::RigidBody&	getRigidBody() const	{	return m_rigidBody;	}
	private:
		RTD::GAVDynamics*			m_dynEngine ;
		RTD::RigidBody				m_rigidBody;
	};
}
