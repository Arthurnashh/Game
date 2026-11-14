// Inventory item Magazine implementation

#include "../wpch.hpp"
#include "magazine.hpp"

#include "../fileLocator.hpp"
#include "../AI/ai.hpp"
#include "../person.hpp"
#include "../Network/network.hpp"

#include <El/Common/randomGen.hpp>


// class MagazineType

MagazineType::MagazineType(ParamEntryPar param)
: base(param)
{
  _modelRefCount = 0;
}

void MagazineType::InitShape()
{
  base::InitShape();

  // force ammo type to load shape
  // scan all modes
  if (_ammo) _ammo->VehicleAddRef(); // force loading model

  if (_modelName.GetLength()>0)
    _modelSpecial = Shapes.New(::GetShapeName(_modelName),ShapeNormal);
  else
    _modelSpecial.Free();

  if (_modelSpecial)
    _modelSpecial->AddLoadHandler(this);
}

void MagazineType::DeinitShape()
{
  if (_modelSpecial)
    _modelSpecial->RemoveLoadHandler(this);

  // allow used ammo type to release shape
  // scan all modes
  if (_ammo) _ammo->VehicleRelease(); // force loading model

  base::DeinitShape();
}

void MagazineType::LODShapeLoaded(LODShape *shape)
{
  if (_modelSpecial && shape == _modelSpecial)
  {
    const ParamEntry &par = *_par;
    _animFire.Init(shape, (par >> "selectionFireAnim").operator RString(), NULL);
  }
  else base::LODShapeLoaded(shape);
}

void MagazineType::LODShapeUnloaded(LODShape *shape)
{
  base::LODShapeUnloaded(shape);
}

void MagazineType::ShapeLoaded(LODShape *shape, int level)
{
  if (_modelSpecial && shape == _modelSpecial)
  {
    _animFire.InitLevel(shape, level);
  }
  else base::ShapeLoaded(shape, level);
}

void MagazineType::ShapeUnloaded(LODShape *shape, int level)
{
  if (_modelSpecial && shape == _modelSpecial)
  {
    _animFire.DeinitLevel(shape, level);
  }
  else base::ShapeUnloaded(shape, level);
}

void MagazineType::AmmoAddRef() const
{
  if (_modelRefCount++==0)
  {
    unconst_cast(this)->InitShape();
  }
}

void MagazineType::AmmoRelease() const
{
  if (--_modelRefCount==0)
  {
    unconst_cast(this)->DeinitShape();
  }
}

RStringB MagazineType::GetPictureName() const
{
  return _picName.GetLength() > 0 ? _picName : GetName();
}

void MagazineType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par, shape);

  _mass = par.ReadValue("weight",0.0f);

  _picName = par >> "picture";

  _scope = par >> "scope";
/*
  if (_scope == 0)
  {
    WarningMessage("Error: creating magazine %s with scope=private", name);
  }
*/

  _description = par >> "Library" >> "libTextDesc";
  _magazineType = par >> "type";
  _maxAmmo = par >> "count";
  _maxLeadSpeed = par >> "maxLeadSpeed";
  _initSpeed = par >> "initSpeed";
  _invInitSpeed = _initSpeed>0 ? 1.0f / _initSpeed : 1e10f;
  _reloadAction = ExtractActionName(par >> "reloadAction");
  _quickReload = par.ReadValue<bool>("quickReload",false);

  ConstParamEntryPtr entry = par.FindEntry("value");
  if (entry) _value = *entry;
  else _value = 1;

  entry = par.FindEntry("useAction");
  if (entry) _useAction = *entry;
  else _useAction = false;
  
  entry = par.FindEntry("useActionTitle");
  if (entry) _useActionTitle = *entry;
  else _useActionTitle = RString();

  _modelName = par >> "modelSpecial";

  RStringB ammo = par >> "ammo";
  if (ammo.GetLength() > 0)
  {
    Ref<EntityType> type = VehicleTypes.New(ammo);
    if (!type) RptF("No ammo class %s for %s", (const char *)ammo,cc_cast(par.GetContext()));
    _ammo = dynamic_cast<AmmoType *>(type.GetRef());
    if (type && !_ammo) RptF("Class %s is not ammo for %s", (const char *)ammo,cc_cast(par.GetContext()));
  }
  else
  {
    _ammo = NULL;
  }

  _tracersEvery = par.ReadValue("tracersEvery", 0);
  _lastRoundsTracer = par.ReadValue("lastRoundsTracer", 0);
}

Object* MagazineType::CreateObject(bool fullCreate) const
{ 
  return new Magazine(this, fullCreate);
}


// class Magazine

Magazine::Magazine(const EntityAIType *type, bool fullCreate)
:base(type, fullCreate)
{
  SetAmmo(Type()->_maxAmmo);
  _reload = 0;
  _reloadDuration = 1;
  _reloadMagazine = 0;
  _reloadMagazineTotal = 0;
  _burstLeft = 0;
}

void Magazine::NeedsReload()
{
  _reload = 1;
  _reloadDuration = 0.8 + 0.2 * GRandGen.RandomValue();
}

