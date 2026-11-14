#ifndef CG_VERTEX2_H
#define CG_VERTEX2_H

// -------------------------------------------------------------------------- //

#include ".\CG_Point2.h"

// -------------------------------------------------------------------------- //

class CG_Vertex2 : public CG_Point2
{
	unsigned int m_ID;

public:

	CG_Vertex2();
	CG_Vertex2(double x, double y, unsigned int id = 0);
	CG_Vertex2(const CG_Point2& p, unsigned int id = 0);

	CG_Vertex2(const CG_Vertex2& other);

	virtual ~CG_Vertex2();

	unsigned int ID() const;

	void ID(unsigned int id);
};

// -------------------------------------------------------------------------- //

#endif