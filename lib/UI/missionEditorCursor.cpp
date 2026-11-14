#include "../wpch.hpp"

#if _ENABLE_EDITOR2 && !_VBS2

#include "missionEditor.hpp"
#include "../world.hpp"
#include "missionEditorCursor.hpp"
#include "../landscape.hpp"
#include "../keyInput.hpp"
#include "../dikCodes.h"
#include "../camera.hpp"
#include "../fileLocator.hpp"
#include <Es/Common/win.h> // GetKeyState

const float DistNearObject = 5;

const EditorProxy *MissionEditCursorContainer::FindProxy(Vector3Par pos, GameState *gstate) const
{
  float minDist2 = Square(DistNearObject);
  const EditorProxy *best = NULL;
  for (int i=0; i<_proxies.Size(); i++)
  {
    EditorObject *edObj = _proxies[i].edObj;
    if (edObj && 
      (
        !edObj->IsVisibleIn3D() || 
        !edObj->IsObjInCurrentOverlay() ||
        edObj->GetScope() == EOSAllNoSelect
      )
    ) continue;

    Object *obj = _proxies[i].object;
    if (!obj) continue;
    float dist2 = pos.Distance2(obj->FutureVisualState().Position());
    if (dist2 < minDist2)
    {
      minDist2 = dist2;
      best = &(_proxies[i]);
    }
  }
  return best;
}

const EditorProxy *MissionEditCursorContainer::FindProxy(Object *obj) const
{
  for (int i=0; i<_proxies.Size(); i++)
    if (
      _proxies[i].object == obj && 
      _proxies[i].edObj && 
      _proxies[i].edObj->GetScope() != EOSAllNoSelect
    )
      return &_proxies[i];
  return NULL;
};

void MissionEditCursorContainer::AddProxy(Object *object,EditorObject *edObj)
{
  int index = _proxies.Add();
  _proxies[index].edObj = edObj;
  if (edObj) _proxies[index].id = edObj->GetArgument("VARIABLE_NAME");
  _proxies[index].object = object; 
}

void MissionEditCursorContainer::UpdateProxy(Object *object,EditorObject *edObj)
{
  for (int i=0; i<_proxies.Size(); i++)
  {
    if (_proxies[i].edObj == edObj)
    {
      if (edObj) _proxies[i].id = edObj->GetArgument("VARIABLE_NAME");
      _proxies[i].object = object; 
      return;
    }
  }
  // proxy doesn't exist, add a new one
  AddProxy(object,edObj);
}

void MissionEditCursorContainer::DeleteProxy(EditorObject *edObj)
{
  for (int i=0; i<_proxies.Size(); i++)
  {
    if (_proxies[i].edObj == edObj)
      _proxies.Delete(i--);
  }
}

// speeds
#define SPEED_FAST    4

// defaults
const float DefCamDistance = 10;
const float DefCamHeight = 1;
const float DefDive = 0.5;

MissionEditorCursor::MissionEditorCursor(MissionEditCursorContainer *parent)
: base(NULL, VehicleTypes.New("EditCursor"), CreateObjectId())
{
  Assert(parent);
  _parent = parent;

  _camDistance = DefCamDistance;
  _camHeight = DefCamHeight;
  _heading = 0;
  _dive = DefDive;
}

MissionEditorCursor::~MissionEditorCursor()
{
}

enum EditObjectMode
{
  EOMNormal,
  EOMMove,
  EOMRotate
};

Vector3 MissionEditorCursor::ExternalCameraPosition( CameraType camType ) const
{ 
  static float scale = 0.2;
  return Vector3(0,0.1,-1)*scale; 
}

