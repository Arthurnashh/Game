#pragma once

#include "RTD/BotBase.h"

namespace RTD
{
    class PIDController;
}

namespace FixedWingLibCGF
{

	class AirplaneBot;    

    /**		
		BankLeftRightController implements a banked coordinated turn maneuver. 
		
		Details of the controller can be found in the Programmer's Guide.
    */ 
    class RTDYNAMICS_EXPORT BankLeftRightController : public RTD::BotControllerBase
    {
    public:

        /** This struct contains the dynamic parameters of this maneuver. 
		
			These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
		*/
        struct Params
        {
			RTD::DynamicControllerParameterBool		            m_bankToLeft;
            RTD::DynamicControllerParameterReal		            m_relativeAzimuthAngle;
            RTD::DynamicControllerParameterReal		            m_bankAngle;
			RTD::DynamicControllerParameterReal		            m_speedKts;
			RTD::DynamicControllerParameterBool					m_useAirBrakes;
			RTD::DynamicControllerParameterBool		            m_restart;
        };

        Params   m_dynParams ;


        BankLeftRightController( AirplaneBot* controlledBot , const std::string& name );

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );


    protected:
		smReal									m_targetHeading;
		RTD::DynamicControllerParameterReal	m_prevAngle;
		RTD::DynamicControllerParameterReal	m_prevRelativeAzimuthAngle;
		bool	                                m_useShortestTurnDirection;

		smReal									m_loadFactorForGivenBank;
    };

}
