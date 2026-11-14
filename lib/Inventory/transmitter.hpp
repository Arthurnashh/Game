#pragma once
#include "inventory.hpp"
class TransmitterVoice;

class ItemTransmitterType : public InventoryItemType
{
  typedef InventoryItemType base;
  friend class ItemTransmitter;

public:
  float _range;
  ItemTransmitterType(ParamEntryPar param) : base(param) {}
  virtual void Load(ParamEntryPar par, const char *shape) override;

  virtual Object* CreateObject(bool fullCreate) const override;

  float Channel(int chanIx) const;
};

class ItemTransmitter : public InventoryItem
{
  typedef InventoryItem base;

public:
  //! what frequency the radio is tuned on (and can record to)
  float tunedFrequency;
  //! true when player is nearby
  bool isAudible;
  //! something has changed (freqv, isOn, ...), so nearbyTransmitters need to be updated
  bool isDirty;

  //! to Calculate3D for submix inside. Voice can be played through VoNSystemXA2::advanceAll
  Ref<TransmitterVoice> _voice3D;

public:
  ItemTransmitter(const EntityAIType *type, bool fullCreate) : base(type, fullCreate), isAudible(false), _chanIx(0), isDirty(false)
  {
    tunedFrequency = Type()->Channel(_chanIx);
  }

  const ItemTransmitterType *Type() const
  {
    return static_cast<const ItemTransmitterType *>(GetType());
  }

  //! position of radio (even when inside some container)
  Vector3 TransmitionPosition() const;

  //! perform simulation
  virtual void Simulate(float deltaT, SimulationImportance prec);

  bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  void PerformAction(const Action *action, AIBrain *unit) override;
  void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  //! Multiplayer
  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  virtual TMError TransferMsg(NetworkMessageContext &ctx) override;
  virtual float CalculateError(NetworkMessageContextWithError &ctx) override;

  INHERIT_SOFTLINK(ItemTransmitter, base);

private:
  int _chanIx;
};


