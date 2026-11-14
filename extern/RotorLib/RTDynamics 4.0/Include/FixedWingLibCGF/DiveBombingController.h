/*
(c) Copyright 2009, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RotorLibCGF/Common.h"
#include "FixedWingLibCGF/Util.h"
#include "RTD/MBDVehicle.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace FixedWingLibCGF
{
	class AirplaneBot;

    /**		
		DiveBombingController implements Dive Bombing maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Ground Attack feature is activated.
    */ 
	class RTDYNAMICS_EXPORT DiveBombingController :	public RTD::BotControllerBase
	{
	public:
		

	public:
		class RTDYNAMICS_EXPORT DynamicControllerParameterOrdnance : public  RTD::DynamicControllerParameter<std::vector<RTD::MBDVehicle*> > 
		{
		public:

			
			DYNAMIC_CONTROLLER_PARAM_DECL_WITH_COPY( DynamicControllerParameterOrdnance , std::vector<RTD::MBDVehicle*> , GENERIC , RTD::DynamicControllerParameter<std::vector<RTD::MBDVehicle*> > );

			virtual std::string					toString() const;
			virtual std::vector<unsigned char>	toBinary() const;			
			virtual bool						fromBinary( const std::vector<unsigned char>& data );
		};


		DiveBombingController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~DiveBombingController(void);
		

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
		struct Params
		{
			Params()
			{
				m_ordnanceToRelease.setDefault( std::vector<RTD::MBDVehicle*>() );
				m_ordnanceToRelease = std::vector<RTD::MBDVehicle*>();
				m_ordnanceToRelease.setReturnDefaultWhenUnspecified(true);
			}

			/** Position of the target in nautical miles. */
			RTD::DynamicControllerParameterVec2r	m_targetPosNM;

			/** Altitude of the target in feet. */
			RTD::DynamicControllerParameterReal		m_targetAltitudeFeet;

			/** Approach altitude in feet. Airplane will fly towards the target át this altitude. */ 
			RTD::DynamicControllerParameterReal		m_desiredAltitudeFeet;

			/** Desired speed in knots. */
			RTD::DynamicControllerParameterReal		m_desiredSpeedKts;

			/** The airplane starts the dive when the angle between the line that is drawn from target to airplane
				and the XY plane exceeds m_diveStartAngle. This angle is in degrees. */
			RTD::DynamicControllerParameterReal		m_diveStartAngle;

			/** A list of the bombs to be released when airplane aims at the target. */
			DynamicControllerParameterOrdnance		m_ordnanceToRelease;

		};

		Params m_dynParams;

		smReal	mp_bombReleaseDistance;
		smReal	mp_aimControlP;
		smReal	mp_abortDecisionTime;
		smReal	mp_cruiseLoadFactor;
		smReal	mp_afterReleaseLoadFactor;
		smReal	mp_HUDBottomVisibilityAngle;
		
		smReal	mp_stickPullBackDistanceMultiplier;
		smReal	mp_stickPullbackAmount;

		smReal	mp_divePitchRate;
		smReal	mp_diveTargetPositionOnHud;

		smReal	mp_waitTimeAfterRelease;

		RTD::PIDController*	mpid_aimController;

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();

		virtual bool	needsGroundAvoidance();

		virtual void	cruise( float deltaTime );
		virtual void	dive( float deltaTime );
		virtual void	finish( float deltaTime );

	protected:		
		unsigned int	m_firstDataLoggerChannelIndex;

		virtual void	updateCommonInfo();
		virtual bool	updateFireSolution();

		smVec3r c_HPRself;
		smVec3r	c_Ptarget;
		smVec3r c_Rtarget;
		smReal  c_AZtarget;
		smReal  c_ELtarget;

		smVec3r c_Phit;
		smReal  c_Thit;
		smReal  c_Ehit;
		smReal  c_Eangle_hit;

		smDouble m_bombReleaseTime;
	};
}
#endif