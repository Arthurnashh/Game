#pragma once

#include "RTD/BotBase.h"
#include "RTD/ReferenceEntity.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace FixedWingLibCGF
{
	
	class AirplaneBot;
    

    /**		
		LowYoyoController implements Low Yoyo maneuver. Details 
		of the execution of the maneuver can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
	*/    
    class RTDYNAMICS_EXPORT LowYoyoController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
			INITIAL_ROLL = 0,
        		DIVE ,
				CLIMB,
				PURSUIT,
				LEVEL_FLIGHT
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
			/** Aircraft which will be taken as reference during the maneuver. */
			RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>	m_target;
			
            /** Load factor to be used during this maneuver. Default value is
             *  the maximum load factor of the aircraft. */
			RTD::DynamicControllerParameterReal						m_desiredLoadFactor;			

			/** The purpose of the Low Yoyo maneuver is to gain speed advantage against the 
				enemy. When a certain amount of speed difference is gained during gain Low Yoyo
				controller finishes the dive phase of the maneuver. In other words, when 
				self speed > enemy speed + m_desiredSpeedAdvantageKts dive finishes. Default value
				is 50 Knots.
			*/
			RTD::DynamicControllerParameterReal						m_desiredSpeedAdvantageKts;

			/** This parameter is used to limit the amount of altitude loss during dive. 
			
				Low Yoyo controller monitors the angle between the vector from attacker to defender 
				and XY plane during dive. When this angle between exceeds this parameter dive
				phase is ended regardless of speed. Default value is 15 degrees.	
			*/
			RTD::DynamicControllerParameterReal						m_climbStartAngle;

			/** Attacker pursues the enemy during Low Yoyo maneuver. This pursuit can be lead,pure or lag
				type depending on this parameter. Default value is 33 meters.
			*/
			RTD::DynamicControllerParameterReal						m_leadLagAmount;


            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool						m_useAirBrakes;
        };

        Params   m_dynParams ;


        LowYoyoController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~LowYoyoController();
        
		virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

		State	getCurrentState() const	{	return m_state;	}

		virtual void	initialRoll(smTime deltaTime);
		virtual void	dive(smTime deltaTime);
		virtual void	climb(smTime deltaTime);
		virtual void	pursuit(smTime deltaTime);
		virtual void	levelFlight(smTime deltaTime);

    protected:
        State   m_state;

		bool			isTargetTurningLeft() const;

		unsigned int	m_firstDataLoggerChannelIndex;
		smVec3r			c_self2Target;
		smReal			c_verticalAngle2Target;
		smReal			c_enemySpeedKts;
		smReal			c_selfSpeedKts;
		smReal			c_climbStartAltitudeDiffFeet;

    };

}

#endif