#include ".\CG_Polygon2Vertex.h"

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex::CG_Polygon2Vertex()
{
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex::CG_Polygon2Vertex(double x, double y, unsigned int id, bool ear, 
									 bool convex, CG_Polygon2Vertex* prev, 
									 CG_Polygon2Vertex* next)
: CG_Vertex2(x, y, id)
, m_Ear(ear)
, m_Convex(convex)
{
	m_Prev = prev;
	m_Next = next;
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex::CG_Polygon2Vertex(const CG_Point2& p, unsigned int id, bool ear, 
									 bool convex, CG_Polygon2Vertex* prev, 
									 CG_Polygon2Vertex* next)
: CG_Vertex2(p, id)
, m_Ear(ear)
, m_Convex(convex)
{
	m_Prev = prev;
	m_Next = next;
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex::CG_Polygon2Vertex(const CG_Polygon2Vertex& other)
: CG_Vertex2(other.X(), other.Y(), other.ID())
, m_Ear(other.m_Ear)
, m_Convex(other.m_Convex)
{
	m_Prev = other.m_Prev;
	m_Next = other.m_Next;
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex::~CG_Polygon2Vertex()
{
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex* CG_Polygon2Vertex::Prev() const
{
	return m_Prev;
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex* CG_Polygon2Vertex::Next() const
{
	return m_Next;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2Vertex::Prev(CG_Polygon2Vertex* prev)
{
	m_Prev = prev;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2Vertex::Next(CG_Polygon2Vertex* next)
{
	m_Next = next;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2Vertex::Ear() const
{
	return m_Ear;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2Vertex::Convex() const
{
	return m_Convex;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2Vertex::Ear(bool ear)
{
	m_Ear = ear;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2Vertex::Convex(bool convex)
{
	m_Convex = convex;
}

// -------------------------------------------------------------------------- //
