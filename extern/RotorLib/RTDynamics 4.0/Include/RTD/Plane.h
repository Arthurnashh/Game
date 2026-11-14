/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:		RTDynamics
Subsystem:		RTDynamics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/LineSegment.h"

namespace RTD
{
	template <class VecType>
	class Plane
	{
		VecType m_v0, m_n;
	public:
		Plane(VecType v0, VecType n)
			: m_v0(v0), m_n(n)
		{
			if ( m_n.length() == 0 )
				throw smExpSingularity("Normal vector has zero length in Plane::Plane()");

			// Make sure the normal vector has non zero length or length larger than one.
			if ( m_n.length() > 1.0f )
			{
				m_n.normalize();
			}
		}

		std::string toString()
		{
			return std::string("String V0=") + smToString( m_v0 ) + " N=" + smToString(m_n);
		}

		bool intersect( const LineSegment<VecType>& ls, bool finiteSegment, VecType& intersectionPoint )
		{
			// http://softsurfer.com/Archive/algorithm_0104/algorithm_0104B.htm

			// We first check if L is parallel to p by testing if n·u=0 which means that the line direction vector u is perpendicular to the plane normal n.
			double dotProduct = m_n.dot( ls.getU() );
			if ( smMath::almostEqual( dotProduct, 0.0 ) )
				return false;

			// Disjointness or coincidence can be determined by testing whether any specific point of L, say P0, is contained in p, 
			// that is whether it satisfies the implicit line equation: n·(P-V0) = 0.
			dotProduct = m_n.dot( ls.getP0() - m_v0 );
			if ( smMath::almostEqual( dotProduct, 0.0 ) ) { intersectionPoint = ls.getP0() ; return true; }
			dotProduct = m_n.dot( ls.getP1() - m_v0 );
			if ( smMath::almostEqual( dotProduct, 0.0 ) ) { intersectionPoint = ls.getP1() ; return true; }

			// If the line and plane are not parallel, then L and p intersect in a unique point P(sI)
			// sI = n dot (V0 - P0) / n dot (P1 - P0) 
			double s_i = m_n.dot( m_v0 - ls.getP0() ) / m_n.dot( ls.getP1() - ls.getP0() );

			// If the line L is a finite segment from P0 to P1, then one just has to check that 
			// 0<=sI<=1 to verify that there is an intersection between the segment and the plane.
			if ( finiteSegment )
				if ( s_i < 0.0f || s_i > 1.0f ) 
					return false;

			intersectionPoint = ls.getPointOnLine( s_i );
			return true;
		}

		static void unitTest1()
		{
			Plane<VecType> p ( VecType(0,0,0), VecType(0,0,1) );

			VecType isectPoint;
			bool result;
			
			result = p.intersect( LineSegment<VecType>( VecType(0,0,10), VecType(0,0,1) ), true, isectPoint );
			RTD::validateAndWarn( result == false, "test 1", "Plane unitTest1");
			
			result = p.intersect( LineSegment<VecType>( VecType(0,0,-10), VecType(0,0,-1) ), true, isectPoint );
			RTD::validateAndWarn( result == false, "test 2", "Plane unitTest1");

			result = p.intersect( LineSegment<VecType>( VecType(0,0,10), VecType(0,0,-1) ), true, isectPoint );
			RTD::validateAndWarn( result == true, "test 3", "Plane unitTest1");
			RTD::validateAndWarn( smMath::almostEqual(isectPoint, smVec3d(0,0,0), smMath::sZeroTolD ), "test 3", "Plane unitTest1");

			result = p.intersect( LineSegment<VecType>( VecType(20,20,20), VecType(0,0,-20) ), true, isectPoint );
			RTD::validateAndWarn( result == true, "test 4", "Plane unitTest1");
			RTD::validateAndWarn( smMath::almostEqual(isectPoint, smVec3d(10,10,0), smMath::sZeroTolD ), "test 4", "Plane unitTest1");
		}

		VecType getNormal() { return m_n; }
	};

	typedef Plane<smVec3r>	Plane3r;
	typedef Plane<smVec3d>	Plane3d;
}