#include "../wpch.hpp"
#include "radio.hpp"
#include <lib/soundXA2.hpp>
#include <lib/World.hpp>
#include <lib/UI/uiActions.hpp>
#include <lib/drawText.hpp>
#include <lib/Network/network.hpp>
#include <lib/person.hpp>
#include <lib/AI/ai.hpp>


void ItemRadioType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par, shape);

  LogF("RADIO CHANNEL LIST:");
  ParamEntryPtr entry = par.FindEntry("Channels");
  if (entry && entry->IsClass())
  {
    int size = entry->GetEntryCount();
    _channels.Realloc(size);
    for (int i=0; i<size; i++)
    {
      ParamEntryVal chan = entry->GetEntry(i);
      if (chan.IsClass())
      {
        RString name = chan >> "name";
        RString url = chan >> "url";
        LogF("    %s : %s",cc_cast(name), cc_cast(url));
        _channels.AddFast(RadioStreamChannel(name,url));
      }
    }
  }
  if (!_channels.Size())
  { // we need to fill in something as default
    LogF("    no radio channels found in config, adding Cro1 and Absolute 60s");
    _channels.Add(RadioStreamChannel("Cro1","http://amp.cesnet.cz:8000/cro1.ogg"));
    _channels.Add(RadioStreamChannel("Absolute 60s","http://icecast.as34763.net:80/a632.ogg"));
  }
  LogF("^^^^ END OF LIST");
}

Object* ItemRadioType::CreateObject(bool fullCreate) const
{
  return new ItemRadio(this, fullCreate);
}

const float ItemRadio::HearingStartDistance = 120.0f;
const float ItemRadio::HearingStopDistance =  130.0f;
const float ItemRadio::HearingMaxDistance =   120.0f;
ItemRadio::ItemRadio(const EntityAIType *type, bool fullCreate) : base(type, fullCreate), isAudible(false), _playing(false), _play(false), 
   _volume(1.0f), _chanIx(0), _dir(DirNext)
{
}

ItemRadio::~ItemRadio()
{
#ifndef _CONSOLE
  if (_radioStream)
  {
    XA2Submix *submix = static_cast<XA2Submix *>(_submix.GetRef());
    if (submix)
    {
      submix->Exclude(_radioStream->_wave, true);
      _radioStream->Exclude(submix);
    }
    _radioStream->_wave = NULL;
    _radioStream = NULL;
    _submix = NULL;
  }
#endif
}

void ItemRadio::Stop()
{ // radio is outside the hearing range => stop its submix
#ifndef _CONSOLE
  if (_radioStream)
  { 
    XA2Submix *submix = static_cast<XA2Submix *>(_submix.GetRef());
    if (submix)
    {
      submix->Exclude(_radioStream->_wave, true); // removed even when _wave is already NULL
      _radioStream->Exclude(submix);
    }
  }
  _submix = NULL;
  _playing = false;
  // Note: _radioStream can be invalidated when no submix is played for given time (checked in InternetRadioSystem2::Simulate)
#endif
}

