#include "wpch.hpp"

/**
@file
Buldozer viewer special classes
*/

#if _ENABLE_BULDOZER
#include "world.hpp"
#include "landscape.hpp"
#include <El/ParamFile/paramFile.hpp>
#include "vehicleAI.hpp"
#include "AI/ai.hpp"
#include "arcadeTemplate.hpp"
#include "global.hpp"
#include "visibility.hpp"
#include "keyInput.hpp"
#include "dikCodes.h"
#include "gameStateExt.hpp"
#include "fileLocator.hpp"

/// object being viewed
class ObjectViewer: public Entity
{
  typedef Vehicle base;

  float _animPhase,_animSpeed;
  Ref<AnimationRT> _rtmAnimation;

  public:
  ObjectViewer( LODShape *shape, const EntityType *type);
  ~ObjectViewer();

  void Simulate( float deltaT, SimulationImportance prec );

  virtual bool PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level);

  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );

  AnimationStyle IsAnimated( int level ) const {return AnimatedGeometry;}
  
  void PrepareProxiesForDrawing(    
    Object *rootObj, int rootLevel, const AnimationContext &animContext, const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so);
  virtual void AnimateBoneMatrix( Matrix4 &mat, const ObjectVisualState &vs, int level, SkeletonIndex boneIndex) const;

  float GetPhase() const {if (_animSpeed<0.01) return _animPhase;else return -1;}
  void SetPhase( float phase ) {_animPhase=phase,_animSpeed=0;}
  
  void SetRTMName(RString rtmName)
  {
    //_rtmName = rtmName;
    if (rtmName == RString(""))
    {
      _rtmAnimation.Free();
    }
    else
    {
      // If skeleton is not present then create the skeleton source and recreate the model
      if (!_shape->GetSkeleton())
      {

        // Load the animation, skeleton will contain used bones
        AnimationRTName artname;
        artname.name = rtmName;
        artname.skeleton = new Skeleton();
        artname.reversed = false;
        artname.streamingEnabled = false;
        AnimationRT a(artname);

        // Create the skeleton source not loaded from file
        FindArrayKey<SBoneRelation> structure;
        structure.Realloc(artname.skeleton->NBones());
        structure.Resize(artname.skeleton->NBones());
        for (int i = 0; i < artname.skeleton->NBones(); i++)
        {
          structure[i]._bone = artname.skeleton->GetBone(SetSkeletonIndex(i));
          structure[i]._parent = RStringB("");
        }
        Ref<SkeletonSource> ss = SkeletonSources.New(RStringB("$BuldozerCustomSkeleton"));
        ss->Init(false, structure);

        // Create the skeleton from the skeleton source to be available for the source
        Ref<Skeleton> skeleton = Skeletons.New(RStringB("$BuldozerCustomSkeleton"));

        // Recreate the shape using new skeleton source
        GWorld->ReloadViewer(_shape->Name(), "house", ss);
      }

      AnimationRTName name;
      name.skeleton = _shape->GetSkeleton();
      if (name.skeleton.NotNull())
      {
        name.name = rtmName;
        name.reversed = false;
        name.streamingEnabled = false;
        _rtmAnimation = new AnimationRT(name);
      }
    }
  }

  void SetControllerPhase(RString controller, float phase, float &minVal, float &maxVal);
  void GetControllers(FindArrayRStringCI &controllers) const;
  
  INHERIT_SOFTLINK(ObjectViewer,base);
};

ObjectViewer::ObjectViewer( LODShape *shape, const EntityType *type)
:base(shape,type,CreateObjectId())
{
  _animPhase =0;
  _animSpeed = 0.5;
}

ObjectViewer::~ObjectViewer()
{
}

void ObjectViewer::SetControllerPhase(RString controller, float phase, float &minVal, float &maxVal)
{
  // find controller based on the name
  const EntityType *type = Type();
  int index = type->_animSources.FindName(controller);
  if (index<0) return;
  // corresponding animationType
  const AnimationHolder &animations = Type()->GetAnimations();
  type->_animSources[index]->SetPhaseWanted(this,phase);
  // scan all animations connected with given controller
  minVal = FLT_MAX, maxVal = -FLT_MAX;
  for (int i=0; i<animations.Size(); i++)
  {
    if (type->_animSources[i]==type->_animSources[index])
    {
      const AnimationType *animType = animations[i];
      float minValAnim, maxValAnim;
      animType->MinMaxValue(minValAnim,maxValAnim);
      saturateMin(minVal,minValAnim);
      saturateMax(maxVal,maxValAnim);
    }
  }
}

