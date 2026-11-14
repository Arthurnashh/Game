#ifdef _MSC_VER
#pragma once
#endif

#ifndef INPUT_INTERFACE_DEF_H
#define INPUT_INTERFACE_DEF_H

#include "inputInterface.h"

/*!
\patch 1.12 Date 08/06/2001 by Jirka
- Added: configuration for zoom on right mouse button
*/
#ifndef DECL_ENUM_USER_ACTION
#define DECL_ENUM_USER_ACTION
DECL_ENUM(UserAction)
#endif
DEFINE_ENUM_BEG(UserAction)
  // user defined keys
  UAMoveForward,
  UAMoveBack,
  UATurnLeft,
  UATurnRight,
  UAMoveUp,
  UAMoveDown,
  UAMoveFastForward,
  UAMoveSlowForward,
  UAMoveLeft,
  UAMoveRight,
  UAEvasiveForward,
  UAEvasiveLeft,
  UAEvasiveRight,
  UAEvasiveBack,
  UAStand,
  UACrouch,
  UAProne,
  UALeanLeft,
  UALeanRight,
  UALeanLeftToggle,
  UALeanRightToggle,
  UAWalkRunToggle,
  UAWalkRunTemp,

  UAToggleWeapons,
  UAFire,
  UADefaultAction,
  UAReloadMagazine,
  UALockTargets,  // next / prev target - DIK_TAB
  UALockEmptyTargets,
  UALockTarget,   // lock target - RIGHT MOUSE BUTTON
  UARevealTarget,
  UATempRaiseWeapon,
  UAToggleRaiseWeapon,
  UAPrevAction,
  UANextAction,
  UAAction,
  UAActionContext,
  UAActionFeeLook,
  UAThrowitem,
  UAHeadlights,
#if _VBS3_TI // convoy trainer, indicators
  UAIndicateLeft,
  UAIndicateRight,
  UAHazardLights,
  UAInteractVeh,
  UAPersItems,
  UAQuickEnter,
  UACommanderOverride,
  UATI_Brightness_Inc,
  UATI_Brightness_Dec,
  UATI_Contrast_Inc,
  UATI_Contrast_Dec,
  UATI_AutoContrast_Toggle,
# if _VBS3_LASE_LEAD
    UALase,
# endif
#endif //_VBS3
  UANightVision,
  UAMiniMap,
  UAMiniMapToggle,
  UAShowMap,
  UAHideMap,
  UAHelp,
  UATimeInc,
  UATimeDec,
  UAOptics,
  UAOpticsMode,
  UAPersonView,
  UATacticalView,
  UALiteUnitInfoToggle,
  UAZoomIn,
  UAZoomInToggle,
  UAZoomOut,
  UAZoomOutToggle,
  UAZoomContIn, // continuous zoom
  UAZoomContOut,
  UAZeroingUp,
  UAZeroingDown,
  UALookAround,
  UALookAroundToggle,
  UALookLeftDown,
  UALookDown,
  UALookRightDown,
  UALookLeft,
  UALookCenter,
  UALookRight,
  UALookLeftUp,
  UALookUp,
  UALookRightUp,
  UALookLeftCont,
  UALookRightCont,
  UALookDownCont,
  UALookUpCont,

  UALookShiftLeftCont,
  UALookShiftRightCont,
  UALookShiftUpCont,
  UALookShiftDownCont,
  UALookShiftForwardCont,
  UALookShiftBackCont,
  UALookRollLeftCont,
  UALookRollRightCont,
  UALookShiftLeft,
  UALookShiftRight,
  UALookShiftUp,
  UALookShiftDown,
  UALookShiftForward,
  UALookShiftBack,
  UALookRollLeft,
  UALookRollRight,
  UALookShiftCenter,
  UAFreeHeadMove,

  UAPrevChannel,
  UANextChannel,
  UAChat,
  UAVoiceOverNet,
  UAPushToTalk,
  UAPushToTalkAll,
  UAPushToTalkSide,
  UAPushToTalkCommand,
  UAPushToTalkGroup,
  UAPushToTalkVehicle,
  UAPushToTalkDirect,

  UANetworkStats,
//#if _ENABLE_UA_NETWORK_PLAYERS
  UANetworkPlayers,
  UADSInterface,
//#endif
  UATurbo,
  UAVehicleTurbo,
  UASlow,

  UAHoldBreath,
  UASalute,
  UATaunt,
  UAClap,
  UAPoint,
  UAThumbsUp,
  UAThrow,
  UASitDown,
  UASurrender,
  UAGetOver,

  /*!
  \patch 1.47 Date 3/5/2002 by Ondra
  - New: Mouseless control scheme possible.
  Aiming may be assigned to joystick buttons or keyboard.
  */
  UAAimUp,
  UAAimDown,
  UAAimLeft,
  UAAimRight,

  UAAimHeadUp,
  UAAimHeadDown,
  UAAimHeadLeft,
  UAAimHeadRight,

  /*!
  \patch 2.01 Date 1/23/2003 by Jirka
  - New: In-game pause is configurable action now
  */
  UAIngamePause,
  /*!
  \patch 2.01 Date 2/6/2003 by Jirka
  - New: In-game menu navigation
  */
