#ifdef _MSC_VER
#pragma once
#endif

#ifndef _THING_HPP
#define _THING_HPP

#include "vehicleAI.hpp"

class ThingType: public EntityAIType
{
	protected:
	friend class Thing;
	friend class ThingEffectLight;

	typedef EntityAIType base;
	float _submerged; // some thing submerge slowly under ground
	// _submerged<=0 should be interpreted as 0, it just make submerging
	// to start later
	float _submergeSpeed; // how fast do we submerge
	float _timeToLive;
	bool _disappearAtContact;
	//@{ air friction coefficients - quadratical, linear, constant
	Vector3 _fricCoef2;
	Vector3 _fricCoef1;
	Vector3 _fricCoef0;
	float _gravityFactor;
	float _airRotation;
	bool _aerodynamics;
	//@}
	
  //@{ ambient "life" creation parameters	
	float _minHeight,_avgHeight,_maxHeight;
	//@}

	public:
	ThingType( ParamEntryPar param );
	~ThingType();

	void Load(ParamEntryPar par, const char *shape);
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;
};

class Thing: public EntityAI
{
	protected:
	enum CrashType {CrashNone,CrashLand,CrashWater,CrashObject};
	CrashType _doCrash;
	float _crashVolume;
	// time of last crash sound (will not repeat for some time)
	Time _timeCrash;
	float _submerged; // some thing submerge - see ThingType
	bool _isCloudlet;

	typedef EntityAI base;

	public:
	Thing( const EntityAIType *name );
	
	const ThingType *Type() const
	{
		return static_cast<const ThingType *>(GetType());
	}
	Vector3 Friction( Vector3Par speed );

  virtual bool RemoteStateSupported() const {return true;}

	void Simulate( float deltaT, SimulationImportance prec );

	float Rigid() const {return 1;} // how much energy is transfered in collision

	void CrashDammage( float ammount, const Vector3 &pos=VZero );

	// building are usually empty
	void Sound( bool inside, float deltaT );
	void UnloadSound();

	void DrawDiags();
	bool QIsManual() const {return false;}
  virtual bool SeenByPeriperalVision() const {return false;}

	void SetCloudlet(bool val) {_isCloudlet = val;}

	Matrix4 InsideCamera( CameraType camType ) const;
	int InsideLOD( CameraType camType ) const;
	
	// no get-in to buildings
	bool QCanIGetIn( Person *who = NULL ) const {return false;}

  //IVisibilityQuery *QueryVisibility(IVisibilityQuery *query, ClipFlags clipFlags, const FrameBase &pos);
	AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate

	USE_CASTING(base)
};

class ThingEffect: public Thing
{
	typedef Thing base;

	public:
	ThingEffect( const EntityAIType *name );

	USE_CASTING(base)
};

#include <Es/Memory/normalNew.hpp>

class ThingEffectLight: public Entity
{
  typedef Entity base;
  
	protected:
	enum CrashType {CrashNone,CrashLand,CrashWater,CrashObject};
	CrashType _doCrash;
	float _crashVolume;
	// time of last crash sound (will not repeat for some time)
	Time _timeCrash;
	float _submerged; // some thing submerge - see ThingType
	float _timeToLive;

  #if _ENABLE_REPORT
  static int _instanceCount;
  static int _maxInstanceCount;
  #endif
  
	bool _isCloudlet;
  bool _forceTI;

	public:
	ThingEffectLight( const ThingType *name );
	~ThingEffectLight();


	const ThingType *Type() const {return static_cast<const ThingType *>(GetNonAIType());}
	Vector3 Friction( Vector3Par speed ) const;

  virtual bool RemoteStateSupported() const override {return true;}

	void Simulate( float deltaT, SimulationImportance prec ) override;

	float Rigid() const override {return 1;} // how much energy is transfered in collision

  bool IsPassable() const override {return true;}
  bool HasGeometry() const override {return false;}
  virtual bool CanBeHit() const override {return false;}
  
	void Sound( bool inside, float deltaT );
	void UnloadSound();

	void SetCloudlet(bool val) {_isCloudlet = val;}
  void SetAmbient(IAmbLifeArea *area, Vector3Par camPos);

	AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
	void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
	void Deanimate( int level, bool setEngineStuff );

  void SetForceTI(bool value) { _forceTI = value; }
  virtual float GetAliveFactor() const { return _forceTI ? 1.0f : 0.0f;}

	USE_CASTING(base)
	USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

enum ThingEffectKind
{
	TEGround,
	TEArmor,
	TEHouse,
	TECartridge, // TODO: right word
	NThingEffectKind
};

//! create thing fo given type - exact class name is randomized
//! also adds it to World and Landscape
Entity *CreateThingEffect
(
	ThingEffectKind kind, // kind
	Matrix4Val pos, Vector3Val vel // position and velocity
);

//! create thing with a known type
//! also adds it to World and Landscape

Entity *CreateThing
(
	VehicleType *type, // kind
	Matrix4Val pos, Vector3Val vel, // position and velocity
  bool forceTi
);

#endif
