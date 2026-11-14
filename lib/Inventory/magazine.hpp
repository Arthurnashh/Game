#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MAGAZINE_HPP
#define _MAGAZINE_HPP

#include "inventory.hpp"
// #include "../weapons.hpp"

class MagazineType : public InventoryItemType
{
private:
  typedef InventoryItemType base;

public:
  int _scope;                 // public / protected for briefing

  RStringB _picName;
  RStringB _description;      // description for overview / library
  int _magazineType;          // slots occupied
  int _maxAmmo;               // how much ammo is available
  float _maxLeadSpeed;        // max lead
  float _initSpeed;           // initial ammo speed
  float _invInitSpeed;        // initial ammo speed
  bool _quickReload;          //when magazine is changed, weapon is ready to fire
  //! used in MP game for decision which body to hide
  float _value;
  //! mass of magazine
  float _mass;

  float TravelSpeed() const
  {
    float travelSpeed = floatMax(_initSpeed,0.1f);
    if (_ammo) saturateMax(travelSpeed,_ammo->maxSpeed);
    return travelSpeed;
  }

  RString _reloadAction;
  RStringB _modelName;
  Ref<LODShape> _modelSpecial;
  mutable int _modelRefCount;
  
  mutable AnimationAnimatedTexture _animFire;

  bool _useAction;
  RString _useActionTitle;

  Ref<const AmmoType> _ammo;  // ammo

  //@{ tracers
  int _tracersEvery;
  //specifies if the last rounds are tracers
  int _lastRoundsTracer; 
  //@}

public:
  MagazineType(ParamEntryPar param);
  virtual void Load(ParamEntryPar par, const char *shape) override;

  RStringB GetPictureName() const;
  RStringB GetDescription() const {return _description;}

  virtual void InitShape() override; // force all used ammo types to load shape
  virtual void DeinitShape() override; // allow all used ammo types to release shape

  //@{ LODShapeLoadHandler
  virtual void LODShapeLoaded(LODShape *shape) override;
  virtual void LODShapeUnloaded(LODShape *shape) override;
  virtual void ShapeLoaded(LODShape *shape, int level) override;
  virtual void ShapeUnloaded(LODShape *shape, int level) override;
  //@} LODShapeLoadHandler
  
  void AmmoAddRef() const; // force all used ammo types to load shape
  void AmmoRelease() const; // allow all used ammo types to release shape

  RStringB GetDisplayName() const {return _displayName;}
  RStringB GetNameSound() const {return _nameSound;}

  virtual Object* CreateObject(bool fullCreate) const override;
};
TypeIsMovable(MagazineType)

/// magazine inserted into weapons, containing ammo
class Magazine : public InventoryItem
{
private:
  typedef InventoryItem base;

public:
  EncryptedInt _ammo;
  int _burstLeft; // how many shots are there in the burst (auto fired)
  /// relative reload state (0 .. 1)
  float _reload;
  /// relative reload duration (multiplier to WeaponModeType::_reloadTime)
  float _reloadDuration;
  /// time left during magazine reloading
  float _reloadMagazine;
  /// total time of current magazine reload
  float _reloadMagazineTotal;

  EncryptedIntSupport _ammoSupport;

  int GetAmmo() const {return GET_ENCRYPTED(_ammo,_ammoSupport);}
  void SetAmmo(int ammo) {SET_ENCRYPTED(_ammo,_ammoSupport,ammo);}
  void SetAmmoNetAware(int ammo);

public:
  Magazine(const EntityAIType *type, bool fullCreate);

  const MagazineType *Type() const
  {
    return static_cast<const MagazineType *>(GetType());
  }

  // cast optimization
  virtual Magazine *GetMagazine() override {return this;}
  virtual const Magazine *GetMagazine() const override {return this;}

  // switch state to shot reloading
  void NeedsReload();

  virtual bool CanAutoShortcut() const override;
  virtual bool UseFromInventory(Person *owner) override;

  LSError Serialize(ParamArchive &ar);
  static Magazine *CreateObject(ParamArchive &ar);

  // network synchronization
  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);

  // JSON serialization
  virtual void JSONSave(JSONValue &container, JSONAllocator &allocator) const override;
  virtual bool JSONLoad(JSONValue &container, Person *person) override;

  INHERIT_SOFTLINK(Magazine, base);
  USE_CASTING(base)
};

#endif // _MAGAZINE_HPP
