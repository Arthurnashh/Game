#pragma once

#include "FixedWingLibCGF/FlightPathController.h"
#include "RTD/BotBase.h"

namespace FixedWingLibCGF
{
    class AirplaneBot;  

    /**		
		LoiterController implements Loiter maneuver. 
		
		Details of the controller can be found in the Programmer's Guide.
    */ 
    class RTDYNAMICS_EXPORT LoiterController : public RTD::BotControllerBase
    {
    public:

        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
        struct Params
        {
			RTD::DynamicControllerParameterVec2r  m_centerPointNM;
			RTD::DynamicControllerParameterReal   m_altitudeFeet;
			RTD::DynamicControllerParameterReal   m_radiusNM;            
            RTD::DynamicControllerParameterBool   m_clockWise;
			RTD::DynamicControllerParameterReal   m_speedKts;
			RTD::DynamicControllerParameterReal   m_desiredLoadFactor;
			RTD::DynamicControllerParameterReal   m_desiredRollRate;
			RTD::DynamicControllerParameterBool	  m_useAirBrakes;			
			RTD::DynamicControllerParameterReal   m_climbRateRateFtMin;
			RTD::DynamicControllerParameterBool	  m_enableTerrainFollowingFlight;
        };

        Params                  m_dynParams ;
        

        LoiterController( AirplaneBot* controlledBot , const std::string& name );

		virtual ~LoiterController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

	protected:
		RTD::CircularFlightPath m_circularPath ;
    };

}