void MissionEditorCursor::Simulate(float deltaT, SimulationImportance prec)
{
  bool IsAppPaused();
  if (IsAppPaused()) return;

  Vector3 position = FutureVisualState().Position();
  const EditorProxy *proxy = _parent->FindProxy(position,GWorld->GetGameState());
  PackedColor color = PackedWhite;
  EditObjectMode mode = EOMNormal;
  if (proxy)
  {
    color = PackedColor(Color(1, 1, 0, 1));
    if (GInput.mouseL)
    {
      mode = EOMMove;
      color = PackedColor(Color(0, 1, 0, 1));
    }
    else if (GInput.mouseR)
    {
      mode = EOMRotate;
      color = PackedColor(Color(0, 1, 1, 1));
    }
  }
  SetConstantColor(color);
  
  // User input
  float speedKeyb = deltaT * 0.1f * _camDistance;
  float speedMouse = deltaT * 0.025f * _camDistance;
  
  float speedX = (GInput.GetAction(UABuldMoveRight) - GInput.GetAction(UABuldMoveLeft)) * speedMouse*Input::MouseRangeFactor;
  float speedZ = (GInput.GetAction(UABuldMoveForward) - GInput.GetAction(UABuldMoveBack)) * speedMouse*Input::MouseRangeFactor;
  speedX += (GInput.GetAction(UABuldRight) - GInput.GetAction(UABuldLeft)) * speedKeyb;
  speedZ += (GInput.GetAction(UABuldForward) - GInput.GetAction(UABuldBack)) * speedKeyb;

  bool faster = GInput.GetAction(UABuldTurbo) > 0.5f;
  if (faster)
  {
    speedKeyb *= SPEED_FAST;
    speedX *= SPEED_FAST;
    speedZ *= SPEED_FAST;
  }

// LogF("Speed x=%.2f, z=%.2f, keyb=%.2f", speedX, speedZ, speedKeyb);

  float rotYAngle = 0, rotXAngle = 0;

  if (GInput.GetAction(UABuldFreeLook))
  {
    // look around by mouse
    rotXAngle += speedZ;
    rotYAngle -= speedX;
  }
  else
  {
    // move cursor by mouse
    switch (mode)
    {
    case EOMNormal:
      {
        //Vector3 offset(speedX * _camDistance, 0, speedZ * _camDistance);
        Vector3 offset(speedX, 0, speedZ);

        // no object selected, rotate camera
        if (GInput.mouseR) 
          rotYAngle -= speedX/_camDistance;
        else
        {
          Matrix4Val rotY = Matrix4(MRotationY, _heading);
          offset.SetRotate(rotY, offset);
          position += offset;
        }
      }
      break;
    case EOMMove:
      {
        //Vector3 offset(speedX * _camDistance, 0, speedZ * _camDistance);
        Vector3 offset(speedX, 0, speedZ);
        Matrix4Val rotY = Matrix4(MRotationY, _heading);
        offset.SetRotate(rotY, offset);
        /*
        Vector3 pos = proxy->object->Position();
        float height = pos.Y() - GLandscape->SurfaceYAboveWater(pos.X(), pos.Z());
        pos += offset;
        pos[1] = GLandscape->SurfaceYAboveWater(pos.X(), pos.Z()) + height;
        */
        _parent->OnObjectMoved(proxy->edObj, offset);
        position += offset;
      }
      break;
    case EOMRotate:
      {
        if (speedX != 0)
          _parent->RotateObject(proxy->edObj, proxy->object->FutureVisualState().Position(), speedX);
      }
      break;
    }
  }

  // look around
  rotYAngle += (GInput.GetAction(UABuldLookLeft)-GInput.GetAction(UABuldLookRight)) * speedKeyb;
  rotXAngle += (GInput.GetAction(UABuldLookUp)-GInput.GetAction(UABuldLookDown)) * speedKeyb;
  // zoom
  static float zoomSpeed=0.5f;
  float distExp = (GInput.GetAction(UABuldZoomOut)-GInput.GetAction(UABuldZoomIn)) * speedKeyb * zoomSpeed;
  _camDistance *= exp(distExp);
  saturate(_camDistance, 0.01f, 2000.0f);
  // move up / down
  if (GInput.GetAction(UABuldDown))
  {
    float offset = -GInput.GetAction(UABuldDown) * speedKeyb * 20; 
    if (mode == EOMMove || mode == EOMRotate)
      _parent->OnObjectMoved(proxy->edObj, Vector3(0,offset,0));

    _camHeight += offset;
    _camHeight = floatMax(_camHeight, -5);
  }
  if (GInput.GetAction(UABuldUp))
  {
    float offset = GInput.GetAction(UABuldUp) * speedKeyb * 20;
    if (mode == EOMMove || mode == EOMRotate)
      _parent->OnObjectMoved(proxy->edObj, Vector3(0,offset,0));

    _camHeight += offset;
    _camHeight = floatMin(_camHeight, 1000);
  }
  // reset camera position
  if (GInput.GetActionToDo(UABuldResetCamera))
  {
    _camDistance = DefCamDistance;
    _camHeight = DefCamHeight;
    _dive = DefDive;
  }
  else
  {
    _dive += rotXAngle;
  }
  _heading += rotYAngle;

  // final calculation of transformation
  position[1] = GLandscape->SurfaceYAboveWater(position[0], position[2]) + _camHeight;

  Matrix3 rotY(MRotationY, _heading);
  Matrix3 rotX(MRotationX, _dive);
  Matrix3 rot = rotY * rotX;

  Matrix4 moveTrans;
  moveTrans.SetOrientation(rot);
  moveTrans.SetPosition(position);

  SetTransform(moveTrans); // finally apply move
  DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
}

