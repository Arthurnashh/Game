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
		LowSpeedGunJinkController implements Low-speed Gun Jink maneuver. Details 
		of the execution of the maneuver can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
	*/
    class RTDYNAMICS_EXPORT LowSpeedGunJinkController : public RTD::BotControllerBase
    {
    public:
		enum State 
		{
			RIGHT_JINK = 0,
			LEFT_JINK,
			LEVEL_WINGS
		};

		/** This struct contains the dynamic parameters of this
		 *  maneuver. */
        struct Params
        {
			/** Enemy (attacker) aircraft that will be taken as reference during
			 *  the maneuver. */
			RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>  m_target;

			/** Load factor to be used during this maneuver. Default value is
			 *  the maximum load factor of the aircraft. */
            RTD::DynamicControllerParameterReal		m_desiredLoadFactor;			

			/** Distance of the reversal referance point from the CG along the
			 *  longitudinal axis of the airplane (feet). Default value is
			 *  one feet. */
			RTD::DynamicControllerParameterReal		m_reversalRefPointFeet;

			/** It can happend that just after a reversal, enemy acquires a firing solution
				again. When this happens another reversal should be made. This can result in 
				continious left/right banking without really turning in any direction.

				To prevent this, after a reversal is initiated we do not make a second reversal for 
				a certain period of time. This parameter gives the time when the next reversal
				can occur. The parameter is in seconds and its default value is 5.
			*/
			RTD::DynamicControllerParameterReal		m_reversalCheckInterval;
			
        };

        Params   m_dynParams ;

        LowSpeedGunJinkController( AirplaneBot* controlledBot , const std::string& name );

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

		virtual void	levelWings(smTime deltaTime);
		virtual void	jink(smTime deltaTime);
		virtual bool	checkReversalConditionAndSetNewState();
    protected:

		smReal	m_checkTimer;
		State	m_state;
		bool	m_selfIsInTheRightHemisphereOfEnemy;
		
		RTD::ReferenceEntityBase*	m_prevEnemy;

    };

}

#endif