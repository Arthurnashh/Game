/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/


#pragma once

#include "RotorLibCGF/Common.h"
#include "RTD/BotBase.h"
#include "RTD/PIDController.h"
#include "RTD/ReferenceEntity.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace RTD
{
	class PIDController;
}

namespace RotorLibCGF
{
    
	class RotorcraftBot;

    /** FollowEntityController implements a entity following maneuver. Details of the 
		controller can be found in the Programmer's Guide.

		This Controller is available only if Air Combat Maneuvers or Ground Attack Maneuvers  
		features are enabled.     
     **/ 
    class RTDYNAMICS_EXPORT FollowEntityController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
			/** Lateral distance from leader in Feet.  */
            RTD::DynamicControllerParameterReal   m_lateralOffsetFeet;

			/** Longitudinal distance from leader in Feet.  */
            RTD::DynamicControllerParameterReal   m_longitudinalOffsetFeet;

			/** Vertical distance from leader in Feet.  */
            RTD::DynamicControllerParameterReal   m_verOffsetFeet;

            /** Leader aircraft which will be taken as reference during this
                maneuver. */
            RTD::DynamicControllerParameterString m_leader;
        };

        Params                  m_dynParams ;

    public:
        FollowEntityController( RotorcraftBot* controlledBot, const std::string& name );
        virtual ~FollowEntityController();

        virtual void initialize();

        virtual void finalize();

		virtual void reset();

		virtual bool readParamStr( const std::string& parameterString , std::string& error );

        virtual void update( float deltaTime );

	protected:
		RTD::StateVariableVec3r	s_Epos;
		RTD::StateVariableVec3rDataHistory	s_EposHistory;
		
		RTD::StateVariableVec3r				s_Vtargetpoint;
		RTD::StateVariableVec3rDataHistory	s_VtargetpointHistory;
		RTD::StateVariableVec3rDataHistory	s_VdHistory;
		RTD::StateVariableString			s_leaderInPrevUpdate;

		RTD::PIDController*	mpid_closure;
		RTD::PIDController*	mpid_vertical;

		smVec3r  c_Vleader_filtered;
		smVec3r	 c_Pleader_filtered;
		smVec3r  c_HPRleader_filtered;
		smVec3r  c_Wleader_filtered;

		

		unsigned int m_firstDataLoggerChannelIndex;

		RTD::ReferenceEntityBase*	m_currentLeader;
    };

}
#endif