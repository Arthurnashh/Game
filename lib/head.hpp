#ifdef _MSC_VER
#pragma once
#endif

#ifndef _HEAD_HPP
#define _HEAD_HPP

#if !_ENABLE_NEWHEAD

#include "animation.hpp"
#include <El/Time/time.hpp>
#include <El/ParamFile/paramFile.hpp>

#ifndef DECL_ENUM_SIMULATION_IMPORTANCE
#define DECL_ENUM_SIMULATION_IMPORTANCE
DECL_ENUM(SimulationImportance)
#endif

struct RandomVector3Type
{
	Vector3 rng;
	float minT, maxT;

	void Load(ParamEntryPar cfg);
};

class RandomVector3	
{
	Vector3 _cur;
	Vector3 _spd;
	float _timeToWanted;

	public:
	RandomVector3();

	operator const Vector3 &() const {return _cur;}
	__forceinline float X() const {return _cur.X();}
	__forceinline float Y() const {return _cur.Y();}
	__forceinline float Z() const {return _cur.Z();}

	void SetWanted(Vector3Par wanted, float time);
	bool Simulate(float deltaT);
	void SetRandomTgt(Vector3Par rng, float minT, float maxT);

	bool SimulateAndSetRandomTgt
	(
		float deltaT,
		Vector3Par rng, float minT, float maxT
	);
	bool SimulateAndSetRandomTgt
	(
		float deltaT, const RandomVector3Type &type
	);
};

// class HeadTypeOld - common class used for head animations etc.
// used f.e. in ManType
class HeadTypeOld
{
public:

	AnimationSection _personality;
	AnimationSection _glasses;

	Animation _lBrow, _mBrow, _rBrow;
	Animation _lMouth, _mMouth, _rMouth;

	Animation _eyelid;
	Animation _lip;

	RandomVector3Type _lBrowRandom, _mBrowRandom, _rBrowRandom;
	RandomVector3Type _lMouthRandom, _mMouthRandom, _rMouthRandom;

	Ref<Texture> _textureOrig;

  RString _bone;

	HeadTypeOld();
	void Load(ParamEntryPar cfg);
	void InitShape(ParamEntryPar cfg, LODShape *shape);
	void InitShapeLevel(LODShape *shape, int level);
	void DeinitShapeLevel(LODShape *shape, int level);
};

struct ManLipInfoItem
{
	float time;
	int phase;
};
TypeIsMovableZeroed(ManLipInfoItem)

/**
TODO: use LipSyncInfo instead, provides the same functionality, but extended (preloads)
*/
class ManLipInfo
{
protected:
	AutoArray<ManLipInfoItem> _items;
	int _current;
	float _freq;
	float _time;
	float _frame;
	float _invFrame;

public:
	ManLipInfo() {_frame = 0.11; _invFrame = 1.0 / _frame;}
	bool AttachWave(AbstractWave *wave, float freq = 1.0f);
	static bool AttachWaveReady(const char *wave);
	bool GetPhase(int &phase);
	float GetPhase();
  void Simulate(float deltaT);
};



// class HeadOld - common class used for head animations etc.
// used f.e. in Man
class HeadOld
{
public:
	Ref<Texture> _glasses;

	float _winkPhase;
	float _forceWinkPhase;
	float _nextWink;

	Ref<Texture> _texture;
	/// wounded textures
	Ref<Texture> _textureWounded[2];

	Vector3 _lBrow, _mBrow, _rBrow;
	Vector3 _lMouth, _mMouth, _rMouth;
	Vector3 _lBrowOld, _mBrowOld, _rBrowOld;
	Vector3 _lMouthOld, _mMouthOld, _rMouthOld;

	RandomVector3 _lBrowRandom, _mBrowRandom,	_rBrowRandom;
	RandomVector3 _lMouthRandom, _mMouthRandom, _rMouthRandom;

	RString _forceMimic;
	float _mimicPhase;
	ConstParamEntryPtr _mimicMode;
	float _nextMimicTime;

	SRef<ManLipInfo> _lipInfo;
	
	bool _randomLip;
	float _actualRandomLip;
	float _wantedRandomLip;
	float _nextChangeRandomLip;
	float _speedRandomLip;

	HeadOld(const HeadTypeOld &type, LODShape *lShape, RString faceType);
	void Animate(
		const HeadTypeOld &type, AnimationContext &animContext, LODShape *lShape, int level, bool isDead, Matrix3Par trans,
		bool hiddenHead
	);
	void Deanimate(
		const HeadTypeOld &type, LODShape *lShape, int level, bool isDead, Matrix3Par trans,
		bool hiddenHead
	);
  void PrepareShapeDrawMatrices(
    const HeadTypeOld &type, Matrix4Array &matrices, const LODShape *lShape, int level, bool isDead, Matrix4Par headMatrix
  );

	void Simulate( const HeadTypeOld &type, float deltaT, SimulationImportance prec, bool dead );

	int GetFaceAnimation() const {return toInt(_forceWinkPhase);}
	void SetFaceAnimation(int phase) {_forceWinkPhase = phase;}

	void SetFace(const HeadTypeOld &type, RString faceType, LODShape *lShape, RString name, RString player = "");
	void SetGlasses(const HeadTypeOld &type, LODShape *lShape, RString name);
	void SetForceMimic(RStringB name);
	void SetMimic(RStringB name);
	void SetMimicMode(RStringB modeName);
	RStringB GetMimicMode() const;
  const Texture *GetFace() const {return _texture;}

	void AttachWave(AbstractWave *wave, float freq = 1.0f);
	bool AttachWaveReady(const char *wave);
	void SetRandomLip(bool set = true); 

protected:
	void NextRandomLip();
};

#endif

#endif
