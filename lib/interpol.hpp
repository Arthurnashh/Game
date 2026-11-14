#ifdef _MSC_VER
#pragma once
#endif

#ifndef _INTERPOL_HPP
#define _INTERPOL_HPP

#include "El/Math/math3d.hpp"
#include <Es/Algorithms/splineNEqD.hpp>

class M4Function
{
	public:
	virtual Matrix4 operator() ( float time ) const = 0;
	virtual ~M4Function(){}

	// query domain
	virtual float MinArg() const = 0;
	virtual float MaxArg() const = 0;
};

class InterpolatorLinear: public M4Function
{
	const Matrix4 *_values;
	const float *_times;
	int _n;

	public:
	InterpolatorLinear( const Matrix4 *values, const float *times, int n );
	Matrix4 operator() ( float time ) const;

	float MinArg() const {return _times[0];}
	float MaxArg() const {return _times[_n-1];}
};

class InterpolatorSpline: public M4Function
{
	const Matrix4 *_values;
	const float *_times;
	int _n;
	Temp<Matrix4> _sValues;

	public:
	InterpolatorSpline( const Matrix4 *values, const float *times, int n );
	Matrix4 operator() ( float time ) const;

	float MinArg() const {return _times[0];}
	float MaxArg() const {return _times[_n-1];}
};

#endif