void MissionEditorCursor::LimitVirtual(CameraType camType, float &heading, float &dive, float &fov) const
{
  heading = 0;
  dive = 0;
  fov = 0.7;
}

MissionEditorCamera::MissionEditorCamera(MissionEditCursorContainer *parent)
: base(VehicleTypes.New("camera"))
{
  Assert(parent);
  _parent = parent;

  _resetTargets = false;
  _dragging = false;
  _lockToTarget = false;
  _isMoving = false;
  _isRotating = false;
  _allowMoveOnEdge = false;

  // read settings from config
  ParamEntryVal wrapper = ::Pars >> "CfgEditCamera";

  _speedFwdBack = wrapper >> "speedFwdBack";
  _speedLeftRight = wrapper >> "speedLeftRight";
  _speedUpDown = wrapper >> "speedUpDown";
  _speedRotate = wrapper >> "speedRotate";
  _speedElevation = wrapper >> "speedElevation";
  _speedTurboMultiplier = wrapper >> "speedTurboMultiplier";

  _selectedCursor = GlobLoadTexture(GetPictureName(wrapper >> "iconSelect"));
  _selectedCursorWidth = wrapper >> "iconSelectSizeX";
  _selectedCursorHeight = wrapper >> "iconSelectSizeY";
  _color = GetPackedColor(wrapper >> "iconSelectColor");
}

MissionEditorCamera::~MissionEditorCamera()
{
}

void MissionEditorCamera::ResetTargets()
{
#if _VBS2
  Detach();
#endif
  _target._target = NULL;
  _target._pos = VZero;
  _oldTarget = NULL;
  _lastTgtPos = _target._pos;
  _movePos = VZero;
  _resetTargets = false;
  _lockToTarget = false;
}

Vector3 MissionEditorCamera::GetGroundIntercept()
{
  return GetGroundIntercept(GInput.cursorX, GInput.cursorY);
}

Vector3 MissionEditorCamera::GetGroundIntercept(float x, float y)
{
  // FIX: convert from 2D space to 3D, see InGameUI::DrawTargetInfo
  x *= (float)GEngine->Width2D() / GEngine->WidthBB();
  y *= (float)GEngine->Height2D() / GEngine->HeightBB();

  Camera& camera = *GScene->GetCamera();
  float posX = x * camera.Left();
  float posY = -y * camera.Top();

  Vector3 cPos = Vector3(posX, posY, 1);
  cPos.Normalize();

  Vector3 cursorPos = camera.PositionModelToWorld(cPos);
  Vector3 cursorDir = camera.DirectionModelToWorld(cPos);

  return GLandscape->IntersectWithGroundOrSea(cursorPos,cursorDir);
}

