// Poseidon - camera holders
// (C) 2000, SUMA
#include "wpch.hpp"

#include "cameraHold.hpp"
#include <El/ParamFile/paramFile.hpp>
#include "paramArchiveExt.hpp"
#include "gameStateExt.hpp"
#include "global.hpp"
#include "keyInput.hpp"
#include "camera.hpp"
#include "landscape.hpp"
#include "engine.hpp"
#include "world.hpp"
#include "scene.hpp"
#include "dikCodes.h"
#include "txtPreload.hpp"
#include "gameDirs.hpp"
#include <El/Clipboard/clipboard.hpp>
#include <Es/Common/win.h>

#if _VBS3
  #include "HLA\VBSVisuals.hpp"
#endif

DEFINE_CASTING(CameraHolder)

void CameraHolder::Command(RString mode)
{
  // camera dependent special command
  if (!strcmpi(mode, "manual on"))
    SetManual(true);
  else if (!strcmpi(mode,"manual off"))
    SetManual(false);
  else if (!strcmpi(mode, "comfy on"))
    SetComfy(true);
  else if (!strcmpi(mode,"comfy off"))
    SetComfy(false);
}

Vector3 CameraTarget::GetPos() const
{
  if (_target)
    return _target->EyePosition(_target->RenderVisualState());
  else if (_pos.SquareSize() > 0.5f)
    return _pos;
  else
    return VZero;
}

Vector3 CameraTarget::PositionAbsToRel(Vector3Val pos) const
{
  if (!_target)
    return pos - _pos;
  Matrix4 toAbs = _target->RenderVisualState().ModelToWorld();
  toAbs.SetDirectionAndUp(toAbs.Direction(),VUp);
  // make direction up pointing up
  return toAbs.InverseScaled().FastTransform(pos);
}

Vector3 CameraTarget::PositionRelToAbs(Vector3Val pos) const
{
  if (!_target)
    return pos + _pos;

  Matrix4 toAbs = _target->RenderVisualState().ModelToWorld();
  toAbs.SetDirectionAndUp(toAbs.Direction(),VUp);
  // make direction up pointing up
  return toAbs.FastTransform(pos);
}

CameraHolder::Pars::Pars()
:_camPos(VZero),
_camFov(0.7),_camFovMin(1.5),_camFovMax(0.07),
_camFocusDistance(-1),_camFocusBlur(1)
{}

LSError CameraVehicle::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  CHECK(_target.Serialize(ar));
  CHECK(_oldTarget.Serialize(ar));
  CHECK(::Serialize(ar, "movePos", _movePos, 1, VZero ));
  CHECK(::Serialize(ar, "wantedOrient", _wantedOrient, 1, M3IdentityP));

  CHECK(ar.Serialize("minFovT", _minFovT, 1, 0));
  CHECK(ar.Serialize("maxFovT", _maxFovT, 1, 0));
  CHECK(ar.Serialize("focusBlurT", _focusBlurT, 1, 0));
  CHECK(ar.Serialize("focusDistanceT", _focusDistanceT, 1, 0));

  CHECK(ar.Serialize("minFov", _minFov, 1, 0));
  CHECK(ar.Serialize("maxFov", _maxFov, 1, 0));
  CHECK(ar.Serialize("focusBlur", _focusBlur, 1, 0));
  CHECK(ar.Serialize("focusDistance", _focusDistance, 1, 0));

  CHECK(::Serialize(ar, "oldTargetTime", _oldTargetTime, 1));
  CHECK(::Serialize(ar, "targetTime", _targetTime, 1));
  CHECK(::Serialize(ar, "movePosTime", _movePosTime, 1));
  CHECK(::Serialize(ar, "orientationTime", _orientationTime, 1));
  CHECK(::Serialize(ar, "minMaxFovTime", _minMaxFovTime, 1));
  CHECK(::Serialize(ar, "focusTime", _focusTime, 1));
  CHECK(::Serialize(ar, "timeCommitted", _timeCommitted, 1));
  CHECK(::Serialize(ar, "startTime", _startTime, 1));

  CHECK(::Serialize(ar, "preloadPos", _preloadPos, 1, VZero));
  CHECK(::Serialize(ar, "preloadDir", _preloadDir, 1, VZero));

  CHECK(ar.Serialize("preloadFov", _preloadFov, 1, 0));
  CHECK(::Serialize(ar, "preloadTimeout", _preloadTimeout, 1));
  CHECK(ar.Serialize("preloadDone", _preloadDone, 1, false));
  
  CHECK(ar.Serialize("lastFov", _lastFov, 1, 0));

  CHECK(::Serialize(ar, "lastTgtPos", _lastTgtPos, 1));

  CHECK(ar.Serialize("accel", _accel, 1, 0));
  CHECK(ar.Serialize("accelTime", _accelTime, 1, 0));

  CHECK(ar.Serialize("inertia", _inertia, 1, false))
  CHECK(ar.Serialize("crossHairs", _crossHairs, false));

  CHECK(ar.Serialize("orientTimeMult", _orientTimeMult, 1, 0));

  return LSOK;
}

LSError CameraTarget::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("target", _target, 1))
  CHECK(::Serialize(ar, "pos", _pos, 1, VZero ))
  return LSOK;
} 

LSError CameraHolder::Pars::Serialize(ParamArchive &ar)
{
  CHECK(::Serialize(ar, "camPos", _camPos, 1, VZero ))
  CHECK(::Serialize(ar, "camDir", _camDir, 1, VForward ))
  CHECK(ar.Serialize("camFov", _camFov, 1, 0 ))
  CHECK(ar.Serialize("camFovMin", _camFovMin, 1, 0 ))
  CHECK(ar.Serialize("camFovMax", _camFovMax, 1, 0 ))
  CHECK(ar.Serialize("camFocusDistance", _camFocusDistance, 1, -1 ))
  CHECK(ar.Serialize("camFocusBlur", _camFocusBlur, 1, 1 ))
  return LSOK;
}

LSError CameraHolder::Set::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("camPos", _camPos, 1, false ))
  CHECK(ar.Serialize("camDir", _camDir, 1, false ))
  CHECK(ar.Serialize("camFov", _camFov, 1, false ))
  CHECK(ar.Serialize("camFovMinMax", _camFovMinMax, 1, false ))
  CHECK(ar.Serialize("camFocus", _focus, 1, false ))
  return LSOK;
}

LSError CameraHolder::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("manual", _manual, 1, false))
  CHECK(ar.Serialize("setPars",_setPars,1));
  CHECK(ar.Serialize("preparedPars",_preparedPars,1));
  CHECK(ar.Serialize("set",_set,1));
  CHECK(ar.Serialize("prep",_prep,1));
  //CHECK(_setPars._camTgt.Serialize(ar))
  return LSOK;
}

