#pragma once

#include "RTD/BotBase.h"
#include "RTD/ReferenceEntity.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace RTD
{
	class PIDController;
}

namespace FixedWingLibCGF
{

    class AirplaneBot;

    /**		
		PursuitController implements Pure, Lead and Lag Pursuit maneuvers. Details 
		of the execution of the maneuver can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
	*/    
    class RTDYNAMICS_EXPORT PursuitController : public RTD::BotControllerBase
    {
    public:

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {	
			/** Aircraft which will be taken as reference during the maneuver. */
			RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*> m_target;

			/** Load factor to be used during this maneuver. Default value is the maximum load factor of the aircraft.*/            
            RTD::DynamicControllerParameterReal   m_desiredLoadFactor;

			/** When true \e m_desiredSpeedKts will be ignored and the controller will
				try to fly with target's speed. Default value is false.*/				
			RTD::DynamicControllerParameterBool	  m_useTargetSpeed;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool      m_useAirBrakes;

            /** Speed during the maneuver in knots. This parameter is ignored
            *   if \e m_useTargetSpeed is true. If not specified current speed is kept.*/
			RTD::DynamicControllerParameterReal   m_desiredSpeedKts;

            /** This paremeter defines the lead or lag distance. For a pure pursuit
            *   set it to zero. Positive values denote lead pursuit whereas
            *   negative values lag pursuit. This parameter is in meters.
            *   Default value is 0. */
			RTD::DynamicControllerParameterReal   m_desiredLeadLag;

            /** PursuitController will try to remain in an altitude which is
            *   \e m_altitudeThresholdFeet above or below the enemy. When it is
            *   outside this band it will try to return to it. This parameter
            *   is in Feet and its default value is 50. */
			RTD::DynamicControllerParameterReal   m_altitudeThresholdFeet;
			
			/** If this parameter is specified, PursuitController will fly at the
				altitude which is determined by this parameter. If it is unspecified,
				then PursuitController will follow enemy altitude. See also 
				m_altitudeThresholdFeet	for additional information.

				This parameter does not have a default, in other words, it is unspecified
				by default.
			*/
			RTD::DynamicControllerParameterReal   m_desiredAltitudeFeet;
        };

        Params   m_dynParams ;


        PursuitController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~PursuitController();
        
		virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

	protected:
		unsigned int m_firstDataLoggerChannelIndex;

		RTD::ReferenceEntityBase*	m_prevTarget;

		smVec3r						c_Ptarget;
		smVec3r						c_Vtarget;
		smVec3r						c_Ptarget_filtered;
		smVec3r						c_Vtarget_filtered;
		smReal						c_isFollowingEnemyFrom6;
		smReal						c_AoAgr;
		smReal						c_azimuthOffset;

    };

}
#endif