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

namespace RTD
{

    class FlightPath
    {
    public:
        virtual ~FlightPath() { }

        /** Return the closest point to the given position on the path.  */
        virtual smVec3r getClosestPointOnPath(const smVec3r& acPos) = 0 ;

        /** The returned value is positive if the point is in the flight path.  */
        virtual smReal getPositionError(const smVec3r& acPos) = 0 ;

        /** Return the normalized tangent vector at the given position on the path.  */
        virtual smVec3r getTangentVectorOnPath(const smVec3r& acPos) = 0 ;

        virtual smVec3r getVectorFromPosToClosestPoint(const smVec3r& acPos) = 0 ;

        /** Returns positive value if the given position is inside of the path, negative value if
         *  outside. */
        virtual smReal  getCentricDirection(const smVec3r& acPos) = 0 ;
    };

    class CircularFlightPath : public FlightPath
    {
    public:
        smVec3r m_RC ;
        smReal  m_radius ;
        bool    m_clockWise ;

        virtual smVec3r getClosestPointOnPath(const smVec3r& RA)
        {
            smVec3r RCA = RA - m_RC ;
            smVec3r RCAn = RCA ;
            RCAn.normalize();

            smVec3r RT = m_RC + RCAn * m_radius;
            return RT;
        }

        virtual smReal getPositionError(const smVec3r& RA)
        {
            smVec3r RCA = RA - m_RC ;

            // Absolute radius error
            smReal ERadius = (RCA).length() - m_radius ;

            return ERadius;
        }

        virtual smVec3r getTangentVectorOnPath(const smVec3r& RA)
        {
            smVec3r RCA = RA - m_RC ;
            smVec3r RCAn = RCA ;
            RCAn.normalize() ;

            // Find Instantaneous tangent vector on circle
            smVec3r VT ;
            smMatrixBaser::Mat4 rotMat ;
            smMathMatrixR::makeRotMat( rotMat, 90.0f, smVec3r(0,0, m_clockWise ? -1.0f : +1.0f) );
            smMathMatrixR::transform3x3( rotMat, RCAn, VT );

            return VT ;
        }

        virtual smVec3r getVectorFromPosToClosestPoint(const smVec3r& RA)
        {
            smVec3r RT = getClosestPointOnPath(RA);

            smVec3r RAT = RT - RA ;

            return RAT;
        }

        virtual smReal getCentricDirection(const smVec3r& RA)
        {
            return getPositionError(RA)/m_radius ;
        }

    };
}

