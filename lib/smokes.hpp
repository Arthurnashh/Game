#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SMOKES_HPP
#define _SMOKES_HPP

#include "vehicle.hpp"
#include "paramFileExt.hpp"
#include "rtAnimation.hpp"
#include <El/SimpleExpression/simpleExpression.hpp>
#include <El/Evaluator/expressImpl.hpp>
#include <El/ParamArchive/serializeClass.hpp>

#include <Es/Memory/normalNew.hpp>

bool NewSprite(const char *name, ShapeParameters pars, Ref<LODShape> &lsws, bool setMaterial = true, bool forceLoad = true);

class Smoke: public Entity
{
  typedef Entity base;

  protected:
  float _animation;
  float _animationSpeed;

  float _timeToLive;

  bool _firstLoop;
  float _alpha;
  float _fadeValue;
  float _fadeIn,_fadeInTime,_fadeInInv;
  float _fadeOut,_fadeOutTime,_fadeOutInv;

  public:
  Smoke( LODShape *shape, const EntityType *type, float duration, float loopedDuration=0 );
  ~Smoke();

  void SetFadeIn( float fadeIn )
  {
    _fadeInInv=( fadeIn>0 ? 1/fadeIn :0 );
    _fadeIn=_fadeInTime=fadeIn;
  }
  void SetFadeOut( float fadeOut )
  {
    _fadeOutInv=( fadeOut>0 ? 1/fadeOut :0 );
    _fadeOut=_fadeOutTime=fadeOut;
  }
  void SetFades( float fadeIn, float fadeOut )
  {
    SetFadeIn(fadeIn);
    SetFadeOut(fadeOut);
  }
  void SetAlpha( float alpha ) {_alpha=alpha;}
  void SetSpeed( Vector3Par speed ){FutureVisualState().SetSpeed(speed);}
  void SetTimeToLive( float time ){_timeToLive=time;}
  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  void Simulate( float deltaT, SimulationImportance prec );

  virtual void DisappearASAP(float time);

  AnimationStyle IsAnimated( int level ) const; // appearence changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2); // alpha animation
  void Deanimate( int level, bool setEngineStuff );

  float Animation() const {return _animation;}
  void Sound( bool inside, float deltaT ){}
  void UnloadSound(){}
  Visible VisibleStyle() const {return _invisible || _shape.IsNull() ? ObjInvisible : ObjVisibleFast;}

  virtual LSError Serialize(ParamArchive &ar);

//  USE_FAST_ALLOCATOR;
};

struct CloudletTItem
{
  float maxT;
  Color color;

  LSError Serialize(ParamArchive &ar);
};
TypeIsSimple(CloudletTItem)

class CloudletTTable : public RefCount, public AutoArray<CloudletTItem>
{
public:
  typedef AutoArray<CloudletTItem> base;

  int Add(float maxT, Color color);
  Color GetColor(float t) const;
};

class CloudletType : public RemoveLLinks
{
protected:
  ConstParamEntryPtr _entry;
  Ref<CloudletTTable> _cloudletTTable;

public:
  CloudletType(ConstParamEntryPtr entry);
  ConstParamEntryPtr GetName() const {return _entry;}
  CloudletTTable *GetTable() {return _cloudletTTable;}
};

/// Traits for CloudletTypeBank
template <>
struct BankTraits<CloudletType> : public DefLLinkBankTraits<CloudletType>
{
  typedef ConstParamEntryPtr NameType;

  // default name comparison
  static int CompareNames( const NameType &n1, const NameType &n2 )
  {
    return n1 != n2;
  }
};

class Cloudlet: public Smoke
{
  typedef Smoke base;

  float _size; // size of cloudlet
  float _growSize; // current size (0..1)
  float _growUp,_growUpTime,_growUpInv;

  float _accY; // gravity acceleration
  float _minYSpeed,_maxYSpeed; // max. descent rate

  float _xSpeed0,_zSpeed0;
  float _xFriction,_zFriction; // deceleration until speed0

  float _t, _dt;
  Ref<CloudletTTable> _cloudletTTable;
  Color _cloudletColor;
    
  public:
  Cloudlet( LODShape *shape, float duration, float loopedDuration=0 );

  void SetGrowUp( float growUp, float size )
  {
    _size=size;
    _growUpInv=( growUp>0 ? 1/growUp :0 );
    _growUp=_growUpTime=growUp;
  }

  void SetClimbRate( float accY, float minY, float maxY )
  {
    _accY=accY;
    _minYSpeed=minY;
    _maxYSpeed=maxY;
  }
  void SetSideSpeed( float xS0, float zS0, float xF, float zF )
  {
    _xSpeed0=xS0,_zSpeed0=zS0;
    _xFriction=xF,_zFriction=zF; // deceleration until speed0
  }
  void SetColor(ColorVal color) {_cloudletColor = color;}
  void SetTemperature(float t, float dt, CloudletTTable *table);

