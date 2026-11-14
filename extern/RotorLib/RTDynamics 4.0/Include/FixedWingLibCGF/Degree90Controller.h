#pragma once

#include "RTD/ReferenceEntity.h"
#include "RTD/BotBase.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace FixedWingLibCGF
{
	class AirplaneBot;
	
    /**		
		Degree90Controller implements Degree 90 maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT Degree90Controller : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        class Params
        {
        public:
			/** Aircraft which will be taken as reference during the maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>  m_target;

            /** Load factor to be used during this maneuver. Default is the maximum
                load factor of the airplane. */
            RTD::DynamicControllerParameterReal                     m_loadFactor ;

            /** Speed during the maneuver in Knots. This parameter does not have
             *  a default. If a speed is not specified, current speed is kept.*/
            RTD::DynamicControllerParameterReal                     m_speedKts ;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool					m_useAirBrakes;

        };

        Params   m_dynParams ;

        Degree90Controller( AirplaneBot* controlledBot , const std::string& name );

        virtual ~Degree90Controller();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );
    };

}

#endif