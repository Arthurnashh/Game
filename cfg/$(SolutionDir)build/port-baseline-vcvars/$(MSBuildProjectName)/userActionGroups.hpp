class UserActionsConflictGroups
{
  // helper groups to define collision groups
  class ActionGroups {
    basic[] = {
      ToggleWeapons,
      DefaultAction,
      ReloadMagazine,
      LockTarget,   // lock target - RIGHT MOUSE BUTTON
      RevealTarget,
      PrevAction,
      NextAction,
      Action,
      ActionContext,
      ActionFreeLook,
      Throwitem,
      Headlights,
      NightVision,
      MiniMap,
      MiniMapToggle,
      ShowMap,
      HideMap,
      Help,
      TimeInc,
      TimeDec,
      Optics,
      PersonView,
      TacticalView,
      LiteUnitInfoToggle,
      ZoomIn,
      ZoomInToggle,
      ZoomOut,
      zoomOutToggle,
      ZoomContIn, // continuous zoom
      ZoomContOut,
      ZeroingUp,
      ZeroingDown,
      LookAround,
      LookAroundToggle,
      LookLeftDown,
      LookDown,
      LookRightDown,
      LookLeft,
      LookCenter,
      LookRight,
      LookLeftUp,
      LookUp,
      LookRightUp,
      LookLeftCont,
      LookRightCont,
      LookDownCont,
      LookUpCont,
      PrevChannel,
      NextChannel,
      Chat,
      VoiceOverNet,
      PushToTalk,
      PushToTalkAll,
      PushToTalkSide,
      PushToTalkCommand,
      PushToTalkGroup,
      PushToTalkVehicle,
      PushToTalkDirect,
      NetworkStats,
      Turbo,
      Slow,
      HoldBreath,
      IngamePause,
      MenuSelect,
      SwapGunner,
      CancelAction,
      CommandWatch,
      Gear,
      Cheat1,
      Cheat2,
      Diary,
      Surrender,
      GetOver,
      Item0,
      Item1,
      Item2,
      Item3,
      Item4,
      Item5,
      Item6,
      Item7,
      Item8,
      Item9,
      ItemsShow
    };
    /*!
      \patch 5099 Date 12/8/2006 by Ondra
      - Fixed: False conflict reported for Salute and SitDown actions for vehicles.
    */
    ManBasic[] = {
      TempRaiseWeapon,
      ToggleRaiseWeapon,
      Salute,
      SitDown,
	  Taunt,
	  Clap,
	  Point,
	  ThumbsUp,
    Throw,
      LockTargets,  // next / prev target - DIK_TAB
      LockEmptyTargets
    };
    VehBasic[] = {
      Fire,
      VehicleTurbo,
      HeliManualFire,
      VehLockTargets, // next / prev target in vehicles
      VehLockEmptyTargets,
      GetOut,
      Eject
    };
    ManMove[] = {
      AimUp,
      AimDown,
      AimLeft,
      AimRight,
      MoveForward,
      MoveBack,
      TurnLeft,
      TurnRight,
      MoveUp,
      MoveDown,
      MoveFastForward,
      MoveSlowForward,
      MoveLeft,
      MoveRight,
      EvasiveForward,
      EvasiveLeft,
      EvasiveRight,
      EvasiveBack,
      Stand,
      Crouch,
      Prone,
      LeanLeft,
      LeanRight,
      LeanLeftToggle,
      LeanRightToggle,
      WalkRunToggle,
      WalkRunTemp
    };
    AimingHead[] = {
      AimHeadUp,
      AimHeadDown,
      AimHeadLeft,
      AimHeadRight
    };
    AirCraft[] = {
      HeliUp,       // Helicopter control
      HeliDown,
      HeliThrottlePos,
      HeliThrottleNeg,
      HeliLeft,
      HeliRight,
      AirBankLeft,
      AirBankRight,
      HeliRudderLeft,
      HeliRudderRight,
      HeliForward,
      HeliBack,
      HeliFastForward,
      AutoHover,
      AutoHoverCancel,
      LandGear,
      LandGearUp,
      FlapsDown,
      FlapsUp

    };
    Heli[] = {
      HeliCollectiveRaise,       // Helicopter control
      HeliCollectiveLower,
      HeliCollectiveRaiseCont,
      HeliCollectiveLowerCont,
      HeliLeft,
      HeliRight,
      HeliCyclicLeft,
      HeliCyclicRight,
      HeliRudderLeft,
      HeliRudderRight,
      CyclicForward,
      CyclicBack,
      HeliFastForward,
      AutoHover,
      AutoHoverCancel,
      LandGear,
      LandGearUp,
      FlapsDown,
      FlapsUp,
      HelicopterTrimOn,
      HelicopterTrimOff,

      HeliThrottleIncrease1,
      HeliThrottleDecrease1,
      HeliThrottleIncrease2,
      HeliThrottleDecrease2,
      HeliThrottleIncrease3,
      HeliThrottleDecrease3,
      HeliStarterToggle1,
      HeliStarterToggle2,
      HeliStarterToggle3,
      HeliBatteriesToggle,
      HeliRotorBrakeToggle,
      HeliEnginesToggle,
      HeliAPUToggle,
      HeliWheelsBrake,
      HeliTrimLeft,
      HeliTrimRight,
      HeliTrimForward,
      HeliTrimBackward,
      HeliTrimRudderLeft,
      HeliTrimRudderRight
    };
    CarMovement[] = {
      CarLeft, // Car control
      CarRight,
      CarWheelLeft,
      CarWheelRight,
      CarForward,
      CarBack,
      CarFastForward,
      CarSlowForward
    };
    CarFreelook[] = {
      CarAimUp,
      CarAimDown,
      CarAimLeft,
      CarAimRight
    };
    Tank[] = {
      TurnIn,
      TurnOut,
    };
    SeaGull[] = { // Seagull control
      SeagullUp,
      SeagullDown,
      SeagullForward,
      SeagullBack,
      SeagullFastForward
    };
    FreeHeadMove[] = {
      FreeHeadMove
    };
    HeadMove[] = {
      LookShiftLeftCont,
      LookShiftRightCont,
      LookShiftUpCont,
      LookShiftDownCont,
      LookShiftForwardCont,
      LookShiftBackCont,
      LookRollLeftCont,
      LookRollRightCont,
      LookShiftLeft,
      LookShiftRight,
      LookShiftUp,
      LookShiftDown,
      LookShiftForward,
      LookRollLeft,
      LookRollRight,
      LookShiftBack,
      LookShiftCenter      
    };
    Command[] = {
      CommandLeft, // Vehicle commands
      CommandRight,
      CommandForward,
      CommandBack,
      CommandFast,
      CommandSlow
    };
    buldozer[] = {
      BuldSwitchCamera,
      BuldFreeLook,
      BuldSelect,
      BuldResetCamera,
      BuldMagnetizePoints,
      BuldMagnetizePlanes,
      BuldMagnetizeYFixed,
      BuldTerrainRaise1m,
      buldTerrainRaise10cm,
      BuldTerrainLower1m,
      BuldTerrainLower10cm,
      BuldTerrainRaise5m,
      BuldTerrainRaise50cm,
      BuldTerrainLower5m,
      BuldTerrainLower50cm,
      BuldTerrainShowNode,
      BuldSelectionType,
      BuldLeft,
      BuldRight,
      BuldForward,
      BuldBack,
      BuldMoveLeft,
      BuldMoveRight,
      BuldMoveForward,
      BuldMoveBack,
      BuldTurbo,
      BuldUp,
      BuldDown,
      BuldLookLeft,
      BuldLookRight,
      BuldLookUp,
      BuldLookDown,
      BuldZoomIn,
      BuldZoomOut,
      BuldTextureInfo
    };
  };
  // CollisionGroups uses the actionGroups within array definitions
  // actions within one collisionGroup collides
  class CollisionGroups {
    manMain[] = {basic, manBasic};
    vehMain[] = {basic, vehBasic};
    manMove[] = {manMove, manBasic, basic};
    commander[] = {command, vehBasic, basic};
    aimingHead[] = {aimingHead, manBasic, basic};

    heli[] = {heli, vehBasic, basic, HeadMove}; //we do not want FreeHeadMove to collide with HeadMove actions (standalone key ctrl vs. ctrl+xxx combo actions)
    aircraft[] = {aircraft, vehBasic, basic, HeadMove}; //we do not want FreeHeadMove to collide with HeadMove actions (standalone key ctrl vs. ctrl+xxx combo actions)
    carMove[] = {CarMovement, vehBasic, basic, HeadMove};
    carLook[] = {CarFreelook, vehBasic, basic, HeadMove};
    TankMove[] = {CarMovement, Tank, vehBasic, basic, HeadMove};
    TankLook[] = {CarFreelook, Tank, vehBasic, basic, HeadMove};

    heliF[] = {heli, vehBasic, basic, FreeHeadMove};
    aircraftF[] = {aircraft, vehBasic, basic, FreeHeadMove};
    carMoveF[] = {CarMovement, vehBasic, basic, FreeHeadMove};
    carLookF[] = {CarFreelook, vehBasic, basic, FreeHeadMove};
    TankMoveF[] = {CarMovement, Tank, vehBasic, basic, FreeHeadMove};
    TankLookF[] = {CarFreelook, Tank, vehBasic, basic, FreeHeadMove};

    seagull[] = {seagull};
    buldozer[] = {buldozer};
  };
  // Exceptions groups are prioritized: actions within the same group cannot collide
  class Exceptions {
    leftAlt[] = {lookAround, commandWatch};
    map[] = {showMap, hideMap};
    lockRevealTarget[] = {lockTarget, revealTarget, holdBreath};
    rollLeft[] = {EvasiveLeft, LeanLeft};
    rollRight[] = {EvasiveRight, LeanRight};
    landGear[] = {LandGear, LandGearUp};
    zoomIn[] = {zoomIn, zoomInToggle};
    zoomOut[] = {zoomOut, zoomOutToggle};
    mouseConfirm[] = {DefaultAction, ActionContext, ActionFreeLook};
    moveUp[] = {zeroingUp, heliUp, seagullUp };
    moveDown[] = {zeroingDown, heliDown, seagullDown };
    autoHover[] = {AutoHover, AutoHoverCancel};
  };
};