  float CloudletClippingCoef() const;
  SimulationImportance WorstImportance() const;
  SimulationImportance BestImportance() const;
  
  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  void Simulate( float deltaT, SimulationImportance prec );
  //void ApplyMove( float deltaT );
  virtual void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &frame, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  ) override;

  //! Virtual method
  virtual bool IsSprite() {return true;}

  #if ALPHA_SPLIT
  void DrawAlpha( int level, ClipFlags clipFlags );
  #endif
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const;

  // cloudlet does not occlude neither view nor fire
  bool OcclusionFire() const {return false;}
  bool OcclusionView() const {return false;}

  virtual float VisibleSize(ObjectVisualState const& vs) const;
  
  bool MustBeSaved() const {return false;}

  
  virtual LSError Serialize(ParamArchive &ar) {return LSOK;}  // TOO: ?? serialize

  virtual bool MustBeInstanced() {return true;};
  virtual void SetupInstances(int cb, const Shape &sMesh, const ObjectInstanceInfo *instances, int nInstances);

  // basic element from which smoke trails are built
  USE_FAST_ALLOCATOR;
};

class CloudletSource: public SerializeClass
{
  protected:
  // source of smoke trail
  // how often generate
  float _interval,_nextTime;

  // parameters of cloudlets
  Ref<LODShape> _cloudletShape;
  float _cloudletDuration;
  float _cloudletAnimPeriod;
  float _cloudletSize;
  float _cloudletAlpha;
  float _cloudletGrowUp;
  float _cloudletFadeIn;
  float _cloudletFadeOut;
  float _cloudletAccY;
  float _cloudletMinYSpeed;
  float _cloudletMaxYSpeed;
  float _cloudletInitT;
  float _cloudletDeltaT;
  Color _cloudletColor;
  Vector3 _cloudletSpeed;

  Vector3 _lastPosition;
  bool _lastPositionValid;

  //float _generalize; // LOD level 1.0 = best, 1e20 = invisible

  Ref<CloudletType> _type;

  public:
  CloudletSource( LODShape *shape = NULL, float interval = 0.3);
  float GetInterval() const {return _interval;}
  void SetInterval( float interval ){_interval=interval;}
  void SetShape( LODShape *shape ){_cloudletShape=shape;}
  void SetAlpha( float alpha ) {_cloudletAlpha=alpha;}
  void SetSize( float size ) {_cloudletSize=size;}
  void SetTimes( float duration, float animPeriod )
  {
    _cloudletDuration=duration;
    _cloudletAnimPeriod=animPeriod;
  }
  void SetFades( float growUp, float fadeIn, float fadeOut )
  {
    _cloudletGrowUp=growUp;
    _cloudletFadeIn=fadeIn;
    _cloudletFadeOut=fadeOut;
  }
  void SetSpeed( Vector3Par speed ) {_cloudletSpeed=speed;}
  void SetClimbRate( float accY, float minY, float maxY )
  {
    _cloudletAccY=accY;
    _cloudletMinYSpeed=minY;
    _cloudletMaxYSpeed=maxY;
  }
  Color GetColor() const {return _cloudletColor;}
  void SetColor(ColorVal color) {_cloudletColor = color;}

  Cloudlet *Drop( Vector3Par pos, Vector3Par speed, float relativeSize);
  void Simulate(Vector3Par pos, Vector3Par speed, float deltaT);

  void Load(ParamEntryPar cls);

  LSError Serialize(ParamArchive &ar);
};

class DustSource: public CloudletSource
{
  typedef CloudletSource base;

  float _size,_sourceSize;

  protected:
  float _generalizeFactor;
  float _maxGeneralize;
  float _windCoef;

  private:
  void Init();

  public:
  DustSource( LODShape *shape, float interval );
  DustSource( float interval=0.05 ); // with basic shape
  void Simulate(Vector3Par pos, Vector3Par speed, float density, float deltaT);
  float GetSize() const {return _size;}
  float GetSourceSize() const {return _sourceSize;}

  void SetSize( float size ) {_size=_sourceSize=size;}
  void SetSize( float size, float sSize ) {_size=size,_sourceSize=sSize;}

  void SetWindCoef(float coef) {_windCoef = coef;}

  LSError Serialize(ParamArchive &ar);

  void Load(ParamEntryPar cls);
};

class WaterSource: public DustSource
{
  public:
  WaterSource( LODShape *shape, float interval );
  WaterSource( float interval=0.05 ); // water shape
};

class ExhaustSource: public DustSource
{
  typedef DustSource base;

  void Init();

  public:
  ExhaustSource( LODShape *shape, float interval );
  ExhaustSource( float interval=0.05 ); // water shape

  void SetSize( float size );
};

class WeaponCloudsSource: public DustSource
{
  typedef DustSource base;

  protected:
  float _timeToLive;

  private:
  void Init();

  public:
  WeaponCloudsSource( float interval=0.05 ); // basic shape
  WeaponCloudsSource( LODShape *shape, float interval );
  void Simulate
  (
    Vector3Par pos, Vector3Par speed, float density, float deltaT
  );
  void Start( float time );
  bool Active() const {return _timeToLive>=0;}

  LSError Serialize(ParamArchive &ar);
};

class SmokeSource: public CloudletSource
{
  typedef CloudletSource base;

protected:
  Vector3 _speed;

  float _density;
  float _size, _sourceSize;
  float _inOutDensity;

  float _timeToLive;
  float _in,_inTime,_inInv;
  float _out,_outTime,_outInv;

public:
  SmokeSource(ParamEntryPar cls);
  void SetSize(float size);
  void SetDensity(float density);

  // SmokeSource(LODShape *shape = NULL, float density = 1.0, float size = 1.0);

  bool Simulate
  (
    Vector3Par pos, Vector3Par speed, float deltaT, SimulationImportance prec
  );

