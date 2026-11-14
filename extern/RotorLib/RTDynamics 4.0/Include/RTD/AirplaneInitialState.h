/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/RigidBody.h"
#include "RTD/RigidBodyState.h"
#include "FixedWingLibCGF/SimplifiedAirplane.h"

namespace RTD
{
    /** AirplaneInitialState is a utility class which makes it easier to set the
     *  initial state (position,speed,attitudes,altitude) of an airplane. The
     *  initial state of an airplane can be specified in aviation unit system.
     *  
     *  The apply method must be called to apply the initial state contained in
     *  this class to the airplane.
     **/
    class AirplaneInitialState
    {
    public:
        smVec2r pos;        // initial position (x,y) [m,m]
        smReal  altitude;   // initial altitude (z) [ft]
        smVec3r hpr;        // initial heading,pitch,roll [deg,deg,deg]

        smVec3r vBCS;       // initial in body coordinates speed [knots,knots,knots]
        smVec3r vWCS;       // initial in world coordinates speed [knots,knots,knots]
        bool    vInBCS ;    // if true, a/c is initialized with BCS velocity.

        AirplaneInitialState()
        {
            pos.set(0,0);
            altitude = 1000.0f ;
            hpr.set(0,0,0);
            vBCS.set(0,0,0);
            vWCS.set(0,0,0);
            vInBCS = true ;
        }

        void apply( FixedWingLibCGF::SimplifiedAirplane* airplane )
        {
            airplane->reset() ;
            airplane->setGearStatus( 0.0f );
			airplane->getPilotInputData()->m_extendGears = false;

            airplane->setPosition( smPositionVec( pos[0], pos[1], altitude * RTD::Units::feet_to_meters ) );

            airplane->setRotation( hpr ) ;

            if ( vInBCS )
            {
                smSpeedVec vInWCS;
                airplane->getState().bcs_to_wcs( vBCS * RTD::Units::knots_to_mps, vInWCS );
                airplane->setVelocity( vInWCS );
            }
            else
            {
                airplane->setVelocity( vWCS * RTD::Units::knots_to_mps );
            }
        }
    };
}