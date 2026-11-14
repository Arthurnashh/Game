#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CAMERA_HOLD_HPP
#define _CAMERA_HOLD_HPP

#include "vehicle.hpp"

struct CameraTarget
{
	OLink(Object) _target;
	Vector3 _pos;

	Vector3 GetPos() const;

	CameraTarget() :_target(NULL),_pos(VZero) {}
	CameraTarget(Object *obj) :_target(obj),_pos(VZero) {}
	CameraTarget(Vector3Par pos) :_target(NULL),_pos(pos) {}

	Vector3 PositionAbsToRel(Vector3Val pos) const;
	Vector3 PositionRelToAbs(Vector3Val pos) const;

	LSError Serialize(ParamArchive &ar);
};

class CameraHolder: public Vehicle
{
	typedef Entity base;

protected:
	bool _manual;
	bool _comfyCamera;
	float m_fDebMouseSpeed;

  struct Pars: public SerializeClass
  {
	  Vector3 _camPos;
    Vector3 _camDir;
	  float _camFov,_camFovMin,_camFovMax;
    //@{ parameters determining camera len focus, blur = 0 means no blur
    float _camFocusDistance,_camFocusBlur;
    //@}
	  CameraTarget _camTgt;
	  
	  LSError Serialize(ParamArchive &ar);
  	//void LoadDefaultValues(ParamArchive &ar);
	  
	  Pars();
  };
  
  
	// note: target need not be committed
	struct Set: public SerializeClass
	{ // what has been set since last commit
		bool _camPos;
    bool _camDir;
		bool _camFov,_camFovMinMax;
		bool _camTgt;
    bool _focus;
		Set()
		{
			_camPos=false;
      _camDir=false;
			_camFov=false,_camFovMinMax=false;
			_camTgt=false;
      _focus=false;
		}

	  LSError Serialize(ParamArchive &ar);
	};
	
  float _bank;

  Pars _setPars;
  Pars _preparedPars;
	/// dirty flags telling which parts need to be set
	Set _set;
	Set _prep;

public:
	CameraHolder(LODShape *shape, const VehicleNonAIType *type, const CreateObjectId &id)
  :base(shape,type,id),_manual(false),_comfyCamera(false),m_fDebMouseSpeed(1.0f)
  {
  }

  ~CameraHolder() {}

  // set destination
  virtual void SetPos(Vector3Par pos)
  {
    _setPars._camPos = pos;
    _set._camPos = true;
  }
  // set direction
  virtual void SetDir(Vector3Par dir)
  {
    _setPars._camDir = dir;
    _set._camDir = true;
  }
	virtual void SetFOV(float fov)
	{
		_setPars._camFov = fov;
    _set._camFov = true;
		SetFOVRange(fov,fov);
	}
  // set camera pars
	virtual void SetFOVRange(float minFov, float maxFov)
  {
    _setPars._camFovMin = minFov;
    _setPars._camFovMax = maxFov;
    _set._camFovMinMax = true;
  }
	virtual void SetFocus(float distance, float blur)
  {
    _setPars._camFocusDistance = distance;
    _setPars._camFocusBlur = blur;
    _set._focus = true;
  }
	virtual void SetTarget(Object *target)
  {
    _setPars._camTgt = target;
    _set._camTgt = true;
  }
	virtual void SetTarget(Vector3Par target)
  {
    _setPars._camTgt = target;
    _set._camTgt = true;
  }

	virtual void PreparePos(Vector3Par pos)
  {
    _preparedPars._camPos = pos;
    _prep._camPos = true;
  }
	virtual void PrepareFOV(float fov)
	{
		_preparedPars._camFov = fov;
    _prep._camFov = true;
		SetFOVRange(fov, fov);
	}
	virtual void PrepareFOVRange(float minFov, float maxFov)
  {
    _preparedPars._camFovMin = minFov;
    _preparedPars._camFovMax = maxFov;
    _prep._camFovMinMax = true;
  }
	virtual void PrepareFocus(float distance, float blur)
  {
    _preparedPars._camFocusDistance = distance;
    _preparedPars._camFocusBlur = blur;
    _prep._focus = true;
  }
	virtual void PrepareTarget(Object *target)
  {
    _preparedPars._camTgt = target;
    _prep._camTgt = true;
  }
	virtual void PrepareTarget(Vector3Par target)
  {
    _preparedPars._camTgt = target;
    _prep._camTgt = true;
  }

	const CameraTarget &GetTarget() const {return _setPars._camTgt;}
	virtual void Command(RString mode); // camera dependent special command
	virtual void Commit(float time) = 0; // commit all deferred settings
	virtual void CommitPrepared(float time) = 0; // commit all prepared settings
	virtual void PreloadPrepared(float time) {} // commit all deferred settings
	
  virtual void SetBank(float bank) {_bank = bank *H_PI/180;} // set camera bank

	virtual void SetManual(bool manual) {_manual = manual;}
	virtual bool GetManual() const {return _manual;}

	virtual void SetComfy(bool comfy) {_comfyCamera = comfy;}
	virtual bool GetComfy() const {return _comfyCamera;}
	
	virtual bool GetCommitted() const {return true;}
  virtual bool GetPreloaded() const {return true;}

	LSError Serialize(ParamArchive &ar);

  virtual Object *GetFocus() const {return NULL;}
  
	USE_CASTING(base)
};