  void SetSourceSize(float sourceSize)
  {
    _sourceSize = sourceSize;
  }
  void SetSize(float size, float sourceSize)
  {
    _size = size;
    _sourceSize = sourceSize;
  }

  void SetIn( float in )
  {
    _inInv=( in>0 ? 1/in :0 );
    _in=_inTime=in;
  }
  void SetOut( float out )
  {
    _outInv=( out>0 ? 1/out :0 );
    _out=_outTime=out;
  }
  void SetSourceTimes( float in, float live, float out )
  {
    _timeToLive=live;
    SetIn(in);
    SetOut(out);
  }

  LSError Serialize(ParamArchive &ar);

  void Load(ParamEntryPar cls);
};

class IExplosion
{
protected:
  float _minExplosionFactor,_maxExplosionFactor;
  Time _explosionTime;
  bool _exploded;

public:
  IExplosion();

  void SetExplosion(float minExp, float maxExp)
  {
    _minExplosionFactor = minExp;
    _maxExplosionFactor = maxExp;
  }
  void Explode(Time time = TIME_MIN);

  virtual void SimulateExplosion() = 0;
  virtual void DoResults() = 0;
  virtual void UndoResults() = 0;
};

class SmokeSourceVehicle: public Entity, public SmokeSource, public IExplosion
{
  typedef Entity base;

  protected:
  Ref<LightPoint> _light;
  float _lightTime;
  OLinkO(EntityAI) _owner;
  WeaponCloudsSource _fire,_darkFire;
  
  public:
  SmokeSourceVehicle
  (
    ParamEntryPar cls,
    float density=1, float size=1,
    EntityAI *owner=NULL
  );
  ~SmokeSourceVehicle(){}

  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  void Simulate( float deltaT, SimulationImportance prec );
  void Sound( bool inside, float deltaT );
  void UnloadSound();

	virtual Object *GetObject() const {return NULL;}
  virtual void SetObject(Object *){};

  virtual IExplosion *GetExplosionInterface() {return this;}
  virtual void SimulateExplosion();
  virtual void DoResults();
  virtual void UndoResults();

  virtual LSError Serialize(ParamArchive &ar);

  USE_CASTING(base)
};

class SmokeSourceOnVehicle: public SmokeSourceVehicle,public AttachedOnVehicle
{
  public:
  SmokeSourceOnVehicle
  (
    ParamEntryPar cls,
    float density=1, float size=1, EntityAI *owner=NULL,
    Object *vehicle=NULL, Vector3Par position=VZero
  );
  ~SmokeSourceOnVehicle(){}

  void UpdatePosition();
  void CleanUp();

	virtual Object *GetObject() const {return _vehicle;}
  virtual void SetObject(Object * obj) {_vehicle = obj;};

  virtual LSError Serialize(ParamArchive &ar);
};

class Slop : public Smoke
{
protected:
  typedef Smoke base;

  float _size;
  float _growSize; // current size (0..1)
  float _growUpTime, _growUpInv;

  Matrix4 _origTransform;

public:
  Slop(LODShape *shape, const EntityType *type, const Matrix4 &trans, float timeToLive = 180, float size = 1);

  void SetGrowUp( float growUp, float size )
  {
    _size = size;
    _growUpTime = growUp;
    _growUpInv = growUp > 0 ? 1 / growUp : 0;
  }

  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  void Simulate(float deltaT, SimulationImportance prec);

  AnimationStyle IsAnimated( int level ) const {return NotAnimated;} // appearance changed with Animate
  
  /// blood slops using splitting - cannot use CB
  virtual bool CanObjDrawAsTask(int level) const {return false;}

  /// instancing has little sense here - we are splitting the surface anyway
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const {return false;}
  
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff){} // no texture animation
  void Deanimate( int level, bool setEngineStuff ){}
  
private:
  void Init(float timeToLive, float size);

//  USE_FAST_ALLOCATOR;
  USE_CASTING(base)
};

class WeaponLightSource : public SerializeClass
{
protected:
  float _timeToLight;
  float _invTotalTimeToLight;
  float _lightIntensity;
  bool _lightMGun;
  Ref<LightPoint> _light;

private:
  void Init();

public:
  WeaponLightSource();
  void Start(float time, float intensity, bool mGun);
  bool Active() const;
  void Simulate(Vector3Par pos, float deltaT);
  virtual LSError Serialize(ParamArchive &ar);
};

class WeaponFireSource: public WeaponCloudsSource
{
  typedef WeaponCloudsSource base;

  WeaponLightSource _light;

private:
  void Init();

public:
  WeaponFireSource( float interval=0.05 ); // fire shape
  WeaponFireSource( LODShape *shape, float interval );
  void Start( float time, float intensity, bool mGun );
  bool Active() const;
  void Simulate
  (
    Vector3Par pos, Vector3Par speed, float density, float deltaT
  );

  virtual LSError Serialize(ParamArchive &ar);
};

class ObjectDestructed: public Entity
{
  typedef Entity base;

  OLink(Object) _destroy;
  float _anim,_speed;

  public:
  ObjectDestructed(LODShape *shape = NULL);
  ObjectDestructed(Object *destroy, float timeToLive = 3);
  ~ObjectDestructed();
  
  void Simulate( float deltaT, SimulationImportance prec );

  virtual LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  static ObjectDestructed *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx);

  USE_CASTING(base)
