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
		ImmelmannController implements Lift Vector Turn maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT LiftVectorTurnController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        class Params
        {
        public:
            /** Enemy aircraft that will be taken as reference during this
             *  maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*> m_target;
			
            /** Speed during the maneuver in Knots. If not specified then the
             *  current speed is kept.*/
            RTD::DynamicControllerParameterReal m_speedKts ;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool m_useAirBrakes;

            /** Load factor to be used during this maneuver. Default value is
             *  the maximum load factor of the aircraft.*/
            RTD::DynamicControllerParameterReal m_loadFactor ;
        };

        Params   m_dynParams ;

        LiftVectorTurnController( AirplaneBot* controlledBot , const std::string& name );

        virtual ~LiftVectorTurnController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

    protected:
		RTD::PIDController*	mpid_roll;
    };

}

#endif