#ifndef CG_TRIANGLE2_H
#define CG_TRIANGLE2_H

// -------------------------------------------------------------------------- //

#include ".\CG_Point2.h"

// -------------------------------------------------------------------------- //

class CG_Triangle2
{
	CG_Point2 m_Vertices[3];

public:
	CG_Triangle2();
	CG_Triangle2(const CG_Point2& v1, const CG_Point2& v2, const CG_Point2& v3);
	CG_Triangle2(const CG_Point2 vertices[3]);

	const CG_Point2& Vertex(unsigned int index) const;

	double SignedTwiceArea() const;
	double SignedArea() const;
};

// -------------------------------------------------------------------------- //

#endif