//  USE_FAST_ALLOCATOR;
};


//------- Particles --------

/*!
  Determines the type of the particle.
*/
#define PARTICLE_TYPE_ENUM(type, prefix, XX) \
  XX(type, prefix, Billboard) \
  XX(type, prefix, SpaceObject)

#ifndef DECL_ENUM_PARTICLE_TYPE
#define DECL_ENUM_PARTICLE_TYPE
DECL_ENUM(ParticleType)
#endif
DECLARE_ENUM(ParticleType, PT, PARTICLE_TYPE_ENUM)

/*!
  Using this class you can receive random values in time which will depend
  on rate of an event and maximal value to change in one event. Now it doesn't
  matter whether you will ask for a random value ten times each second or
  one in ten seconds. The result will be in the same boundary.
  Moreover in case the rate is too small and we ask for values often than _rate
  parameter, the result will be interpolated (this is not done yet).
  Note that to use this class you have to call Init(...) method first.
*/
class CRandomQuantity
{
private:
  //! Rate of the event (in NumberOfEvents/ms).
  float _rate;
  //! Maximum value to change in one event.
  float _maxChangeInEvent;
public:
  //! Constructor
  CRandomQuantity()
  {
  }
  //! Initialization of RQ.
  void Init(float rate, float maxChangeInEvent)
  {
    _rate = rate;
    _maxChangeInEvent = maxChangeInEvent;
  }
  //! Retrieving a value.
  /*!
    According to time period dT this method will return random number within 
    specified boundary.
    \param dT Time period the RandomQuantity according to _rate could have change the value.
    \return Desired random value.
  */
  float Get(int dT)
  {
    // MaxChangeDuringPeriod 
    float mcdp = _rate * dT * _maxChangeInEvent;
    return (rand() * mcdp * 2) / RAND_MAX - mcdp;
  }
};

/*!
  This class represents a random walk according to specified parameters. Using
  this class we can retrieve it's result in an arbitrary time. Random walk starts
  in point zero. At every event value can change by an arbitrary value in interval
  (- _maxChangeInEvent, _maxChangeInEvent). This class also provides a linear
  interpolation on returned values. Note that method Get(Time) must be called with
  forward time in sequence - forward walk.
*/
class CRandomWalk
{
private:
  //! Time period between events (ms).
  /*!
    This suppose to be a reasonable value not too much bigger than rate of calling
    the Get(Time) method. (50-30 ms = 20-30 FPS). At least in a present implementation.
    That's because next values in time we receive using iteration from current
    state. It would may be better to get next point directly. (Using inv Gaus curve?).
    Value -1 means the random walk returns always 0.
  */
  int _eventPeriod;
  //! Maximum value to change in one event.
  float _maxChangeInEvent;
  //! Time of the previous event.
  Time _prevTime;
  //! Value of the previous event.
  float _prevValue;
  //! Value of the next event.
  float _nextValue;
public:
  //! Constructor.
  CRandomWalk()
  {
    _eventPeriod = -1;
  }
  //! Initialization of a random walk.
  void Init(int eventPeriod, float maxChangeInEvent)
  {
    _eventPeriod = eventPeriod;
    _maxChangeInEvent = maxChangeInEvent;
    _prevTime = Time(); // Set to zero
    _prevValue = 0;
    _nextValue = (rand() * _maxChangeInEvent * 2) / RAND_MAX - _maxChangeInEvent;
  }
  //! Returns the value of the random walk in time T since the beginning.
  /*!
    This method is intend for forward walk only.
  */
  float Get(Time t)
  {
    Assert(t > _prevTime);
    // In case we don't use the random walk
    if (_eventPeriod == -1) return 0.0f;
    // Find the proper interval using iteration.
    Time nextTime = _prevTime.AddMs(_eventPeriod);
    while (nextTime < t)
    {
      _prevValue = _nextValue;
      _nextValue += (rand() * _maxChangeInEvent * 2) / RAND_MAX - _maxChangeInEvent;
      _prevTime = nextTime;
      nextTime = _prevTime.AddMs(_eventPeriod);
    }
    // Use linear interpolation to get the value
    return (t.Diff(_prevTime) * (_nextValue - _prevValue)) / _eventPeriod + _prevValue;
  }
  // GetDelta
};


//! skeletal animation system

struct ParticleSkeletalAnimation: public RefCount
{
  Ref<AnimationRT> _animation;
  //WeightInfo _Weights;
  Ref<Skeleton> _skeleton;
};

struct CParticleType;

/// Particle values with randomization
struct CParticleRandomization
{
  // ----- LifeTime -----
  //! Expected time for a particle to live (in s).
  float _lifeTime;
  // ----- State of the particle -----
  //! Position of the particle in space.
  Vector3 _position;
  // ----- Movement changes -----
  //! Vector which determines direction of a movement and speed (m/s) by it's magnitude in the beginning.
  Vector3 _moveVelocity;
  //! Scalar which determines rotation speed of the particle at the beginning(rot/sec).
  float _rotationVelocity;
  // ----- Appearance -----
  //! Rotation angle of the particle
  float _angle;
  //! Size of the particle in time.
  /*!
    Note that this variable is intend for rendering only. Not a physics computation.
  */
  float _sizeCoef;
  //! Color and transparency of the particle in time.
  Color _colorCoef;
  // ----- Random changes -----
  //! Period of the random change of the direction of the particle (in s).
  float _randomDirectionPeriod;
  //! Intensity of the random change of the direction of the particle (in m/s).
  float _randomDirectionIntensity;

