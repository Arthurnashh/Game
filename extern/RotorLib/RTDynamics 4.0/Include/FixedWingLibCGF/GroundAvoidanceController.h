

#pragma once

#include "RTD/BotBase.h"

#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES
namespace FixedWingLibCGF
{
    class AirplaneBot;


    /** GroundAvoidanceController implements a basic ground avoidance algorithm. Details of the 
		controller can be found in the Programmer's Guide.

		This Controller is available only if Air Combat Maneuvers or Ground Attack Maneuvers features are enabled.     
     **/ 
    class RTDYNAMICS_EXPORT GroundAvoidanceController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        class Params
        {
        public:
            /** This is the minimum height above terrain ( aircraft altitude -
            *   terrain height ) which is acceptable. If the airplane goes below
            *   this ground avoidance will be executed. Otherwise aircraft
            *   will fly level. */
            RTD::DynamicControllerParameterReal  m_levelOffHeightFeet ;

            /** Load factor to be used during this maneuver. Note that small
            *   load factors might result in too slow pitching. As a result it
            *   might not be possible to avoid the terrain in time.
		    *
            *   Default value of this parameter is the maximum load factor of
            *    the aircraft.*/
            RTD::DynamicControllerParameterReal  m_loadFactor ;

            /** Speed during the maneuver in Knots. This parameter does not
             *  have a default. If it is not specified, current speed wil lbe
             *  kept.*/
            RTD::DynamicControllerParameterReal  m_speedKts ;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool	 m_useAirBrakes;
        };

        Params   m_dynParams ;

        GroundAvoidanceController( AirplaneBot* controlledBot , const std::string& name );

        virtual ~GroundAvoidanceController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

    private:
        smReal          m_minimumAltitudeInFeet;
		smReal			m_hysteresis;
        unsigned int    m_firstDataLoggerChannelIndex;
    };
}

#endif