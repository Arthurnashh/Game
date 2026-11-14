#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DETECTOR_HPP
#define _DETECTOR_HPP

//#include "house.hpp"
#include "vehicleAI.hpp"

/// flag pole type
class FlagCarrierType: public EntityAIType
{
  typedef EntityAIType base;
  friend class FlagCarrier;
  
  float _animSpeed;
  
  public:
  FlagCarrierType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  void InitShape();
  void DeinitShape(); // before shape is unloaded

  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);

  virtual Object* CreateObject(bool unused) const;

  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
  bool AbstractOnly() const {return false;}
  bool IsSimulationRequired() const;
  bool IsSoundRequired() const;
};

/// flag pole
// TODO: should have a VisualState?
class FlagCarrier: public EntityAI
{
  typedef EntityAI base;

protected:
  Ref<Texture> _flagTexture;

  /// phase 0 = raised, 1 = lowered (taken off)
  float _phase;
  float _phaseWanted;

public:
  FlagCarrier(const EntityAIType *type);
  ~FlagCarrier();

  __forceinline const FlagCarrierType *Type() const
  {
    return static_cast<const FlagCarrierType *>(GetType());
  }

  AnimationStyle IsAnimated(int level) const;

  float GetCtrlFlag(const ObjectVisualState &vs) const {return _phase;}

  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  Texture *GetFlagTexture();
  Texture *GetFlagTextureInternal();
  void SetFlagTexture(RString name);

  void LowerFlag(float lowered) override;
  float GetFlagLowered() const override;
  
  bool QIsManual() const {return false;}

  void Simulate( float deltaT, SimulationImportance prec );

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);

  USE_CASTING(base)
};

#endif
