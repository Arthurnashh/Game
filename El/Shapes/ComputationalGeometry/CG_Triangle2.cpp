#include ".\CG_Triangle2.h"

// -------------------------------------------------------------------------- //

CG_Triangle2::CG_Triangle2()
{
}

// -------------------------------------------------------------------------- //

CG_Triangle2::CG_Triangle2(const CG_Point2& v1, const CG_Point2& v2, const CG_Point2& v3)
{
	m_Vertices[0] = v1;
	m_Vertices[1] = v2;
	m_Vertices[2] = v3;
}

// -------------------------------------------------------------------------- //

CG_Triangle2::CG_Triangle2(const CG_Point2 vertices[3])
{
	m_Vertices[0] = vertices[0];
	m_Vertices[1] = vertices[1];
	m_Vertices[2] = vertices[2];
}

// -------------------------------------------------------------------------- //

const CG_Point2& CG_Triangle2::Vertex(unsigned int index) const
{
	return m_Vertices[index];
}

// -------------------------------------------------------------------------- //

double CG_Triangle2::SignedTwiceArea() const
{
	const CG_Point2& a = m_Vertices[0];
	const CG_Point2& b = m_Vertices[1];
	const CG_Point2& c = m_Vertices[2];
	return ((b.X() - a.X()) * (c.Y() - a.Y()) - (c.X() - a.X()) * (b.Y() - a.Y()));
}

// -------------------------------------------------------------------------- //

double CG_Triangle2::SignedArea() const
{
	return (0.5 * SignedTwiceArea());
}

// -------------------------------------------------------------------------- //
