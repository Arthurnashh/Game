/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:		RTDynamics
Subsystem:		VRFPlugin

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

namespace RTD
{

	template<class T>
	class Band
	{
	public:
		T m_begin;
		T m_end;
		bool m_valid;

		/** Constructs an invalid band */
		Band()
			: m_begin(0)
			, m_end(0)
			, m_valid(false)
		{
		}

		/** Constructs a band, its validity is checked and set.*/
		Band(float begin, float end)
			: m_begin(begin)
			, m_end(end)
		{
			// Band is only valid if begin < end.
			m_valid = (m_begin <= m_end);
		}

		T length()
		{
			return m_end - m_begin;
		}

		std::string toString()
		{
			return std::string() + "[" + smToString(m_begin) + "," + smToString(m_end) + "]" ;
		}

		/** Returns the intersection with the given band.
		*/
		Band intersect( const Band& other )
		{
			Band ol;

			// Other band to the left of this one.
			if ( m_begin > other.m_end ) return ol;
			// Other band to the right of this one.
			if ( m_end < other.m_begin ) return ol;

			float begin = smMax( m_begin, other.m_begin );
			float end   = smMin( m_end, other.m_end );

			return Band(begin, end);
		}

		static void runUnitTests()
		{
			Band a, b;

			a = Band( 3, 5 ); b = Band( -2, 2 );
			fprintf(stderr, "Intersection must be zero(invalid): %d\n", a.intersect(b).m_valid );

			a = Band( 3, 5 ); b = Band( 12, 332 );
			fprintf(stderr, "Intersection must be zero(invalid): %d\n", a.intersect(b).m_valid );

			a = Band( 3, 5 ); b = Band( 1, 4 );
			fprintf(stderr, "Intersection must be (valid): %d [%.2f,%.2f]\n", a.intersect(b).m_valid, a.intersect(b).m_begin, a.intersect(b).m_end );

			a = Band( -3, 5 ); b = Band( 1, 4 );
			fprintf(stderr, "Intersection must be (valid): %d [%.2f,%.2f]\n", a.intersect(b).m_valid, a.intersect(b).m_begin, a.intersect(b).m_end );

			a = Band( -3, 5 ); b = Band( 1, 4 );
			fprintf(stderr, "Intersection must be (valid): %d [%.2f,%.2f]\n", b.intersect(a).m_valid, b.intersect(a).m_begin, b.intersect(a).m_end );
		}
	};
	typedef Band<float>		BandF;
	typedef Band<double>	BandD;

}