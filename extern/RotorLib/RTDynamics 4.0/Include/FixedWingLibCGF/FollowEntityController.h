

#pragma once

#include "RTD/BotBase.h"
#include "RTD/PIDController.h"
#include "RTD/ReferenceEntity.h"
#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace RTD
{
	class PIDController;
}

namespace FixedWingLibCGF
{
    
	class AirplaneBot;

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
			/** Lateral distance from leader in Feet. Positive values are to the right of leader when viewed from above. */
            RTD::DynamicControllerParameterReal   m_lateralOffsetFeet;

			/** Longitudinal distance from leader in Feet. Positive values are to the front of the leader when viewed above.  
			*/
            RTD::DynamicControllerParameterReal   m_longitudinalOffsetFeet;

			/** Vertical distance from leader in Feet.  */
            RTD::DynamicControllerParameterReal   m_verOffsetFeet;

            /** Leader aircraft which will be taken as reference during this
                maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*> m_leader;
        };

        Params                  m_dynParams ;

    public:
        FollowEntityController( AirplaneBot* controlledBot, const std::string& name );
        virtual ~FollowEntityController();

        virtual void initialize();

        virtual void finalize();

		virtual bool readParamStr( const std::string& parameterString , std::string& error );

        virtual void update( float deltaTime );

		/** Return the position error in world coordinates. */
		const smVec3r& getPositionError() const { return c_Epos; }

	protected:

		bool	m_alignmentMode;
		bool    m_overshootDetected;
		smReal	m_overshootPreventionFactor;
		smReal	m_alignmentModeFuzzy;

		smVec3r	c_Epos;
		RTD::Vec3rDataHistory	c_EposHistory;
		smVec3r	c_Vtargetpoint;
		smVec3r	c_Vdistcorr;

		RTD::PIDController*	mpid_alignment;
		RTD::PIDController*	mpid_follow;
		RTD::PIDController*	mpid_vertical;

		smVec3r  c_Vleader;
		smVec3r	 c_Pleader;
		smVec3r  c_HPRleader;
		smVec3r  c_Wleader;

		smVec3r  c_Vleader_filtered;
		smVec3r	 c_Pleader_filtered;
		smVec3r  c_HPRleader_filtered;
		smVec3r  c_Wleader_filtered;

		RTD::Vec3rDataHistory	c_VdHistory;

		unsigned int m_firstDataLoggerChannelIndex;

		RTD::ReferenceEntityBase*	m_prevLeader;


    };

}

#endif