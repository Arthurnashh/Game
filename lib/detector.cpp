#include "wpch.hpp"

#include "detector.hpp"
#include <El/Common/randomGen.hpp>
#include "AI/ai.hpp"
#include "person.hpp"
#include "global.hpp"
#include "landscape.hpp"
#include "world.hpp"
#include "fileLocator.hpp"

#include "dynSound.hpp"

#include "arcadeTemplate.hpp"

#include "gameStateExt.hpp"
#include "paramArchiveExt.hpp"

#include "Network/network.hpp"
//#include "strIncl.hpp"
#include "stringtableExt.hpp"

#include "UI/uiActions.hpp"
#include <Es/Common/delegate.hpp>

#include <El/Evaluator/express.hpp>

#include "Cloth/ClothObject.h"


FlagType::FlagType(ParamEntryPar param)
:base(param)
{
  ParamEntryVal config = Pars>>"CfgCloth">>"flag";
  
  _clothType = new ClothObjectType;
  _clothType->Init(config);
}

void FlagType::InitShape()
{
  base::InitShape();

  ParamEntryVal par = *_par;

  _fabric.Init(_shape, (par >> "selectionFabric").operator RString(), NULL);
  // default size in case everything else fails
}


void FlagType::InitFabric()
{
  if (_shape->NLevels()<=0) return;
  ShapeUsed shape = _shape->Level(0);
  int selIndex = _fabric.GetSelection(0);
  if (selIndex>=0)
  {
    ShapeUsedGeometryLock<> lock(_shape,0);
    
    // scan points of selection
    const NamedSelection::Access sel(shape->NamedSel(selIndex),_shape,shape);

    // flag base
    // calculate base 
    float minX = +1e10, minY = +1e10;
    float maxX = -1e10, maxY = -1e10;
    for (int i=0; i<sel.Size(); i++)
    {
      // for each point:
      int index = sel[i];
      Vector3Val val = shape->Pos(index);
      saturateMin(minX,val.X());
      saturateMin(minY,val.Y());
      saturateMax(maxX,val.X());
      saturateMax(maxY,val.Y());
    }
    float xSize = maxX-minX;
    float ySize = maxY-minY;

    _clothType->InitShape(minX,minY,xSize,ySize);
  }
}

void FlagType::DeinitShape()
{
  base::DeinitShape();
  _fabric.Deinit(_shape);
}
void FlagType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);
  _fabric.InitLevel(_shape,level);
}
void FlagType::DeinitShapeLevel(int level)
{
  _fabric.DeinitLevel(_shape,level);
  base::DeinitShapeLevel(level);
}


Object* FlagType::CreateObject(bool unused) const
{
  Object* v;

  if (!stricmp(_simName,"flag") ) v = new Flag(this,CreateObjectId());
  else 
  {
    LogF("Unknown vehicle simulation %s: type %s (FlagType called)",
      cc_cast(_simName), cc_cast(GetName())
      );
    v = NULL;
  }
  return v;
}



DEFINE_CASTING(Flag)

Flag::Flag(const EntityType *name, const CreateObjectId &id)
: base(name->GetShape(),name,id)
{
  // note: we need to simulate cloth quite often
  SetSimulationPrecision(0.1f);
  RandomizeSimulationTime();
  //SetTimeOffset(0.1f);
  SetTimeOffset(0);
}

bool Flag::InitAsync(Matrix4Par pos)
{
  if (_cloth)
  {
    return true;
  }
  if (_shape->CheckLevelLoaded(0,true))
  {
    Init(pos, true);
    // after we initialized the flag, it would be strange if there is no cloth
    // this probably means model is wrong
    DoAssert(_cloth);
    return true;
  }
  return false;
}


void Flag::Init(Matrix4Par pos, bool init)
{
  // make sure level 0 is loaded so that we can test the selection
  ShapeUsed level0 = _shape->Level(0);
  if (Type()->_fabric.GetSelection(0)>=0)
  {
    unconst_cast(Type())->InitFabric();
    _cloth = new ClothObject();
    _cloth->Init(*Type()->_clothType,pos);
  }
}

Flag::~Flag()
{
  _cloth.Free();
}

void Flag::SetFlagTexture(Texture *texture)
{
  if (_texture != texture)
  {
    _texture = texture;
  }
}

