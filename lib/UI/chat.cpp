#include "../wpch.hpp"
#include "chat.hpp"
#include "uiControls.hpp"
#include "../world.hpp"
#include "resincl.hpp"
#include "../AI/ai.hpp"
#include "../transport.hpp"
#include "../person.hpp"
#include "../unitsOnLand.hpp"
#include "../landscape.hpp"
#include "../vehicleAI.hpp"
#include "../keyInput.hpp"
#include "../dikCodes.h"
#include "../stringtableExt.hpp"
#include "../mbcs.hpp"
#include <El/Common/perfProf.hpp>
#include <Es/Common/win.h>
#include <lib/Inventory/transmitter.hpp>
#include <lib/Network/voiceXA2.hpp>
#include "../AI/aiRadio.hpp"
#include "../Network/network.hpp"



/*!
\file
Implementation file for chat processing - chat edit, chat (radio message) list
*/

//! maximal total number of chat messages in history
#define CHAT_ITEMS					100

RString GetFullPlayerName()
{
	const PlayerIdentity *identity = GetNetworkManager().FindIdentity(GetNetworkManager().GetPlayer());
	if (identity)
		return identity->GetName();
	else
#if _VBS2 // VBS2 nickname
    return Glob.header.playerHandle;
#else
		return Glob.header.GetPlayerName();
#endif
}

RString GetFullPlayerName(Person *person)
{
	int player = person->GetRemotePlayer();
	if (player == 1) return person->GetInfo()._fullName;
	
	const PlayerIdentity *identity = GetNetworkManager().FindIdentity(player);
	if (identity)
		return identity->GetName();
	else
		return person->GetInfo()._fullName;
}

ChatList::ChatList()
{
	_x = 0;
	_y = 0;
	_w = 0;
  _h = 0;
	_rows = 0;
	_lastSpeaking = UITIME_MIN;
	
	_font = NULL;
	_size = 0;

	_bgColor = PackedColor(Color(0, 0, 0, 0.8));
	_colors[CCGlobal] = PackedColor(Color(0.8, 0.8, 0.8, 1));
	_colors[CCVehicle] = PackedColor(Color(0.9, 0.8, 0, 1));
	_colors[CCDirect] = PackedColor(Color(0.9, 0, 0.8, 1));
  _colors[CCItemTransmitter] = _colors[CCGlobal];
  for (int i=CCCustom1; i<=CCCustomLast; i++)
  {
    _colors[i] = _colors[CCGlobal];
  }
#if _ENABLE_DIRECT_MESSAGES
  _colors[CCDirectSpeaking] = PackedColor(Color(1, 0, 0, 1));
#endif
	_colors[CCSystem] = PackedColor(Color(1.0, 0.1, 0.1, 1));
	_colors[CCStatus] = _colors[CCGlobal];
	_enable = true;
  _scriptEnable = true;
  _shadow = false;

	_offset = -1;
	_lastSpeaking = UITIME_MIN;
}

void ChatList::Free()
{
  _font = NULL;
 // _iconNormalMessage = NULL;
 // _iconPlayerMessage = NULL;
}

void ChatList::BrowseUp()
{
	if (_offset < Size() - 1) _offset++;
}

void ChatList::BrowseDown()
{
	if (_offset > 0) _offset--;
}

void ChatList::BrowseReset()
{
	_offset = -1;
}

void ChatList::Load(ParamEntryPar cls)
{
	_x = cls >> "x";
	_y = cls >> "y";
	_w = cls >> "w";
  _h = cls >> "h";
	_rows = cls >> "rows";

	_font = GEngine->LoadFont(GetFontID(cls >> "font"));
	_size = cls >> "size";

  _bgColor = GetPackedColor(cls >> "colorBackground");
  _bgPlayerColor = GetPackedColor(cls >> "colorPlayerBackground");
  _colors[CCGlobal] = GetPackedColor(cls >> "colorGlobalChannel");
  _colors[CCVehicle] = GetPackedColor(cls >> "colorVehicleChannel");
  _colors[CCDirect] = GetPackedColor(cls >> "colorDirectChannel");
  if (cls.FindEntry("colorSystemChannel")) //else engine default is taken
    _colors[CCSystem] = GetPackedColor(cls >> "colorSystemChannel");
  if (cls.FindEntry("colorStatusChannel")) //else engine default is taken
    _colors[CCStatus] = GetPackedColor(cls >> "colorStatusChannel");
  for (int i=CCCustom1; i<=CCCustomLast; i++)
  {
    CustomChannelInfo &info = GCustomChannels.GetCustomChannelInfo(i);
    if (info.active)
      _colors[i] = info.color;
    else
      _colors[i] = _colors[CCGlobal];
  }
#if _ENABLE_DIRECT_MESSAGES
  // TODO:
#endif
  _shadow = cls >> "shadow";
  _shadowColor = GetPackedColor(cls >> "shadowColor");

//  _iconNormalMessage = GlobLoadTexture(cls >> "iconNormalMessage");
//  _iconPlayerMessage = GlobLoadTexture(cls >> "iconPlayerMessage");
}

void ChatList::Add(RString text, RString colorClass)
{
	while (Size() >= CHAT_ITEMS)
	{
		Delete(CHAT_ITEMS - 1);
	}
	Insert(0);
	Set(0).channel = CCStatus;
	Set(0).from = RString();
	Set(0).time = Glob.clock;
	Set(0).text = text;
	Set(0).uiTime = Glob.uiTime;
  Set(0).gameTime = Glob.time;
	Set(0).playerMsg = false;
	Set(0).forceDisplay = true;
  Set(0).colorClass = colorClass;

	if (_offset > 0) _offset++;

  //@{ _DEDICATED_CLIENT
  if ( IsDedicatedClient() )
  {
    if (!text.IsEmpty())
    {
      RString format = Format(" > %s", cc_cast(text));
      int ConsoleF(const char *format, ...);
      ConsoleF(format);
    }
  }
}

