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

namespace FixedWingLibCGF
{

class AirplaneBot;

/**		
	FollowRouteController is used to fly the airplane over user defined waypoints with user provided parameters.
	
	Details of the controller can be found in the Programmer's Guide.
*/ 
class RTDYNAMICS_EXPORT FollowRouteController : public RTD::BotControllerBase
{
public:
    // Information about the waypoints
    struct WaypointInfo
    {
        smVec2r positionNM;		// Position of the waypoint in nautical miles
        smReal  altitudeFeet;   // The cruise altitude during flight to this waypoint in feet.
        smReal  speedKts;		// The cruise speed during flight to this waypoint in knots.
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

	enum ContinuationMode
	{
		END_AND_LEVEL_FLIGHT = 0, 
		CONTINUE_FROM_FIRSTWAYPOINT,
		REVERSE_DIRECTION_AND_CONTINUE,
		NUM_OF_CONTINUATIONMODES
	};

	enum WaypointIndexDirection
	{
		INCREMENT_WAYPOINT_INDEX = 0,
		DECREMENT_WAYPOINT_INDEX,	
		NUM_WP_SWTICH_DIRECTION
	};

    /** This struct contains the dynamic parameters of this maneuver. 
	
		These parameters are explained in the FixedWingLib CGF section of the Programmer's Guide.
	*/
	struct Params
	{		
		DynamicControllerParameterWP			                    m_waypoints;
		RTD::DynamicControllerParameterReal		                    m_wpChangeCoefficient;
		RTD::DynamicControllerParameterReal		                    m_desiredLoadFactor;
		RTD::DynamicControllerParameterReal		                    m_desiredRollRate;
		RTD::DynamicControllerParameterBool		                    m_useAirBrakes;
		RTD::DynamicControllerParameterInt		                    m_startWPIndex;
		RTD::DynamicControllerParameterEnum<WaypointIndexDirection>    m_initialWPIndexDirection;
		RTD::DynamicControllerParameterBool		                    m_enableTerrainFollowingFlight;
		RTD::DynamicControllerParameterBool		                    m_avoidInvertedFlight;
		RTD::DynamicControllerParameterReal	                        m_weaveFrequencyWeavePerMin;
		RTD::DynamicControllerParameterReal	                        m_weaveAmplitude;
		RTD::DynamicControllerParameterEnum<ContinuationMode>	    m_continuationMode;
	};

	Params	m_dynParams;

    FollowRouteController( AirplaneBot* controlledBot , const std::string& name );
    virtual ~FollowRouteController(void);

    virtual void    initialize();
    virtual void    update( float deltaTime );
    virtual void    finalize();

    virtual WaypointIndexDirection     getCurrentIndexDirection() const;
    virtual int     getCurrentWaypointIndex() const;
    virtual void    setCurrentWaypointIndex( int wi );

    virtual bool    readParamStr( const std::string& parameterString , std::string& error );

	/** Returns the index of the closest waypoint to the given point.
	*	Returns -1 if there aren't any waypoints.
	*/
	static int		findClosestWaypoint( const Params& params, const smVec2r& point );
protected:
    // Holds the index of the waypoint to which we are flying.
    RTD::StateVariableInt     s_currentWaypoint;
    RTD::StateVariableUInt    s_currentWPIndexDirection;
};

}