void ItemRadio::Simulate(float deltaT, SimulationImportance prec)
{
  base::Simulate(deltaT, prec);
#ifndef _CONSOLE
  if (IsOn())
  {
    // Decide whether this ItemRadio should be played or not based on Camera? position
    float distance = HearingStopDistance;
    if (GWorld->CameraOn() && !GWorld->GetCameraEffect())
    { 
      Camera *cam = GScene->GetCamera();
      if (cam) distance = cam->Position().Distance(TransmitionPosition());
    }
    if (_play && distance>=HearingStopDistance) _play = false;       //stop
    else if (!_play && distance<HearingStartDistance) _play = true; //start

    // When _radioStrem was invalidated, remove submix and related Refs
    if (_radioStream && !_radioStream->valid)
    { // invalid stream => reset it
      _playing = false;
      XA2Submix *submix = static_cast<XA2Submix *>(_submix.GetRef());
      if (submix)
      {
        submix->Exclude(_radioStream->_wave, true); // removed even when _wave is already NULL
        _radioStream->Exclude(submix);
      }
      _radioStream->_wave = NULL; // RefAbstractWave, so it is UnLoaded and freed, link inside GSoundScene->_all2DSounds set to NULL
      _radioStream = NULL;
      _submix = NULL;
    }
    bool IsAppPaused();
    if (_play && !IsAppPaused())
    {
      if (!_radioStream)
      {
        _radioStream = AddInternetRadio(_url);
      }
      if (_radioStream)
      {
        if (!_radioStream->_wave || _radioStream->_wave->IsTerminated()) 
        {
          _radioStream->_wave = GSoundScene->OpenAndPlayOnce2D(_radioStream->Name());
          if (_radioStream->_wave) 
          {
            if (_radioStream->_wave->LoadError())
            { // skip this radio, tune next radio channel
              _radioStream->_wave = NULL;
              if (_dir==DirNext) TuneNext();
              else TunePrev();
              return;
            }
            else _radioStream->_wave->debugMe = true;
          }
          //GSoundScene->AddSound(_radioStream->_wave); //it is played from 
        }
        if (_radioStream->_wave)
        {
          if (!_playing)
          {
            _playing = true;
            _submix = GSoundsys->CreateSubmix();
            XA2Submix *submix = static_cast<XA2Submix *>(_submix.GetRef());
            if (submix) 
            {
              //Include, but do not set sendlist (set in InternetRadioSystem2::Simulate(), as one wave can be sent to multiple submixes)
              submix->Include(_radioStream->_wave, true); 
              _radioStream->Include(submix);
            }
          }
          if (_submix)
          {
            _submix->Calculate3D(true, TransmitionPosition(), VZero, HearingMaxDistance, _volume);
            #if DIAG_RADIO_STREAM
            Vector3 pos = TransmitionPosition();
            LogF("submix: %x, wave: %x pos [%.2f,%.2f]", _submix.GetRef(), _radioStream->_wave.GetRef(), pos.X(), pos.Z());
            XA2Submix *submix = static_cast<XA2Submix *>(_submix.GetRef());
            if (submix) submix->ReportVolume();
            #endif
          }
        }
      }
    }
    else
    {
      Stop();
    }
  }
  else
  {
    Stop();
  }
#endif
}

void ItemRadio::TuneChannel(RString url)
{
  Stop();
  _radioStream = NULL; //new one will be created in simulation
  _url = url;
  LogF("Tuning radio to: %s", cc_cast(url));
}

bool ItemRadio::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
  case ATRadioNextChannel: 
    {
      const AutoArray<ItemRadioType::RadioStreamChannel> &channels = Type()->Channels();
      int chIx = (_chanIx+1)%channels.Size();
      params.Add(CreateTextASCII(NULL, channels[chIx].name));
    }
    return true;
  case ATRadioPrevChannel:
    {
      const AutoArray<ItemRadioType::RadioStreamChannel> &channels = Type()->Channels();
      int chIx = _chanIx ? _chanIx : channels.Size();
      chIx--;
      params.Add(CreateTextASCII(NULL, channels[chIx].name));
    }
    return true;  
  case ATRadioOn: 
  case ATRadioOff:
    {
      const AutoArray<ItemRadioType::RadioStreamChannel> &channels = Type()->Channels();
      int chIx = _chanIx%channels.Size();
      params.Add(CreateTextASCII(NULL, channels[chIx].name));
    }
    return true;
  }
  return base::GetActionParams(params, action, unit);
}

void ItemRadio::TuneNext()
{
  const AutoArray<ItemRadioType::RadioStreamChannel> &channels = Type()->Channels();
  _chanIx = (_chanIx+1)%channels.Size();
  TuneChannel(channels[_chanIx].url);
  _dir = DirNext;
}

void ItemRadio::TunePrev()
{
  const AutoArray<ItemRadioType::RadioStreamChannel> &channels = Type()->Channels();
  if (!_chanIx) _chanIx = channels.Size();
  _chanIx--;
  TuneChannel(channels[_chanIx].url);
  _dir = DirPrev;
}