  void Init();
  void Init(const CParticleType *type);
  bool Init(const GameState *state, GameValuePar oper1);
};

typedef Pair<SkeletonIndex,float> SkeletonBoneWeight;

TypeIsSimple(SkeletonBoneWeight)

/// Common particle values
struct CParticleType : public RefCount
{
  //! Type of the particle.
  ParticleType _type;
  //! Time period (int s) the OnTimer event will be executed with.
  float _timerPeriod;
  // ----- LifeTime -----
  //! Expected time for a particle to live (in s).
  float _lifeTime;
  // ----- State of the particle -----
  //! Position of the particle in space.
  Vector3 _position;
  // ----- Movement changes -----
  //! Vector which determines direction of a movement and speed (m/s) by it's magnitude in the beginning.
  Vector3 _moveVelocity;
  //! Scalar which determines rotation speed of the particle at the beginning(rot/sec).
  float _rotationVelocity;
  // ----- Movement parameters -----
  //! Weight of a particle (in kg).
  float _weight;
  //! Volume of a particle (in m^3).
  float _volume;
  //! Impact of the density of the environment to movement of the particle (0..1).
  float _rubbing;
  // ----- Appearance -----
  //! Rotation angle of the particle
  float _angle;
  //! Size of the particle in time.
  /*!
    Note that this variable is intend for rendering only. Not a physics computation.
  */
  AutoArray<float> _size;
  //! Color and transparency of the particle in time.
  AutoArray<Color> _color;
  //! Speed of animation of the particle in time (in cycles per second).
  AutoArray<float> _animationSpeed;
  /// multiplier to modify _size
  float _sizeCoef;
  /// multipliers to modify _color
  Color _colorCoef;
  /// multiplier to modify _animationSpeed
  float _animationSpeedCoef;
  // ----- Random changes -----
  //! Period of the random change of the direction of the particle (in s).
  float _randomDirectionPeriod;
  //! Intensity of the random change of the direction of the particle (in m/s).
  float _randomDirectionIntensity;
  // ----- Scripts -----
  //! Script which will be executed regularly in specified period.
  RString _onTimerScript;
  //! Script which will be executed before destroying the particle.
  RString _beforeDestroyScript;
  //! position is relative to object
  OLink(Object) _object;
  //! object was valid on creation
  bool _isObject;
  //! skinning to object, if any
  SkinnedPosition _skinning;

  CParticleType()
  {
    _isObject = false;
    _sizeCoef = 1;
    _colorCoef = Color(1, 1, 1, 1);
    _animationSpeedCoef = 1;
  }
};

struct CParticleParams
{
  /** currently used for debugging, but perhaps might have other uses as well*/
  RString _sourceTypeName;
  //! Name of the _shape
  RString _shapeName;
  //! Frame source
  FrameSource _frameSource;
  Ref<LODShape> _shape;
  RString _animationName;
  Ref<CParticleType> _type;
  /// sometimes we want the particle to be create on surface only
  bool _onSurface;

  bool Init(const GameState *state, GameValuePar oper1, bool relativePos);

  static bool GetObjectPosition( const GameArrayType &array, Vector3 &pos, Object *obj, const GameState *state );

};

/*!
  Provides a particle simulation.
*/
class CParticle : public Entity
{
private:
  typedef Entity base;

  //! Orientation of the particle after rotation - usually set by random.
  Matrix3 _rotOrientation0_m_Orientation;
  Matrix3 _invRotOrientation0;
  // ----- Behavior -----
  Ref<CParticleType> _type;
  CParticleRandomization _randomization;
  // instanced values from _type (to not change them when type changed)
  float _timerPeriod;
  float _rubbing;
  float _weight;
  float _volume;
  float _sizeCoef;
  Color _colorCoef;
  float _animationSpeedCoef;

  /// size relative to what is in _values
  float _relativeSize;
  //! Last age of the particle OnTimer event was called.
  float _lastOnTimerScriptCalling;
  //! Actual age of a particle (in s).
  float _age;
  //! if skeletal animation is needed
  Ref<ParticleSkeletalAnimation> _skeletalAnim;
  //! Orientation of the particle - rotation around random vector (angle).
  float _rotation;
  //! Phase of the animation
  float _animationPhase;
  //! Frame source
  FrameSource _frameSource;

public:
  CParticle(const CParticleParams &params, const CParticleRandomization &randomization, float relativeSize);
  ~CParticle();
  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  void Simulate(float deltaT, SimulationImportance prec);

  void UpdateRenderingState(float shrinkedAge);
  AnimationStyle IsAnimated(int level) const;
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const;
  virtual bool MustBeInstanced() {return true;};
  virtual void SetupInstances(int cb, const Shape &sMesh, const ObjectInstanceInfo *instances, int nInstances);
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate(int level, bool setEngineStuff);
  virtual void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &frame, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  ) override;
  virtual bool IsSprite() {return true;}
  virtual float VisibleSize(ObjectVisualState const& vs) const;
  virtual bool GetFrameSource(FrameSource &frameSource) const;
  virtual float GetAnimationPhase() const;
  virtual float GetAngle() const {return _randomization._angle + _randomization._rotationVelocity;}

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

