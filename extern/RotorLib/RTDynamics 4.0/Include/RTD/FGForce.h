/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once


#include "RTD/ForceGenerator.h"
#include "RTD/VehicleBase.h"

namespace RTD
{
	/**
	*	FGForce generates (only) a force on the body.
	*/
	class RTDYNAMICS_EXPORT FGForce : public ForceGenerator
	{
	private:
		VehicleBase*		 m_parentVehicle ;
	public:
		FGForce( const std::string& name, VehicleBase* vehicle ) :
		  ForceGenerator( name , vehicle ), SimulationVariableUser( name , vehicle ), m_parentVehicle( vehicle )
		{
			// Set here, because it will not be changed later on.
			o_forceInWorldCS = smVec3r(0,0,0) ;

			s_force.init( this , "force" , smVec3r(0.0f,0.0f,0.0f) );
			s_forceScale.init( this , "forceScale" , 1.0f );
		}

		virtual void  update( smTime timestep )
		{
			o_forceInWorldCS = s_force.getValue() * s_forceScale ;
			o_momentAtCogInBodyCS= smVec3r(0,0,0) ;
		}

		void setForce( const smVec3r& force )
		{
			s_force = force;
		}

		RTD::StateVariableVec3r s_force;
		RTD::StateVariableReal	s_forceScale;
	};
}
