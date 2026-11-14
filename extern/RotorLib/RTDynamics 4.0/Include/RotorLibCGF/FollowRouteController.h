/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RotorLibCGF/Common.h"

namespace RotorLibCGF
{

class RotorcraftBot;

class RTDYNAMICS_EXPORT FollowRouteController :   public RTD::BotControllerBase
{
public:
    /** 
		Information about the waypoints
	*/
    struct WaypointInfo
    {
		/** Position of the waypoint in meters. */
        smVec2r position;   

		/** The cruise altitude during flight to this waypoint in meters. */
        smReal  altitude;  

		/** The cruise speed during flight to this waypoint in meters/second. */
        smReal  speed;     
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
		RTD::DynamicControllerParameterReal	    m_weaveFrequencyWeavePerMin;
		RTD::DynamicControllerParameterReal	    m_weaveAmplitude;
		RTD::DynamicControllerParameterBool		m_terrainFollowingEnabled;
		};

	Params   m_dynParams ;

    FollowRouteController( RotorcraftBot* controlledBot , const std::string& name );


    virtual ~FollowRouteController(void);


    virtual void    initialize();
    virtual void    update( float deltaTime );
    virtual void    finalize();

    unsigned int    getCurrentWaypointIndex() const;
    virtual void    setCurrentWaypointIndex( unsigned int wi );

    virtual bool    readParamStr( const std::string& parameterString , std::string& error );
protected:
    // Holds the index of the waypoint to which we are flying.
	RTD::StateVariableUInt    s_currentWaypoint;

};

}