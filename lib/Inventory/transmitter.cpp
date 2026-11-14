#include "../wpch.hpp"
#include "transmitter.hpp"
#include <lib/World.hpp>
#include <lib/scene.hpp>
#include <El/SoundFile/soundFile.hpp>
#include <lib/Network/voiceXA2.hpp>
#include <lib/Network/network.hpp>
#include <lib/drawText.hpp>
#include <lib/UI/uiActions.hpp>
#include <lib/person.hpp>
#include <lib/AI/ai.hpp>

void ItemTransmitterType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par, shape);

  if (par.FindEntry("range")) _range = par >> "range";
  else _range = 1000.f;
  GetNetworkManager().SetVoNTransmitterRange(_range);
}

Object* ItemTransmitterType::CreateObject(bool fullCreate) const
{
  return new ItemTransmitter(this, fullCreate);
}

float ItemTransmitterType::Channel(int chanIx) const
{
  static float freqvList[] = {87.8f, 89.5f, 91.3f, 91.9f, 94.6f, 96.6f, 99.7f, 102.5f};
  chanIx = chanIx % lenof(freqvList);
  if ( chanIx<0 ) chanIx += lenof(freqvList);
  return freqvList[chanIx];
}

void ItemTransmitter::Simulate(float deltaT, SimulationImportance prec)
{
  base::Simulate(deltaT, prec);
  if (IsDedicatedServer()) return; //no simulation when no player there
  if (IsOn())
  {
    const float maxRadius2in = Square(50.0f);
    const float maxRadius2out = Square(60.0f); //adding some hysteresis
    bool audible = false;
    if (GScene->GetCamera())
    {
      Vector3Val pos = GScene->GetCamera()->Position();
      audible = pos.Distance2(TransmitionPosition()) <= (isAudible ? maxRadius2out : maxRadius2in);
    }
    if (audible)
    { // update position
      // TODO: obstruction, occlusion
#ifndef _CONSOLE
      Vector3 pos = TransmitionPosition();
      if (!_voice3D) _voice3D = new TransmitterVoice(pos.X(), pos.Y(), pos.Z(), Type()->_range, static_cast<SoundSystemXA2 *>(GSoundsys)->XA2Device());
      else
      {
        _voice3D->Set3DPosition(pos, VZero);
        _voice3D->UpdateSoundPars(TTChan3D);
      }
#endif
    }
    if (audible!=isAudible || isDirty)
    {
      isAudible = audible;
      GWorld->UpdateTransmitterItem(this); //send subscribeRadio message
    }
  }
  else 
  {
    isAudible = IsOn();
    if (isDirty) GWorld->UpdateTransmitterItem(this);
  }
}

Vector3 ItemTransmitter::TransmitionPosition() const
{
  // find the root of the hierarchy
  const Entity *root = this;
  while (root->GetHierarchyParent()) root = root->GetHierarchyParent();

  return root->FutureVisualState().Position();
}

bool ItemTransmitter::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  float freqv = 0;
  switch (action->GetType())
  {
  case ATTransmitterNextChannel: 
    freqv = Type()->Channel(_chanIx+1);
    break;
  case ATTransmitterPrevChannel:
    freqv = Type()->Channel(_chanIx-1);
    break;
  case ATTransmitterOn: 
  case ATTransmitterOff:
    freqv = Type()->Channel(_chanIx);
    Assert(freqv==tunedFrequency);
    break;
  }
  if (freqv>0)
  {
    char strFreq[16];
    sprintf(strFreq, "%.1f", freqv);
    params.Add(CreateTextASCII(NULL, strFreq));
    return true;
  }
  return base::GetActionParams(params, action, unit);
}

void ItemTransmitter::PerformAction(const Action *action, AIBrain *unit)
{
  switch (action->GetType())
  {
  case ATTransmitterOn:  SwitchOn(true); break;
  case ATTransmitterOff: SwitchOn(false); break;
  case ATTransmitterNextChannel: tunedFrequency=Type()->Channel(++_chanIx); break;
  case ATTransmitterPrevChannel: tunedFrequency=Type()->Channel(--_chanIx); break;
  case ATDropItem: 
    {
    ChatChannel ActualChatChannel();
    void SetChatChannel(ChatChannel channel);
    SetChatChannel(ActualChatChannel());
    }
    //continue to default action
  default:
    base::PerformAction(action, unit);
    return;
  }
  // when on/off or frequency changes, the NMTSubscribeRadios message need to be sent
  isDirty = true; 
}

void ItemTransmitter::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  if (IsPowered())
  { //on/off possible only when batteries are inserted and some energy inside is still available
    UIAction action(new ActionBasic(IsOn() ? ATTransmitterOff : ATTransmitterOn, this)); add(action); 
    { UIAction action(new ActionBasic(ATTransmitterNextChannel, this)); add(action); }
    { UIAction action(new ActionBasic(ATTransmitterPrevChannel, this)); add(action); }
  }

  base::GetActions(add, unit);
}

#define ITEM_TRANSMITTER_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateItemTransmitter)

DEFINE_NETWORK_OBJECT(ItemTransmitter, base, ITEM_TRANSMITTER_MSG_LIST)

#define UPDATE_ITEM_TRANSMITTER_MSG(MessageName, XX) \
  XX(MessageName, int, chanIx, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 0), DOC_MSG("Transmitting channel number"), TRANSF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE)

DECLARE_NET_INDICES_EX_ERR(UpdateItemTransmitter, UpdateInventoryItem, UPDATE_ITEM_TRANSMITTER_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateItemTransmitter, UpdateInventoryItem, UPDATE_ITEM_TRANSMITTER_MSG, NoErrorInitialFunc)

NetworkMessageFormat &ItemTransmitter::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateDamage:
    base::CreateFormat(cls, format);
    break;
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_ITEM_TRANSMITTER_MSG(UpdateItemTransmitter, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError ItemTransmitter::TransferMsg(NetworkMessageContext &ctx)
{
  TMCHECK(base::TransferMsg(ctx))
  
  if (ctx.GetClass() == NMCUpdateGeneric)
  {
    PREPARE_TRANSFER(UpdateItemTransmitter)

    // volume and isOn can be changed with no care
    TRANSF(chanIx);
    if (!ctx.IsSending())
    {
      tunedFrequency=Type()->Channel(_chanIx);
      isDirty = true;
    }
  }

  return TMOK;
}

float ItemTransmitter::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = base::CalculateError(ctx);
  if (ctx.GetClass() == NMCUpdateGeneric)
  {
    PREPARE_TRANSFER(UpdateItemTransmitter)

    // on/off and chanIx
    if ( message->_chanIx!=_chanIx ) error += ERR_COEF_STRUCTURE;
  }
  return error;
}

