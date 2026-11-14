#include <El/elementpch.hpp>

#include "time.hpp"

//////////////////////////////////////////////////////////////////////////////
// classes Time, UITime implementation

float AbstractTime::Diff( const AbstractTime &x ) const
{
	// TODO: consider inline
	// TODO: consider using it as default operator -
	// current operator - overflow caused some nasty and hard to find bugs
	// safe difference - no overflow possible
	return 1e-3f * ( (__int64)_time-(__int64)x._time);
}


void UITime::operator +=(float diff)
{
	__int64 result = _time;
	result += ::toLargeInt(1e3f * diff);
	if (result < -INT_MAX) result = -INT_MAX;
	if (result > INT_MAX) result = INT_MAX;
	_time = result;
}

void UITime::operator -=(float diff)
{
	__int64 result = _time;
	result -= ::toLargeInt(1e3f * diff);
	if (result < -INT_MAX) result = -INT_MAX;
	if (result > INT_MAX) result = INT_MAX;
	_time = result;
}

float UITime::operator -(UITimeVal src) const
{
	__int64 result = _time;
	result -= src._time;
	if (result < -INT_MAX) result = -INT_MAX;
	if (result > INT_MAX) result = INT_MAX;
	return 1e-3 * result;
}

UITime UITime::operator -(float diff) const
{
	UITime ret = *this;
	ret -= diff;
	return ret;
}

UITime UITime::operator +(float diff) const
{
	UITime ret = *this;
	ret += diff;
	return ret;
}

void Time::operator +=(float diff)
{
	__int64 result = _time;
	result += ::to64bInt(1e3f * diff);
	if (result < -INT_MAX) result = -INT_MAX;
	if (result > INT_MAX) result = INT_MAX;
	_time = result;
}

void Time::operator -=(float diff)
{
	__int64 result = _time;
	result -= ::to64bInt(1e3f * diff);
	if (result < -INT_MAX) result = -INT_MAX;
	if (result > INT_MAX) result = INT_MAX;
	_time = result;
}

float Time::operator -(TimeVal src) const
{
	__int64 result = _time;
	result -= src._time;
	if (result < -INT_MAX) result = -INT_MAX;
	if (result > INT_MAX) result = INT_MAX;
	return 1e-3 * result;
}

Time Time::operator -(float diff) const
{
	Time ret = *this;
	ret -= diff;
	return ret;
}

Time Time::operator +(float diff) const
{
	Time ret = *this;
	ret += diff;
	return ret;
}
