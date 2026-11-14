#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SEAGULL_HPP
#define _SEAGULL_HPP

#include "vehicle.hpp"
#include "animation.hpp"
#include "lights.hpp"
#include "paramFileExt.hpp"

class SeaGullAuto;

#include "cameraHold.hpp"
#include "motion.hpp"

class AIBrain;
class SeaGullType;

class SeaGullVisualState : public EntityVisualState
{
  typedef EntityVisualState base;
  friend class SeaGull;
  friend class SeaGullAuto;

protected:
  MovesVisualState _moves;
  
  void Interpolate(Object *obj, const SeaGullVisualState & t1state, float t, SeaGullVisualState& interpolatedResult) const;
public:
  SeaGullVisualState(const SeaGullType & type);
  virtual void Interpolate(Object *obj, const ObjectVisualState & t1state, float t, ObjectVisualState& interpolatedResult) const
  {
    return Interpolate(obj,static_cast_checked<const SeaGullVisualState &>(t1state),t,static_cast_checked<SeaGullVisualState &>(interpolatedResult));
  }
  
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const SeaGullVisualState &>(src);}
  virtual SeaGullVisualState *Clone() const {return new SeaGullVisualState(*this);}
};

class SeaGullType: public EntityType
{
  friend class SeaGull;
  friend class SeaGullAuto;
  
  typedef EntityType base;

  //@{ life simulation parameters
  float _minHeight;
  float _avgHeight;
  float _maxHeight;
  float _maxSpeed;
  float _minSpeed;
  float _straightDistance;
  float _acceleration;
  float _turning;
  float _precision;
  Vector3 _fricCoef2;
  Vector3 _fricCoef1;
  Vector3 _fricCoef0;
  //@}
	SoundPars _singSound;
	SoundPars _flySound;

  Ref<MovesTypeBase> _movesType;
  bool _canBeShot;
  
public:
  explicit SeaGullType(ParamEntryPar param);
  ~SeaGullType();
  
  void Load(ParamEntryPar param, const char *shapeName);
  void InitShape();
  void DeinitShape();

  virtual Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new SeaGullVisualState(*this); }

  bool AbstractOnly() const {return false;}

  static FSMEntityConditionFunc *CreateEntityConditionFunc(RString name);
  static FSMEntityActionFunc *CreateEntityActionInFunc(RString name);
  static FSMEntityActionFunc *CreateEntityActionOutFunc(RString name);
  virtual MapNameToConditionFunc *GetCreateEntityConditionFunc() const {return &CreateEntityConditionFunc;}
  virtual MapNameToActionFunc *GetCreateEntityActionInFunc() const {return &CreateEntityActionInFunc;}
  virtual MapNameToActionFunc *GetCreateEntityActionOutFunc() const {return &CreateEntityActionOutFunc;}
};

#if _DEBUG
  #define ENABLE_BIRD_TRACKING 0
#endif

#if ENABLE_BIRD_TRACKING
  #include "smokes.hpp"
#endif

enum SeagullFSMFlags
{
  FSM_NO_BACKWARDS = 1,
  FSM_LANDED = 2,
  FSM_BREAKING = 4,
  FSM_LANDING = 8
};

//! used for respawning, and as ambient entity

class SeaGull: public CameraHolder
{
	typedef CameraHolder base;

protected:
	RefAbstractWave _sound;
	RefAbstractWave _flySound;

#if ENABLE_BIRD_TRACKING
	CloudletSource _cloudlets;
#endif
	
	float _wingPhase;
	float _wingSpeed;
	float _wingBase; // which kind of animation - landed/winging
	Time _nextCreek;

	// rotor force is wing speed
	float _rpm,_rpmWanted; // landing control
	float _mainRotor,_mainRotorWanted; // main rotor thrust
	float _cyclicForwardWanted;
	float _cyclicAsideWanted; // main rotor thrust direction
	float _wingDive,_wingDiveWanted; // use wings to control forward movement
	float _thrust,_thrustWanted;
  mutable int   _fsmFlags; //TODO: shouldn't be mutable, FSMMoveCompletedVertical shouldn't change this value, create special FSMLand action instead
  float _breakingFactor;  //when FSMBreaking, then friction is multiplied by this value

	bool _landContact;
  Object::Zoom _zoom;

public:
	SeaGull(const EntityType *type);
	~SeaGull();
	
  const SeaGullType *Type() const {return static_cast<const SeaGullType *>(GetNonAIType());}
  const MovesTypeBase *GetMovesType() const {return Type()->_movesType;}

  /// @{ Visual state
  SeaGullVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  const char *GetControllerScheme() const {return "Aircraft";}

  virtual bool CanObjDrawAsTask(int level) const;
  virtual bool CanObjDrawShadowAsTask(int level) const;
	AnimationStyle IsAnimated(int level) const; // appearance changed with Animate
	void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
	void Deanimate(int level, bool setEngineStuff);

  virtual bool RemoteStateSupported() const {return true;}
	void Simulate(float deltaT, SimulationImportance prec);
	void Sound(bool inside, float deltaT);
	void UnloadSound();

  // Collision Detection
  SeaGullVisualState SimulateMove(float deltaT, SimulationImportance prec );

  float ValidatePotentialState(const SeaGullVisualState& moveTrans, Vector3 *repairMove=NULL);

	bool OcclusionFire() const;
	bool OcclusionView() const {return false;}

