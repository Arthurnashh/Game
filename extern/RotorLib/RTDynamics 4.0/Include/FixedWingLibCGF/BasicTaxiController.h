#pragma once

#include "RotorLibCGF/Common.h"
#include "RTD/ParamDb.h"


namespace FixedWingLibCGF
{
	class AirplaneBot;

	class RTDYNAMICS_EXPORT BasicTaxiController :	public RTD::BotControllerBase
	{
	public:
		struct Params
		{
			RTD::DynamicControllerParameterReal    m_desiredDirection;
			RTD::DynamicControllerParameterReal    m_desiredSpeedKts;
		};
		Params	m_dynParams;

		BasicTaxiController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~BasicTaxiController();

		virtual void	initialize();
		virtual void	update( float deltaTime );
		virtual void	finalize();
		virtual void	reset();
	};

	class RTDYNAMICS_EXPORT TaxiPositionController :	public RTD::BotControllerBase
	{
	public:


		struct Params
		{
			RTD::DynamicControllerParameterVec2r		m_desiredPositionNM;
			RTD::DynamicControllerParameterReal			m_desiredSpeedKts;
			RTD::DynamicControllerParameterBool			m_stopOnPosition; // If enabled the vehicle will stop when it arrives the desired position.
		};

        Params	m_dynParams;

        TaxiPositionController( AirplaneBot* controlledBot , const std::string& name );

        virtual void	update( float deltaTime );

    protected:
		

		smVec3r m_HPR;		//  Current orientation
		smVec3r	m_V;		//	Current velocity
		smVec3r	m_Vgr;		//	Current ground velocity

		smVec3r	m_XYZ;		//  Current position

		smVec3r	m_D;		//	Displacement vector ( m_desiredPosition - m_XYZ )
		smVec3r	m_Dgr;		//	Ground displacement vector;

		smVec3r	m_FW;		//  Forward vector
		smVec3r	m_SW;		//  Sideward vector

		smVec3r	m_FWgr;		//  Ground forward vector
		smVec3r	m_SWgr;		//  Ground sideward vector

		smReal	m_DOM;		//	Direction of motion ( heading of m_Vgr vector )

		virtual ~TaxiPositionController();

		virtual void	initialize();		
		virtual void	finalize();
		virtual void	reset();

		void updateVehicleStatus();
	};

	class RTDYNAMICS_EXPORT TaxiRouteController :	public RTD::BotControllerBase
	{
	public:
        struct WaypointInfo
        {
            smVec2r positionNM;		// Position of the waypoint in nautical miles
            smReal  speedKts;		// The cruise speed during flight to this waypoint in knots.
        };

        enum WaypointIndexDirection
        {
	        INCREMENT_WAYPOINT_INDEX = 0,
	        DECREMENT_WAYPOINT_INDEX,	
	        NUM_WP_SWTICH_DIRECTION
        };

	    class RTDYNAMICS_EXPORT DynamicControllerParameterWP : public  RTD::DynamicControllerParameter<std::vector<WaypointInfo> > 
	    {
	    public:

    		
		    DYNAMIC_CONTROLLER_PARAM_DECL_WITH_COPY( DynamicControllerParameterWP , std::vector<WaypointInfo> , GENERIC , RTD::DynamicControllerParameter<std::vector<WaypointInfo> > );

		    virtual std::string		toString() const;
		    virtual bool			fromString( const std::string& str );		
		    virtual std::vector<unsigned char>		toBinary() const;
		    virtual bool							fromBinary( const std::vector<unsigned char>& data );

	    };


		struct Params 
		{
            DynamicControllerParameterWP			m_waypoints;
		    RTD::DynamicControllerParameterReal		m_wpChangeCoefficient;
		    RTD::DynamicControllerParameterInt	    m_startWPIndex; 
            RTD::DynamicControllerParameterEnum<WaypointIndexDirection>    m_initialWPIndexDirection; 
			RTD::DynamicControllerParameterReal		m_pathAlignmentCoefficient;
		};
		
		Params m_dynParams;

		TaxiRouteController( AirplaneBot* controlledBot , const std::string& name );
		virtual ~TaxiRouteController();

		virtual void initialize();
		virtual void update( float deltaTime );
		virtual void finalize();
		virtual void reset();

	    virtual int	 getCurrentWaypointIndex() const;
		virtual void setCurrentWaypointIndex( int wi );
		virtual WaypointIndexDirection  getCurrentIndexDirection() const;

    protected:
  		RTD::StateVariableInt	s_currentWaypoint;
		RTD::StateVariableUInt  s_currentIndexDirection;

	};
}
