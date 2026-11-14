#pragma once

#include "RTD/BotBase.h"
#include "RTD/ReferenceEntity.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
namespace FixedWingLibCGF
{
    class AirplaneBot;

    /**		
		HardTurnController implements Hard Turn maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT HardTurnController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        class Params
        {
        public:
			/** Enemy aircraft which will be taken as reference during the maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*> m_target;

            /** Load factor to be used during this maneuver. Default value is maximum
                load factor of the airplane. */
            RTD::DynamicControllerParameterReal m_loadFactor ;

            /** Speed during the maneuver in Knots. Default value is the
                maximum speed of the aircraft. */
            RTD::DynamicControllerParameterReal m_speedKts ;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool					m_useAirBrakes;
        };

        Params   m_dynParams ;

        HardTurnController( AirplaneBot* controlledBot , const std::string& name );

        virtual ~HardTurnController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

    protected:
        smReal m_initAltitudeFeet;
        smReal  m_dirToTarget ;
    };

}

#endif