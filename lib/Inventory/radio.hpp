#pragma once
#include "inventory.hpp"
class RadioStream2;

// Streaming Radio classes (OggStream)
class ItemRadioType : public InventoryItemType
{
  typedef InventoryItemType base;

  class RadioStreamChannel
  {
  public:
    RString name;
    RString url;
    RadioStreamChannel() {}
    RadioStreamChannel(RString chan, RString site) : name(chan), url(site) {}

    ClassIsMovableZeroed(RadioStreamChannel)
  };
  AutoArray<RadioStreamChannel> _channels;
  friend class ItemRadio;

public:
  ItemRadioType(ParamEntryPar param) : base(param) {}

  const AutoArray<RadioStreamChannel> &Channels() const { return _channels; }

  virtual void Load(ParamEntryPar par, const char *shape) override;
  virtual Object* CreateObject(bool fullCreate) const override;
};


class ItemRadio : public InventoryItem
{
  typedef InventoryItem base;
  static const float HearingStartDistance;  // radio within this range from Camera should start playing
  static const float HearingStopDistance;   // radio outside this distance from Camera should stop playing
  static const float HearingMaxDistance;    // max hearing distance (for Calculate3D)

  Ref<RadioStream2> _radioStream;
  Ref<AbstractSubmix> _submix;
  bool _playing;     // radio is playing, its related source wave sent to the submix
  RString _url;      // needed for reconnecting
  bool _play;        // whether this ItemRadio should be played or not based on Player position
  int _chanIx;       // what channel url am I tuned to (config clas Channels)

public:
  //! volume can be changed by ATRadioVolumeUp/Down
  float _volume; //1.0f default

  //! true when player is nearby
  bool isAudible;

public:
  ItemRadio(const EntityAIType *type, bool fullCreate);
  ~ItemRadio();

  const ItemRadioType *Type() const
  {
    return static_cast<const ItemRadioType *>(GetType());
  }

  //! position of radio (even when inside some container)
  Vector3 TransmitionPosition() const;

  //! radio is outside the hearing range => stop its submix
  void Stop();

  //! perform simulation
  virtual void Simulate(float deltaT, SimulationImportance prec) override;
  //! tune to different url
  void TuneChannel(RString url);

  bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  void PerformAction(const Action *action, AIBrain *unit) override;
  void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  //! Multiplayer
  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  virtual TMError TransferMsg(NetworkMessageContext &ctx) override;
  virtual float CalculateError(NetworkMessageContextWithError &ctx) override;

  INHERIT_SOFTLINK(ItemRadio, base);
private:
  enum {DirNext, DirPrev} _dir;
  void TuneNext();
  void TunePrev();
};