void ChatList::Add(ChatChannel channel, RString from, RString text, bool playerMsg, bool forceDisplay)
{
  //original subtitles effect for direct speech
  /*if (channel == CCDirect)
	{
		GWorld->SetTitleEffect(CreateTitleEffect(TitPlainDown ,text));
		return;
	}*/
  //now is used chat 

	while (Size() >= CHAT_ITEMS)
	{
		Delete(CHAT_ITEMS - 1);
	}
	Insert(0);
	Set(0).channel = channel;
	Set(0).from = from;
	Set(0).time = Glob.clock;
	Set(0).text = text;
	Set(0).uiTime = Glob.uiTime;
  Set(0).gameTime = Glob.time;
	Set(0).playerMsg = playerMsg;
	Set(0).forceDisplay = forceDisplay;

	if (_offset > 0) _offset++;

  //@{ _DEDICATED_CLIENT
  if ( IsDedicatedClient() )
  {
  if (!text.IsEmpty())
  {
    RString format = Format(" > %s", cc_cast(text));
    int ConsoleF(const char *format, ...);
    ConsoleF(format);
  }
}
  //@}
}

void ChatList::Add(ChatChannel channel, AIBrain *sender, RString text, bool playerMsg, bool forceDisplay)
{
	char from[256];
	from[0] = 0;
	bool fromFullyServed = false;
  if (sender)
	{
		Person *person = sender->GetPerson();
		Assert(person);

		switch (channel)
		{
    case CCGlobal:
			break;
		case CCVehicle:
			{
				Transport *veh = sender->GetVehicleIn();
				if (!veh) break;
        if (person == veh->Driver())
        {
          if (veh->GetType()->IsKindOf(GWorld->Preloaded(VTypeAir)))
            strcpy(from, LocalizeString(IDS_PILOT));
          else
            strcpy(from, LocalizeString(IDS_DRIVER));
        }
        else
        {
          TurretContext context;
          if (veh->FindTurret(person, context) && context._turretType)
          {
            strcpy(from, context._turretType->_gunnerName);
          }
          else if(person && !person->IsNetworkPlayer())
          {
            strcpy(from, GetFullPlayerName(person));
          }
        }
/*
				if (person == veh->Commander())
					strcpy(from, LocalizeString(IDS_COMMANDER));
				else if (person == veh->Driver())
				{
					if (veh->GetType()->IsKindOf(GWorld->Preloaded(VTypeAir)))
						strcpy(from, LocalizeString(IDS_PILOT));
					else
						strcpy(from, LocalizeString(IDS_DRIVER));
				}
				else if (person == veh->Gunner())
					strcpy(from, LocalizeString(IDS_GUNNER));
				else if (sender == veh->CommanderUnit())
					strcpy(from, LocalizeString(IDS_COMMANDER));
				else
					LogF("Warning: unknown position");
*/
			}
			break;
		case CCDirect:
#if _ENABLE_DIRECT_MESSAGES
    case CCDirectSpeaking:
#endif
      //original subtitles effect for direct speech
			/*
      GWorld->SetTitleEffect(CreateTitleEffect(TitPlainDown ,text));
			return;
      */
      //now is used chat
      if (person && person->IsNetworkPlayer())
      {
        strcat(from, "(");
        strcat(from, GetFullPlayerName(person));
        strcat(from, ")");
      }
      else
      {
        strcpy(from, GetFullPlayerName(person));
  		}
      Add(channel, from, text, playerMsg, forceDisplay);
      return;
    case CCItemTransmitter:
      { // degrade text when transmitting on long distance
        if (sender)
        {
          Vector3Val senderPos = sender->Position(sender->GetFutureVisualState());
          Vector3Val camPos = GScene->GetCamera()->Position();
          float quality = senderPos.Distance2(camPos) / GetNetworkManager().GetVoNTransmitterRange2();
          float errProbability = 0.54f*quality*quality - 0.04f; //no degradation at the close beggining and max degradation set to 50%
          saturate(errProbability, 0.f, 0.5f); //0 super, 0.5 very bad
          char *errText = text.MutableData();
          for (int i=0; i<text.GetLength(); i++)
          {
            if ( GRandGen.RandomValue() < errProbability ) errText[i] = '.'; //degrade
          }
        }
      }
      break;
		} //end of Switch
    
    if (channel>=CCCustom1 && channel<=CCCustomLast)
    {
      CustomChannelInfo &info = GCustomChannels.GetCustomChannelInfo(channel);
      info.GetCallSign(person, /*OUT*/from);
      fromFullyServed = true;
    }
		
		if (person && person->IsNetworkPlayer() && !fromFullyServed)
		{
			if (from[0])
			{
/*
				strcat(from, " (");
				strcat(from, person->GetInfo()._name);
				strcat(from, ")");
*/
				strcat(from, " (");
				strcat(from, GetFullPlayerName(person));
				strcat(from, ")");
			}
			else
			{
/*
				strcpy(from, person->GetInfo()._name);
*/
				strcpy(from, GetFullPlayerName(person));
			}
		}

	}
	Add(channel, from, text, playerMsg, forceDisplay);
}

RString StrUprUniversal(const char *text);

void ChatList::AddUC(ChatChannel channel, RString from, RString text, bool playerMsg, bool forceDisplay)
{
  // Make the text uppercase
  RString textUTF8 = StrUprUniversal(text);

  // Add text to the list
  Add(channel, from, textUTF8, playerMsg, forceDisplay);
}

void ChatList::AddUC(ChatChannel channel, AIBrain *sender, RString text, bool playerMsg, bool forceDisplay)
{
  // Make the text uppercase
  RString textUTF8 = StrUprUniversal(text);

  // Add text to the list
  Add(channel, sender, textUTF8, playerMsg, forceDisplay);
}

