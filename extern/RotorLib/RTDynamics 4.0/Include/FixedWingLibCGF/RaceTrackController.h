#pragma once

#include "RTD/BotBase.h"


namespace FixedWingLibCGF
{
	class AirplaneBot;

    /**		
		RaceTrackController implements race track flying pattern.Details of the execution of 
		the maneuver can be found in the Programmer's Guide.
	*/    
    class RTDYNAMICS_EXPORT RaceTrackController : public RTD::BotControllerBase
    {
    public:
        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
			/** Starting point of the race track path. In nautical miles. */
            RTD::DynamicControllerParameterVec2r    m_startPointNM ;

            /** Direction of the race track path in degrees. RTD coordinate system
            *   conventions are used in defining the direction. I.e. north is
            *   zero, positive values denote clockwise rotation (when viewed
            *   from above). 
			*
			*	Default value is zero. */
            RTD::DynamicControllerParameterReal     m_direction ;

            /** Length of the track in nautical miles. */
            RTD::DynamicControllerParameterReal     m_lengthNMiles ;

			/** Flight speed in knots. Default value is 350 Kts. */
            RTD::DynamicControllerParameterReal     m_speedKts ;

            /** When set to true, enables to use of air brakes for
             *  deceleration. Default value is true. */
			RTD::DynamicControllerParameterBool      m_useAirBrakes;

            /** Flight altitude in feet. If no altitude is specified, curent
             *  altitude is kept. */
            RTD::DynamicControllerParameterReal     m_altitudeFeet ;

            /** Load factor to be used during this maneuver. Note that too small
             *  load factors can prevent that the race track is tightly
             *  followed.
			 *
			 *  Default value is 2.0g.
			 */
            RTD::DynamicControllerParameterReal     m_loadFactor ;
        };

        Params   m_dynParams ;        

        RaceTrackController( FixedWingLibCGF::AirplaneBot* controlledBot , const std::string& name ) ;
		virtual ~RaceTrackController();

        virtual void initialize() ;

        virtual void finalize() {} ;

        virtual void update( float deltaTime ) ;

        
	protected:
		Params   m_lastDynParams ;

		void rebuildRoute() ;
    };

}