LSError CameraConstruct::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  CHECK(ar.Serialize("radiusConstr", _radius,1, 50))
  CHECK(ar.Serialize("maxhalConstr",_maxHAL,1,20))
  CHECK(::Serialize(ar, "camCenter", _constructCenter, 1, VZero ))

  CHECK(ar.Serialize("focusBlurConstr", _focusBlur,1, 1))
  CHECK(ar.Serialize("focusDistanceConstr",_focusDistance,1,-1))

  CHECK(ar.Serialize("crossHairsConstr", _crossHairs, 1, false))
  CHECK(ar.Serialize("inertiaConstr", _inertia, 1, false))

  CHECK(_target.Serialize(ar))
  return LSOK;
}

CameraVehicle::CameraVehicle(const EntityType *type)
:base(NULL,type,CreateObjectId()),
_inertia(false), // manual camera simulation 
_crossHairs(true)
{
  SetTimeOffset(0);
  SetSimulationPrecision(0,0);
   // set all target properties to invalid values
  _movePos = VZero;
  _minFovT = _maxFovT = -1;
  // set all times to infinite future
  _movePosTime=TIME_MAX;
  _minMaxFovTime=TIME_MAX;
  _targetTime = TIME_MAX;
  _oldTargetTime = TIME_MAX;
  _focusTime = TIME_MAX;
  _focusDistanceT =0;
  _focusBlurT =0;
  _preloadTimeout = TIME_MIN;
  _preloadDone = true;
  // all committed
  _timeCommitted = TIME_MIN;
   // set all actual properties to default values
  _minFov=0.7,_maxFov=0.7;
  _lastFov=0.7;
  _lastTgtPos=VZero;
  _wantedOrient=M3IdentityP;
  _focusBlur = 1,_focusDistance =-1;

  RString name = (*type->_par) >> "crossHairs";
  RString GetPictureName( RString baseName );
  _crossHairsTexture = GlobLoadTexture(GetPictureName(name));
  _bank = 0;
#if _VBS2
  _lookDirAttached = VZero;
  _bankIfAttached = true;
#endif
}

// camera effect parameters
void CameraVehicle::DrawCameraCockpit()
{
  if( !_manual ) return; // nothing to draw
  if( !_crossHairs ) return;

  // check if we are drawing from inside
  // assume yes - we are manual

  {
    // draw circle around locked target
    Color lockColor(1,1,0,1);
    Vector3 lockPos = _target._pos;
    if( _target._target )
    {
      lockColor = Color(1,0,0,1);
      lockPos = _target._target->EyePosition(_target._target->RenderVisualState());
    }
    float ls2 = lockPos.SquareSize();
    if( ls2>0.5f && ls2<1e9f )
    {
      PackedColor color=PackedColor(lockColor);
      float w = GEngine->WidthBB();
      float h = GEngine->HeightBB();

      const Camera &camera = *GScene->GetCamera();
      Matrix4Val camInvTransform=camera.GetInvTransform();

      Point3 pos = camInvTransform.FastTransform(lockPos);
      if (pos.Z() >= 0)
      {
        float invZ = 1.0f / pos.Z();
        float cx = pos.X() * invZ * camera.InvLeft();
        float cy = - pos.Y() * invZ * camera.InvTop();

        float wScreen = toInt(w*0.05f);
        float hScreen = toInt(h*(0.05f*4/3));
        float xScreen = w*cx*0.5f+w*0.5f-wScreen*0.5f;
        float yScreen = h*cy*0.5f+h*0.5f-hScreen*0.5f;
        Texture *texture = GScene->Preloaded(CursorTarget);
        MipInfo mip=GLOB_ENGINE->TextBank()->UseMipmap(texture,0,0);
        GLOB_ENGINE->Draw2D(mip, PackedBlack, Rect2DAbs(xScreen+1, yScreen+1, wScreen, hScreen));
        GLOB_ENGINE->Draw2D(mip, color, Rect2DAbs(xScreen, yScreen, wScreen, hScreen));
      }
    }
  }
  {
    // draw 2D cursor in screen center
    float w = GEngine->WidthBB();
    float h = GEngine->HeightBB();

    float wScreen = toInt(w*0.05f);
    float hScreen = toInt(h*(0.05f*4/3));
    float xScreen = toInt(w*0.5f)-wScreen*0.5f;
    float yScreen = toInt(h*0.5f)-hScreen*0.5f;
    // Texture *texture = GScene->Preloaded(CursorAim);
    // MipInfo mip=GLOB_ENGINE->TextBank()->UseMipmap(texture,0,0);_crossHairsTexture
    MipInfo mip=GLOB_ENGINE->TextBank()->UseMipmap(_crossHairsTexture,0,0);
    PackedColor color=PackedWhite;
    GLOB_ENGINE->Draw2D(mip, PackedBlack, Rect2DAbs(xScreen+1, yScreen+1, wScreen, hScreen));
    GLOB_ENGINE->Draw2D(mip, color, Rect2DAbs(xScreen, yScreen, wScreen, hScreen));
  }
}

Vector3 CameraVehicle::GetTarget() const
{
  if (GetManual())
    return _target.GetPos();
  Vector3 nTgt = _target.GetPos();
  Vector3 oTgt = _oldTarget.GetPos();
  if( oTgt.SquareSize()<0.5 )
    return nTgt;
  if( nTgt.SquareSize()<0.5 )
    return oTgt;
  // both targets - interpolate
  if( Glob.time>=_targetTime )
    return nTgt;
  Vector3 oRel = oTgt-RenderVisualState().Position();
  Vector3 nRel = nTgt-RenderVisualState().Position();
  Vector3 oDir = oRel.Normalized();
  Vector3 nDir = nRel.Normalized();
  float oDist = oDir*oRel;
  float nDist = nDir*nRel;

  float ipol = 1;
  if( _targetTime>_oldTargetTime )
    ipol = (Glob.time-_oldTargetTime)/(_targetTime-_oldTargetTime);
  saturate(ipol,0,1);
  // total time 
  // interpolate distance and direction separately
  // TODO: interpolate direction using quaternions
  Vector3 iDir = nDir*ipol + oDir*(1-ipol);
  float iDist = nDist*ipol + oDist*(1-ipol);
  return  iDir*iDist + RenderVisualState().Position();
}

void CameraVehicle::CamEffectFocus(float &distance, float &blur) const
{
  blur = _focusBlur;
  if (_focusDistance<0)
    distance = GWorld->AutoFocus(GetRenderVisualStateAge(),unconst_cast(this),RenderVisualState().Position(),GetTarget()-RenderVisualState().Position());
  else
    distance = _focusDistance;
}

float CameraVehicle::CamEffectFOV() const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
  return -1.0f;
#else
  // transition between old and new target

#if _VBS3
  // if we do not have a target return the fov.
  if(!_target._target)
    _lastFov = _minFov;