class CameraVehicle: public CameraHolder
{
	typedef CameraHolder base;

protected:
	Vector3 _movePos; // where should I move
  Matrix3 _wantedOrient;
	CameraTarget _oldTarget;
	CameraTarget _target;
	//@{ target values
	float _minFovT,_maxFovT; // target values
	float _focusBlurT,_focusDistanceT;
	//@}

  //@{ current values
	float _minFov,_maxFov; // negative means invalid
	float _focusBlur,_focusDistance;
	//@}

	Time _oldTargetTime;
	Time _targetTime;
	Time _movePosTime; // when target should be reached
  Time _orientationTime;
	Time _minMaxFovTime;
	Time _focusTime;

	Time _timeCommitted;
  Time _startTime;

  //@{ parameters for preloading using camPrepare/camPreload
  Vector3 _preloadPos;
  Vector3 _preloadDir;
  float _preloadFov;
  Time _preloadTimeout;
  /// true once preloading finished
  bool _preloadDone;
  //@}

	mutable float _lastFov;
	mutable Vector3 _lastTgtPos;

  float _accel;
  float _accelTime;
  
	bool _inertia; // manual camera simulation 
	bool _crossHairs;

  Ref<Texture> _crossHairsTexture;

  float _orientTimeMult;

#if _VBS2
  // direction to look if camera is attached
    // if VZero, camera will look at vehicle it is attached to
  Vector3 _lookDirAttached;
  // if true, camera bank will match the bank of the vehicle it is attached to
  bool _bankIfAttached;
#endif

public:
	Vector3 GetTarget() const;

	CameraVehicle(const EntityType *type);
  ~CameraVehicle() {}

#if _VBS2
  virtual void CalcAttachedPosition(float deltaT);
  void SetAttachedCameraLookDir(Vector3 lookDir) {_lookDirAttached = lookDir;}
  void SetAttachedCameraBank(bool bank) {_bankIfAttached = bank;}
  void GetFOV(float &minFov, float &maxFov) {minFov = _minFov; maxFov = _maxFov;}
#endif

  virtual Object *GetFocus() const {return _target._target;}
	virtual void ResetTargets();

	virtual void Simulate(float deltaT, SimulationImportance prec);
	virtual void SimulateDebugCamera(float deltaT);
  virtual bool SimulationReady(SimulationImportance prec) const {return true;}

	virtual bool OcclusionFire() const {return false;}
	virtual bool OcclusionView() const {return false;}

	virtual Visible VisibleStyle() const {return ObjInvisible;}

	// camera effect parameters
	virtual void DrawCameraCockpit();
	virtual bool CameraAutoTerminate() {return _manual;}

	virtual float OutsideCameraDistance(CameraType camType ) const {return 0.0f;}
	virtual float CamEffectFOV() const;
	virtual void CamEffectFocus(float &distance, float &blur) const;

  void StartFrame() {} // start frame - used for motion blur

  virtual void Command(RString mode); // camera dependent special command
	virtual void Commit(float time); // commit all settings
  virtual void CommitPrepared(float time); // commit all prepared settings
  virtual void PreloadPrepared(float time);
  virtual bool GetPreloaded() const;
	virtual bool GetCommitted() const;

  virtual LSError Serialize(ParamArchive &ar);
};

class CameraConstruct : public CameraHolder
{
  //camera with restricted movement used for warfare construction menu (COIN) 
  typedef CameraHolder base;

private:
  //warfare base center
  Vector3 _constructCenter;
  //max camera distance form _constructCenter
  float _radius;
  //max height above land
  float _maxHAL;

  CameraTarget _oldTarget;
  CameraTarget _target;
 
  Time _oldTargetTime;
  Time _targetTime;

  //object rotation - step Pi/4 
  float _lastRotateChange;
  Vector3 _lastDirection;

protected:
  Vector3 _movePos; // where should I move
  //@{ target values
  float _minFovT,_maxFovT; // target values
  float _focusBlurT,_focusDistanceT;
  //@}

  //@{ current values
  float _minFov,_maxFov; // negative means invalid
  float _focusBlur,_focusDistance;
  //@}

  Time _movePosTime; // when target should be reached
  Time _minMaxFovTime;
  Time _focusTime;

  Time _timeCommitted;
  Time _startTime;

  //@{ parameters for preloading using camPrepare/camPreload
  Vector3 _preloadPos;
  Vector3 _preloadDir;
  float _preloadFov;
  Time _preloadTimeout;
  /// true once preloading finished
  bool _preloadDone;
  //@}

  mutable float _lastFov;
  mutable Vector3 _lastTgtPos;

  float _accel;
  float _accelTime;

  bool _inertia; // manual camera simulation 
  bool _crossHairs;

public:
  CameraConstruct(const EntityType *type);
  ~CameraConstruct() {}

  virtual void Simulate(float deltaT, SimulationImportance prec);
  virtual bool SimulationReady(SimulationImportance prec) const {return true;}
  // set destination
  virtual void SetPos(Vector3Par pos) 
  {
    _setPars._camPos = pos;
    _set._camPos = true;
  }

  virtual void Commit(float time); // commit all settings
  virtual void CommitPrepared(float time); // commit all prepared settings

  virtual bool CameraAutoTerminate() {return _manual;}

  virtual bool GetPreloaded() const {return _preloadDone;}
  virtual bool GetCommitted() const;

  virtual bool EnabledCommanding() {return false;}

  virtual	LSError Serialize(ParamArchive &ar);

  //defines camera restriction zone
  void SetParams(Vector3 center, float radius, float maxHal)
  {
    _radius = radius;
    _maxHAL = maxHal;
    _constructCenter = center;
  }

  USE_CASTING(base)
};

#endif