/*
  UAMenuUp, = UAPrevAction
  UAMenuDown, = UANextAction
  */
  UAMenuSelect,
  UAMenuBack,
  // Helicopter control
  UAHeliUp,
  UAHeliDown,
  UAHeliLeft,
  UAHeliRight,
  UAAirBankLeft,
  UAAirBankRight,
  UAHeliRudderLeft,
  UAHeliRudderRight,
  UAHeliForward,
  UAHeliBack,
  UAHeliFastForward,
  UAAutoHover,
  UAAutoHoverCancel,
  UAHeliThrottlePos,
  UAHeliThrottleNeg,

  UAHeliCyclicForward,
  UAHeliCyclicBack,
  UAHeliCyclicLeft,
  UAHeliCyclicRight,
  UAHeliCollectiveRaise,
  UAHeliCollectiveLower,
  UAHeliCollectiveRaiseCont,
  UAHeliCollectiveLowerCont,

  UAHeliThrottleIncrease1,
  UAHeliThrottleDecrease1,
  UAHeliThrottleIncrease2,
  UAHeliThrottleDecrease2,
  UAHeliThrottleIncrease3,
  UAHeliThrottleDecrease3,
  UAHeliStarterToggle1,
  UAHeliStarterToggle2,
  UAHeliStarterToggle3,
  UAHeliBatteriesToggle,
  UAHeliRotorBrakeToggle,
  UAHeliEnginesToggle,
  UAHeliAPUToggle,
  UAHeliWheelsBrake,
  UAHeliTrimLeft,
  UAHeliTrimRight,
  UAHeliTrimForward,
  UAHeliTrimBackward,
  UAHeliTrimRudderLeft,
  UAHeliTrimRudderRight,

  // Seagull control
  UASeagullUp,
  UASeagullDown,
  UASeagullForward,
  UASeagullBack,
  UASeagullFastForward,

  // Car control
  UACarLeft,
  UACarRight,
  UACarWheelLeft,
  UACarWheelRight,
  UACarForward,
  UACarBack,
  UACarFastForward,
  UACarSlowForward,

  UACarAimUp,
  UACarAimDown,
  UACarAimLeft,
  UACarAimRight,

  // Vehicle commands
  UACommandLeft,
  UACommandRight,
  UACommandForward,
  UACommandBack,
  UACommandFast,
  UACommandSlow,

  UAVehLockTargets, // next / prev target in vehicles
  UAVehLockEmptyTargets,

  UASwapGunner,
  UAHeliManualFire,
  UATurnIn,
  UATurnOut,

  UACancelAction,

  UACommandWatch,

  UAGear,
  UAGetOut,
  UAEject,
  UALandGear,
  UALandGearUp,
  UAFlapsDown,
  UAFlapsUp,
  UAHelicopterTrimOn,
  UAHelicopterTrimOff,
  /*!
  \patch_internal 1.79 Date 7/25/2002 by Jirka
  - Added: cheat keys are configurable
  */
#if _ENABLE_CHEATS
  UACheat1,
  UACheat2,
#endif
  // Buldozer actions
  UABuldSwitchCamera,
  UABuldFreeLook,
  UABuldSelect,
  UABuldResetCamera,
  UABuldMagnetizePoints,
  UABuldMagnetizePlanes,
  UABuldMagnetizeYFixed,
  UABuldTerrainRaise1m,
  UABuldTerrainRaise10cm,
  UABuldTerrainLower1m,
  UABuldTerrainLower10cm,
  UABuldTerrainRaise5m,
  UABuldTerrainRaise50cm,
  UABuldTerrainLower5m,
  UABuldTerrainLower50cm,
  UABuldTerrainShowNode,
  UABuldSelectionType,
  UABuldLeft,
  UABuldRight,
  UABuldForward,
  UABuldBack,
  UABuldMoveLeft,
  UABuldMoveRight,
  UABuldMoveForward,
  UABuldMoveBack,
  UABuldTurbo,
  UABuldUp,
  UABuldDown,
  UABuldLookLeft,
  UABuldLookRight,
  UABuldLookUp,
  UABuldLookDown,
  UABuldZoomIn,
  UABuldZoomOut,
  UABuldTextureInfo,
#if _ENABLE_IDENTITIES
  UADiary,
#endif
  UAUser1,
  UAUser2,
  UAUser3,
  UAUser4,
  UAUser5,
  UAUser6,
  UAUser7,
  UAUser8,
  UAUser9,
  UAUser10,
  UAUser11,
  UAUser12,
  UAUser13,
  UAUser14,
  UAUser15,
  UAUser16,
  UAUser17,
  UAUser18,
  UAUser19,
  UAUser20,

  // item shortcuts
  UAItem0,
  UAItemFirst = UAItem0,
  UAItem1,
  UAItem2,
  UAItem3,
  UAItem4,
  UAItem5,
  UAItem6,
  UAItem7,
  UAItem8,
  UAItem9,
  UAItemLast = UAItem9,
  UAItemsShow,

  UAAltTab,

  UAN // terminator
DEFINE_ENUM_END(UserAction)

#endif


