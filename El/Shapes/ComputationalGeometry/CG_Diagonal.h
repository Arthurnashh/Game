#ifndef CG_DIAGONAL_H
#define CG_DIAGONAL_H

// -------------------------------------------------------------------------- //

#include ".\CG_Segment2.h"

// -------------------------------------------------------------------------- //

class CG_Diagonal : public CG_Segment2
{
	bool m_FromConvex;
	bool m_ToConvex;
	bool m_Essential;

public:

	CG_Diagonal();
	CG_Diagonal(const CG_Point2& from, const CG_Point2& to, bool fromConvex, 
				bool toConvex, bool essential = true);

	CG_Diagonal(const CG_Diagonal& other);

	virtual ~CG_Diagonal();

	bool FromConvex() const;
	bool ToConvex() const;

	void FromConvex(bool fromConvex);
	void ToConvex(bool toConvex);

	bool Essential() const;

	void Essential(bool essential);
};

// -------------------------------------------------------------------------- //

#endif