Vector3 MissionEditorCamera::GetCursorDir()
{
  Camera& camera = *GScene->GetCamera();
  float posX = GInput.cursorX * camera.Left();
  float posY = -GInput.cursorY * camera.Top();

  Vector3 cPos = Vector3(posX, posY, 1);
  Vector3 cursorPos = camera.PositionModelToWorld(cPos);
  return camera.DirectionModelToWorld(cPos);
}

void MissionEditorCamera::Simulate(float deltaT, SimulationImportance prec, GameState *gstate, bool cursorOverCamera)
{
  // allow access to camera effects (lock onto object etc)
  base::Simulate(deltaT,prec);

  if (_resetTargets)
    ResetTargets();

  if (deltaT == 0)
    return;

  bool IsAppPaused();
  if (IsAppPaused()) return;

  // camera controlled by external script?
  if ((_target._target || _target._pos != VZero) && !_lockToTarget) return; // TODO: will need to be improved

  Vector3 position = FutureVisualState().Position();  

  // turn when mouse is at screen edge, rather than move
  bool turnOnEdge = false;

  // move when mouse is at screen edge
  bool moveOnEdge = true;

  float headingChange = 0;
  float diveChange = 0;

  float speedX = (GInput.GetAction(UABuldMoveRight) - GInput.GetAction(UABuldMoveLeft)) * deltaT*Input::MouseRangeFactor;
  float speedY = (GInput.GetAction(UABuldMoveForward) - GInput.GetAction(UABuldMoveBack)) * deltaT*Input::MouseRangeFactor;  

  // rotate camera?
  if (_isRotating)
  {
    _dragging = true;
    moveOnEdge = false;

    float maxSpeed = 1;
    //if (GInput.GetAction(UABuldTurbo) > 0.5f)
      //maxSpeed *= _speedTurboMultiplier;

    // change camera orientation
    headingChange -= speedX * _speedRotate * maxSpeed;
    diveChange -= speedY * _speedElevation * maxSpeed;
    
    /*
    // smooth movement based on mouse position
    if (fabs(GInput.cursorX) > 0.2)
      _heading -= GInput.cursorX * deltaT * 2;

    if (fabs(GInput.cursorY) > 0.2)
      _dive += GInput.cursorY * deltaT * 2;
    */ 
  }

  // user dragging some units?
  //if (_parent->IsDragging())
    //turnOnEdge = true;

  if (turnOnEdge || _dragging || (_allowMoveOnEdge && moveOnEdge))
  {
    // move on screen edges
    float mouseX = 0.5 + GInput.cursorX * 0.5;
    float mouseY = 0.5 + GInput.cursorY * 0.5;

    saturate(mouseX,0,1);
    saturate(mouseY,0,1);

    float x = 0;
    float y = 0;

    // distance from edge to trigger a move
    float dis = 0.1;
    float max = exp(dis * 100.0f);

    float dif = dis - mouseX;
    if (dif > 0)
      x = -exp(dif * 100.0f) / max;
    else
    {
      dif = mouseX - (1 - dis);
      if (dif > 0)
        x = exp(dif * 100.0f) / max;
    }

    dif = dis - mouseY;
    if (dif > 0)
      y = exp(dif * 100.0f) / max;
    else
    {
      dif = mouseY - (1 - dis);
      if (dif > 0)
        y = -exp(dif * 100.0f) / max;
    }

    // wait for mouse to re-enter clear space before allowing another screen edge detection
    if (turnOnEdge)// || GInput.keys[DIK_LCONTROL])
    {
      float maxSpeed = 1;
      if (GInput.GetAction(UABuldTurbo) > 0.5f)
        maxSpeed *= _speedTurboMultiplier;

      headingChange -= x * _speedRotate * maxSpeed * deltaT;
      diveChange -= y * _speedElevation * maxSpeed * deltaT;
    }
    else
    {
      if (_dragging && x == 0 && y == 0)
        _dragging = false;   

      if (!_dragging)
      {
        float maxSpeed = 30;
        if (GInput.GetAction(UABuldTurbo) > 0.5f)
          maxSpeed *= _speedTurboMultiplier;

        Matrix3 speedOrient;
        speedOrient.SetUpAndDirection(VUp, FutureVisualState().Direction());
        Vector3 speedWanted = speedOrient * Vector3(x * _speedLeftRight,0,y * _speedFwdBack);
        position += speedWanted * maxSpeed * deltaT;
      }
    }
  }

  //-!

  // keyboard movement
  // 
  float maxSpeed = 25;
  if (GInput.GetAction(UABuldTurbo) > 0.5f)
    maxSpeed *= _speedTurboMultiplier;
  else if (GInput.keys[DIK_LCONTROL]>0)
    maxSpeed /= _speedTurboMultiplier;

  // heading / dive by keyboard
  float headSpeed = GInput.keys[DIK_NUMPAD4] - GInput.keys[DIK_NUMPAD6];
  float diveSpeed = GInput.keys[DIK_NUMPAD2] - GInput.keys[DIK_NUMPAD8];

  headingChange += headSpeed*2*deltaT;
  diveChange += diveSpeed*2*deltaT;
  
  // up/down
  float cursorZ = cursorOverCamera ? GInput.cursorMovedZ : 0;
  float up = (GInput.GetAction(UAHeliUp) - GInput.GetAction(UAHeliDown) + cursorZ) * deltaT * _speedUpDown * maxSpeed / 1.5;

  // fwd/back/left/right
  float forward =
  (
    //(GInput.GetAction(UABuldMoveForward)-GInput.GetAction(UABuldMoveBack)) +  // this is moving mouse forward and back
    (GInput.GetAction(UAMoveForward)-GInput.GetAction(UAMoveBack))
  ) * _speedFwdBack;

  float aside =
  (
    //(GInput.GetAction(UABuldMoveRight)-GInput.GetAction(UABuldMoveLeft)) +
    (GInput.GetAction(UATurnRight)-GInput.GetAction(UATurnLeft))
  ) * _speedLeftRight;

#if _SPACEMOUSE
  //SpaceMouse
  aside += GInput.spaceMouseAxis[0]*deltaT*10.0;
  forward += GInput.spaceMouseAxis[2]*deltaT*10.0;
  up += GInput.spaceMouseAxis[1]*deltaT*10.0;

  diveChange -= GInput.spaceMouseAxis[3]*deltaT;
  headingChange += GInput.spaceMouseAxis[4]*deltaT;

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
  _isMoving = forward != 0 || aside != 0 || up != 0;

#if _VBS2
  if (_lockToTarget && _isMoving)
  {
    if (!_attachedTo)
    {
      UnLock();
      return;
    }
    
    Vector3 dir = Vector3(VRotate,_attachedTo->GetInvTransform(),Direction());
    
    float angle = atan2(dir.Z(), dir.X());
    float heading = angle + PI/2;

    float xzSize = dir.SizeXZ();
    float dive = atan2(dir.Y(),xzSize);
    float dis = _attachedOffset.Size() + -up * deltaT * 10 * MaxSpeed() / MAX_CAMERA_SPEED;

    heading += aside * deltaT * MaxSpeed() / MAX_CAMERA_SPEED;
    dive += -forward * deltaT * MaxSpeed() / MAX_CAMERA_SPEED;

    saturate(dive,-H_PI/2*0.9,+H_PI/2*0.9);
    saturate(dis,1,200);

    Matrix3 camOrient = Matrix3(MRotationY,heading)*Matrix3(MRotationX,dive);
    Vector3 newPos = camOrient * VForward * dis;

    /*
    Vector3 worldPos = PositionModelToWorld(newPos);
    float minCamY = GLandscape->SurfaceY(worldPos.X(),worldPos.Z());
    minCamY += 0.1;
    if( worldPos.Y()<minCamY ) newPos[1] += minCamY - worldPos[1];
    */

    _attachedOffset = newPos;
    return;
  }

  //VBS added a factor for higher camera altitude
  float speedHeightFactor =  (Position().Y() - GLandscape->SurfaceYAboveWater(Position().X(), Position().Z())) / 50.0f;
  saturate(speedHeightFactor, 0.2f, 10.0f);
  float maxSpeed = MaxSpeed() * speedHeightFactor;

#endif

  Matrix3 speedOrient;
  speedOrient.SetUpAndDirection( VUp, FutureVisualState().Direction() );
  Vector3 speedWanted = speedOrient * Vector3(aside,0,forward);
  position += speedWanted * maxSpeed * deltaT;
  //-!

  float height = FutureVisualState().Position().Y() - GLandscape->SurfaceYAboveWater(FutureVisualState().Position().X(), FutureVisualState().Position().Z());
  if (height < 0.5 && up < 0) up = 0;
  position[1] = GLandscape->SurfaceYAboveWater(position[0], position[2]) + height + up;

  // final calculation of transformation
  Matrix3 rotY(MRotationY, -headingChange);
  Matrix3 rotX(MRotationX, diveChange);
  Matrix3 rot = FutureVisualState().Orientation().InverseRotation() * rotY;
  rot = rot.InverseRotation() * rotX;
  
  Matrix4 moveTrans;
  moveTrans.SetOrientation(rot);
  moveTrans.SetPosition(position);
  SetTransform(moveTrans); // finally apply move
  //-!
}