class UserActionGroups
{
  class All
  {
    name = $STR_USRACT_GROUP_ALL;
    group[] = {
      MoveForward,
      MoveBack,
      TurnLeft,
      TurnRight,
      MoveUp,
      MoveDown,
      MoveFastForward,
      MoveSlowForward,
      MoveLeft,
      MoveRight,
      EvasiveForward,
      EvasiveLeft,
      EvasiveRight,
      Stand,
      Crouch,
      Prone,
      LeanLeft,
      LeanRight,
      LeanLeftToggle,
      LeanRightToggle,
      WalkRunToggle,
      WalkRunTemp,

      ToggleWeapons,
      Fire,
      DefaultAction,
      ReloadMagazine,
      opticsMode,
      LockTargets,  // next / prev target - DIK_TAB
      LockEmptyTargets,
      LockTarget,   // lock target - RIGHT MOUSE BUTTON
      RevealTarget,
      TempRaiseWeapon,
      ToggleRaiseWeapon,
      PrevAction,
      NextAction,
      Action,
      ActionContext,
      ActionFreeLook,
      Throwitem,
      Headlights,
      NightVision,
      MiniMap,
      MiniMapToggle,
      ShowMap,
      HideMap,
      Help,
      TimeInc,
      TimeDec,
      Optics,
      PersonView,
      TacticalView,
      LiteUnitInfoToggle,
      ZoomIn,
      ZoomInToggle,
      ZoomOut,
      zoomOutToggle,
      ZoomContIn, // continuous zoom
      ZoomContOut,
      ZeroingUp,
      ZeroingDown,
      LookAround,
      LookAroundToggle,
      LookLeftDown,
      LookDown,
      LookRightDown,
      LookLeft,
      LookCenter,
      LookRight,
      LookLeftUp,
      LookUp,
      LookRightUp,
      LookLeftCont,
      LookRightCont,
      LookDownCont,
      LookUpCont,

      LookShiftLeftCont,
      LookShiftRightCont,
      LookShiftUpCont,
      LookShiftDownCont,
      LookShiftForwardCont,
      LookShiftBackCont,
      LookRollLeftCont,
      LookRollRightCont,
      LookShiftLeft,
      LookShiftRight,
      LookShiftUp,
      LookShiftDown,
      LookShiftForward,
      LookShiftBack,
      LookRollLeft,
      LookRollRight,
      LookShiftCenter,
      FreeHeadMove,

      PrevChannel,
      NextChannel,
      Chat,
      VoiceOverNet,
      PushToTalk,
      PushToTalkAll,
      PushToTalkSide,
      PushToTalkCommand,
      PushToTalkGroup,
      PushToTalkVehicle,
      PushToTalkDirect,
      NetworkStats,
      Turbo,
      VehicleTurbo,
      Slow,

      HoldBreath,
      Salute,
      SitDown,	  
	  Taunt,
	  Clap,
	  Point,
	  ThumbsUp,
    Throw,

      AimUp,
      AimDown,
      AimLeft,
      AimRight,

      AimHeadUp,
      AimHeadDown,
      AimHeadLeft,
      AimHeadRight,

      IngamePause,

      MenuSelect,
      MenuBack,
      // Helicopter control
      HeliThrottlePos,
      HeliThrottleNeg,
      HeliUp,
      HeliDown,
      HeliLeft,
      HeliRight,
      AirBankLeft,
      AirBankRight,
      HeliRudderLeft,
      HeliRudderRight,
      HeliForward,
      HeliBack,
      HeliFastForward,

      HeliCollectiveRaise,
      HeliCollectiveLower,
      HeliCollectiveRaiseCont,
      HeliCollectiveLowerCont,
      HeliCyclicLeft,
      HeliCyclicRight,
      CyclicForward,
      CyclicBack,

      AutoHover,
      AutoHoverCancel,
      LandGear,
      LandGearUp,
      FlapsDown,
      FlapsUp,
      HelicopterTrimOn,
      HelicopterTrimOff,

      HeliThrottleIncrease1,
      HeliThrottleDecrease1,
      HeliThrottleIncrease2,
      HeliThrottleDecrease2,
      HeliThrottleIncrease3,
      HeliThrottleDecrease3,
      HeliStarterToggle1,
      HeliStarterToggle2,
      HeliStarterToggle3,
      HeliBatteriesToggle,
      HeliRotorBrakeToggle,
      HeliEnginesToggle,
      HeliAPUToggle,
      heliWheelsBrake,
      HeliTrimLeft,
      HeliTrimRight,
      HeliTrimForward,
      HeliTrimBackward,
      HeliTrimRudderLeft,
      HeliTrimRudderRight,
      // Seagull control
      SeagullUp,
      SeagullDown,
      SeagullForward,
      SeagullBack,
      SeagullFastForward,

      // Car control
      CarLeft,
      CarRight,
      CarWheelLeft,
      CarWheelRight,
      CarForward,
      CarBack,
      CarFastForward,
      CarSlowForward,
/*
      IndicateLeft,
      IndicateRight,
      IndicateHazard,
*/
      CarAimUp,
      CarAimDown,
      CarAimLeft,
      CarAimRight,

      // Vehicle commands
      CommandLeft,
      CommandRight,
      CommandForward,
      CommandBack,
      CommandFast,
      CommandSlow,

      VehLockTargets, // next / prev target in vehicles
      VehLockEmptyTargets,

      SwapGunner,
      HeliManualFire,
      TurnIn,
      TurnOut,
/*  VBS_3_TI
      InteractVeh,
      PersItems,
      QuickEnter,
*/
      GetOut,
      Eject,
/*  VBS_3_TI
      CommanderOverride,

      TI_Brightness_Inc,
      TI_Brightness_Dec,
      TI_Contrast_Inc,
      TI_Contrast_Dec,
      TI_AutoContrast_Toggle,
*/
      CancelAction,

      CommandWatch,

      Gear,

      Cheat1,
      Cheat2,

      Diary,
      DSInterface,

      Surrender,
      GetOver,

      Item0,
      Item1,
      Item2,
      Item3,
      Item4,
      Item5,
      Item6,
      Item7,
      Item8,
      Item9,
      ItemsShow
    };
  };
  class BasicInfantryControls
  {
    name = $STR_USRACT_GROUP_INFANTRY;
    group[] = {
        MoveForward,
        MoveBack,
        TurnLeft,
        TurnRight,
        MoveFastForward,
        MoveSlowForward,
        MoveLeft,
        MoveRight,
        EvasiveForward,
        EvasiveLeft,
        EvasiveRight,
        Stand,
        Crouch,
        Prone,
        LeanLeft,
        LeanRight,
        LeanLeftToggle,
        LeanRightToggle,
        WalkRunToggle,
        WalkRunTemp,

        ToggleWeapons,
        DefaultAction,
        ReloadMagazine,

        LockTargets,  // next / prev target - DIK_TAB
        LockEmptyTargets,
        LockTarget,   // lock target - RIGHT MOUSE BUTTON
        RevealTarget,
        TempRaiseWeapon,
        ToggleRaiseWeapon,
        PrevAction,
        NextAction,
        MenuBack,
        Action,
        ActionContext,
        ActionFreeLook,

        Optics,
        LookAround,
        LookAroundToggle,

        Gear,
        MiniMap,
        MiniMapToggle,

        Diary,

        Throwitem,

        Headlights,
        NightVision,
        ShowMap,
        HideMap,

        PersonView,
        TacticalView,
        LiteUnitInfoToggle,

        ZoomIn,
        ZoomInToggle,
        ZoomOut,
        zoomOutToggle,
        ZeroingUp,
        ZeroingDown,

        HoldBreath,
        Salute,
        SitDown,
        Surrender,
		Taunt,
		Clap,
		Point,
		ThumbsUp,
    Throw,
        GetOver,

        VoiceOverNet,
        PrevChannel,
        NextChannel,
        Chat,
        PushToTalk,
        PushToTalkAll,
        PushToTalkSide,
        PushToTalkCommand,
        PushToTalkGroup,
        PushToTalkVehicle,
        PushToTalkDirect,

        AimUp,
        AimDown,
        AimLeft,
        AimRight,

        AimHeadUp,
        AimHeadDown,
        AimHeadLeft,
        AimHeadRight,

        LookLeftDown,
        LookDown,
        LookRightDown,
        LookLeft,
        LookCenter,
        LookRight,
        LookLeftUp,
        LookUp,
        LookRightUp,
        LookLeftCont,
        LookRightCont,
        LookDownCont,
        LookUpCont
    };
  };
  class Car
  {
    name = $STR_USRACT_GROUP_VEHICLE;
    group[] = {
        // Car control
        CarLeft,
        CarRight,
        CarWheelLeft,
        CarWheelRight,
        CarForward,
        CarBack,
        CarFastForward,
        CarSlowForward,
/*
        IndicateLeft,
        IndicateRight,
        IndicateHazard,
*/
        CarAimUp,
        CarAimDown,
        CarAimLeft,
        CarAimRight,

        TurnIn,
        TurnOut,
/* VBS_3_TI
        InteractVeh,
        PersItems,
        QuickEnter,
        CommanderOverride,
*/
        GetOut,
        Eject,

        Fire
    };
  };
  class Air
  {
    name = $STR_USRACT_GROUP_FLYING;
    group[] = {
      // Helicopter control
      HeliUp,
      HeliDown,
      HeliThrottlePos,
      HeliThrottleNeg,
      HeliLeft,
      HeliRight,
      AirBankLeft,
      AirBankRight,
      HeliRudderLeft,
      HeliRudderRight,
      HeliForward,
      HeliBack,
      HeliFastForward,
      HeliManualFire,
      AutoHover,
      AutoHoverCancel,
      LandGear,
      LandGearUp,
      FlapsDown,
      FlapsUp,

/* VBS_3_TI
      InteractVeh,
      PersItems,
      QuickEnter,
      CommanderOverride,
*/
      GetOut,
      Eject,
      // Seagull control
      SeagullUp,
      SeagullDown,
      SeagullForward,
      SeagullBack,
      SeagullFastForward,

      Fire,

      LookShiftLeftCont,
      LookShiftRightCont,
      LookShiftUpCont,
      LookShiftDownCont,
      LookShiftForwardCont,
      LookShiftBackCont,
      LookRollLeftCont,
      LookRollRightCont,
      LookShiftLeft,
      LookShiftRight,
      LookShiftUp,
      LookShiftDown,
      LookShiftForward,
      LookShiftBack,
      LookRollLeft,
      LookRollRight,
      LookShiftCenter,
      FreeHeadMove
    };
  };

