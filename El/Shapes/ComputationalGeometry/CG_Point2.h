#ifndef CG_POINT2_H
#define CG_POINT2_H

// -------------------------------------------------------------------------- //

const double TOLERANCE = 0.00000001;

// -------------------------------------------------------------------------- //

class CG_Point2
{
	double m_X;
	double m_Y;

public:

	CG_Point2();
	CG_Point2(double x, double y);

	CG_Point2(const CG_Point2& other);

	virtual ~CG_Point2();

	bool operator == (const CG_Point2& other) const;
	bool operator != (const CG_Point2& other) const;

	double X() const;
	double Y() const;

	void X(double x);
	void Y(double y);
};

// -------------------------------------------------------------------------- //

#endif