#endif

  Vector3 tPos = GetTarget();

  if( tPos.SquareSize()>=0.5 )
    _lastTgtPos=tPos;

  // calculate FOV based on target
  if( _lastTgtPos.SquareSize()>0.5 )
  {
    Vector3 norm = _lastTgtPos - FutureVisualStateRaw().Position();
    Vector3 dir = norm.Normalized();

  #if _VBS2
    if(GVBSVisuals.rteVisible && _attachedTo)
    {
      Vector3 aside = const_cast<CameraVehicle *>(this)->FutureVisualStateRaw().Orientation().DirectionAside();
      aside[1] = 0.0f;
      const_cast<CameraVehicle *>(this)->FutureVisualStateRaw().SetDirectionAndAside(dir, aside);  // bypass Protect
    }
    else
    {
      Vector3 up = VUp;
      if (_attachedTo && _bankIfAttached) 
        up = _attachedTo->RenderVisualState().DirectionUp();
      else
      {
        Matrix3 rotZ(MRotationZ,_bank);
        up = up * rotZ;
      }
      const_cast<CameraVehicle *>(this)->SetDirectionAndUp(dir, up);
    }
  #else
    const_cast<CameraVehicle *>(this)->FutureVisualStateRaw().SetDirectionAndUp(dir,VUp);  // bypass Protect
  #endif
    DoAssert(_minFov>=0.001 && _minFov<10);
    DoAssert(_maxFov>=0.001 && _maxFov<10);
    
    float invDist=(_lastTgtPos-RenderVisualState().Position()).InvSize();
    float fov=invDist*10;
    saturate(fov,_minFov,_maxFov);
    _lastFov=fov;
  }
  return _lastFov;
#endif
}

void ClipboardSaveText(QOStream &stream);

void CameraVehicle::ResetTargets()
{
  Vector3 tPos = FutureVisualState().Position()+FutureVisualState().Direction()*1e5; // virtual infinity
  _target = tPos;
  _oldTarget = NULL;
  _lastTgtPos = _target._pos;
}

#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>