  //Adding a dedicated group for helicopters
  class Helicopter
  {
    name = $STR_USRACT_GROUP_HELI;
    group[] = 
    {
      HeliCyclicForward,
      HeliCyclicBack,
      HeliCyclicLeft,
      HeliCyclicRight,
      HeliCollectiveRaiseCont,
      HeliCollectiveLowerCont,
      HeliCollectiveRaise,
      HeliCollectiveLower,
      HeliRudderLeft,
      HeliRudderRight,
      HeliLeft,
      HeliRight,
      //HeliFastForward, ? Not relevant for TKOH?
      AutoHover,
      AutoHoverCancel,

      HeliThrottleIncrease1,
      HeliThrottleDecrease1,
      HeliThrottleIncrease2,
      HeliThrottleDecrease2,
      HeliThrottleIncrease3,
      HeliThrottleDecrease3,
      HeliStarterToggle1,
      HeliStarterToggle2,
      HeliStarterToggle3,
      HeliBatteriesToggle,
      HeliRotorBrakeToggle,
      HeliEnginesToggle,
      HeliAPUToggle,
      heliWheelsBrake,
      LandGear,
      LandGearUp,
      HelicopterTrimOn,
      HelicopterTrimOff,
      HeliTrimLeft,
      HeliTrimRight,
      HeliTrimForward,
      HeliTrimBackward,
      HeliTrimRudderLeft,
      HeliTrimRudderRight,

      minimap,
      minimapToggle,
      
      GetOut,
      Eject,

      LockTarget, 
      VehLockTargets,
      VehLockEmptyTargets,
      DefaultAction,
      Fire,

      FreeHeadMove,
      LookShiftLeftCont,
      LookShiftRightCont,
      LookShiftUpCont,
      LookShiftDownCont,
      LookShiftForwardCont,
      LookShiftBackCont,
      LookRollLeftCont,
      LookRollRightCont,
      LookShiftLeft,
      LookShiftRight,
      LookShiftUp,
      LookShiftDown,
      LookShiftForward,
      LookShiftBack,
      LookRollLeft,
      LookRollRight,
      LookShiftCenter
    };
  };