	float TrackingSpeed() const {return 100.0f;}
	float OutsideCameraDistance(CameraType camType) const {return 3.0f;}
	Vector3 ExternalCameraPosition(CameraType camType) const;

	void QuickStart();

	// no load/save
	bool MustBeSaved() const {return false;}
  // zooming
  Object::Zoom *GetZoom() {return &_zoom;}
  virtual float GetCameraFOV() const {return _zoom._camFOV;}

  //compute body friction using config air friction parameters
  Vector3 BodyFriction(Vector3Val oSpeed);

	USE_CASTING(base)
};

//! high level controled - used for re-spawning and special effects

#ifndef DECL_ENUM_SEAGULL_AUTOPILOT_STATE
#define DECL_ENUM_SEAGULL_AUTOPILOT_STATE
DECL_ENUM(SeaGullAutopilotState)
#endif

// FSM functions factories

#define SEAGULL_FSM_CONDITIONS(XX) \
  XX(moveCompleted, SeaGullAuto, FSMMoveCompleted) \
  XX(waitCompleted, SeaGullAuto, FSMWaitCompleted) \
  XX(timeElapsed, SeaGullAuto, FSMTimeElapsed) \
  XX(moveCompletedVertical, SeaGullAuto, FSMMoveCompletedVertical)

#define SEAGULL_FSM_ACTIONS(XX) \
  XX(randomMove, SeaGullAuto, FSMRandomMove) \
  XX(randomMoveLand, SeaGullAuto, FSMRandomMoveLand) \
  XX(stop, SeaGullAuto, FSMStop) \
  XX(relativeMove, SeaGullAuto, FSMRelativeMove) \
  XX(wait, SeaGullAuto, FSMWait) \
  XX(switchAction, SeaGullAuto, FSMSwitchAction) \
  XX(setTimer, SeaGullAuto, FSMSetTimer) \
  XX(setNoBackwards, SeaGullAuto, FSMSetNoBackwards) \
  XX(break, SeaGullAuto, FSMBreak) \
  XX(land, SeaGullAuto, FSMLand)

class SeaGullAuto: public SeaGull
{
	typedef SeaGull base;

protected:
	Vector3 _pilotSpeed;
	float _pilotHeading;
	float _pilotHeight;
	//float _dirCompensate;  // how much we compensate for estimated change
	bool _pilotHeadingSet;
	// esp. when landing
  //head looking around parameters
  float _headXRotWanted, _headYRotWanted, _headYRotWantedCont;
  float _headXRot, _headYRot;
  float _mouseDive;  //mouse fwd/back affect _wingDive
  //frame identifier (used to call KeyboardPilots only once per frame) the counterpart is member of struct Input
  mutable int _inputFrameID;

	// helpers for keyboard control
	bool _pressedForward,_pressedBack; // recognize fast speed-up, fast brake
	bool _pressedUp,_pressedDown;

	//Ref<SeaGullPilot> _pilot;
	Time _lastPilotTime; // avoid calculating pilot too often

	SeaGullAutopilotState _state;

  /// id of attached player
  int _player;

  Moves _moves;
  Time _waitUntil;
  Time _timer;
	
public:
	SeaGullAuto(const EntityType *type);

  int GetPlayer() const {return _player;}
  void SetPlayer(int player) {_player = player;}

	void Simulate(float deltaT, SimulationImportance prec);
	void KeyboardPilot(AIBrain *unit, float deltaT );

  bool KeyboardUsedInFrame() const;
  
	void AvoidGround(float minHeight);
	void Autopilot(Vector3Par target, Vector3Par tgtSpeed, Vector3Par direction, Vector3Par speed);
	void ResetAutopilot();

	void DrawDiags();

  bool PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level);

	void MakeLanded();
	void MakeAirborne( float height , bool playerIn);
  void SetAmbient(IAmbLifeArea *area, Vector3Par camPos);
  bool IsPassable() const;
  bool HasGeometry() const;

	bool IsVirtualX(CameraType camType) const;
	
	//! moved from obsolete SeaGullPilotCamera
	virtual void CamControl(float deltaT);

	// CameraHolder implementation
	virtual void Command(RString mode);
	virtual void Commit(float time);
	virtual void CommitPrepared(float time);

  // Head looking around
  bool MoveHead(float deltaT, float aimX, float aimY);
  virtual Matrix4 InsideCamera( CameraType camType ) const;
  Matrix4 InternalCameraTransform(Matrix4 &base, CameraType camType) const;
  Vector3 GetCameraDirection( CameraType camType ) const;

  // FSM functions
  SEAGULL_FSM_CONDITIONS(DECLARE_ENTITY_FSM_CONDITION)
  SEAGULL_FSM_ACTIONS(DECLARE_ENTITY_FSM_ACTION)

	// Multiplayer support
	DECLARE_NETWORK_OBJECT
	static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  static SeaGullAuto *CreateObject(NetworkMessageContext &ctx);
	TMError TransferMsg(NetworkMessageContext &ctx);
	float CalculateError(NetworkMessageContextWithError &ctx);

	USE_CASTING(base)
private:
  void RandomMoveFunc(const float *params, int paramsCount, bool land);
  void SwitchMove(int action=0);  //switch to default move when called without parameters
  ///test and move out, if necessary. Returns true, when moving out.
  bool UpdateMoveOut(); //
};

#endif