/*!
\patch 5119 Date 1/18/2007 by Jirka
- Fixed: Some international characters was clipped in the chat log
\patch 5148 Date 3/27/2007 by Jirka
- Improved: UI - Chat log text border
\patch 5196 Date 12/11/2007 by Ondra
- New: Added difficulty option VonID (VON ID), allowing to see in-game who is currently speaking over the radio.
\patch 5209 Date 1/2/2008 by Ondra
- Fixed: When switching VON channels quickly, the VON ID indication could still show the old channel.
*/

void ChatList::OnDraw()
{
	// if (GWorld->GetTitleEffect()) return; // hide chat when titles appears

	if (_rows <= 0 || !_font) return;

  SCOPE_GRF("chatList",0);
  
  bool multiplayer = GetNetworkManager().GetClientState() > NCSNone;
  if (!multiplayer && !GWorld->IsSimulationEnabled()) return;

	const float startDim = 25, endDim = 30;
	int w = GEngine->Width2D();
	int h = GEngine->Height2D();

// patch
	float top = _y + (_rows - 1) * _h;
	int row = _rows - 1;

  float shadowX = 0.075 * _h;
  float shadowY = 0.1 * _h;

  AutoArray< SpeakingIndication, MemAllocLocal<SpeakingIndication, 64> > speaking;
  
  // traverse all players
  const AutoArray<PlayerIdentity> *pid = GetNetworkManager().GetIdentities();
  if (pid && Glob.config.IsEnabled(DTVonID))
  {
    for (int i=0; i<pid->Size(); i++)
    {
      const PlayerIdentity &identity = pid->Get(i);
      bool isSpeaking = false;
      ChatChannel cc = CCGlobal;
      if (identity.dpnid == GetNetworkManager().GetPlayer())
      {
        isSpeaking = GetNetworkManager().IsVoiceRecording();
        // check which channel are we transmitting on
        cc = ActualChatChannel();
        if (cc==CCDirect) isSpeaking = false;
      }
      else
      {
        isSpeaking = GetNetworkManager().IsVoicePlaying(identity.dpnid);
        if (isSpeaking)
        {
          bool audible2D,audible3D;
          int channel = GetNetworkManager().CheckVoiceChannel(identity.dpnid,audible2D,audible3D);
          if (!audible2D)
          {
            isSpeaking = false;
          }
          else
          {
            // note: CheckVoiceChannel return the information as VoNChatChannel
            extern ChatChannel VoNChatChannel2ChatChannel(int channel);
            cc = VoNChatChannel2ChatChannel(channel);
          }
        }
      }
      
      if (isSpeaking)
      {
        SpeakingIndication &item = speaking.Append();
        item.name = identity.name;
        item.channel = cc;
        item.lastSpeaking = Glob.uiTime;
        _lastSpeaking = Glob.uiTime;
      }
    }
  }
  
  // merge the list of speakers into a more stable one
  for (int i=0; i<speaking.Size(); i++)
  {
    const SpeakingIndication &item = speaking[i];
    // check if the speaker is already present in the stable list
    bool alreadyThere = false;
    for (int j=0; j<_speaking.Size(); j++)
    {
      SpeakingIndication &stable = _speaking[j];
      if (!strcmp(stable.name,item.name))
      {
        stable.lastSpeaking = item.lastSpeaking;
        stable.channel = item.channel;
        alreadyThere = true;
        break;
      }
    }
    if (!alreadyThere)
    {
      _speaking.Append(item);
    }
  }
  // expire speakers which did not talk for some time
  int s,d;
  for (s=d=0; s<_speaking.Size(); s++)
  {
    if (_speaking[s].lastSpeaking>=Glob.uiTime-1.0f)
    {
      if (d!=s) _speaking[d] = _speaking[s];
      d++;
    }
  }
  _speaking.Resize(d);
  _speaking.CompactIfNeeded();

  const float border = TEXT_BORDER;
  // when we indicate somebody speaking, we use the last row for it
  // prevent the row disappearing too often by checking _lastSpeaking
  if (_speaking.Size()>0 || _lastSpeaking>Glob.uiTime-2.0f)
  {
    // display the information
    float x = _x + border;
    float lineWidth = _w - 2.0f * border;
    
    for(int i=0; i<_speaking.Size(); i++)
    {
      const SpeakingIndication &item = _speaking[i];
		  float alpha = 1;
		  PackedColor color = PackedColorRGB(_colors[item.channel], toIntFloor(_colors[item.channel].A8() * alpha));
		  PackedColor colorB = PackedColorRGB(_bgColor, toIntFloor(_bgColor.A8() * alpha));
  		PackedColor colorShadow = PackedColorRGB(_shadowColor, toIntFloor(_shadowColor.A8() * alpha));

      float left = _x + border;
		  float wline = GEngine->GetTextWidth(_size, _font, item.name) + 2.0f * border;
  		
		  // check if there is enough space to display
		  if (wline>lineWidth) break;
	    MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(NULL, 0, 0);
	    GEngine->Draw2D(mip, colorB, Rect2DPixel((x-border) * w, top * h, wline * w, _h * h));
	    
      if(_shadow == 1)
      {
        GEngine->DrawText(
          Point2DFloat(x + shadowX, top + 0.5f * (_h - _size)+ shadowY) , _size,
          Rect2DFloat(left, top, _w + shadowX, _h + shadowX),
          _font, colorShadow, item.name, 0
          );
      }

      GEngine->DrawText(
        Point2DFloat(x, top + 0.5f * (_h - _size)), _size,
        Rect2DFloat(left, top, _w, _h),
        _font, color, item.name, _shadow
        );
	    
	    // insert one space between speaker names
	    float space = GEngine->GetTextWidth(_size, _font, " ");
	    x += wline+space;
	    lineWidth -= wline+space;
    }
    
		row--;
		if (row < 0) return;
		top -= _h;
	}

	int n = Size();
	if (n == 0) return;
	int begin = _offset;
	saturate(begin, 0, n - 1);
	for (int i=begin; i<n; i++)
	{
		const ChatItem &item = Get(i);
		if (item.text.GetLength() == 0) continue;
		if (!Enabled() && !item.forceDisplay) continue; 
    float age = multiplayer ? Glob.uiTime - item.uiTime : Glob.time - item.gameTime;
		float alpha = 1;
		if (_offset == -1 && age > startDim)
		{
			// interpolate dim
			if (age > endDim) continue;
			alpha = (endDim - age) * (1.0 / (endDim - startDim));
		}
		RString buffer;
		if (item.from.GetLength() == 0)
			buffer = item.text;
		else
			buffer = item.from + RString(": \"") + item.text + RString("\"");
    PackedColor color = PackedColorRGB(_colors[item.channel], toIntFloor(_colors[item.channel].A8() * alpha));
    if (item.channel==CCStatus && !item.colorClass.IsEmpty())
    {
      const ParamEntry &cls = Pars >> "RscChatListDefault";
      if (cls.FindEntry(item.colorClass))
      {
        color = GetPackedColor(cls >> item.colorClass);
      }
    }
		PackedColor colorB = PackedColorRGB(_bgColor, toIntFloor(_bgColor.A8() * alpha));
		PackedColor colorShadow = PackedColorRGB(_shadowColor, toIntFloor(_shadowColor.A8() * alpha));
/*
		if (item.playerMsg)
		{
			// make background less intensive
			colorB = PackedColorRGB(color, color.A8()/2);
			// make foreground more intensive
			color = PackedColor(Color(0, 0, 0, alpha));
		}
*/
    // player messages indication now using the icons
    Texture *texture = NULL; //item.playerMsg ? _iconPlayerMessage : _iconNormalMessage;
    if(item.playerMsg)
    {
      colorB = color;
      color = PackedWhite; 
    }

    float texW = 0;
    if (texture && texture->AHeight() > 0)
    {
      // keep aspect of the texture source
      texW = _h * (texture->AWidth() * h) / (texture->AHeight() * w);
    }

		AUTO_STATIC_ARRAY(int, lines, 32);
		lines.Add(0);

    float left = _x + border;
    float lineWidth = _w - 2.0f * border;

		if (texW + GEngine->GetTextWidth(_size, _font, buffer) > lineWidth)
		{
			const char *p = buffer;
			const char *word = NULL;
			float width = texW;
			while (*p != 0)
			{
				const char *pLast = p;
				char c = *p++;
				if ((unsigned char)c <= 32)
				{
					word = p;
				}

        // read the single character in UTF-8 encoding
        char temp[7];
        int j = 0;
        temp[j++] = c;
        if (c & 0x80)
        {
          DoAssert((c & 0xc0) == 0xc0); // the first character
          while (((c = *p) & 0xc0) == 0x80)
          {
            p++;
            if (j < 5) temp[j++] = c;
            else
            {
              Fail("Character too long");
            }
          };
        }
        temp[j] = 0;
        width += GEngine->GetTextWidth(_size, _font, temp);

				if (width > lineWidth)
				{
					if (word) p = word;
					else p = pLast;
					lines.Add(p - buffer);
					word = NULL;
					width = 0;
				}
			}
		}

		for (int i=lines.Size()-1; i>=0; i--)
		{
			char *line = buffer.MutableData() + lines[i];

			float wline = GEngine->GetTextWidth(_size, _font, line) + 2.0f * border;
      if (i == 0) wline += texW;
			MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(NULL, 0, 0);
			GEngine->Draw2D(mip, colorB, Rect2DPixel(_x * w, top * h, wline * w, _h * h));
      if (i == 0 && texW > 0)
      {
        MipInfo mip = GEngine->TextBank()->UseMipmap(texture, 0, 0);
        GEngine->Draw2D(mip, color, Rect2DPixel(left * w, top * h, texW * w, _h * h));
        left += texW;
      }

      if(_shadow == 1)
      {
        GEngine->DrawText(
          Point2DFloat(left + shadowX, top + 0.5f * (_h - _size)+ shadowY) , _size,
          Rect2DFloat(_x, top, _w + shadowX, _h + shadowX),
          _font, colorShadow, line, 0
          );
      }
			GEngine->DrawText
			(
				Point2DFloat(left, top + 0.5 * (_h - _size)), _size,
				Rect2DFloat(_x, top, _w, _h),
				_font, color, line, _shadow
			);
			*line = 0;

			row--;
			if (row < 0) return;
			top -= _h;
		}
	}
}

