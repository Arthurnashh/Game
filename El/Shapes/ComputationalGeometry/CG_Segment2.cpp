// -------------------------------------------------------------------------- //

#include <math.h>

// -------------------------------------------------------------------------- //

#include ".\CG_Segment2.h"
#include ".\CG_Triangle2.h"

// -------------------------------------------------------------------------- //

CG_Segment2::CG_Segment2()
{
}

// -------------------------------------------------------------------------- //

CG_Segment2::CG_Segment2(const CG_Point2& from, const CG_Point2& to)
: m_From(from)
, m_To(to)
{
}

// -------------------------------------------------------------------------- //

CG_Segment2::CG_Segment2(const CG_Segment2& other)
: m_From(other.m_From)
, m_To(other.m_To)
{
}

// -------------------------------------------------------------------------- //

CG_Segment2::~CG_Segment2()
{
}

// -------------------------------------------------------------------------- //

const CG_Point2& CG_Segment2::From() const
{
	return m_From;
}

// -------------------------------------------------------------------------- //

CG_Point2& CG_Segment2::From() 
{
	return m_From;
}

// -------------------------------------------------------------------------- //

const CG_Point2& CG_Segment2::To() const
{
	return m_To;
}

// -------------------------------------------------------------------------- //

CG_Point2& CG_Segment2::To()
{
	return m_To;
}

// -------------------------------------------------------------------------- //

void CG_Segment2::From(const CG_Point2& from)
{
	m_From = from;
}

// -------------------------------------------------------------------------- //

void CG_Segment2::To(const CG_Point2& to)
{
	m_To = to;
}

// -------------------------------------------------------------------------- //

double CG_Segment2::SquaredLength() const
{
	double dx = m_To.X() - m_From.X();
	double dy = m_To.Y() - m_From.Y();

	return (dx * dx + dy * dy);
}

// -------------------------------------------------------------------------- //

double CG_Segment2::Length() const
{
	return sqrt(SquaredLength());
}

// -------------------------------------------------------------------------- //

CG_Segment2 CG_Segment2::Inverted() const
{
	return CG_Segment2(m_To, m_From);
}

// -------------------------------------------------------------------------- //

bool CG_Segment2::Contains(const CG_Point2& p, double tolerance) const
{
	if (!IsCollinear(p, tolerance)) return false;

	if (m_From.X() != m_To.X())
	{
		return (((m_From.X() <= p.X()) && (p.X() <= m_To.X())) ||
				((m_From.X() >= p.X()) && (p.X() >= m_To.X())));
	}
	else
	{
		return (((m_From.Y() <= p.Y()) && (p.Y() <= m_To.Y())) ||
				((m_From.Y() >= p.Y()) && (p.Y() >= m_To.Y())));
	}
}

// -------------------------------------------------------------------------- //

bool CG_Segment2::IsLeft(const CG_Point2& p, double tolerance) const
{
	CG_Triangle2 t(m_From, m_To, p);

	return (t.SignedTwiceArea() > tolerance);
}

// -------------------------------------------------------------------------- //

bool CG_Segment2::IsLeftOn(const CG_Point2& p, double tolerance) const
{
	return (IsLeft(p, tolerance) || IsCollinear(p, tolerance));
}

// -------------------------------------------------------------------------- //

bool CG_Segment2::IsCollinear(const CG_Point2& p, double tolerance) const
{
	CG_Triangle2 t(m_From, m_To, p);

	return (fabs(t.SignedTwiceArea()) <= tolerance);
}

// -------------------------------------------------------------------------- //

bool CG_Segment2::IntersectsProperly(const CG_Segment2& other, double tolerance) const
{
	if (
		IsCollinear(other.m_From, tolerance) ||
		IsCollinear(other.m_To, tolerance)   ||
		other.IsCollinear(m_From, tolerance) ||
		other.IsCollinear(m_To, tolerance)
	   )
	{
		return false;
	}

	return ((IsLeft(other.m_From, tolerance) ^ IsLeft(other.m_To, tolerance)) &&
			(other.IsLeft(m_From, tolerance) ^ other.IsLeft(m_To, tolerance)));
}

// -------------------------------------------------------------------------- //

bool CG_Segment2::Intersects(const CG_Segment2& other, double tolerance) const
{
	if (IntersectsProperly(other, tolerance))
	{
		return true;
	}
	else if (
			 Contains(other.m_From, tolerance) ||
			 Contains(other.m_To, tolerance) ||
			 other.Contains(m_From, tolerance) ||
			 other.Contains(m_To, tolerance)
			)
	{
		return true;
	}
	else
	{
		return false;
	}
}

// -------------------------------------------------------------------------- //