  class Buldozer
  {
    name = $STR_USRACT_GROUP_BULDOZER;
    group[] = {
      // Buldozer actions
      BuldSwitchCamera,
      BuldFreeLook,
      BuldSelect,
      BuldResetCamera,
      BuldMagnetizePoints,
      BuldMagnetizePlanes,
      BuldMagnetizeYFixed,
      BuldTerrainRaise1m,
      BuldTerrainRaise10cm,
      BuldTerrainLower1m,
      BuldTerrainLower10cm,
      BuldTerrainRaise5m,
      BuldTerrainRaise50cm,
      BuldTerrainLower5m,
      BuldTerrainLower50cm,
      BuldTerrainShowNode,
      BuldSelectionType,
      BuldLeft,
      BuldRight,
      BuldForward,
      BuldBack,
      BuldMoveLeft,
      BuldMoveRight,
      BuldMoveForward,
      BuldMoveBack,
      BuldTurbo,
      BuldUp,
      BuldDown,
      BuldLookLeft,
      BuldLookRight,
      BuldLookUp,
      BuldLookDown,
      BuldZoomIn,
      BuldZoomOut,
      BuldTextureInfo
    };
  };
  class UserActions
  {
    name = $STR_USRACT_GROUP_USER;
    group[] = {
      // Specific User actions used in addons
      User1,
      User2,
      User3,
      User4,
      User5,
      User6,
      User7,
      User8,
      User9,
      User10,
      User11,
      User12,
      User13,
      User14,
      User15,
      User16,
      User17,
      User18,
      User19,
      User20
    };
  };
};