class WhatUnitsCrewFunc : public ICrewFunc
{
protected:
  RefArray<NetworkObject> &_units;

public:
  WhatUnitsCrewFunc(RefArray<NetworkObject> &units) : _units(units) {}

  virtual bool operator () (Person *person)
  {
    if (person && person->IsRemotePlayer() && person->Brain() && person->Brain()->GetLifeState() != LifeStateDead)
      _units.Add(person);
    return false; // continue
  }
};

static void WhatUnitsVehicle(RefArray<NetworkObject> &units, Transport *veh)
{
  WhatUnitsCrewFunc func(units);
  veh->ForEachCrew(func);
}

/*
static void WhatUnitsAll(RefArray<NetworkObject> &units)
{
	AICenter *center = GWorld->GetEastCenter();
	if (center) WhatUnitsSide(units, center);
	center = GWorld->GetWestCenter();
	if (center) WhatUnitsSide(units, center);
	center = GWorld->GetGuerrilaCenter();
	if (center) WhatUnitsSide(units, center);
	center = GWorld->GetCivilianCenter();
	if (center) WhatUnitsSide(units, center);
}
*/

//! Retrieves list of players, which can receive given message
/*!
	\param units retrieved list of units
	\param channel channel on which communication runs
	\param object channel's owner 
*/
void WhatUnits(RefArray<NetworkObject> &units,  ChatChannel channel, NetworkObject *object)
{
	switch (channel)
	{
		case CCVehicle:
			{
				Transport *veh = dynamic_cast<Transport *>(object);
				if (veh) WhatUnitsVehicle(units, veh);
			}
			break;
		case CCGlobal:
/*
			WhatUnitsAll(units);
*/
			break;
    case CCDirect:
#if _ENABLE_DIRECT_MESSAGES
    case CCDirectSpeaking:
#endif
      {
        Person *person = dynamic_cast<Person *>(object);
        if (person) WhatUnitsDirect(units, person);
      }
      break;
		default:
      if (channel>=CCCustom1 && channel<=CCCustomLast)
      {
        CustomChannelInfo &info = GCustomChannels.GetCustomChannelInfo(channel);
        units = info.units;
        break;
      }
			ErrF("Bad radio channel %d (for object %s)",toInt(channel),cc_cast(object->GetDebugName()));
			break;
	}
}

