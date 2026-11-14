#pragma once

#include "RTD/ReferenceEntity.h"
#include "RTD/BotBase.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace FixedWingLibCGF
{
    class AirplaneBot;

    /**		
		DisengEgressController implements Disengage/Egress maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT DisengEgressController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        class Params
        {
        public:
			/** Aircraft which will be taken as reference during the maneuver. */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*>  m_target;

            /** Load factor to be used during this maneuver. Default value is
                the maximum load factor of the aircraft.*/
            RTD::DynamicControllerParameterReal                     m_loadFactor ;

			/** If true maneuver will be executed assuming a defensive position (i.e. 
                attacker is behind and approaching). Default value is false.*/
            RTD::DynamicControllerParameterBool                       m_isDefensive ;
        };

        Params   m_dynParams ;

        DisengEgressController( AirplaneBot* controlledBot , const std::string& name );

        virtual ~DisengEgressController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

    protected:
    };

}

#endif