void ObjectViewer::GetControllers(FindArrayRStringCI &controllers) const
{
  controllers.Clear();
  const EntityType *type = Type();
  for (int i=0; i<type->_animSources.Size(); i++)
  {
    const RStringB &name = type->_animSources[i]->GetName();
    if (name.GetLength()<=0) continue;
    controllers.AddUnique(name);
  }
}

void ObjectViewer::AnimateBoneMatrix( Matrix4 &mat, const ObjectVisualState &vs, int level, SkeletonIndex boneIndex) const
{
  const AnimationHolder &animations = Type()->GetAnimations();
  if ((animations.Size() == 0) && (boneIndex != SkeletonIndexNone))
  {
    if (_rtmAnimation.NotNull())
    {
      _rtmAnimation->Matrix(mat, _animPhase, boneIndex);
      return;
    }
  }
  base::AnimateBoneMatrix(mat, vs, level, boneIndex);
}

void ObjectViewer::PrepareProxiesForDrawing(    
  Object *rootObj, int rootLevel, const AnimationContext &animContext, const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so)
{
  const VisualState &vs = RenderVisualState();
  float dist2 = so->distance2;
  // draw all proxy objects
  const EntityType *entType = Type();
  for (int i=0; i<entType->GetProxyCountForLevel(level); i++)
  {
    const ProxyObjectTyped &proxy = entType->GetProxyForLevel(level,i);

    // check if the corresponding proxy face is hidden
    int section = entType->GetProxySectionForLevel(level,i);
    if (section>=0)
    {
      Assert(_shape->IsLevelLocked(level));
      const Shape *shape = _shape->GetLevelLocked(level);

      int spec = animContext.GetSection(shape, section).Special();
      if (spec & IsHidden) continue;
    }

    Object *obj = proxy.obj;
    SkeletonIndex boneIndex = entType->GetProxyBoneForLevel(level, i);

    // transformation matrix
    Matrix4 proxyTransform = obj->RenderVisualState().Transform();
    AnimateBoneMatrix(proxyTransform,vs,level,boneIndex);
    Matrix4Val pTransform = transform * proxyTransform;

    // LOD detection
    LODShape *pshape = proxy.obj->GetShape();
    if (!pshape) continue;
    int level = GScene->LevelFromDistance2(
      pshape, dist2, pTransform.Scale());
    if (level == LOD_INVISIBLE) continue;

    GScene->ProxyForDrawing(proxy.obj, rootObj, level, rootLevel, clipFlags, dist2, pTransform);
  }
}

/// viewer camera

class CameraViewer: public Vehicle
{
  OLink(ObjectViewer) _tgt;
  float _camFOV;
  typedef Vehicle base;

  public:
  CameraViewer( ObjectViewer *tgt, LODShape *shape, RString name );
  ~CameraViewer();

  ObjectViewer *GetTarget() const {return _tgt;}
  void SetTarget( ObjectViewer *tgt ) {_tgt=tgt;}

  virtual void Simulate( float deltaT, SimulationImportance prec );
  virtual void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  ) override;
  virtual void DrawShadowVolume(int cb, LODShape *shape, int level, const PositionRender &frame, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const {return false;}

  void LimitVirtual
  (
    CameraType camType, float &heading, float &dive, float &fov
  ) const;
  void InitVirtual
  (
    CameraType camType, float &heading, float &dive, float &fov
  ) const;
  bool IsContinuous( CameraType camType ) const {return true;}
  float GetCameraFOV() const { return _camFOV; }
};

CameraViewer::CameraViewer( ObjectViewer *tgt, LODShape *shape, RString name )
:base(shape,VehicleTypes.New(name),CreateObjectId()),
_tgt(tgt),
_camFOV(1.0f)
{
}

CameraViewer::~CameraViewer()
{
}

static Vector3 InitCamPos=Vector3(0,0,-10);