void MissionEditorCamera::UnLock()
{
  ResetTargets();
}

void MissionEditorCamera::LockTo(Object *lockTo)
{
  if (!lockTo) return;
  _lockToTarget = true;
  SetTarget(lockTo); // keeps camera looking at the target
  Commit(0);
#if _VBS2
  AttachTo(lockTo, _target.PositionAbsToRel(Position()));
#endif
}

void MissionEditorCamera::LookAt(Object *target, bool lockCamera)
{
  // TODO: put this in the config
  Vector3 pos(0,5,-15);
  
  // set new camera target
  SetTarget(target);
  Commit(0);

  if (lockCamera)
  {
    _lockToTarget = true;
#if _VBS2
    AttachTo(target, pos);
#endif
  }
  else
  {
#if _VBS2
    Detach();
#endif
    const CameraTarget &camTgt = GetTarget();  
    SetPos(camTgt.PositionRelToAbs(pos));
    Commit(0);
    _resetTargets = true; // reset to no target after 1 simulation loop
  }
}

void MissionEditorCamera::LookAt(Vector3Par target)
{
  // TODO: put this in the config
  Vector3 pos(0,5,-15);

  // allow the camera to reposition
#if _VBS2
  Detach();
#endif

  SetTarget(target);
  Commit(0);
  const CameraTarget &camTgt = GetTarget();  
  SetPos(camTgt.PositionRelToAbs(pos));
  Commit(0);
  _resetTargets = true; // reset to no target after 1 simulation loop
}

