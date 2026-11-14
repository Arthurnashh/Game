#pragma once

#include "RTD/BotBase.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace RTD
{
    class PIDController;
}

namespace FixedWingLibCGF
{

    class AirplaneBot;

    /**		
		ExtensionController implements Extension maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT ExtensionController : public RTD::BotControllerBase
    {
    public:
        ExtensionController( AirplaneBot* controlledBot , const std::string& name );

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );
    };

}

#endif