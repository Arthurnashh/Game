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
		ReversalController implements Reversal Maneuver.Details of the execution of 
		the maneuver can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
	*/    
    class RTDYNAMICS_EXPORT ReversalController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
            /** Airplane is waiting for attacker's overshoot.  */
            WAITING = 0,
            /** Airplane is flying a lift vector turn as a part of the
             *  reversal maneuver. */
            PERFORMING_LIFT_VECTOR_TURN,
            /** Airplane is flying a pure pursuit as a part of the
             *  reversal maneuver. */
            PERFORMING_PURE_PURSUIT
        };

        /** EndingManeuver can be used to specify the maneuver the airplane
         *  will fly after completing the reversal maneuver. */
        enum EndingManeuver
        {
            /** Perform lift vector after completion of the reversal maneuver.*/
            LIFT_VECTOR_TURN,
            /** Perform pure pursuit after completion of the reversal
             *  maneuver. */
            PURE_PURSUIT,
            /** Perform one of the lift vector turn or pure pursuit maneuvers
             *  after completion of the reversal maneuver.  */
            RANDOM
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
			/** Aircraft which will be taken as reference during the maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>  m_target;

            /** Load factor to be used during this maneuver. Default value is
             *  the maximum load factor of the aircraft. */
            RTD::DynamicControllerParameterReal                     m_loadFactor ;

            /** Desired flight speed in knots. If a value is not specified, current speed is used. */
            RTD::DynamicControllerParameterReal                     m_speedKts;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool						m_useAirBrakes;

            /** The ending maneuver. When the attacker overshoots the
             *  defender’s 3-9 line, the ending maneuver will be performed.  Default value is PERFORMING_PURE_PURSUIT.
             *  
             *  See also \e EndingManeuver.     */
            RTD::DynamicControllerParameterEnum<EndingManeuver>		m_endingManeuver;
        };

        Params   m_dynParams ;

        ReversalController( AirplaneBot* controlledBot , const std::string& name );

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

        /** Returns the current state of the maneuver. */
        State   getState();

    protected:
        State   m_state;

        bool    m_lastIsTargetInFrontHemisphere;
    };

}


#endif