void CameraVehicle::SimulateDebugCamera (float deltaT)
{
  if (_target.GetPos().SquareSize() < 0.5f)
    ResetTargets();     

  deltaT /= GWorld->GetAcceleratedTime();
  // manual controls
  // use basic controls to control movement

	float debStrafe = 0.0f;
	float debUpDown = 0.0f;

  float forward = 0.0f;
  float aside = 0.0f;
  float up = 0.0f;
  float expChange = 0.0f;
  float headSpeed = 0.0f;
  float diveSpeed = 0.0f;
  bool resetTargets = false;
  bool lockCamera = false;
  bool switchInertia = false;
  bool switchCrossHair = false;
  bool deleteCamera = false;

  float speed = GInput.GetAction(UABuldTurbo) * 7.0f + 1.0f;

  if (!_comfyCamera)
  {
    forward =
    (
      (GInput.GetAction(UABuldMoveForward) - GInput.GetAction(UABuldMoveBack)) * 4.0f * Input::MouseRangeFactor +
      (GInput.GetAction(UAMoveForward) -GInput.GetAction(UAMoveBack)) * Input::MouseRangeFactor
    ) * speed;
    aside =
    (
      (GInput.GetAction(UABuldMoveRight) - GInput.GetAction(UABuldMoveLeft)) * 4.0f * Input::MouseRangeFactor +
      (GInput.GetAction(UATurnRight) - GInput.GetAction(UATurnLeft)) * Input::MouseRangeFactor
    ) * speed;
    up = (GInput.GetAction(UABuldUp) - GInput.GetAction(UABuldDown)) * 4.0f * speed;
    expChange = GInput.GetAction(UABuldZoomOut) - GInput.GetAction(UABuldZoomIn);
    headSpeed = GInput.GetAction(UABuldLookLeft) - GInput.GetAction(UABuldLookRight);
    diveSpeed = GInput.GetAction(UABuldLookDown) - GInput.GetAction(UABuldLookUp);
  }
  else
  {
		static const float SpeedMulPercent = 0.15f;
		m_fDebMouseSpeed = floatMinMax(m_fDebMouseSpeed + m_fDebMouseSpeed*(GInput.mouseWheelUp - GInput.mouseWheelDown)*SpeedMulPercent, 0.001f, 1000.0f);

    float scale = m_fDebMouseSpeed;
    scale *= (GInput.keys[DIK_LCONTROL] || GInput.keys[DIK_RCONTROL]) ? 10.0f : 1.0f;
    scale *= (GInput.keys[DIK_LSHIFT]   || GInput.keys[DIK_RSHIFT]  ) ? 10.0f : 1.0f;
    scale *= (GInput.keys[DIK_LALT]     || GInput.keys[DIK_RALT]    ) ?  0.1f : 1.0f;

    static const float base_go = 5.0f;
    const float go = GInput.mouseL * base_go - GInput.mouseR * base_go;
    const float mouse_forward = scale * go * Input::MouseRangeFactor;
		float finalScale = scale * base_go * Input::MouseRangeFactor;
    const float kb_forward = (GInput.GetAction(UAMoveForward) - GInput.GetAction(UAMoveBack))*finalScale;
    forward = mouse_forward + kb_forward;

		debStrafe = (GInput.GetAction(UATurnRight) - GInput.GetAction(UATurnLeft))*finalScale;
		debUpDown = (GInput.GetAction(UAMoveUp) - GInput.GetAction(UAMoveDown))*finalScale;

    const float lookX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimHeadLeft, UAAimHeadRight);
    const float lookY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UAAimHeadDown, UAAimHeadUp);
    const float rotXWanted = lookY * deltaT * 10.0f;
    const float rotYWanted = lookX * deltaT * 10.0f;
    diveSpeed = rotXWanted;
    headSpeed = rotYWanted;

    expChange = GInput.GetAction(UABuldZoomOut) - GInput.GetAction(UABuldZoomIn);
  }

  if (GInput.GetActionToDo(UABuldFreeLook))
  {
    resetTargets = true;
    _lastFov = 0.7f; //reset zoom
  }
  if (GInput.GetActionToDo(UAToggleWeapons))
    lockCamera = true;
  if (GInput.keysToDo[DIK_DIVIDE])
  {
    lockCamera = true;
    GInput.keysToDo[DIK_DIVIDE] = false;
  }
  if (GInput.keysToDo[DIK_DELETE])
  {
    switchInertia = true;
    GInput.keysToDo[DIK_DELETE] = false;
  }
  if (GInput.GetActionToDo(UAHeadlights))
    switchCrossHair = true;
  if (GInput.GetActionToDo(UAOptics))
    deleteCamera = true;
  if (GInput.GetActionToDo(UALockTarget))
  {
    if (_focusBlur > 0.5f)
      _focusBlur = 0.0f;
    else
      _focusBlur = 1.0f;
  }
  const float ZoomSpeed = 4.0f;
  const float change = pow(ZoomSpeed, expChange * deltaT);
  _lastFov *= change;
  saturate(_lastFov, 0.01f, 2.0f);
  _minFov = _maxFov = _lastFov; // force manual FOV
  DoAssert(_minFov>=0.001f && _minFov<10.0f);
  DoAssert(_maxFov>=0.001f && _maxFov<10.0f);

  // turn to follow camera direction
  // (i.e follow weapon)

  Matrix3 speedOrient;
  speedOrient.SetUpAndDirection( VUp, FutureVisualState().Direction() );

  Vector3 position = FutureVisualState().Position();
  if (!_comfyCamera)
  {
    Vector3 speedWanted = speedOrient * Vector3(aside,up,forward);
    if( _inertia )
    {
      // smooth changes
      speedWanted *= 4.0f;

      Vector3 accel = speedWanted - FutureVisualState()._speed;
      saturate(accel[0], -5.0f,  +5.0f);
      saturate(accel[1], -5.0f,  +5.0f);
      saturate(accel[2],-10.0f, +10.0f);
      FutureVisualState()._speed += accel*deltaT;
    }
    else
      FutureVisualState()._speed = speedWanted;

    const float surfY = GLandscape->SurfaceY(position[0], position[2]);
    saturateMax(position[1], surfY + 0.2f);
  }
  else
  {
    FutureVisualState()._speed = 
			forward * FutureVisualState().Orientation().Direction() + 
			debStrafe*FutureVisualState().Orientation().DirectionAside() + 
			Vector3(0.0f, debUpDown, 0.0f);
  }
  position += FutureVisualState()._speed * deltaT;
  Move(position);

  // get mouse movement
  // or numeric keyboard
  const float headChange = headSpeed * 2.0f * deltaT * _lastFov;
  const float diveChange = diveSpeed * 2.0f * deltaT * _lastFov;
  Matrix3 orient = FutureVisualState().Orientation();
  if( headChange )
  {
    Matrix3 rotY(MRotationY, headChange);
    orient = rotY * orient;
  }
  if( diveChange )
  {
    Matrix3 rotX(MRotationX, diveChange);
    // neutralize heading
    Matrix3 head, invHead;
    head.SetDirectionAndUp(FutureVisualState().Direction(), VUp);
    invHead = head.InverseRotation();
    orient = head * rotX * invHead * orient;
    ResetTargets();
  }
  SetOrientation(orient);

  if( headChange || diveChange )
    ResetTargets();

  if (resetTargets)
  {
    // unlock camera
    ResetTargets();
  } // fire
  if (lockCamera)
  {
    // lock camera
    // shoot probe ray
    // if nothing is hit, fix at ground point
    // try to find some lock
    const float objDist = Glob.config.objectsZ;
    CollisionBuffer retVal;
    GLandscape->ObjectCollisionLine(
          GetFutureVisualStateAge()
        , retVal, Landscape::FilterIgnoreOne(this)
        , FutureVisualState().Position()
        , FutureVisualState().Position() + FutureVisualState().Direction() * objDist
        ,0/*2*/, ObjIntersectGeom
      );
    // find nearest intersection
    Object *minObj = NULL;
    float minDist2 = 1e10;
    for( int i=0; i<retVal.Size(); i++ )
    {
      Object *obj = retVal[0].object;
      //if( !dyn_cast<Entity>(obj) ) continue;
      const float dist2 = obj->FutureVisualState().Position().Distance2(FutureVisualState().Position());
      if (dist2 < minDist2)
      {
        minDist2 = dist2;
        minObj = obj;
      }
    }
    if( minObj )
      _target = minObj;
    else
    {
      const Vector3 land = GLandscape->IntersectWithGroundOrSea(FutureVisualState().Position(),FutureVisualState().Direction());
      _target = land;
    }
  } // selectweapon
  if (switchInertia)
    _inertia = !_inertia;
  if (switchCrossHair)
    _crossHairs = !_crossHairs;
  if (deleteCamera)
    SetDelete(); // vehicle should be removed
  if (GInput.GetActionToDo(UADefaultAction))
  {
    // save text (preferably to clipboard)
    char buf[1024];
    QOStrStream f;
    const char *myName = "_camera"; // TODO: check symbolic name
    {
      time_t tb;
      time(&tb);
      struct tm *lt = localtime(&tb);
      sprintf(buf,";comment \"%d:%02d:%02d\";\r\n",lt->tm_hour,lt->tm_min,lt->tm_sec);
      f.write(buf,strlen(buf));
    }

    if( _target._target )
    {
      // TODO: get symbolic name (expression)
      RString aiName = _target._target->GetVarName();
      if (aiName.GetLength()>0)
        sprintf(buf,"%s camPrepareTarget %s;\r\n",myName,(const char *)aiName);
      else if (dyn_cast<EntityAI,Object>(_target._target))
      {
        RString symName = _target._target->GetDebugName();
        sprintf(buf,"%s camPrepareTarget '%s';\r\n",myName,(const char *)symName);
      }
      else
      {
        // no entity, we have to use nearestObject
        Vector3Val pos = _target._target->FutureVisualState().Position();
        OIDInt id = _target._target->ID().Encode();
#if _HELISIM_LARGE_OBJECTID
        sprintf(buf,"%s camPrepareTarget ([%.1f,%.1f] nearestObject %I64d)\r\n",myName,pos.X(),pos.Z(),id);
#else
        sprintf(buf,"%s camPrepareTarget ([%.1f,%.1f] nearestObject %d);\r\n",myName,pos.X(),pos.Z(),id);
#endif
      }
      f.write(buf,strlen(buf));
    }
    else
    {
      if( _target._pos.SquareSize() > 0.5f )
      {
        float surfY = GLandscape->SurfaceYAboveWater(_target._pos[0],_target._pos[2]);
        sprintf(buf,"%s camPrepareTarget [%.2f,%.2f,%.2f];\r\n",myName,
          _target._pos[0],_target._pos[2],_target._pos[1]-surfY);
        f.write(buf,strlen(buf));
      }
    }
    if (_target._target && GInput.keys[DIK_LSHIFT] )
    {
      Vector3 relPos = _target.PositionAbsToRel(FutureVisualState().Position());
      sprintf(buf,"%s camPrepareRelPos [%.2f,%.2f,%.2f];\r\n",myName,
        relPos[0],relPos[2],relPos[1]);
      f.write(buf,strlen(buf));
    }
    else
    {
      const float surfY = GLandscape->SurfaceYAboveWater(FutureVisualState().Position()[0],FutureVisualState().Position()[2]);
      sprintf(buf,"%s camPreparePos [%.2f,%.2f,%.2f];\r\n",myName, FutureVisualState().Position()[0],FutureVisualState().Position()[2],FutureVisualState().Position()[1]-surfY);
      f.write(buf,strlen(buf));
    }
    {
      sprintf(buf,"%s camPrepareFOV %.3f;\r\n",myName,_lastFov);
      f.write(buf,strlen(buf));
    }

    {
      sprintf(buf,"%s camCommitPrepared 0\r\n",myName);
      f.write(buf,strlen(buf));
    }

    #if 0
    {
      sprintf(buf,"@camCommitted _camera\r\n");
      f.write(buf,strlen(buf));
    }
    #endif

    char logFilename[MAX_PATH];
    if (GetLocalSettingsDir(logFilename))
    {
      CreateDirectory(logFilename, NULL);
      TerminateBy(logFilename, '\\');
    }
    else
      logFilename[0] = 0;
    strcat(logFilename, "clipboard.txt");

    ExportToClipboardAndFile(f.str(), f.pcount(), logFilename);
  } // fire - save parameters
  else if (GInput.GetActionToDo(UAAction))
  {
    // process a script, typically obtained by copying into the clipboard
    RString cameraScript = ImportFromClipboard();
    // the script assumes _camera contains a camera object

    if (!cameraScript.IsEmpty())
    {
      _movePos = VZero;
      _minFovT = _maxFovT = -1;

      GameVarSpace vars(false);
      int gshandle = GGameState.BeginContext(&vars);
      GGameState.VarSetLocal("_camera",GameValueExt(this));
      GGameState.Execute(cameraScript, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
      GGameState.EndContext(gshandle);

      // if movement was commited, execute it as if automatic camera
      if( _movePos.SquareSize()>0.5 )
      {
        if( Glob.time>=_movePosTime )
        {
          Move(_movePos);
        }
      }
      if( _minFovT>=0 )
      {
        _minFov = _minFovT;
        _maxFov = _maxFovT;
      }
    }
  }
}

