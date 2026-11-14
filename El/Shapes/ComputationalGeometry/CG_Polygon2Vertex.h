#ifndef CG_POLYGON2VERTEX2_H
#define CG_POLYGON2VERTEX2_H

// -------------------------------------------------------------------------- //

#include ".\CG_Vertex2.h"

// -------------------------------------------------------------------------- //

class CG_Polygon2Vertex : public CG_Vertex2
{
	bool m_Ear;
	bool m_Convex;

	CG_Polygon2Vertex* m_Prev;
	CG_Polygon2Vertex* m_Next;

public:

	CG_Polygon2Vertex();
	CG_Polygon2Vertex(double x, double y, 
					  unsigned int id = 0, bool ear = false,
					  bool convex = true, CG_Polygon2Vertex* prev = 0,
					  CG_Polygon2Vertex* next = 0);
	CG_Polygon2Vertex(const CG_Point2& p,
					  unsigned int id = 0, bool ear = false,
					  bool convex = true, CG_Polygon2Vertex* prev = 0,
					  CG_Polygon2Vertex* next = 0);

	CG_Polygon2Vertex(const CG_Polygon2Vertex& other);

	virtual ~CG_Polygon2Vertex();

	CG_Polygon2Vertex* Prev() const;
	CG_Polygon2Vertex* Next() const;

	void Prev(CG_Polygon2Vertex* prev);
	void Next(CG_Polygon2Vertex* next);

	bool Ear() const;
	bool Convex() const;

	void Ear(bool ear);
	void Convex(bool convex);
};

// -------------------------------------------------------------------------- //

#endif