void Flag::FlagSimulate(Matrix4Par pos, float deltaT, SimulationImportance prec)
{
  base::Simulate(deltaT,prec);
  if (_cloth)
  {
    Vector3 wind = GLandscape->GetWindSpeed();

    // calculate world space transformation of flag
    Vector3 inertia = VZero; // global external force 
    _cloth->Simulate(*Type()->_clothType,pos,FutureVisualState().Speed(),deltaT,wind,inertia,prec);
  }
}


void Flag::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  // when we are passed setEngineStuff=true, it means we should not do any shape animation
  // this is used in Scene.AdjustComplexity to check proxy complexity
  if (_cloth && !setEngineStuff)
  {
    // we need to know world space coordinates of the flag
    // scan cloth particles and animates points of flag animation as appropriate
    ShapeUsed shape = _shape->Level(level);

    int selIndex = Type()->_fabric.GetSelection(level);
    if (selIndex>=0)
    {
      // scan points of selection
      const NamedSelection::Access sel(shape->NamedSel(selIndex),_shape,shape);

      float xSize = Type()->_clothType->GetSizeX();
      float ySize = Type()->_clothType->GetSizeY();
      float xMin = Type()->_clothType->GetMinX();
      float yMin = Type()->_clothType->GetMinY();
      float invXSize = 1/xSize;
      float invYSize = 1/ySize;

      //Matrix4 worldToModel = animContext.GetVisualState().cast<Flag>().GetInvTransform();
      Matrix4 worldToModel = _cloth->WorldPosUsed().InverseScaled(); // animContext.GetVisualState().cast<Flag>().GetInvTransform();

      for (int i=0; i<sel.Size(); i++)
      {
        // for each point:
        // estimate where in flag it is
        // 
        int index = sel[i];
        Vector3Val val = shape->Pos(index);

        float xIndexF = (val.X()-xMin)*invXSize;
        float yIndexF = (val.Y()-yMin)*invYSize;

        // get simulation result for point xIndexF, yIndexF
        Vector3 pos = _cloth->GetPosition(xIndexF,yIndexF);
        Vector3 norm = _cloth->GetNormal(xIndexF,yIndexF);

        Vector3 mpos = worldToModel.FastTransform(pos);
        Vector3 mnorm = worldToModel.Rotate(norm);

        animContext.SetPos(shape, index) = mpos;
        animContext.SetNorm(shape, index).Set(mnorm);

      }
      animContext.InvalidateNormals(shape);
    }
  }

  Type()->_fabric.SetTexture(animContext, _shape, level, _texture);

  base::Animate(animContext, level, setEngineStuff, this, dist2);
}

void Flag::Deanimate(int level, bool setEngineStuff)
{
  base::Deanimate(level, setEngineStuff);
}

FlagCarrierType::FlagCarrierType( ParamEntryPar param )
:base(param)
{
}
void FlagCarrierType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _animSpeed = 0.5;
  //_invAnimSpeed = 1.0 / _animSpeed;

}
void FlagCarrierType::InitShape()
{
  base::InitShape();
}
void FlagCarrierType::DeinitShape()
{
  base::DeinitShape();
}

void FlagCarrierType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);
  if (_shape->GetSkeleton())
  {
    // shape initialization
    ShapeUsed shape = _shape->Level(level);
    _shape->GetSkeleton()->PrepareProxyBones(_shape, shape.InitShape());
  }
}
void FlagCarrierType::DeinitShapeLevel(int level)
{
  if (_shape->GetSkeleton())
  {
    // shape deinitialization
    ShapeUsed shape = _shape->Level(level);
    _shape->GetSkeleton()->FinishProxyBones(_shape, shape.InitShape());
  }
  base::DeinitShapeLevel(level);
}


bool FlagCarrierType::IsSimulationRequired() const
{
  return true;
}
bool FlagCarrierType::IsSoundRequired() const
{
  return false;
}

AnimationSource *FlagCarrierType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
  if (!strcmpi(source,"flag")) return _animSources.CreateAnimationSource(&FlagCarrier::GetCtrlFlag);
  
  return base::CreateAnimationSource(type,source);
}


Object* FlagCarrierType::CreateObject(bool unused) const
{ 
  return new FlagCarrier(this);
}


DEFINE_CASTING(FlagCarrier)