//! Retrieves list of players, which can receive given message
/*!
	\param unit sender unit
	\param units retrieved list of units
	\param channel channel on which communication runs
*/
void WhatUnits(AIBrain *unit, RefArray<NetworkObject> &units, ChatChannel channel)
{
	switch (channel)
	{
		case CCVehicle:
			{
				Transport *veh = unit->GetVehicleIn();
				if (veh) WhatUnitsVehicle(units, veh);
			}
			break;
		case CCGlobal:
			break;
		case CCDirect:
#if _ENABLE_DIRECT_MESSAGES
    case CCDirectSpeaking:
#endif
			{
        Person *person = unit->GetPerson();
        if (person) WhatUnitsDirect(units, person);
      }
			break;
    case CCItemTransmitter:
      GetNetworkManager().WhatUnitsOnTransmitterChannel(units);
      break;
    case CCNone:
			break;
		default:
      if (channel>=CCCustom1 && channel<=CCCustomLast)
      {
        CustomChannelInfo &info = GCustomChannels.GetCustomChannelInfo(channel);
        units = info.units;
        break;
      }
      ErrF("Bad radio channel %d (for unit %s)",toInt(channel), cc_cast(unit->GetDebugName()));
			break;
	}
}

//! Retrieves list of players, which can receive given message
/*!
	Assume player is a message sender.
	\param units retrieved list of units
	\param channel channel on which communication runs
*/
static void WhatUnits(RefArray<NetworkObject> &units, ChatChannel channel)
{
	Person *person = GWorld->GetRealPlayer();
	if (!person) return;
	AIBrain *unit = person->Brain();
	if (!unit) return;

	WhatUnits(unit, units, channel);
}

//! Sends chat message by player
/*!
	\param channel channel on which communication runs
	\text message text
*/
void SendChat(ChatChannel channel, RString text)
{
	if (text.GetLength() > 0 && text[0] == '#')
	{
		GChatList.Add(channel, "", text, false, true);
		GetNetworkManager().ProcessCommand(text);
		return;
	}

	AIBrain *sender = GWorld->GetRealPlayer() ? GWorld->GetRealPlayer()->Brain() : NULL; 

  if (
    GetNetworkManager().GetClientState() != NCSPlaying ||
    !sender || sender->GetLifeState()==LifeStateDead
  )
	{
		GetNetworkManager().Chat(channel, text);
		GChatList.Add(channel, GetFullPlayerName(), text, false, true);
	}
	else
	{
		// send actual state
		RefArray<NetworkObject> units;
		WhatUnits(units, channel);
		if (channel == CCGlobal || units.Size() > 0)
			GetNetworkManager().Chat(channel, sender, units, text);

		GChatList.Add(channel, sender, text, false, true);
	}
}

//! Sends mission radio message
/*!
	\param channel channel on which communication runs
	\param object channel's owner 
	\param wave name of class with message description
	\param sender sender unit
	\param senderName sender name
*/
void SendRadioChatWave
(
	ChatChannel channel,
	NetworkObject *object,
	RString wave,
	AIBrain *sender, RString senderName
)
{
	if (GetNetworkManager().GetClientState() != NCSPlaying) return;
	RefArray<NetworkObject> units;
	WhatUnits(units, channel, object);
	if (channel == CCGlobal || units.Size() > 0)
		GetNetworkManager().RadioChatWave(channel, units, wave, sender, senderName);
}

//! Sends mission radio message
/*!
	\param channel channel on which communication runs
	\param wave name of class with message description
	\param sender sender unit
	\param senderName sender name
*/
void SendRadioChatWave
(
	ChatChannel channel,
	RString wave,
	AIBrain *sender, RString senderName
)
{
	if (GetNetworkManager().GetClientState() != NCSPlaying) return;
	RefArray<NetworkObject> units;
	WhatUnits(sender, units, channel);
	if (channel == CCGlobal || units.Size() > 0)
		GetNetworkManager().RadioChatWave(channel, units, wave, sender, senderName);
}

//! Sends radio message
/*!
	\param channel channel on which communication runs
	\param sender sender unit
	\param object channel's owner 
	\param text text of message
	\param sentence radio sentence of message
*/
void SendRadioChat
(
	ChatChannel channel, AIBrain *sender,
	NetworkObject *object,
	RString text, RadioSentence &sentence
)
{
	if (GetNetworkManager().GetClientState() != NCSPlaying) return;
	RefArray<NetworkObject> units;
	WhatUnits(units, channel, object);
	if (channel == CCGlobal || units.Size() > 0)
		GetNetworkManager().RadioChat(channel, sender, units, text, sentence);
}

