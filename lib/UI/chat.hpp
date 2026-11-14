#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CHAT_HPP
#define _CHAT_HPP

#include <Es/Strings/rString.hpp>
#include "../global.hpp"
#include "../engine.hpp"
#include "../speaker.hpp"

/*!
\file
Interface file for chat processing - chat edit, chat (radio message) list
*/

#define ENABLE_CHAT_DEFAULT true

//! channel for chat
DEFINE_ENUM_BEG(ChatChannel)
	CCNone = -1,
	CCGlobal,
	CCVehicle,
  CCItemTransmitter,
	CCDirect,
  CCCustom1,
  CCCustom2,
  CCCustom3,
  CCCustom4,
  CCCustom5,
  CCCustom6,
  CCCustom7,
  CCCustom8,
  CCCustom9,
  CCCustom10,
  CCCustomLast = CCCustom10,
#if _ENABLE_DIRECT_MESSAGES
  CCDirectSpeaking, // TODO: merge CCDirect and CCDirectSpeaking
#endif
  CCStatus,
  CCSystem,
	CCN
DEFINE_ENUM_END(ChatChannel)

#define CCCustomN (CCCustomLast-CCCustom1+1)

ChatChannel ActualChatChannel();

//! single row of chat history
struct ChatItem
{
	ChatChannel channel;
	RString from;
	Clock time;
	RString text;
	UITime uiTime;
  Time gameTime;
	bool playerMsg;
	bool forceDisplay;
  RString colorClass;
};
TypeIsMovableZeroed(ChatItem);

class NetworkObject;
class AIBrain;
class RadioSentence;

/// indication about who is currently speaking
struct SpeakingIndication
{
  /// name of the player
  RString name;
  /// which channel (for color coding)
	ChatChannel channel;
	/// time when we were last speaking
	UITime lastSpeaking;
};

TypeIsMovableZeroed(SpeakingIndication)

//! Chat history
/*!
	This class encapsulates both data and drawing of chat history.
*/
class ChatList : public AutoArray<ChatItem>
{
protected:
	// chat parameters
	//@{
	//! position (top) of chat list
	float _x;			// top
	//! position (left) of chat list
	float _y;			// left
	//! width of chat list
	float _w;			// width
  //! height of the single row
  float _h;			// width
	//! maximal number of displayed rows
	int _rows;

	//! used font
	Ref<Font> _font;
	//! font size
	float _size;
  //enabled shadow
  int _shadow;
  //shadow color
  PackedColor _shadowColor;

  /// indication of messages not important to player
  //Ref<Texture> _iconNormalMessage;
  /// indication of messages important to player
  //Ref<Texture> _iconPlayerMessage;

	//! colors of texts for each channel
	PackedColor _colors[CCN];
	//! background color
	PackedColor _bgColor;
  //! player background color
  PackedColor _bgPlayerColor;
	//! if is drawing enabled
	bool _enable;
  //! if is drawing enabled by the script
  bool _scriptEnable;

	//! offset for browsing
	int _offset;
	
	/// last time when somebody was speaking
	/** done so that line is not disappearing too frequently */
	UITime _lastSpeaking;

	/// list of active speakers
  AutoArray<SpeakingIndication> _speaking;
public:
	//! constructor
	ChatList();

  /// free resources (used in DDTerm to avoid holding the textures)
  void Free();

	//! returns color of texts for given channel
	PackedColor GetColor(int channel) {return _colors[channel];}
  void SetColor(int channel, PackedColor &color) { _colors[channel]=color; }

	//! load placement and parameters of chat list
	void Load(ParamEntryPar cls);
	
	//! adds single message to chat list
	/*!
		\param channel on which channel communication runs
		\param from sender name
		\param text text of message
		\param playerMsg if message has enforcement for player
		\param forceDisplay 
		\patch 1.01 Date 7/10/2001 by Jirka
		- Fixed: display chat messages even when radio protocol is disabled
		\patch_internal 1.01 Date 7/10/2001 by Jirka
		- parameter forceDisplay added
	*/
	void Add(ChatChannel channel, RString from, RString text, bool playerMsg, bool forceDisplay);
	//! adds single message to chat list
	/*!
		\param channel on which channel communication runs
		\param sender sender unit
		\param text text of message
		\param playerMsg if message has enforcement for player
		\param forceDisplay 
		\patch_internal 1.01 Date 7/10/2001 by Jirka
		- display chat messages even when radio protocol is disabled
		- parameter forceDisplay added
	*/
	void Add(ChatChannel channel, AIBrain *sender, RString text, bool playerMsg, bool forceDisplay);
	//! adds single message to chat list using CCStatus
	/*!
		\param text text of message
    \param colorClass of message
	*/
	void Add(RString text, RString colorClass);

  /// variant of Add with converting to uppercase
  void AddUC(ChatChannel channel, RString from, RString text, bool playerMsg, bool forceDisplay);
  /// variant of Add with converting to uppercase
  void AddUC(ChatChannel channel, AIBrain *sender, RString text, bool playerMsg, bool forceDisplay);

	//! enables / disables displaying of chat list
	void Enable(bool enable) {_enable = enable;}
  //! enables / disables displaying of chat list from the script
  bool ScriptEnable(bool enable) {bool enabled = _scriptEnable; _scriptEnable = enable; return enabled;}
	//! returns if displaying of chat list is enabled
	bool Enabled() const {return _enable && _scriptEnable;}
  //! returns if displaying of chat list is enabled by the profile
  bool ProfileEnabled() const {return _enable;}
  //! returns if displaying of chat list is enabled by the script
  bool ScriptEnabled() const {return _scriptEnable;}

	//! draws chat list
	void OnDraw();

	//! browse line up
	void BrowseUp();
	//! browse line down
	void BrowseDown();
	//! cancel browse mode
	void BrowseReset();
};

void SendChat(ChatChannel channel, RString text);
void SendRadioChat(ChatChannel channel, AIBrain *sender, NetworkObject *object, RString text, RadioSentence &sentence);
void SendRadioChatWave(ChatChannel channel, NetworkObject *object, RString wave, AIBrain *sender, RString senderName);
void SendRadioChatWave(ChatChannel channel, RString wave, AIBrain *sender, RString senderName);
void SendMarker(ChatChannel channel, AIBrain *sender, ArcadeMarkerInfo &marker);

//! operates on DisplayChannel::_disabledChannels
void ResetDisabledChannels();
void DisableChannel(int chan);
bool ChannelDisabled(int chan);

extern ChatList GChatList;

#endif