FlagCarrier::FlagCarrier(const EntityAIType *type)
: base(type)
{
  _phase = _phaseWanted = 0;

  // the flag is moving, but the flagpole itself is not
  _static=true;

  // Default precision is too low, take at least 30FPS.
  SetSimulationPrecision(0.03f);
  RandomizeSimulationTime();
  SetTimeOffset(0.03f);
}

FlagCarrier::~FlagCarrier()
{
}

AnimationStyle FlagCarrier::IsAnimated(int level) const
{
  return NotAnimated;
}

Texture *FlagCarrier::GetFlagTexture()
{
  return _phase>=1.0f ? NULL : _flagTexture;
}

Texture *FlagCarrier::GetFlagTextureInternal()
{
  return _flagTexture;
}

void FlagCarrier::SetFlagTexture(RString name)
{
  _flagTexture = NULL;
  if (name.GetLength() > 0)
  {
    RString FindPicture(RString name);

    RString fullName = FindPicture(name);
    if (fullName.GetLength() > 0)
    {
      fullName.Lower();
      _flagTexture = GlobLoadTexture(fullName);      
    }
  }
  else
  {
    _phaseWanted = _phase = 1.0f;
  }
}

bool CheckSupply(EntityAI *vehicle, EntityAI *parent, SupportCheckF check, float limit, bool now);

void FlagCarrier::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  if (_flagTexture)
  {
    if (_phase>0)
    {
      UIAction action(new ActionBasic(ATReturnFlag, this));
      int index = GetType()->GetSupplyPointIndex();
      if (index >= 0) action._memoryPoints.Add(index);
      add(action);
    }
    if (_phase<1)
    {
      UIAction action(new ActionBasic(ATTakeFlag, this));
      int index = GetType()->GetSupplyPointIndex();
      if (index >= 0) action._memoryPoints.Add(index);
      add(action);
    }
  }
  base::GetActions(add, unit);
}

void FlagCarrier::LowerFlag(float lowered)
{
  DoAssert(IsLocal());
  _phaseWanted = lowered;
}

float FlagCarrier::GetFlagLowered() const
{
  return _phase;
}

void FlagCarrier::Simulate( float deltaT, SimulationImportance prec )
{
  base::Simulate(deltaT, prec);
  SimulatePost(deltaT, prec);
  
  {
    if (_phaseWanted>_phase)
    {
      _phase += Type()->_animSpeed * deltaT;
      if (_phase > _phaseWanted) _phase = _phaseWanted;
    }
    if (_phaseWanted<_phase)
    {
      _phase -= Type()->_animSpeed * deltaT;
      if (_phase < _phaseWanted) _phase = _phaseWanted;
    }
  }
}

LSError FlagCarrier::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))

  if (ar.IsSaving())
  {
    RString name = _flagTexture ? _flagTexture->Name() : "";
    CHECK(ar.Serialize("flagTexture", name, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    RString name;
    CHECK(ar.Serialize("flagTexture", name, 1))
    _flagTexture = name.GetLength() > 0 ? GlobLoadTexture(name) : NULL;
  }

  CHECK(ar.Serialize("phase", _phase, 1, 0.0f))
  CHECK(ar.Serialize("phaseWanted", _phaseWanted, 1, 0.0f))
  return LSOK;
}

#define FLAG_CARRIER_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateFlag)

DEFINE_NETWORK_OBJECT(FlagCarrier, base, FLAG_CARRIER_MSG_LIST)

#define UPDATE_FLAG_MSG(MessageName, XX) \
  XX(MessageName, RString, flagTexture, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Flag texture"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, float, phaseWanted, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Target position"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \

DECLARE_NET_INDICES_EX_ERR(UpdateFlag, UpdateVehicleAI, UPDATE_FLAG_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateFlag, UpdateVehicleAI, UPDATE_FLAG_MSG, NoErrorInitialFunc)

NetworkMessageFormat &FlagCarrier::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_FLAG_MSG(UpdateFlag, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError FlagCarrier::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateFlag)

      TRANSF(phaseWanted)
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float FlagCarrier::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    {
      error += base::CalculateError(ctx);
      
      PREPARE_TRANSFER(UpdateFlag)

      RString name = _flagTexture ? _flagTexture->Name() : "";
      ICALCERRE_NEQSTR(flagTexture, name, ERR_COEF_MODE)
      ICALCERR_ABSDIF(float, phaseWanted, ERR_COEF_VALUE_MAJOR)
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

