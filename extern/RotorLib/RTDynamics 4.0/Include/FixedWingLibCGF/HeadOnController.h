#pragma once

#include "RTD/ReferenceEntity.h"
#ifdef RTDYNAMICS_EXPORT_MILITARY_FEATURES

namespace FixedWingLibCGF
{
    class AirplaneBot;

    /**		
		HeadOnController implements Head On maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT HeadOnController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
            FRONTAL_APPROACH,
            LEVEL_FLIGHT_AFTER_FLYPAST,
            LEVEL_FLIGHT_DUE_TO_EXIT_CONDITIONS
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        class Params
        {
        public:
            /** Enemy aircraft which will be taken as reference during this
            *   maneuver. If the parameter is not specified or specified
            *   aircraft becomes inactive controller will perform level
            *   flight.
            */
            RTD::DynamicControllerParameter<RTD::ReferenceEntityBase*> m_target;

            /** Inner radius of the fly-past region in feet. Default value is
             *  1000 feet.
             */
            RTD::DynamicControllerParameterReal m_flyPastRegionInnerRadiusFeet ;

            /** Width of the fly-past region in feet.
            *       \e m_flyPastRegionInnerRadiusFeet +  m_flyPastRegionWidthFeet
            *   gives the outer radius.
            *  
            *   Default value is 100 feet.
            */
            RTD::DynamicControllerParameterReal m_flyPastRegionWidthFeet ;

			/** Height of the fly-past region in feet. Default value is 100 feet. */
            RTD::DynamicControllerParameterReal m_flyPastRegionHeightFeet ;

            /** Load factor to be used during this maneuver. Default value is the
             *  maximum load factor of the aircraft.
             */
            RTD::DynamicControllerParameterReal m_loadFactor ;

            /** Speed during the maneuver in Knots. This parameter does not have
             *  a default. If it is not specified, than current speed will be
             *  kept.
             */
            RTD::DynamicControllerParameterReal m_speedKts ;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool	m_useAirBrakes;
        };

        Params   m_dynParams ;

        HeadOnController( AirplaneBot* controlledBot , const std::string& name );

        virtual ~HeadOnController();

        virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

        /** Returns the direction vector to the selected
         *  fly-past-region. */
        const smVec3r& getDirectionVector() { return m_direction ; }

    protected:
        void levelFlight(float deltaTime);
        void frontalApproach(float deltaTime);

        unsigned int    m_firstDataLoggerChannelIndex ;
        smVec3r         m_direction ;
        smReal          m_distanceToTargetFeet ;
        bool            m_leftRegionSelected ;
        smReal          m_distanceToRight ;
        smReal          m_distanceToLeft ;

        bool            m_lastLongHemisphere;
        State           m_state ;
        smReal          m_ETA ;
        smVec3r         m_Rflypastregioncenter_WCS;
    };

}
#endif