#pragma once
#include <lib/vehicleAI.hpp>

typedef EntityAI InventoryBase;

class PoweredItemType : public EntityAIType
{
  typedef EntityAIType base;

public:
  struct EnergySupply
  {
    RString m_varName;   // power etc.
    float   m_input;     // energy input in units per second
    EnergySupply() {}
    EnergySupply(RString var, float watt) : m_varName(var), m_input(watt) {}
    ClassIsMovableZeroed(EnergySupply)
  };

  AutoArray<EnergySupply> m_energySupply;
  AutoArray<EnergySupply> m_internalSupply;

  PoweredItemType(ParamEntryPar param) : base(param) {}
  virtual void Load(ParamEntryPar par, const char *shape) override;
};

class PoweredItem : public InventoryBase
{
  typedef InventoryBase base;
  // when no energy remains, device is switched OFF
  bool _isOn;
  float _internalEnergy;
public:

  PoweredItem(const EntityAIType *type, bool fullCreate);

  const PoweredItemType *Type() const
  {
    return static_cast<const PoweredItemType *>(GetType());
  }
  //--- added to experiment with occlusion culling optimizations for inventory objects
  virtual float DrawingAreaRatio() const override;

  virtual int NAttachments() const = 0;
  virtual const InventoryItem *GetAttachment(int i) const = 0;
  virtual InventoryItem *GetAttachment(int i) = 0;
  void SwitchOn(bool onOff);

  bool ForEachItem(const std::function<bool(InventoryItem *)> &func);
  bool ForEachItem(const std::function<bool(const InventoryItem *)> &func) const;

  bool ForEachAttachment(const std::function<bool(const InventoryItem *item)> &func) const;
  bool ForEachAttachment(const std::function<bool(InventoryItem *item)> &func);

  // return true when some energy remains in its suppliers
  virtual bool IsPowered() const;
  //! some inventoryItem devices can be switched on/off (radios, transmitters)
  virtual bool IsOn() const { return _isOn; }

  // consume some energy from its supplies
  virtual void Simulate(float deltaT, SimulationImportance prec) override;
  virtual LSError Serialize(ParamArchive &ar) override;

  INHERIT_SOFTLINK(PoweredItem, base);
};