bool Magazine::UseFromInventory(Person *owner)
{
  const MagazineType *type = Type();
  if (!type) return false;
  const AmmoType *ammo = type->_ammo;
  if (!ammo) return false;

  switch (ammo->_simulation)
  {
  case AmmoShotShell:
  case AmmoShotSmoke:
  case AmmoShotIlluminating:
    // throwable
    if (type->_initSpeed < 30)
    {
      return owner->Throw(this);
    }
    break;
  case AmmoShotTimeBomb:
  case AmmoShotPipeBomb:
  case AmmoShotMine:
    {
      Ref<Action> action = new ActionMagazine(ATPutItem, owner, owner, this);
      owner->StartActionProcessing(action, owner->Brain());
      return true;
    }
    break;
  }

  // by default, try to reload the magazine to the current weapon
  if (GetAmmo() > 0)
  {
    WeaponModePtr ptr = owner->GetCurrentWeapon();
    if (ptr.IsValid())
    {
      const WeaponType *weapon = ptr.GetWeaponType();
      if (weapon) for (int i=0; i<weapon->_muzzles.Size(); i++)
      {
        const MuzzleType *muzzle = weapon->_muzzles[i];
        if (muzzle->_modes.Size() == 0) continue;
        for (int j=0; j<muzzle->_magazines.Size(); j++)
        {
          if (muzzle->_magazines[j] == type)
          {
            // magazine can be used in this weapon
            ptr._muzzle = i;
            ptr._mode = 0;
            owner->ReloadMagazine(*owner->GetWeaponsState(), ptr, this);
            return true;
          }
        }
      }
    }
  }

  return false;
}

bool Magazine::CanAutoShortcut() const
{
  const MagazineType *type = Type();
  if (!type) return false;
  const AmmoType *ammo = type->_ammo;
  if (!ammo) return false;

  switch (ammo->_simulation)
  {
  case AmmoShotShell:
  case AmmoShotSmoke:
  case AmmoShotIlluminating:
    // throwable
    return type->_initSpeed < 30;
  case AmmoShotTimeBomb:
  case AmmoShotPipeBomb:
  case AmmoShotMine:
    // putable
    return true;
  default:
    return false;
  }
}

Magazine *Magazine::CreateObject(ParamArchive &ar)
{
  return dyn_cast<Magazine>(base::CreateObject(ar));
};  

LSError Magazine::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))

  int ammo = GetAmmo();
  CHECK(ar.Serialize("ammo", ammo, 1, 0));
  SetAmmo(ammo);
  CHECK(ar.Serialize("burstLeft", _burstLeft, 1, 0));
  CHECK(ar.Serialize("reload", _reload, 1, 0));
  CHECK(ar.Serialize("reloadDuration", _reloadDuration, 1, 1));
  CHECK(ar.Serialize("reloadMagazine", _reloadMagazine, 1, 0));
  CHECK(ar.Serialize("reloadMagazineTotal", _reloadMagazineTotal, 1, 0));
  return LSOK;
}

#define MAGAZINE_MSG_LIST(XX) \
  XX(UpdatePosition, UpdatePositionMagazine)

DEFINE_NETWORK_OBJECT(Magazine, base, MAGAZINE_MSG_LIST)

#define UPDATE_POSITION_MAGAZINE_MSG(MessageName, XX) \
  XX(MessageName, int, ammo, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("Ammo count"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MINOR) \
  XX(MessageName, int, burstLeft, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("How many shots are there in the burst (auto fired)"), TRANSF, ET_NONE, 0) \
  XX(MessageName, float, reload, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Time rest to reload shot"), TRANSF, ET_NONE, 0) \
  XX(MessageName, float, reloadDuration, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Relative reloading duration (depending on ability)"), TRANSF, ET_NONE, 0) \
  XX(MessageName, float, reloadMagazine, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Time rest to reload magazine"), TRANSF, ET_NONE, 0) \
  XX(MessageName, float, reloadMagazineTotal, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Time rest to reload magazine"), TRANSF, ET_NONE, 0)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionMagazine, UpdatePositionVehicle, UPDATE_POSITION_MAGAZINE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionMagazine, UpdatePositionVehicle, UPDATE_POSITION_MAGAZINE_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Magazine::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateDamage:
    base::CreateFormat(cls, format);
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_MAGAZINE_MSG(UpdatePositionMagazine, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError Magazine::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateDamage:
    TMCHECK(base::TransferMsg(ctx))
    break;
  case NMCUpdatePosition:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdatePositionMagazine)

      int ammo = GetAmmo();
      TRANSF_EX(ammo, ammo)
      SetAmmo(ammo);
      TRANSF(burstLeft)
      TRANSF(reload)
      TRANSF(reloadDuration)
      TRANSF(reloadMagazine)
      TRANSF(reloadMagazineTotal)
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }

  return TMOK;
}

float Magazine::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateDamage:
    {
      error += base::CalculateError(ctx);
    }
    break;
  case NMCUpdatePosition:
    {
      error += base::CalculateError(ctx);
      {
        PREPARE_TRANSFER(UpdatePositionMagazine)
        error += abs(GetAmmo() - message->_ammo) * ERR_COEF_MODE;
      }
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

void Magazine::SetAmmoNetAware(int ammo)
{
  if (IsLocal())
  {
    SetAmmo(ammo);
  }
  else
  {
    GetNetworkManager().AskForSetMagazineAmmo(this,ammo);
  }
}

void Magazine::JSONSave(JSONValue &container, JSONAllocator &allocator) const
{
  base::JSONSave(container, allocator);

  JSONValue newState;
  if (!container.HasMember("state"))
  {
    newState.SetObject();
    container.AddMember("state", newState, allocator);
  }

  JSONValue &state = container["state"];
  if (state.IsObject())
  {
    state.AddMember("ammo", GetAmmo(), allocator);
  }
}

bool Magazine::JSONLoad(JSONValue &container, Person *person)
{
  if (!base::JSONLoad(container, person)) return false;

  JSONValue &state = container["state"];
  if (state.IsObject())
  {
    int ammo = state["ammo"].GetInt();
    SetAmmo(ammo);
  }
  return true;
}

DEFINE_CASTING(Magazine)