/// Simple particle source
class CParticleSourceBase
{
protected:
  CParticleParams _params;
  CParticleRandomization _variance;
  float _interval;
  float _nextTime;
  float _circleRadius;
  Vector3 _circleVelocity;
  //! Position in world coordinates set in previous simulation step
  Vector3 _oldPosition;
  //! Flag to determine the _oldPosition have already been initialized
  bool _isOldPositionInitialized;

public:
  //! Constructor
  CParticleSourceBase();
  void SetParams(const CParticleParams &params);
  void SetRandom(const CParticleRandomization &values);
  void SetCircle(float circleRadius, Vector3Par circleVelocity);
  void SetInterval(float interval) {_interval = interval;}
  void AttachObject(Object *object, Vector3Par pos);
  void AttachObject(Object *object); // reattach source to a new object
  void DetachObject();

  void Simulate(Entity *source, float deltaT, SimulationImportance prec);
  LSError Serialize(ParamArchive &ar);
  /// load fixed parameters and compile expression parameters
  void Load(RString name, EffectVariablesPar evars);
  /// direct access to parameters (used in EffectsSourceRT)
  CParticleType *AccessType() {return _params._type;}
  /// direct access to randomization (used in EffectsSourceRT)
  CParticleRandomization &AccessVariance() {return _variance;}

protected:
  //!{ Transformation to world upon source and object
  Vector3 TransformPositionModelToWorld(const Entity* source, Vector3Par position);
  Vector3 TransformDirectionModelToWorld(const Entity* source, Vector3Par direction);
  Vector3 TransformSpeed(const Entity* source, Vector3Par speed);
  //!}
  /// drop one particle
  void Drop(Entity *source, float relativeSize, Vector3Par previousPosition, float positionFactor, Vector3Par sourceSpeed);
  /// rough estimation - used for generalization
  Vector3 EstimateDropPosition(Entity *source) const;
};

/// Particle source entity
class CParticleSource : public Entity, public CParticleSourceBase
{
  typedef Entity base;

public:
  //! Constructor
  CParticleSource(const EntityType *type);
  virtual void Simulate(float deltaT, SimulationImportance prec);
  virtual LSError Serialize(ParamArchive &ar);
  USE_CASTING(base)
};


/*
  Base member for all destruction effect. This structure determinate effect behaviour under water. Is flag
  is set to false then effect is disabled under water, else is enabled. Structure was created to fixed
  news:eh818f$23j$1@new_server.localdomain
*/
struct DestructEffect
{
  bool _doUnderWater;

  void Load() { _doUnderWater = false; }
  LSError Serialize(ParamArchive &ar);
};

struct LightEffect : public DestructEffect
{
  typedef DestructEffect base;

  Ref<LightPoint> _light;
  Vector3 _pos;
  float _timeToLive;

  void Load(const DestructionEffectType &effType, EffectVariablesPar evars, LODShape *shape, const Vector3 *defPos, float lifeTime);
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(LightEffect)

struct ParticlesEffect : public DestructEffect
{
  typedef DestructEffect base;

  CParticleSourceBase _source;
  float _timeToLive;

  void Load(const DestructionEffectType &effType, EffectVariablesPar evars, Object *obj, const Vector3 *defPos, float lifeTime);
  LSError Serialize(ParamArchive &ar);
};
TypeContainsOLink(ParticlesEffect)

class DynSoundObject;

struct SoundEffect : public DestructEffect
{
  typedef DestructEffect base;

  RefR<DynSoundObject> _sound;
  float _timeToLive;

  void Load(const DestructionEffectType &effType, float lifeTime);
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(SoundEffect)

/// change position during destruction
struct DestructPosEffect : public DestructEffect
{
  typedef DestructEffect base;

  /// delay before changing the position
  float _delay;
  /// position change duration
  float _duration;
  /// time left: _duration...0
  float _time;

  LSError Serialize(ParamArchive &ar);
  void Load(ParamEntryPar cls, EffectVariablesPar evars);
};
TypeIsMovableZeroed(DestructPosEffect)

/// spawn some debris during destruction
struct RuinsEffect : public DestructEffect
{
  typedef DestructEffect base;

  /// model which will be replacing the one
  /** once the model is spawned, empty string is assigned here */
  RString _shape;
  /// if we want to be able to recover, we need to keep the link for ruins
  OLink(Entity) _ruin;
  /*
  /// delay before ruins start to appear
  float _delay;
  /// duration of the appearing
  float _duration;
  /// current time: 0..._duration
  float _time;
  */

  LSError Serialize(ParamArchive &ar);
  void Load(RStringB shapeName, EffectVariablesPar evars);
};
TypeContainsOLink(RuinsEffect)

/// causing damage around once destroyed
struct DamageAroundEffect : public DestructEffect
{
  typedef DestructEffect base;

  /// radius is the building radius multiplied by this
  float radiusRatio;
  /// same as with normal ammo
  float indirectHit;

  LSError Serialize(ParamArchive &ar);
  void Load(ParamEntryPar cls, EffectVariablesPar evars);
};
TypeContainsOLink(DamageAroundEffect)

/// simple expressions for all elements of Vector3
struct ExpressionCodeVector
{
  ExpressionCode _data[3];

  bool Compile(ParamEntryPar entry, const RString *varNames, int varCount);
  bool Evaluate(Vector3 &result, const float *varValues, int varCount) const;
  ExpressionCode &operator[](int i) {return _data[i];}
  const ExpressionCode &operator[](int i) const {return _data[i];}
};
TypeIsMovable(ExpressionCodeVector)

/// simple expressions for all elements of Color
struct ExpressionCodeColor
{
  ExpressionCode _data[4];

