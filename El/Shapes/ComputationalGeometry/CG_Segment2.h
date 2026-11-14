#ifndef CG_SEGMENT2_H
#define CG_SEGMENT2_H

// -------------------------------------------------------------------------- //

#include ".\CG_Point2.h"

// -------------------------------------------------------------------------- //

class CG_Segment2
{
	CG_Point2 m_From;
	CG_Point2 m_To;

public:

	CG_Segment2();
	CG_Segment2(const CG_Point2& from, const CG_Point2& to);

	CG_Segment2(const CG_Segment2& other);

	virtual ~CG_Segment2();

	const CG_Point2& From() const;
	CG_Point2& From();

	const CG_Point2& To() const;
	CG_Point2& To();

	void From(const CG_Point2& from);
	void To(const CG_Point2& to);

	double SquaredLength() const;
	double Length() const;

	// returns the segment obtained by this with the endpoints swapped
	CG_Segment2 Inverted() const;

	// returns true if the given point lies on this segment (endpoints are comprised)
	bool Contains(const CG_Point2& p, double tolerance) const;

	// returns true if the given point is strictly in the semiplane at
	// the left of the line containing this segment for an observer placed
	// in m_From and looking at m_To
	bool IsLeft(const CG_Point2& p, double tolerance) const;

	// returns true if the given point is in the semiplane at
	// the left of the line containing this segment for an observer placed
	// in m_From and looking at m_To or if it lies on the line
	bool IsLeftOn(const CG_Point2& p, double tolerance) const;

	// returns true if the given point lies on the line containing this segment 
	bool IsCollinear(const CG_Point2& p, double tolerance) const;

	// returns true if this segment and the given one share a point interior
	// to both (if one endpoint of one of the segments lies on the other segment
	// the result is false).
	bool IntersectsProperly(const CG_Segment2& other, double tolerance) const;

	// returns true if this segment and the given one intersect
	// (if one endpoint of one of the segments lies on the other segment
	// the result is true).
	bool Intersects(const CG_Segment2& other, double tolerance) const;
};

// -------------------------------------------------------------------------- //

#endif