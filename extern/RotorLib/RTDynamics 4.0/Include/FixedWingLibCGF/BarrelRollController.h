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
		BarrelRollController implements Barrel Roll maneuver. Details of the 
		execution of the maneuver can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
	class RTDYNAMICS_EXPORT BarrelRollController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
            /** Target aircraft. If target is not specified or is inactive
             *  level flight is executed.*/
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*> m_target;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool      m_useAirBrakes;
        };

        Params                  m_dynParams ;

		smReal					mp_pullUpPitch;
		smReal					mp_loadFactor;
    public:
        BarrelRollController( AirplaneBot* controlledBot, const std::string& name );
        virtual ~BarrelRollController();

        virtual void initialize();

        virtual void finalize();

		virtual bool readParamStr( const std::string& parameterString , std::string& error );

        virtual void update( float deltaTime );

		virtual void levelWings( smTime deltaTime );
		virtual void pullUpNose( smTime deltaTime );
		virtual void roll( smTime deltaTime );
		virtual void pursuit( smTime deltaTime );

	protected:

		unsigned int m_firstDataLoggerChannelIndex;

		RTD::ReferenceEntityBase*	m_prevTarget;
		bool	m_leftRoll;

    };

}

#endif