void CameraVehicle::Simulate(float deltaT, SimulationImportance prec)
{
  if (_preloadTimeout>Glob.time)
  {
    //handle preloading (when requested)
    float preloadDist = floatMin(GScene->GetFogMaxRange(),Glob.config.horizontZ,Glob.config.objectsZ,700);
    float timeout = 15;
    bool ret = GLandscape->PreloadData(_preloadPos,preloadDist,0.008f,0.5,NULL,false,timeout,false,&_preloadDir,_preloadFov);
    _preloadDone = ret;
  }
  else
    _preloadDone = true;

  if( !GetManual() )
  {
    //_moveTrans=*this;
    if( _movePos.SquareSize()>0.5 && !IsAttached())
    {
      if( Glob.time>=_movePosTime )
      {
        Move(_movePos);
        FutureVisualState().SetSpeed(VZero);
      }
      else
      {
        Vector3 offset = _movePos - FutureVisualState().Position();
        float t = _movePosTime - _startTime;
        float t0 = _movePosTime - Glob.time;
        if (_accelTime > 0 && t > 0)
        {
          float s = 0;
          if (t0 <= _accelTime)
            s = 0.5 * _accel * t0 * t0;
          else if (t - t0 >= _accelTime)
          {
            float t2 = t0 - _accelTime;
            s = 0.5 * _accel * _accelTime * _accelTime + _accel * _accelTime * t2;
          }
          else
          {
            float t2 = t - _accelTime - _accelTime;
            float t3 = t - t0;
            s = 0.5 * _accel * _accelTime * _accelTime + _accel * _accelTime * t2 +
              0.5 * _accel * _accelTime * _accelTime - 0.5 * _accel * t3 * t3;
          }
          Vector3 pos = _movePos - s * offset.Normalized();
/*
RString msg = Format("Total time: %.3f, remaining: %.3f, value: %.3f, pos [%.3f, %.3f]", t, t0, s, pos[0], pos[2]);
LogF(msg);
LogF("Last pos [%.3f, %.3f]", Position()[0], Position()[2]);
DIAG_MESSAGE(100, msg);
_speed = deltaT > 0 ? (pos - Position()) * (1.0 / deltaT) : VZero;
*/
          Move(pos);
        }
        else
        {
          // smooth interpolation
          // control speed so that you are in position in time
          FutureVisualState()._speed = offset * (1 / t0);
          Move(FutureVisualState().Position()+FutureVisualState()._speed*deltaT);
        }
      }
    }

#if _HELISIM_RENDER_TO_TEXTURE
    {
      if (Glob.time >= _orientationTime)
      {
        Matrix4 toWorld = WorldTransform(FutureVisualState());
        SetDirectionAndUp(toWorld.Rotate(_wantedOrient.Direction()), toWorld.Rotate(_wantedOrient.DirectionUp()) );
      }
      else
      {
        const Matrix3 &origOrient = FutureVisualState().Transform().Orientation();

        Quaternion<float> q0(origOrient);
        Quaternion<float> q1(_wantedOrient);

        float t = (_orientationTime - Glob.time) * _orientTimeMult; // 1 -> 0
        saturate(t, 0.0f, 1.0f);        
        Matrix3 interpolated;
        q1.Nlerp(q0, t).UnitToMatrix(interpolated);

        Matrix4 toWorld = WorldTransform(FutureVisualState());
        SetDirectionAndUp(toWorld.Rotate(interpolated.Direction()), interpolated.DirectionUp());
      }
    }
#endif

    if( _minFovT>=0 )
    {
      // interpolate minFov
      if( Glob.time<_minMaxFovTime )
      {
        float invTime = deltaT/(_minMaxFovTime-Glob.time);
        //do not make bigger step than (_minFovT-_minFov) or (_maxFovT-_maxFov)
        saturateMin(invTime, 1.0f); 
        _minFov += (_minFovT-_minFov)*invTime;
        _maxFov += (_maxFovT-_maxFov)*invTime;
        
        DoAssert(_minFov>=0.001 && _minFov<10);
        DoAssert(_maxFov>=0.001 && _maxFov<10);
      }
      else
      {
        _minFov = _minFovT;
        _maxFov = _maxFovT;
        DoAssert(_minFov>=0.001 && _minFov<10);
        DoAssert(_maxFov>=0.001 && _maxFov<10);
      }
    }
    if (Glob.time<_focusTime)
    {
      float invTime = deltaT/(_focusTime-Glob.time);
      _focusDistance += (_focusDistanceT-_focusDistance)*invTime;
      _focusBlur += (_focusBlurT-_focusBlur)*invTime;
    }
    else
    {
      _focusDistance = _focusDistanceT;
      _focusBlur = _focusBlurT;
    }

    //_integrationDone=true;
  }
  else
    SimulateDebugCamera(deltaT);
  
#if _VBS2
  MoveAttached(deltaT);
#endif

}

void CameraVehicle::Command( RString mode )
{
  // camera dependent special command
  if (!strcmpi(mode, "inertia on"))
    _inertia = true;
  else if (!strcmpi(mode,"inertia off"))
    _inertia = false;
  else
    base::Command(mode);
}

