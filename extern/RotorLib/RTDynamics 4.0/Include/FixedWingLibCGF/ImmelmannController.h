#pragma once

#include "FixedWingLibCGF/BasicLowLevelController.h"
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
		ImmelmannController implements Immelmann maneuver. Details of the execution of the maneuver 
		can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
    */ 
    class RTDYNAMICS_EXPORT ImmelmannController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
            LEVEL_WINGS = 0,
				FIRST_QUARTER_LOOP,
                ROLL_TO_EXITDIR,
				SECOND_QUARTER_LOOP,
                LEVEL_WINGS_EXIT,
                LEVEL_FLIGHT
        };

        /** This struct contains the dynamic parameters of this
         *  maneuver. */
        struct Params
        {
            /** Load that will be pulled during the first quarter loop (from
            *   level to 90 up pitch).
            * 
            *   Default value of this parameter is the maximum load factor of 
            *   the aircraft.*/ 
            RTD::DynamicControllerParameterReal   m_desiredLoadFactor1;

            /** Load that will be pulled during the second quarter loop (from
            *   90 up pitch attitude to 0 degree pitch, 180 roll attitude).
            *  
            *   This parameter does not have a default. If it is not specified
            *   it is assumed to be equal to m_desiredLoadFactor1.
            */
			RTD::DynamicControllerParameterReal   m_desiredLoadFactor2;

            /** When aircraft reaches 90 degree pitch up attitude it will rotate
            *   to point its lift vector towards \e m_desiredExitDirection.
            *   After that second quarter loop will start and when it finishes,
            *   aircraft's nose heading will be pointing this direction.
            *  
            *   This parameter does not have a default. If it is not specified
            *   then exit direction is the opposite direction of the initial
            *   direction. E.g. if before the maneuver airplane heading was 50
            *   degrees, after the maneuver it will be 180+50 = 230 degrees.
			*/
			RTD::DynamicControllerParameterReal   m_desiredExitDirection;
        };

        Params   m_dynParams ;


        ImmelmannController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~ImmelmannController();
        
		virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

		State	getCurrentState() const	{	return m_state;	}

		virtual void	levelWings(smTime deltaTime);
		virtual void	firstQuarterLoop(smTime deltaTime);
		virtual void	rollToExitDirection(smTime deltaTime);
		virtual void	secondQuarterLoop(smTime deltaTime);
		virtual void	levelWingsExit(smTime deltaTime);
		virtual void	levelFlight(smTime deltaTime);
    protected:
        State   m_state;
        BasicLowLevelController::Params  m_defaultParams;

		RTD::PIDController*	mpid_roll;

    };

}

#endif