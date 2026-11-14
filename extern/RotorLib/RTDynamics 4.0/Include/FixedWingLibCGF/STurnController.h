#pragma once

#include "FixedWingLibCGF/FlightPathController.h"
#include "RTD/BotBase.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace FixedWingLibCGF
{
	class AirplaneBot;    

    /**		
		STurnController implements S Turn maneuver.Details of the execution of 
		the maneuver can be found in the Programmer's Guide.

		This Controller is available only if Air Combat Maneuvers or Ground Attack Maneuvers  
		features are enabled.     

	*/    
    class RTDYNAMICS_EXPORT STurnController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
            STATE_UNINITALIZED,
            STATE_FIRST_HALF_CIRCLE,
            STATE_SECOND_HALF_CIRCLE,
            STATE_FINAL_LEVEL_FLIGHT,
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
            /** If true S-turn maneuver will start by turning left. Default
             *  value is true. */
            RTD::DynamicControllerParameterBool     m_initTurnToLeft;

			/** Load factor to be used during this maneuver. Default value is 2.0g.*/ 
            RTD::DynamicControllerParameterReal   m_loadFactor;

            /** Speed during the maneuver in knots. If a speed is not
             *  specified, current speed will be kept. */
            RTD::DynamicControllerParameterReal   m_speedKts;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool	 m_useAirBrakes;
        };

        Params   m_dynParams ;
        smReal   m_initHeading ;
        smReal   m_initAltFeet ;
        smReal   m_targetHeading ;
        RTD::CircularFlightPath m_path ;

        State    m_state;

        const smReal  m_toleranceDeg ;

    public:
        STurnController( AirplaneBot* controlledBot , const std::string& name ) ;

        void updatePath( int phase );
        void initialize() ;
        void finalize() ;
        void update( float deltaTime ) ;

        /** Returns the current state of the maneuver.  */
        State getState() ;
    };

}

#endif