#include ".\CG_Diagonal.h"

// -------------------------------------------------------------------------- //

CG_Diagonal::CG_Diagonal()
: CG_Segment2()
{
}

// -------------------------------------------------------------------------- //

CG_Diagonal::CG_Diagonal(const CG_Point2& from, const CG_Point2& to, bool fromConvex,
						 bool toConvex, bool essential)
: CG_Segment2(from, to)
, m_FromConvex(fromConvex)
, m_ToConvex(toConvex)
, m_Essential(essential)
{
}

// -------------------------------------------------------------------------- //

CG_Diagonal::CG_Diagonal(const CG_Diagonal& other)
: CG_Segment2(other.From(), other.To())
, m_FromConvex(other.m_FromConvex)
, m_ToConvex(other.m_ToConvex)
, m_Essential(other.m_Essential)
{
}

// -------------------------------------------------------------------------- //

CG_Diagonal::~CG_Diagonal()
{
}

// -------------------------------------------------------------------------- //

bool CG_Diagonal::FromConvex() const
{
	return m_FromConvex;
}

// -------------------------------------------------------------------------- //

bool CG_Diagonal::ToConvex() const
{
	return m_ToConvex;
}

// -------------------------------------------------------------------------- //

void CG_Diagonal::FromConvex(bool fromConvex)
{
	m_FromConvex = fromConvex;
}

// -------------------------------------------------------------------------- //

void CG_Diagonal::ToConvex(bool toConvex)
{
	m_ToConvex = toConvex;
}

// -------------------------------------------------------------------------- //

bool CG_Diagonal::Essential() const
{
	return m_Essential;
}

// -------------------------------------------------------------------------- //

void CG_Diagonal::Essential(bool essential)
{
	m_Essential = essential;
}

// -------------------------------------------------------------------------- //
