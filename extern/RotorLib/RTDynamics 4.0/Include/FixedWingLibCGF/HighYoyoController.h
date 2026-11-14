#pragma once

#include "RTD/BotBase.h"
#include "RTD/ReferenceEntity.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
namespace FixedWingLibCGF
{

     class AirplaneBot;

    /**		
		HighYoyoController implements High Yoyo maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note than this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT HighYoyoController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
            LEVEL_WINGS = 0,
				CLIMB ,
				DIVE ,
                PURSUIT,
				LEVEL_FLIGHT
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
            /** Desired load factor during the maneuver. Default value of this
             *  parameter is the maximum load factor of the aircraft.*/
            RTD::DynamicControllerParameterReal						m_desiredLoadFactor;

            /** During High-Yoyo, airplane will first try to roll out to level
            *   its wings. When airplane's bank angle becomes less than \e
            *   m_climbStartBankAngle climb phase of High-Yoyo will start.
            *  
            *   Default value is 0 degrees.*/			    
			RTD::DynamicControllerParameterReal						m_climbStartBankAngle;

            /** Target aircraft. If not defined than aircraft will commence level
             *  flight. */
			RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>	m_target;

            /** During climb, pitch angle of the airplane will increase and it
            *   will gain altitude. When the pitch exceeds \e m_diveStartPitch
            *   aircraft will roll to fly inverted and start a dive towards the
            *   target aircraft.
            *  
            *   Default value is 45 degrees.*/
			RTD::DynamicControllerParameterReal						m_diveStartPitch;

			/** Attacker pursues the enemy during High Yoyo maneuver. This pursuit can be lead,pure or lag
				type depending on this parameter. Default value is 100 feet.
			*/
			RTD::DynamicControllerParameterReal						m_leadLagAmountFeet;


            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool						m_useAirBrakes;
        };

        Params   m_dynParams ;


        HighYoyoController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~HighYoyoController();
        
		virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

		State	getCurrentState() const	{	return m_state;	}

		virtual void	levelWings(smTime deltaTime);
		virtual void	climb(smTime deltaTime);
		virtual void	dive(smTime deltaTime);
		virtual void	pursuit(smTime deltaTime);
		virtual void	levelFlight(smTime deltaTime);

    protected:
        State   m_state;

		RTD::PIDController*	mpid_controller;
    };

}

#endif