#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TRACKS_HPP
#define _TRACKS_HPP

#include "vehicle.hpp"

enum {TrackLODLevels=1}; // auto build several LOD levels

#include <Es/Memory/normalNew.hpp>

class TrackStep: public Entity
{
private:
	Time _startTime;
	float _alpha; // current alpha value
	Ref<Texture> _texture;

public:
	TrackStep(float alpha, Texture *texture);
	~TrackStep();

	void Change(int level, Vector3Par lastLeft, Vector3Par lastRight, float lastV, Vector3Par left, Vector3Par right, float v);
	void Final(); // perform surface split
	void StartTime();
	Time GetStartTime() const {return _startTime;}
	
  bool SimulationReady(SimulationImportance prec) const {return true;}
	void Simulate(float deltaT, SimulationImportance prec);

	void UpdateAlpha();

	virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const;
	AnimationStyle IsAnimated(int level) const; // appearance changed with Animate
	void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
	void Deanimate(int level, bool setEngineStuff);

	bool MustBeSaved() const {return false;}

	int PassNum(int lod);
  virtual int PassOrder(int lod) const;

	USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! mark left by tracks (tanks and other caterpillar vehicles)

class TrackDraw: public RefCount
{
	struct TrackLODLevel
	{
		bool _initDone;
		Vector3 _lastL,_lastR;
		//Vector3 _currL,_currR;
		float _lastV; // update v depending on distance
		TrackLODLevel():_initDone(false),_lastV(0){}
	};
	TrackLODLevel _lods[TrackLODLevels];
	 
	Ref<TrackStep> _lastPart;
	
	bool _offsets;
	Vector3 _lOffset,_rOffset; // points in object model
	
	Texture *_texture;
	float _alpha;
	
public:
	TrackDraw();
	void SetOffsets(Vector3Par lOffset, Vector3Par rOffset,	Texture *texture, float alpha);
	TrackStep *Update(const Frame &pos, bool force);

	Vector3Val LeftPos() const {return _lOffset;}
	Vector3Val RightPos() const {return _rOffset;}
	const Vector3 CenterPos() const {return (_lOffset + _rOffset) * 0.5f;}
	void Skip(const Frame &pos);
};

#include <Es/Memory/normalNew.hpp>

//! footstep mark left by men walking
class Mark: public Entity
{
	typedef Entity base;
	float _alpha,_alphaSpeed;

public:
	Mark(LODShape *shape, float alpha=1.0f, float timeToLive=10.0f);
	~Mark();

  bool SimulationReady(SimulationImportance prec) const {return true;}
	void Simulate(float deltaT, SimulationImportance prec);

	void UpdateAlpha();

  float DrawingAreaRatio() const;
	virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const;
	AnimationStyle IsAnimated(int level) const;
	int PassNum(int lod);
	void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
	void Deanimate(int level, bool setEngineStuff);

	bool MustBeSaved() const {return false;}
  virtual int PassOrder(int lod) const;

	USE_FAST_ALLOCATOR;
};

#include <Es/Memory/debugNew.hpp>

class TrackAccumulator
{
protected:
	Ref<TrackStep> _accumulate;
	bool Merge( TrackStep *merge );
	void Terminate();
  //! Function to determine the track should be terminated
  bool ShouldTerminate(const Frame &pos, Vector3Par avgTrackPos, bool terminate) const;
};

class TrackOptimized: public TrackAccumulator
{
	TrackDraw _left,_right;
public:
	TrackOptimized(const LODShape *lShape, ParamEntryPar par);
	void Update( const Frame &pos, float deltaT, bool terminate );
	Vector3 LeftPos() const {return _left.CenterPos();}
	Vector3 RightPos() const {return _right.CenterPos();}
};

class TrackOptimizedFour: public TrackAccumulator
{
	Ref<TrackStep> _accumulate;
	TrackDraw _fLeft,_fRight;
	TrackDraw _bLeft,_bRight;
public:
	TrackOptimizedFour(const LODShape *lShape, ParamEntryPar par);
	void Update( const Frame &pos, float deltaT, bool terminate );
	Vector3 BackLeftPos() const {return _bLeft.CenterPos();}
	Vector3 BackRightPos() const {return _bRight.CenterPos();}
	Vector3 FrontLeftPos() const {return _fLeft.CenterPos();}
	Vector3 FrontRightPos() const {return _fRight.CenterPos();}
};

#endif