  bool Compile(ParamEntryPar entry, const RString *varNames, int varCount);
  bool Compile(const IParamArrayValue &entry, const RString *varNames, int varCount);
  bool Evaluate(Color &result, const float *varValues, int varCount) const;
  ExpressionCode &operator[](int i) {return _data[i];}
  const ExpressionCode &operator[](int i) const {return _data[i];}
};
TypeIsMovable(ExpressionCodeColor)

/// encapsulation of different effect sources (smoke, fire, light, sound)
class EffectsSource : public SerializeClass
{
protected:
  OLink(Entity) _entity; // attached to entity

  AutoArray<LightEffect> _lights;
  AutoArray<ParticlesEffect> _particleSources;
  AutoArray<SoundEffect> _sounds;

public:
  EffectsSource() {}
  void Init(RString type, Entity *entity, EffectVariables &evars, float intensity, float density, float fireIntensity, float fireDentensity, float lifeTime);

  // return if the simulation of all effects finished
  bool Simulate(float deltaT, SimulationImportance prec);

  virtual LSError Serialize(ParamArchive &ar);
};

struct LightEffectRT
{
  Ref<LightPoint> _light;

  // effect arguments as compiled expressions
  ExpressionCodeVector _position;
  ExpressionCodeColor _diffuse;
  ExpressionCodeColor _ambient;
  ExpressionCode _brightness;
  
  void Init(RString name, EVarSet evarSet);
  void Simulate(Entity *entity, float deltaT, SimulationImportance prec, const float *varValues, int varCount, bool lightOn = true);
};
TypeIsMovableZeroed(LightEffectRT)

struct ParticlesEffectRT
{
  CParticleSourceBase _source;

  // effect arguments as compiled expressions
  ExpressionCode _interval;
  ExpressionCode _circleRadius;
  ExpressionCodeVector _circleVelocity;

  ExpressionCode _timerPeriod;
  ExpressionCode _lifeTime;
  ExpressionCodeVector _position;
  ExpressionCodeVector _moveVelocity;
  ExpressionCode _rotationVelocity;
  ExpressionCode _weight;
  ExpressionCode _volume;
  ExpressionCode _rubbing;
  ExpressionCode _angle;
  ExpressionCode _sizeCoef;
  ExpressionCodeColor _colorCoef;
  ExpressionCode _animationSpeedCoef;
  ExpressionCode _randomDirectionPeriod;
  ExpressionCode _randomDirectionIntensity;

  ExpressionCode _lifeTimeVar;
  ExpressionCodeVector _positionVar;
  ExpressionCodeVector _moveVelocityVar;
  ExpressionCode _rotationVelocityVar;
  ExpressionCodeColor _colorVar;
  ExpressionCode _randomDirectionPeriodVar;
  ExpressionCode _randomDirectionIntensityVar;

  void Init(RString name, EVarSet evarSet);
  void Simulate(Entity *entity, float deltaT, SimulationImportance prec, const float *varValues, int varCount);
};
TypeContainsOLink(ParticlesEffectRT)

struct SoundEffectRT
{
  RefR<DynSoundObject> _sound;

  // TODO: effect arguments as compiled expressions
  void Init(ParamEntryPar cls);
  void Simulate(Entity *entity, float deltaT, SimulationImportance prec);
};
TypeIsMovableZeroed(SoundEffectRT)

/// encapsulation of different effect sources (smoke, fire, light, sound) - changing during life time
class EffectsSourceRT
{
protected:
  OLink(Entity) _entity; // attached to entity
  EVarSet _evarSet;
  AutoArray<LightEffectRT> _lights;
  AutoArray<ParticlesEffectRT> _particleSources;
  AutoArray<SoundEffectRT> _sounds;

public:
  EffectsSourceRT() {}
  EffectsSourceRT(ParamEntryPar cls, Entity *entity, EVarSet evarSet) { Init(cls, entity, evarSet); }
  /// load the parameters from config
  void Init(ParamEntryPar cls, Entity *entity, EVarSet evarSet);
  /// simulate all effects
  void Simulate(float deltaT, SimulationImportance prec, const float *varValues, int varCount, bool lightOn = true);
};
TypeContainsOLink(EffectsSourceRT)

class DestructionEffects : public Entity, public IExplosion
{
  typedef Entity base;

protected:
  OLink(Object) _object; // attached to object
  OLinkO(EntityAI) _owner;
  /// damage can be done by local effects only
  bool _doDamage;

  float _intensity;
  float _interval;
  float _fireIntensity;
  float _fireInterval;
  float _lifeTime;

  AutoArray<LightEffect> _lights;
  AutoArray<ParticlesEffect> _particleSources;
  AutoArray<SoundEffect> _sounds;
  AutoArray<DestructPosEffect> _destructPos;
  AutoArray<RuinsEffect> _ruins;
  AutoArray<DamageAroundEffect> _damageAround;

  /// OperCache was already invalidated after destruction effects took place
  bool _operCacheHandled;

public:
  DestructionEffects();
  DestructionEffects(
    const AutoArray<DestructionEffectType> &effectTypes, EntityAI *owner, Object *obj,
    float intensity, float density, float fireIntensity, float fireDentensity, float lifeTime,
    const Vector3 *defPos, bool doDamage
  );