void ItemRadio::PerformAction(const Action *action, AIBrain *unit)
{
  switch (action->GetType())
  {
  case ATRadioOn:  SwitchOn(true); return;
  case ATRadioOff: SwitchOn(false); return;
  case ATRadioVolumeUp: _volume *= 2.3; return;  //2 is few, 3 is much
  case ATRadioVolumeDown: _volume /= 2.3; return;
  case ATRadioNextChannel: 
    TuneNext();
    return;
  case ATRadioPrevChannel:
    TunePrev();
    return;
  }

  base::PerformAction(action, unit);
}

void ItemRadio::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  if (IsPowered())
  { 
    UIAction action(new ActionBasic(IsOn() ? ATRadioOff : ATRadioOn, this)); add(action); 
    if (_volume<81.0f) //3*3*3*3
    { 
      UIAction action(new ActionBasic(ATRadioVolumeUp, this)); add(action); 
    }
    if (_volume>0.1f)
    { 
      UIAction action(new ActionBasic(ATRadioVolumeDown, this)); add(action); 
    }
    { UIAction action(new ActionBasic(ATRadioNextChannel, this)); add(action); }
    { UIAction action(new ActionBasic(ATRadioPrevChannel, this)); add(action); }
  }

  base::GetActions(add, unit);
}

Vector3 ItemRadio::TransmitionPosition() const
{
  // find the root of the hierarchy
  const Entity *root = this;
  while (root->GetHierarchyParent()) root = root->GetHierarchyParent();

  return root->FutureVisualState().Position();
}


#define ITEM_RADIO_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateItemRadio)

DEFINE_NETWORK_OBJECT(ItemRadio, base, ITEM_RADIO_MSG_LIST)

#define UPDATE_ITEM_RADIO_MSG(MessageName, XX) \
  XX(MessageName, float, volume, NDTFloat, float, NCTNone, DEFVALUE(float, 1.0f), DOC_MSG("Radio volume"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, RString, url, NDTString, RString, NCTNone, DEFVALUE(RString, ""), DOC_MSG("Streaming url"), TRANSF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE)

DECLARE_NET_INDICES_EX_ERR(UpdateItemRadio, UpdateInventoryItem, UPDATE_ITEM_RADIO_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateItemRadio, UpdateInventoryItem, UPDATE_ITEM_RADIO_MSG, NoErrorInitialFunc)

NetworkMessageFormat &ItemRadio::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateDamage:
    base::CreateFormat(cls, format);
    break;
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_ITEM_RADIO_MSG(UpdateItemRadio, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError ItemRadio::TransferMsg(NetworkMessageContext &ctx)
{
  TMCHECK(base::TransferMsg(ctx))
  
  if (ctx.GetClass() == NMCUpdateGeneric)
  {
    PREPARE_TRANSFER(UpdateItemRadio)

    // volume and isOn can be changed with no care
    TRANSF(volume);
    // change url carefully
    if (ctx.IsSending())
    {
      TRANSF(url);
    }
    else
    {
      if (stricmp(message->_url, _url) != 0)
      { // url has changed
        // try to fix _chanIx, but when not found, never mind
        const AutoArray<ItemRadioType::RadioStreamChannel> &channels = Type()->Channels();
        for (int i=0; i<channels.Size(); i++)
        {
          if (channels[i].url==message->_url)
          {
            _chanIx = i; break;
          }
        }
        TuneChannel(message->_url);
      }
    }
  }

  return TMOK;
}

float ItemRadio::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = base::CalculateError(ctx);
  if (ctx.GetClass() == NMCUpdateGeneric)
  {
    PREPARE_TRANSFER(UpdateItemRadio)

    // volume, on/off and url
    float vol = message->_volume / _volume;
    if (vol < 1) vol = 1/vol;
    error += ERR_COEF_MODE * (vol - 1);
    if (stricmp(message->_url, _url)!=0) error += ERR_COEF_STRUCTURE;
  }
  return error;
}