#if _VBS2
void CameraVehicle::CalcAttachedPosition(float delta)
{
  if (!_attachedTo)
    return;

  if(GVBSVisuals.rteVisible)
  {
    SetPosition(_attachedTo->FutureVisualState().Position() + _attachedOffset);
    SetTarget(_attachedTo->FutureVisualState().Position());
  }
  else
  {
    Matrix4 transform = _attachedTo->FutureVisualState().ModelToWorld();
    SetPos(transform.FastTransform(_attachedOffset));  
    if (_lookDirAttached == VZero)
      SetTarget(_attachedTo->FutureVisualState().Position());
    else
    {
      float maxDist = 100;  // just some distance away
      Matrix3 orient;
      orient.SetIdentity();
      orient.SetDirectionAndUp(_lookDirAttached,VUp);  
      SetOrientation(_attachedTo->FutureVisualState().Orientation()*orient);
      SetTarget(_attachedTo->FutureVisualState().Position()+(FutureVisualState().Orientation().Direction()*maxDist));
    }
  }
  Commit(0);

  // bank is set in CamEffectFOV()
}
#endif

void CameraVehicle::Commit(float time)
{
  _startTime = Glob.time;
  Time cTime = _startTime + time;
  // commit all settings
  _accel = 0.001;
  _accelTime = 0;
  if (_set._camPos)
  {
    if (time<=0)
    {
      // sudden change: reset any visibility information
      // reset only when camera moved a lot
      float dist2 = _movePos.Distance2(_setPars._camPos);
      if (dist2>Square(5))
      {
        GScene->OnCameraChanged();
        GWorld->OnCameraChanged();
      }
    }
    _movePos = _setPars._camPos;
    _movePosTime = cTime;
    _set._camPos = false;
    if (_inertia && time > 0)
    {
      float s = FutureVisualState().Position().Distance(_movePos);
      float invT2 = 1.0f / (time * time);
      saturateMax(_accel, 4 * s * invT2);
      float discriminant = time * time - 4 * s / _accel;
      if (discriminant>=0)
        _accelTime = 0.5f * (time - sqrt(discriminant));
      else
        _accelTime = 0.5f * time;
    }

  }
  if (_set._camDir)
  {    
    _wantedOrient.SetDirectionAndUp(WorldTransform(FutureVisualState()).Rotate(_setPars._camDir), VUp);
    _orientationTime = cTime;
    _orientTimeMult = 1.0f / time;
    _set._camDir = false;
  }
  if( _set._camTgt )
  {
    _oldTarget = _target;
    _target = _setPars._camTgt;
    _targetTime = cTime;
    _oldTargetTime = Glob.time;
    _set._camTgt = false;
  }
  if( _set._camFovMinMax )
  {
    if (time<=0 && fabs(_minFovT-_setPars._camFovMin)>0.01f) GScene->OnCameraCut();
    _minFovT = _setPars._camFovMin;
    _maxFovT = _setPars._camFovMax;
    _minMaxFovTime = cTime;
    _set._camFovMinMax = false;
  }
  if (_set._focus)
  {
    _focusBlurT = _setPars._camFocusBlur;
    _focusDistanceT = _setPars._camFocusDistance;
    _focusTime = cTime;
    _set._focus = false;
  }

  if( cTime>_timeCommitted )
    _timeCommitted = cTime;
  if (time<=0)
  {
    // commit immediately
    Simulate(0,SimulateVisibleNear);
  }
}

void CameraVehicle::CommitPrepared(float time)
{
  // those parameters which were prepared should be transferred to set now
  if (_prep._camPos)
  {
    _setPars._camPos = _preparedPars._camPos;
    _set._camPos = true,_prep._camPos = false;
  }
  if (_prep._camFov)
  {
    _setPars._camFov = _preparedPars._camFov;
    _set._camFov = true,_prep._camFov = false;
  }
  if (_prep._camTgt)
  {
    _setPars._camTgt = _preparedPars._camTgt;
    _set._camTgt = true,_prep._camPos = false;
  }
  if (_prep._camFovMinMax)
  {
    _setPars._camFovMin = _preparedPars._camFovMin;
    _setPars._camFovMax = _preparedPars._camFovMax;
    _set._camFovMinMax = true,_prep._camFovMinMax = false;
  }
  if (_prep._focus)
  {
    _setPars._camFocusDistance = _preparedPars._camFocusDistance;
    _setPars._camFocusBlur = _preparedPars._camFocusBlur;
    _set._focus = true,_prep._focus = false;
  }
  Commit(time);
  
  Time timeout = Glob.time+time;
  if (_preloadTimeout>timeout)
  {
    // once commit has finished, we do not need to pre-load for it any more
    _preloadTimeout = timeout;
  }
}
void CameraVehicle::PreloadPrepared(float time)
{
  // we need:
  // position, direction, fov
  // we use preferably prepared values 
  // if not available, we use set values
  // if not available, we use current values
  // fill with current values
  Vector3 pos = FutureVisualState().Position();
  Vector3 dir = FutureVisualState().Direction();
  float fov = 1;
  if (_prep._camPos) pos = _preparedPars._camPos;
  else if (_set._camPos) pos = _setPars._camPos;
  if (_prep._camTgt) dir = _preparedPars._camTgt.GetPos()-pos;
  else if (_set._camTgt) dir = _setPars._camTgt.GetPos()-pos;
  if (_prep._camFov) fov = _preparedPars._camFov;
  else if (_set._camFov) fov = _setPars._camFov;
  
  // now we can initiate the preload
  // we want it to timeout after given time
  // do not allow preload longer than 30 sec
  const float MaxPreload = 30.0f;
  if (time<=0 || time>MaxPreload) time = MaxPreload;
  
  _preloadTimeout = Glob.time+time;
  _preloadPos = pos;
  _preloadDir = dir;
  _preloadFov = fov;
  _preloadDone = false;
}

bool CameraVehicle::GetCommitted() const
{
  // check if all times have been reached
  return Glob.time>=_timeCommitted;
}

bool CameraVehicle::GetPreloaded() const
{
  return _preloadDone;
}

DEFINE_CASTING(CameraConstruct)

CameraConstruct::CameraConstruct(const EntityType *type):base(NULL,type,CreateObjectId())
{
  _crossHairs = false;
  _inertia = false;

  void ShowCinemaBorder(bool show);
  ShowCinemaBorder(false);

  _radius = (*type->_par) >> "radius";
  _maxHAL = (*type->_par) >> "maxHAL";
  _constructCenter = Vector3(-1,-1,-1);
  _manual = true;

  // set all target properties to invalid values
  _movePos = VZero;
  _minFovT = _maxFovT = -1;
  // set all times to infinite future
  _movePosTime=TIME_MAX;
  _minMaxFovTime=TIME_MAX;
  _focusTime = TIME_MAX;
  _focusBlurT =0;
  _preloadTimeout = TIME_MIN;
  _preloadDone = true;
  // all committed
  _timeCommitted = TIME_MIN;
  // set all actual properties to default values
  _minFov=0.7,_maxFov=0.7;
  _lastFov=0.7;
  _lastTgtPos=VZero;

  _focusBlur = 1,_focusDistance =-1;
  _lastRotateChange = 0;
  _lastDirection = FutureVisualState().Direction();
  SetTimeOffset(0);
}


static void SetObjectPosition(Object *obj, Matrix4Par trans)
{
  obj->VisualCut();
  obj->MoveNetAware(trans);
  obj->OnPositionChanged();
  GScene->GetShadowCache().ShadowChanged(obj);
}