extern float GetBuldozerMouseSpeed();
void CameraViewer::Simulate( float deltaT, SimulationImportance prec )
{
  float scale = GInput.keys[DIK_LCONTROL] || GInput.keys[DIK_RCONTROL] ? 0.1 : 1;
  float moveX = 0;
  float moveY = 0;
  float moveZ = 0;

  deltaT*=scale;

  float turnY = 0;

  if (GInput.mouseL)
  {
    moveX = (GInput.GetAction(UABuldMoveRight) - GInput.GetAction(UABuldMoveLeft)) * 5*deltaT/3*GetBuldozerMouseSpeed()*Input::MouseRangeFactor;
    moveZ = (GInput.GetAction(UABuldMoveForward) - GInput.GetAction(UABuldMoveBack)) * 5*deltaT*GetBuldozerMouseSpeed()*Input::MouseRangeFactor;
  }

  moveX += (GInput.keys[DIK_RIGHT]-GInput.keys[DIK_LEFT])*4*deltaT;
  moveY += (GInput.keys[DIK_UP]-GInput.keys[DIK_DOWN])*4*deltaT;

  moveZ +=
  (
    GInput.keys[DIK_E]*8+
    GInput.keys[DIK_W]*2-
    GInput.keys[DIK_S]*2
  )*deltaT;
  moveY +=
  (
    GInput.keys[DIK_Q]*2-
    GInput.keys[DIK_Z]*2
  )*deltaT;
  moveX +=
  (
    GInput.keys[DIK_C]*2-
    GInput.keys[DIK_X]*2
  )*deltaT;
  turnY +=
  (
    GInput.keys[DIK_D]*2-
    GInput.keys[DIK_A]*2
  )*deltaT;

#if _SPACEMOUSE
  //SpaceMouse
  moveX += GInput.spaceMouseAxis[0]*deltaT*10.0;
  moveZ += GInput.spaceMouseAxis[2]*deltaT*10.0;
  moveY += GInput.spaceMouseAxis[1]*deltaT*10.0;

//  turnY -= GInput.spaceMouseAxis[3]*deltaT;
//  float turnX = GInput.spaceMouseAxis[4]*deltaT;
#endif

  Vector3 pos = FutureVisualState().Position();
  Matrix3 orient = FutureVisualState().Orientation();

  pos += moveX*FutureVisualState().DirectionAside();
  pos += moveY*FutureVisualState().DirectionUp();
  pos += moveZ*FutureVisualState().Direction();

  Matrix3 rot(MRotationY,-turnY);
  orient = orient*rot;
  if (GInput.keys[DIK_NUMPAD5])
  {
    if (_tgt)
    {
      pos = _tgt->FutureVisualState().Position()+InitCamPos;
    }
    _camFOV=1.0f;
    orient.SetIdentity();
  }

  SetPosition(pos);
  SetOrientation(orient);
  //zoom
  static float zoomSpeed = 0.9;
  static float cursMovedZSpeed = 0.03f;
  _camFOV += (GInput.GetAction(UABuldZoomOut)-GInput.GetAction(UABuldZoomIn)) * deltaT * zoomSpeed;
  //_camFOV += GInput.cursorMovedZ * cursMovedZSpeed; //when animation present, it zooms and animate at the same time (cannot be allowed)
  float dummy = 0;
  LimitVirtual(GWorld->GetCameraType(), dummy, dummy, _camFOV);
}

void CameraViewer::Draw(
  int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
  , int drawListId
#endif
)
{
}

void CameraViewer::DrawShadowVolume(int cb, LODShape *shape, int level, const PositionRender &frame, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
, int drawListId
#endif
)
{
}

bool ObjectViewer::PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level)
{
  const AnimationHolder &animations = Type()->GetAnimations();
  if (animations.Size() == 0)
  {
    if (_rtmAnimation.NotNull())
    {
      ShapeUsed lShape = shape->Level(level);

//      // Set matrices size and set the items to zero (to be able later to determine the unset bones)
//      matrices.Resize(lShape->GetSubSkeletonSize());
//      for (int i = 0; i < matrices.Size(); i++)
//      {
//        matrices[i] = MZero;
//      }

      // It is important that the matrices hasn't been set yet - otherwise they
      // could be identities instead of zeroes and produce wrong addition in matrix blending
      _rtmAnimation->PrepareMatrices(matrices, _animPhase, 1, lShape,shape->GetSkeleton());

      // Find all bones with parents and set their matrix to the parent matrix (Example: facial bones)
      for (int i = 0; i < lShape->GetSubSkeletonSize(); i++)
      {
        SubSkeletonIndex ssi;
        SetSubSkeletonIndex(ssi, i);
        SkeletonIndex si = lShape->GetSkeletonIndexFromSubSkeletonIndex(ssi);
        
        // If the bone has got some parent then set the matrix to the parent matrix
        SkeletonIndex parent = shape->GetSkeleton()->GetParent(si);
        if (parent != SkeletonIndexNone)
        {
          if (matrices[i] == MIdentity)
          {
            const SubSkeletonIndexSet &ssis = lShape->GetSubSkeletonIndexFromSkeletonIndex(parent);
            if (ssis.Size() > 0)
            {
              matrices[i] = matrices[GetSubSkeletonIndex(ssis[0])];
            }
          }
        }
      }

      return true;
    }
  }
  base::PrepareShapeDrawMatrices(matrices, vs, shape, level);
  return true;
}