//! Sends marker
/*!
	\param channel channel on which communication runs
	\param sender sender unit
	\param marker marker description
*/
void SendMarker
(
	ChatChannel channel, AIBrain *sender, ArcadeMarkerInfo &marker
)
{
	if (GetNetworkManager().GetClientState() < NCSGameLoaded) return;
	RefArray<NetworkObject> units;
	WhatUnits(units, channel);
		GetNetworkManager().MarkerCreate(channel, sender, units, marker);
}

//! Channel selection display
class DisplayChannel : public Display
{
public:
	//! actual selected channel
	static int _channel;
  //! backup of selected channel (forcing Push to Talk Channel actions to change _channel only temporary)
  static int _backupChannel;
  //! list of disabled channels (configured by disableChannels entry in mission descriptionExt)
  static AutoArray<int> _disabledChannels;

  //! special CCItemTransmitter channels
  static int _itemTransmitterChannel;
  static RefArray<ItemTransmitter> _itemTransmitterChannels;

public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayChannel(ControlsContainer *parent);
	void OnButtonClicked(int idc) {}
	void ResetHUD();
};

int DisplayChannel::_channel = CCGlobal;
int DisplayChannel::_backupChannel = CCGlobal;
AutoArray<int> DisplayChannel::_disabledChannels;
int DisplayChannel::_itemTransmitterChannel = 0;
RefArray<ItemTransmitter> DisplayChannel::_itemTransmitterChannels;

//! Creates channel selection display
AbstractOptionsUI *CreateChannelUI()
{
	return new DisplayChannel(NULL);
}

//! returns currently selected chat channel
ChatChannel ActualChatChannel()
{
	return (ChatChannel)DisplayChannel::_channel;
}

//! Backup chat channel
void BackupChatChannel()
{
  DisplayChannel::_backupChannel = DisplayChannel::_channel;
}

//! Restore chat channel
void RestoreChatChannel()
{
  if (DisplayChannel::_channel != DisplayChannel::_backupChannel)
  {
    DisplayChannel::_channel = DisplayChannel::_backupChannel;
    GWorld->OnChannelChanged();
  }
}

bool IsPlayerDead();

bool ChannelValid()
{
  int channel = DisplayChannel::_channel;
  // something need to be enabled to avoid infinite loop
  if (channel==CCDirect) return true;

  if (channel>=CCCustom1 && channel<=CCCustomLast)
  {
    return GCustomChannels.ChannelActive(channel);
  }
  if (channel==CCSystem || channel==CCStatus) return false;
  if (channel==CCGlobal)
  {
    // global channel is always valid for the admin
    if (GetNetworkManager().IsServer() || GetNetworkManager().IsGameMaster()) return true;
    else return false; // global channel enabled for admins only in DayZ
  }
  if (channel==CCItemTransmitter)
  {
    bool ret = (DisplayChannel::_itemTransmitterChannel < DisplayChannel::_itemTransmitterChannels.Size() && DisplayChannel::_itemTransmitterChannel>=0);
    if (ret)
    {
      Ref<ItemTransmitter> tr = DisplayChannel::_itemTransmitterChannels[DisplayChannel::_itemTransmitterChannel];
      if (tr->IsOn())
      {
        GetNetworkManager().SetVoNTransmitterFreq(tr->tunedFrequency);
      }
      else return false;
    }
    return ret;
  }
  if (!IsPlayerDead())
  {
    NetworkClientState state =GetNetworkManager().GetClientState();
    if (state==NCSPlaying)
    {
      // for ingame player any channel is always valid
      // TODO: sometimes we may want to disable group or vehicle channels?
      for (int i=0; i<DisplayChannel::_disabledChannels.Size(); i++)
      {
        if (DisplayChannel::_disabledChannels[i]==channel) return false; //channel disabled by mission descriptionExt config
      }
      return true;
    }
  }
  return false;
}

//! Enumerate all players ItemTransmitters
static void RefreshItemTransmitters()
{
  DisplayChannel::_itemTransmitterChannels.Resize(0);
  Person *player = GWorld->PlayerOn();
  if (player)
  {
    Ref<EntityType> type=VehicleTypes.New("ItemTransmitter","");
    player->ForEachItem([type](InventoryItem *item) -> bool
    {
      if (item->Type()==type.GetRef())
      {
        ItemTransmitter *radio = static_cast<ItemTransmitter *>(item);
        DisplayChannel::_itemTransmitterChannels.Add(radio);
        //RptF(" radio: %x, freqv: %.1f",radio, radio->tunedFrequency);
      }
      return false; // continue
    });  
  }
  if (DisplayChannel::_itemTransmitterChannel>=DisplayChannel::_itemTransmitterChannels.Size())
    DisplayChannel::_itemTransmitterChannel = DisplayChannel::_itemTransmitterChannels.Size()-1;
}

//! Select next chat channel
void NextChatChannel()
{
  RefreshItemTransmitters(); //fill in DisplayChannel::_itemTransmitterChannels
  do 
  {
    if (DisplayChannel::_channel == CCItemTransmitter)
    {
      DisplayChannel::_itemTransmitterChannel++;
      if (DisplayChannel::_itemTransmitterChannel >= DisplayChannel::_itemTransmitterChannels.Size()) DisplayChannel::_channel++;
    }
    else 
    {
      DisplayChannel::_channel++;
      if (DisplayChannel::_channel == CCItemTransmitter) DisplayChannel::_itemTransmitterChannel = 0;
    }
	  if (DisplayChannel::_channel >= CCN) DisplayChannel::_channel = 0;
	  // we need to validate if we are allowed to use the channel
	  // if not, we need to switch to another one
  } while(!ChannelValid());
}

