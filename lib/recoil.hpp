#ifdef _MSC_VER
#pragma once
#endif

#ifndef _RECOIL_HPP
#define _RECOIL_HPP

/*!
\file
Interface file for RecoilFunction clas
*/

#include <Es/Strings/rString.hpp>
#include <El/ParamFile/paramFile.hpp>


/// one part of the recoil - one impulse
struct RecoilItem
{
	float _time;
	/// vertical angular speed difference (around x axis)
	float _impulseAngX;
	/// horizontal angular speed difference (around y axis)
	float _impulseAngY;
	/// front-back offset (along z axis)
	float _offsetZ;
};

TypeIsSimple(RecoilItem)

/// force feedback converted from actual recoil position
struct RecoilFFRamp
{
	float _startTime,_duration;
	float _begAmplitude,_endAmplitude;
};


/// recoil impulses as a function of time
class RecoilFunction: public RefCount
{
	AutoArray<RecoilItem> _queue;
	ConstParamEntryPtr _cfg;

  /// find last item before or equal to given time
  int FindTime(float time) const;
  
	public:
	//! construct empty
	RecoilFunction();
	//! construct from config (CfgRecoils)
	RecoilFunction( RStringB name );
	~RecoilFunction();

	const RStringB &GetName() const;

	//! direct construction
	void AddItem(const RecoilItem &item);
	//! CfgRecoil-like construction
	void AddItem(float time, float offset, float angle);

  /// get recoil parameters for active item	
	void GetRecoil(int index, float time, float &impulseAngX, float &impulseAngY, float &offsetZ) const;
	
	bool GetTerminated( int index, float time ) const;

  /// check if we should advance to the next item 
  bool CheckItemDone(int index, float time) const;
	//! convert two recoil items to force feedback ramp effect
	bool GetFFRamp(float time, RecoilFFRamp &ramp) const;
};

#endif
