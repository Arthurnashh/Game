/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "Util/smTimeBasedPulse.h"
#include "RTD/ForceGenerator.h"
#include "RTD/VehicleBase.h"

namespace RTD
{
	/**
	*	Torque generates (only) torque on the body
	*/
	class RTDYNAMICS_EXPORT FGTorque : public ForceGenerator
	{
	private:

		VehicleBase*		 m_parentVehicle ;
	public:
		FGTorque( const std::string& name, VehicleBase* vehicle ) :
		  ForceGenerator( name , vehicle ), SimulationVariableUser( name , vehicle ), m_parentVehicle( vehicle )
		{
			// Set here, because it will not be changed later on.
			o_forceInWorldCS = smVec3r(0,0,0) ;

			s_torqueVec.init( this , "torqueVec" , smVec3r(0.0f,0.0f,0.0f) );
			s_torqueScale.init( this , "torqueScale" , 1.0f );
		}

		virtual void  update( smTime timestep )
		{
			o_momentAtCogInBodyCS = s_torqueVec.getValue() * s_torqueScale ;
			o_forceInWorldCS = smVec3r(0,0,0) ;
		}

		void setTorque( const smTorqueVec& torque )
		{
			s_torqueVec = torque ;
		}

		RTD::StateVariableVec3r s_torqueVec;
		RTD::StateVariableReal	s_torqueScale;
	};
}