void MissionEditorCamera::DrawSelectedIcon(Vector3Val pos, PackedColor color)
{
  DrawIcon(pos, _selectedCursor, _selectedCursorWidth, _selectedCursorHeight, color, true);
}

#define MAX_DIS (150 * 150)

bool MissionEditorCamera::DrawIcon(Vector3Val pos, Texture *texture, float width, float height, PackedColor color, bool shadow)
{
  const Camera *camera = GScene->GetCamera();
  Vector3 dir = pos - camera->Position();

  float coef = 0.07;

  Matrix4Val camInvTransform=FutureVisualState().GetInvTransform();
  Vector3Val dirCam = camInvTransform.Rotate(dir);

  const int w3d = GEngine->Width();
  const int h3d = GEngine->Height();

  float xScreen = 0, yScreen = 0;
  if (dirCam.Z() > 0)
  {
    // crude scale depending on distance - TODO: move into config
    float dist = MAX_DIS - pos.Distance2(camera->Position());
    saturate(dist,1,MAX_DIS);
    width = width / 2 + ((width - width / 2) * dist / MAX_DIS);
    height = height / 2 + ((height - height / 2) * dist / MAX_DIS);

    float invZ= 1/dirCam.Z();
    xScreen = 0.5 * (1.0 + dirCam.X() * invZ * camera->InvLeft() - width * coef);
    yScreen = 0.5 * (1.0 - dirCam.Y() * invZ * camera->InvTop() - height * coef);

    Draw2DPars pars;
    pars.mip = GEngine->TextBank()->UseMipmap(texture,0,0);
    pars.spec = NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog;
    pars.SetU(0,1);
    pars.SetV(0,1);

    Rect2DAbs rect;
    rect.x = xScreen*w3d;
    rect.y = yScreen*h3d;
    rect.w = width*coef*w3d;
    rect.h = height*coef*h3d;

    // on screen?
    if (rect.x > -rect.w && rect.x < w3d && rect.y > -rect.h && rect.y < h3d)
    {
      // shadow
      if (shadow)
      {
        rect.x += 1;
        rect.y += 1;

        PackedColor colorBlack=PackedColorRGB(PackedBlack, color.A8());
        pars.SetColor(colorBlack);
        GEngine->Draw2D(pars,rect);

        rect.x -= 1;
        rect.y -= 1;
      }

      pars.SetColor(color);
      GEngine->Draw2D(pars,rect);
      return true;
    }
  }
  return false;
}