//! Select previous chat channel
void PrevChatChannel()
{ 
  RefreshItemTransmitters();
  do 
  {
    if (DisplayChannel::_channel == CCItemTransmitter)
    {
      DisplayChannel::_itemTransmitterChannel--;
      if (DisplayChannel::_itemTransmitterChannel < 0) DisplayChannel::_channel--;
    }
    else
    {
      DisplayChannel::_channel--;
      if (DisplayChannel::_channel == CCItemTransmitter) DisplayChannel::_itemTransmitterChannel = DisplayChannel::_itemTransmitterChannels.Size()-1;
    }
	  if (DisplayChannel::_channel >= CCN) DisplayChannel::_channel = 0;
	  
  	if (DisplayChannel::_channel < 0) DisplayChannel::_channel = CCN - 1;
	  // we need to validate if we are allowed to use the channel
	  // if not, we need to switch to another one
  } while(!ChannelValid());
}

//! Select chat channel
void SetChatChannel(ChatChannel channel)
{
  DisplayChannel::_channel = channel;
  if ( !ChannelValid() ) NextChatChannel();
}

void ResetDisabledChannels()
{
  DisplayChannel::_disabledChannels.Clear();
}

void DisableChannel(int chan)
{
  DisplayChannel::_disabledChannels.Add(chan);
}

bool ChannelDisabled(int chan)
{
  for (int i=0; i<DisplayChannel::_disabledChannels.Size(); i++)
  {
    if (DisplayChannel::_disabledChannels[i]==chan) return true; //channel disabled by mission descriptionExt config
  }
  return false;
}

DisplayChannel::DisplayChannel(ControlsContainer *parent)
:Display(parent)
{
	Load("RscDisplayChannel");
	SetCursor(NULL);
	ResetHUD();
}

void DisplayChannel::ResetHUD()
{
  int channel = _channel;

	CStatic *text = dynamic_cast<CStatic *>(GetCtrl(IDC_CHANNEL));
	if (text)
	{
    if (channel>=CCCustom1 && channel<=CCCustomLast)
    {
      CustomChannelInfo &info = GCustomChannels.GetCustomChannelInfo(channel);
      text->SetText(info.label);
    }
    else if (channel==CCItemTransmitter && ChannelValid())
    {
      ItemTransmitter *radio = _itemTransmitterChannels[_itemTransmitterChannel];
      text->SetText(Format("%.1f FM", radio->tunedFrequency));
    }
		else 
    {
      text->SetText(LocalizeString(IDS_CHANNEL_GLOBAL + channel));
    }
		text->SetTextColor(GChatList.GetColor(channel));
	}

  text = dynamic_cast<CStatic *>(GetCtrl(IDC_CHANNEL_VOICE));
  if (text)
  {
    text->SetTextColor(GChatList.GetColor(channel));
  }
}

//! Chat line display
/*!
	Enables write chat messages
*/
class DisplayChatLine : public Display
{
public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayChatLine(ControlsContainer *parent);
  ~DisplayChatLine();

	void OnButtonClicked(int idc);
	bool OnKeyDown(int dikCode);
	bool OnKeyUp(int dikCode);
	void DestroyHUD(int exit);
};

/*!
\patch 5139 Date 3/14/2007 by Ondra
- Fixed: All input ignored after MP session left while chat window still open.
*/
DisplayChatLine::DisplayChatLine(ControlsContainer *parent)
:Display(parent)
{
  GInput.ChangeGameFocus(+1); //call it as early as possible in order not to skip it by some possible return
	Load("RscDisplayChat");
	SetCursor(NULL);
	// make sure chat in a screen which disabled display does not enable it
  _enableDisplay = _enableDisplay && (!parent || parent->DisplayEnabled());

  CEdit *edit = dynamic_cast<CEdit *>(GetCtrl(IDC_CHAT));
  if (edit)
  {
    /// Limit for the chat text (avoid too long messages)
    edit->SetMaxChars(MaxChatLength);
  }
}

DisplayChatLine::~DisplayChatLine()
{
  GInput.ChangeGameFocus(-1);
}

void DisplayChatLine::OnButtonClicked(int idc)
{
	if (idc == IDC_OK)
	{
		CEdit *edit = dynamic_cast<CEdit *>(GetCtrl(IDC_CHAT));
		if (edit)
		{
			SendChat(ActualChatChannel(), edit->GetText());
		}
	}
	Display::OnButtonClicked(idc);
}

//! Global chat history
ChatList GChatList;

/*!
\patch 1.42 Date 1/10/2002 by Jirka
- Added: browsing in chat history using Page Up and Page Down in chat line 
*/

bool DisplayChatLine::OnKeyDown(int dikCode)
{
	// catch RETURN, ESC, UP, DOWN, PAGE UP, PAGE DOWN (avoid transmit into game)
	if (dikCode == DIK_ESCAPE || dikCode == DIK_RETURN || dikCode == DIK_NUMPADENTER ||
		dikCode == DIK_UP || dikCode == DIK_DOWN ||
    dikCode == DIK_PRIOR || dikCode == DIK_NEXT) return true;

	return Display::OnKeyDown(dikCode);
}

bool DisplayChatLine::OnKeyUp(int dikCode)
{
	if (dikCode == DIK_RETURN || dikCode == DIK_NUMPADENTER)
	{
		OnButtonClicked(IDC_OK);
		return true;
	}
	else if (dikCode == DIK_ESCAPE)
	{
		OnButtonClicked(IDC_CANCEL);
		return true;
	}
	else if (dikCode == DIK_DOWN)
	{
		PrevChatChannel();
		if (GWorld->ChatChannel()) GWorld->ChatChannel()->ResetHUD();
		if (GWorld->VoiceChat()) GWorld->VoiceChat()->ResetHUD();
		GWorld->OnChannelChanged();
		return true;
	}
	else if (dikCode == DIK_UP)
	{
		NextChatChannel();
		if (GWorld->ChatChannel()) GWorld->ChatChannel()->ResetHUD();
		if (GWorld->VoiceChat()) GWorld->VoiceChat()->ResetHUD();
		GWorld->OnChannelChanged();
		return true;
	}
	else if (dikCode == DIK_PRIOR)
	{
		GChatList.BrowseUp();
		return true;
	}
	else if (dikCode == DIK_NEXT)
	{
		GChatList.BrowseDown();
		return true;
	}

	return Display::OnKeyUp(dikCode);
}