void CameraConstruct::Simulate(float deltaT, SimulationImportance prec)
{

  deltaT /= GWorld->GetAcceleratedTime();
  // manual controls
  // use basic controls to control movement

  float forward = 0;
  float aside = 0;
  float up = 0;
  float expChange = 0;
  float headSpeed = 0;
  float diveSpeed = 0;

  bool deleteCamera = false;
  void ShowCinemaBorder(bool show);
  ShowCinemaBorder(false);

  float speed = 5;
  // float speed = GInput.GetAction(UABuldTurbo)*7+1;
  if(GInput.GetAction(UABuldTurbo)>0) speed = 2;
  forward =
    (
    //(GInput.GetAction(UABuldMoveForward)-GInput.GetAction(UABuldMoveBack))*4*Input::MouseRangeFactor+
    (GInput.GetAction(UAMoveForward)-GInput.GetAction(UAMoveBack))*Input::MouseRangeFactor
    )*speed;
  aside =
    (
   // (GInput.GetAction(UABuldMoveRight)-GInput.GetAction(UABuldMoveLeft))*4*Input::MouseRangeFactor+
    (GInput.GetAction(UATurnRight)-GInput.GetAction(UATurnLeft))*Input::MouseRangeFactor
    )*speed;
  up = (GInput.GetAction(UABuldUp)-GInput.GetAction(UABuldDown))*4*speed;
  expChange = GInput.GetAction(UABuldZoomOut)-GInput.GetAction(UABuldZoomIn);
  headSpeed = 
    (GInput.GetAction(UABuldLookLeft)-GInput.GetAction(UABuldLookRight)+
    (-GInput.GetAction(UABuldMoveRight)+GInput.GetAction(UABuldMoveLeft))*Input::MouseRangeFactor
    );
  diveSpeed = 
    (GInput.GetAction(UABuldLookDown)-GInput.GetAction(UABuldLookUp)+
    (-GInput.GetAction(UABuldMoveForward)+GInput.GetAction(UABuldMoveBack))*Input::MouseRangeFactor
    );
#if _SPACEMOUSE
  //SpaceMouse
  aside += GInput.spaceMouseAxis[0]*deltaT*10.0;
  forward += GInput.spaceMouseAxis[2]*deltaT*10.0;
  up += GInput.spaceMouseAxis[1]*deltaT*10.0;

  diveSpeed -= GInput.spaceMouseAxis[3]*deltaT;
  headSpeed += GInput.spaceMouseAxis[4]*deltaT;

  /*  LogF("SpaceMouse X:%.5f Y:%.5f Z:%.5f rx:%.5f ry:%.5f rz:%.5f  up:%.5f cursorMovedZ:%.5f"
  , GInput.spaceMouseAxis[0]
  , GInput.spaceMouseAxis[1]
  , GInput.spaceMouseAxis[2]
  , GInput.spaceMouseAxis[3]
  , GInput.spaceMouseAxis[4]
  , GInput.spaceMouseAxis[5]
  , up
  , GInput.cursorMovedZ
  );
  */
#endif



  // turn to follow camera direction
  // (i.e follow weapon)
  float _radiusSQR = _radius * _radius;
  Matrix3 speedOrient;
  speedOrient.SetUpAndDirection( VUp, FutureVisualState().Direction() );

  Vector3 speedWanted = speedOrient * Vector3(aside,up,forward);
  FutureVisualState()._speed = speedWanted;

  //used to keep constant height above land
  Vector3 position = FutureVisualState().Position();
  float alt = position[1] - GLandscape->SurfaceY(position[0],position[2]);

  Vector3 newPosition = FutureVisualState().Position()+2*FutureVisualState()._speed*deltaT;
  if(_constructCenter.X()<0) _constructCenter = FutureVisualState().Position();
  if(
    Square(_constructCenter.X() - newPosition[0]) + 
    Square(_constructCenter.Z() - newPosition[2]) > _radiusSQR )
  {
    float size = sqrtf(Square(_constructCenter.X() - newPosition[0]) +  Square(_constructCenter.Z() - newPosition[2]));
    float xc = (newPosition[0]-_constructCenter.X()) / size;
    float zc = (newPosition[2]-_constructCenter.Z()) / size;

    newPosition[0] = _constructCenter.X() + _radius * xc;
    newPosition[2] = _constructCenter.Z() + _radius * zc;
  }

  //new height above land 
  float surfY = GLandscape->SurfaceY(newPosition[0],newPosition[2]);
  newPosition[1] = surfY + alt + FutureVisualState()._speed.Y()*deltaT;
  //float surfY = GLandscape->SurfaceYAboveWater(position[0],position[2]);
  saturate(newPosition[1],surfY+0.2f,surfY+_maxHAL);
  
  Move(newPosition);
  //when control or alt are pressed, mouse is used for object rotation
  if(!(GInput.keyPressed[DIK_LCONTROL] || GInput.keyPressed[DIK_RCONTROL]) || !_target._target)
  {
    // get mouse movement
    // or numeric keyboard
    float headChange=headSpeed*2*deltaT*_lastFov;
    float diveChange=diveSpeed*2*deltaT*_lastFov;
    Matrix3 orient=FutureVisualState().Orientation();
    if( headChange )
    {
      Matrix3 rotY(MRotationY,headChange);
      orient=rotY*orient;
      SetOrientation(orient);
    }

    if( diveChange )
    {
      Matrix3 orientOld=FutureVisualState().Orientation();
      Matrix3 rotX(MRotationX,diveChange);
      // neutralize heading
      Matrix3 head,invHead;
      head.SetDirectionAndUp(FutureVisualState().Direction(),VUp);
      invHead=head.InverseRotation();
      orient=head*rotX*invHead*orient; 
      if(orient.Direction().Y()>0)
      { //max pitch
        Vector3 dir = Vector3(orient.Direction().X(),0,orient.Direction().Z());
        dir.Normalize();
        orient.SetDirectionAndUp(dir,VUp);
      }
      else if(orient.DirectionUp().Y()<0.01) // 0.01 is some minimal angle
      { //min pitch
        Vector3 dir = Vector3(orientOld.Direction().X(),0,orientOld.Direction().Z());
        dir.Normalize();
        orient.SetDirectionAndUp(dir,VUp);
        Matrix3 rotX(MRotationX,1.5607964); //1.5607964 is arcos(0.01)
        orient= orient*rotX; 
      }
      SetOrientation(orient);
    }
  }
  else if(_target._target)
  {
    if((GInput.keyPressed[DIK_LCONTROL] || GInput.keyPressed[DIK_RCONTROL])
      && ! (GInput.keyPressed[DIK_LALT] || GInput.keyPressed[DIK_RALT]))
    {
      Vector3 dir = Vector3(FutureVisualState().Direction().X(),0,FutureVisualState().Direction().Z());
      dir.Normalize();
      float angel = acos(dir.X());
      if(dir.Z()<0) angel = 6.28318 - angel;

      float headChange=headSpeed*2*deltaT*_lastFov;
      Matrix3 orient=_target._target->FutureVisualState().Orientation();
      if( headChange )
      {
        Matrix3 rotY(MRotationY,headChange);
        orient=rotY*orient;
        _target._target->SetOrientation(orient);
      }
    }
    else if(
      (GInput.keyPressed[DIK_LCONTROL] || GInput.keyPressed[DIK_RCONTROL]) && 
      (GInput.keyPressed[DIK_LALT] || GInput.keyPressed[DIK_RALT]))
    {
      float headChange=headSpeed*2*deltaT*_lastFov;
      _lastRotateChange += headChange;

      if(fabsf(_lastRotateChange) > 0.785398) //0.785398 is pi/4
      {

        Vector3 dir = Vector3(_target._target->FutureVisualState().Direction().X(),0,_target._target->FutureVisualState().Direction().Z());
        dir.Normalize();
        float angel = acos(dir.X());
        if(dir.Z()<0) angel = 6.28318 - angel;
        angel+=6.28318;

        if(_lastRotateChange > 0)
          angel += 0.785399;
        else 
          angel -= 0.785398+6.28318;

        angel = (toInt(angel/0.785398))*0.785398;
        dir[0] = cos(angel);
        dir[2] = sin(angel);

        Matrix3 orient;
        orient.SetDirectionAndUp(dir,VUp);

        _target._target->SetOrientation(orient);
        _lastRotateChange = 0;
      }
    }
  }

  /*----------------------------------------------------------------------------------*/
  if(_target._target)
  {
    AspectSettings asp;
    GEngine->GetAspectSettings(asp);
    // convert to screen space
    float xScreen = 0.5 * (asp.uiBottomRightX - asp.uiTopLeftX) + asp.uiTopLeftX;
    float yScreen = 0.5 * (asp.uiBottomRightY - asp.uiTopLeftY) + asp.uiTopLeftY;

    const Camera *camera = GScene->GetCamera();
    if (camera)
    {
      // convert to the camera space
      Vector3 dirCam((2.0f * xScreen - 1.0f) * camera->Left(), -(2.0f * yScreen - 1.0f) * camera->Top(), 1.0f);
      dirCam.Normalize();

      // convert to the world space
      Vector3 dir = FutureVisualState().DirectionModelToWorld(dirCam);
      // find the intersection with landscape
      Vector3 landPos = GLandscape->IntersectWithGroundOrSea(position, dir);
      landPos[1] = 0;

      /*if(
        Square(_constructCenter.X() - landPos[0]) + 
        Square(_constructCenter.Z() - landPos[2]) > _radiusSQR )
      {

        float Px = position.X() - _constructCenter.X();
        float Pz = position.Z() - _constructCenter.Z();
        Vector3 dir = Vector3(Direction().X(),0,Direction().Z());
        dir.Normalize();

        float c = Square(Px) + Square(Pz) - _radiusSQR;
        float b = (2 * Px * dir.X() + 2 * Pz * dir.Z());
        float a = Square(dir.X()) + Square(dir.Z());

        float D = (Square(b) - 4*a*c);
        if(D>=0)
        {
          D = sqrtf(D);
          float t1 = (-b - D)/ (2*a);
          float t2 = (-b + D)/ (2*a);
          t1 = floatMax(t1,t2);

          landPos[0] = position.X() + Direction().X() * t1; 
          landPos[1] = 0;
          landPos[2] = position.Z() + Direction().Z() * t1; 
        }

        
        {
        float size = sqrtf(Square(_constructCenter.X() - landPos[0]) +  Square(_constructCenter.Z() - landPos[2]));
        float xc = (landPos[0]-_constructCenter.X()) / size;
        float zc = (landPos[2]-_constructCenter.Z()) / size;

        landPos[0] = _constructCenter.X() + _radius * xc;
        landPos[2] = _constructCenter.Z() + _radius * zc;
        }
        
      }*/

      // let vehicle adjust position
      Matrix4 trans = _target._target->FutureVisualState().Transform();
      Object *objt = _target._target;
      if (objt)
      {
        trans.SetPosition(landPos);
        // change vehicle orientation relative to terrain under it
        objt->PlaceOnSurface(trans);
        Vector3 surfPos = trans.Position();
        surfPos[1] += landPos[1];
        trans.SetPosition(surfPos);
      }
      else
      {
        landPos[1] += GLandscape->SurfaceYAboveWater(landPos.X(), landPos.Z());
        trans.SetPosition(landPos);
      }

      SetObjectPosition(objt, trans);
    }
  }
  /*----------------------------------------------------------------------------------*/

  if (deleteCamera)
    SetDelete(); // vehicle should be removed
}

