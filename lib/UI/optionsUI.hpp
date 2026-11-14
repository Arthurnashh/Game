#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OPTIONS_UI_HPP
#define _OPTIONS_UI_HPP

#include "../types.hpp"
#include <El/Math/math3d.hpp>
#include <Es/Strings/rString.hpp>
#include <El/ParamArchive/serializeClass.hpp>

//! Basic interface for displays hierarchy
class AbstractOptionsUI //: public SerializeClass
{
	public:
	//! Draws displays hierarchy
	/*!
		Draw overlay - called before FinishDraw.
		\param vehicle vehicle with camera on
		\param alpha transparency
	*/
	virtual void DrawHUD(EntityAI *vehicle, float alpha)=0;

	//! Single simulation step of displays hierarchy
	/*!
		Simulate controls - called during simulation.
		\param vehicle vehicle with camera on
	*/
	virtual void SimulateHUD(EntityAI *vehicle)=0;

	//! called when UI is hidden. Introduced (and called) for map UI.
	virtual void OnHide(){}
	//! destructor
	virtual ~AbstractOptionsUI() {}

	//! Destroys display hierarchy
	/*!
		In most cases removes reference from GWorld.
		\param exit exit code
	*/
	virtual void DestroyHUD(int exit) {}
	//! Initialize display
	virtual void ResetHUD() {};
  //! Initialize display
  virtual void InitHUD() {};

	//! called by framework if key is pressed (see Windows SDK)
	virtual bool DoKeyDown(int dikCode) {return false;}
	//! called by framework if key is depressed (see Windows SDK)
	virtual bool DoKeyUp(int dikCode) {return false;}
	//! called by framework if char is entered (see Windows SDK)
	virtual bool DoChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {return false;}
	//! called by framework if IME char is entered (see Windows SDK)
	virtual bool DoIMEChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) {return false;}
	//! called by framework if IME composition is entered (see Windows SDK)
	virtual bool DoIMEComposition( unsigned nChar, unsigned nFlags ) {return false;}

	//! called by world simulation when addon is used that has not been registered
	virtual bool DoUnregisteredAddonUsed( RString addon ) {return false;}

  /// return the unique type identifier
  virtual int GetIDD() const {return -1;}

	//! returns if display is on top of displays hierarchy
	virtual bool IsTopmost() const {return true;}
  //! Return ID of topmost display in hierarchy
  virtual int GetTopID() const {return -1;}
  //! returns if game simulation is enabled
	virtual bool IsSimulationEnabled() const {return true;}
	//! returns if game display is enabled
	virtual bool IsDisplayEnabled() const {return true;}
  //! returns if game is already initialized
  virtual bool IsGameInitialized() const {return true;}
	//! returns if InGameUI (HUD) is enabled
	virtual bool IsUIEnabled() const {return true;}
  //! returns if display is ready for drawing
  virtual bool IsDisplayReady() const {return true;}
  //! serialize anything that needs to be persistant across sessions
	virtual LSError Serialize(ParamArchive &ar);
  //@{ map related interface
  /// return the target cursor is pointing at
  virtual EntityAI *GetTarget() const {return NULL;}
  /// retrieve the position cursor is pointing at (returns true when succeeded)
  virtual bool GetPosition(Vector3 &pos) const {return false;}
  //@}

#if _ENABLE_CHEATS
  /// the unit we are debugging in the map
  virtual Object *GetObserver() const {return NULL;}
#endif
	//virtual LSError Serialize(ParamArchive &ar);
};

AbstractOptionsUI *CreateMainOptionsUI();
AbstractOptionsUI *CreateDSOptionsUI();
AbstractOptionsUI *CreateTurnOptionsUI();
AbstractOptionsUI *CreateEndOptionsUI(int mode);
AbstractOptionsUI *CreateChatUI();
AbstractOptionsUI *CreateVoiceChatUI();
AbstractOptionsUI *CreatePlayerKilledUI();

AbstractOptionsUI *CreatePrepareTurnUI();
AbstractOptionsUI *CreateMainMapUI();

class IControlObject
{
public:
  virtual ~IControlObject() {}
  virtual void Draw() = 0;
};

#endif