  void Init(
    const AutoArray<DestructionEffectType> &effectTypes, EntityAI *owner, Object *obj,
    float intensity, float density, float fireIntensity, float fireDentensity, float lifeTime,
    const Vector3 *defPos, bool doDamage
  );

  static void ComputeDestructionEffectsPars(float &fuelAmount, float &fireIntensity, float &objSize, float &intensity, float &lifeTime, float &fireInterval, float &interval);
  
  virtual void Simulate(float deltaT, SimulationImportance prec);

  /// spawn ruins (handle processing of _ruins)
  bool SpawnRuins(bool &finished, bool async=true);

  /// perform immediately what Simulate would do when living long enough
  void SimulateResults();

  virtual void DoResults();
  /// we may be required to undo any long-term effects we started, like ruins
  virtual void UndoResults();

  virtual IExplosion *GetExplosionInterface() {return this;}
  virtual void SimulateExplosion();

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  static DestructionEffects *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx);

  USE_CASTING(base)
};

class Explosion: public Entity
{
  typedef Entity base;

/*
  //SoundPars _soundPars;
  float _exSize;
  WeaponCloudsSource _fire;
  bool _soundDone;
  Ref<LightPoint> _light;
  float _minLightTime;
*/
  EffectsSource _effects;

public:
  Explosion(LODShape *shape, RString effectsName, Entity *owner=NULL, float duration = 1);
  ~Explosion();

  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  void Simulate( float deltaT, SimulationImportance prec );
  void Sound( bool inside, float deltaT );

  virtual LSError Serialize(ParamArchive &ar);
};

class Crater : public Smoke, public TLinkBidirD
{
  typedef Smoke base;

protected:
  float _size;
  RString _effectsType;
  float _effectsTimeToLive;

  EffectsSource _effects;

#if _VBS3_CRATERS_DEFORM_TERRAIN
  //! Original shape - when aligning, we take this shape into consideration
  Ref<LODShape> _originalShape;
  //! Scale of the object set in the constructor - the shape points will be transformed using it
  float _desiredScale;
#endif

#if _VBS3_CRATERS_BIAS
  //! Bias of the crater - this value can vary among craters and helps to solve z-fighting artifact between craters
  int _bias;
#endif

public:
  //! Constructor
  Crater(const Matrix4 &transform, LODShape *shape, const EntityType *type,
    RString effectsType, EffectVariables &evars, float effectsTimeToLive, float timeToLive=20.0f, float size=1.0f);
  //! Destructor
  ~Crater();
#if _VBS3_CRATERS_LATEINIT
  //! Virtual method
  virtual bool HasRoadway() const {return true;}
  //! Virtual method
  virtual void LateInit();
#endif
  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  void Simulate( float deltaT, SimulationImportance prec );

  virtual AnimationStyle IsAnimated( int level ) const {return NotAnimated;} // appearance changed with Animate
  virtual bool CanObjDrawAsTask(int level) const {return false;} // tasking not possible, uses splitting
  virtual bool CanObjDrawShadowAsTask(int level) const {return false;} // tasking not possible, uses splitting
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const {return false;}

  //! Virtual method
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2) {} // no texture animation
  //! Virtual method
  void Deanimate(int level, bool setEngineStuff) {}
#if _VBS3_CRATERS_DEFORM_TERRAIN
  virtual int PassNum(int lod) {return 1;};
  virtual bool IsGroundAligned() const {return true;}
  virtual float DesiredScale() const {return _desiredScale;}
  virtual void AlignWithGround();
#endif
#if _VBS3_CRATERS_BIAS
  //! Virtual method
  virtual int Bias() const;
#endif

  virtual LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  static Crater *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx);

  //  USE_FAST_ALLOCATOR;
  USE_CASTING(base)

protected:
  void Init(RString effectsType, EffectVariables &evars, float effectsTimeToLive, float timeToLive, float size);
};

class CraterOnVehicle: public Crater, public AttachedOnVehicle
{
  typedef Crater base;

public:
  CraterOnVehicle(
    LODShape *shape, const EntityType *type, RString effectsType, EffectVariables &evars,
    float effectsTimeToLive, float timeToLive, float size,
    Object *vehicle, Vector3Par position, Vector3Par direction);

  CraterOnVehicle(LODShape *shape, const EntityType *type, EffectVariables &evars); // used for serialization only

  void UpdatePosition();

  virtual LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  static CraterOnVehicle *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx);

  //  USE_FAST_ALLOCATOR;
  USE_CASTING(base)
};

class ProjectileDecalOnVehicle: public Entity
{
  typedef Entity base;

public:
  Matrix4 _localTrans;
  OLink(Object) _vehicle;
  SkeletonIndex _bone;
  float _depth;

  static float MIN_DEPTH;
  static float MAX_DEPTH;

public:
  ProjectileDecalOnVehicle(Object *vehicle, LODShape *shape, const EntityType *type, float hitDepth, 
    SkeletonIndex bone, Vector3Par hitPosLcl, Vector3Par hitDir, Vector3Par dirUp);

  virtual bool SimulationReady(SimulationImportance prec) const override {return true;}
  void Simulate( float deltaT, SimulationImportance prec ) override;

  Matrix4 GetFutureTrans(bool inWorldSpace = false) const;
};

#endif