void ObjectViewer::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  ShapeUsed shape = _shape->Level(level);
  if (shape->IsAnimated() && (!_shape->GetSkeleton()))
  {
    shape->SetPhase(animContext, _animPhase, 0);
  }
}

void ObjectViewer::Deanimate( int level, bool setEngineStuff )
{
}

void ObjectViewer::Simulate( float deltaT, SimulationImportance prec )
{
  float scale = GInput.keys[DIK_LCONTROL] || GInput.keys[DIK_RCONTROL] ? 0.1 : 1;

  deltaT*=scale;

  _animPhase += deltaT*_animSpeed;
  _animPhase = fastFmod(_animPhase,1);

  // react to keys
  // mouse rotate object

  const float speedKeyb=2;
  const float speedMouse=0.02*GetBuldozerMouseSpeed();

  float speedX = 0;
  float speedY = 0;

  Matrix3 rot=M3Identity;

  if (GInput.mouseR)
  {
    speedX = (GInput.GetAction(UABuldMoveRight) - GInput.GetAction(UABuldMoveLeft)) * speedMouse*scale*Input::MouseRangeFactor;
    speedY = (GInput.GetAction(UABuldMoveForward) - GInput.GetAction(UABuldMoveBack)) * speedMouse*scale*2*Input::MouseRangeFactor;
  }

  speedX += (GInput.keys[DIK_NUMPAD4]-GInput.keys[DIK_NUMPAD6])*speedKeyb*deltaT;
  speedY += (GInput.keys[DIK_NUMPAD8]-GInput.keys[DIK_NUMPAD2])*speedKeyb*deltaT;

  if (_animSpeed>0 )
  {
    _animSpeed *=pow(2,deltaT*(GInput.keys[DIK_RBRACKET]-GInput.keys[DIK_LBRACKET]));
    saturate(_animSpeed,1e-3,10);
  }
  else
  {
    if (GInput.keys[DIK_RBRACKET]-GInput.keys[DIK_LBRACKET])
    {
      _animSpeed=0.5;
    }
  }

  // calculate heading and dive from current orientation
  float head = -atan2(FutureVisualState().Direction().X(),FutureVisualState().Direction().Z());
  float xzSize = FutureVisualState().Direction().SizeXZ();
  float dive = -atan2(FutureVisualState().Direction().Y(),xzSize);

#if _SPACEMOUSE
  //SpaceMouse
  speedY += GInput.spaceMouseAxis[3]*deltaT*5.0;
  speedX += GInput.spaceMouseAxis[4]*deltaT*5.0;
#endif

  head += speedX;
  dive += speedY;

  if (GInput.keys[DIK_NUMPAD5])
  {
    dive = head = 0;
  }

  saturate(dive,-H_PI/2*0.9999,+H_PI/2*0.9999);

  rot= Matrix3(MRotationY,head);
  rot= rot * Matrix3(MRotationX,dive);

  SetOrientation(rot);
}

void CameraViewer::LimitVirtual
(
  CameraType camType, float &heading, float &dive, float &fov
) const
{
  heading=AngleDifference(heading,0);
  saturate(dive,-H_PI*0.3,+H_PI*0.3);
  switch (camType)
  {
    case CamInternal:
      saturate(fov,0.01,2);
    break;
    default:
      base::LimitVirtual(camType,heading,dive,fov);
    break;
  }
}

void CameraViewer::InitVirtual
(
  CameraType camType, float &heading, float &dive, float &fov
) const
{
  base::InitVirtual(camType,heading,dive,fov);
}

extern bool ObjViewer;