void DisplayChatLine::DestroyHUD(int exit)
{
	GWorld->DestroyChat(exit);
	GChatList.BrowseReset();
}

//! Creates chat line display
AbstractOptionsUI *CreateChatUI()
{
	return new DisplayChatLine(NULL);
}

//! Voice chat indicator display
/*!
	Display picture when voice recording is enabled.
	Color of picture indicates channel on which transfer runs.
*/
class DisplayVoiceChat : public Display
{
public:
	//! constructor
	/*!
		\param parent parent display
	*/
	DisplayVoiceChat(ControlsContainer *parent);
	bool OnKeyDown(int dikCode);
	bool OnKeyUp(int dikCode);
	void OnSimulate(EntityAI *vehicle);
	void DestroyHUD(int exit);

	void ResetHUD();
};

DisplayVoiceChat::DisplayVoiceChat(ControlsContainer *parent)
:Display(parent)
{
	Load("RscDisplayVoiceChat");
	SetCursor(NULL);

	ResetHUD();
}

bool DisplayVoiceChat::OnKeyDown(int dikCode)
{
	return false;
}

bool DisplayVoiceChat::OnKeyUp(int dikCode)
{
	return false;
}

/*!
\patch 5112 Date 1/3/2007 by Bebul
- New: Push to Talk actions for voice over net
*/
void DisplayVoiceChat::OnSimulate(EntityAI *vehicle)
{
	if (GInput.GetActionToDo(UAVoiceOverNet, true, false))
	{
    // just disable voice recording from microphone and request disabling of voice transmission
    // we can't disable transmission right now, because we have to wait some time for messages to be sent
    GetNetworkManager().SetVoiceCapture(false);
    GetNetworkManager().SetDisableVoiceRequest(INetworkManager::DVRDisableVoiceToggle);
    GInput.VoNToggleOn = false;
    GetNetworkManager().SetVoiceToggleOn(false);
    Exit(IDC_CANCEL);
	}
  else if ( !GInput.VoNToggleOn ) 
  {
    if (
      !GInput.GetAction(UAPushToTalk) && 
      !GInput.GetAction(UAPushToTalkAll) && 
      !GInput.GetAction(UAPushToTalkSide) && 
      !GInput.GetAction(UAPushToTalkCommand) && 
      !GInput.GetAction(UAPushToTalkGroup) && 
      !GInput.GetAction(UAPushToTalkVehicle) && 
      !GInput.GetAction(UAPushToTalkDirect)
    )
    {
      RestoreChatChannel();

      // just disable voice recording from microphone and request disabling of voice transmission
      // we can't disable transmission right now, because we have to wait some time for messages to be sent
      GetNetworkManager().SetVoiceCapture(false);
      GetNetworkManager().SetDisableVoiceRequest(INetworkManager::DVRDisableVoice);
      Exit(IDC_CANCEL);
    }
  }
}

void DisplayVoiceChat::ResetHUD()
{
	ChatChannel channel = ActualChatChannel();
  GetNetworkManager().SetVoiceChannel(channel);

	CStatic *text = dynamic_cast<CStatic *>(GetCtrl(IDC_VOICE_CHAT));
	if (text)
	{
//		text->SetText(LocalizeString(IDS_CHANNEL_GLOBAL + channel));
		text->SetTextColor(GChatList.GetColor(channel));
	}
}

void DisplayVoiceChat::DestroyHUD(int exit)
{
	GWorld->DestroyVoiceChat(exit);
}

AbstractOptionsUI *CreateVoiceChatUI()
{
	return new DisplayVoiceChat(NULL);
}

void PersonsRefArray2NO(RefArray<Person> &persons, RefArray<NetworkObject> &netObj)
{
  int size = persons.Size();
  if (size)
  {
    netObj.Realloc(size);
    netObj.Resize(size);
    for (int i=0; i<size; i++)
    {
      netObj[i] = persons[i]; //copy (and cast)
    }
  }
}

void NORefArray2Persons(RefArray<NetworkObject> &netObj, RefArray<Person> &persons)
{
  int size = netObj.Size();
  if (size)
  {
    persons.Realloc(size);
    persons.Resize(size);
    for (int i=0; i<size; i++)
    {
      persons[i] = static_cast<Person *>(netObj[i].GetRef()); //copy (and static_cast)
    }
  }
}

static const EnumName ChatChannelNames[]=
{
  EnumName(CCNone, "NONE"),
  EnumName(CCGlobal, "GLOBAL"),
  EnumName(CCVehicle, "VEHICLE"),
  EnumName(CCDirect, "DIRECT"),
  EnumName(CCCustom1, "CUSTOM1"),
  EnumName(CCCustom2, "CUSTOM2"),
  EnumName(CCCustom3, "CUSTOM3"),
  EnumName(CCCustom4, "CUSTOM4"),
  EnumName(CCCustom5, "CUSTOM5"),
  EnumName(CCCustom6, "CUSTOM6"),
  EnumName(CCCustom7, "CUSTOM7"),
  EnumName(CCCustom8, "CUSTOM8"),
  EnumName(CCCustom9, "CUSTOM9"),
  EnumName(CCCustom10, "CUSTOM10"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(ChatChannel dummy)
{
  return ChatChannelNames;
}
