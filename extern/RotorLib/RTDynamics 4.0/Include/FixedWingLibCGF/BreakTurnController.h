#pragma once

#include "RTD/ReferenceEntity.h"
#include "RTD/BotBase.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES


namespace FixedWingLibCGF
{
	class AirplaneBot;    

    /**		
		BreakTurnController implements Break Turn maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT BreakTurnController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
            ROLL_FOR_HARDTURN = 0,
            HARDTURN,
            ACCEL_AND_LEVEL_OFF,
            LEVEL_FLIGHT
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        class Params
        {
        public:
			/** Attacker aircraft which is taken as reference during the maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>  m_target;

            /** Load factor to be used during this maneuver. Default is maximum load
                factor of the airplane.
             */
            RTD::DynamicControllerParameterReal                     m_loadFactor ;

        };

        Params   m_dynParams ;

        BreakTurnController( AirplaneBot* controlledBot , const std::string& name );

        virtual ~BreakTurnController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

        void    rollForHardTurn( float deltaTime);
        void    hardturn( float deltaTime );
        void    accelAndLevelOff( float deltaTime );

    protected:
        State   m_state;
    };

}
#endif