void World::SetViewerPhase( float time )
{
  Object *cobj=_cameraOn;
  ObjectViewer *oldViewer=dynamic_cast<ObjectViewer *>(cobj);
  CameraViewer *camViewer=dynamic_cast<CameraViewer *>(cobj);
  if (camViewer) oldViewer = camViewer->GetTarget();
  if (oldViewer) oldViewer->SetPhase(time);
}

float World::GetViewerPhase() const
{
  Object *cobj=_cameraOn;
  ObjectViewer *oldViewer=dynamic_cast<ObjectViewer *>(cobj);
  CameraViewer *camViewer=dynamic_cast<CameraViewer *>(cobj);
  if (camViewer) oldViewer = camViewer->GetTarget();
  if (oldViewer) return oldViewer->GetPhase();
  return 0;
}

void World::SetViewerRTMName(RString rtmName)
{
  Object *cobj=_cameraOn;
  ObjectViewer *oldViewer=dynamic_cast<ObjectViewer *>(cobj);
  CameraViewer *camViewer=dynamic_cast<CameraViewer *>(cobj);
  if (camViewer) oldViewer = camViewer->GetTarget();
  if (oldViewer) oldViewer->SetRTMName(rtmName);
}

void World::SetViewerController(RString name, float value, float &minVal, float &maxVal)
{
  minVal = 0, maxVal = 1;
  ObjectViewer *oViewer = dynamic_cast<ObjectViewer *>(_cameraOn.GetLink());
  CameraViewer *camViewer=dynamic_cast<CameraViewer *>(_cameraOn.GetLink());
  if (camViewer) oViewer = camViewer->GetTarget();
  if (!oViewer) return;
  oViewer->SetControllerPhase(name,value,minVal,maxVal);
}
void World::GetViewerControllers(FindArrayRStringCI &controllers) const
{
  ObjectViewer *oViewer = dynamic_cast<ObjectViewer *>(_cameraOn.GetLink());
  CameraViewer *camViewer=dynamic_cast<CameraViewer *>(_cameraOn.GetLink());
  if (camViewer) oViewer = camViewer->GetTarget();
  if (!oViewer) return;
  oViewer->GetControllers(controllers);
}

