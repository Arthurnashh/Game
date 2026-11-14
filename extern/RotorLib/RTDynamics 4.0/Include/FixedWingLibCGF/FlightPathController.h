/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RotorLibCGF/Common.h"
#include "RTD/Math.h"
#include "RTD/FlightPath.h"

namespace FixedWingLibCGF
{
    class AirplaneBot;

	class RTDYNAMICS_EXPORT FlightPathController : public RTD::BotControllerBase
    {
    public:
		class RTDYNAMICS_EXPORT DynamicControllerParameterFlightPath : public  RTD::DynamicControllerParameter<RTD::FlightPath*> 
		{
		public:

			
			DYNAMIC_CONTROLLER_PARAM_DECL_WITH_COPY( DynamicControllerParameterFlightPath , RTD::FlightPath* , GENERIC , RTD::DynamicControllerParameter<RTD::FlightPath*> );

			virtual std::string						toString() const;
			virtual std::vector<unsigned char>		toBinary() const;
			virtual bool							fromBinary( const std::vector<unsigned char>& data );

		};

        FlightPathController( AirplaneBot* controlledBot , const std::string& name );
        virtual ~FlightPathController();

        struct Params
        {
            RTD::DynamicControllerParameterReal			m_desiredSpeedKts;
			RTD::DynamicControllerParameterBool			m_useAirBrakes;
            RTD::DynamicControllerParameterReal			m_desiredAltitudeFeet;
			RTD::DynamicControllerParameterReal			m_desiredLoadFactor;
			RTD::DynamicControllerParameterReal			m_desiredRollRate;
			RTD::DynamicControllerParameterReal			m_climbRateRateFtMin;
			RTD::DynamicControllerParameterBool			m_enableTerrainFollowingFlight;
            DynamicControllerParameterFlightPath		m_path ;
        };

        Params                  m_dynParams;
        RTD::RealDataHistory    m_centricPosErrorHist ;
        unsigned int		    m_firstDataLoggerChannelIndex;

        virtual void    initialize();
        virtual void    update( float deltaTime );
        virtual void    finalize();

        bool isCentricPosErrStableAndSmallEnough(smReal threadhold);

    };
}