void MissionEditorCamera::DrawLine(Vector3Val from, Vector3Val to, PackedColor color, bool shadow)
{
  const Camera *camera = GScene->GetCamera();
  Vector3 dirFrom = from - camera->Position();
  Vector3 dirTo = to - camera->Position();

  if (from.Distance2(camera->Position()) > MAX_DIS) return;

  const int w3d = GEngine->Width();
  const int h3d = GEngine->Height();

  Matrix4Val camInvTransform = camera->GetInvTransform();
  Vector3 dirCam;
  float invZ;

  dirCam = camInvTransform.Rotate(dirFrom);
  invZ = 1/dirCam.Z();
  float xScreenFrom = 0.5 * (1.0 + dirCam.X() * invZ * camera->InvLeft()) * w3d;
  float yScreenFrom = 0.5 * (1.0 - dirCam.Y() * invZ * camera->InvTop()) * h3d;

  dirCam = camInvTransform.Rotate(dirTo);
  invZ = 1/dirCam.Z();
  float xScreenTo = 0.5 * (1.0 + dirCam.X() * invZ * camera->InvLeft()) * w3d;
  float yScreenTo = 0.5 * (1.0 - dirCam.Y() * invZ * camera->InvTop()) * h3d;

  Line2DAbs line;
  line.beg = Point2DAbs(xScreenFrom, yScreenFrom);
  line.end = Point2DAbs(xScreenTo, yScreenTo);  
  if (shadow) 
  {
    Line2DAbs lineShadow = line;
    lineShadow.beg.x++;
    lineShadow.beg.y++;
    lineShadow.end.x++;
    lineShadow.end.y++;
    PackedColor colorShadow = PackedBlack;
    GEngine->DrawLine(lineShadow, colorShadow, colorShadow);    
  }
  GEngine->DrawLine(line, color, color);
}

#endif // _ENABLE_EDITOR2