void World::ReloadViewer(void *buf, int len, const char *classDesc, const char *filename, SkeletonSource *ss)
{
  //LogF("Start reload");
  ObjViewer=true;

  // create temporary type
  // check property for config class name
  QIStrStream sFile((char *)buf,len);

  // reversed or no?
  // how should I no before I load shape?
  ModelTarget mt;
  mt._bonesInterval = 60;
  Ref<LODShape> shape = new LODShape(sFile,filename,false,mt,ss);

  if (shape->GetSkeleton()/*shape->GetAnimations()*/)
  {
    shape->OptimizeRendering(VBStaticSkinned);
  }
  else if (shape->IsAnimated())
  {
    shape->OptimizeRendering(VBDynamic);
  }
  else
  {
    shape->OptimizeRendering(VBStatic);
  }

  //type->_shapeReversed);
  const char *viewClass="";
  if( shape->NLevels()>1 )
  {
    viewClass=shape->PropertyValue(".viewclass");
  }
  //LogF("Reload levels %d",shape->NLevels());
  //LogF("Reload class %s",viewClass);

  // check camera object
  Object *obj=_cameraOn;
  EntityAI *camAI=dyn_cast<EntityAI>(obj);
  const char *camName = "";
  if( camAI ) camName = camAI->GetType()->GetName();
  if( !strcmpi(camName,viewClass) )
  {
    sFile.seekg(0,QIOS::beg);
    if( camAI )
    {
      // if there is already an object with the same class, we can only reload shape
      const VehicleType *type=camAI->GetType();
      const_cast<VehicleType *>(type)->ReloadShape(sFile,filename);
      return;
    }
    else
    {
      Object *cobj=_cameraOn;
      ObjectViewer *oldViewer=dynamic_cast<ObjectViewer *>(cobj);
      CameraViewer *camViewer=dynamic_cast<CameraViewer *>(cobj);
      if (camViewer) oldViewer = camViewer->GetTarget();
      if (oldViewer)
      {
        // reload simple object
        Frame oldPos=oldViewer->GetFrameBase();

        DeleteVehicle(oldViewer); 
        // create a temporary type attached with the object
        Ref<EntityPlainType> type = new EntityPlainType(shape);
        Ref<ObjectViewer> obj=new ObjectViewer(shape,type);
        obj->SetTransform(oldPos.Transform());
        AddVehicle(obj);
        Object *cobj=obj;
        GetGameState()->VarSet("bis_buldozer_zero", GameValueExt(cobj), true, false, GWorld->GetMissionNamespace()); // what namespace?
        // camera deleted - reassign it
        if (camViewer)
        {
          // no change in camera
          camViewer->SetTarget(obj);
        }
        else
        {
          _cameraOn = obj.GetRef();
          GetGameState()->VarSet("bis_buldozer_cursor", GameValueExt(cobj), true, false, GWorld->GetMissionNamespace()); // what namespace?
          SetCameraType(CamInternal);
        }

        //_camType = _camTypeMain = CamGroup;
        return;
      }
    }
  }

  Vehicle *camVeh = dynamic_cast<Vehicle *>((Object *)_cameraOn);
  if( camVeh )
  {
    // note: camVeh need not be in Vehicle list
    DeleteVehicle(camVeh);
  }
  _vehicles.Clear();

  // if possible object is previewed on training island
  extern bool NoLandscape;
  if( !NoLandscape )
  {
    SwitchLandscape("Intro");
  }
  else
  {
    //LogF("Reset landscape");
    Reset();
  }

  float mid = GLandscape->GetLandRange()*GLandscape->GetLandGrid()*0.5;
  Vector3 cPos(mid,50,mid);

  if( !*viewClass )
  {
    // preview simple object
    Ref<EntityPlainType> type = new EntityPlainType(shape);
    Ref<ObjectViewer> obj=new ObjectViewer(shape,type);
    Ref<CameraViewer> cam=new CameraViewer(obj,shape,"ObjView");
    obj->SetPosition(cPos);
    cam->SetPosition(cPos+InitCamPos);
    AddVehicle(obj);
    GetGameState()->VarSet("bis_buldozer_zero", GameValueExt(obj.GetRef()), true, false, GWorld->GetMissionNamespace()); // what namespace?
    AddVehicle(cam);
    GetGameState()->VarSet("bis_buldozer_cursor", GameValueExt(cam.GetRef()), true, false, GWorld->GetMissionNamespace()); // what namespace?
    Object *cobj=cam;
    _cameraOn = cobj;
    SetCameraType(CamInternal);

    //LogF("End reload - simple ");
    return;
  }

  ParamEntryVal vehicles=Pars>>"CfgVehicles";
  if( !vehicles.FindEntry(viewClass) )
  {
    return;
  }

  if( (vehicles>>viewClass>>"reversed").GetInt()==0 )
  {
    // reload shape nonreversed
    shape=new LODShape(sFile,false);
    viewClass=shape->PropertyValue(".viewclass");
  }

  Fail("Vehicle preview not supported");
  // note: it is very unlikely tempBank can work
  // some functionality in EntityAIType is tied to VehicleTypes
  VehicleTypeBank tempBank;

  Ref<EntityType> vType=tempBank.New(viewClass);
  EntityAIType *type=dynamic_cast<EntityAIType *>(vType.GetRef());
  if( !type ) return;

  type->AttachShape(shape);


  // TODO: Detach shape after creating the vehicle

  // if possible vehicle is created from object
  ArcadeUnitInfo info;


  // TODO: use default side
  //info.side = type->_typicalSide;
  info.side = TWest;
  info.vehicle = type->GetName();
  info.type = type;
  info.position = cPos;

  TargetSide side = info.side;

  _mode = GModeArcade;
  SetEndMode(EMContinue);
  _endMissionCheated = false;
  _endFailed = false;
  EnableRadio();
  EnableSentences();

  Glob.header.playerSide = side;

  if (_ui) _ui->ResetHUD();

  AIGlobalInit();

  //InitNoCenters(t);

  GetSensorList()->UpdateAll();

  // new data ready - adapt texture cache

  //_engine->TextBank()->FlushTextures();

  _sensorList->UpdateAll();
  EntityAIFull *ai=dyn_cast<EntityAIFull, Object>(_cameraOn);
  if (_ui && ai)
  {
    _ui->ResetVehicle(ai);
  }

  //LogF("End reload");
}

void World::ReloadViewer(const char *filename, const char *classDesc, SkeletonSource *ss)
{
  QIFStreamB file;
  file.AutoOpen(filename);
  AutoArray<char> data;
  WriteAutoArrayChar writeData(data);
  file.Process(writeData);
  ReloadViewer(data.Data(),data.Size(),classDesc,filename,ss);
}

#endif
