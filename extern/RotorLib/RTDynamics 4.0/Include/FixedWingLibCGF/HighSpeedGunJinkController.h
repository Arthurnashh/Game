#pragma once

#include "RTD/BotBase.h"
#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
namespace RTD
{
    class PIDController;
}

namespace FixedWingLibCGF
{
    class AirplaneBot;    

    /**		
		HighSpeedGunJinkController implements High Speed Gun Jink maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note than this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT HighSpeedGunJinkController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
			ROLL,
            WATCH_AND_TURN,
            LEVEL_FLIGHT
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
            /** Enemy aicraft (attacker) which well be taken as reference during
             *  the maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>  m_target;

            /** Load factor to be used during this maneuver. Default value is
             *  the maximum load factor of the aircraft. */
            RTD::DynamicControllerParameterReal                     m_loadFactor ;
            
            /** Incremental bank angle in degrees. Default value is 30
             *  degrees. */
			RTD::DynamicControllerParameterReal                     m_bankAngle ;

            /** Maximum bank angle in degrees. Default value is 120
             *  degrees. */
			RTD::DynamicControllerParameterReal                     m_maximumBankAngle ;


            /** Speed during the maneuver in Knots. Default value is the maximum
             *  speed of the aircraft. */
            RTD::DynamicControllerParameterReal                     m_speedKts;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool						m_useAirBrakes;

            /** The bot checks if the target is tracking at certain
             *  intervals. Use m_checkInterval to specify this interval in
             *  seconds.
             **/
            RTD::DynamicControllerParameterReal                     m_checkInterval;


        };

        Params   m_dynParams ;

        HighSpeedGunJinkController( AirplaneBot* controlledBot , const std::string& name );

        virtual void initialize();
        virtual void finalize() {};


        virtual void update( float deltaTime );

		virtual void roll(float deltaTime);
        virtual void watchAndTurn(float deltaTime);
        virtual bool isTargetTracking();

    protected:
        State   m_state;
        smDouble	m_watchStartTime;
		smReal		m_desiredRoll;
    };

}

#endif