#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ENV_TRACKER_HPP
#define _ENV_TRACKER_HPP

#include <El/Time/time.hpp>

/// track how much our surroundings are hiding us

class SurroundTracker
{
	Vector3 _lastPos;
	Time _lastTime;

  /// density when viewed from 50 m
	float _value50m;

  /// density when viewed from 300 m
	float _value300m;

	public:
	SurroundTracker();

  /// compute hiding 
	void Update(const Object *who, Vector3Par pos);

  /// how much is this one hiding
  float Hiding(float dist2, float radius2, float objArea, float bestArea, float invBestArea, float bestRadius) const;

  /// compute current value if needed, and return it
	float Track(const Object *who, Vector3Par pos, float dist);

	float GetDensity(float dist) const {return InterpolativC(dist,50.0f,300.0f,_value50m,_value300m);}
};

/// track how much is the unit lit

class LightTracker
{
	Vector3 _lastPos;
	Time _lastTime;

	float _value;

	public:
	LightTracker();

  /// compute hiding 
	void Update(const Object *who, Vector3Par pos);

  /// how much is this one hiding
  float GetValue() const {return _value;}

  /// compute current value if needed, and return it
	float Track(const Object *who, Vector3Par pos);
};

#endif
