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
		ScissorController implements Scissors Maneuver.Details of the execution of 
		the maneuver can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
	*/    
    class RTDYNAMICS_EXPORT ScissorController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
            /** Target aircraft. If target is not specified or is inactive,
             *  level flight is executed.*/
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>  m_target;

            /** Load factor to be used during this maneuver. Default is the maximum
                load factor of the airplane. */
            RTD::DynamicControllerParameterReal                     m_desiredLoadFactor ;

            /** Speed during the maneuver in Knots. When unspecified, the
             *  current speed of the aircraft will be used. */
            RTD::DynamicControllerParameterReal                     m_speedKts;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool						m_useAirBrakes;
        };

        Params                  m_dynParams ;

    public:
        ScissorController( AirplaneBot* controlledBot, const std::string& name );
        virtual ~ScissorController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

	protected:

    };

}

#endif