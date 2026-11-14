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
		SplitSController implements Split S Maneuver.Details of the execution of 
		the maneuver can be found in the Programmer's Guide.

		Note that this maneuver is available only if Air Combat Maneuvers feature is activated.
	*/    
    class RTDYNAMICS_EXPORT SplitSController : public RTD::BotControllerBase
    {
    public:
        enum State
        {
            INVERTED_FLIGHT = 0,
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
            /** Load factor during the first quarter loop (from initial inverted
             *  flight with 0 degree pitch attitude to 90 degree nose down
             *  pitch.) 
			 *
			 *  Default value is the maximum load factor of the aircraft.
			 */
            RTD::DynamicControllerParameterReal   m_desiredLoadFactor1;

            /** Load factor during the second quarter loop (from 90 degree nose
             *  down pitch to 0 degree pitch attitude with leveled wings.) 
			 *
			 *	If no value is specified m_desiredLoadFactor2 is taken equal to
			 *  m_desiredLoadFactor1.
			 */
			RTD::DynamicControllerParameterReal   m_desiredLoadFactor2;

            /** When the aircraft reaches 90 degree nose down pitch, it will
            *   rotate to point its lift vector to \e m_desiredExitDirection
            *   direction. Then second quarter loop will be executed and
            *   airplane will exit the maneuver with the nose heading equal
            *   to \e m_desiredExitDirection. 
			*
            *   This parameter does not have a default. If it is not specified,
            *   then exit direction is the opposite direction of the initial
            *   direction. E.g. if the maneuver airplane heading was 50 degrees
            *   before, the maneuver it will be 180+50 = 230 degrees afterwards.
			*/
			RTD::DynamicControllerParameterReal   m_desiredExitDirection;
        };

        Params   m_dynParams ;

        SplitSController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~SplitSController();
        
		virtual void initialize();

        virtual void finalize();

        virtual void update( float deltaTime );

		State	        getCurrentState() const	{	return m_state;	}
		virtual void	invertedFlight(smTime deltaTime);
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