void CameraConstruct::Commit(float time)
{
  _startTime = Glob.time;
  Time cTime = _startTime + time;
  // commit all settings
  _accel = 0.001;
  _accelTime = 0;

  if( _set._camTgt )
  {
    _oldTarget = _target;
    _target = _setPars._camTgt;
    _targetTime = cTime;
    _oldTargetTime = Glob.time;
    _set._camTgt = false;
  }
  else
  {
    _oldTarget = _target;
    _target._target = NULL;
    _targetTime = cTime;
    _oldTargetTime = Glob.time;
    _set._camTgt = false;
  }

  if( cTime>_timeCommitted )
    _timeCommitted = cTime;
  if (time<=0)
    // commit immediately
    Simulate(0,SimulateVisibleNear);
}

void CameraConstruct::CommitPrepared(float time)
{
  // those parameters which were prepared should be transferred to set now
  if (_prep._camPos)
  {
    _setPars._camPos = _preparedPars._camPos;
    _set._camPos = true,_prep._camPos = false;
  }
  if (_prep._camFov)
  {
    _setPars._camFov = _preparedPars._camFov;
    _set._camFov = true,_prep._camFov = false;
  }
  if (_prep._camFovMinMax)
  {
    _setPars._camFovMin = _preparedPars._camFovMin;
    _setPars._camFovMax = _preparedPars._camFovMax;
    _set._camFovMinMax = true,_prep._camFovMinMax = false;
  }
  if (_prep._focus)
  {
    _setPars._camFocusDistance = _preparedPars._camFocusDistance;
    _setPars._camFocusBlur = _preparedPars._camFocusBlur;
    _set._focus = true,_prep._focus = false;
  }
  if (_prep._camTgt)
  {
    _setPars._camTgt = _preparedPars._camTgt;
    _set._camTgt = true,_prep._camPos = false;
  }
  Commit(time);

  Time timeout = Glob.time+time;
  if (_preloadTimeout>timeout)
    // once commit has finished, we do not need to pre-load for it any more
    _preloadTimeout = timeout;
}

bool CameraConstruct::GetCommitted() const
{
  // check if all times have been reached
  return Glob.time>=_timeCommitted;
}
