package com.bistudio.JNIScripting;

public class RVEngine
{
  /** 
   * Namespace - set of variables.
   */
  public static class GameNamespace extends NativeObject {}
  /** 
   * Part of code (compiled).
   */
  public static class GameCode extends NativeObject {}
  /** 
   * A game object (like a soldier, vehicle or building).
   */
  public static class GameObject extends NativeObject {}
  /** 
   * 
   */
  public static class GameVector extends NativeObject {}
  /** 
   * 
   */
  public static class GameTrans extends NativeObject {}
  /** 
   * 
   */
  public static class GameOrient extends NativeObject {}
  /** 
   * The name of the side (see <f>west</f>, <f>east</f>, <f>civilian</f> and <f>resistance</f>).
   */
  public static class GameSide extends NativeObject {}
  /** 
   * A group.
   */
  public static class GameGroup extends NativeObject {}
  /** 
   * 
   */
  public static class GameText extends NativeObject {}
  /** 
   * 
   */
  public static class GameScript extends NativeObject {}
  /** 
   * 
   */
  public static class GameTarget extends NativeObject {}
  /** 
   * 
   */
  public static class GameJavaClass extends NativeObject {}
  /** 
   * Config file entry.
   */
  public static class GameConfig extends NativeObject {}
  /** 
   * Display UI object.
   */
  public static class GameDisplay extends NativeObject {}
  /** 
   * Control UI object.
   */
  public static class GameControl extends NativeObject {}
  /** 
   * Team member (agent or team).
   */
  public static class GameTeamMember extends NativeObject {}
  /** 
   * Task.
   */
  public static class GameTask extends NativeObject {}
  /** 
   * Diary record.
   */
  public static class GameDiaryRecord extends NativeObject {}
  /** 
   * Location.
   */
  public static class GameLocation extends NativeObject {}

  /** 
   * Attaches a static object to the given waypoint.
<br/><br/>
   * <b>Example:</b> [grp, 2] waypointAttachObject 1234<br/>
   * @param oper1 waypoint
   * @param oper2 idStatic or object
   * @since 1.86
   */
  public native static void waypointAttachObject(java.util.List oper1, Object oper2);
  /** 
   * Set if leader can issue attack commands.
   * @param oper1 group
   * @param oper2 enable
   * @since 2.92
   */
  public native static void enableAttack(Object oper1, boolean oper2);
  /** 
   * Sets H5 bold font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH5B "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH5B(GameControl oper1, String oper2);
  /** 
   * Chek if given area is flat and emty.
<br/><br/>
   * <b>Example:</b> newposition = position isFlatEmpty(10, 0, 0.5, 5, 0, false, player)<br/>
   * @param oper1 position
   * @param oper2 [float minDistance,float precizePos,float maxGradient,float gradientRadius,float onWater,bool onShore,object skipobj]
   * @since 5501
   */
  public native static java.util.List isFlatEmpty(java.util.List oper1, java.util.List oper2);
  /** 
   * Sets text color of given control. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control ctrlSetTextColor [1, 0, 0, 1]<br/>
   * @param oper1 display
   * @param oper2 color
   * @since 2.50
   */
  public native static void ctrlSetTextColor(GameControl oper1, java.util.List oper2);
  /** 
   * Removes the action with the given id.
<br/><br/>
   * <b>Example:</b> player removeAction 0<br/>
   * @param oper1 unit
   * @param oper2 index
   * @since 1.11
   */
  public native static void removeAction(GameObject oper1, float oper2);
  /** 
   * Check target database of the unit for all targets in max. distance radius around it. The output is list of items: [position, type, side, subj. cost, object]
   * @param oper1 unit
   * @param oper2 radius
   */
  public native static java.util.List nearTargets(GameObject oper1, float oper2);
  /** 
   * Sets the marker size. Size is in format [a-axis, b-axis]. The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerSizeLocal [100, 200]<br/>
   * @param oper1 marker
   * @param oper2 size
   * @since 2.92
   */
  public native static void setMarkerSizeLocal(String oper1, java.util.List oper2);
  /** 
   * Prepares the camera field of view (zoom). See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareFov 0.1<br/>
   * @param oper1 camera
   * @param oper2 fieldOfView
   * @since 2.95
   */
  public native static void camPrepareFov(GameObject oper1, float oper2);
  /** 
   * Removes the item with the given index from the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _control lbDelete 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 5501
   */
  public native static void lnbDeleteColumn(GameControl oper1, float oper2);
  /** 
   * Set a new size (width, height) of given location.
   * @param oper1 location
   * @param oper2 [sizeX, sizeZ]
   * @since 2.90
   */
  public native static void setSize(GameLocation oper1, java.util.List oper2);
  /** 
   * Sets a unit's mimic. The following values are recognized: "Default", "Normal", "Smile", "Hurt", "Ironic", "Sad", "Cynic", "Surprised", "Agresive" and "Angry".
<br/><br/>
   * <b>Example:</b> setMimic "Angry"<br/>
   * @param oper1 person
   * @param oper2 mimic
   */
  public native static void setMimic(GameObject oper1, String oper2);
  /** 
   * Lock / unlock the unit to using conversation. Implemented by the counter, so lock - unlock need to be matched.
<br/><br/>
   * <b>Example:</b> player disableConversation true<br/>
   * @param oper1 unit
   * @param oper2 lock
   */
  public native static void disableConversation(GameObject oper1, boolean oper2);
  /** 
   * Set variable to given value in the variable space of given FSM. The FSM handle is the number returned by the execFSM command.
   * @param oper1 FSM handle
   * @param oper2 [name, value]
   * @since 5501
   */
  public native static void setFSMVariable(float oper1, java.util.List oper2);
  /** 
   * Set the structured text which will be displayed in structured text control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetStructuredText parseText "First line&lt;img image=data\isniper.paa/&gt;&ltbr/&gt;Second line"<br/>
   * @param oper1 control
   * @param oper2 structured text
   * @since 2.53
   */
  public native static void ctrlSetStructuredText(GameControl oper1, GameText oper2);
  /** 
   * Attach a object to given location.
   * @param oper1 location
   * @param oper2 object
   * @since 2.90
   */
  public native static void attachObject(GameLocation oper1, GameObject oper2);
  /** 
   * Causes a smooth change in the music volume. The change duration is given by time, the target volume by volume. The default music volume is 0.5.
<br/><br/>
   * <b>Example:</b> 5 fadeMusic 0<br/>
   * @param oper1 time
   * @param oper2 volume
   */
  public native static void fadeMusic(float oper1, float oper2);
  /** 
   * Pass a non-verbal communication to the receiver.
   * @param oper1 person
   * @param oper2 [receiver, topic, sentence id, [argument name, argument value, argument text, argument speech], ...]
   * @since 5500
   */
  public native static void kbReact(GameObject oper1, java.util.List oper2);
  /** 
   * Makes the unit peform an action. The format of action can be [type, target, param1, param2, param3]. Only type is required, target defaults to unit and param1, param2, param3 are type specific.
<br/><br/>
   * <b>Example:</b> soldierOne action ["eject", vehicle soldierOne]<br/>
   * @param oper1 unit
   * @param oper2 action
   */
  public native static void action(GameObject oper1, java.util.List oper2);
  /** 
   * Returns how much space is in backpack for given weapon or magazine.
<br/><br/>
   * <b>Example:</b> [weapons, magazines] = backpack backpackSpaceFor "m16"<br/>
   * @param oper1 backpack
   * @param oper2 weapon or magazine type
   * @since 5501
   */
  public native static java.util.List backpackSpaceFor(GameObject oper1, String oper2);
  /** 
   * The unit will fire from the given weapon.
<br/><br/>
   * <b>Example:</b> soldierOne fire "HandGrenade"<br/>
   * @param oper1 unit
   * @param oper2 weaponName
   */
  public native static void fire(GameObject oper1, String oper2);
  /** 
   * The unit will fire from the given weapon. The array has format [muzzle, mode, magazine] or [muzzle, mode].
<br/><br/>
   * <b>Example:</b> soldierOne fire ["throw","SmokeShell","SmokeShell"]<br/>
   * @param oper1 unit
   * @param oper2 array
   */
  public native static void fire(GameObject oper1, java.util.List oper2);
  /** 
   * Creates the new overlay dialog for the specified type of overlay.
   * @param oper1 map
   * @param oper2 config
   * @since 2.92
   */
  public native static void newOverlay(GameControl oper1, GameConfig oper2);
  /** 
   * Selects the item with the given index of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _control lbSetCurSel row<br/>
   * @param oper1 control
   * @param oper2 row
   * @since 5501
   */
  public native static void lnbSetCurSelRow(GameControl oper1, float oper2);
  /** 
   * Attach a target to the simple task. Overrides setSimpleTaskDestination.
<br/><br/>
   * <b>Example:</b> task setSimpleTaskTarget [vehicle, true]<br/>
   * @param oper1 task
   * @param oper2 [target,precise position]
   * @since 5500
   */
  public native static void setSimpleTaskTarget(GameTask oper1, java.util.List oper2);
  /** 
   * 
   */
  public native static void diag_enable(String oper1, boolean oper2);
  /** 
   * Executes a script. Argument is passed to the script as local variable _this. The script is first searched in the mission folder, then in the campaign scripts subfolder and finally in the global scripts folder.
<br/><br/>
   * <b>Example:</b> [player, jeepOne] exec "getIn.sqs"<br/>
   * @param oper1 argument
   * @param oper2 script
   */
  public native static void exec(Object oper1, String oper2);
  /** 
   * Disables switching to the next waypoint (the current waypoint will never complete while lockWp is used). This is sometimes used during cut-scenes.
<br/><br/>
   * <b>Example:</b> groupOne lockWP true<br/>
   * @param oper1 group
   * @param oper2 lockWP
   */
  public native static void lockWp(Object oper1, boolean oper2);
  /** 
   * Update particle source to create particles on circle with given radius. Velocity is transformed and added to total velocity.
   * @param oper1 particleSource
   * @param oper2 [radius, velocity]
   * @since 2.56
   */
  public native static void setParticleCircle(GameObject oper1, java.util.List oper2);
  /** 
   * Prepares the camera position relative to the current position of the currect target (see <f>camPrepareTarget</f>). See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareRelPos [10,5]<br/>
   * @param oper1 camera
   * @param oper2 position
   * @since 2.95
   */
  public native static void camPrepareRelPos(GameObject oper1, java.util.List oper2);
  /** 
   * Search for the position nearest (up to maxDistance) to the center, with the free area (vehicle of the given type can be placed anywhere) of the given radius. When not found, empty array is returned.
   * @param oper1 center
   * @param oper2 [radius, maxDistance] or [radius, maxDistance, vehicleType]
   * @since 5501
   */
  public native static java.util.List findEmptyPosition(java.util.List oper1, java.util.List oper2);
  /** 
   * Set position of 2d listbox columns.
<br/><br/>
   * <b>Example:</b> _control lbSetColumnsPos [pos1,pos2,...]<br/>
   * @param oper1 control
   * @param oper2 [pos1,pos2,...]
   * @since 5501
   */
  public native static void lnbSetColumnsPos(GameControl oper1, java.util.List oper2);
  /** 
   * Returns the picture name of the item with the given position of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _picture = _control lnbPicture [row, column]<br/>
   * @param oper1 control
   * @param oper2 [row, column]
   * @since 5501
   */
  public native static String lnbPicture(GameControl oper1, java.util.List oper2);
  /** 
   * Return animation phase of animation on object.
<br/><br/>
   * <b>Example:</b> house animationPhase "doors1"<br/>
   * @param oper1 object
   * @param oper2 animation
   * @since 1.75
   */
  public native static float animationPhase(GameObject oper1, String oper2);
  /** 
   * Tells the group it owns the vehicle now.
   * @param oper1 group
   * @param oper2 vehicle
   * @since 5128
   */
  public native static void addVehicle(GameGroup oper1, GameObject oper2);
  /** 
   * Removes a previously added menu item.
   * @param oper1 map
   * @param oper2 index of menu item to delete
   * @since 2.92
   */
  public native static void removeMenuItem(GameControl oper1, float oper2);
  /** 
   * Removes a previously added menu item.
   * @param oper1 map
   * @param oper2 text of menu item to delete
   * @since 2.92
   */
  public native static void removeMenuItem(GameControl oper1, String oper2);
  /** 
   * Set object arguments in mission editor.
   * @param oper1 map
   * @param oper2 [object, [name1, value1, ...]]
   * @since 2.92
   */
  public native static Object setObjectArguments(GameControl oper1, java.util.List oper2);
  /** 
   * Return the value of variable in the given namespace.
   * @param oper1 namespace
   * @param oper2 name
   * @since 5501
   */
  public native static Object getVariable(GameNamespace oper1, String oper2);
  /** 
   * Return the value of variable in the given namespace. If not found, default value is returned.
   * @param oper1 namespace
   * @param oper2 [name, default value]
   * @since rev.80912
   */
  public native static Object getVariable(GameNamespace oper1, java.util.List oper2);
  /** 
   * Get variable from the variable space of given map.
   * @param oper1 map
   * @param oper2 name
   * @since 2.92
   */
  public native static void getVariable(GameControl oper1, String oper2);
  /** 
   * Return the value of variable in the variable space of given object.
   * @param oper1 object
   * @param oper2 name
   * @since 2.92
   */
  public native static Object getVariable(GameObject oper1, String oper2);
  /** 
   * Return the value of variable in the variable space of given object. If not found, default value is returned.
   * @param oper1 object
   * @param oper2 [name, default value]
   * @since 2.92
   */
  public native static Object getVariable(GameObject oper1, java.util.List oper2);
  /** 
   * Return the value of variable in the variable space of given group.
   * @param oper1 group
   * @param oper2 name
   * @since 5501
   */
  public native static Object getVariable(GameGroup oper1, String oper2);
  /** 
   * Return the value of variable in the variable space of given group. If not found, default value is returned.
   * @param oper1 group
   * @param oper2 [name, dafault value]
   * @since 5501
   */
  public native static Object getVariable(GameGroup oper1, java.util.List oper2);
  /** 
   * Return the value of variable in the variable space of given team member.
   * @param oper1 teamMember
   * @param oper2 name
   * @since 2.92
   */
  public native static Object getVariable(GameTeamMember oper1, String oper2);
  /** 
   * Return the value of variable in the variable space of given team member.
   * @param oper1 teamMember
   * @param oper2 [name, default value]
   * @since 2.92
   */
  public native static Object getVariable(GameTeamMember oper1, java.util.List oper2);
  /** 
   * Return the value of variable in the variable space of given task.
   * @param oper1 task
   * @param oper2 name
   * @since 2.92
   */
  public native static Object getVariable(GameTask oper1, String oper2);
  /** 
   * Return the value of variable in the variable space of given location.
   * @param oper1 location
   * @param oper2 name
   * @since 2.92
   */
  public native static Object getVariable(GameLocation oper1, String oper2);
  /** 
   * Assigns the soldier as driver of the given vehicle.
<br/><br/>
   * <b>Example:</b> player assignAsDriver tankOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void assignAsDriver(GameObject oper1, GameObject oper2);
  /** 
   * Returns the additional text (invisible) in an item with the given position of the given 2D listbox.
<br/><br/>
   * <b>Example:</b> _data = _control lnbData [row, column]<br/>
   * @param oper1 control
   * @param oper2 [row, column]
   * @since 5501
   */
  public native static String lnbData(GameControl oper1, java.util.List oper2);
  /** 
   * Assigns an action to the given button or active text. Action is any expression in this scripting language.
<br/><br/>
   * <b>Example:</b> _control buttonSetAction "player exec ""reply.sqs"""<br/>
   * @param oper1 control
   * @param oper2 action
   * @since 2.91
   */
  public native static void buttonSetAction(GameControl oper1, String oper2);
  /** 
   * 
   */
  public native static Object diag_stressMem(float oper1, float oper2);
  /** 
   * 
   */
  public native static void diag_setenv(String oper1, java.util.List oper2);
  /** 
   * Returns parameters describing group in high command bar. Return value is [string,float[4]]
<br/><br/>
   * <b>Example:</b> unit hcGroupParams group<br/>
   * @param oper1 unit
   * @param oper2 group
   * @since 5501
   */
  public native static java.util.List hcGroupParams(GameObject oper1, GameGroup oper2);
  /** 
   * Sets how the target is known to the other centers. They behave like the target was seen age seconds ago. Possible age values are: "ACTUAL", "5 MIN", "10 MIN", "15 MIN", "30 MIN", "60 MIN", "120 MIN" or "UNKNOWN".
<br/><br/>
   * <b>Example:</b> player setTargetAge "10 MIN"<br/>
   * @param oper1 object
   * @param oper2 age
   * @since 2.32
   */
  public native static void setTargetAge(GameObject oper1, String oper2);
  /** 
   * Check if conversation topic was registered to given person.
   * @param oper1 person
   * @param oper2 name
   * @since 2.92
   */
  public native static boolean kbHasTopic(GameObject oper1, String oper2);
  /** 
   * Forces all units in the list to get in their assigned vehicle.
<br/><br/>
   * <b>Example:</b> [unitOne, unitTwo] orderGetIn true<br/>
   * @param oper1 unitArray
   * @param oper2 order
   */
  public native static void orderGetIn(java.util.List oper1, boolean oper2);
  /** 
   * Sets the marker size. Size is in format [a-axis, b-axis]. The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerSize [100, 200]<br/>
   * @param oper1 marker
   * @param oper2 size
   * @since 1.21
   */
  public native static void setMarkerSize(String oper1, java.util.List oper2);
  /** 
   * Text background - the right argument uses format ["text","type",speed] or ["text","type"]. If speed is not given, it's assumed to be one. Type may be one of: "PLAIN", "PLAIN DOWN", "BLACK", "BLACK FADED", "BLACK OUT", "BLACK IN", "WHITE OUT" or "WHITE IN".
The left argument define layer in which the effect is show, 0 is the back most.
<br/><br/>
   * <b>Example:</b> 0 cutText ["", "BLACK OUT"]<br/>
   * @param oper1 layer
   * @param oper2 effect
   * @since 5126
   */
  public native static void cutText(float oper1, java.util.List oper2);
  /** 
   * Causes a smooth change in the radio volume. The change duration is given by time, the target volume by volume. The default radio volume is 1.0.
<br/><br/>
   * <b>Example:</b> 5 fadeRadio 0.1<br/>
   * @param oper1 time
   * @param oper2 volume
   */
  public native static void fadeRadio(float oper1, float oper2);
  /** 
   * Prepares the camera dive angle. See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareDive -0.1<br/>
   * @param oper1 camera
   * @param oper2 dive
   * @since 2.95
   */
  public native static void camPrepareDive(GameObject oper1, float oper2);
  /** 
   * Sets the additional text (invisible) in the item with the given index of the given 2D listbox to the given data.
<br/><br/>
   * <b>Example:</b> _control lnbSetData [[row, column], "#1"]<br/>
   * @param oper1 control
   * @param oper2 [[row, column], text]
   * @since 5501
   */
  public native static void lnbSetText(GameControl oper1, java.util.List oper2);
  /** 
   * Add given member to given team.
<br/><br/>
   * <b>Example:</b> _team addTeamMember _teamMember<br/>
   * @param oper1 team
   * @param oper2 member
   * @since 2.90
   */
  public native static void addTeamMember(GameTeamMember oper1, GameTeamMember oper2);
  /** 
   * Executes the given command for each team member of the team (recursively).
   * @param oper1 command
   * @param oper2 team
   * @since 2.92
   */
  public native static void forEachMemberTeam(GameCode oper1, GameTeamMember oper2);
  /** 
   * 
   */
  public native static void diag_engStressVM(float oper1, float oper2);
  /** 
   * Xbox Live MP only: adds score to the Xbox Live Statistics score for the given unit (or the commander unit of the given vehicle).
<br/><br/>
   * <b>Example:</b> player addLiveStats 10<br/>
   * @param oper1 unit
   * @param oper2 score
   * @since 2.10
   */
  public native static void addLiveStats(GameObject oper1, float oper2);
  /** 
   * Create child display of given display and load from resource "name".
<br/><br/>
   * <b>Example:</b> _display createDisplay "RscObserver"<br/>
   * @param oper1 parent
   * @param oper2 name
   * @since 2.53
   */
  public native static void createDisplay(GameDisplay oper1, String oper2);
  /** 
   * Updates the icon to be shown in 2D editor for the specified editor object. If maintain size is false, icon will not scale depending on the scale of the map. If maintain size is a number, the icon will maintain size if map scale is below that number.
   * @param oper1 map
   * @param oper2 [object, string identifier, color, offset, width, height, maintain size?, angle, shadow]
   * @since 2.92
   */
  public native static void updateDrawIcon(GameControl oper1, java.util.List oper2);
  /** 
   * Enable/disable given AI feature. Feature may be one of: "AwareFormationSoft", "CombatFormationSoft".
<br/><br/>
   * <b>Example:</b> "AwareFormationSoft" enableAIFeature true<br/>
   * @param oper1 feature
   * @param oper2 enabled
   * @since 60220
   */
  public native static void enableAIFeature(String oper1, boolean oper2);
  /** 
   * Assigns a static object to the trigger. The activation source is changed to "STATIC".
<br/><br/>
   * <b>Example:</b> trigger triggerAttachObject 1234<br/>
   * @param oper1 trigger
   * @param oper2 objectId
   * @since 1.86
   */
  public native static void triggerAttachObject(GameObject oper1, float oper2);
  /** 
   * Sets the person's face.
<br/><br/>
   * <b>Example:</b> setFace "Face10"<br/>
   * @param oper1 person
   * @param oper2 soldier
   */
  public native static void setFace(GameObject oper1, String oper2);
  /** 
   * Sets rank of given unit. Possible values: PRIVATE, CORPORAL, SERGEANT, LIEUTENANT, CAPTAIN, MAJOR or COLONEL.
<br/><br/>
   * <b>Example:</b> player setRank "COLONEL"<br/>
   * @param oper1 unit
   * @param oper2 rank
   * @since 2.92
   */
  public native static void setRank(GameObject oper1, String oper2);
  /** 
   * Creates a new menu item. Menu can be "file" or "view", index is index as returned from addMenu command. priority is optional and determines where in the menu the item will reside (higher priority items first).
   * @param oper1 map
   * @param oper2 [menu or index,text,command,priority]
   * @since 2.92
   */
  public native static float addMenuItem(GameControl oper1, java.util.List oper2);
  /** 
   * Sets how friendly side1 is with side2. For a value smaller than 0.6 it results in being enemy, otherwise it's friendly.
   * @param oper1 side1
   * @param oper2 [side2, value]
   * @since 1.86
   */
  public native static void setFriend(GameSide oper1, java.util.List oper2);
  /** 
   * Create a new AI task (subtask of parentTask). Type is name of registered task type.
   * @param oper1 sender
   * @param oper2 [receiver, [type] or [type, parentTask], priority, name1, value1, name2, value2, ...]
   * @since 2.92
   */
  public native static GameTask sendTask(GameTeamMember oper1, java.util.List oper2);
  /** 
   * Switches the vehicle's engine on (true) or off (false).
<br/><br/>
   * <b>Example:</b> vehicle player engineOn false<br/>
   * @param oper1 vehicle
   * @param oper2 on
   * @since 1.90
   */
  public native static void engineOn(GameObject oper1, boolean oper2);
  /** 
   * Synchronizes the waypoint with other waypoints. Each waypoint is given as an array [group, index].
<br/><br/>
   * <b>Example:</b> [group1, 2] synchronizeWaypoint [[group2, 3]]<br/>
   * @param oper1 waypoint
   * @param oper2 [waypoint1, waypoint2, ...]
   * @since 1.86
   */
  public native static void synchronizeWaypoint(java.util.List oper1, java.util.List oper2);
  /** 
   * Synchronizes the trigger with waypoints. Each waypoint is given as an array [group, index].
<br/><br/>
   * <b>Example:</b> trigger synchronizeWaypoint []<br/>
   * @param oper1 trigger
   * @param oper2 [waypoint1, waypoint2, ...]
   * @since 1.86
   */
  public native static void synchronizeWaypoint(GameObject oper1, java.util.List oper2);
  /** 
   * Draw icon in map.
   * @param oper1 map
   * @param oper2 [texture, color, position, width, height, angle, text, shadow]
   * @since 2.35
   */
  public native static void drawIcon(GameControl oper1, java.util.List oper2);
  /** 
   * Adds an item with the given text to the given listbox or combobox. It returns the index of the newly added item.
<br/><br/>
   * <b>Example:</b> _index = _control lbAdd "First item"<br/>
   * @param oper1 control
   * @param oper2 text
   * @since 2.91
   */
  public native static float lbAdd(GameControl oper1, String oper2);
  /** 
   * Set ti parameters for specified vehicle, pars: engine/body, tracks/wheels, main gun.
   * @param oper1 vehicle
   * @param oper2 tiParams
   */
  public native static void setVehicleTiPars(GameObject oper1, java.util.List oper2);
  /** 
   * Commit control animation.
<br/><br/>
   * <b>Example:</b> _control ctrlCommit 2<br/>
   * @param oper1 control
   * @param oper2 time
   * @since 2.50
   */
  public native static void ctrlCommit(GameControl oper1, float oper2);
  /** 
   * Find the road segments within the circle of given radius.
<br/><br/>
   * <b>Example:</b> _list = player nearRoads 50<br/>
   * @param oper1 position or object
   * @param oper2 radius
   * @since 5500
   */
  public native static java.util.List nearRoads(Object oper1, float oper2);
  /** 
   * Moves the soldier into the vehicle's cargo position. (Immediately, without animation).
<br/><br/>
   * <b>Example:</b> soldierOne moveInCargo jeepOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void moveInCargo(GameObject oper1, GameObject oper2);
  /** 
   * Moves the soldier into a vehicle's specified cargo position. (Immediately, without animation).
<br/><br/>
   * <b>Example:</b> soldierOne moveInCargo jeepOne<br/>
   * @param oper1 soldier
   * @param oper2 [vehicle, CargoIndex]
   */
  public native static void moveInCargo(GameObject oper1, java.util.List oper2);
  /** 
   * Set time when old pars are replaced by last set, if 0 set immediately
<br/><br/>
   * <b>Example:</b> ...<br/>
   * @param oper1 string
   * @param oper2 number
   * @since 2.92
   */
  public native static void ppEffectCommit(String oper1, float oper2);
  /** 
   * Set time when old pars are replaced by last set, if 0 set immediately
<br/><br/>
   * <b>Example:</b> hndl ppEffectCommit 0<br/>
   * @param oper1 number
   * @param oper2 number
   * @since 2.92
   */
  public native static void ppEffectCommit(float oper1, float oper2);
  /** 
   * Set time when old pars are replaced by last set, if 0 set immediately
<br/><br/>
   * <b>Example:</b> hndl ppEffectCommit 0<br/>
   * @param oper1 array
   * @param oper2 scalar
   * @since 2.92
   */
  public native static void ppEffectCommit(java.util.List oper1, float oper2);
  /** 
   * Orders the unit to move to the given position (format <ar>Position</ar>) (silently).
<br/><br/>
   * <b>Example:</b> soldierOne doMove getMarkerPos "MarkerMoveOne"<br/>
   * @param oper1 unit
   * @param oper2 position
   */
  public native static void doMove(Object oper1, java.util.List oper2);
  /** 
   * Insert an object to the editor and assign arguments. Create script is not called. Returns the ID of the new EditorObject. Subtype class is optional.
   * @param oper1 map
   * @param oper2 [type, value, [name1, value1, ...], subtype class]
   * @since 2.92
   */
  public native static String insertEditorObject(GameControl oper1, java.util.List oper2);
  /** 
   * Returns the picture name of the item with the given index of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _picture = _control lbPicture 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.91
   */
  public native static String lbPicture(GameControl oper1, float oper2);
  /** 
   * Adds an item with the given text to the given listbox or combobox. It returns the index of the newly added item.
<br/><br/>
   * <b>Example:</b> _index = _control lbAdd "First item"<br/>
   * @param oper1 control
   * @param oper2 position
   * @since 5501
   */
  public native static float lnbAddColumn(GameControl oper1, float oper2);
  /** 
   * Defines the trigger activation type. The first argument - who activates trigger (side, radio, vehicle or group member): "NONE", "EAST", "WEST", "GUER", "CIV", "LOGIC", "ANY", "ALPHA", "BRAVO", "CHARLIE", "DELTA", "ECHO", "FOXTROT", "GOLF", "HOTEL", "INDIA", "JULIET", "STATIC", "VEHICLE", "GROUP", "LEADER" or "MEMBER". The second argument - when is it activated (presention or detection by the specified side): "PRESENT", "NOT PRESENT", "WEST D", "EAST D", "GUER D" or "CIV D". The third argument - whether the activation is repeating.
<br/><br/>
   * <b>Example:</b> trigger setTriggerActivation ["WEST", "EAST D", true]<br/>
   * @param oper1 trigger
   * @param oper2 [by, type, repeating]
   * @since 1.86
   */
  public native static void setTriggerActivation(GameObject oper1, java.util.List oper2);
  /** 
   * Changes the fog value smoothly during the given time (in seconds). A time of zero means there will be an immediate change. A fog level of zero is minimum fog and a fog level of one is maximum fog.
<br/><br/>
   * <b>Example:</b> 1800 setFog 0.5<br/>
   * @param oper1 time
   * @param oper2 fog
   */
  public native static void setFog(float oper1, float oper2);
  /** 
   * Execute statement attached to vehicle. This statement is also propagated over network in MP games.
<br/><br/>
   * <b>Example:</b> soldier3 setVehicleInit "this allowfleeing 0"<br/>
   * @param oper1 vehicle
   * @param oper2 statement
   * @since 2.33
   */
  public native static void setVehicleInit(GameObject oper1, String oper2);
  /** 
   * Defines the time between condition satisfaction and waypoint finish (randomly from min to max, with an average value mid).
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointTimeout [5, 10, 6]<br/>
   * @param oper1 waypoint
   * @param oper2 [min, mid, max]
   * @since 1.86
   */
  public native static void setWaypointTimeout(java.util.List oper1, java.util.List oper2);
  /** 
   * Lock the gunner position of the vehicle turret.
   * @param oper1 vehicle
   * @param oper2 [turret path, lock]
   * @since 5501
   */
  public native static void lockTurret(GameObject oper1, java.util.List oper2);
  /** 
   * Returns the text color of the item with the given index of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _color = _control lbColor 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.91
   */
  public native static java.util.List lbColor(GameControl oper1, float oper2);
  /** 
   * focusRange is in format [distance,blur].
Sets the camera focus blur. It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetFocus [50, 1]<br/>
   * @param oper1 camera
   * @param oper2 focusRange
   */
  public native static void camSetFocus(GameObject oper1, java.util.List oper2);
  /** 
   * Controls the lamppost mode. Mode may be "ON", "OFF" and "AUTO". "AUTO" is default and means the lampost is only on during nighttime.
<br/><br/>
   * <b>Example:</b> nearestObject  [player, "StreetLamp"] switchLight "Off"<br/>
   * @param oper1 lamppost
   * @param oper2 mode
   */
  public native static void switchLight(GameObject oper1, String oper2);
  /** 
   * Terminate the effect in the given layer and set duration of the fade out phase to the given time.
<br/><br/>
   * <b>Example:</b> 0 cutFadeIn 1.0<br/>
   * @param oper1 layer
   * @param oper2 duration
   * @since 5126
   */
  public native static void cutFadeOut(float oper1, float oper2);
  /** 
   * Damages / repairs the object. Damage 0 means the object is fully functional, damage 1 means it's completely destroyed / dead.
Note: this function is identical to <f>setDammage</f>. It was introduced to fix a spelling error in the original function name.
<br/><br/>
   * <b>Example:</b> player setdamage 1<br/>
   * @param oper1 object
   * @param oper2 damage
   * @since 1.50
   */
  public native static void setDamage(GameObject oper1, float oper2);
  /** 
   * Changes the overcast level to the given value smoothly during the given time (in seconds). A time of zero means an immediate change. An overcast level of zero means clear (sunny) weather and with an overcast level of one, storms and showers are very likely.
<br/><br/>
   * <b>Example:</b> 1800 setOvercast 0.5<br/>
   * @param oper1 time
   * @param oper2 overcast
   */
  public native static void setOvercast(float oper1, float oper2);
  /** 
   * Sets the camera dive angle. It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetDive -0.1<br/>
   * @param oper1 camera
   * @param oper2 dive
   */
  public native static void camSetDive(GameObject oper1, float oper2);
  /** 
   * Converts position from world space to object model space.
   * @param oper1 object
   * @param oper2 worldPos
   * @since 2.92
   */
  public native static java.util.List worldToModel(GameObject oper1, java.util.List oper2);
  /** 
   * Return child control with specified idc.
<br/><br/>
   * <b>Example:</b> _control = _display displayCtrl 101<br/>
   * @param oper1 display
   * @param oper2 idc
   * @since 2.50
   */
  public native static GameControl displayCtrl(GameDisplay oper1, float oper2);
  /** 
   * Returns the given indexed position in a building. The returned value is in format <ar>Position</ar>.
<br/><br/>
   * <b>Example:</b> building buildingPos 1<br/>
   * @param oper1 building
   * @param oper2 index
   */
  public native static java.util.List buildingPos(GameObject oper1, float oper2);
  /** 
   * Creates a previously added menu.
   * @param oper1 map
   * @param oper2 index
   * @since 2.92
   */
  public native static void createMenu(GameControl oper1, float oper2);
  /** 
   * Check whether gunner position of the vehicle turret is locked.
   * @param oper1 vehicle
   * @param oper2 turret path
   * @since 5501
   */
  public native static boolean lockedTurret(GameObject oper1, java.util.List oper2);
  /** 
   * Causes a smooth change in the master speech volume. The change duration is given by time, the target volume by volume. The default master volume is 1.0.
<br/><br/>
   * <b>Example:</b> 5 fadeSound 0.1<br/>
   * @param oper1 time
   * @param oper2 volume
   */
  public native static void fadeSpeech(float oper1, float oper2);
  /** 
   * Selects the shape (type) of the marker. Shape can be "ICON", "RECTANGLE" or "ELLIPSE". The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerShapeLocal "RECTANGLE"<br/>
   * @param oper1 name
   * @param oper2 shape
   * @since 2.92
   */
  public native static void setMarkerShapeLocal(String oper1, String oper2);
  /** 
   * Sets the description shown in the HUD while the waypoint is active.
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointDescription "Move here."<br/>
   * @param oper1 waypoint
   * @param oper2 text
   * @since 1.86
   */
  public native static void setWaypointDescription(java.util.List oper1, String oper2);
  /** 
   * Switches the camera to the given vehicle / camera. Mode is one of: "INTERNAL" (1st person), "GUNNER" (optics / sights), "EXTERNAL" (3rd person) or "GROUP" (group).
<br/><br/>
   * <b>Example:</b> sniperOne switchCamera "gunner"<br/>
   * @param oper1 unit
   * @param oper2 mode
   */
  public native static void switchCamera(GameObject oper1, String oper2);
  /** 
   * Return the list of all objects of given type.
   * @param oper1 map
   * @param oper2 type
   * @since 2.92
   */
  public native static java.util.List listObjects(GameControl oper1, String oper2);
  /** 
   * Adds number to the unit rating. This is usually used to reward for completed mission objectives. The rating for killed enemies and killed friendlies is adjusted automatically. When the rating is lower than zero, a unit is considered "renegade" and is an enemy to everyone.
<br/><br/>
   * <b>Example:</b> player addRating 1000<br/>
   * @param oper1 unit
   * @param oper2 number
   */
  public native static void addRating(GameObject oper1, float oper2);
  /** 
   * focusRange is in format [distance,blur].
Prepares the camera focus blur. See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareFocus [50, 1]<br/>
   * @param oper1 camera
   * @param oper2 focusRange
   * @since 2.95
   */
  public native static void camPrepareFocus(GameObject oper1, java.util.List oper2);
  /** 
   * Check whether all data are loaded to nearObjects will return in reasonable time.
<br/><br/>
   * <b>Example:</b> _ok = position player nearObjectsReady 50<br/>
   * @param oper1 position
   * @param oper2 radius
   * @since 5501
   */
  public native static boolean nearObjectsReady(Object oper1, float oper2);
  /** 
   * Removes all event handlers of the given type which were added by <f>addEventHandler</f>.
<br/><br/>
   * <b>Example:</b> player removeAllEventHandlers "killed"<br/>
   * @param oper1 object
   * @param oper2 handlerType
   * @since 1.85
   */
  public native static void removeAllEventHandlers(GameObject oper1, String oper2);
  /** 
   * Returns the hiding position in format <ar>Position</ar>. If enemy is null it is the some position in front of the object or enemy position otherwise.
   * @param oper1 object
   * @param oper2 enemy
   * @since 2.92
   */
  public native static java.util.List getHideFrom(GameObject oper1, GameObject oper2);
  /** 
   * Allow/dissallow 3D mode.
   * @param oper1 map
   * @param oper2 bool
   * @since 2.92
   */
  public native static void allow3DMode(GameControl oper1, boolean oper2);
  /** 
   * Check whether given row of the given listbox is selected.
<br/><br/>
   * <b>Example:</b> _selected = _control lbIsSelected 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.92
   */
  public native static boolean lbIsSelected(GameControl oper1, float oper2);
  /** 
   * Draw location in the map.
   * @param oper1 map
   * @param oper2 location
   * @since 2.90
   */
  public native static void drawLocation(GameControl oper1, GameLocation oper2);
  /** 
   * Adds a magazine to the turret. Use turret path [-1] for driver's turret. Note: you may create invalid combinations by using this function, for example by adding 20 grenades. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> player addMagazine "M16"<br/>
   * @param oper1 transport
   * @param oper2 [weaponName, turret path]
   */
  public native static void addMagazineTurret(GameObject oper1, java.util.List oper2);
  /** 
   * Set waypoint's visibility.
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointVisible false<br/>
   * @param oper1 waypoint
   * @param oper2 visible
   * @since 5500
   */
  public native static void setWaypointVisible(java.util.List oper1, boolean oper2);
  /** 
   * Set interval of emitting particles from particle source.
<br/><br/>
   * <b>Example:</b> _source setDropInterval 0.05<br/>
   * @param oper1 particleSource
   * @param oper2 interval
   * @since 2.56
   */
  public native static void setDropInterval(GameObject oper1, float oper2);
  /** 
   * Returns if sides are friendly or hostile.  For a value smaller than 0.6 it results in being enemy, otherwise it's friendly.
<br/><br/>
   * <b>Example:</b> value = west getFriend east<br/>
   * @param oper1 side1
   * @param oper2 side2
   * @since 5501
   */
  public native static float getFriend(GameSide oper1, GameSide oper2);
  /** 
   * Sets the object position. The pos array uses the <ar>Position</ar> format.
<br/><br/>
   * <b>Example:</b> player setPos [getpos player select 0, getpos player select 1 + 10]<br/>
   * @param oper1 obj
   * @param oper2 pos
   */
  public native static void setPos(GameObject oper1, java.util.List oper2);
  /** 
   * Selects the shape (type) of the marker. Shape can be "ICON", "RECTANGLE" or "ELLIPSE". The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerShape "RECTANGLE"<br/>
   * @param oper1 name
   * @param oper2 shape
   * @since 1.86
   */
  public native static void setMarkerShape(String oper1, String oper2);
  /** 
   * This command defines the level of access a user has to editor objects. objects is an array of either Editor Objects (eg [_unit_0]) or actual Game Objects (eg [player]). If the array is empty then the command will automatically parse all editor objects. editor type is the editor type to effect (eg unit), or  for none. condition is an executable string that must evaluate to true or false. If true, the scope of the evaluated editor object will be modified. scope is one of HIDE, VIEW, SELECT, LINKTO, LINKFROM, ALLNODRAG, ALLNOTREE or ALL. subordinates also is a boolean value, if true then subordinates in the editor will be assigned the same scope as the parent.
<br/><br/>
   * <b>Example:</b> _map setEditorObjectScope [[],vehicle,side effectiveCommander _x != side player,HIDE,false];<br/>
   * @param oper1 map
   * @param oper2 [objects,editor type,condition,scope,subordinates also]
   * @since 2.92
   */
  public native static void setEditorObjectScope(GameControl oper1, java.util.List oper2);
  /** 
   * Forces a helicopter landing. The landing mode may be "LAND" (a complete stop), "GET IN" (hovering very low for another unit to get in), "GET OUT" (hovering low for another unit to get out) or "NONE" (cancel landing).
<br/><br/>
   * <b>Example:</b> cobraOne land "LAND"<br/>
   * @param oper1 helicopter
   * @param oper2 mode
   */
  public native static void land(GameObject oper1, String oper2);
  /** 
   * Find entities in the circle with given radius. If typeName(s) is (are) given, only entities of given type (or its subtype) are listed.
<br/><br/>
   * <b>Example:</b> _list = position player nearObjects 50<br/>
   * @param oper1 position
   * @param oper2 radius or [typeName, radius] or [[typeName1, typeName2, ...], radius]
   * @since 5501
   */
  public native static java.util.List nearEntities(Object oper1, Object oper2);
  /** 
   * Sets the text label of an existing marker. The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerTextLocal You are here.<br/>
   * @param oper1 name
   * @param oper2 text
   * @since 2.92
   */
  public native static void setMarkerTextLocal(String oper1, String oper2);
  /** 
   * Set ambient color of light.
   * @param oper1 light
   * @param oper2 [r, g, b]
   * @since 2.58
   */
  public native static void setLightAmbient(GameObject oper1, java.util.List oper2);
  /** 
   * Remove a simple task from the list of simple tasks.
   * @param oper1 person
   * @param oper2 task
   * @since 5501
   */
  public native static void removeSimpleTask(GameObject oper1, GameTask oper2);
  /** 
   * Executes a script. Argument is passed to the script as local variable _this.
   * @param oper1 arguments
   * @param oper2 script
   * @since 2.50
   */
  public native static GameScript spawn(Object oper1, GameCode oper2);
  /** 
   * Prepares the camera field of view range for auto zooming. See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareFovRange [0.1, 0.5]<br/>
   * @param oper1 camera
   * @param oper2 fovRange
   * @since 2.95
   */
  public native static void camPrepareFovRange(GameObject oper1, java.util.List oper2);
  /** 
   * Set object's up vector. Direction vector will remain unchanged.
   * @param oper1 object
   * @param oper2 [x, z, y]
   * @since 2.61
   */
  public native static void setVectorUp(GameObject oper1, java.util.List oper2);
  /** 
   * Checks how many vehicles belong to the given side.
<br/><br/>
   * <b>Example:</b> west countSide list triggerOne<br/>
   * @param oper1 side
   * @param oper2 array
   */
  public native static float countSide(GameSide oper1, java.util.List oper2);
  /** 
   * Sets tooltip text color of given control. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control ctrlSetTooltipColorText [1, 0, 0, 1]<br/>
   * @param oper1 display
   * @param oper2 color
   * @since 2.92
   */
  public native static void ctrlSetTooltipColorText(GameControl oper1, java.util.List oper2);
  /** 
   * Prepares the camera heading. See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareDir 150<br/>
   * @param oper1 camera
   * @param oper2 direction
   * @since 2.95
   */
  public native static void camPrepareDir(GameObject oper1, float oper2);
  /** 
   * Limit speed of given vehicle to given value (in km/h).
   * @param oper1 object
   * @param oper2 speed
   * @since 2.92
   */
  public native static void limitSpeed(GameObject oper1, float oper2);
  /** 
   * Make the person tell to the receiver the sentence.
   * @param oper1 person
   * @param oper2 [receiver, topic, sentence id, [argument name, argument value, argument text, argument speech], ...]
   * @since 2.92
   */
  public native static void kbTell(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the object position. The pos array uses the <ar>PositionATL</ar> format.
<br/><br/>
   * <b>Example:</b> player setPosASL [getPosATL player select 0, getPosATL player select 1 + 10, getPosATL select 2]<br/>
   * @param oper1 obj
   * @param oper2 pos
   * @since 2.53
   */
  public native static void setPosATL(GameObject oper1, java.util.List oper2);
  /** 
   * Return name of object argument in mission editor.
   * @param oper1 map
   * @param oper2 [object, argument]
   * @since 2.35
   */
  public native static String getObjectArgument(GameControl oper1, java.util.List oper2);
  /** 
   * Set the destination for path planning of the pilot.
   * @param oper1 object
   * @param oper2 [position, planningMode, forceReplan]
   * @since 2.92
   */
  public native static void setDestination(GameObject oper1, java.util.List oper2);
  /** 
   * Sets skill of given unit. Skill may vary from 0.2 to 1.0.
<br/><br/>
   * <b>Example:</b> player setUnitSkill 1.0<br/>
   * @param oper1 unit
   * @param oper2 skill
   * @since 2.33
   */
  public native static void setUnitAbility(GameObject oper1, float oper2);
  /** 
   * 
   */
  public native static void diag_objScale(String oper1, float oper2);
  /** 
   * Show the add editor object dialog, type is editor object type, class is class definition to automatically select, side filters by a certain side, pos is position to create the object.
   * @param oper1 map
   * @param oper2 [type, class, side, position]
   * @since 2.92
   */
  public native static Object showNewEditorObject(GameControl oper1, java.util.List oper2);
  /** 
   * Defines an action performed when the user double clicks on the map. Command receives:<br/>
<br/>
_pos <t>array</t> position<br/>
_units <t>array</t> selected units<br/>
_shift,_alt <t>bool</t> key state
   * @param oper1 map
   * @param oper2 command
   * @since 2.92
   */
  public native static Object onDoubleClick(GameControl oper1, String oper2);
  /** 
   * Checks whether the type typeName1 is inherited from the type typeName1.
<br/><br/>
   * <b>Example:</b> "Tank" isKindOf "Land"<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 typeName1
   * @param oper2 typeName2
   * @since 5160
   */
  public native static boolean isKindOf(String oper1, String oper2);
  /** 
   * Checks whether the object is of the given type.
<br/><br/>
   * <b>Example:</b> vehicle player isKindOf "Tank"<br/>
   * @param oper1 object
   * @param oper2 typeName
   * @since 2.30
   */
  public native static boolean isKindOf(GameObject oper1, String oper2);
  /** 
   * Check if given item was said by person to someone.
   * @param oper1 person
   * @param oper2 [receiver, topic, sentence id, max. age]
   * @since 5500
   */
  public native static boolean kbWasSaid(GameObject oper1, java.util.List oper2);
  /** 
   * Hide object (cannot hide static objects).
<br/><br/>
   * <b>Example:</b> player hideObject true<br/>
   * @param oper1 object
   * @param oper2 hidden
   * @since 2.92
   */
  public native static void hideObject(GameObject oper1, boolean oper2);
  /** 
   * Show the edit object dialog for the given object.
   * @param oper1 map
   * @param oper2 object
   * @since 2.92
   */
  public native static Object editObject(GameControl oper1, String oper2);
  /** 
   * Find named selection in object which is in specified lod intersected by given section of a line.
<br/><br/>
   * <b>Example:</b> [tank, "VIEW" intersect [[1500, 1500, 2], [1550, 1500, 2]]<br/>
   * @param oper1 [object, lod name]
   * @param oper2 [begin, end]
   * @since 2.53
   */
  public native static java.util.List intersect(java.util.List oper1, java.util.List oper2);
  /** 
   * Sets the armor (or health for men) state of the vehicle (a value from 0 to 1).
<br/><br/>
   * <b>Example:</b> player setVehicleArmor 0.5<br/>
   * @param oper1 object
   * @param oper2 value
   * @since 2.32
   */
  public native static void setVehicleArmor(GameObject oper1, float oper2);
  /** 
   * Returns the total number of user-added menu items belonging to the given menu.
   * @param oper1 map
   * @param oper2 menu index
   * @since 2.92
   */
  public native static float nMenuItems(GameControl oper1, Object oper2);
  /** 
   * Convert position in the map from screen coordinates to world coordinates.
   * @param oper1 map
   * @param oper2 [x, y]
   * @since 5127
   */
  public native static java.util.List ctrlMapScreenToWorld(GameControl oper1, java.util.List oper2);
  /** 
   * Unregister a task type.
   * @param oper1 teamMember
   * @param oper2 name
   * @since 2.90
   */
  public native static boolean unregisterTask(GameTeamMember oper1, String oper2);
  /** 
   * Joins the unit to the given group, if position id is available, this one is used. Avoid any radio communication related to joining.
<br/><br/>
   * <b>Example:</b> player joinAsSilent [_group, 4]<br/>
   * @param oper1 unit
   * @param oper2 [group, id]
   */
  public native static void joinAsSilent(GameObject oper1, java.util.List oper2);
  /** 
   * Set object's direction and up vector
   * @param oper1 object
   * @param oper2 [[x, z, y],[x, y, z]]
   * @since 5164
   */
  public native static void setVectorDirAndUp(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the flag side.
<br/><br/>
   * <b>Example:</b> setFlagSide east<br/>
   * @param oper1 flag
   * @param oper2 side
   */
  public native static void setFlagSide(GameObject oper1, GameSide oper2);
  /** 
   * Changes the waypoint type. Type can be: "MOVE", "DESTROY", "GETIN", "SAD", "JOIN", "LEADER", "GETOUT", "CYCLE", "LOAD", "UNLOAD", "TR UNLOAD", "HOLD", "SENTRY", "GUARD", "TALK", "SCRIPTED", "SUPPORT", "GETIN NEAREST", "AND" or "OR".
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointType "HOLD"<br/>
   * @param oper1 waypoint
   * @param oper2 type
   * @since 1.86
   */
  public native static void setWaypointType(java.util.List oper1, String oper2);
  /** 
   * Returns the number of available cargo, driver, gunner or commander positions in the vehicle.
<br/><br/>
   * <b>Example:</b> _freePositions = (vehicle player) freePositions "cargo"<br/>
   * @param oper1 vehicle
   * @param oper2 position
   * @since 2.92
   */
  public native static float emptyPositions(GameObject oper1, String oper2);
  /** 
   * Executes the given command for each member of the team (recursively).
   * @param oper1 command
   * @param oper2 team
   * @since 2.92
   */
  public native static void forEachMember(GameCode oper1, GameTeamMember oper2);
  /** 
   * Set a new state of the task.
   * @param oper1 task
   * @param oper2 state
   * @since 2.89
   */
  public native static void setTaskState(GameTask oper1, String oper2);
  /** 
   * 
   */
  public native static void diag_timing(String oper1, boolean oper2);
  /** 
   * Enable / disable simulation for given entity.
   * @param oper1 entity
   * @param oper2 enable
   */
  public native static void enableSimulation(GameObject oper1, boolean oper2);
  /** 
   * Sets the briefing objective status. Status may be one of: "ACTIVE", "FAILED", "DONE" or "HIDDEN".
<br/><br/>
   * <b>Example:</b> "obj_1" objStatus "FAILED"<br/>
   * @param oper1 objective
   * @param oper2 status
   */
  public native static void objStatus(String oper1, String oper2);
  /** 
   * Returns direction where is given weapon aiming.
<br/><br/>
   * <b>Example:</b> _dir = _vehicle weaponDirection "M16"<br/>
   * @param oper1 vehicle
   * @param oper2 weaponName
   * @since 2.61
   */
  public native static java.util.List weaponDirection(GameObject oper1, String oper2);
  /** 
   * Removes the magazine from the turret. Use turret path [-1] for driver's turret.  Note: you may create invalid combinations by using this function. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> vehicle player removeMagazine ["60rnd_cmflaremagazine",[-1]]<br/>
   * @param oper1 transport
   * @param oper2 [weaponName, turret path]
   */
  public native static void removeMagazineTurret(GameObject oper1, java.util.List oper2);
  /** 
   * Force the speed of the given object.
   * @param oper1 object
   * @param oper2 speed
   * @since 2.92
   */
  public native static void forceSpeed(GameObject oper1, float oper2);
  /** 
   * Moves the soldier into the vehicle's driver position. (Immediately, without animation).
<br/><br/>
   * <b>Example:</b> soldierOne moveInDriver jeepOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void moveInDriver(GameObject oper1, GameObject oper2);
  /** 
   * Draw arrow in map.
   * @param oper1 map
   * @param oper2 [position1, position2, color]
   * @since 2.35
   */
  public native static void drawArrow(GameControl oper1, java.util.List oper2);
  /** 
   * Removes all MP event handlers of the given type which were added by <f>addMPEventHandler</f>.
<br/><br/>
   * <b>Example:</b> player removeAllMPEventHandlers "mpkilled"<br/>
   * @param oper1 object
   * @param oper2 handlerType
   * @since 1.85
   */
  public native static void removeAllMPEventHandlers(GameObject oper1, String oper2);
  /** 
   * Remove all even handlers from the given control.
<br/><br/>
   * <b>Example:</b> _control ctrlRemoveAllEventHandlers "KeyDown"<br/>
   * @param oper1 control
   * @param oper2 handler name
   * @since 5501
   */
  public native static void ctrlRemoveAllEventHandlers(GameControl oper1, String oper2);
  /** 
   * Set if given location has rectangular shape.
   * @param oper1 location
   * @param oper2 rectangular
   * @since 2.90
   */
  public native static void setRectangular(GameLocation oper1, boolean oper2);
  /** 
   * Adds a new menu button. Priority is optional.
   * @param oper1 map
   * @param oper2 [text,priority]
   * @since 2.92
   */
  public native static float addMenu(GameControl oper1, java.util.List oper2);
  /** 
   * Changes the object position. If the markers array contains more than one marker names, the position of a random one is used. Otherwise, the given position is used. The object is placed inside a circle with this position as its center and placement as its radius.
<br/><br/>
   * <b>Example:</b> player setVehiclePosition [[0, 0, 0], ["Marker1"], 0]<br/>
   * @param oper1 object
   * @param oper2 [position, markers, placement]
   * @since 2.32
   */
  public native static void setVehiclePosition(GameObject oper1, java.util.List oper2);
  /** 
   * Assigns the soldier to the cargo / passenger space of the given vehicle.
<br/><br/>
   * <b>Example:</b> player assignAsCargo tankOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void assignAsCargo(GameObject oper1, GameObject oper2);
  /** 
   * Sends the message to the side radio channel. The message is defined in the description.ext file or radio protocol.
<br/><br/>
   * <b>Example:</b> soldierOne sideRadio "messageOne"<br/>
   * @param oper1 unit
   * @param oper2 radioName
   */
  public native static void sideRadio(Object oper1, String oper2);
  /** 
   * Moves the soldier into the vehicle's commander position. (Immediatetely, without animation).
<br/><br/>
   * <b>Example:</b> soldierOne moveInCommander jeepOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void moveInCommander(GameObject oper1, GameObject oper2);
  /** 
   * Sets P font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontP "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontP(GameControl oper1, String oper2);
  /** 
   * Sets P font size of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeightP 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontP(GameControl oper1, float oper2);
  /** 
   * Creates an empty vehicle of the given type. Pos is in format <ar>Position</ar>.
See <cl>CfgVehicles</cl> for possible type values. Vehicle is not transferred through network in MP games.
<br/><br/>
   * <b>Example:</b> _tank = "M1Abrams" createVehicleLocal getmarkerpos "tankFactory"<br/>
   * @param oper1 type
   * @param oper2 pos
   * @since 2.56
   */
  public native static GameObject createVehicleLocal(String oper1, java.util.List oper2);
  /** 
   * If [] is given, the trigger is detached from the assigned vehicle. If the activation source is "VEHICLE", "GROUP", "LEADER" or "MEMBER", it's changed to "NONE". If [vehicle] is given, the trigger is attached to the vehicle or its group. When the source is "GROUP", "LEADER" or "MEMBER", it's attached to the group, otherwise it's attached to the vehicle and the source is changed to "VEHICLE".
<br/><br/>
   * <b>Example:</b> trigger triggerAttachVehicle [player]<br/>
   * @param oper1 trigger
   * @param oper2 [] or [vehicle]
   * @since 1.86
   */
  public native static void triggerAttachVehicle(GameObject oper1, java.util.List oper2);
  /** 
   * Moves the marker. The format of pos is <ar>Position2D</ar>. The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerPos getMarkerPos "MarkerTwo"<br/>
   * @param oper1 markerName
   * @param oper2 pos
   */
  public native static void setMarkerPos(String oper1, java.util.List oper2);
  /** 
   * Sets number of second before auto-scroll starts.
<br/><br/>
   * <b>Example:</b> _control ctrlSetAutoScrollDelay 5<br/>
   * @param oper1 control
   * @param oper2 delay
   * @since 5501
   */
  public native static void ctrlSetAutoScrollDelay(GameControl oper1, float oper2);
  /** 
   * Set the icon to be shown in 2D editor for the specified editor object. If maintain size is false, icon will not scale depending on the scale of the map. If maintain size is a number, the icon will maintain size if map scale is below that number. is3D, show line and priority are optional.
   * @param oper1 map
   * @param oper2 [object, texture, color, offset, width, height, maintain size?, angle, string identifier, shadow, is3D, draw line?, priority]
   * @since 2.92
   */
  public native static void setDrawIcon(GameControl oper1, java.util.List oper2);
  /** 
   * Order AI airplane to land at given airport
<br/><br/>
   * <b>Example:</b> plane landAt 0<br/>
   * @param oper1 airplane
   * @param oper2 airportId
   * @since 2.92
   */
  public native static void landAt(GameObject oper1, float oper2);
  /** 
   * Sets skill of given type of person (commander unit). Value of skill may vary from 0 to 1.
<br/><br/>
   * <b>Example:</b> hero setSkill ["Endurance", 1]<br/>
   * @param oper1 vehicle
   * @param oper2 [type, skill]
   * @since 2.65
   */
  public native static void setSkill(GameObject oper1, java.util.List oper2);
  /** 
   * Sets ability levell of person (commander unit). Value of skill may vary from 0 to 1.
<br/><br/>
   * <b>Example:</b> hero setSkill 1<br/>
   * @param oper1 vehicle
   * @param oper2 skill
   * @since 1.75
   */
  public native static void setSkill(GameObject oper1, float oper2);
  /** 
   * Sets the marker alpha. The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerAlpha 0.5<br/>
   * @param oper1 marker
   * @param oper2 alpha
   * @since 5501
   */
  public native static void setMarkerAlphaLocal(String oper1, float oper2);
  /** 
   * Saves object's properties to objects.sav file in campaign directory as entry name.
<br/><br/>
   * <b>Example:</b> player saveStatus "playerState"<br/>
   * @param oper1 object
   * @param oper2 name
   * @since 1.75
   */
  public native static boolean saveStatus(GameObject oper1, String oper2);
  /** 
   * Checks whether given subject is present in the diary of given person.
   * @param oper1 person
   * @param oper2 name
   * @since 5501
   */
  public native static boolean diarySubjectExists(GameObject oper1, String oper2);
  /** 
   * Orders a unit to commence firing on the given target (silently). If the target is objNull, the unit is ordered to commence firing on its current target (set using doTarget or commandTarget).
<br/><br/>
   * <b>Example:</b> soldierOne doFire objNull<br/>
   * @param oper1 unit
   * @param oper2 target
   */
  public native static void doFire(Object oper1, GameObject oper2);
  /** 
   * Obsolete.
   * @param oper1 obj
   * @param oper2 allow
   */
  public native static void allowDamage(GameObject oper1, boolean oper2);
  /** 
   * Attaches a script to a scripted waypoint. Command consist of a script name and additional script arguments.
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointScript "find.sqs player"<br/>
   * @param oper1 waypoint
   * @param oper2 command
   * @since 1.86
   */
  public native static void setWaypointScript(java.util.List oper1, String oper2);
  /** 
   * Sets the main font of given control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFont "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFont(GameControl oper1, String oper2);
  /** 
   * Sets the amount or repair resources in the cargo space of a repair vehicle. An amount of one is a full cargo space.
<br/><br/>
   * <b>Example:</b> repairTruckOne setRepairCargo 0<br/>
   * @param oper1 vehicle
   * @param oper2 amount
   */
  public native static void setRepairCargo(GameObject oper1, float oper2);
  /** 
   * Sets id (integer value) to vehicle. By this id vehicle is referenced by triggers and waypoints.
<br/><br/>
   * <b>Example:</b> player setVehicleId 1<br/>
   * @param oper1 object
   * @param oper2 id
   * @since 2.33
   */
  public native static void setVehicleId(GameObject oper1, float oper2);
  /** 
   * Sets the additional integer value in the item with the given index of the given listbox or combobox to the given value.
<br/><br/>
   * <b>Example:</b> _control lbSetValue [0, 1]<br/>
   * @param oper1 control
   * @param oper2 [index, value]
   * @since 2.91
   */
  public native static void lbSetValue(GameControl oper1, java.util.List oper2);
  /** 
   * Add an event handler to the given display. Returns id of the handler or -1 when failed.
<br/><br/>
   * <b>Example:</b> _id = _display displayAddEventHandler ["KeyDown", ""]<br/>
   * @param oper1 display
   * @param oper2 [handler name, function]
   * @since 5501
   */
  public native static float displayAddEventHandler(GameDisplay oper1, java.util.List oper2);
  /** 
   * Set the leader of given team.
   * @param oper1 team
   * @param oper2 leader
   * @since 2.92
   */
  public native static void setLeader(GameTeamMember oper1, GameTeamMember oper2);
  /** 
   * Close given display.
<br/><br/>
   * <b>Example:</b> _display closeDisplay IDC_OK<br/>
   * @param oper1 display
   * @param oper2 exitcode
   * @since 2.53
   */
  public native static void closeDisplay(GameDisplay oper1, float oper2);
  /** 
   * Return a list of all the children of the specified object.
   * @param oper1 map
   * @param oper2 object
   * @since 2.92
   */
  public native static java.util.List getObjectChildren(GameControl oper1, String oper2);
  /** 
   * Adds bacpacks to the cargo space.  MP synchronized. The format of backpacks is [backpacksName, count].
For backpacksName values see <cl>CfgWeapons</cl>.
<br/><br/>
   * <b>Example:</b> rearmTruckOne addBackpackCargo ["DSHKM_TK_GUE_Bag_EP1", 2]<br/>
   * @param oper1 unit
   * @param oper2 backpacks
   */
  public native static void addBackpackCargoGlobal(GameObject oper1, java.util.List oper2);
  /** 
   * Sets tooltip text of given control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetTooltip "tooltip"<br/>
   * @param oper1 display
   * @param oper2 text
   * @since 2.92
   */
  public native static void ctrlSetTooltip(GameControl oper1, String oper2);
  /** 
   * Stops an AI unit. This function is obsolete. Use disableAI to get better control over stopping a unit.
<br/><br/>
   * <b>Example:</b> soldierOne stop true<br/>
   * @param oper1 unit
   * @param oper2 stop
   */
  public native static void stop(GameObject oper1, boolean oper2);
  /** 
   * Functionally same as Say, only difference is sound played as 2D (mono) allways.
<br/><br/>
   * <b>Example:</b> soldierOne say "speechId"<br/>
   * @param oper1 unit or [unit, target]
   * @param oper2 speechName
   * @since 5501
   */
  public native static void say2D(Object oper1, String oper2);
  /** 
   * Functionally same as Say, only difference is sound played as 2D (mono) allways.
   * @param oper1 unit or [unit, target]
   * @param oper2 speechName
   * @since 5501
   */
  public native static void say2D(Object oper1, java.util.List oper2);
  /** 
   * Enable/disable using light on gun
<br/><br/>
   * <b>Example:</b> groupOne enableGunLights true<br/>
   * @param oper1 group
   * @param oper2 enable
   */
  public native static void enableGunLights(Object oper1, boolean oper2);
  /** 
   * Counts how many units in the array are considered unknown to the given unit.
<br/><br/>
   * <b>Example:</b> player countUnknown list triggerOne<br/>
   * @param oper1 unit
   * @param oper2 array
   */
  public native static float countUnknown(GameObject oper1, java.util.List oper2);
  /** 
   * Selects index element of the array. Index 0 denotes the first element, 1 the second, etc.
<br/><br/>
   * <b>Example:</b> [1, 2, 3] select 1<br/>
<br/>
   * <b>Example result:</b> 2<br/>
   * @param oper1 array
   * @param oper2 index
   */
  public native static Object select(java.util.List oper1, float oper2);
  /** 
   * If the index is false, this selects the first element of the array. If it is true, it selects the second one.
   * @param oper1 array
   * @param oper2 index
   */
  public native static Object select(java.util.List oper1, boolean oper2);
  /** 
   * Returns subentry with given index.
<br/><br/>
   * <b>Example:</b> (configFile >> "CfgVehicles") select 0<br/>
   * @param oper1 config
   * @param oper2 index
   * @since 2.35
   */
  public native static GameConfig select(GameConfig oper1, float oper2);
  /** 
   * Adds magazines to the weapon cargo space. This is used for infantry weapons. The format of magazines is [magazineName, count].
For magazineName values see <cl>CfgWeapons</cl>.
<br/><br/>
   * <b>Example:</b> rearmTruckOne addMagazineCargo ["M16", 10]<br/>
   * @param oper1 unit
   * @param oper2 magazines
   */
  public native static void addMagazineCargo(GameObject oper1, java.util.List oper2);
  /** 
   * Returns the shown text in the item with the given index of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _text = _control lbText 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.91
   */
  public native static String lbText(GameControl oper1, float oper2);
  /** 
   * Set parameters to particle source. Array is in format <ar>ParticleArray</ar>.
   * @param oper1 particleSource
   * @param oper2 array
   * @since 2.56
   */
  public native static void setParticleParams(GameObject oper1, java.util.List oper2);
  /** 
   * Delete the editor object. Requires all editor object links to be removed prior.
   * @param oper1 map
   * @param oper2 object
   * @since 2.92
   */
  public native static Object deleteEditorObject(GameControl oper1, String oper2);
  /** 
   * Switches the camera to the given vehicle / camera. The format of effect is [name, position]. Name is one of: "Internal", "External", "Fixed" or "FixedWithZoom". Position is one of: "TOP", "LEFT", "RIGHT", "FRONT" or "BACK" ("BACK" is normally used).
<br/><br/>
   * <b>Example:</b> cameraEffect ["External", "Back"]<br/>
   * @param oper1 camera
   * @param oper2 effect
   */
  public native static void cameraEffect(GameObject oper1, java.util.List oper2);
  /** 
   * Load HTML from file to given control.
<br/><br/>
   * <b>Example:</b> _control htmlLoad "briefing.html"<br/>
   * @param oper1 control
   * @param oper2 filename
   * @since 2.53
   */
  public native static void htmlLoad(GameControl oper1, String oper2);
  /** 
   * Adds magazines to the weapon cargo space. This is used for infantry weapons.  MP synchronized. The format of magazines is [magazineName, count].
For magazineName values see <cl>CfgWeapons</cl>.
<br/><br/>
   * <b>Example:</b> rearmTruckOne addMagazineCargo ["M16", 10]<br/>
   * @param oper1 unit
   * @param oper2 magazines
   */
  public native static void addMagazineCargoGlobal(GameObject oper1, java.util.List oper2);
  /** 
   * Show/hide map legend.
   * @param oper1 map
   * @param oper2 bool
   * @since 2.92
   */
  public native static void showLegend(GameControl oper1, boolean oper2);
  /** 
   * Activate camera thermal vision
   * @param oper1 bool
   * @param oper2 mode index
   */
  public native static void setCamUseTi(boolean oper1, float oper2);
  /** 
   * Sets the unit position rules. Mode may be one of: "DOWN", "UP" or "AUTO".
<br/><br/>
   * <b>Example:</b> soldierOne setUnitPos "Down"<br/>
   * @param oper1 unit
   * @param oper2 mode
   */
  public native static void setUnitPos(GameObject oper1, String oper2);
  /** 
   * Adjust parameters for specified post process effect
<br/><br/>
   * <b>Example:</b> "radialBlurr" ppEffectEnable []<br/>
   * @param oper1 string
   * @param oper2 array
   * @since 2.92
   */
  public native static void ppEffectAdjust(String oper1, java.util.List oper2);
  /** 
   * Set post process effect parameters
<br/><br/>
   * <b>Example:</b> hndl ppEffectAdjust [0.0075, 0.0075, 0.1, 0.1]<br/>
   * @param oper1 number
   * @param oper2 array
   * @since 2.92
   */
  public native static void ppEffectAdjust(float oper1, java.util.List oper2);
  /** 
   * Sets the additional integer value in the item with the given position of the given 2D listbox  to the given value.
<br/><br/>
   * <b>Example:</b> _control lnbSetValue [[row, column], 1]<br/>
   * @param oper1 control
   * @param oper2 [[row, column], value]
   * @since 5501
   */
  public native static void lnbSetValue(GameControl oper1, java.util.List oper2);
  /** 
   * Select the subject page in a log.
   * @param oper1 person
   * @param oper2 subject
   * @since 5500
   */
  public native static void selectDiarySubject(GameObject oper1, String oper2);
  /** 
   * Search for selection in the object model (first in the memory level, then in other levels). Returns position in model space.
   * @param oper1 object
   * @param oper2 selection name
   * @since 2.92
   */
  public native static java.util.List selectionPosition(GameObject oper1, String oper2);
  /** 
   * Sets the marker alpha. The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerAlpha 0.5<br/>
   * @param oper1 marker
   * @param oper2 alpha
   * @since 5501
   */
  public native static void setMarkerAlpha(String oper1, float oper2);
  /** 
   * Draw rectangle in map.
   * @param oper1 map
   * @param oper2 [center, a, b, angle, color, fill]
   * @since 2.35
   */
  public native static void drawRectangle(GameControl oper1, java.util.List oper2);
  /** 
   * Preload all data for given object.
   * @param oper1 distance
   * @param oper2 object
   * @since 2.50
   */
  public native static boolean preloadObject(float oper1, Object oper2);
  /** 
   * Defines the music track played on activation. Track is a subclass name of CfgMusic. "$NONE$" (no change) or "$STOP$" (stops the current music track).
<br/><br/>
   * <b>Example:</b> trigger setMusicEffect "Track1"<br/>
   * @param oper1 trigger or waypoint
   * @param oper2 track
   * @since 1.86
   */
  public native static void setMusicEffect(Object oper1, String oper2);
  /** 
   * Allows the getting in of vehicles of all units in the list.
<br/><br/>
   * <b>Example:</b> [unitOne, unitTwo] allowGetIn false<br/>
   * @param oper1 unitArray
   * @param oper2 allow
   */
  public native static void allowGetIn(java.util.List oper1, boolean oper2);
  /** 
   * Compile and execute function (sqf). Argument is passed to the script as local variable _this. The function is first searched in the mission folder, then in the campaign scripts folder and finally in the global scripts folder.
<br/><br/>
   * <b>Example:</b> player execVM "test.sqf"<br/>
   * @param oper1 argument
   * @param oper2 filename
   * @since 2.60
   */
  public native static GameScript execVM(Object oper1, String oper2);
  /** 
   * Executes the given command for each element in array.
It's executed as follows:

for each element of array an element is assigned as _x and the command is executed.
<br/><br/>
   * <b>Example:</b> "_x setdammage 1" forEach units group player<br/>
   * @param oper1 command
   * @param oper2 array
   */
  public native static void forEach(GameCode oper1, java.util.List oper2);
  /** 
   * Show / hide given control.
<br/><br/>
   * <b>Example:</b> _control ctrlShow false<br/>
   * @param oper1 control
   * @param oper2 show
   * @since 2.50
   */
  public native static void ctrlShow(GameControl oper1, boolean oper2);
  /** 
   * Selects the fill texture for the marker ("RECTANGLE" or "ELLIPSE"). Brush is the name of the subclass in CfgMarkerBrushes. The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerBrush "DiagGrid"<br/>
   * @param oper1 name
   * @param oper2 brush
   * @since 1.86
   */
  public native static void setMarkerBrush(String oper1, String oper2);
  /** 
   * temporary debugging command
   * @param oper1 unit
   * @param oper2 behaviour
   * @since 5501
   */
  public native static void diag_setCombatMode(GameObject oper1, String oper2);
  /** 
   * Register a new task type. Parameters are defined in the given config class (subclass of CfgTasks).
   * @param oper1 teamMember
   * @param oper2 entry name
   * @since 2.90
   */
  public native static boolean registerTask(GameTeamMember oper1, String oper2);
  /** 
   * Set a result of the task.
   * @param oper1 task
   * @param oper2 [state, result]
   * @since 2.92
   */
  public native static void setTaskResult(GameTask oper1, java.util.List oper2);
  /** 
   * Enable / disable reload right after magazine is empty.
<br/><br/>
   * <b>Example:</b> _vehicle enableReload false<br/>
   * @param oper1 object
   * @param oper2 enable
   * @since 2.92
   */
  public native static void enableReload(GameObject oper1, boolean oper2);
  /** 
   * Set / reset the unconscious life state of the given unit (in MP works only for a local unit).
   * @param oper1 unit
   * @param oper2 set
   * @since 5501
   */
  public native static void setUnconscious(GameObject oper1, boolean oper2);
  /** 
   * Find objects in the circle with given radius. If typeName is given, only objects of given type (or its subtype) are listed.
<br/><br/>
   * <b>Example:</b> _list = position player nearObjects 50<br/>
   * @param oper1 position
   * @param oper2 radius or [typeName, radius]
   * @since 2.92
   */
  public native static java.util.List nearObjects(Object oper1, Object oper2);
  /** 
   * Returns the unit in the vehicle turret.
   * @param oper1 vehicle
   * @param oper2 turret path
   * @since 5501
   */
  public native static GameObject turretUnit(GameObject oper1, java.util.List oper2);
  /** 
   * Counts how many units in the array are considered enemy by the given unit.
<br/><br/>
   * <b>Example:</b> player countEnemy list triggerOne<br/>
   * @param oper1 unit
   * @param oper2 array
   */
  public native static float countEnemy(GameObject oper1, java.util.List oper2);
  /** 
   * Create a new subject page in a log.
<br/><br/>
   * <b>Example:</b> _index = player createDiarySubject ["myPage", "My page"]<br/>
   * @param oper1 person
   * @param oper2 [subject, display name] or [subject, display name, picture]
   * @since 2.92
   */
  public native static float createDiarySubject(GameObject oper1, java.util.List oper2);
  /** 
   * Execute the scripted FSM. Argument is passed to the FSM as local variable _this. The FSM file is first searched in the mission folder, then in the campaign scripts folder and finally in the global scripts folder. Return the FSM handler or 0 when failed.
<br/><br/>
   * <b>Example:</b> player execFSM "test.fsm"<br/>
   * @param oper1 argument
   * @param oper2 filename
   * @since 5500
   */
  public native static float execFSM(Object oper1, String oper2);
  /** 
   * Executes the function body. Argument pars is passed as _this.
<br/><br/>
   * <b>Example:</b> [1,2] call {(_this select 0)+(_this select 1)}<br/>
<br/>
   * <b>Example result:</b> 3<br/>
   * @param oper1 pars
   * @param oper2 body
   * @since 1.85
   */
  public native static Object call(Object oper1, GameCode oper2);
  /** 
   * Selects the given weapon.
For weapon values see <cl>CfgWeapons</cl>.
<br/><br/>
   * <b>Example:</b> soldierOne selectWeapon "LAWLauncher"<br/>
   * @param oper1 unit
   * @param oper2 weapon
   * @since 1.27
   */
  public native static void selectWeapon(GameObject oper1, String oper2);
  /** 
   * Set how vehicle is locked for player. Possible values: UNLOCKED, DEFAULT or LOCKED.
<br/><br/>
   * <b>Example:</b> veh1 setVehicleLock "LOCKED"<br/>
   * @param oper1 vehicle
   * @param oper2 state
   * @since 2.33
   */
  public native static void setVehicleLock(GameObject oper1, String oper2);
  /** 
   * Sets H2 bold font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH2B "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH2B(GameControl oper1, String oper2);
  /** 
   * Sets the flag texture. If the texture is "", the flag is not drawn.
<br/><br/>
   * <b>Example:</b> setFlagTexture "usa_vlajka.pac"<br/>
   * @param oper1 flag
   * @param oper2 texture
   */
  public native static void setFlagTexture(GameObject oper1, String oper2);
  /** 
   * When used on a person, the given move is started immediately (there is no transition). Use switchmove "" to switch back to the default movement if there is no transition back, otherwise the person may be stuck.
<br/><br/>
   * <b>Example:</b> soldierOne switchMove "FXStandDip"<br/>
   * @param oper1 soldier
   * @param oper2 moveName
   */
  public native static void switchMove(GameObject oper1, String oper2);
  /** 
   * Orders the unit to target the given target (via the radio).
<br/><br/>
   * <b>Example:</b> soldierOne commandTarget player<br/>
   * @param oper1 unit
   * @param oper2 position
   */
  public native static void commandTarget(Object oper1, GameObject oper2);
  /** 
   * Defines an action performed when the user right clicks on the map and selects New Object. Set to empty for default behavior. Command receives:<br/>
<br/>
_pos <t>array</t> position
   * @param oper1 map
   * @param oper2 command
   * @since 2.92
   */
  public native static Object onShowNewObject(GameControl oper1, String oper2);
  /** 
   * Attaches an object to another object. The offset is applied to the object center unless a memory point is provided.
<br/><br/>
   * <b>Example:</b> player attachTo [car, [0,2,0],hatch1]<br/>
   * @param oper1 obj
   * @param oper2 [obj, offset, MemPoint]
   * @since 5501
   */
  public native static void attachTo(GameObject oper1, java.util.List oper2);
  /** 
   * Draw line in map.
   * @param oper1 map
   * @param oper2 [position1, position2, color]
   * @since 2.35
   */
  public native static void drawLine(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the group identity. nameFormat contains strings "%CLASS_NAME" where CLASS_NAME is name of class in CfgWorlds, parameters are names of subclasses of CLASS_NAME with the description of the name subpart.
   * @param oper1 group
   * @param oper2 [nameFomat, nameParam1, ...]
   */
  public native static void setGroupid(Object oper1, java.util.List oper2);
  /** 
   * Sets how much ammunition (compared to the current configuration of magazines, but fully loaded) the vehicle has. The value ranges from 0 to 1.
<br/><br/>
   * <b>Example:</b> player setVehicleAmmo 0<br/>
   * @param oper1 object
   * @param oper2 value
   * @since 2.32
   */
  public native static void setVehicleAmmo(GameObject oper1, float oper2);
  /** 
   * Switch FSM debug log on/off.
<br/><br/>
   * <b>Example:</b> BIS_grpMainFSM debugFSM true<br/>
   * @param oper1 FSM handle
   * @param oper2 true
   * @since 5501
   */
  public native static void debugFSM(float oper1, boolean oper2);
  /** 
   * Sets the camera position (format <ar>Position</ar>). It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetPos getPos player<br/>
   * @param oper1 camera
   * @param oper2 position
   */
  public native static void camSetPos(GameObject oper1, java.util.List oper2);
  /** 
   * Creates an empty vehicle of the given type. Pos is in format <ar>Position</ar>.
See <cl>CfgVehicles</cl> for possible type values.
<br/><br/>
   * <b>Example:</b> _tank = "M1Abrams" createVehicle getmarkerpos "tankFactory"<br/>
   * @param oper1 type
   * @param oper2 pos
   * @since 1.34
   */
  public native static GameObject createVehicle(String oper1, java.util.List oper2);
  /** 
   * Returns the additional integer value in the item with the given position of the given 2D listbox.
<br/><br/>
   * <b>Example:</b> _value = _control lnbValue [row, column]<br/>
   * @param oper1 control
   * @param oper2 [row, column]
   * @since 5501
   */
  public native static float lnbValue(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the current position in the progress bar.
<br/><br/>
   * <b>Example:</b> _control sliderSetPosition 0<br/>
   * @param oper1 control
   * @param oper2 pos
   * @since 2.92
   */
  public native static void progressSetPosition(GameControl oper1, float oper2);
  /** 
   * Remove a given event handler from the given control.
<br/><br/>
   * <b>Example:</b> _control ctrlRemoveEventHandler ["KeyDown", 0]<br/>
   * @param oper1 control
   * @param oper2 [handler name, id]
   * @since 5501
   */
  public native static void ctrlRemoveEventHandler(GameControl oper1, java.util.List oper2);
  /** 
   * The waypoint is done only when the condition is fulfilled. When the waypoint is done, the statement expression is executed.
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointStatements ["true", ""]<br/>
   * @param oper1 waypoint
   * @param oper2 [condition, statement]
   * @since 1.86
   */
  public native static void setWaypointStatements(java.util.List oper1, java.util.List oper2);
  /** 
   * Register conversation topic to given person.
   * @param oper1 person
   * @param oper2 [name, filename(, task type(, player event handler))]
   * @since 2.92
   */
  public native static void kbAddTopic(GameObject oper1, java.util.List oper2);
  /** 
   * Enable / disable given control.
<br/><br/>
   * <b>Example:</b> _control ctrlEnable false<br/>
   * @param oper1 control
   * @param oper2 enable
   * @since 2.50
   */
  public native static void ctrlEnable(GameControl oper1, boolean oper2);
  /** 
   * Set vehicle as respawnable in MP games. Delay is respawn delay, default respawnDelay from description.ext is used. Count tells how many respawns is processed (default unlimited).
<br/><br/>
   * <b>Example:</b> car respawnVehicle [5.0, 3]<br/>
   * @param oper1 vehicle
   * @param oper2 [delay = -1, count = 0]
   * @since 2.52
   */
  public native static void respawnVehicle(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the height level for the helicopter. The accepted range is from 50 to 1000.
<br/><br/>
   * <b>Example:</b> cobraOne flyInHeight 150<br/>
   * @param oper1 helicopter
   * @param oper2 height
   */
  public native static void flyInHeight(GameObject oper1, float oper2);
  /** 
   * Sets the identity of a person. Identities are defined in the descripion.ext file of the mission or campaign.
<br/><br/>
   * <b>Example:</b> setIdentity "JohnDoe"<br/>
   * @param oper1 person
   * @param oper2 identity
   */
  public native static void setIdentity(GameObject oper1, String oper2);
  /** 
   * Defines the title effect. Type can be "NONE", "OBJECT", "RES" or "TEXT". For "TEXT", the effect defines a subtype: "PLAIN", "PLAIN DOWN", "BLACK", "BLACK FADED", "BLACK OUT", "BLACK IN", "WHITE OUT" or "WHITE IN". Text is shown as text itself. For "OBJECT", text defines the shown object, a subclass of CfgTitles. For "RES", text defines a resource class, a subclass of RscTitles.
<br/><br/>
   * <b>Example:</b> trigger setTitleEffect ["TEXT", "PLAIN DOWN", "Hello world."]<br/>
   * @param oper1 trigger or waypoint
   * @param oper2 [type, effect, text]
   * @since 1.86
   */
  public native static void setTitleEffect(Object oper1, java.util.List oper2);
  /** 
   * Joins all units in the array to the given group. Avoid any radio communication related to joining.
<br/><br/>
   * <b>Example:</b> [unitOne, unitTwo] joinSilent player<br/>
   * @param oper1 unitArray
   * @param oper2 group
   */
  public native static void joinSilent(java.util.List oper1, Object oper2);
  /** 
   * Enable/disable specified post process effect, is overloaded form multiple effects.
<br/><br/>
   * <b>Example:</b> "radialBlurr" ppEffectEnable true, ["chromAberration", "radialBlurr"] ppEffectEnable true<br/>
   * @param oper1 string
   * @param oper2 bool
   * @since 2.92
   */
  public native static void ppEffectEnable(String oper1, boolean oper2);
  /** 
   * Enable/disable specified post process effect, is overloaded form multiple effects.
<br/><br/>
   * <b>Example:</b> "radialBlurr" ppEffectEnable true, ["chromAberration", "radialBlurr"] ppEffectEnable true<br/>
   * @param oper1 array
   * @param oper2 bool
   * @since 2.92
   */
  public native static void ppEffectEnable(java.util.List oper1, boolean oper2);
  /** 
   * Enable / disable post process effect
<br/><br/>
   * <b>Example:</b> hndl ppEffectEnable true<br/>
   * @param oper1 number
   * @param oper2 bool
   * @since 2.92
   */
  public native static void ppEffectEnable(float oper1, boolean oper2);
  /** 
   * When used on a person, the given move is started immediately (there is no transition).
<br/><br/>
   * <b>Example:</b> soldierOne switchGesture "Wave"<br/>
   * @param oper1 soldier
   * @param oper2 moveName
   * @since 5500
   */
  public native static void switchGesture(GameObject oper1, String oper2);
  /** 
   * Sets given event handler of given display.
<br/><br/>
   * <b>Example:</b> _control displaySetEventHandler ["KeyDown", ""]<br/>
   * @param oper1 display
   * @param oper2 [handler name, function]
   * @since 2.54
   */
  public native static void displaySetEventHandler(GameDisplay oper1, java.util.List oper2);
  /** 
   * Commits the camera changes smoothly over time. A time of zero results in an immediate change.
<br/><br/>
   * <b>Example:</b> _camera camCommit 5<br/>
   * @param oper1 camera
   * @param oper2 time
   */
  public native static void camCommit(GameObject oper1, float oper2);
  /** 
   * interpolates and sets vectors. Time has to be from &lt;0,1&gt;
   * @param oper1 soldier
   * @param oper2 [position1, position2, velocity1, velocity2, direction1, direction2, up1, up2, time]
   */
  public native static void setVelocityTransformation(GameObject oper1, java.util.List oper2);
  /** 
   * Returns the shown text in the item with the given position of the given 2D listbox.
<br/><br/>
   * <b>Example:</b> _text = _control lnbText [row, column]<br/>
   * @param oper1 control
   * @param oper2 [row, column]
   * @since 5501
   */
  public native static String lnbText(GameControl oper1, java.util.List oper2);
  /** 
   * Obsolete.
   * @param oper1 obj
   * @param oper2 allow
   */
  public native static void allowDammage(GameObject oper1, boolean oper2);
  /** 
   * Changes default cursor texture ("Track", "Move","Array", "Scroll") to custom one. To restore default texture, write empty string. If new texture does not exist, default cursor texture is used.
<br/><br/>
   * <b>Example:</b> <map_control> ctrlMapCursor ["Track","customCursor"]<br/>
   * @param oper1 control
   * @param oper2 texture names
   * @since 5501
   */
  public native static void ctrlMapCursor(GameControl oper1, java.util.List oper2);
  /** 
   * Enable/disable using IR lasers for AI
<br/><br/>
   * <b>Example:</b> groupOne enableIRLasers true<br/>
   * @param oper1 group
   * @param oper2 enable
   */
  public native static void enableIRLasers(Object oper1, boolean oper2);
  /** 
   * Sets H4 bold font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH4B "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH4B(GameControl oper1, String oper2);
  /** 
   * Loads person's identity from objects.sav file in campaign directory (from entry name).
<br/><br/>
   * <b>Example:</b> player loadIdentity "playerIdentity"<br/>
   * @param oper1 person
   * @param oper2 name
   * @since 1.75
   */
  public native static boolean loadIdentity(GameObject oper1, String oper2);
  /** 
   * Create a new AI task (subtask of parentTask). Type is name of registered task type.
   * @param oper1 teamMember
   * @param oper2 [[type] or [type, parentTask], priority, name1, value1, name2, value2, ...]
   * @since 2.62
   */
  public native static GameTask createTask(GameTeamMember oper1, java.util.List oper2);
  /** 
   * Set a new text attached to given location.
   * @param oper1 location
   * @param oper2 text
   * @since 2.90
   */
  public native static void setText(GameLocation oper1, String oper2);
  /** 
   * Set a new target side of given location.
   * @param oper1 location
   * @param oper2 side
   * @since 2.90
   */
  public native static void setSide(GameLocation oper1, GameSide oper2);
  /** 
   * Sets the group behaviour mode. Behaviour is one of: "CARELESS", "SAFE", "AWARE", "COMBAT" or "STEALTH".
<br/><br/>
   * <b>Example:</b> groupOne setBehaviour "SAFE"<br/>
   * @param oper1 group
   * @param oper2 behaviour
   */
  public native static void setBehaviour(Object oper1, String oper2);
  /** 
   * Removes group from unit's high command bar.
<br/><br/>
   * <b>Example:</b> unit HCRemoveGroup group<br/>
   * @param oper1 unit
   * @param oper2 group
   * @since 5501
   */
  public native static void hcRemoveGroup(GameObject oper1, GameGroup oper2);
  /** 
   * Creates the load overlay dialog for the specified type of overlay.
   * @param oper1 map
   * @param oper2 config
   * @since 2.92
   */
  public native static void loadOverlay(GameControl oper1, GameConfig oper2);
  /** 
   * Assigns the soldier as gunner of the given vehicle.
<br/><br/>
   * <b>Example:</b> player assignAsGunner tankOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void assignAsGunner(GameObject oper1, GameObject oper2);
  /** 
   * When used on a person, a smooth transition to the given move will be initiated.
<br/><br/>
   * <b>Example:</b> soldierOne playMove "Stand"<br/>
   * @param oper1 soldier
   * @param oper2 moveName
   */
  public native static void playMove(GameObject oper1, String oper2);
  /** 
   * Convert screen coordinates in map to world coordinates.
   * @param oper1 map
   * @param oper2 [x, y]
   * @since 2.54
   */
  public native static java.util.List posScreenToWorld(GameControl oper1, java.util.List oper2);
  /** 
   * Adds bacpacks to the cargo space. The format of backpacks is [backpacksName, count].
For backpacksName values see <cl>CfgWeapons</cl>.
<br/><br/>
   * <b>Example:</b> rearmTruckOne addBackpackCargo ["DSHKM_TK_GUE_Bag_EP1", 2]<br/>
   * @param oper1 unit
   * @param oper2 backpacks
   */
  public native static void addBackpackCargo(GameObject oper1, java.util.List oper2);
  /** 
   * Creates a JVM object representing a FSM, pass it arguments and register it for execution.
   * @param oper1 className
   * @param oper2 args
   * @since 85126
   */
  public native static void jExecFSM(String oper1, java.util.List oper2);
  /** 
   * Sets the facial animation phase (eye blinking). Blink is in the range from 0 to 1.
<br/><br/>
   * <b>Example:</b> setFaceAnimation 0.5<br/>
   * @param oper1 person
   * @param oper2 blink
   */
  public native static void setFaceanimation(GameObject oper1, float oper2);
  /** 
   * Counts how many vehicles in the array are of the given type. For types see <cl>CfgVehicles</cl>.
<br/><br/>
   * <b>Example:</b> "Tank" countType list triggerOne<br/>
   * @param oper1 typeName
   * @param oper2 array
   */
  public native static float countType(String oper1, java.util.List oper2);
  /** 
   * Moves the waypoint to a random position in a circle with the given center and radius.
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointPosition [position player, 0]<br/>
   * @param oper1 waypoint
   * @param oper2 [center, radius]
   * @since 1.86
   */
  public native static void setWaypointPosition(java.util.List oper1, java.util.List oper2);
  /** 
   * Types text to the group radio channel.
Note: this function only types text to the list, it does not broadcast the message. If you want the message to show on all computers, you have to execute it on all of them.
<br/><br/>
   * <b>Example:</b> soldierOne groupChat "Show this text"<br/>
   * @param oper1 unit
   * @param oper2 chatText
   */
  public native static void groupChat(GameObject oper1, String oper2);
  /** 
   * Types text to the global radio channel.
Note: this function only types text to the list, it does not broadcast the message. If you want the message to show on all computers, you have to execute it on all of them.
<br/><br/>
   * <b>Example:</b> soldierOne globalChat "Show this text"<br/>
   * @param oper1 unit
   * @param oper2 chatText
   */
  public native static void globalChat(GameObject oper1, String oper2);
  /** 
   * Sets H6 bold font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH6B "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH6B(GameControl oper1, String oper2);
  /** 
   * Switch between elapsed gamet time and audio time being used as animation timer. Used for audio/animation synchronization.
<br/><br/>
   * <b>Example:</b> player useAudioTimeForMoves true<br/>
   * @param oper1 soldier
   * @param oper2 toggle
   */
  public native static void useAudioTimeForMoves(GameObject oper1, boolean oper2);
  /** 
   * Causes a smooth change in the master volume. The change duration is given by time, the target volume by volume. The default master volume is 1.0.
<br/><br/>
   * <b>Example:</b> 5 fadeSound 0.1<br/>
   * @param oper1 time
   * @param oper2 volume
   */
  public native static void fadeSound(float oper1, float oper2);
  /** 
   * Select unit from unit's group.
<br/><br/>
   * <b>Example:</b> player groupSelectUnit [unit,true]<br/>
   * @param oper1 unit
   * @param oper2 [unit,bool]
   * @since 5501
   */
  public native static void groupSelectUnit(GameObject oper1, java.util.List oper2);
  /** 
   * Changes the rain density smoothly during the given time (in seconds). A time of zero means an immediate change. A rain level of zero is no rain and a rain level of one is maximum rain. Rain is not possible when overcast is smaller than 0.7.
<br/><br/>
   * <b>Example:</b> 60 setRain 1<br/>
   * @param oper1 time
   * @param oper2 rainDensity
   * @since 1.75
   */
  public native static void setRain(float oper1, float oper2);
  /** 
   * Sets the text and command for the menu item. index is index as returned from addMenuItem command. command is optional.
   * @param oper1 map
   * @param oper2 [menu item index,text,command]
   * @since 2.92
   */
  public native static void updateMenuItem(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the formation heading. The accepted heading range is from 0 to 360. The formation is facing this direction unless an enemy is seen. When the group is moving, this value is overridden by the movement direction.
<br/><br/>
   * <b>Example:</b> player setformdir 180<br/>
   * @param oper1 group
   * @param oper2 heading
   */
  public native static void setFormDir(Object oper1, float oper2);
  /** 
   * Creates a unit of the given type. The format of unitInfo is:
[pos (<ar>Position</ar>), group (<t>Group</t>), init (<t>String</t>), skill (<t>Number</t>), rank (<t>String</t>)].
Note: init, skill and rank are optional. Their default values are "", 0.5, "PRIVATE".
<br/><br/>
   * <b>Example:</b> "SoldierWB" createUnit [getMarkerPos "barracks", groupAlpha]<br/>
   * @param oper1 type
   * @param oper2 unitInfo
   * @since 1.34
   */
  public native static void createUnit(String oper1, java.util.List oper2);
  /** 
   * Creates a unit (person) of the given type (type is a name of a subclass of CfgVehicles) and makes it a member of the given group. If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The unit is placed inside a circle with this position as its center and placement as its radius. Special properties can be: "NONE" and "FORM".
<br/><br/>
   * <b>Example:</b> unit = group player createUnit ["SoldierWB", position player, [], 0, "FORM"]<br/>
   * @param oper1 group
   * @param oper2 [type, position, markers, placement, special]
   * @since 2.32
   */
  public native static GameObject createUnit(GameGroup oper1, java.util.List oper2);
  /** 
   * Remove all drawn links for the given editor object for the given editor object type. Pass an empty string as param type to remove all draw links for an object.
   * @param oper1 map
   * @param oper2 [from, param type]
   * @since 2.92
   */
  public native static void removeDrawLinks(GameControl oper1, java.util.List oper2);
  /** 
   * Types text to the command radio channel.
Note: this function only types text to the list, it does not broadcast the message. If you want the message to show on all computers, you have to execute it on all of them.
<br/><br/>
   * <b>Example:</b> soldierOne commandChat "Show this text"<br/>
   * @param oper1 unit
   * @param oper2 chatText
   * @since 5501
   */
  public native static void commandChat(Object oper1, String oper2);
  /** 
   * Resource background - the right argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The left argument define layer in which the effect is show, 0 is the back most.
The resource can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> 0 cutRsc ["binocular", "PLAIN"]<br/>
   * @param oper1 layer
   * @param oper2 effect
   * @since 5126
   */
  public native static void cutRsc(float oper1, java.util.List oper2);
  /** 
   * Sets the picture in the item with the given position of the given 2D listbox. Name is the picture name. The picture is searched in the mission directory, the dtaExt subdirectory of the campaign directory and the dtaExt directory and the data bank (or directory).
<br/><br/>
   * <b>Example:</b> _control lnbSetPicture [[row, column], "iskoda"]<br/>
   * @param oper1 control
   * @param oper2 [[row, column], name]
   * @since 5501
   */
  public native static void lnbSetPicture(GameControl oper1, java.util.List oper2);
  /** 
   * Returns the editor object scope of the specified editor object.
   * @param oper1 map
   * @param oper2 object
   * @since 2.92
   */
  public native static String getEditorObjectScope(GameControl oper1, String oper2);
  /** 
   * The editor will draw a line between the two specified editor objects. Line type can be LINE or ARROW.
   * @param oper1 map
   * @param oper2 [from, to, param type, line type, color]
   * @since 2.92
   */
  public native static void drawLink(GameControl oper1, java.util.List oper2);
  /** 
   * Lock the driver position of the vehicle.
   * @param oper1 vehicle
   * @param oper2 lock
   * @since 5501
   */
  public native static void lockDriver(GameObject oper1, boolean oper2);
  /** 
   * Sets the marker color. Color is one of: "Default", "ColorBlack", "ColorRed", "ColorRedAlpha", "ColorGreen", "ColorGreenAlpha", "ColorBlue", "ColorYellow" or "ColorWhite". The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerColorLocal "ColorBlack"<br/>
   * @param oper1 marker
   * @param oper2 color
   * @since 2.92
   */
  public native static void setMarkerColorLocal(String oper1, String oper2);
  /** 
   * Adds an row to the end od 2d listbox . It returns the index of the newly added row.
<br/><br/>
   * <b>Example:</b> _index = _control lnbAdd [t1,t2,t3,...]<br/>
   * @param oper1 control
   * @param oper2 [text1,text2,...]
   * @since 5501
   */
  public native static float lnbAddRow(GameControl oper1, java.util.List oper2);
  /** 
   * Locks the vehicle (disables mounting / dismounting) for the player.
<br/><br/>
   * <b>Example:</b> jeepOne lock true<br/>
   * @param oper1 vehicle
   * @param oper2 lock
   */
  public native static void lock(GameObject oper1, boolean oper2);
  /** 
   * Sets background color of given control. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control ctrlSetBackgroundColor [1, 0, 0, 1]<br/>
   * @param oper1 display
   * @param oper2 color
   * @since 2.50
   */
  public native static void ctrlSetBackgroundColor(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the camera field of view range for auto zooming. It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetFovRange [0.1, 0.5]<br/>
   * @param oper1 camera
   * @param oper2 fovRange
   */
  public native static void camSetFovRange(GameObject oper1, java.util.List oper2);
  /** 
   * Return units in group below passed height.
<br/><br/>
   * <b>Example:</b> group player unitsBelowHeight 50<br/>
   * @param oper1 group
   * @param oper2 height
   * @since 5501
   */
  public native static java.util.List unitsBelowHeight(GameGroup oper1, float oper2);
  /** 
   * Return units below passed height.
<br/><br/>
   * <b>Example:</b> units group player unitsBelowHeight 50<br/>
   * @param oper1 array
   * @param oper2 height
   * @since 5501
   */
  public native static java.util.List unitsBelowHeight(java.util.List oper1, float oper2);
  /** 
   * Returns all weapons of given turret. Use turret path [-1] for drivers turret.
<br/><br/>
   * <b>Example:</b> vehicle player weaponsTurret [0,0]<br/>
   * @param oper1 transport
   * @param oper2 turret path
   * @since 5501
   */
  public native static java.util.List weaponsTurret(GameObject oper1, java.util.List oper2);
  /** 
   * Sends the message to the global radio channel. The message is defined in the description.ext file or radio protocol.
<br/><br/>
   * <b>Example:</b> soldierOne globalRadio "messageOne"<br/>
   * @param oper1 unit
   * @param oper2 radioName
   */
  public native static void globalRadio(GameObject oper1, String oper2);
  /** 
   * Sets the text label of an existing marker. The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerText You are here.<br/>
   * @param oper1 name
   * @param oper2 text
   * @since 1.86
   */
  public native static void setMarkerText(String oper1, String oper2);
  /** 
   * Enables parts of the AI behaviour. Section is one of "TARGET" (enables watching assigned targets), "AUTOTARGET" (enables independed target assigning and watching of unknown targets) or "MOVE" (enables movement).
<br/><br/>
   * <b>Example:</b> soldierOne enableAI "Move"<br/>
   * @param oper1 unit
   * @param oper2 section
   * @since 2.53
   */
  public native static void enableAI(GameObject oper1, String oper2);
  /** 
   * Remove given objects from the unit's list of synchronized objects.
   * @param oper1 unit
   * @param oper2 [objects]
   * @since 5501
   */
  public native static void synchronizeObjectsRemove(GameObject oper1, java.util.List oper2);
  /** 
   * MP only: adds unit score. This is shown in the "I" MP screen as the total score.
<br/><br/>
   * <b>Example:</b> player addScore 10<br/>
   * @param oper1 unit
   * @param oper2 score
   */
  public native static void addScore(GameObject oper1, float oper2);
  /** 
   * When used on a person, a smooth transition to the given action will be initiated.
<br/><br/>
   * <b>Example:</b> soldierOne playAction "SitDown"<br/>
   * @param oper1 soldier
   * @param oper2 action
   * @since 5500
   */
  public native static void playAction(GameObject oper1, String oper2);
  /** 
   * Adds (or inserts when index is given) a new waypoint to a group. The waypoint is placed randomly in a circle with the given center and radius. The function returns a waypoint with format [group, index].
<br/><br/>
   * <b>Example:</b> grp addWaypoint [position player, 0]<br/>
   * @param oper1 group
   * @param oper2 [center, radius] or [center, radius, index]
   * @since 1.86
   */
  public native static java.util.List addWaypoint(GameGroup oper1, java.util.List oper2);
  /** 
   * Creates a camera or an actor of the given type on the given initial position (format <ar>Position</ar>). Its type is one of "CAMERA" or "SEAGULL".
<br/><br/>
   * <b>Example:</b> _camera = camCreate getPos player<br/>
   * @param oper1 type
   * @param oper2 position
   */
  public native static GameObject camCreate(String oper1, java.util.List oper2);
  /** 
   * Set the selection state of the given row of the given listbox. Listbox must support multiple selection
<br/><br/>
   * <b>Example:</b> _control lbSetSelection [0, true]<br/>
   * @param oper1 control
   * @param oper2 [index, selected]
   * @since 5501
   */
  public native static void lbSetSelected(GameControl oper1, java.util.List oper2);
  /** 
   * 
   */
  public native static void diag_objEnable(String oper1, boolean oper2);
  /** 
   * Adds the backpack to the unit. Note: unit with secondary or two-slot weapon cannot have backpack.
<br/><br/>
   * <b>Example:</b> player addBackpack "usbackpack"<br/>
   * @param oper1 unit
   * @param oper2 backpackName
   */
  public native static void addBackpack(GameObject oper1, String oper2);
  /** 
   * Orders the unit to move to the given position (format <ar>Position</ar>) (via the radio).
<br/><br/>
   * <b>Example:</b> soldierOne commandMove getMarkerPos "MarkerMoveOne"<br/>
   * @param oper1 unit
   * @param oper2 position
   */
  public native static void commandMove(Object oper1, java.util.List oper2);
  /** 
   * Returns a structured text created by the given structured or plain text by setting attributes to the given values.
<br/><br/>
   * <b>Example:</b> txt = img setAttributes ["image", "data\iSoldier.paa"]<br/>
   * @param oper1 text
   * @param oper2 [name1, value1, name2, value2, ...]
   * @since 2.01
   */
  public native static GameText setAttributes(Object oper1, java.util.List oper2);
  /** 
   * Add given objects to the unit's list of synchronized objects.
   * @param oper1 unit
   * @param oper2 [objects]
   * @since 5501
   */
  public native static void synchronizeObjectsAdd(GameObject oper1, java.util.List oper2);
  /** 
   * Orders a unit to follow the given unit (via the radio).
<br/><br/>
   * <b>Example:</b> soldierOne commandFollow player<br/>
   * @param oper1 unit
   * @param oper2 formationLeader
   */
  public native static void commandFollow(Object oper1, GameObject oper2);
  /** 
   * Sets map mode to MAP, 3D or PREVIEW.
   * @param oper1 map
   * @param oper2 mode
   * @since 2.92
   */
  public native static void setEditorMode(GameControl oper1, String oper2);
  /** 
   * Counts how many shots the unit has left for the given magazine type.
<br/><br/>
   * <b>Example:</b> player ammo "M16"<br/>
   * @param oper1 gunner or vehicle
   * @param oper2 magazine
   */
  public native static float ammo(GameObject oper1, String oper2);
  /** 
   * Attach light to given object (at given position).
   * @param oper1 light
   * @param oper2 [object, position]
   * @since 2.58
   */
  public native static void lightAttachObject(GameObject oper1, java.util.List oper2);
  /** 
   * Returns skill of given type of person (commander unit). Value of skill may vary from 0 to 1.
<br/><br/>
   * <b>Example:</b> hero skill "Endurance"<br/>
   * @param oper1 vehicle
   * @param oper2 type
   * @since 2.65
   */
  public native static float skill(GameObject oper1, String oper2);
  /** 
   * Adds the next frame to the map animation. The format of frame is [time, zoom, position], the format of position is <ar>Position2D</ar>.
<br/><br/>
   * <b>Example:</b> _map ctrlMapAnimAdd [1, 0.1, getMarkerPos "anim1"]<br/>
   * @param oper1 map
   * @param oper2 frame
   * @since 1.92
   */
  public native static void ctrlMapAnimAdd(GameControl oper1, java.util.List oper2);
  /** 
   * Convert position in the map from world coordinates to screen coordinates.
   * @param oper1 map
   * @param oper2 position
   * @since 5127
   */
  public native static java.util.List ctrlMapWorldToScreen(GameControl oper1, java.util.List oper2);
  /** 
   * Call a method name of class cls, passing arguments args.
   * @param oper1 cls
   * @param oper2 [name, args]
   * @since 84775
   */
  public native static Object jCall(GameJavaClass oper1, java.util.List oper2);
  /** 
   * 
   */
  public native static float diag_flushMem(String oper1, float oper2);
  /** 
   * The unit will play the given sound. If the unit is a person, it will also pefrorm the corresponding lipsync effect. The sound is defined in the description.ext file. If target is given, titles will be written to the conversation history.
<br/><br/>
   * <b>Example:</b> soldierOne say "speechId"<br/>
   * @param oper1 unit or [unit, target]
   * @param oper2 speechName
   */
  public native static void say(Object oper1, String oper2);
  /** 
   * The format of speechName is [sound, maxTitlesDistance] or [sound, maxTitlesDistance, speed]. The unit will play the given sound. If the unit is a person, it will also pefrorm the corresponding lipsync effect. If the camera is not withing the given range, the title is not shown. The sound is defined in the description.ext file.  If target is given, titles will be written to the conversation history.
   * @param oper1 unit or [unit, target]
   * @param oper2 speechName
   * @since 1.75
   */
  public native static void say(Object oper1, java.util.List oper2);
  /** 
   * Changes the array size. This function can be used to add or remove elements from the array.
<br/><br/>
   * <b>Example:</b> array resize 2<br/>
   * @param oper1 array
   * @param oper2 count
   * @since 1.75
   */
  public native static void resize(java.util.List oper1, float oper2);
  /** 
   * Set the radius around the waypoint where is the waypoint completed.
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointCompletionRadius 30<br/>
   * @param oper1 waypoint
   * @param oper2 radius
   * @since 5500
   */
  public native static void setWaypointCompletionRadius(java.util.List oper1, float oper2);
  /** 
   * Find the nearest enemy from the specified position.
   * @param oper1 object
   * @param oper2 position
   * @since 2.92
   */
  public native static GameObject findNearestEnemy(GameObject oper1, Object oper2);
  /** 
   * Equal to setUnitPos, for usage in formation FSM (to avoid collision with setUnitPos used by the mission).
<br/><br/>
   * <b>Example:</b> soldierOne setUnitPosWeak "Down"<br/>
   * @param oper1 unit
   * @param oper2 mode
   * @since 5117
   */
  public native static void setUnitPosWeak(GameObject oper1, String oper2);
  /** 
   * Removes all magazines of the given type from the unit. Note: you may create invalid combinations by using this function. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> player removeMagazines "M16"<br/>
   * @param oper1 unit
   * @param oper2 weaponName
   */
  public native static void removeMagazines(GameObject oper1, String oper2);
  /** 
   * When used on a person, a smooth transition to the given move will be initiated.
<br/><br/>
   * <b>Example:</b> soldierOne playGesture "Wave"<br/>
   * @param oper1 soldier
   * @param oper2 moveName
   * @since 5500
   */
  public native static void playGesture(GameObject oper1, String oper2);
  /** 
   * Attach descriptions to the simple task.
   * @param oper1 task
   * @param oper2 [description, descriptionShort, descriptionHUD]
   * @since 5500
   */
  public native static void setSimpleTaskDescription(GameTask oper1, java.util.List oper2);
  /** 
   * Sets the object heading. The accepted heading range is from 0 to 360.
<br/><br/>
   * <b>Example:</b> player setDir 180<br/>
   * @param oper1 obj
   * @param oper2 heading
   */
  public native static void setDir(GameObject oper1, float oper2);
  /** 
   * Sets the camera target. It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetTarget player<br/>
   * @param oper1 camera
   * @param oper2 target
   */
  public native static void camSetTarget(GameObject oper1, GameObject oper2);
  /** 
   * Sets the camera target to a position (format <ar>Position</ar>). It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetTarget getPos player<br/>
   * @param oper1 camera
   * @param oper2 position
   */
  public native static void camSetTarget(GameObject oper1, java.util.List oper2);
  /** 
   * Format of element is [index, value].
Changes an element of the given array. If the element does not exist, index+1 is called to create it.
<br/><br/>
   * <b>Example:</b> array set [0, "Hello"]<br/>
   * @param oper1 array
   * @param oper2 element
   * @since 1.75
   */
  public native static void set(java.util.List oper1, java.util.List oper2);
  /** 
   * Set object's direction vector. Up vector will remain unchanged.
   * @param oper1 object
   * @param oper2 [x, z, y]
   * @since 2.61
   */
  public native static void setVectorDir(GameObject oper1, java.util.List oper2);
  /** 
   * Low level command to person to move to given position.
   * @param oper1 person
   * @param oper2 position
   * @since 2.61
   */
  public native static void moveTo(GameObject oper1, java.util.List oper2);
  /** 
   * Return the proxy object associated with the given editor object.
   * @param oper1 map
   * @param oper2 object
   * @since 2.92
   */
  public native static GameObject getObjectProxy(GameControl oper1, String oper2);
  /** 
   * Counts the elements in the array for which the given condition is true.

It is calculated as follows:

1) Set the count to 0.
2) For each element in the array assign an element as _x and evaluate the condition expression. If it's true, increase the count.
<br/><br/>
   * <b>Example:</b> "_x > 2" count [0, 1, 1, 2, 3, 3]<br/>
<br/>
   * <b>Example result:</b> 2<br/>
   * @param oper1 condition
   * @param oper2 array
   */
  public native static float count(GameCode oper1, java.util.List oper2);
  /** 
   * Set group icons properties.
<br/><br/>
   * <b>Example:</b> setGroupIcon[id,"b_inf",[offsetX,ofsetY]]<br/>
   * @param oper1 group
   * @param oper2 properties
   * @since 5501
   */
  public native static void setGroupIcon(GameGroup oper1, java.util.List oper2);
  /** 
   * Set group icons parameters. [color,string,float,bool]
<br/><br/>
   * <b>Example:</b> group setGroupIconParams [[1,1,1,1],"text",scale,show]<br/>
   * @param oper1 group
   * @param oper2 properties
   * @since 5501
   */
  public native static void setGroupIconParams(GameGroup oper1, java.util.List oper2);
  /** 
   * Return object argument in mission editor.
   * @param oper1 map
   * @param oper2 [object, argument]
   * @since 2.35
   */
  public native static Object evalObjectArgument(GameControl oper1, java.util.List oper2);
  /** 
   * Create an event handler for given variable. The event handler will be executed when some client in MP exports a variable using publicVariable on all clients except the publishing one.
<br/><br/>
   * <b>Example:</b> "alarm" addPublicVariableEventHandler {_this execVM "alarm.sqf"}<br/>
   * @param oper1 name
   * @param oper2 code
   */
  public native static void addPublicVariableEventHandler(String oper1, GameCode oper2);
  /** 
   * Create single missions display as a child of given display. The mission dialog will be set to the directory given as an argument "root".
<br/><br/>
   * <b>Example:</b> _display createMissionDisplay "Test missions"<br/>
   * @param oper1 parent
   * @param oper2 root
   * @since 5140
   */
  public native static GameDisplay createMissionDisplay(GameDisplay oper1, String oper2);
  /** 
   * Create single missions display as a child of given display. The mission dialog will be set to the directory given as an argument "root". Mission space defines container's config class name.
<br/><br/>
   * <b>Example:</b> _display createMissionDisplay ["","Tutorial"]<br/>
   * @param oper1 parent
   * @param oper2 [root, missions space]
   * @since 5140
   */
  public native static GameDisplay createMissionDisplay(GameDisplay oper1, java.util.List oper2);
  /** 
   * Set a new position of given location.
   * @param oper1 location
   * @param oper2 position
   * @since 2.90
   */
  public native static void setPosition(GameLocation oper1, java.util.List oper2);
  /** 
   * Sets the marker type. Type may be any of: "Flag", "Flag1", "Dot", "Destroy", "Start", "End", "Warning", "Join", "Pickup", "Unknown", "Marker", "Arrow" or "Empty". The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerTypeLocal "Arrow"<br/>
   * @param oper1 markerName
   * @param oper2 markerType
   * @since 2.92
   */
  public native static void setMarkerTypeLocal(String oper1, String oper2);
  /** 
   * Enable/disable transport vision modes(Ti)
   * @param oper1 vehicle
   * @param oper2 enable/disable
   */
  public native static void disableTIEquipment(GameObject oper1, boolean oper2);
  /** 
   * Sends the message to the group radio channel. The message is defined in the description.ext file or radio protocol.
<br/><br/>
   * <b>Example:</b> soldierOne groupRadio "messageOne"<br/>
   * @param oper1 unit
   * @param oper2 radioName
   */
  public native static void groupRadio(GameObject oper1, String oper2);
  /** 
   * Sets the fuel amount in the cargo space of a refuelling vehicle. A fuel level of one is a full gas tank.
<br/><br/>
   * <b>Example:</b> refuelTruckOne setFuelCargo 0<br/>
   * @param oper1 vehicle
   * @param oper2 amount
   */
  public native static void setFuelCargo(GameObject oper1, float oper2);
  /** 
   * The format of handler is [type,command]. Check scripting topic Event handlers for more information. The index of the currently added handler is returned.
<br/><br/>
   * <b>Example:</b> player addMPEventHandler ["mpkilled",{_this exec "playerKilled.sqs"}]<br/>
   * @param oper1 object
   * @param oper2 handler
   * @since 1.85
   */
  public native static Object addMPEventHandler(GameObject oper1, java.util.List oper2);
  /** 
   * Add group to unit's high command bar. Array parameters are group, group name and team (teammain, teamred, teamgreen, teamblue, teamyellow) . Group is the only necessary parameter.
<br/><br/>
   * <b>Example:</b> unit hcSetGroup [group,"HQ","teamred"] 
 or 
 player hcSetGroup [group]<br/>
   * @param oper1 unit
   * @param oper2 array
   * @since 5501
   */
  public native static void hcSetGroup(GameObject oper1, java.util.List oper2);
  /** 
   * Damages / repairs the object. Damage 0 means the object is fully functional, damage 1 means it's completely destroyed / dead.
<br/><br/>
   * <b>Example:</b> player setdammage 1<br/>
   * @param oper1 obj
   * @param oper2 dammage
   */
  public native static void setDammage(GameObject oper1, float oper2);
  /** 
   * Creates a hint dialog with the given title and text.
   * @param oper1 title
   * @param oper2 text
   * @since 2.01
   */
  public native static void hintC(String oper1, String oper2);
  /** 
   * Creates a hint dialog with the given title and text.
   * @param oper1 title
   * @param oper2 text
   * @since 2.01
   */
  public native static void hintC(String oper1, GameText oper2);
  /** 
   * Creates a hint dialog with the given title and text. Texts can be plain or structured.
   * @param oper1 title
   * @param oper2 [text1, text2, ...]
   * @since 2.01
   */
  public native static void hintC(String oper1, java.util.List oper2);
  /** 
   * Marks the unit as captive. If the unit is a vehicle, the vehicle commander is marked instead.
A captive is neutral to everyone.
Note: This function does not remove the unit's weapons.
<br/><br/>
   * <b>Example:</b> setCaptive player<br/>
   * @param oper1 person
   * @param oper2 captive
   */
  public native static void setCaptive(GameObject oper1, Object oper2);
  /** 
   * The first argument can modify the condition of when the trigger is activated. The result of the activation defined by trigger activation is in variable this. Variable thisList contains all vehicles which caused the activation. Activ and desactiv expressions are launched upon trigger activation / deactivation.
<br/><br/>
   * <b>Example:</b> trigger setTriggerStatements ["this", "ok = true", "ok = false"]<br/>
   * @param oper1 trigger
   * @param oper2 [cond, activ, desactiv]
   * @since 1.86
   */
  public native static void setTriggerStatements(GameObject oper1, java.util.List oper2);
  /** 
   * Returns the object where the object should search for cover. The minDist, visibilityPosition and ignoreObject parameters are optional. visibilityPosition is used to select cover that can see a certain position. ignoreObject is an object that is ignored in visibility check.
   * @param oper1 object
   * @param oper2 [position, hidePosition, maxDist, minDist, visibilityPosition, ignoreObject]
   * @since 2.92
   */
  public native static GameObject findCover(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the group's combat mode (engagement rules). Mode can be one of: "BLUE" (Never fire), "GREEN" (Hold fire - defend only), "WHITE" (Hold fire, engage at will), "YELLOW" (Fire at will) or "RED" (Fire at will, engage at will).
<br/><br/>
   * <b>Example:</b> groupOne setCombatMode "BLUE"<br/>
   * @param oper1 group
   * @param oper2 mode
   */
  public native static void setCombatMode(Object oper1, String oper2);
  /** 
   * Sets the team member's combat mode (engagement rules). Mode can be one of: "BLUE" (Never fire), "GREEN" (Hold fire - defend only), "WHITE" (Hold fire, engage at will), "YELLOW" (Fire at will) or "RED" (Fire at will, engage at will).
<br/><br/>
   * <b>Example:</b> (teamMember agent1) setCombatMode "BLUE"<br/>
   * @param oper1 teamMember
   * @param oper2 mode
   */
  public native static void setCombatMode(GameTeamMember oper1, String oper2);
  /** 
   * Loads object's properties from objects.sav file in campaign directory (from entry name).
<br/><br/>
   * <b>Example:</b> player loadStatus "playerState"<br/>
   * @param oper1 object
   * @param oper2 name
   * @since 1.75
   */
  public native static boolean loadStatus(GameObject oper1, String oper2);
  /** 
   * Removes an icon for an editor object.
   * @param oper1 map
   * @param oper2 [object, string identifier]
   * @since 2.92
   */
  public native static void removeDrawIcon(GameControl oper1, java.util.List oper2);
  /** 
   * Assigns the vehicle (specifically its commander unit) to the given team. The possible team values are: "MAIN", "RED", "GREEN", "BLUE" and "YELLOW".
<br/><br/>
   * <b>Example:</b> soldier2 assignTeam "RED"<br/>
   * @param oper1 vehicle
   * @param oper2 team
   * @since 2.05
   */
  public native static void assignTeam(GameObject oper1, String oper2);
  /** 
   * Selects the item with the given index of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _control lbSetCurSel 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.91
   */
  public native static void lbSetCurSel(GameControl oper1, float oper2);
  /** 
   * Sets the camera field of view (zoom). It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetFov 0.1<br/>
   * @param oper1 camera
   * @param oper2 fieldOfView
   */
  public native static void camSetFov(GameObject oper1, float oper2);
  /** 
   * Sets the group speed mode. Mode may be one of: "LIMITED" (half speed), "NORMAL" (full speed, maintain formation) or "FULL" (do not wait for any other units in the formation).
<br/><br/>
   * <b>Example:</b> groupOne setSpeedMode "LIMITED"<br/>
   * @param oper1 group
   * @param oper2 mode
   */
  public native static void setSpeedMode(Object oper1, String oper2);
  /** 
   * Sets the text label attached to the trigger object. This is used for example as a radio slot label for radio activated triggers.
<br/><br/>
   * <b>Example:</b> trigger setTriggerText "Call for support"<br/>
   * @param oper1 trigger
   * @param oper2 text
   * @since 1.86
   */
  public native static void setTriggerText(GameObject oper1, String oper2);
  /** 
   * Sets the radio message (0, 0, map radio) to the given text. Use "NULL" to disable the radio slot.
<br/><br/>
   * <b>Example:</b> 0 setRadioMsg "Alpha Radio"<br/>
   * @param oper1 index
   * @param oper2 text
   */
  public native static void setRadioMsg(float oper1, String oper2);
  /** 
   * Sets the group formation. Formation is one of: "COLUMN", "STAG COLUMN", "WEDGE", "ECH LEFT", "ECH RIGHT", "VEE" or "LINE".
<br/><br/>
   * <b>Example:</b> groupOne setFormation "LINE"<br/>
   * @param oper1 group
   * @param oper2 formation
   */
  public native static void setFormation(Object oper1, String oper2);
  /** 
   * Set a new formation to given team.
<br/><br/>
   * <b>Example:</b> _team setFormation "Wedge"<br/>
   * @param oper1 team
   * @param oper2 formation
   * @since 2.90
   */
  public native static void setFormation(GameTeamMember oper1, String oper2);
  /** 
   * Removes MP event handler added by <f>addMPEventHandler</f>. Format of handler is [type,index]. Index is returned by addMPEventHandler. When any handler is removed, all handler indices higher that the deleted one should be decremented.
<br/><br/>
   * <b>Example:</b> player removeMPEventHandler ["killed",0]<br/>
   * @param oper1 object
   * @param oper2 handler
   * @since 1.85
   */
  public native static void removeMPEventHandler(GameObject oper1, java.util.List oper2);
  /** 
   * Joins the unit to the given group, if position id is available, this one is used.
<br/><br/>
   * <b>Example:</b> player joinAs [_group, 4]<br/>
   * @param oper1 unit
   * @param oper2 [group, id]
   */
  public native static void joinAs(GameObject oper1, java.util.List oper2);
  /** 
   * Remove a given event handler from the given display.
<br/><br/>
   * <b>Example:</b> _display displayRemoveEventHandler ["KeyDown", 0]<br/>
   * @param oper1 display
   * @param oper2 [handler name, id]
   * @since 5501
   */
  public native static void displayRemoveEventHandler(GameDisplay oper1, java.util.List oper2);
  /** 
   * Switches the unit behaviour when the waypoint becomes active. Possible values are: "UNCHANGED", "CARELESS", "SAFE", "AWARE", "COMBAT" and "STEALTH".
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointBehaviour "AWARE"<br/>
   * @param oper1 waypoint
   * @param oper2 mode
   * @since 1.86
   */
  public native static void setWaypointBehaviour(java.util.List oper1, String oper2);
  /** 
   * Allow/dissallow file ops (load/save etc).
   * @param oper1 map
   * @param oper2 bool
   * @since 2.92
   */
  public native static void allowFileOperations(GameControl oper1, boolean oper2);
  /** 
   * The statement is executed when the trigger or waypoint is activated and the effects are launched depending on the result. If the result is a boolean and true, the effect was launched. If the result is an object, the effect was launched if the result is the player or the player vehicle. If the result is an array, the effect was launched if the result contains the player or the player vehicle.
<br/><br/>
   * <b>Example:</b> trigger setEffectCondition "thisList"<br/>
   * @param oper1 trigger or waypoint
   * @param oper2 statement
   * @since 1.86
   */
  public native static void setEffectCondition(Object oper1, String oper2);
  /** 
   * Send a result of the task to the task sender.
   * @param oper1 task
   * @param oper2 [state, result, sentence]
   * @since 2.92
   */
  public native static void sendTaskResult(GameTask oper1, java.util.List oper2);
  /** 
   * Create a new simple task (subtask of parentTask).
   * @param oper1 person
   * @param oper2 [name] or [name, parentTask]
   * @since 5500
   */
  public native static GameTask createSimpleTask(GameObject oper1, java.util.List oper2);
  /** 
   * Sets whether or not the object is visible even if the tree is collapsed.
   * @param oper1 map
   * @param oper2 [object, visible if tree collapsed]
   * @since 2.92
   */
  public native static void setVisibleIfTreeCollapsed(GameControl oper1, java.util.List oper2);
  /** 
   * Sets P bold font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontPB "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontPB(GameControl oper1, String oper2);
  /** 
   * Sets given event handler of given editor.
<br/><br/>
   * <b>Example:</b> _map editorSetEventHandler ["SelectObject", ""]<br/>
   * @param oper1 map
   * @param oper2 [handler name, function]
   * @since 2.92
   */
  public native static void editorSetEventHandler(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the text color of the item with the given index of the given listbox or combobox. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control lbSetColor [0, [0, 1, 0, 0.5]]<br/>
   * @param oper1 control
   * @param oper2 [index, color]
   * @since 2.91
   */
  public native static void lbSetColor(GameControl oper1, java.util.List oper2);
  /** 
   * Create a new record in a log.
<br/><br/>
   * <b>Example:</b> _record = player createDiaryRecord ["diary", "Kill all enemies."]<br/>
   * @param oper1 person
   * @param oper2 [subject, text(, task(, state))] or [subject, [title, text](, task(, state))]
   * @since 2.53
   */
  public native static GameDiaryRecord createDiaryRecord(GameObject oper1, java.util.List oper2);
  /** 
   * Set a new importance of given location.
   * @param oper1 location
   * @param oper2 importance
   * @since 2.90
   */
  public native static void setImportance(GameLocation oper1, float oper2);
  /** 
   * Get group icon properties.
<br/><br/>
   * <b>Example:</b> group getGroupIcon id<br/>
   * @param oper1 group
   * @param oper2 ID
   * @since 5501
   */
  public native static java.util.List getGroupIcon(GameGroup oper1, float oper2);
  /** 
   * Set variable to given value in the given namespace.
   * @param oper1 namespace
   * @param oper2 [name, value]
   * @since 5501
   */
  public native static void setVariable(GameNamespace oper1, java.util.List oper2);
  /** 
   * Set variable to given value in the variable space of given map.
   * @param oper1 map
   * @param oper2 [name, value]
   * @since 2.92
   */
  public native static void setVariable(GameControl oper1, java.util.List oper2);
  /** 
   * Set variable to given value in the variable space of given object. If public is true then the value is broadcast to all computers.
   * @param oper1 object
   * @param oper2 [name, value, public]
   * @since 2.92
   */
  public native static void setVariable(GameObject oper1, java.util.List oper2);
  /** 
   * Set variable to given value in the variable space of given group. If public is true then the value is broadcast to all computers.
   * @param oper1 group
   * @param oper2 [name, value]
   * @since 5501
   */
  public native static void setVariable(GameGroup oper1, java.util.List oper2);
  /** 
   * Set variable to given value in the variable space of given team member. If public is true then the value is broadcast to all computers.
<br/><br/>
   * <b>Example:</b> team1 setVariable ["owner",player,true]<br/>
   * @param oper1 teamMember
   * @param oper2 [name, value, public]
   * @since 2.92
   */
  public native static void setVariable(GameTeamMember oper1, java.util.List oper2);
  /** 
   * Set variable to given value in the variable space of given task.
   * @param oper1 task
   * @param oper2 [name, value]
   * @since 2.92
   */
  public native static void setVariable(GameTask oper1, java.util.List oper2);
  /** 
   * Set variable to given value in the variable space of given location.
   * @param oper1 location
   * @param oper2 [name, value]
   * @since 2.92
   */
  public native static void setVariable(GameLocation oper1, java.util.List oper2);
  /** 
   * Orders the unit to watch the given position (format <ar>Position</ar>) (silently).
<br/><br/>
   * <b>Example:</b> soldierOne doWatch getMarkerPos "MarkerMoveOne"<br/>
   * @param oper1 unit
   * @param oper2 position
   */
  public native static void doWatch(Object oper1, java.util.List oper2);
  /** 
   * Orders the unit to watch the given target (silently).
<br/><br/>
   * <b>Example:</b> soldierOne doWatch player<br/>
   * @param oper1 unit
   * @param oper2 target
   */
  public native static void doWatch(Object oper1, GameObject oper2);
  /** 
   * Sets the name of the variable which contains a reference to this object. It is necessary in MP to change the variable content after a respawn.
<br/><br/>
   * <b>Example:</b> player setVehicleVarName "aP"<br/>
   * @param oper1 object
   * @param oper2 name
   * @since 2.32
   */
  public native static void setVehicleVarName(GameObject oper1, String oper2);
  /** 
   * Prepares the camera target. See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareTarget player<br/>
   * @param oper1 camera
   * @param oper2 target
   * @since 2.95
   */
  public native static void camPrepareTarget(GameObject oper1, GameObject oper2);
  /** 
   * Prepares the camera target to a position (format <ar>Position</ar>). See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareTarget getPos player<br/>
   * @param oper1 camera
   * @param oper2 position
   * @since 2.95
   */
  public native static void camPrepareTarget(GameObject oper1, java.util.List oper2);
  /** 
   * Shifts an editor object to the end of the objects array. This means that the object will be drawn last (after all other objects).
   * @param oper1 map
   * @param oper2 object
   * @since 2.92
   */
  public native static void moveObjectToEnd(GameControl oper1, String oper2);
  /** 
   * Defines an action performed when the user clicks on the map. Command receives:<br/>
<br/>
_pos <t>array</t> position<br/>
_units <t>array</t> selected units<br/>
_shift,_alt <t>bool</t> key state<br/>
_this <t>any</t> parameters passed to this function<br/>
<br/>If the click is processed, command should return true.
<br/><br/>
   * <b>Example:</b> "SoldierEG" onMapSingleClick "_this createUnit [_pos, group player]"<br/>
   * @param oper1 parameters
   * @param oper2 command
   * @since 5500
   */
  public native static void onMapSingleClick(Object oper1, Object oper2);
  /** 
   * Set diffuse color of light.
   * @param oper1 light
   * @param oper2 [r, g, b]
   * @since 2.58
   */
  public native static void setLightColor(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the additional text (invisible) in the item with the given position of the given 2D listbox to the given data.
<br/><br/>
   * <b>Example:</b> _control lnbSetData [[row, column], "#1"]<br/>
   * @param oper1 control
   * @param oper2 [[row, column], data]
   * @since 5501
   */
  public native static void lnbSetData(GameControl oper1, java.util.List oper2);
  /** 
   * Executes the given command for each agent member of the team (recursively).
   * @param oper1 command
   * @param oper2 team
   * @since 2.92
   */
  public native static void forEachMemberAgent(GameCode oper1, GameTeamMember oper2);
  /** 
   * Returns the arctangens of x/y. The returned value is in degrees, in the range from -180 to +180, using the signs of both parameters to determine the quadrant of the returned value.
<br/><br/>
   * <b>Example:</b> 5 atan2 3<br/>
<br/>
   * <b>Example result:</b> 59.0362<br/>
   * @param oper1 x
   * @param oper2 y
   */
  public native static float atan2(float oper1, float oper2);
  /** 
   * Sets the object position. The pos array uses the <ar>PositionASL</ar> format.
<br/><br/>
   * <b>Example:</b> player setPosASL [getposASL player select 0, getposASL player select 1 + 10, getPosASL select 2]<br/>
   * @param oper1 obj
   * @param oper2 pos
   * @since 2.53
   */
  public native static void setPosASL(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the waypoint position. Waypoint uses format <ar>Waypoint</ar>. Position uses format <ar>Position2D</ar>.
<br/><br/>
   * <b>Example:</b> [groupOne, 1] setWPPos getMarkerPos "MarkerOne"<br/>
   * @param oper1 wapoint
   * @param oper2 position
   * @since 1.21
   */
  public native static void setWPPos(java.util.List oper1, java.util.List oper2);
  /** 
   * Sends a simple command to the vehicle's driver / gunner.
<br/><br/>
   * <b>Example:</b> vehicle player sendSimpleCommand "STOP"<br/>
   * @param oper1 object
   * @param oper2 command
   * @since 2.26
   */
  public native static void sendSimpleCommand(GameObject oper1, String oper2);
  /** 
   * Sets H1 font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH1 "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH1(GameControl oper1, String oper2);
  /** 
   * Moves the soldier into the vehicle's gunner position. (Immediately, without animation).
<br/><br/>
   * <b>Example:</b> soldierOne moveInGunner jeepOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void moveInGunner(GameObject oper1, GameObject oper2);
  /** 
   * Sets the text that will be shown in given control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetText "Hello, world."<br/>
   * @param oper1 control
   * @param oper2 text
   * @since 2.50
   */
  public native static void ctrlSetText(GameControl oper1, String oper2);
  /** 
   * Find object nearest to given position with given type.
<br/><br/>
   * <b>Example:</b> obj = position player nearestObject "Building"<br/>
   * @param oper1 position
   * @param oper2 type
   * @since 2.01
   */
  public native static GameObject nearestObject(java.util.List oper1, String oper2);
  /** 
   * Find object nearest to given position with given Visitor id.
<br/><br/>
   * <b>Example:</b> obj = position player nearestObject 1234<br/>
   * @param oper1 position
   * @param oper2 id
   * @since 2.01
   */
  public native static GameObject nearestObject(java.util.List oper1, float oper2);
  /** 
   * Removes all magazines of the given type from the unit. Use turret path [-1] for driver's turret. Note: you may create invalid combinations by using this function. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> vehicle player removeMagazines ["60rnd_cmflaremagazine",,[-1]]<br/>
   * @param oper1 transport
   * @param oper2 [weaponName, turret path]
   */
  public native static void removeMagazinesTurret(GameObject oper1, java.util.List oper2);
  /** 
   * The format of handler is [type,command]. Check scripting topic Event handlers for more information. The index of the currently added handler is returned.
<br/><br/>
   * <b>Example:</b> player addEventHandler ["killed",{_this exec "playerKilled.sqs"}]<br/>
   * @param oper1 object
   * @param oper2 handler
   * @since 1.85
   */
  public native static Object addEventHandler(GameObject oper1, java.util.List oper2);
  /** 
   * Preload the scene for he prepared camera. Time gives timeout, zero means no (infinite) timeout.
<br/><br/>
   * <b>Example:</b> _camera camCommit 5<br/>
   * @param oper1 camera
   * @param oper2 time
   * @since 2.95
   */
  public native static void camPreload(GameObject oper1, float oper2);
  /** 
   * Sets H2 font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH2 "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH2(GameControl oper1, String oper2);
  /** 
   * Sets the orientation of the marker. Angle is in degrees. The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerDir 90<br/>
   * @param oper1 name
   * @param oper2 angle
   * @since 1.86
   */
  public native static void setMarkerDir(String oper1, float oper2);
  /** 
   * The group combat mode is switched when the waypoint becomes active. Possible values are: "NO CHANGE", "BLUE", "GREEN", "WHITE", "YELLOW" and "RED".
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointCombatMode "RED"<br/>
   * @param oper1 waypoint
   * @param oper2 mode
   * @since 1.86
   */
  public native static void setWaypointCombatMode(java.util.List oper1, String oper2);
  /** 
   * Sets text color of given control when control is selected. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control ctrlSetActiveColor [1, 0, 0, 1]<br/>
   * @param oper1 display
   * @param oper2 color
   * @since 2.50
   */
  public native static void ctrlSetActiveColor(GameControl oper1, java.util.List oper2);
  /** 
   * Checks whether the unit knows about the target (and how much). If the unit is a vehicle, the vehicle commander is considered instead.
<br/><br/>
   * <b>Example:</b> soldierOne knowsAbout jeepOne<br/>
   * @param oper1 unit
   * @param oper2 target
   */
  public native static float knowsAbout(Object oper1, GameObject oper2);
  /** 
   * Checks whether the side knows about the target (and how much).
<br/><br/>
   * <b>Example:</b> east knowsAbout jeepOne<br/>
   * @param oper1 side
   * @param oper2 target
   * @since 5501
   */
  public native static float knowsAbout(GameSide oper1, GameObject oper2);
  /** 
   * Removes row with the given index from the given  listbox or combobox.
<br/><br/>
   * <b>Example:</b> _control lnbDelete 0<br/>
   * @param oper1 control
   * @param oper2 row
   * @since 5501
   */
  public native static void lnbDeleteRow(GameControl oper1, float oper2);
  /** 
   * Sets the text color of the item with the given position of the given 2D listbox . Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control lnbSetColor [[row, column], [0, 1, 0, 0.5]]<br/>
   * @param oper1 control
   * @param oper2 [[row, column], color]
   * @since 5501
   */
  public native static void lnbSetColor(GameControl oper1, java.util.List oper2);
  /** 
   * 
   */
  public native static void diag_engEnable(String oper1, boolean oper2);
  /** 
   * Control what the unit is looking at (target or position) (format <ar>Position</ar>)
<br/><br/>
   * <b>Example:</b> someSoldier lookAt otherSoldier; otherSoldier lookAt getMarkerPos "markerOne"<br/>
   * @param oper1 unit(s)
   * @param oper2 position
   * @since 2.40
   */
  public native static void lookAt(Object oper1, Object oper2);
  /** 
   * Register target list knowledge base database to given person.
<br/><br/>
   * <b>Example:</b> _unit kbAddDatabase "chat.txt"<br/>
   * @param oper1 person
   * @param oper2 filename
   * @since 2.45
   */
  public native static boolean kbAddDatabaseTargets(GameObject oper1, String oper2);
  /** 
   * Force player to walk.
<br/><br/>
   * <b>Example:</b> player forceWalk true<br/>
   * @param oper1 player
   * @param oper2 force walk
   * @since 5501
   */
  public native static void forceWalk(GameObject oper1, boolean oper2);
  /** 
   * Sets H3 font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH3 "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH3(GameControl oper1, String oper2);
  /** 
   * Orders a unit to process command defined by FSM file (via the radio).
<br/><br/>
   * <b>Example:</b> soldierOne commandFSM ["move.fsm", position player, player]<br/>
   * @param oper1 unit(s)
   * @param oper2 [fsm name, position, target]
   * @since 2.53
   */
  public native static void commandFSM(Object oper1, java.util.List oper2);
  /** 
   * Lock the cargo position of the vehicle.
   * @param oper1 vehicle
   * @param oper2 [cargo index, lock]
   * @since 5501
   */
  public native static void lockCargo(GameObject oper1, java.util.List oper2);
  /** 
   * Lock the all cargo positions of the vehicle.
   * @param oper1 vehicle
   * @param oper2 lock
   * @since 5501
   */
  public native static void lockCargo(GameObject oper1, boolean oper2);
  /** 
   * Sets tooltip background color of given control. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control ctrlSetTooltipColorShade [1, 0, 0, 1]<br/>
   * @param oper1 display
   * @param oper2 color
   * @since 2.92
   */
  public native static void ctrlSetTooltipColorShade(GameControl oper1, java.util.List oper2);
  /** 
   * Switches the group formation when the waypoint becomes active. Possible values are: "NO CHANGE", "COLUMN", "STAG COLUMN", "WEDGE", "ECH LEFT", "ECH RIGHT", "VEE" and "LINE".
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointFormation "LINE"<br/>
   * @param oper1 waypoint
   * @param oper2 formation
   * @since 1.86
   */
  public native static void setWaypointFormation(java.util.List oper1, String oper2);
  /** 
   * Sets the speed (a click on the arrow results in a move per line. A click on the scale outside the thumb results in a move per page) of the given slider.
<br/><br/>
   * <b>Example:</b> _control sliderSetSpeed [0.5, 2.0]<br/>
   * @param oper1 control
   * @param oper2 [line, page]
   * @since 2.92
   */
  public native static void sliderSetSpeed(GameControl oper1, java.util.List oper2);
  /** 
   * Set camera interest for given entity.
<br/><br/>
   * <b>Example:</b> _soldier setCameraInterest 50<br/>
   * @param oper1 entity
   * @param oper2 interest
   * @since 2.57
   */
  public native static void setCameraInterest(GameObject oper1, float oper2);
  /** 
   * Converts position from object model space to world space.
   * @param oper1 object
   * @param oper2 modelPos
   * @since 2.92
   */
  public native static java.util.List modelToWorld(GameObject oper1, java.util.List oper2);
  /** 
   * Moves the marker. The format of pos is <ar>Position2D</ar>. The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerPosLocal getMarkerPos "MarkerTwo"<br/>
   * @param oper1 markerName
   * @param oper2 pos
   * @since 2.92
   */
  public native static void setMarkerPosLocal(String oper1, java.util.List oper2);
  /** 
   * When used on a person, a smooth transition to the given move will be initiated, but all previous playAction are discarded.
<br/><br/>
   * <b>Example:</b> soldierOne playMoveNow "Stand"<br/>
   * @param oper1 soldier
   * @param oper2 moveName
   * @since 5501
   */
  public native static void playMoveNow(GameObject oper1, String oper2);
  /** 
   * Checks whether x is equal to any element in the array.
<br/><br/>
   * <b>Example:</b> 1 in [0, 1, 2]<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 x
   * @param oper2 array
   */
  public native static boolean in(Object oper1, java.util.List oper2);
  /** 
   * Checks whether the soldier is mounted in the vehicle.
<br/><br/>
   * <b>Example:</b> player in jeepOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static boolean in(GameObject oper1, GameObject oper2);
  /** 
   * Check if the position is inside area defined by the location.
   * @param oper1 position
   * @param oper2 location
   * @since 2.92
   */
  public native static boolean in(java.util.List oper1, GameLocation oper2);
  /** 
   * Assigns the soldier as commander of the given vehicle.
<br/><br/>
   * <b>Example:</b> player assignAsCommander tankOne<br/>
   * @param oper1 soldier
   * @param oper2 vehicle
   */
  public native static void assignAsCommander(GameObject oper1, GameObject oper2);
  /** 
   * Counts how many units in the array are considered friendly by the given unit.
<br/><br/>
   * <b>Example:</b> player countFriendly list triggerOne<br/>
   * @param oper1 unit
   * @param oper2 array
   */
  public native static float countFriendly(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the marker color. Color is one of: "Default", "ColorBlack", "ColorRed", "ColorRedAlpha", "ColorGreen", "ColorGreenAlpha", "ColorBlue", "ColorYellow" or "ColorWhite". The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerColor "ColorBlack"<br/>
   * @param oper1 marker
   * @param oper2 color
   * @since 1.21
   */
  public native static void setMarkerColor(String oper1, String oper2);
  /** 
   * Adds a magazine to the unit. Note: you may create invalid combinations by using this function, for example by adding 20 grenades. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> player addMagazine "M16"<br/>
   * @param oper1 unit
   * @param oper2 weaponName
   */
  public native static void addMagazine(GameObject oper1, String oper2);
  /** 
   * Orders the unit to target the given target (silently).
<br/><br/>
   * <b>Example:</b> soldierOne doTarget player<br/>
   * @param oper1 unit
   * @param oper2 position
   */
  public native static void doTarget(Object oper1, GameObject oper2);
  /** 
   * Types text to the side radio channel.
Note: this function only types text to the list, it does not broadcast the message. If you want the message to show on all computers, you have to execute it on all of them.
<br/><br/>
   * <b>Example:</b> soldierOne sideChat "Show this text"<br/>
   * @param oper1 unit
   * @param oper2 chatText
   */
  public native static void sideChat(Object oper1, String oper2);
  /** 
   * Sets H4 font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH4 "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH4(GameControl oper1, String oper2);
  /** 
   * Returns how good is weapon aimed to target.
<br/><br/>
   * <b>Example:</b> bool = vehicle AimedAtTarget [target]; bool = vehicle AimedAtTarget [target,1]<br/>
   * @param oper1 vehicle
   * @param oper2 [target, weapon]
   * @since 5501
   */
  public native static float aimedAtTarget(GameObject oper1, java.util.List oper2);
  /** 
   * Reveals the unit to the group. It does not matter whether the group can know about the unit or not.
<br/><br/>
   * <b>Example:</b> soldierOne reveal soldierTwo<br/>
   * @param oper1 group
   * @param oper2 unit
   */
  public native static void reveal(Object oper1, GameObject oper2);
  /** 
   * Switch the group to process the given waypoint.
   * @param oper1 group
   * @param oper2 waypoint
   * @since 5129
   */
  public native static void setCurrentWaypoint(GameGroup oper1, java.util.List oper2);
  /** 
   * Add an event handler to the given control. Returns id of the handler or -1 when failed.
<br/><br/>
   * <b>Example:</b> _id = _control ctrlAddEventHandler ["KeyDown", ""]<br/>
   * @param oper1 control
   * @param oper2 [handler name, function]
   * @since 5501
   */
  public native static float ctrlAddEventHandler(GameControl oper1, java.util.List oper2);
  /** 
   * Delete (unregister) resources of the team member.
<br/><br/>
   * <b>Example:</b> teamMember _agent deleteResources ["Legs"]<br/>
   * @param oper1 teamMember
   * @param oper2 [resource1, resource2, ...]
   * @since 2.90
   */
  public native static void deleteResources(GameTeamMember oper1, java.util.List oper2);
  /** 
   * Orders a unit to commence firing on the given target (via the radio). If the target is objNull, the unit is ordered to commence firing on its current target (set using doTarget or commandTarget).
<br/><br/>
   * <b>Example:</b> soldierOne commandFire player<br/>
   * @param oper1 unit
   * @param oper2 target
   */
  public native static void commandFire(Object oper1, GameObject oper2);
  /** 
   * Draw ellipse in map.
   * @param oper1 map
   * @param oper2 [center, a, b, angle, color, fill]
   * @since 2.35
   */
  public native static void drawEllipse(GameControl oper1, java.util.List oper2);
  /** 
   * Sets H5 font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH5 "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH5(GameControl oper1, String oper2);
  /** 
   * Adds weapons to the weapon cargo space. This is used for infantry weapons. The format of weapons is [weaponName, count].
For weaponName values see <cl>CfgWeapons</cl>.
<br/><br/>
   * <b>Example:</b> rearmTruckOne addWeaponCargo ["M16", 10]<br/>
   * @param oper1 unit
   * @param oper2 weapons
   */
  public native static void addWeaponCargo(GameObject oper1, java.util.List oper2);
  /** 
   * Return the value of variable in the variable space of given FSM. The FSM handle is the number returned by the execFSM command.
   * @param oper1 FSM handle
   * @param oper2 name
   * @since 5501
   */
  public native static Object getFSMVariable(float oper1, String oper2);
  /** 
   * Disables parts of the AI behaviour to get more control over a unit. Section is one of "TARGET" (disables watching assigned targets), "AUTOTARGET" (disables independed target assigning and watching of unknown targets) or "MOVE" (disables movement).
<br/><br/>
   * <b>Example:</b> soldierOne disableAI "Move"<br/>
   * @param oper1 unit
   * @param oper2 section
   */
  public native static void disableAI(GameObject oper1, String oper2);
  /** 
   * Sets the additional text (invisible) in the item with the given index of the given listbox or combobox to the given data.
<br/><br/>
   * <b>Example:</b> _control lbSetData [1, "#1"]<br/>
   * @param oper1 control
   * @param oper2 [index, data]
   * @since 2.91
   */
  public native static void lbSetData(GameControl oper1, java.util.List oper2);
  /** 
   * Creates a new JVM UI panel based on given class. The second parameter defines the position and size of the panel.
   * @param oper1 className
   * @param oper2 [x, y, width, height]
   * @since 86290
   */
  public native static void jAddPanel(String oper1, java.util.List oper2);
  /** 
   * Saves person's identity to objects.sav file in campaign directory as entry name.
<br/><br/>
   * <b>Example:</b> player saveIdentity "playerIdentity"<br/>
   * @param oper1 person
   * @param oper2 name
   * @since 1.75
   */
  public native static boolean saveIdentity(GameObject oper1, String oper2);
  /** 
   * Set a new type of given location.
   * @param oper1 location
   * @param oper2 type
   * @since 2.90
   */
  public native static void setType(GameLocation oper1, String oper2);
  /** 
   * Set which global variable will contain given location.
   * @param oper1 location
   * @param oper2 name
   * @since 2.90
   */
  public native static void setName(GameLocation oper1, String oper2);
  /** 
   * Sets tooltip border color of given control. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control ctrlSetTooltipColorBox [1, 0, 0, 1]<br/>
   * @param oper1 display
   * @param oper2 color
   * @since 2.92
   */
  public native static void ctrlSetTooltipColorBox(GameControl oper1, java.util.List oper2);
  /** 
   * Executes a command on the given camera / actor object. The "manual on" and "manual off" commands are recognized for all types. For the "camera" type, the following commands can be used: "inertia on" and "inertia off". For the "seagull" type it's one of: "landed" and "airborne".
<br/><br/>
   * <b>Example:</b> _camera camCommand "Manual on"<br/>
   * @param oper1 camera
   * @param oper2 command
   */
  public native static void camCommand(GameObject oper1, String oper2);
  /** 
   * Damage / repair part of object. Damage 0 means fully functional, damage 1 means completely destroyed / dead.
<br/><br/>
   * <b>Example:</b> vehicle player setHit ["engine", 1]<br/>
   * @param oper1 object
   * @param oper2 [part, damage]
   * @since 2.30
   */
  public native static void setHit(GameObject oper1, java.util.List oper2);
  /** 
   * Sets H5 font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH6 "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH6(GameControl oper1, String oper2);
  /** 
   * Get the speed for the given speed mode. SpeedMode can be: "AUTO","SLOW","NORMAL","FAST".
   * @param oper1 object
   * @param oper2 speedMode
   * @since 2.92
   */
  public native static float getSpeed(GameObject oper1, String oper2);
  /** 
   * Removes the magazine from the unit. Note: you may create invalid combinations by using this function. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> player removeMagazine "M16"<br/>
   * @param oper1 unit
   * @param oper2 weaponName
   */
  public native static void removeMagazine(GameObject oper1, Object oper2);
  /** 
   * Check whether cargo position of the vehicle is locked.
   * @param oper1 vehicle
   * @param oper2 cargo index
   * @since 5501
   */
  public native static boolean lockedCargo(GameObject oper1, float oper2);
  /** 
   * Enable / disable continuous centering the map on the camera position
   * @param oper1 control
   * @param oper2 enable
   * @since 5501
   */
  public native static void mapCenterOnCamera(GameControl oper1, boolean oper2);
  /** 
   * Remove icon with given ID from group.
<br/><br/>
   * <b>Example:</b> group removeGroupIcon id<br/>
   * @param oper1 group
   * @param oper2 icon ID
   * @since 5501
   */
  public native static void removeGroupIcon(GameGroup oper1, float oper2);
  /** 
   * Selects the fill texture for the marker ("RECTANGLE" or "ELLIPSE"). Brush is the name of the subclass in CfgMarkerBrushes. The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerBrushLocal "DiagGrid"<br/>
   * @param oper1 name
   * @param oper2 brush
   * @since 2.92
   */
  public native static void setMarkerBrushLocal(String oper1, String oper2);
  /** 
   * Removes the weapon from the unit. Note: you may create invalid combinations by using this function. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> player removeWeapon "M16"<br/>
   * @param oper1 unit
   * @param oper2 weaponName
   */
  public native static void removeWeapon(GameObject oper1, String oper2);
  /** 
   * Returns the additional integer value in the item with the given index of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _value = _control lbValue 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.91
   */
  public native static float lbValue(GameControl oper1, float oper2);
  /** 
   * Returns the text color of the item with the given position of the given 2D listbox.
<br/><br/>
   * <b>Example:</b> _color = _control lnbColor [row, column]<br/>
   * @param oper1 control
   * @param oper2 [row, column]
   * @since 5501
   */
  public native static java.util.List lnbColor(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the current thumb position of the given slider.
<br/><br/>
   * <b>Example:</b> _control sliderSetPosition 0<br/>
   * @param oper1 control
   * @param oper2 pos
   * @since 2.92
   */
  public native static void sliderSetPosition(GameControl oper1, float oper2);
  /** 
   * Sets the limits of the slider with id idc of the given slider.
<br/><br/>
   * <b>Example:</b> _control sliderSetRange [0, 10]<br/>
   * @param oper1 control
   * @param oper2 [min, max]
   * @since 2.92
   */
  public native static void sliderSetRange(GameControl oper1, java.util.List oper2);
  /** 
   * Add (register) resources to the team member.
<br/><br/>
   * <b>Example:</b> teamMember _agent addResources ["Arms", "Legs"]<br/>
   * @param oper1 teamMember
   * @param oper2 [resource1, resource2, ...]
   * @since 2.90
   */
  public native static void addResources(GameTeamMember oper1, java.util.List oper2);
  /** 
   * Adds the weapon to the unit. Note: you may create invalid combinations by using this function, for example by adding two rifles. When doing so, application behaviour is undefined.
<br/><br/>
   * <b>Example:</b> player addWeapon "AK74"<br/>
   * @param oper1 unit
   * @param oper2 weaponName
   */
  public native static void addWeapon(GameObject oper1, String oper2);
  /** 
   * Prepares the camera position (format <ar>Position</ar>). See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPreparePos getPos player<br/>
   * @param oper1 camera
   * @param oper2 position
   * @since 2.95
   */
  public native static void camPreparePos(GameObject oper1, java.util.List oper2);
  /** 
   * Set the current task of the formation member.
   * @param oper1 object
   * @param oper2 task
   * @since 2.92
   */
  public native static void setFormationTask(GameObject oper1, String oper2);
  /** 
   * Controls fireplace buring. Set to true to inflame the fireplace.
<br/><br/>
   * <b>Example:</b> fireplaceOne inflame true<br/>
   * @param oper1 fireplace
   * @param oper2 burn
   */
  public native static void inflame(GameObject oper1, boolean oper2);
  /** 
   * Sets construction camera parameters.
<br/><br/>
   * <b>Example:</b> _camera camConstuctionSetParams [getpos player,50,20]<br/>
   * @param oper1 camera
   * @param oper2 [[x,y,z],radius, max above land]
   * @since 5501
   */
  public native static void camConstuctionSetParams(GameObject oper1, java.util.List oper2);
  /** 
   * When used on a person, the given action is started immediately (there is no transition). Use switchmove "" to switch back to the default movement if there is no transition back, otherwise the person may be stuck.
<br/><br/>
   * <b>Example:</b> soldierOne switchAction "SitDown"<br/>
   * @param oper1 soldier
   * @param oper2 action
   * @since 5500
   */
  public native static void switchAction(GameObject oper1, String oper2);
  /** 
   * Set owner of the airport
<br/><br/>
   * <b>Example:</b> 0 setAirportSide east<br/>
   * @param oper1 airportId
   * @param oper2 side
   * @since 2.92
   */
  public native static void setAirportSide(float oper1, GameSide oper2);
  /** 
   * Force person to beep using VoN system.
   * @param oper1 person
   * @param oper2 [channel, 2D_3D, frequency, seconds]
   * @since 5214
   */
  public native static void vonSay(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the group courage. The less courage, the sooner the group will start fleeing. 0 means maximum courage, while 1 means always fleeing.
<br/><br/>
   * <b>Example:</b> soldierOne allowFleeing 0<br/>
   * @param oper1 unit
   * @param oper2 courage
   */
  public native static void allowFleeing(Object oper1, float oper2);
  /** 
   * Makes the unit to apply suppressive fire on known enemies.
   * @param oper1 unit
   * @param oper2 timeToSuppress
   * @since 59874
   */
  public native static void suppressFor(GameObject oper1, float oper2);
  /** 
   * Add an object to the editor and assign arguments. Create script is called with _new equal to true. Returns the ID of the new EditorObject. Subtype class is optional.
   * @param oper1 map
   * @param oper2 [type, [name1, value1, ...], subtype class]
   * @since 2.92
   */
  public native static String addEditorObject(GameControl oper1, java.util.List oper2);
  /** 
   * Sets wanted position and size for control animation. Width and height are optional.
<br/><br/>
   * <b>Example:</b> _control ctrlSetPosition [0.5, 0.5]<br/>
   * @param oper1 control
   * @param oper2 [x, y, w, h]
   * @since 2.50
   */
  public native static void ctrlSetPosition(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the condition determining when the waypoint is shown. Possible values are: "NEVER", "EASY" and "ALWAYS".
<br/><br/>
   * <b>Example:</b> [grp, 2] showWaypoint "ALWAYS"<br/>
   * @param oper1 waypoint
   * @param oper2 show
   * @since 1.86
   */
  public native static void showWaypoint(java.util.List oper1, String oper2);
  /** 
   * Sends the message to the command radio channel. The message is defined in the description.ext file or radio protocol.
<br/><br/>
   * <b>Example:</b> soldierOne commandRadio "messageOne"<br/>
   * @param oper1 unit
   * @param oper2 radioName
   * @since 5501
   */
  public native static void commandRadio(Object oper1, String oper2);
  /** 
   * Sends the message to the vehicle radio channel. The message is defined in the description.ext file or radio protocol.
<br/><br/>
   * <b>Example:</b> soldierOne vehicleRadio "messageOne"<br/>
   * @param oper1 unit
   * @param oper2 radioName
   */
  public native static void vehicleRadio(GameObject oper1, String oper2);
  /** 
   * Return object of given type with given arguments. Use [type, game value] to search by object reference of a specific editor object type.
   * @param oper1 map
   * @param oper2 [type, name1, value1, ...]
   * @since 2.35
   */
  public native static String findEditorObject(GameControl oper1, java.util.List oper2);
  /** 
   * Return object that matches the provided reference.
   * @param oper1 map
   * @param oper2 value
   * @since 2.92
   */
  public native static String findEditorObject(GameControl oper1, Object oper2);
  /** 
   * Sets the flag owner. When the owner is set to objNull, the flag is returned to the flagpole.
<br/><br/>
   * <b>Example:</b> setFlagOwner objNull<br/>
   * @param oper1 flag
   * @param oper2 owner
   */
  public native static void setFlagOwner(GameObject oper1, GameObject oper2);
  /** 
   * Convert world coordinates to screen coordinates in map.
   * @param oper1 map
   * @param oper2 position
   * @since 2.54
   */
  public native static java.util.List posWorldToScreen(GameControl oper1, java.util.List oper2);
  /** 
   * Set a new direction (angle) of given location.
   * @param oper1 location
   * @param oper2 direction
   * @since 2.90
   */
  public native static void setDirection(GameLocation oper1, float oper2);
  /** 
   * For waypoints attached to a house, this defines the target house position.
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointHousePosition 1<br/>
   * @param oper1 waypoint
   * @param oper2 pos
   * @since 1.86
   */
  public native static void setWaypointHousePosition(java.util.List oper1, float oper2);
  /** 
   * Prepares the camera bank angle. See also <f>camPreload</f> and <f>camCommitPrepared</f>.
<br/><br/>
   * <b>Example:</b> _camera camPrepareBank -0.1<br/>
   * @param oper1 camera
   * @param oper2 bank
   * @since 2.95
   */
  public native static void camPrepareBank(GameObject oper1, float oper2);
  /** 
   * Sets the marker type. Type may be any of: "Flag", "Flag1", "Dot", "Destroy", "Start", "End", "Warning", "Join", "Pickup", "Unknown", "Marker", "Arrow" or "Empty". The marker is modified on all computers in a network session.
<br/><br/>
   * <b>Example:</b> "MarkerOne" setMarkerType "Arrow"<br/>
   * @param oper1 markerName
   * @param oper2 markerType
   */
  public native static void setMarkerType(String oper1, String oper2);
  /** 
   * Creates a move waypoint on the given position (format <ar>Position</ar>) and makes it the currently active group waypoint.
<br/><br/>
   * <b>Example:</b> groupOne move getPos player<br/>
   * @param oper1 group
   * @param oper2 pos
   */
  public native static void move(Object oper1, java.util.List oper2);
  /** 
   * Commits the prepared camera changes smoothly over time. A time of zero results in an immediate change.
<br/><br/>
   * <b>Example:</b> _camera camCommit 5<br/>
   * @param oper1 camera
   * @param oper2 time
   * @since 2.95
   */
  public native static void camCommitPrepared(GameObject oper1, float oper2);
  /** 
   * Set states and/or makes an action of/on weapon
<br/><br/>
   * <b>Example:</b> _done = _vehicle setWeaponState [true, , 0]<br/>
   * @param oper1 vehicle
   * @param oper2 [reloadInstantly, weaponName, reloadTime]
   * @since 1.18
   */
  public native static boolean setWeaponState(GameObject oper1, java.util.List oper2);
  /** 
   * Sets the velocity (speed vector) of a vehicle.
   * @param oper1 vehicle
   * @param oper2 [x, z, y]
   * @since 1.80
   */
  public native static void setVelocity(GameObject oper1, java.util.List oper2);
  /** 
   * Sets wanted transparency for control animation.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFade 1<br/>
   * @param oper1 control
   * @param oper2 fade
   * @since 2.50
   */
  public native static void ctrlSetFade(GameControl oper1, float oper2);
  /** 
   * Sets the picture in the item with the given index of the given listbox or combobox. Name is the picture name. The picture is searched in the mission directory, the dtaExt subdirectory of the campaign directory and the dtaExt directory and the data bank (or directory).
<br/><br/>
   * <b>Example:</b> _control lbSetPicture [0, "iskoda"]<br/>
   * @param oper1 control
   * @param oper2 [index, name]
   * @since 2.91
   */
  public native static void lbSetPicture(GameControl oper1, java.util.List oper2);
  /** 
   * Set the task as a current task of the person.
   * @param oper1 person
   * @param oper2 task
   * @since 5501
   */
  public native static void setCurrentTask(GameObject oper1, GameTask oper2);
  /** 
   * Attaches a vehicle to the given waypoint.
<br/><br/>
   * <b>Example:</b> [grp, 2] waypointAttachVehicle vehicle player<br/>
   * @param oper1 waypoint
   * @param oper2 vehicle
   * @since 1.86
   */
  public native static void waypointAttachVehicle(java.util.List oper1, GameObject oper2);
  /** 
   * It sets the data for hiding. ObjectWhereHide can be taken using findCover. HidePosition can be taken using getHideFrom.
   * @param oper1 object
   * @param oper2 [objectWhereHide, hidePosition]
   * @since 2.92
   */
  public native static void setHideBehind(GameObject oper1, java.util.List oper2);
  /** 
   * The greater of a,b
<br/><br/>
   * <b>Example:</b> 3 max 2<br/>
<br/>
   * <b>Example result:</b> 3<br/>
   * @param oper1 a
   * @param oper2 b
   */
  public native static float max(float oper1, float oper2);
  /** 
   * Orders the unit to watch the given position (format <ar>Position</ar>) (via the radio).
<br/><br/>
   * <b>Example:</b> soldierOne commandWatch getMarkerPos "MarkerMoveOne"<br/>
   * @param oper1 unit
   * @param oper2 position
   */
  public native static void commandWatch(Object oper1, java.util.List oper2);
  /** 
   * Orders the unit to watch the given target (via the radio).
<br/><br/>
   * <b>Example:</b> soldierOne commandWatch player<br/>
   * @param oper1 unit
   * @param oper2 target
   */
  public native static void commandWatch(Object oper1, GameObject oper2);
  /** 
   * Toggle the drawing of 3D icons.
   * @param oper1 map
   * @param oper2 bool
   * @since 2.92
   */
  public native static void show3DIcons(GameControl oper1, boolean oper2);
  /** 
   * Sets the camera heading. It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetDir 150<br/>
   * @param oper1 camera
   * @param oper2 direction
   */
  public native static void camSetDir(GameObject oper1, float oper2);
  /** 
   * Defines the different sound effects. Sound / voice plays a 2D / 3D sound from CfgSounds. SoundEnv plays an enviromental sound from CfgEnvSounds. SoundDet (only for triggers) creates a dynamic sound object attached to a trigger defined in CfgSFX.
<br/><br/>
   * <b>Example:</b> trigger setSoundEffect ["Alarm", "", "", ""]<br/>
   * @param oper1 trigger or waypoint
   * @param oper2 [sound, voice, soundEnv, soundDet]
   * @since 1.86
   */
  public native static void setSoundEffect(Object oper1, java.util.List oper2);
  /** 
   * Set the proxy object associated with the given editor object.
   * @param oper1 map
   * @param oper2 [object, proxy object]
   * @since 2.92
   */
  public native static Object setObjectProxy(GameControl oper1, java.util.List oper2);
  /** 
   * Set if given team member was inserted directly from editor.
<br/><br/>
   * <b>Example:</b> _teamMember setFromEditor true<br/>
   * @param oper1 teamMember
   * @param oper2 fromEditor
   * @since 2.90
   */
  public native static void setFromEditor(GameTeamMember oper1, boolean oper2);
  /** 
   * Computes the distance between two objects or positions.
<br/><br/>
   * <b>Example:</b> player distance leader player<br/>
   * @param oper1 obj1 or pos1
   * @param oper2 obj2 or pos2
   */
  public native static float distance(Object oper1, Object oper2);
  /** 
   * Computes the distance between two locations or positions.
   * @param oper1 location1
   * @param oper2 location2
   * @since 5501
   */
  public native static float distance(GameLocation oper1, GameLocation oper2);
  /** 
   * Computes the distance between two locations or positions.
   * @param oper1 location1
   * @param oper2 pos2
   * @since 5501
   */
  public native static float distance(GameLocation oper1, java.util.List oper2);
  /** 
   * Computes the distance between two locations or positions.
   * @param oper1 pos1
   * @param oper2 location2
   * @since 5501
   */
  public native static float distance(java.util.List oper1, GameLocation oper2);
  /** 
   * Unregister conversation topic from given person.
   * @param oper1 person
   * @param oper2 name
   * @since 2.92
   */
  public native static void kbRemoveTopic(GameObject oper1, String oper2);
  /** 
   * The remainder of a divided by b. Note that the remainer is calculated in the real domain.
<br/><br/>
   * <b>Example:</b> 3 mod 2<br/>
<br/>
   * <b>Example result:</b> 1<br/>
   * @param oper1 a
   * @param oper2 b
   */
  public native static float mod(float oper1, float oper2);
  /** 
   * Functionally same as Say, only difference is sound played as 3D allways.
<br/><br/>
   * <b>Example:</b> soldierOne say "speechId"<br/>
   * @param oper1 unit or [unit, target]
   * @param oper2 speechName
   * @since 5501
   */
  public native static void say3D(Object oper1, String oper2);
  /** 
   * Functionally same as Say, only difference is sound played as 3D allways.
   * @param oper1 unit or [unit, target]
   * @param oper2 speechName
   * @since 5501
   */
  public native static void say3D(Object oper1, java.util.List oper2);
  /** 
   * Object background - the right argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The left argument define layer in which the effect is show, 0 is the back most.
The object can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> 0 cutObj ["TVSet", "plain"]<br/>
   * @param oper1 layer
   * @param oper2 effect
   * @since 5126
   */
  public native static void cutObj(float oper1, java.util.List oper2);
  /** 
   * Moves the soldier into the vehicle's turret. (Immediately, without animation).
<br/><br/>
   * <b>Example:</b> soldierOne moveInTurret [tank, [0, 0]]<br/>
   * @param oper1 soldier
   * @param oper2 [vehicle, turret path]
   */
  public native static void moveInTurret(GameObject oper1, java.util.List oper2);
  /** 
   * Gunner in unit's vehicle fire at given target. If weapon is not given, current is used.
<br/><br/>
   * <b>Example:</b> bool = vehicle FireAtTarget [target]; bool = player FireAtTarget [target,1]<br/>
   * @param oper1 vehicle
   * @param oper2 [target, weapon]
   * @since 5501
   */
  public native static boolean fireAtTarget(GameObject oper1, java.util.List oper2);
  /** 
   * Set brightness of light.
   * @param oper1 light
   * @param oper2 brightness
   * @since 2.58
   */
  public native static void setLightBrightness(GameObject oper1, float oper2);
  /** 
   * Sends the message to the direct channel. The message is defined in the description.ext file or radio protocol.
<br/><br/>
   * <b>Example:</b> soldierOne directSay "messageOne"<br/>
   * @param oper1 unit
   * @param oper2 radioName
   * @since 5501
   */
  public native static void directSay(GameObject oper1, String oper2);
  /** 
   * Returns the given indexed exit in a building. The returned value is in format <ar>Position</ar>.
<br/><br/>
   * <b>Example:</b> buildingExit [building, 1]<br/>
   * @param oper1 building
   * @param oper2 index
   */
  public native static java.util.List buildingExit(GameObject oper1, float oper2);
  /** 
   * Synchronizes the trigger with waypoints. Each waypoint is given as an array [group, index].
<br/><br/>
   * <b>Example:</b> trigger synchronizeWaypoint []<br/>
   * @param oper1 trigger
   * @param oper2 [waypoint1, waypoint2, ...]
   * @since 1.86
   */
  public native static void synchronizeTrigger(GameObject oper1, java.util.List oper2);
  /** 
   * Checks whether the unit has the given weapon.
<br/><br/>
   * <b>Example:</b> player hasWeapon "M16"<br/>
   * @param oper1 unit
   * @param oper2 weaponName
   */
  public native static boolean hasWeapon(GameObject oper1, String oper2);
  /** 
   * FSM debugging enabled for all FSM newly created by the entity.
<br/><br/>
   * <b>Example:</b> soldier diag_debugFSM true<br/>
   * @param oper1 entity
   * @param oper2 2.0
   */
  public native static void diag_debugFSM(GameObject oper1, float oper2);
  /** 
   * Joins all units in the array to the given group.
Note: the total number of group members cannot exceed 12.
Note: This function is unsupported in MP in version 1.33 and before.
<br/><br/>
   * <b>Example:</b> [unitOne, unitTwo] join player<br/>
   * @param oper1 unitArray
   * @param oper2 group
   */
  public native static void join(java.util.List oper1, Object oper2);
  /** 
   * Sets H1 font size of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeightH1 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontHeightH1(GameControl oper1, float oper2);
  /** 
   * Orders a unit to follow the given unit (silently).
<br/><br/>
   * <b>Example:</b> soldierOne doFollow player<br/>
   * @param oper1 unit
   * @param oper2 position
   */
  public native static void doFollow(Object oper1, GameObject oper2);
  /** 
   * Sets the object position. The pos array uses the <ar>PositionASL</ar> format. The version of the command does not offset based on object center.
   * @param oper1 obj
   * @param oper2 pos
   * @since 2.53
   */
  public native static void setPosASL2(GameObject oper1, java.util.List oper2);
  /** 
   * Defines if scroll should rewind when auto-scroll reach end.
<br/><br/>
   * <b>Example:</b> _control ctrlSetAutoScrollDelay 5<br/>
   * @param oper1 control
   * @param oper2 delay
   * @since 5501
   */
  public native static void ctrlSetAutoScrollRewind(GameControl oper1, boolean oper2);
  /** 
   * Sets given event handler of given control
<br/><br/>
   * <b>Example:</b> _control ctrlSetEventHandler ["KeyDown", ""]<br/>
   * @param oper1 control
   * @param oper2 [handler name, function]
   * @since 2.54
   */
  public native static void ctrlSetEventHandler(GameControl oper1, java.util.List oper2);
  /** 
   * The smaller of a,b
<br/><br/>
   * <b>Example:</b> 3 min 2<br/>
<br/>
   * <b>Example result:</b> 2<br/>
   * @param oper1 a
   * @param oper2 b
   */
  public native static float min(float oper1, float oper2);
  /** 
   * Set randomization of particle source parameters.
   * @param oper1 particleSource
   * @param oper2 [lifeTime, position, moveVelocity, rotationVelocity, size, color, randomDirectionPeriod, randomDirectionIntensity, {angle}]
   * @since 2.56
   */
  public native static void setParticleRandom(GameObject oper1, java.util.List oper2);
  /** 
   * Execute an editor script for the specified object.
<br/><br/>
   * <b>Example:</b> _map execEditorScript ["_team_1", "create"]<br/>
   * @param oper1 map
   * @param oper2 [object, script]
   * @since 2.92
   */
  public native static Object execEditorScript(GameControl oper1, java.util.List oper2);
  /** 
   * Returns all magazines of given turret. Use turret path [-1] for drivers turret.
<br/><br/>
   * <b>Example:</b> vehicle player magazinesTurret [0,0]<br/>
   * @param oper1 transport
   * @param oper2 turret path
   * @since 5501
   */
  public native static java.util.List magazinesTurret(GameObject oper1, java.util.List oper2);
  /** 
   * When used on a person, a smooth transition to the given action will be initiated, but all previous playAction are discarded.
<br/><br/>
   * <b>Example:</b> soldierOne playActionNow "SitDown"<br/>
   * @param oper1 soldier
   * @param oper2 action
   * @since 5501
   */
  public native static void playActionNow(GameObject oper1, String oper2);
  /** 
   * Sets the main font size of given control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeight 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontHeight(GameControl oper1, float oper2);
  /** 
   * Process animation on object. Animation is defined in config file. Wanted animation phase is set to phase.
<br/><br/>
   * <b>Example:</b> house animate ["doors1", 1]<br/>
   * @param oper1 object
   * @param oper2 [animation, phase]
   * @since 1.75
   */
  public native static void animate(GameObject oper1, java.util.List oper2);
  /** 
   * 
   * @param oper1 object
   * @param oper2 texture
   * @since 1.75
   */
  public native static void setObjectTexture(GameObject oper1, java.util.List oper2);
  /** 
   * Sets H2 font size of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeightH2 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontHeightH2(GameControl oper1, float oper2);
  /** 
   * Aggregate candidates.
   * @param oper1 [speaker, side, unit, place, time]
   * @param oper2 candidates
   * @since 2.92
   */
  public native static java.util.List targetsAggregate(java.util.List oper1, java.util.List oper2);
  /** 
   * Set airport to be used by AI on getout
<br/><br/>
   * <b>Example:</b> plane assignToAirport 0<br/>
   * @param oper1 airplane
   * @param oper2 airportId
   * @since 2.92
   */
  public native static void assignToAirport(GameObject oper1, float oper2);
  /** 
   * Sets H1 bold font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH1B "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH1B(GameControl oper1, String oper2);
  /** 
   * Attach a destination to the simple task. Overrides setSimpleTaskTarget.
   * @param oper1 task
   * @param oper2 position
   * @since 5500
   */
  public native static void setSimpleTaskDestination(GameTask oper1, java.util.List oper2);
  /** 
   * Sets H3 font size of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeightH3 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontHeightH3(GameControl oper1, float oper2);
  /** 
   * Check if findEmptyPosition can be called without waiting for files.
   * @param oper1 center
   * @param oper2 [radius, maxDistance]
   * @since 5501
   */
  public native static java.util.List findEmptyPositionReady(java.util.List oper1, java.util.List oper2);
  /** 
   * Returns the position of the first array element that matches x, returns -1 if not found.
<br/><br/>
   * <b>Example:</b> [0, 1, 2] find 1<br/>
<br/>
   * <b>Example result:</b> 1<br/>
   * @param oper1 array
   * @param oper2 x
   * @since 2.92
   */
  public native static float find(java.util.List oper1, Object oper2);
  /** 
   * Sets the type of action processed by the trigger after activation (no action, a waypoints switch or an end of mission): "NONE", "SWITCH", "END1", "END2", "END3", "END4", "END5", "END6", "LOOSE" or "WIN".
<br/><br/>
   * <b>Example:</b> trigger setTriggerType "END1"<br/>
   * @param oper1 trigger
   * @param oper2 action
   * @since 1.86
   */
  public native static void setTriggerType(GameObject oper1, String oper2);
  /** 
   * Sets the camera bank angle. It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetBank -0.1<br/>
   * @param oper1 camera
   * @param oper2 bank
   */
  public native static void camSetBank(GameObject oper1, float oper2);
  /** 
   * Sets rank of given unit. Possible values: PRIVATE, CORPORAL, SERGEANT, LIEUTENANT, CAPTAIN, MAJOR or COLONEL.
<br/><br/>
   * <b>Example:</b> player setUnitRank "COLONEL"<br/>
   * @param oper1 unit
   * @param oper2 rank
   * @since 2.33
   */
  public native static void setUnitRank(GameObject oper1, String oper2);
  /** 
   * a and b
<br/><br/>
   * <b>Example:</b> alive player and alive leader player<br/>
   * @param oper1 a
   * @param oper2 b
   */
  public native static boolean and(boolean oper1, boolean oper2);
  /** 
   * Sets number of second required to scroll to next line. If speed smaller than 0, auto-scroll is disabled.
<br/><br/>
   * <b>Example:</b> _control ctrlSetAutoScrollSpeed 5<br/>
   * @param oper1 control
   * @param oper2 speed
   * @since 5501
   */
  public native static void ctrlSetAutoScrollSpeed(GameControl oper1, float oper2);
  /** 
   * Select given group in HC bar.
<br/><br/>
   * <b>Example:</b> unit hcSelectGroup [group,true]<br/>
   * @param oper1 unit
   * @param oper2 array
   * @since 5501
   */
  public native static void hcSelectGroup(GameObject oper1, java.util.List oper2);
  /** 
   * Find all targets known to sender matching given query.
   * @param oper1 speaker
   * @param oper2 [receiver, side, unit, place, time]
   * @since 2.92
   */
  public native static java.util.List targetsQuery(GameObject oper1, java.util.List oper2);
  /** 
   * Sets H4 font size of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeightH4 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontHeightH4(GameControl oper1, float oper2);
  /** 
   * Sets wanted scale for control animation. Top left corner remains same.
<br/><br/>
   * <b>Example:</b> _control ctrlScale 0.5<br/>
   * @param oper1 control
   * @param oper2 scale
   * @since 5501
   */
  public native static void ctrlSetScale(GameControl oper1, float oper2);
  /** 
   * Ceases the using of the vehicle in the group. It unassigns all grouped units from the vehicle.
<br/><br/>
   * <b>Example:</b> soldierOne leaveVehicle jeepOne<br/>
   * @param oper1 group
   * @param oper2 vehicle
   */
  public native static void leaveVehicle(GameObject oper1, GameObject oper2);
  /** 
   * Ceases the using of the vehicle in the group. It unassigns all grouped units from the vehicle.
<br/><br/>
   * <b>Example:</b> groupOne leaveVehicle jeepOne<br/>
   * @param oper1 group
   * @param oper2 vehicle
   */
  public native static void leaveVehicle(GameGroup oper1, GameObject oper2);
  /** 
   * Center the map on, and point the camera at, the position.
   * @param oper1 map
   * @param oper2 position
   * @since 2.92
   */
  public native static void lookAtPos(GameControl oper1, java.util.List oper2);
  /** 
   * Sets the orientation of the marker. Angle is in degrees. The marker is only modified on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> "Marker1" setMarkerDirLocal 90<br/>
   * @param oper1 name
   * @param oper2 angle
   * @since 2.92
   */
  public native static void setMarkerDirLocal(String oper1, float oper2);
  /** 
   * Register knowledge base database to given person.
<br/><br/>
   * <b>Example:</b> _unit kbAddDatabase "chat.txt"<br/>
   * @param oper1 person
   * @param oper2 filename
   * @since 2.42
   */
  public native static boolean kbAddDatabase(GameObject oper1, String oper2);
  /** 
   * Sets the amount of ammo resources in the cargo space of a repair vehicle. Ammo resources are used to resupply vehicles; soldiers use individual magazines instead. An amount of one indicates a full cargo.
<br/><br/>
   * <b>Example:</b> rearmTruckOne setAmmoCargo 0<br/>
   * @param oper1 vehicle
   * @param oper2 ammoCargo
   */
  public native static void setAmmoCargo(GameObject oper1, float oper2);
  /** 
   * Sets H5 font size of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeightH5 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontHeightH5(GameControl oper1, float oper2);
  /** 
   * Creates an action. The action ID should be returned, but due to a bug, it's not.
To determine the action ID, use following algorithm: the first action added to the given vehicle has ID zero, the next vehicle has ID one, etc.
<br/><br/>
   * <b>Example:</b> player addAction ["Hello", "hello.sqs"]<br/>
   * @param oper1 unit
   * @param oper2 [action, script filename(, arguments, priority, showWindow, hideOnUse, shortcut, condition)]
   * @since 1.10
   */
  public native static Object addAction(GameObject oper1, java.util.List oper2);
  /** 
   * Add icon to a group. Returns icon ID
<br/><br/>
   * <b>Example:</b> id = group addGroupIcon["b_inf",[offsetX,ofsetY]]<br/>
   * @param oper1 group
   * @param oper2 properties
   * @since 5501
   */
  public native static float addGroupIcon(GameGroup oper1, java.util.List oper2);
  /** 
   * Adds weapons to the weapon cargo space. This is used for infantry weapons.  MP synchronized. The format of weapons is [weaponName, count].
For weaponName values see <cl>CfgWeapons</cl>.
<br/><br/>
   * <b>Example:</b> rearmTruckOne addWeaponCargo ["M16", 10]<br/>
   * @param oper1 unit
   * @param oper2 weapons
   */
  public native static void addWeaponCargoGlobal(GameObject oper1, java.util.List oper2);
  /** 
   * Control what the unit is glancing at (target or position) (format <ar>Position</ar>)
<br/><br/>
   * <b>Example:</b> someSoldier glanceAt otherSoldier; otherSoldier glanceAt getMarkerPos "markerOne"<br/>
   * @param oper1 unit(s)
   * @param oper2 position
   * @since 2.40
   */
  public native static void glanceAt(Object oper1, Object oper2);
  /** 
   * Defines the area controlled by the trigger. The area is rectangular or elliptic, the width is 2 * a, the height is 2 * b. It is rotated angle degrees.
<br/><br/>
   * <b>Example:</b> trigger setTriggerArea [100, 50, 45, false]<br/>
   * @param oper1 trigger
   * @param oper2 [a, b, angle, rectangle]
   * @since 1.86
   */
  public native static void setTriggerArea(GameObject oper1, java.util.List oper2);
  /** 
   * a or b
<br/><br/>
   * <b>Example:</b> not alive player or not alive leader player<br/>
   * @param oper1 a
   * @param oper2 b
   */
  public native static boolean or(boolean oper1, boolean oper2);
  /** 
   * Sets H3 bold font of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontH3B "TahomaB"<br/>
   * @param oper1 control
   * @param oper2 name
   * @since 2.50
   */
  public native static void ctrlSetFontH3B(GameControl oper1, String oper2);
  /** 
   * Returns the additional text (invisible) in an item with the given index of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _data = _control lbData 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.91
   */
  public native static String lbData(GameControl oper1, float oper2);
  /** 
   * Sets H6 font size of given HTML control.
<br/><br/>
   * <b>Example:</b> _control ctrlSetFontHeightH6 0.05<br/>
   * @param oper1 control
   * @param oper2 height
   * @since 2.50
   */
  public native static void ctrlSetFontHeightH6(GameControl oper1, float oper2);
  /** 
   * Select group leader.
<br/><br/>
   * <b>Example:</b> group player selectLeader player<br/>
   * @param oper1 group
   * @param oper2 unit
   * @since 2.33
   */
  public native static void selectLeader(GameGroup oper1, GameObject oper2);
  /** 
   * Sets the camera position relative to the current position of the currect target (see <f>camSetTarget</f>). It does not automatically commit changes.
<br/><br/>
   * <b>Example:</b> _camera camSetRelPos [10,5]<br/>
   * @param oper1 camera
   * @param oper2 position
   */
  public native static void camSetRelPos(GameObject oper1, java.util.List oper2);
  /** 
   * Orders a unit to process command defined by FSM file (silently).
<br/><br/>
   * <b>Example:</b> soldierOne doFSM ["move.fsm", position player, player]<br/>
   * @param oper1 unit(s)
   * @param oper2 [fsm name, position, target]
   * @since 2.53
   */
  public native static void doFSM(Object oper1, java.util.List oper2);
  /** 
   * Switch on remote control of the unit.
<br/><br/>
   * <b>Example:</b> player remoteControl gunner _uav<br/>
   * @param oper1 who
   * @param oper2 whom
   * @since 5501
   */
  public native static void remoteControl(GameObject oper1, GameObject oper2);
  /** 
   * Sets the fuel amount. A fuel level of one is a full gas tank.
<br/><br/>
   * <b>Example:</b> jeepOne setFuel 0<br/>
   * @param oper1 vehicle
   * @param oper2 amount
   */
  public native static void setFuel(GameObject oper1, float oper2);
  /** 
   * Types text to the vehicle radio channel.
Note: this function only types text to the list, it does not broadcast the message. If you want the message to show on all computers, you have to execute it on all of them.
<br/><br/>
   * <b>Example:</b> soldierOne vehicleChat "Show this text"<br/>
   * @param oper1 unit
   * @param oper2 chatText
   */
  public native static void vehicleChat(GameObject oper1, String oper2);
  /** 
   * Removes the item with the given index from the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _control lbDelete 0<br/>
   * @param oper1 control
   * @param oper2 index
   * @since 2.91
   */
  public native static void lbDelete(GameControl oper1, float oper2);
  /** 
   * Remove given member from given team.
<br/><br/>
   * <b>Example:</b> _team removeTeamMember _teamMember<br/>
   * @param oper1 team
   * @param oper2 member
   * @since 2.90
   */
  public native static void removeTeamMember(GameTeamMember oper1, GameTeamMember oper2);
  /** 
   * Removes event handler added by <f>addEventHandler</f>. Format of handler is [type,index]. Index is returned by addEventHandler. When any handler is removed, all handler indices higher that the deleted one should be decremented.
<br/><br/>
   * <b>Example:</b> player removeEventHandler ["killed",0]<br/>
   * @param oper1 object
   * @param oper2 handler
   * @since 1.85
   */
  public native static void removeEventHandler(GameObject oper1, java.util.List oper2);
  /** 
   * Switches the group speed mode when the waypoint becomes active. Possible values are: "UNCHANGED", "LIMITED", "NORMAL" and "FULL".
<br/><br/>
   * <b>Example:</b> [grp, 2] setWaypointSpeed "FULL"<br/>
   * @param oper1 waypoint
   * @param oper2 mode
   * @since 1.86
   */
  public native static void setWaypointSpeed(java.util.List oper1, String oper2);
  /** 
   * Sets background color of given control. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _control ctrlSetForegroundColor [1, 0, 0, 1]<br/>
   * @param oper1 display
   * @param oper2 color
   * @since 2.50
   */
  public native static void ctrlSetForegroundColor(GameControl oper1, java.util.List oper2);
  /** 
   * Select an editor object. Does not un-select previously selected objects.
   * @param oper1 map
   * @param oper2 object
   * @since 2.92
   */
  public native static Object selectEditorObject(GameControl oper1, String oper2);
  /** 
   * Copy the chain of waypoints from source to target group. The target group will start to process waypoints from the first one.
   * @param oper1 groupTo
   * @param oper2 groupFrom
   * @since 5500
   */
  public native static void copyWaypoints(GameGroup oper1, GameGroup oper2);
  /** 
   * Defines the time between condition satisfaction and trigger activation (randomly from min to max, with an average value mid). If the last argument is true, the condition must be fullfilled all the time.
<br/><br/>
   * <b>Example:</b> trigger setTriggerTimeout [5, 10, 7, false]<br/>
   * @param oper1 trigger
   * @param oper2 [min, mid, max, interruptable]
   * @since 1.86
   */
  public native static void setTriggerTimeout(GameObject oper1, java.util.List oper2);
  /** 
   * Remove all even handlers from the given display.
<br/><br/>
   * <b>Example:</b> _display displayRemoveAllEventHandlers "KeyDown"<br/>
   * @param oper1 display
   * @param oper2 handler name
   * @since 5501
   */
  public native static void displayRemoveAllEventHandlers(GameDisplay oper1, String oper2);
  /** 
   * Update the editor object tree.
   * @param oper1 map
   * @since 2.92
   */
  /* Update the editor object tree.*/
  public native static void updateObjectTree(GameControl oper1);
  /** 
   * Creates a structured text by replacing %1, %2, etc. in format by plain or structured texts given as arguments.
<br/><br/>
   * <b>Example:</b> txt = formatText ["Image: %1", image "data\isniper.paa"]<br/>
   * @param oper1 [format, arg1, arg2, ...]
   * @since 2.01
   */
  /* Creates a structured text by replacing %1, %2, etc. in format by plain or structured texts given as arguments.*/
  public native static GameText formatText(java.util.List oper1);
  /** 
   * Returns selected groups in high command.
<br/><br/>
   * <b>Example:</b> array = hcAllGroups unit<br/>
   * @param oper1 unit
   * @since 5501
   */
  /* Returns selected groups in high command.*/
  public native static java.util.List hcAllGroups(GameObject oper1);
  /** 
   * Gets the marker direction. See <f>setMarkerDir</f>.
<br/><br/>
   * <b>Example:</b> markerDir "MarkerOne"<br/>
   * @param oper1 markerName
   * @since 2.92
   */
  /* Gets the marker direction. See <f>setMarkerDir</f>.*/
  public native static java.util.List markerDir(String oper1);
  /** 
   * Checks whether the unit is a captive. If the unit is a vehicle, its commander is checked instead.
<br/><br/>
   * <b>Example:</b> captive player<br/>
   * @param oper1 unit
   */
  /* Checks whether the unit is a captive. If the unit is a vehicle, its commander is checked instead.*/
  public native static float captiveNum(GameObject oper1);
  /** 
   * Returns trigger text.
   * @param oper1 trigger
   * @since 5501
   */
  /* Returns trigger text.*/
  public native static String triggerText(GameObject oper1);
  /** 
   * Sets if group icons raises onclick and onover events.
<br/><br/>
   * <b>Example:</b> selectableGroupIcons true<br/>
   * @param oper1 bool
   * @since 5501
   */
  /* Sets if group icons raises onclick and onover events.*/
  public native static void setGroupIconsSelectable(boolean oper1);
  /** 
   * Removes column with given index.
<br/><br/>
   * <b>Example:</b> lnbDeleteColumn [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 5501
   */
  /* Removes column with given index.*/
  public native static void lnbDeleteColumn(java.util.List oper1);
  /** 
   * Returns the number of players playing on the given side.
   * @param oper1 side
   * @since 1.80
   */
  /* Returns the number of players playing on the given side.*/
  public native static float playersNumber(GameSide oper1);
  /** 
   * Return camera interest for given entity.
   * @param oper1 entity
   * @since 2.57
   */
  /* Return camera interest for given entity.*/
  public native static float cameraInterest(GameObject oper1);
  /** 
   * Returns true if vehicle has enabled auto hover
   * @param oper1 vehicle
   * @since 5501
   */
  /* Returns true if vehicle has enabled auto hover*/
  public native static boolean isAutoHoverOn(GameObject oper1);
  /** 
   * Removes all weapons from the vehicle cargo space.
<br/><br/>
   * <b>Example:</b> clearWeaponCargo jeepOne<br/>
   * @param oper1 unit
   */
  /* Removes all weapons from the vehicle cargo space.*/
  public native static void clearWeaponCargo(GameObject oper1);
  /** 
   * On server machine, return the ID of the client where the object is local. Otherwise return 0.
   * @param oper1 object
   * @since 5501
   */
  /* On server machine, return the ID of the client where the object is local. Otherwise return 0.*/
  public native static float owner(GameObject oper1);
  /** 
   * Enable/disable environmental effects (ambient life + sound).
<br/><br/>
   * <b>Example:</b> enableEnvironment false<br/>
   * @param oper1 enabled
   * @since 2.92
   */
  /* Enable/disable environmental effects (ambient life + sound).*/
  public native static void enableEnvironment(boolean oper1);
  /** 
   * The ceil value of x.
<br/><br/>
   * <b>Example:</b> ceil 5.25<br/>
<br/>
   * <b>Example result:</b> 6<br/>
   * @param oper1 x
   */
  /* The ceil value of x.*/
  public native static float ceil(float oper1);
  /** 
   * Returns the position of original (loaded) center of object in model coordinates.  The result is in format [x, z, y]
   * @param oper1 object
   * @since 5501
   */
  /* Returns the position of original (loaded) center of object in model coordinates.  The result is in format [x, z, y]*/
  public native static java.util.List boundingCenter(GameObject oper1);
  /** 
   * This statement is launched whenever a player is connected to a MP session. Variables _id, _name and _uid are set. See also getPlayerUID, kickUID, banUID.
   * @param oper1 statement
   * @since 2.10
   */
  /* This statement is launched whenever a player is connected to a MP session. Variables _id, _name and _uid are set. See also getPlayerUID, kickUID, banUID.*/
  public native static void onPlayerConnected(Object oper1);
  /** 
   * Adds magazines from the campaign pool to the person (depending on the weapons the person has).
<br/><br/>
   * <b>Example:</b> fillWeaponsFromPool victor<br/>
   * @param oper1 person
   * @since 1.75
   */
  /* Adds magazines from the campaign pool to the person (depending on the weapons the person has).*/
  public native static void fillWeaponsFromPool(GameObject oper1);
  /** 
   * Capture the first frame, where section takes longer than the threshold (in seconds).
   * @param oper1 [section, threshold]
   * @since 5501
   */
  /* Capture the first frame, where section takes longer than the threshold (in seconds).*/
  public native static void diag_captureSlowFrame(java.util.List oper1);
  /** 
   * merges the given config file into the main config. Location is relative to the Userdir (default) or mission directory depending on the the value of UserDir
<br/><br/>
   * <b>Example:</b> mergeConfigFile [myconfig,false]<br/>
   * @param oper1 [FileName,UserDir]
   * @since 5501
   */
  /* merges the given config file into the main config. Location is relative to the Userdir (default) or mission directory depending on the the value of UserDir*/
  public native static void diag_mergeConfigFile(java.util.List oper1);
  /** 
   * Extract array from config entry.
<br/><br/>
   * <b>Example:</b> _array = getArray (configFile >> "CfgVehicles" >> "Thing" >> "threat")<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Extract array from config entry.*/
  public native static java.util.List getArray(GameConfig oper1);
  /** 
   * Enables the compass (the default is true).
   * @param oper1 show
   */
  /* Enables the compass (the default is true).*/
  public native static void showCompass(boolean oper1);
  /** 
   * Checks whether the given key is active in the current user profile.
<br/><br/>
   * <b>Example:</b> ok = isKeyActive "M04"<br/>
   * @param oper1 keyName
   * @since 2.06
   */
  /* Checks whether the given key is active in the current user profile.*/
  public native static boolean isKeyActive(String oper1);
  /** 
   * Enables the radio (the default is false).
   * @param oper1 show
   */
  /* Enables the radio (the default is false).*/
  public native static void showRadio(boolean oper1);
  /** 
   * If the unit has a flag, this flag is returned.
If not, objNull is returned.
<br/><br/>
   * <b>Example:</b> flag player<br/>
   * @param oper1 unit
   */
  /* If the unit has a flag, this flag is returned.
If not, objNull is returned.*/
  public native static GameObject flag(GameObject oper1);
  /** 
   * Switches on / off the logging of frames where a section takes longer than the threshold. Only a single capture can be active. If section is empty, logging is swithed off.
<br/><br/>
   * <b>Example:</b> diag_perfCap ["aiAll", 0.010]<br/>
   * @param oper1 [section, threshold]
   * @since 2.20
   */
  /* Switches on / off the logging of frames where a section takes longer than the threshold. Only a single capture can be active. If section is empty, logging is swithed off.*/
  public native static void diag_perfCap(java.util.List oper1);
  /** 
   * Obsolete.
Enables the ID card (the default is false).
   * @param oper1 show
   */
  /* Obsolete.
Enables the ID card (the default is false).*/
  public native static void showWarrant(boolean oper1);
  /** 
   * Forces the map to display.
<br/><br/>
   * <b>Example:</b> forceMap true<br/>
   * @param oper1 show
   * @since 1.27
   */
  /* Forces the map to display.*/
  public native static void forceMap(boolean oper1);
  /** 
   * Enables the notepad (the default is true).
   * @param oper1 show
   */
  /* Enables the notepad (the default is true).*/
  public native static void showPad(boolean oper1);
  /** 
   * Returns the index of the selected item of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _index = lbCurSel _control<br/>
   * @param oper1 control
   * @since 2.91
   */
  /* Returns the index of the selected item of the given listbox or combobox.*/
  public native static float lnbCurSelRow(GameControl oper1);
  /** 
   * Returns the index of the selected row id 2D listbox.
<br/><br/>
   * <b>Example:</b> _row = lnbCurSel 105<br/>
   * @param oper1 idc
   * @since 5501
   */
  /* Returns the index of the selected row id 2D listbox.*/
  public native static float lnbCurSelRow(float oper1);
  /** 
   * Shows a text hint. The text can contain several lines. \n is used to indicate the end of a line.
<br/><br/>
   * <b>Example:</b> hint "Press W to move forward"<br/>
   * @param oper1 text
   */
  /* Shows a text hint. The text can contain several lines. \n is used to indicate the end of a line.*/
  public native static void hint(Object oper1);
  /** 
   * Return object's up vector in world coordinates as [x, z, y].
   * @param oper1 obj
   * @since 2.61
   */
  /* Return object's up vector in world coordinates as [x, z, y].*/
  public native static java.util.List vectorUp(GameObject oper1);
  /** 
   * Returns the name of a button (on the keyboard, mouse or joystick) with the given code.
<br/><br/>
   * <b>Example:</b> name = keyName 28<br/>
<br/>
   * <b>Example result:</b> "Enter"<br/>
   * @param oper1 dikCode
   * @since 2.01
   */
  /* Returns the name of a button (on the keyboard, mouse or joystick) with the given code.*/
  public native static String keyName(float oper1);
  /** 
   * Selects the row with the given index of the 2D listbox.
<br/><br/>
   * <b>Example:</b> lnbSetCurSel [105, 0]<br/>
   * @param oper1 [idc, index]
   * @since 5501
   */
  /* Selects the row with the given index of the 2D listbox.*/
  public native static void lnbSetCurSelRow(java.util.List oper1);
  /** 
   * Sets camera shake default params.
   * @param oper1 [power, duration, maxDistance, frequency, minSpeed, minMass, caliberCoefHit, caliberCoefFire, vehicleCoef]
   * @since 5501
   */
  /* Sets camera shake default params.*/
  public native static void setCamShakeDefParams(java.util.List oper1);
  /** 
   * Loads a new landscape defined in a subclass of CfgWorlds, named worldname.
   * @param oper1 worldname
   * @since 1.85
   */
  /* Loads a new landscape defined in a subclass of CfgWorlds, named worldname.*/
  public native static void diag_switchLandscape(String oper1);
  /** 
   * Returns unit's backpack.
   * @param oper1 unit
   * @since 5501
   */
  /* Returns unit's backpack.*/
  public native static GameObject unitBackpack(GameObject oper1);
  /** 
   * Returns selected groups in high command.
<br/><br/>
   * <b>Example:</b> array = hcSelected unit<br/>
   * @param oper1 unit
   * @since 5501
   */
  /* Returns selected groups in high command.*/
  public native static java.util.List hcSelected(GameObject oper1);
  /** 
   * returns all backpack types and count from the cargo space.
<br/><br/>
   * <b>Example:</b> getBackpackCargo jeepOne<br/>
   * @param oper1 container
   */
  /* returns all backpack types and count from the cargo space.*/
  public native static java.util.List getBackpackCargo(GameObject oper1);
  /** 
   * Gets the marker color. See <f>setMarkerColor</f>.
Note: this function is identical to <f>getMarkerColor</f>.
<br/><br/>
   * <b>Example:</b> markerColor "MarkerOne"<br/>
   * @param oper1 markerName
   * @since 1.50
   */
  /* Gets the marker color. See <f>setMarkerColor</f>.
Note: this function is identical to <f>getMarkerColor</f>.*/
  public native static String markerColor(String oper1);
  /** 
   * If loading screen is show, set progress bar to the given value (from interval [0, 1])
   * @param oper1 progress
   * @since 5501
   */
  /* If loading screen is show, set progress bar to the given value (from interval [0, 1])*/
  public native static void progressLoadingScreen(float oper1);
  /** 
   * Creates a mine of the given type (type is the name of the subclass of CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The mine is placed inside a circle with this position as its center and placement as its radius.
<br/><br/>
   * <b>Example:</b> mine = createMine ["MineMine", position player, [], 0]<br/>
   * @param oper1 [type, position, markers, placement]
   * @since 2.32
   */
  /* Creates a mine of the given type (type is the name of the subclass of CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The mine is placed inside a circle with this position as its center and placement as its radius.*/
  public native static GameObject createMine(java.util.List oper1);
  /** 
   * Enables the watch (the default is true).
   * @param oper1 show
   */
  /* Enables the watch (the default is true).*/
  public native static void showWatch(boolean oper1);
  /** 
   * Dumps the argument's type and value to the report file.
<br/><br/>
   * <b>Example:</b> diag_log player<br/>
   * @param oper1 anything
   * @since 5501
   */
  /* Dumps the argument's type and value to the report file.*/
  public native static void diag_log(Object oper1);
  /** 
   * Sets the additional text (invisible) in the item with the given position of the 2D listbox.
<br/><br/>
   * <b>Example:</b> lnbSetData [101, [0,1], "#1"]<br/>
   * @param oper1 [idc, [row, column], data]
   * @since 5501
   */
  /* Sets the additional text (invisible) in the item with the given position of the 2D listbox.*/
  public native static void lnbsetColumnsPos(java.util.List oper1);
  /** 
   * Send message to given adress using UDP protocol.
   * @param oper1 [ip,port,"message"]
   * @since 5501
   */
  /* Send message to given adress using UDP protocol.*/
  public native static boolean sendUDPMessage(java.util.List oper1);
  /** 
   * Check if config entry represents number.
<br/><br/>
   * <b>Example:</b> _ok = isNumber (configFile >> "CfgVehicles")<br/>
<br/>
   * <b>Example result:</b> false<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Check if config entry represents number.*/
  public native static boolean isNumber(GameConfig oper1);
  /** 
   * Closes the topmost user dialog as if a button with id idc was pressed.
<br/><br/>
   * <b>Example:</b> closeDialog 1<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Closes the topmost user dialog as if a button with id idc was pressed.*/
  public native static void closeDialog(float oper1);
  /** 
   * Returns the picture name of the item with the given position of the 2D listbox.
<br/><br/>
   * <b>Example:</b> _picture = lnbPicture [101, [0,1]]<br/>
   * @param oper1 [idc, [row, column]
   * @since 5501
   */
  /* Returns the picture name of the item with the given position of the 2D listbox.*/
  public native static String lnbPicture(java.util.List oper1);
  /** 
   * Deletes a status created by saveStatus from the campaign progress file.
<br/><br/>
   * <b>Example:</b> deleteStatus "playerState"<br/>
   * @param oper1 statusName
   * @since 1.75
   */
  /* Deletes a status created by saveStatus from the campaign progress file.*/
  public native static boolean deleteStatus(String oper1);
  /** 
   * Return the result of helicopter landing position searching (performed after land command). The value can be "Found" (position found), "NotFound" (position not found), "NotReady" (position searching is still in progress) or empty string when wrong argument given.
   * @param oper1 helicopter
   * @since 5501
   */
  /* Return the result of helicopter landing position searching (performed after land command). The value can be "Found" (position found), "NotFound" (position not found), "NotReady" (position searching is still in progress) or empty string when wrong argument given.*/
  public native static String landResult(GameObject oper1);
  /** 
   * Returns trigger activation in the form [by, type, repeating]
   * @param oper1 trigger
   * @since 5501
   */
  /* Returns trigger activation in the form [by, type, repeating]*/
  public native static java.util.List triggerActivation(GameObject oper1);
  /** 
   * Assigns an action to the control with id idc of the topmost user dialog. Action is any expression in this scripting language. The function can be used for buttons and active texts.
<br/><br/>
   * <b>Example:</b> buttonSetAction [100, "player exec ""reply.sqs"""]<br/>
   * @param oper1 [idc, action]
   * @since 1.50
   */
  /* Assigns an action to the control with id idc of the topmost user dialog. Action is any expression in this scripting language. The function can be used for buttons and active texts.*/
  public native static void buttonSetAction(java.util.List oper1);
  /** 
   * Convert the string to lower case.
<br/><br/>
   * <b>Example:</b> toLower "ArmA"<br/>
<br/>
   * <b>Example result:</b> "arma"<br/>
   * @param oper1 string
   * @since 5195
   */
  /* Convert the string to lower case.*/
  public native static String toLower(String oper1);
  /** 
   * Gets waypoint visibility.
<br/><br/>
   * <b>Example:</b> visible = waypointVisible [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets waypoint visibility.*/
  public native static float waypointVisible(java.util.List oper1);
  /** 
   * Returns the additional text (invisible) in an item with the given position of the 2D listbox.
<br/><br/>
   * <b>Example:</b> _data = lnbData [101, [0,1]]<br/>
   * @param oper1 [idc, [row, column]
   * @since 5501
   */
  /* Returns the additional text (invisible) in an item with the given position of the 2D listbox.*/
  public native static String lnbData(java.util.List oper1);
  /** 
   * Check whether magazine is reloaded whenever emptied.
   * @param oper1 vehicle
   * @since 2.92
   */
  /* Check whether magazine is reloaded whenever emptied.*/
  public native static boolean reloadEnabled(GameObject oper1);
  /** 
   * Orders the unit to stop (via the radio).
Note: the stop command is never finished; the unit will never be ready.
<br/><br/>
   * <b>Example:</b> commandStop unitOne<br/>
   * @param oper1 unit
   */
  /* Orders the unit to stop (via the radio).
Note: the stop command is never finished; the unit will never be ready.*/
  public native static void commandStop(Object oper1);
  /** 
   * Gets the waypoint timeout values.
<br/><br/>
   * <b>Example:</b> waypointTimeout [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint timeout values.*/
  public native static java.util.List waypointTimeout(java.util.List oper1);
  /** 
   * Create the commanding menu described by the given config class. When class name is empty, current commanding menu is hidden.
   * @param oper1 class name
   * @since 5500
   */
  /* Create the commanding menu described by the given config class. When class name is empty, current commanding menu is hidden.*/
  public native static void showCommandingMenu(String oper1);
  /** 
   * Text background - the argument uses format ["text","type",speed] or ["text","type"]. If speed is not given, it's assumed to be one. Type may be one of: "PLAIN", "PLAIN DOWN", "BLACK", "BLACK FADED", "BLACK OUT", "BLACK IN", "WHITE OUT" or "WHITE IN".
<br/><br/>
   * <b>Example:</b> cutText ["", "BLACK OUT"]<br/>
   * @param oper1 effect
   */
  /* Text background - the argument uses format ["text","type",speed] or ["text","type"]. If speed is not given, it's assumed to be one. Type may be one of: "PLAIN", "PLAIN DOWN", "BLACK", "BLACK FADED", "BLACK OUT", "BLACK IN", "WHITE OUT" or "WHITE IN".*/
  public native static void cutText(java.util.List oper1);
  /** 
   * Destroys the given marker. The marker is only destroyed on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> deleteMarkerLocal "Marker1"<br/>
   * @param oper1 name
   * @since 2.92
   */
  /* Destroys the given marker. The marker is only destroyed on the computer where the command is called.*/
  public native static void deleteMarkerLocal(String oper1);
  /** 
   * Checks whether the unit is a captive. If the unit is a vehicle, its commander is checked instead.
<br/><br/>
   * <b>Example:</b> captive player<br/>
   * @param oper1 unit
   */
  /* Checks whether the unit is a captive. If the unit is a vehicle, its commander is checked instead.*/
  public native static boolean captive(GameObject oper1);
  /** 
   * Sets the additional text (invisible) in the item with the given position of the 2D listbox.
<br/><br/>
   * <b>Example:</b> lnbSetData [101, [0,1], "#1"]<br/>
   * @param oper1 [idc, [row, column], data]
   * @since 5501
   */
  /* Sets the additional text (invisible) in the item with the given position of the 2D listbox.*/
  public native static void lnbSetText(java.util.List oper1);
  /** 
   * Capture the single frame, following after the number frames given as parameter.
   * @param oper1 frameNo
   * @since 5501
   */
  /* Capture the single frame, following after the number frames given as parameter.*/
  public native static void diag_captureFrame(float oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engStressVM(float oper1);
  /** 
   * Returns the estimated time left in the game. Using this function the designer can provide a "time left" estimate that is shown in the "Game in progress" screen or in the master browser. For missions with a hard set limit adjusted via Param1, the following example can be used in the init.sqs file.
<br/><br/>
   * <b>Example:</b> estimatedTimeLeft Param1<br/>
   * @param oper1 timeLeft
   * @since 1.34
   */
  /* Returns the estimated time left in the game. Using this function the designer can provide a "time left" estimate that is shown in the "Game in progress" screen or in the master browser. For missions with a hard set limit adjusted via Param1, the following example can be used in the init.sqs file.*/
  public native static void estimatedTimeLeft(float oper1);
  /** 
   * Gets the type of the marker. See <f>setMarkerType</f>.
<br/><br/>
   * <b>Example:</b> getMarkerType "MarkerOne"<br/>
   * @param oper1 marker
   * @since 1.21
   */
  /* Gets the type of the marker. See <f>setMarkerType</f>.*/
  public native static String getMarkerType(String oper1);
  /** 
   * Returns the position on landscape (in world coordinates) corresponding to the given point on screen (in UI coordinates).
   * @param oper1 position
   * @since 5501
   */
  /* Returns the position on landscape (in world coordinates) corresponding to the given point on screen (in UI coordinates).*/
  public native static java.util.List screenToWorld(java.util.List oper1);
  /** 
   * Returns the object heading in the range from 0 to 360.
<br/><br/>
   * <b>Example:</b> getDir player<br/>
   * @param oper1 obj
   */
  /* Returns the object heading in the range from 0 to 360.*/
  public native static float getDir(GameObject oper1);
  /** 
   * True if auto-scroll should move back to start after it reach end.
<br/><br/>
   * <b>Example:</b> _rewind = ctrlAutoScrollRewind _control<br/>
   * @param oper1 control
   * @since 5501
   */
  /* True if auto-scroll should move back to start after it reach end.*/
  public native static boolean ctrlAutoScrollRewind(GameControl oper1);
  /** 
   * Check if fireplace is inflamed (buring).
<br/><br/>
   * <b>Example:</b> inflamed fireplaceOne<br/>
   * @param oper1 fireplace
   * @since 1.04
   */
  /* Check if fireplace is inflamed (buring).*/
  public native static boolean inflamed(GameObject oper1);
  /** 
   * Plays music defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> playMusic "musicname"<br/>
   * @param oper1 name
   */
  /* Plays music defined in the description.ext file.*/
  public native static void playMusic(String oper1);
  /** 
   * Plays music defined in the description.ext file. The format of nameAndPos is [name,position]. Position is in seconds.
<br/><br/>
   * <b>Example:</b> playMusic ["Track13", 30]<br/>
   * @param oper1 nameAndPos
   */
  /* Plays music defined in the description.ext file. The format of nameAndPos is [name,position]. Position is in seconds.*/
  public native static void playMusic(java.util.List oper1);
  /** 
   * Open the diary screen on the record specified by link.
   * @param oper1 link
   * @since 5500
   */
  /* Open the diary screen on the record specified by link.*/
  public native static void processDiaryLink(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engStressRT(float oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engStressRT(java.util.List oper1);
  /** 
   * Checks a side of the airport.
<br/><br/>
   * <b>Example:</b> airportSide 0<br/>
<br/>
   * <b>Example result:</b> west<br/>
   * @param oper1 airportId
   * @since 2.92
   */
  /* Checks a side of the airport.*/
  public native static void airportSide(float oper1);
  /** 
   * Returns the action assigned to the given button or active text. The action is any expression in this scripting language.
<br/><br/>
   * <b>Example:</b> _action = buttonAction _button<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Returns the action assigned to the given button or active text. The action is any expression in this scripting language.*/
  public native static String buttonAction(GameControl oper1);
  /** 
   * Returns the action assigned to the control with id idc of the topmost user dialog. The action is any expression in this scripting language. The function can be used for buttons and active texts.
<br/><br/>
   * <b>Example:</b> _action = buttonAction 100<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Returns the action assigned to the control with id idc of the topmost user dialog. The action is any expression in this scripting language. The function can be used for buttons and active texts.*/
  public native static String buttonAction(float oper1);
  /** 
   * Adds an item with the given text to the listbox or combobox with id idc of the topmost user dialog. It returns the index of the newly added item.
<br/><br/>
   * <b>Example:</b> _index = lbAdd [101, "First item"]<br/>
   * @param oper1 [idc, text]
   * @since 1.50
   */
  /* Adds an item with the given text to the listbox or combobox with id idc of the topmost user dialog. It returns the index of the newly added item.*/
  public native static float lbAdd(java.util.List oper1);
  /** 
   * The arctangens of x, the result is in degrees.
<br/><br/>
   * <b>Example:</b> atg 1<br/>
<br/>
   * <b>Example result:</b> 45<br/>
   * @param oper1 x
   */
  /* The arctangens of x, the result is in degrees.*/
  public native static float atg(float oper1);
  /** 
   * When used on a flag, the returned value is the person that has this flag.
When used on anything else, objNull is returned.
<br/><br/>
   * <b>Example:</b> flagowner flagOne<br/>
   * @param oper1 flag
   */
  /* When used on a flag, the returned value is the person that has this flag.
When used on anything else, objNull is returned.*/
  public native static GameObject flagOwner(GameObject oper1);
  /** 
   * Converts x from radians to degrees.
<br/><br/>
   * <b>Example:</b> deg 1<br/>
<br/>
   * <b>Example result:</b> 57.295<br/>
   * @param oper1 x
   */
  /* Converts x from radians to degrees.*/
  public native static float deg(float oper1);
  /** 
   * Returns the number of weapons of type name in the weapon pool (used in campaigns to transfer weapons to the next mission).
   * @param oper1 name
   * @since 1.75
   */
  /* Returns the number of weapons of type name in the weapon pool (used in campaigns to transfer weapons to the next mission).*/
  public native static float queryWeaponPool(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_stat(String oper1);
  /** 
   * Adds an column at given position. It returns the index of the newly added column.
<br/><br/>
   * <b>Example:</b> _index = lnbAddColumn [105, 0.8]<br/>
   * @param oper1 [idc, position]
   * @since 1.50
   */
  /* Adds an column at given position. It returns the index of the newly added column.*/
  public native static float lnbAddColumn(java.util.List oper1);
  /** 
   * The square root of x.
<br/><br/>
   * <b>Example:</b> sqrt 9<br/>
<br/>
   * <b>Example result:</b> 3<br/>
   * @param oper1 x
   */
  /* The square root of x.*/
  public native static float sqrt(float oper1);
  /** 
   * Returns type-name of expression. Type is returned as string
<br/><br/>
   * <b>Example:</b> typeName "hello"<br/>
<br/>
   * <b>Example result:</b> "string"<br/>
   * @param oper1 any
   * @since 2.00
   */
  /* Returns type-name of expression. Type is returned as string*/
  public native static String typeName(Object oper1);
  /** 
   * Returns the soldier assigned to the given vehicle as a gunner.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Returns the soldier assigned to the given vehicle as a gunner.*/
  public native static GameObject assignedGunner(GameObject oper1);
  /** 
   * Creates a structured text by joining the given structured or plain texts.
<br/><br/>
   * <b>Example:</b> txt = composeText ["First line", image "data\isniper.paa", lineBreak, "Second line"]<br/>
   * @param oper1 [text1, text2, ...]
   * @since 2.01
   */
  /* Creates a structured text by joining the given structured or plain texts.*/
  public native static GameText composeText(java.util.List oper1);
  /** 
   * Check if config entry represents array.
<br/><br/>
   * <b>Example:</b> _ok = isArray (configFile >> "CfgVehicles")<br/>
<br/>
   * <b>Example result:</b> false<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Check if config entry represents array.*/
  public native static boolean isArray(GameConfig oper1);
  /** 
   * Returns the picture name of the item with the given index of the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _picture = lbPicture [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Returns the picture name of the item with the given index of the listbox or combobox with id idc of the topmost user dialog.*/
  public native static String lbPicture(java.util.List oper1);
  /** 
   * Check if the control animation is finished.
<br/><br/>
   * <b>Example:</b> _done = ctrlCommitted _control<br/>
   * @param oper1 control
   * @since 2.50
   */
  /* Check if the control animation is finished.*/
  public native static boolean ctrlCommitted(GameControl oper1);
  /** 
   * Returns the current time of the most important RTM animation currently being played on the soldier.
<br/><br/>
   * <b>Example:</b> moveTime player<br/>
   * @param oper1 soldier
   * @since 5500
   */
  /* Returns the current time of the most important RTM animation currently being played on the soldier.*/
  public native static float moveTime(GameObject oper1);
  /** 
   * Checks which team does the vehicle (its commander unit) belong to.
<br/><br/>
   * <b>Example:</b> assignedTeam soldier2<br/>
<br/>
   * <b>Example result:</b> MAIN<br/>
   * @param oper1 vehicle
   * @since 2.05
   */
  /* Checks which team does the vehicle (its commander unit) belong to.*/
  public native static String assignedTeam(GameObject oper1);
  /** 
   * Sets the actual mission date and time.
   * @param oper1 [year, month, day, hour, minute]
   * @since 1.86
   */
  /* Sets the actual mission date and time.*/
  public native static void setDate(java.util.List oper1);
  /** 
   * Creates an (independent) agent (person) of the given type (type is a name of a subclass of CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The unit is placed inside a circle with this position as its center and placement as its radius. Special properties can be: "NONE" and "FORM".
<br/><br/>
   * <b>Example:</b> agent = createAgent ["SoldierWB", position player, [], 0, "FORM"]<br/>
   * @param oper1 [type, position, markers, placement, special]
   * @since 2.89
   */
  /* Creates an (independent) agent (person) of the given type (type is a name of a subclass of CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The unit is placed inside a circle with this position as its center and placement as its radius. Special properties can be: "NONE" and "FORM".*/
  public native static GameObject createAgent(java.util.List oper1);
  /** 
   * Clears the map animation.
   * @param oper1 control
   * @since 2.92
   */
  /* Clears the map animation.*/
  public native static void ctrlMapAnimClear(GameControl oper1);
  /** 
   * Returns true if manul fire is on.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Returns true if manul fire is on.*/
  public native static boolean isManualFire(GameObject oper1);
  /** 
   * Returns the text color of the item with the given index of the listbox or combobox with id idc of the topmost user dialog. The color is returned in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _color = lbColor [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Returns the text color of the item with the given index of the listbox or combobox with id idc of the topmost user dialog. The color is returned in format <ar>Color</ar>.*/
  public native static java.util.List lbColor(java.util.List oper1);
  /** 
   * Defines an action performed when HC group has been selected. Command receives <br/>
_group <t>group</t> selected group <br/>
_isSelected <t>bool</t> new selection state
   * @param oper1 command
   * @since 5501
   */
  /* Defines an action performed when HC group has been selected. Command receives <br/>
_group <t>group</t> selected group <br/>
_isSelected <t>bool</t> new selection state*/
  public native static void onHCGroupSelectionChanged(Object oper1);
  /** 
   * Adds list of rows of strings.
<br/><br/>
   * <b>Example:</b> _index = lnbAddArray  [idc,[[[text,text,text], [value], [data]], [[text,text,text], [value, value], [data]]]] . Returns row index.<br/>
   * @param oper1 [IDC,[[[text,text], [value,..], [data,..]], [[text,text], [value,..], [data,..]],]]
   * @since 5501
   */
  /* Adds list of rows of strings.*/
  public native static float lnbAddArray(java.util.List oper1);
  /** 
   * Returns the driver of the vehicle. If the vehicle is not a vehicle, but a person, the person is returned instead.
<br/><br/>
   * <b>Example:</b> driver vehicle player<br/>
   * @param oper1 vehicle
   */
  /* Returns the driver of the vehicle. If the vehicle is not a vehicle, but a person, the person is returned instead.*/
  public native static GameObject driver(GameObject oper1);
  /** 
   * Return the name of the current primary animation.
   * @param oper1 man
   * @since 2.92
   */
  /* Return the name of the current primary animation.*/
  public native static String animationState(GameObject oper1);
  /** 
   * Return the direction unit watching in formation.
   * @param oper1 person
   * @since 2.92
   */
  /* Return the direction unit watching in formation.*/
  public native static float formationDirection(GameObject oper1);
  /** 
   * Text title - the argument uses format ["text","type",speed] or ["text","type"]. If speed is not given, it's assumed to be one.
<br/><br/>
   * <b>Example:</b> titleText ["Show this text", "PLAIN"]<br/>
   * @param oper1 effect
   */
  /* Text title - the argument uses format ["text","type",speed] or ["text","type"]. If speed is not given, it's assumed to be one.*/
  public native static void titleText(java.util.List oper1);
  /** 
   * Return the state of input devices mapped to given input action (see CfgDefaultKeysMapping for action names).
   * @param oper1 name
   * @since 5500
   */
  /* Return the state of input devices mapped to given input action (see CfgDefaultKeysMapping for action names).*/
  public native static float inputAction(String oper1);
  /** 
   * Returns the current weapon state as an array of strings in the follow format [WeaponName, MuzzleName, ModeName, MagazineName].
<br/><br/>
   * <b>Example:</b> weaponState player or weaponState [vehicle player,[0]]<br/>
   * @param oper1 Gunner or [veh,turret path]
   * @since 5790
   */
  /* Returns the current weapon state as an array of strings in the follow format [WeaponName, MuzzleName, ModeName, MagazineName].*/
  public native static java.util.List weaponState(Object oper1);
  /** 
   * Starts video capturing.
   * @param oper1 [folderName, filePrefix, frameDuration, timeToReadResources]
   * @since 5164
   */
  /* Starts video capturing.*/
  public native static void diag_startVideo(java.util.List oper1);
  /** 
   * Return all simple tasks assigned to given person.
   * @param oper1 person
   * @since 5501
   */
  /* Return all simple tasks assigned to given person.*/
  public native static java.util.List simpleTasks(GameObject oper1);
  /** 
   * Returns the current scale of control.
<br/><br/>
   * <b>Example:</b> _scale = ctrlScale _control<br/>
   * @param oper1 control
   * @since 5501
   */
  /* Returns the current scale of control.*/
  public native static float ctrlScale(GameControl oper1);
  /** 
   * Adds a point guarded by the given side. If idStatic is not negative, the position of a static object with the given id is guarded. If the given vehicle is valid, the starting position of the vehicle is guarded, otherwise the given position is guarded.
<br/><br/>
   * <b>Example:</b> point = createGuardedPoint [East, [0, 0], -1, vehicle player]<br/>
   * @param oper1 [side, position, idStatic, vehicle]
   * @since 1.86
   */
  /* Adds a point guarded by the given side. If idStatic is not negative, the position of a static object with the given id is guarded. If the given vehicle is valid, the starting position of the vehicle is guarded, otherwise the given position is guarded.*/
  public native static void createGuardedPoint(java.util.List oper1);
  /** 
   * Returns zeroing of unit's weapon.
   * @param oper1 gunner
   * @since 5501
   */
  /* Returns zeroing of unit's weapon.*/
  public native static float currentZeroing(GameObject oper1);
  /** 
   * Gets the waypoint speed.
<br/><br/>
   * <b>Example:</b> waypointSpeed [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint speed.*/
  public native static String waypointSpeed(java.util.List oper1);
  /** 
   * Plays the map animation.
   * @param oper1 control
   * @since 2.92
   */
  /* Plays the map animation.*/
  public native static void ctrlMapAnimCommit(GameControl oper1);
  /** 
   * Finish the mission. The end type can be "CONTINUE", "KILLED", "LOSER", "END1", "END2", "END3", "END4", "END5", or "END6". Mission saves will not be deleted.
   * @param oper1 end type
   * @since 5501
   */
  /* Finish the mission. The end type can be "CONTINUE", "KILLED", "LOSER", "END1", "END2", "END3", "END4", "END5", or "END6". Mission saves will not be deleted.*/
  public native static void failMission(String oper1);
  /** 
   * Gets the type of the marker. See <f>setMarkerType</f>.
Note: this function is identical to <f>getMarkerType</f>.
<br/><br/>
   * <b>Example:</b> markerType "MarkerOne"<br/>
   * @param oper1 markerName
   * @since 1.50
   */
  /* Gets the type of the marker. See <f>setMarkerType</f>.
Note: this function is identical to <f>getMarkerType</f>.*/
  public native static String markerType(String oper1);
  /** 
   * Returns the name of a vehicle's secondary weapon (an empty string if there is none).
<br/><br/>
   * <b>Example:</b> secondaryWeapon player<br/>
   * @param oper1 vehicle
   * @since 1.75
   */
  /* Returns the name of a vehicle's secondary weapon (an empty string if there is none).*/
  public native static String secondaryWeapon(GameObject oper1);
  /** 
   * Adds text into the events section of the mpreport.txt file.
   * @param oper1 text
   * @since 1.80
   */
  /* Adds text into the events section of the mpreport.txt file.*/
  public native static void VBS_addEvent(String oper1);
  /** 
   * Return the position in vehicle person is assigned to.
   * @param oper1 person
   * @since 5130
   */
  /* Return the position in vehicle person is assigned to.*/
  public native static java.util.List assignedVehicleRole(GameObject oper1);
  /** 
   * Returns the object heading in the range of 0 to 360.
<br/><br/>
   * <b>Example:</b> direction player<br/>
   * @param oper1 object
   * @since 1.50
   */
  /* Returns the object heading in the range of 0 to 360.*/
  public native static float direction(GameObject oper1);
  /** 
   * Return direction (angle) of given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return direction (angle) of given location.*/
  public native static float direction(GameLocation oper1);
  /** 
   * The arcsine of x, the result is in degrees.
<br/><br/>
   * <b>Example:</b> asin 0.5<br/>
<br/>
   * <b>Example result:</b> 30<br/>
   * @param oper1 x
   */
  /* The arcsine of x, the result is in degrees.*/
  public native static float asin(float oper1);
  /** 
   * Clears all items in the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> lbClear _control<br/>
   * @param oper1 control
   * @since 2.91
   */
  /* Clears all items in the given listbox or combobox.*/
  public native static void lnbClear(GameControl oper1);
  /** 
   * Clears all items in the 2D listbox.
<br/><br/>
   * <b>Example:</b> lnbClear 101<br/>
   * @param oper1 idc
   * @since 5501
   */
  /* Clears all items in the 2D listbox.*/
  public native static void lnbClear(float oper1);
  /** 
   * Creates a new AI group for the given center (side).
<br/><br/>
   * <b>Example:</b> group = createGroup East<br/>
   * @param oper1 side
   * @since 1.86
   */
  /* Creates a new AI group for the given center (side).*/
  public native static GameGroup createGroup(GameSide oper1);
  /** 
   * Adds the next frame to the map animation. The format of frame is [time, zoom, position], the format of position is <ar>Position2D</ar>.
<br/><br/>
   * <b>Example:</b> mapAnimAdd [1, 0.1, getMarkerPos "anim1"]<br/>
   * @param oper1 frame
   * @since 1.27
   */
  /* Adds the next frame to the map animation. The format of frame is [time, zoom, position], the format of position is <ar>Position2D</ar>.*/
  public native static void mapAnimAdd(java.util.List oper1);
  /** 
   * Reload all weapons.
   * @param oper1 vehicle
   * @since 2.92
   */
  /* Reload all weapons.*/
  public native static void reload(GameObject oper1);
  /** 
   * Shows or hides HC bar. There must be some groups under hc command to show hc bar.
<br/><br/>
   * <b>Example:</b> hcShowBar true<br/>
   * @param oper1 bool
   * @since 5501
   */
  /* Shows or hides HC bar. There must be some groups under hc command to show hc bar.*/
  public native static void hcShowBar(boolean oper1);
  /** 
   * Checks the unit rating. This rating is increased for killing enemies, decreased for killing friendlies and can be changed by a mission designer.
<br/><br/>
   * <b>Example:</b> rating player<br/>
   * @param oper1 unit
   */
  /* Checks the unit rating. This rating is increased for killing enemies, decreased for killing friendlies and can be changed by a mission designer.*/
  public native static float rating(GameObject oper1);
  /** 
   * Dissolves the given team. All members become members of the main team. Possible team values are: "RED", "GREEN", "BLUE" or "YELLOW".
<br/><br/>
   * <b>Example:</b> dissolveTeam "RED"<br/>
   * @param oper1 team
   * @since 2.05
   */
  /* Dissolves the given team. All members become members of the main team. Possible team values are: "RED", "GREEN", "BLUE" or "YELLOW".*/
  public native static void dissolveTeam(String oper1);
  /** 
   * Returns the next available menu item index.
   * @param oper1 map
   * @since 2.92
   */
  /* Returns the next available menu item index.*/
  public native static float nextMenuItemIndex(GameControl oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engAdjustTree(java.util.List oper1);
  /** 
   * Returns whether given control is shown.
<br/><br/>
   * <b>Example:</b> _ok = ctrlShown _control<br/>
   * @param oper1 control
   * @since 2.50
   */
  /* Returns whether given control is shown.*/
  public native static boolean ctrlShown(GameControl oper1);
  /** 
   * Returns target created by gunner in given vehicle.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Returns target created by gunner in given vehicle.*/
  public native static GameObject laserTarget(GameObject oper1);
  /** 
   * Gets the waypoint position. Waypoint uses format <ar>Waypoint</ar>.
Note: this function is identical to <f>getWaypointPosition</f>.
<br/><br/>
   * <b>Example:</b> waypointPosition [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 1.50
   */
  /* Gets the waypoint position. Waypoint uses format <ar>Waypoint</ar>.
Note: this function is identical to <f>getWaypointPosition</f>.*/
  public native static java.util.List waypointPosition(java.util.List oper1);
  /** 
   * Replaces the string with the given name with the corresponding localized text from the stringtable.csv file.
<br/><br/>
   * <b>Example:</b> localize "STR_DN_FROG"<br/>
   * @param oper1 stringName
   */
  /* Replaces the string with the given name with the corresponding localized text from the stringtable.csv file.*/
  public native static String localize(String oper1);
  /** 
   * Assign a friendly name to the VM script thjis command is executed from.
<br/><br/>
   * <b>Example:</b> scriptName "Trading"<br/>
   * @param oper1 name
   * @since 5501
   */
  /* Assign a friendly name to the VM script thjis command is executed from.*/
  public native static void scriptName(String oper1);
  /** 
   * Imports all groups into the RTE.
   * @param oper1 map
   * @since 2.92
   */
  /* Imports all groups into the RTE.*/
  public native static void importAllGroups(GameControl oper1);
  /** 
   * Destroys the given AI group.
<br/><br/>
   * <b>Example:</b> deleteGroup group<br/>
   * @param oper1 group
   */
  /* Destroys the given AI group.*/
  public native static void deleteGroup(GameGroup oper1);
  /** 
   * Return current task of given person.
   * @param oper1 person
   * @since 5501
   */
  /* Return current task of given person.*/
  public native static GameTask currentTask(GameObject oper1);
  /** 
   * Save the current overlay.
<br/><br/>
   * <b>Example:</b> saveOverlay _map<br/>
   * @param oper1 map
   * @since 2.92
   */
  /* Save the current overlay.*/
  public native static void saveOverlay(GameControl oper1);
  /** 
   * Enables/disables camera shakes.
   * @param oper1 true/false
   * @since 5501
   */
  /* Enables/disables camera shakes.*/
  public native static void enableCamShake(boolean oper1);
  /** 
   * Enable / disable showing of HUD.
   * @param oper1 enable
   * @since 5501
   */
  /* Enable / disable showing of HUD.*/
  public native static void showHUD(boolean oper1);
  /** 
   * Create post process effect specified by name and priority
   * @param oper1 effect
   * @since 5501
   */
  /* Create post process effect specified by name and priority*/
  public native static Object ppEffectCreate(java.util.List oper1);
  /** 
   * Transfers weapons and magazines from the weapon pool (used in campaigns to transfer weapons to the next mission) into the cargo space of object obj.
   * @param oper1 obj
   * @since 1.75
   */
  /* Transfers weapons and magazines from the weapon pool (used in campaigns to transfer weapons to the next mission) into the cargo space of object obj.*/
  public native static void putWeaponPool(GameObject oper1);
  /** 
   * Create a team and name it.
<br/><br/>
   * <b>Example:</b> _team = createTeam ["USMC_Team", "Fire Team Red"]<br/>
   * @param oper1 [type, name]
   * @since 2.90
   */
  /* Create a team and name it.*/
  public native static GameTeamMember createTeam(java.util.List oper1);
  /** 
   * Find the nearest location (to the given position) of given type.
   * @param oper1 [position, type]
   * @since 2.90
   */
  /* Find the nearest location (to the given position) of given type.*/
  public native static GameLocation nearestLocation(java.util.List oper1);
  /** 
   * Orders the unit to get out from the vehicle (via the radio).
<br/><br/>
   * <b>Example:</b> commandGetOut unitOne<br/>
   * @param oper1 unit
   * @since 2.28
   */
  /* Orders the unit to get out from the vehicle (via the radio).*/
  public native static void commandGetOut(Object oper1);
  /** 
   * Find the road segments connected to the given road segment.
   * @param oper1 road segment
   * @since 5500
   */
  /* Find the road segments connected to the given road segment.*/
  public native static java.util.List roadsConnectedTo(GameObject oper1);
  /** 
   * Hide object (cannot hide static objects).
   * @param oper1 object
   * @since 2.90
   */
  /* Hide object (cannot hide static objects).*/
  public native static void hideObject(GameObject oper1);
  /** 
   * Returns trigger timeout in the form [min, mid, max, interruptable]
   * @param oper1 trigger
   * @since 5501
   */
  /* Returns trigger timeout in the form [min, mid, max, interruptable]*/
  public native static java.util.List triggerTimeout(GameObject oper1);
  /** 
   * Return if given team was inserted directly from mission editor.
<br/><br/>
   * <b>Example:</b> _fromEditor = fromEditor _member<br/>
   * @param oper1 teamMember
   * @since 2.90
   */
  /* Return if given team was inserted directly from mission editor.*/
  public native static boolean fromEditor(GameTeamMember oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engTexMemLimit(float oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_reloadCurves(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_densMan(float oper1);
  /** 
   * Saves the variable value into the global campaign space. The variable is then available to all following missions in the campaign.
<br/><br/>
   * <b>Example:</b> saveVar "varOne"<br/>
   * @param oper1 varName
   */
  /* Saves the variable value into the global campaign space. The variable is then available to all following missions in the campaign.*/
  public native static void saveVar(String oper1);
  /** 
   * Creates a structured text by parsing the given XML description.
<br/><br/>
   * <b>Example:</b> txt = parseText "First line&lt;img image=data\isniper.paa/&gt;&ltbr/&gt;Second line"<br/>
   * @param oper1 text
   * @since 2.01
   */
  /* Creates a structured text by parsing the given XML description.*/
  public native static GameText parseText(String oper1);
  /** 
   * Defines an action performed when palyer clicked on group marker (3D or in a map)
   * @param oper1 command
   * @since 5501
   */
  /* Defines an action performed when palyer clicked on group marker (3D or in a map)*/
  public native static void onGroupIconOverEnter(Object oper1);
  /** 
   * Returns the object position in format <ar>PositionATL</ar>.
<br/><br/>
   * <b>Example:</b> getPosATL player<br/>
   * @param oper1 obj
   * @since 2.53
   */
  /* Returns the object position in format <ar>PositionATL</ar>.*/
  public native static java.util.List getPosATL(GameObject oper1);
  /** 
   * Suspend execution of script for given time.
<br/><br/>
   * <b>Example:</b> sleep 0.5<br/>
   * @param oper1 delay
   * @since 2.60
   */
  /* Suspend execution of script for given time.*/
  public native static void sleep(float oper1);
  /** 
   * The cosine of x, the argument is in degrees.
<br/><br/>
   * <b>Example:</b> cos 60<br/>
<br/>
   * <b>Example result:</b> 0.5<br/>
   * @param oper1 x
   */
  /* The cosine of x, the argument is in degrees.*/
  public native static float cos(float oper1);
  /** 
   * Checks whether the unit has some ammo remaining.
<br/><br/>
   * <b>Example:</b> someAmmo vehicle player<br/>
   * @param oper1 unit
   */
  /* Checks whether the unit has some ammo remaining.*/
  public native static boolean someAmmo(GameObject oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engAdjustTreeExt(java.util.List oper1);
  /** 
   * Check if file is signed by any key present in game keys folders. Note: On client, it does not check against the keys accepted by server.
   * @param oper1 filename
   * @since 2.91
   */
  /* Check if file is signed by any key present in game keys folders. Note: On client, it does not check against the keys accepted by server.*/
  public native static boolean verifySignature(String oper1);
  /** 
   * Commit the current overlay.
<br/><br/>
   * <b>Example:</b> commitOverlay _map<br/>
   * @param oper1 map
   * @since 2.92
   */
  /* Commit the current overlay.*/
  public native static void commitOverlay(GameControl oper1);
  /** 
   * Moves the top most JVM panel to a new position.
   * @param oper1 [x, y]
   * @since 86290
   */
  /* Moves the top most JVM panel to a new position.*/
  public native static void jMovePanel(java.util.List oper1);
  /** 
   * Creates a structured text containing the given plain text.
<br/><br/>
   * <b>Example:</b> txt2 = text "Hello world."<br/>
   * @param oper1 text
   * @since 2.01
   */
  /* Creates a structured text containing the given plain text.*/
  public native static GameText text(String oper1);
  /** 
   * Return text attached to given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return text attached to given location.*/
  public native static String text(GameLocation oper1);
  /** 
   * Clear the current overlay.
<br/><br/>
   * <b>Example:</b> clearOverlay _map<br/>
   * @param oper1 map
   * @since 2.92
   */
  /* Clear the current overlay.*/
  public native static void clearOverlay(GameControl oper1);
  /** 
   * Returns the unlocalized text value of a side as: East, West, Resistance, Civilian, or Unknown.
<br/><br/>
   * <b>Example:</b> WFSideText West<br/>
   * @param oper1 side
   * @since 1.04
   */
  /* Returns the unlocalized text value of a side as: East, West, Resistance, Civilian, or Unknown.*/
  public native static String WFSideText(GameSide oper1);
  /** 
   * Returns the unlocalized text value of a object's side as: East, West, Resistance, Civilian, or Unknown.
<br/><br/>
   * <b>Example:</b> WFSideText player<br/>
   * @param oper1 object
   * @since 5501
   */
  /* Returns the unlocalized text value of a object's side as: East, West, Resistance, Civilian, or Unknown.*/
  public native static String WFSideText(GameObject oper1);
  /** 
   * Returns the unlocalized text value of a group's side as: East, West, Resistance, Civilian, or Unknown.
<br/><br/>
   * <b>Example:</b> WFSideText group player<br/>
   * @param oper1 group
   * @since 5501
   */
  /* Returns the unlocalized text value of a group's side as: East, West, Resistance, Civilian, or Unknown.*/
  public native static String WFSideText(GameGroup oper1);
  /** 
   * Unassigns the vehicle (its commander unit) from his team. This is equal to vehicle assignTeam "MAIN".
<br/><br/>
   * <b>Example:</b> unassignTeam soldier2<br/>
   * @param oper1 vehicle
   * @since 2.05
   */
  /* Unassigns the vehicle (its commander unit) from his team. This is equal to vehicle assignTeam "MAIN".*/
  public native static void unassignTeam(GameObject oper1);
  /** 
   * Returns a list of button names assigned to the given user action. A maximum of maxKeys keys is listed. You can find the action names in config class ControllerSchemes.
<br/><br/>
   * <b>Example:</b> list = actionKeysNames "ReloadMagazine"<br/>
   * @param oper1 action or [action, maxKeys]
   * @since 2.01
   */
  /* Returns a list of button names assigned to the given user action. A maximum of maxKeys keys is listed. You can find the action names in config class ControllerSchemes.*/
  public native static String actionKeysNames(Object oper1);
  /** 
   * Returns a list of button names assigned to the given user action. A maximum of maxKeys keys is listed. You can find the action names in config class ControllerSchemes.
<br/><br/>
   * <b>Example:</b> list = actionKeysNames "ReloadMagazine"<br/>
   * @param oper1 action or [action, maxKeys]
   * @since 2.01
   */
  /* Returns a list of button names assigned to the given user action. A maximum of maxKeys keys is listed. You can find the action names in config class ControllerSchemes.*/
  public native static java.util.List actionKeysNamesArray(Object oper1);
  /** 
   * Check if latest low level moveTo command failed.
   * @param oper1 person
   * @since 2.92
   */
  /* Check if latest low level moveTo command failed.*/
  public native static boolean moveToFailed(GameObject oper1);
  /** 
   * The arctangens of x, the result is in degrees.
<br/><br/>
   * <b>Example:</b> atan 1<br/>
<br/>
   * <b>Example result:</b> 45<br/>
   * @param oper1 x
   */
  /* The arctangens of x, the result is in degrees.*/
  public native static float atan(float oper1);
  /** 
   * Returns the object damage in a range of 0 to 1. Note: this function is identical to <f>getDammage</f>.
<br/><br/>
   * <b>Example:</b> damage player<br/>
   * @param oper1 object
   * @since 1.50
   */
  /* Returns the object damage in a range of 0 to 1. Note: this function is identical to <f>getDammage</f>.*/
  public native static float damage(GameObject oper1);
  /** 
   * Collapse the object tree.
<br/><br/>
   * <b>Example:</b> collapseObjectTree _map<br/>
   * @param oper1 map
   * @since 2.92
   */
  /* Collapse the object tree.*/
  public native static void collapseObjectTree(GameControl oper1);
  /** 
   * Returns the side of the unit.
<br/><br/>
   * <b>Example:</b> side player == west<br/>
   * @param oper1 unit
   */
  /* Returns the side of the unit.*/
  public native static GameSide side(GameObject oper1);
  /** 
   * Returns the side of the group.
<br/><br/>
   * <b>Example:</b> side group player == west<br/>
   * @param oper1 group
   * @since 5501
   */
  /* Returns the side of the group.*/
  public native static GameSide side(GameGroup oper1);
  /** 
   * Return target side of given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return target side of given location.*/
  public native static GameSide side(GameLocation oper1);
  /** 
   * Resource title - the argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
Resource can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> titleRsc ["BIS", "PLAIN"]<br/>
   * @param oper1 effect
   */
  /* Resource title - the argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
Resource can be defined in the description.ext file.*/
  public native static void titleRsc(java.util.List oper1);
  /** 
   * Defines an action performed when palyer clicked on group marker (3D or in a map)
   * @param oper1 command
   * @since 5501
   */
  /* Defines an action performed when palyer clicked on group marker (3D or in a map)*/
  public native static void onGroupIconClick(Object oper1);
  /** 
   * Checks whether the unit is ready. A unit is busy when it's given an order like "move", until the command is finished.
<br/><br/>
   * <b>Example:</b> unitReady player<br/>
   * @param oper1 unit
   */
  /* Checks whether the unit is ready. A unit is busy when it's given an order like "move", until the command is finished.*/
  public native static boolean unitReady(Object oper1);
  /** 
   * Sets the desired terrain resolution (in meters). For default landscapes the supported resolutions are: 50, 25, 12.5, 6.25 and 3.125. If you select an unsupported resolution, the nearest supported resolution is used instead.
<br/><br/>
   * <b>Example:</b> setTerrainGrid 12.5<br/>
   * @param oper1 grid
   * @since 1.75
   */
  /* Sets the desired terrain resolution (in meters). For default landscapes the supported resolutions are: 50, 25, 12.5, 6.25 and 3.125. If you select an unsupported resolution, the nearest supported resolution is used instead.*/
  public native static void setTerrainGrid(float oper1);
  /** 
   * Checks whether the value is equal to objNull.
Note: a==ObjNull does not work, because objNull is not equal to anything, even to itself.
<br/><br/>
   * <b>Example:</b> isNull objNull<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 obj
   */
  /* Checks whether the value is equal to objNull.
Note: a==ObjNull does not work, because objNull is not equal to anything, even to itself.*/
  public native static boolean isNull(GameObject oper1);
  /** 
   * Checks whether the value is equal to grpNull.
Note: a==GrpNull does not work, because grpNull is not equal to anything, even to itself.
<br/><br/>
   * <b>Example:</b> isNull group player<br/>
<br/>
   * <b>Example result:</b> false<br/>
   * @param oper1 grp
   */
  /* Checks whether the value is equal to grpNull.
Note: a==GrpNull does not work, because grpNull is not equal to anything, even to itself.*/
  public native static boolean isNull(GameGroup oper1);
  /** 
   * Checks whether the value is equal to controlNull.
Note: a==controlNull does not work, because controlNull is not equal to anything, even to itself.
<br/><br/>
   * <b>Example:</b> isNull controlNull<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Checks whether the value is equal to controlNull.
Note: a==controlNull does not work, because controlNull is not equal to anything, even to itself.*/
  public native static boolean isNull(GameControl oper1);
  /** 
   * Checks whether the value is equal to displayNull.
Note: a==displayNull does not work, because displayNull is not equal to anything, even to itself.
<br/><br/>
   * <b>Example:</b> isNull displayNull<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 display
   * @since 2.92
   */
  /* Checks whether the value is equal to displayNull.
Note: a==displayNull does not work, because displayNull is not equal to anything, even to itself.*/
  public native static boolean isNull(GameDisplay oper1);
  /** 
   * Checks whether the value is equal to taskNull.
Note: a==TaskNull does not work, because taskNull is not equal to anything, even to itself.
<br/><br/>
   * <b>Example:</b> isNull taskNull<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 task
   * @since 5160
   */
  /* Checks whether the value is equal to taskNull.
Note: a==TaskNull does not work, because taskNull is not equal to anything, even to itself.*/
  public native static boolean isNull(GameTask oper1);
  /** 
   * Check whether the value is null.
   * @param oper1 location
   * @since 5501
   */
  /* Check whether the value is null.*/
  public native static boolean isNull(GameLocation oper1);
  /** 
   * Returns trigger type.
   * @param oper1 trigger
   * @since 5501
   */
  /* Returns trigger type.*/
  public native static String triggerType(GameObject oper1);
  /** 
   * Defines a sound (voice) that is played the first time when the Team switch section in the briefing is selected.
<br/><br/>
   * <b>Example:</b> onBriefingTeamSwitch "TeamSwitchVoiceOver"<br/>
   * @param oper1 sound
   * @since 2.92
   */
  /* Defines a sound (voice) that is played the first time when the Team switch section in the briefing is selected.*/
  public native static void onBriefingTeamSwitch(String oper1);
  /** 
   * This statement is launched whenever a player is disconnected from a MP session. Variables _id, _name and _uid are set.
   * @param oper1 statement
   * @since 2.10
   */
  /* This statement is launched whenever a player is disconnected from a MP session. Variables _id, _name and _uid are set.*/
  public native static void onPlayerDisconnected(Object oper1);
  /** 
   * Returns the formation leader for the given unit. For dead units objNull is returned. The result is often the same as groupLeader, but not always.
<br/><br/>
   * <b>Example:</b> formLeader player != leader player<br/>
   * @param oper1 unit
   */
  /* Returns the formation leader for the given unit. For dead units objNull is returned. The result is often the same as groupLeader, but not always.*/
  public native static GameObject formLeader(GameObject oper1);
  /** 
   * Returns the content of the given file.
<br/><br/>
   * <b>Example:</b> loadFile "myFunction.sqf"<br/>
<br/>
   * <b>Example result:</b> "if a>b then {a} else {b}"<br/>
   * @param oper1 filename
   * @since 1.82
   */
  /* Returns the content of the given file.*/
  public native static String loadFile(String oper1);
  /** 
   * Check if given AI feature is currently enabled.
   * @param oper1 feature
   * @since 60220
   */
  /* Check if given AI feature is currently enabled.*/
  public native static boolean checkAIFeature(String oper1);
  /** 
   * Check whether given position is on road.
   * @param oper1 position or object
   * @since 5501
   */
  /* Check whether given position is on road.*/
  public native static boolean isOnRoad(Object oper1);
  /** 
   * Obsolete.
   * @param oper1 effect
   */
  /* Obsolete.*/
  public native static void titleCut(java.util.List oper1);
  /** 
   * Defines an action performed when commnad mode change. Command receives <br/>
_isHighCommand <t>bool</t> 
   * @param oper1 command
   * @since 5501
   */
  /* Defines an action performed when commnad mode change. Command receives <br/>
_isHighCommand <t>bool</t> */
  public native static void onCommandModeChanged(Object oper1);
  /** 
   * Return the target assigned to the vehicle.
   * @param oper1 vehicle
   * @since 2.92
   */
  /* Return the target assigned to the vehicle.*/
  public native static GameObject assignedTarget(GameObject oper1);
  /** 
   * Return a list of members in given team.
<br/><br/>
   * <b>Example:</b> _members = members _team<br/>
   * @param oper1 team
   * @since 2.90
   */
  /* Return a list of members in given team.*/
  public native static java.util.List members(GameTeamMember oper1);
  /** 
   * Captures a screenshot and stores it to file filename.
   * @param oper1 filename
   * @since 1.85
   */
  /* Captures a screenshot and stores it to file filename.*/
  public native static void diag_screenshot(String oper1);
  /** 
   * Returns trigger area in the form [a, b, angle, rectangle]
   * @param oper1 trigger
   * @since 5501
   */
  /* Returns trigger area in the form [a, b, angle, rectangle]*/
  public native static java.util.List triggerArea(GameObject oper1);
  /** 
   * Same as <f>hint</f>, but without a sound.
   * @param oper1 text
   * @since 5501
   */
  /* Same as <f>hint</f>, but without a sound.*/
  public native static void hintSilent(Object oper1);
  /** 
   * The natural logarithm of x.
<br/><br/>
   * <b>Example:</b> ln 10<br/>
<br/>
   * <b>Example result:</b> 2.302<br/>
   * @param oper1 x
   */
  /* The natural logarithm of x.*/
  public native static float ln(float oper1);
  /** 
   * Sets the additional integer value in the item with the given index of the listbox or combobox with id idc of the topmost user dialog to the given value.
<br/><br/>
   * <b>Example:</b> lbSetValue [101, 0, 1]<br/>
   * @param oper1 [idc, index, value]
   * @since 1.50
   */
  /* Sets the additional integer value in the item with the given index of the listbox or combobox with id idc of the topmost user dialog to the given value.*/
  public native static void lbSetValue(java.util.List oper1);
  /** 
   * Move (UI) mouse pointer to specified position of the screen.
<br/><br/>
   * <b>Example:</b> setMousePosition [0.5, 0.5]<br/>
   * @param oper1 [x, y]
   * @since 2.91
   */
  /* Move (UI) mouse pointer to specified position of the screen.*/
  public native static void setMousePosition(java.util.List oper1);
  /** 
   * Check if config entry represents config class.
<br/><br/>
   * <b>Example:</b> _ok = isClass (configFile >> "CfgVehicles")<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Check if config entry represents config class.*/
  public native static boolean isClass(GameConfig oper1);
  /** 
   * Return a person for given agent.
<br/><br/>
   * <b>Example:</b> _person = agent _agent<br/>
   * @param oper1 agent
   * @since 2.90
   */
  /* Return a person for given agent.*/
  public native static GameObject agent(GameTeamMember oper1);
  /** 
   * Returns a list of all deaths of the given unit. Items are in format <ar>VBS_Kill</ar>.
   * @param oper1 name
   * @since 1.80
   */
  /* Returns a list of all deaths of the given unit. Items are in format <ar>VBS_Kill</ar>.*/
  public native static java.util.List VBS_killed(String oper1);
  /** 
   * Return if task is completed. (state Succeeded, Failed or Canceled)
   * @param oper1 task
   * @since 2.89
   */
  /* Return if task is completed. (state Succeeded, Failed or Canceled)*/
  public native static boolean taskCompleted(GameTask oper1);
  /** 
   * Return position of given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return position of given location.*/
  public native static java.util.List locationPosition(GameLocation oper1);
  /** 
   * Shows info about new,changed or failed task. The text can contain several lines. \n is used to indicate the end of a line.
<br/><br/>
   * <b>Example:</b> taskHint "Capture town."<br/>
   * @param oper1 text
   */
  /* Shows info about new,changed or failed task. The text can contain several lines. \n is used to indicate the end of a line.*/
  public native static void taskHint(java.util.List oper1);
  /** 
   * Gets the marker size. See <f>setMarkerSize</f>.
<br/><br/>
   * <b>Example:</b> getMarkerSize "MarkerOne"<br/>
   * @param oper1 marker
   * @since 1.21
   */
  /* Gets the marker size. See <f>setMarkerSize</f>.*/
  public native static java.util.List getMarkerSize(String oper1);
  /** 
   * Convert float number to date.
<br/><br/>
   * <b>Example:</b> date = NumberToDate [2008,0.5324]<br/>
   * @param oper1 [year,time]
   * @since 5501
   */
  /* Convert float number to date.*/
  public native static java.util.List NumberToDate(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_setLight(String oper1);
  /** 
   * Gets unit faction. If faction is not defined, returns empty string.
   * @param oper1 unit
   * @since 5501
   */
  /* Gets unit faction. If faction is not defined, returns empty string.*/
  public native static String faction(GameObject oper1);
  /** 
   * Returns true if player is forced to walk
   * @param oper1 player
   * @since 5501
   */
  /* Returns true if player is forced to walk*/
  public native static boolean isForcedWalk(GameObject oper1);
  /** 
   * Returns the group to which the given unit is assigned. For dead units, grpNull is returned.
<br/><br/>
   * <b>Example:</b> group player == group leader player<br/>
   * @param oper1 obj
   */
  /* Returns the group to which the given unit is assigned. For dead units, grpNull is returned.*/
  public native static GameGroup group(GameObject oper1);
  /** 
   * Returns vision mode of unit's weapon. 0-dayTime, 1-night vision, 2-FLIR 
   * @param oper1 gunner
   * @since 5501
   */
  /* Returns vision mode of unit's weapon. 0-dayTime, 1-night vision, 2-FLIR */
  public native static float currentVisionMode(GameObject oper1);
  /** 
   * Returns the shown text in the item with the given index of the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _text = lbText [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Returns the shown text in the item with the given index of the listbox or combobox with id idc of the topmost user dialog.*/
  public native static String lbText(java.util.List oper1);
  /** 
   * Returns gear slot item name.
<br/><br/>
   * <b>Example:</b> weapon = gearSlotData _control<br/>
   * @param oper1 control
   * @since 5501
   */
  /* Returns gear slot item name.*/
  public native static String gearSlotData(GameControl oper1);
  /** 
   * Return the list of objects synchronized with the given unit.
   * @param oper1 unit
   * @since 5501
   */
  /* Return the list of objects synchronized with the given unit.*/
  public native static java.util.List synchronizedObjects(GameObject oper1);
  /** 
   * Terminate (abort) the script
   * @param oper1 script
   * @since 2.50
   */
  /* Terminate (abort) the script*/
  public native static void terminate(GameScript oper1);
  /** 
   * Returns a list of nearest objects of the given types to the given position or object, within the specified distance.
Pos may be using format [x,y,z, ["type",...], limit] or [object, ["type",...], limit].
<br/><br/>
   * <b>Example:</b> nearestObjects [player, ["Car","Tank"], 200]<br/>
   * @param oper1 pos
   * @since 2.92
   */
  /* Returns a list of nearest objects of the given types to the given position or object, within the specified distance.
Pos may be using format [x,y,z, ["type",...], limit] or [object, ["type",...], limit].*/
  public native static java.util.List nearestObjects(java.util.List oper1);
  /** 
   * Set / clear using of night vision during cutscenes.
   * @param oper1 set
   * @since 2.73
   */
  /* Set / clear using of night vision during cutscenes.*/
  public native static void camUseNVG(boolean oper1);
  /** 
   * Drops a particle into the scene. Array is in format <ar>ParticleArray</ar>.
   * @param oper1 array
   * @since 1.50
   */
  /* Drops a particle into the scene. Array is in format <ar>ParticleArray</ar>.*/
  public native static void drop(java.util.List oper1);
  /** 
   * Delete the given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Delete the given location.*/
  public native static void deleteLocation(GameLocation oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engUseStaticVB(boolean oper1);
  /** 
   * The first argument of the array is in format string. This string may contain references to the following arguments using format %1, %2, etc. Each %x is replaced by the corresponding argument. %x may appear in the string in any order.
<br/><br/>
   * <b>Example:</b> format ["%1 - %2 - %1", 1, "text"]<br/>
<br/>
   * <b>Example result:</b> "1 - text - 1"<br/>
   * @param oper1 format
   */
  /* The first argument of the array is in format string. This string may contain references to the following arguments using format %1, %2, etc. Each %x is replaced by the corresponding argument. %x may appear in the string in any order.*/
  public native static String format(java.util.List oper1);
  /** 
   * True, if number is finite (not infinite and valid number)
<br/><br/>
   * <b>Example:</b> finite 10/0<br/>
<br/>
   * <b>Example result:</b> false<br/>
   * @param oper1 x
   */
  /* True, if number is finite (not infinite and valid number)*/
  public native static boolean finite(float oper1);
  /** 
   * Gets the house position assigned to the waypoint.
<br/><br/>
   * <b>Example:</b> waypointHousePosition [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the house position assigned to the waypoint.*/
  public native static float waypointHousePosition(java.util.List oper1);
  /** 
   * Fetches a reference to the mission editor camera.
   * @param oper1 map
   * @since 2.92
   */
  /* Fetches a reference to the mission editor camera.*/
  public native static GameObject getEditorCamera(GameControl oper1);
  /** 
   * Enables the map (the default is true).
<br/><br/>
   * <b>Example:</b> showMap true<br/>
   * @param oper1 show
   */
  /* Enables the map (the default is true).*/
  public native static void showMap(boolean oper1);
  /** 
   * Destroys the given marker. The marker is destroyed on all computers in a network session.
<br/><br/>
   * <b>Example:</b> deleteMarker "Marker1"<br/>
   * @param oper1 name
   */
  /* Destroys the given marker. The marker is destroyed on all computers in a network session.*/
  public native static void deleteMarker(String oper1);
  /** 
   * Converts position in world space into screen (UI) space.
   * @param oper1 position
   * @since 5501
   */
  /* Converts position in world space into screen (UI) space.*/
  public native static java.util.List worldToScreen(java.util.List oper1);
  /** 
   * Returns a list of units that would activate the given trigger. For a trigger of type "Not present" the list is the same as the list that would be returned for type "Present".
<br/><br/>
   * <b>Example:</b> list triggerOne<br/>
   * @param oper1 trigger
   */
  /* Returns a list of units that would activate the given trigger. For a trigger of type "Not present" the list is the same as the list that would be returned for type "Present".*/
  public native static java.util.List list(GameObject oper1);
  /** 
   * Returns the group leader for the given unit. For dead units, objNull is returned.
<br/><br/>
   * <b>Example:</b> leader player<br/>
   * @param oper1 unit
   */
  /* Returns the group leader for the given unit. For dead units, objNull is returned.*/
  public native static GameObject leader(GameObject oper1);
  /** 
   * Returns the group leader for the given group. For a dead unit, grpNull is returned.
<br/><br/>
   * <b>Example:</b> leader group player == leader player<br/>
   * @param oper1 grp
   */
  /* Returns the group leader for the given group. For a dead unit, grpNull is returned.*/
  public native static GameObject leader(GameGroup oper1);
  /** 
   * Return the leader of given team.
   * @param oper1 team
   * @since 2.92
   */
  /* Return the leader of given team.*/
  public native static GameTeamMember leader(GameTeamMember oper1);
  /** 
   * Return the name of the currently selected weapon (on the primary turret for vehicles).
   * @param oper1 vehicle
   * @since 5500
   */
  /* Return the name of the currently selected weapon (on the primary turret for vehicles).*/
  public native static String currentWeapon(GameObject oper1);
  /** 
   * Convert the string to the array of characters.
<br/><br/>
   * <b>Example:</b> toArray "ArmA"<br/>
<br/>
   * <b>Example result:</b> [65, 114, 109, 65]<br/>
   * @param oper1 string
   * @since 5195
   */
  /* Convert the string to the array of characters.*/
  public native static java.util.List toArray(String oper1);
  /** 
   * Returns the index of the selected item of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _index = lbCurSel _control<br/>
   * @param oper1 control
   * @since 2.91
   */
  /* Returns the index of the selected item of the given listbox or combobox.*/
  public native static float lbCurSel(GameControl oper1);
  /** 
   * Returns the index of the selected item of the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _index = lbCurSel 101<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Returns the index of the selected item of the listbox or combobox with id idc of the topmost user dialog.*/
  public native static float lbCurSel(float oper1);
  /** 
   * Returns current muzzle of unit's weapon.
   * @param oper1 gunner
   * @since 5501
   */
  /* Returns current muzzle of unit's weapon.*/
  public native static String currentMuzzle(GameObject oper1);
  /** 
   * Sets the additional integer value in the item with the position index of the 2D listbox.
<br/><br/>
   * <b>Example:</b> lnbSetValue [101, [0,1], 1]<br/>
   * @param oper1 [idc, [row, column], value]
   * @since 5501
   */
  /* Sets the additional integer value in the item with the position index of the 2D listbox.*/
  public native static void lnbSetValue(java.util.List oper1);
  /** 
   * Returns the array of selected rows indices in the given listbox.
<br/><br/>
   * <b>Example:</b> _indices = lbSelection _control<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Returns the array of selected rows indices in the given listbox.*/
  public native static java.util.List lbSelection(GameControl oper1);
  /** 
   * Returns the index of the selected item of the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _index = lbCurSel _control<br/>
   * @param oper1 control
   * @since 2.91
   */
  /* Returns the index of the selected item of the given listbox or combobox.*/
  public native static java.util.List lnbGetColumnsPosition(GameControl oper1);
  /** 
   * Returns the columns position in 2D listbox.
<br/><br/>
   * <b>Example:</b> _n = lnbGetColumnsSize 105<br/>
   * @param oper1 idc
   * @since 5501
   */
  /* Returns the columns position in 2D listbox.*/
  public native static java.util.List lnbGetColumnsPosition(float oper1);
  /** 
   * Returns name of config entry.
<br/><br/>
   * <b>Example:</b> _name = configName (configFile >> "CfgVehicles")<br/>
<br/>
   * <b>Example result:</b> "CfgVehicles"<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Returns name of config entry.*/
  public native static String configName(GameConfig oper1);
  /** 
   * Returns the marker positon in format [x,z,y].
<br/><br/>
   * <b>Example:</b> getMarkerPos "markerOne"<br/>
   * @param oper1 markerName
   */
  /* Returns the marker positon in format [x,z,y].*/
  public native static java.util.List getMarkerPos(String oper1);
  /** 
   * Enable / disable showing of in-game UI during currently active camera effect.
   * @param oper1 enable
   * @since 5501
   */
  /* Enable / disable showing of in-game UI during currently active camera effect.*/
  public native static void cameraEffectEnableHUD(boolean oper1);
  /** 
   * Creates image of operation field structure (for vehicle).
   * @param oper1 [x, z]
   * @since 2.54
   */
  /* Creates image of operation field structure (for vehicle).*/
  public native static void diag_operFieldVehicle(java.util.List oper1);
  /** 
   * Returns the descripction of the task.
   * @param oper1 task
   * @since 5501
   */
  /* Returns the descripction of the task.*/
  public native static java.util.List taskDescription(GameTask oper1);
  /** 
   * Compile and execute function (sqf). The function is first searched in the mission folder, then in the campaign scripts folder and finally in the global scripts folder.
<br/><br/>
   * <b>Example:</b> execVM "test.sqf"<br/>
   * @param oper1 filename
   * @since 2.58
   */
  /* Compile and execute function (sqf). The function is first searched in the mission folder, then in the campaign scripts folder and finally in the global scripts folder.*/
  public native static GameScript execVM(String oper1);
  /** 
   * Gets the waypoint type.
<br/><br/>
   * <b>Example:</b> waypointType [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint type.*/
  public native static String waypointType(java.util.List oper1);
  /** 
   * Gets the marker size. See <f>setMarkerSize</f>.
Note: this function is identical to <f>getMarkerSize</f>.
<br/><br/>
   * <b>Example:</b> markerSize "MarkerOne"<br/>
   * @param oper1 markerName
   * @since 1.50
   */
  /* Gets the marker size. See <f>setMarkerSize</f>.
Note: this function is identical to <f>getMarkerSize</f>.*/
  public native static java.util.List markerSize(String oper1);
  /** 
   * Shows / hides the control with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> ctrlShow [100, true]<br/>
   * @param oper1 [idc, show]
   * @since 1.50
   */
  /* Shows / hides the control with id idc of the topmost user dialog.*/
  public native static void ctrlShow(java.util.List oper1);
  /** 
   * Resource title  - argument uses format ["name","type",speed] or ["name","type"]. Speed is ignored.
Preload data
The resource can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> titleRsc ["BIS", "PLAIN"]<br/>
   * @param oper1 effect
   */
  /* Resource title  - argument uses format ["name","type",speed] or ["name","type"]. Speed is ignored.
Preload data
The resource can be defined in the description.ext file.*/
  public native static boolean preloadTitleRsc(java.util.List oper1);
  /** 
   * Sets the time acceleration coeficient. It can also be used to slow down time in cutscenes.
<br/><br/>
   * <b>Example:</b> setAccTime 0.1<br/>
   * @param oper1 accFactor
   */
  /* Sets the time acceleration coeficient. It can also be used to slow down time in cutscenes.*/
  public native static void setAccTime(float oper1);
  /** 
   * Removes the waypoint.
<br/><br/>
   * <b>Example:</b> deleteWaypoint [grp, 2]<br/>
   * @param oper1 waypoint
   * @since 1.86
   */
  /* Removes the waypoint.*/
  public native static void deleteWaypoint(java.util.List oper1);
  /** 
   * The tangens of x, the argument is in degrees.
<br/><br/>
   * <b>Example:</b> tg 45<br/>
<br/>
   * <b>Example result:</b> 1<br/>
   * @param oper1 x
   */
  /* The tangens of x, the argument is in degrees.*/
  public native static float tg(float oper1);
  /** 
   * Checks a current morale level of the unit (-1..+1).
<br/><br/>
   * <b>Example:</b> morale leader player<br/>
<br/>
   * <b>Example result:</b> 0.5<br/>
   * @param oper1 unit
   * @since 5500
   */
  /* Checks a current morale level of the unit (-1..+1).*/
  public native static float morale(GameObject oper1);
  /** 
   * Sets new zoom limits for the main map.
   * @param oper1 [min, max, default]
   */
  /* Sets new zoom limits for the main map.*/
  public native static void diag_setMapScale(java.util.List oper1);
  /** 
   * not a
<br/><br/>
   * <b>Example:</b> not false<br/>
<br/>
   * <b>Example result:</b> true<br/>
   * @param oper1 a
   */
  /* not a*/
  public native static boolean not(boolean oper1);
  /** 
   * Gets the object attached to the waypoint.
<br/><br/>
   * <b>Example:</b> waypointAttachedObject [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the object attached to the waypoint.*/
  public native static GameObject waypointAttachedObject(java.util.List oper1);
  /** 
   * Set current (forced == false) or permanent (forced == true) wind direction and force.
   * @param oper1 [x, z, forced]
   * @since 5501
   */
  /* Set current (forced == false) or permanent (forced == true) wind direction and force.*/
  public native static void setWind(java.util.List oper1);
  /** 
   * Creates a new marker on the given position. The marker name has to be unique. The marker is only created on the computer where the command is called.
<br/><br/>
   * <b>Example:</b> marker = createMarkerLocal [Marker1, position player]<br/>
   * @param oper1 [name, position]
   * @since 2.92
   */
  /* Creates a new marker on the given position. The marker name has to be unique. The marker is only created on the computer where the command is called.*/
  public native static String createMarkerLocal(java.util.List oper1);
  /** 
   * Log all frames, where section takes longer than the threshold (in seconds).
   * @param oper1 [section, threshold]
   * @since 5501
   */
  /* Log all frames, where section takes longer than the threshold (in seconds).*/
  public native static void diag_logSlowFrame(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_objToggle(String oper1);
  /** 
   * Converts any variable to a string.
<br/><br/>
   * <b>Example:</b> str(2+3)<br/>
<br/>
   * <b>Example result:</b> "5"<br/>
   * @param oper1 any value
   * @since 2.00
   */
  /* Converts any variable to a string.*/
  public native static String str(Object oper1);
  /** 
   * Execute the scripted FSM. The FSM file is first searched in the mission folder, then in the campaign scripts folder and finally in the global scripts folder. Return the FSM handler or 0 when failed.
<br/><br/>
   * <b>Example:</b> execFSM "test.fsm"<br/>
   * @param oper1 filename
   * @since 5500
   */
  /* Execute the scripted FSM. The FSM file is first searched in the mission folder, then in the campaign scripts folder and finally in the global scripts folder. Return the FSM handler or 0 when failed.*/
  public native static float execFSM(String oper1);
  /** 
   * Enables radio transmissions to be heard and seen on screen.
   * @param oper1 enable
   */
  /* Enables radio transmissions to be heard and seen on screen.*/
  public native static void enableRadio(boolean oper1);
  /** 
   * Checks whether the unit is fleeing. A dead or empty unit returns false.
<br/><br/>
   * <b>Example:</b> fleeing player<br/>
   * @param oper1 unit
   */
  /* Checks whether the unit is fleeing. A dead or empty unit returns false.*/
  public native static boolean fleeing(GameObject oper1);
  /** 
   * Tests whether the variable is null. The function returns true if the variable is null and false if it's not.
<br/><br/>
   * <b>Example:</b> if (isNil("_pokus")) then {_pokus=0;}<br/>
   * @param oper1 variable
   * @since 2.00
   */
  /* Tests whether the variable is null. The function returns true if the variable is null and false if it's not.*/
  public native static boolean isNil(Object oper1);
  /** 
   * Executes the given code.
<br/><br/>
   * <b>Example:</b> call {"x=2"}<br/>
   * @param oper1 code
   * @since 1.85
   */
  /* Executes the given code.*/
  public native static Object call(GameCode oper1);
  /** 
   * Restarts the mission editor camera (if it was deleted by a script, for example).
   * @param oper1 map
   * @since 2.92
   */
  /* Restarts the mission editor camera (if it was deleted by a script, for example).*/
  public native static void restartEditorCamera(GameControl oper1);
  /** 
   * Returns number of seconds to auto-scroll one line. -1 if auto-scroll is disabled. -2 if scrollbar not present.
<br/><br/>
   * <b>Example:</b> _speed = ctrlAutoScrollSpeed _control<br/>
   * @param oper1 control
   * @since 5501
   */
  /* Returns number of seconds to auto-scroll one line. -1 if auto-scroll is disabled. -2 if scrollbar not present.*/
  public native static float ctrlAutoScrollSpeed(GameControl oper1);
  /** 
   * Checks whether the vehicle is locked for the player. If it's locked, the player cannot mount / dismount without an order.
<br/><br/>
   * <b>Example:</b> locked jeepOne<br/>
   * @param oper1 unit
   */
  /* Checks whether the vehicle is locked for the player. If it's locked, the player cannot mount / dismount without an order.*/
  public native static boolean locked(GameObject oper1);
  /** 
   * returns all weapons types and count from the cargo space.
<br/><br/>
   * <b>Example:</b> getWeaponCargo jeepOne<br/>
   * @param oper1 container
   */
  /* returns all weapons types and count from the cargo space.*/
  public native static java.util.List getWeaponCargo(GameObject oper1);
  /** 
   * Creates a sound source of the given type (type is the name of the subclass of CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The sound source is placed inside a circle with this position as its center and placement as its radius.
<br/><br/>
   * <b>Example:</b> soundSource = createSoundSource ["LittleDog", position player, [], 0]<br/>
   * @param oper1 [type, position, markers, placement]
   * @since 2.32
   */
  /* Creates a sound source of the given type (type is the name of the subclass of CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The sound source is placed inside a circle with this position as its center and placement as its radius.*/
  public native static GameObject createSoundSource(java.util.List oper1);
  /** 
   * Checks whether the map animation has finished.
   * @param oper1 control
   * @since 2.92
   */
  /* Checks whether the map animation has finished.*/
  public native static boolean ctrlMapAnimDone(GameControl oper1);
  /** 
   * Copy the text to the clipboard.
   * @param oper1 text
   * @since 5500
   */
  /* Copy the text to the clipboard.*/
  public native static void copyToClipboard(String oper1);
  /** 
   * Returns true if the editor is set to draw 3D icons.
   * @param oper1 map
   * @since 2.92
   */
  /* Returns true if the editor is set to draw 3D icons.*/
  public native static boolean isShowing3DIcons(GameControl oper1);
  /** 
   * Return an agent for given person.
<br/><br/>
   * <b>Example:</b> _agent = teamMember player<br/>
   * @param oper1 person
   * @since 2.90
   */
  /* Return an agent for given person.*/
  public native static GameTeamMember teamMember(GameObject oper1);
  /** 
   * Check if team member is an agent.
   * @param oper1 teamMember
   * @since 2.92
   */
  /* Check if team member is an agent.*/
  public native static boolean isAgent(GameTeamMember oper1);
  /** 
   * Select a random file currently in use by a mission.
   * @param oper1 position
   * @since 1.85
   */
  /* Select a random file currently in use by a mission.*/
  public native static String diag_randomFile(java.util.List oper1);
  /** 
   * Sorts the given listbox or combobox by item value.
<br/><br/>
   * <b>Example:</b> lbSortByValue _control<br/>
   * @param oper1 control
   * @since 5148
   */
  /* Sorts the given listbox or combobox by item value.*/
  public native static float lbSortByValue(GameControl oper1);
  /** 
   * Returns a list of button images or names assigned to the given user action. A maximum of maxKeys keys is listed. You can find the action names in config class ControllerSchemes.
<br/><br/>
   * <b>Example:</b> text = actionKeysImages "ReloadMagazine"<br/>
   * @param oper1 action or [action, maxKeys]
   * @since 2.01
   */
  /* Returns a list of button images or names assigned to the given user action. A maximum of maxKeys keys is listed. You can find the action names in config class ControllerSchemes.*/
  public native static GameText actionKeysImages(Object oper1);
  /** 
   * Convert the array of characters to the string.
<br/><br/>
   * <b>Example:</b> toString [65, 114, 109, 65]<br/>
<br/>
   * <b>Example result:</b> "ArmA"<br/>
   * @param oper1 characters
   * @since 5195
   */
  /* Convert the array of characters to the string.*/
  public native static String toString(java.util.List oper1);
  /** 
   * Return the vehicle person is assigned to.
   * @param oper1 person
   * @since 5130
   */
  /* Return the vehicle person is assigned to.*/
  public native static GameObject assignedVehicle(GameObject oper1);
  /** 
   * Remove all groups from unit's high command bar.
<br/><br/>
   * <b>Example:</b> hcRemoveAllGroups unit<br/>
   * @param oper1 unit
   * @since 5501
   */
  /* Remove all groups from unit's high command bar.*/
  public native static void hcRemoveAllGroups(GameObject oper1);
  /** 
   * Suspend execution of script until condition is satisfied.
<br/><br/>
   * <b>Example:</b> _i = 0; waitUntil {_i = _i + 1; _i >= 100}<br/>
   * @param oper1 condition
   * @since 2.60
   */
  /* Suspend execution of script until condition is satisfied.*/
  public native static void waitUntil(GameCode oper1);
  /** 
   * Defines an action performed just before the preload screen started.
   * @param oper1 command
   * @since 5501
   */
  /* Defines an action performed just before the preload screen started.*/
  public native static void onPreloadStarted(Object oper1);
  /** 
   * Returns an array with all the units in the group.
<br/><br/>
   * <b>Example:</b> player in units group player<br/>
   * @param oper1 grp
   */
  /* Returns an array with all the units in the group.*/
  public native static java.util.List units(GameGroup oper1);
  /** 
   * Returns an array with all the units in the group of the given object. For a destroyed object an empty array is returned.
<br/><br/>
   * <b>Example:</b> player in units player<br/>
   * @param oper1 unit
   */
  /* Returns an array with all the units in the group of the given object. For a destroyed object an empty array is returned.*/
  public native static java.util.List units(GameObject oper1);
  /** 
   * Check if latest low level moveTo command is finished.
   * @param oper1 person
   * @since 2.62
   */
  /* Check if latest low level moveTo command is finished.*/
  public native static boolean moveToCompleted(GameObject oper1);
  /** 
   * Returns a list of dikCodes of buttons assigned to the given user action.
<br/><br/>
   * <b>Example:</b> array = actionKeys "ReloadMagazine"<br/>
   * @param oper1 action
   * @since 2.01
   */
  /* Returns a list of dikCodes of buttons assigned to the given user action.*/
  public native static java.util.List actionKeys(String oper1);
  /** 
   * Return position of unit in the formation.
   * @param oper1 person
   * @since 2.92
   */
  /* Return position of unit in the formation.*/
  public native static java.util.List formationPosition(GameObject oper1);
  /** 
   * Returns a structured text, containing an image or  name (if no image is found) of the button (on the keyboard, mouse or joystick) with the given code.
<br/><br/>
   * <b>Example:</b> name = keyImage 28<br/>
<br/>
   * <b>Example result:</b> "Enter"<br/>
   * @param oper1 dikCode
   * @since 2.01
   */
  /* Returns a structured text, containing an image or  name (if no image is found) of the button (on the keyboard, mouse or joystick) with the given code.*/
  public native static GameText keyImage(float oper1);
  /** 
   * same as hideObject.
   * @param oper1 object
   * @since 2.90
   */
  /* same as hideObject.*/
  public native static void deleteCollection(GameObject oper1);
  /** 
   * Check if script is already finished
   * @param oper1 script
   * @since 2.50
   */
  /* Check if script is already finished*/
  public native static boolean scriptDone(GameScript oper1);
  /** 
   * Creates a vehicle of the given type (type is the name of the subclass in CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The vehicle is placed inside a circle with this position as center and placement as its radius. Special properties can be: "NONE", "FLY" and "FORM".
<br/><br/>
   * <b>Example:</b> veh = createVehicle ["Cobra", position player, [], 0, "FLY"]<br/>
   * @param oper1 [type, position, markers, placement, special]
   * @since 2.32
   */
  /* Creates a vehicle of the given type (type is the name of the subclass in CfgVehicles). If the markers array contains several marker names, the position of a random one is used. Otherwise, the given position is used. The vehicle is placed inside a circle with this position as center and placement as its radius. Special properties can be: "NONE", "FLY" and "FORM".*/
  public native static GameObject createVehicle(java.util.List oper1);
  /** 
   * AIS end state.
<br/><br/>
   * <b>Example:</b> AISFinishHeal [wounded,medic,true]<br/>
   * @param oper1 [unit,unit,bool]
   * @since 5501
   */
  /* AIS end state.*/
  public native static void AISFinishHeal(java.util.List oper1);
  /** 
   * Returns the additional integer value in the item with the given position of the 2D listbox.
<br/><br/>
   * <b>Example:</b> _value = lnbValue [101, [0,1]]<br/>
   * @param oper1 [idc, [row, column]
   * @since 5501
   */
  /* Returns the additional integer value in the item with the given position of the 2D listbox.*/
  public native static float lnbValue(java.util.List oper1);
  /** 
   * Host the MP mission described by config class. Should be called as a reaction to some UI action in some dialog.
   * @param oper1 [Config,Display]
   * @since 1.54
   */
  /* Host the MP mission described by config class. Should be called as a reaction to some UI action in some dialog.*/
  public native static void hostMission(java.util.List oper1);
  /** 
   * Check whether the given FSM completes. The FSM handle is the number returned by the execFSM command.
   * @param oper1 FSM handle
   * @since 5500
   */
  /* Check whether the given FSM completes. The FSM handle is the number returned by the execFSM command.*/
  public native static boolean completedFSM(float oper1);
  /** 
   * Enables / disables the control with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> ctrlEnable [100, false]<br/>
   * @param oper1 [idc, enable]
   * @since 1.50
   */
  /* Enables / disables the control with id idc of the topmost user dialog.*/
  public native static void ctrlEnable(java.util.List oper1);
  /** 
   * The mission is launched (from the main menu). Both campaign and mission are given as their directory name. If the campaign is empty, a single mission is launched. If skipBriefing is true, the intro and briefing are skipped.
<br/><br/>
   * <b>Example:</b> playMission["XOutrage","x05Negotiator.Noe"]<br/>
   * @param oper1 [campaign, mission] or [campaign, mission, skipBriefing]
   * @since 2.19
   */
  /* The mission is launched (from the main menu). Both campaign and mission are given as their directory name. If the campaign is empty, a single mission is launched. If skipBriefing is true, the intro and briefing are skipped.*/
  public native static void playMission(java.util.List oper1);
  /** 
   * Hides the body of the given person.
<br/><br/>
   * <b>Example:</b> hideBody player<br/>
   * @param oper1 person
   * @since 2.10
   */
  /* Hides the body of the given person.*/
  public native static void hideBody(GameObject oper1);
  /** 
   * Removes all magazines from the vehicle cargo space.
<br/><br/>
   * <b>Example:</b> clearMagazineCargo jeepOne<br/>
   * @param oper1 unit
   */
  /* Removes all magazines from the vehicle cargo space.*/
  public native static void clearMagazineCargo(GameObject oper1);
  /** 
   * Returns current weapon mode of unit's weapon.
   * @param oper1 gunner
   * @since 5501
   */
  /* Returns current weapon mode of unit's weapon.*/
  public native static String currentWeaponMode(GameObject oper1);
  /** 
   * Returns the current position in the progress bar.
<br/><br/>
   * <b>Example:</b> _pos = progressPosition _control<br/>
   * @param oper1 control
   * @since 5500
   */
  /* Returns the current position in the progress bar.*/
  public native static float progressPosition(GameControl oper1);
  /** 
   * Return the current scale of the map control.
   * @param oper1 control
   * @since 2.92
   */
  /* Return the current scale of the map control.*/
  public native static float ctrlMapScale(GameControl oper1);
  /** 
   * Moves the soldier out of vehicle. (Immediately, without animation).
   * @param oper1 soldier
   * @since 5501
   */
  /* Moves the soldier out of vehicle. (Immediately, without animation).*/
  public native static void moveOut(GameObject oper1);
  /** 
   * Returns the shown text in the item with the given position of the 2D listbox.
<br/><br/>
   * <b>Example:</b> _text = lnbText [101, [0,1]]<br/>
   * @param oper1 [idc, [row, column]]
   * @since 1.50
   */
  /* Returns the shown text in the item with the given position of the 2D listbox.*/
  public native static String lnbText(java.util.List oper1);
  /** 
   * Checks whether the appropriate version of the application is available. If it's not, a warning message is shown and false is returned. The version format is "Major.Minor", e.g. "1.30".
<br/><br/>
   * <b>Example:</b> requiredVersion "1.30"<br/>
   * @param oper1 version
   * @since 1.21
   */
  /* Checks whether the appropriate version of the application is available. If it's not, a warning message is shown and false is returned. The version format is "Major.Minor", e.g. "1.30".*/
  public native static boolean requiredVersion(String oper1);
  /** 
   * Returns trigger statements in the form [cond, activ, desactiv]
   * @param oper1 trigger
   * @since 5501
   */
  /* Returns trigger statements in the form [cond, activ, desactiv]*/
  public native static java.util.List triggerStatements(GameObject oper1);
  /** 
   * Return expected destination of unit as a [position, planningMode, forceReplan].
   * @param oper1 person
   * @since 2.92
   */
  /* Return expected destination of unit as a [position, planningMode, forceReplan].*/
  public native static java.util.List expectedDestination(GameObject oper1);
  /** 
   * Creates a user dialog from the resource template name. If a user dialog already exists, it creates a child dialog of the topmost user dialog. The class name is searched in the description.ext file of the mission, the description.ext file of the campaign and the globlal resource.cpp file. The function returns its success.
<br/><br/>
   * <b>Example:</b> _ok = createDialog "RscDisplayGame"<br/>
   * @param oper1 name
   * @since 1.50
   */
  /* Creates a user dialog from the resource template name. If a user dialog already exists, it creates a child dialog of the topmost user dialog. The class name is searched in the description.ext file of the mission, the description.ext file of the campaign and the globlal resource.cpp file. The function returns its success.*/
  public native static boolean createDialog(String oper1);
  /** 
   * Removes all weapons from the vehicle cargo space. MP synchronized.
<br/><br/>
   * <b>Example:</b> clearWeaponCargoGlobal jeepOne<br/>
   * @param oper1 unit
   */
  /* Removes all weapons from the vehicle cargo space. MP synchronized.*/
  public native static void clearWeaponCargoGlobal(GameObject oper1);
  /** 
   * Sets if group icons are visible.
<br/><br/>
   * <b>Example:</b> showGroupIcons [true,true]<br/>
   * @param oper1 array
   * @since 5501
   */
  /* Sets if group icons are visible.*/
  public native static void setGroupIconsVisible(java.util.List oper1);
  /** 
   * Returns the object position in format <ar>Position</ar>.
<br/><br/>
   * <b>Example:</b> position player<br/>
   * @param oper1 object
   * @since 1.50
   */
  /* Returns the object position in format <ar>Position</ar>.*/
  public native static java.util.List position(GameObject oper1);
  /** 
   * Return (raw) position of given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return (raw) position of given location.*/
  public native static java.util.List position(GameLocation oper1);
  /** 
   * Creates a JVM object representing a FSM and register it for execution.
   * @param oper1 className
   * @since 85126
   */
  /* Creates a JVM object representing a FSM and register it for execution.*/
  public native static void jExecFSM(String oper1);
  /** 
   * Check if cargo of this vehicle want to get out when in combat.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Check if cargo of this vehicle want to get out when in combat.*/
  public native static boolean canUnloadInCombat(GameObject oper1);
  /** 
   * The exponential value of x.
<br/><br/>
   * <b>Example:</b> exp 1<br/>
<br/>
   * <b>Example result:</b> 2.7182<br/>
   * @param oper1 x
   */
  /* The exponential value of x.*/
  public native static float exp(float oper1);
  /** 
   * Enables radio transmissions to be heard and seen on screen. It does not affect KBTell conversations.
   * @param oper1 enable
   */
  /* Enables radio transmissions to be heard and seen on screen. It does not affect KBTell conversations.*/
  public native static void enableSentences(boolean oper1);
  /** 
   * The round value of x.
<br/><br/>
   * <b>Example:</b> round -5.25<br/>
<br/>
   * <b>Example result:</b> -5<br/>
   * @param oper1 x
   */
  /* The round value of x.*/
  public native static float round(float oper1);
  /** 
   * Returns an array of names of all special items of the vehicle.
<br/><br/>
   * <b>Example:</b> items player<br/>
   * @param oper1 unit
   * @since 5501
   */
  /* Returns an array of names of all special items of the vehicle.*/
  public native static java.util.List items(GameObject oper1);
  /** 
   * Defines a sound (voice) that is played the first time when the Notes section in the briefing is selected.
<br/><br/>
   * <b>Example:</b> onBriefingNotes "NotesVoiceOver"<br/>
   * @param oper1 sound
   * @since 1.75
   */
  /* Defines a sound (voice) that is played the first time when the Notes section in the briefing is selected.*/
  public native static void onBriefingNotes(String oper1);
  /** 
   * Finish the mission. The end type can be "CONTINUE", "KILLED", "LOSER", "END1", "END2", "END3", "END4", "END5", or "END6".
   * @param oper1 end type
   * @since 5501
   */
  /* Finish the mission. The end type can be "CONTINUE", "KILLED", "LOSER", "END1", "END2", "END3", "END4", "END5", or "END6".*/
  public native static void endMission(String oper1);
  /** 
   * Gets the marker shape. See <f>setMarkerShape</f>.
<br/><br/>
   * <b>Example:</b> markerShape "MarkerOne"<br/>
   * @param oper1 name
   * @since 5501
   */
  /* Gets the marker shape. See <f>setMarkerShape</f>.*/
  public native static String markerShape(String oper1);
  /** 
   * Gets the waypoint behavior.
<br/><br/>
   * <b>Example:</b> waypointBehaviour [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint behavior.*/
  public native static String waypointBehaviour(java.util.List oper1);
  /** 
   * Returns the commander of the vehicle. If the vehicle is not a vehicle, but a person, the person is returned instead.
<br/><br/>
   * <b>Example:</b> commander vehicle player<br/>
   * @param oper1 vehicle
   */
  /* Returns the commander of the vehicle. If the vehicle is not a vehicle, but a person, the person is returned instead.*/
  public native static GameObject commander(GameObject oper1);
  /** 
   * Gets the waypoint script.
<br/><br/>
   * <b>Example:</b> waypointScript [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint script.*/
  public native static String waypointScript(java.util.List oper1);
  /** 
   * Preload all textures and models around given position
   * @param oper1 position
   * @since 2.50
   */
  /* Preload all textures and models around given position*/
  public native static boolean preloadCamera(java.util.List oper1);
  /** 
   * Checks whether the given vehicle is able to move. It does not test for fuel, only the damage status is checked.
<br/><br/>
   * <b>Example:</b> canMove vehicle player<br/>
   * @param oper1 vehicle
   */
  /* Checks whether the given vehicle is able to move. It does not test for fuel, only the damage status is checked.*/
  public native static boolean canMove(GameObject oper1);
  /** 
   * Gets the waypoint statements.
<br/><br/>
   * <b>Example:</b> waypointStatements [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint statements.*/
  public native static java.util.List waypointStatements(java.util.List oper1);
  /** 
   * Return a type of given team.
<br/><br/>
   * <b>Example:</b> _type = teamType _team<br/>
   * @param oper1 team
   * @since 2.90
   */
  /* Return a type of given team.*/
  public native static String teamType(GameTeamMember oper1);
  /** 
   * Return a name of given team.
<br/><br/>
   * <b>Example:</b> _name = teamName _team<br/>
   * @param oper1 team
   * @since 2.90
   */
  /* Return a name of given team.*/
  public native static String teamName(GameTeamMember oper1);
  /** 
   * Create MP role for the unit.
<br/><br/>
   * <b>Example:</b> setPlayable aP<br/>
   * @param oper1 unit
   * @since 2.92
   */
  /* Create MP role for the unit.*/
  public native static void setPlayable(GameObject oper1);
  /** 
   * Add a unit into the list of units available for team switch.
   * @param oper1 person
   * @since 2.92
   */
  /* Add a unit into the list of units available for team switch.*/
  public native static void addSwitchableUnit(GameObject oper1);
  /** 
   * Parse string containing real number.
<br/><br/>
   * <b>Example:</b> parseNumber "0.125"<br/>
   * @param oper1 string
   * @since 2.92
   */
  /* Parse string containing real number.*/
  public native static float parseNumber(String oper1);
  /** 
   * The floor value of x.
<br/><br/>
   * <b>Example:</b> floor 5.25 <br/>
<br/>
   * <b>Example result:</b> 5<br/>
   * @param oper1 x
   */
  /* The floor value of x.*/
  public native static float floor(float oper1);
  /** 
   * Return the rank of the given unit for comparison.
   * @param oper1 unit
   * @since 5500
   */
  /* Return the rank of the given unit for comparison.*/
  public native static float rankId(GameObject oper1);
  /** 
   * Clears all items in the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> lbClear _control<br/>
   * @param oper1 control
   * @since 2.91
   */
  /* Clears all items in the given listbox or combobox.*/
  public native static void lbClear(GameControl oper1);
  /** 
   * Clears all items in the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> lbClear 101<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Clears all items in the listbox or combobox with id idc of the topmost user dialog.*/
  public native static void lbClear(float oper1);
  /** 
   * Resource background - the argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The resource can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> cutRsc ["binocular", "PLAIN"]<br/>
   * @param oper1 effect
   */
  /* Resource background - the argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The resource can be defined in the description.ext file.*/
  public native static void cutRsc(java.util.List oper1);
  /** 
   * Sets the picture in the item with the given position of the 2D listbox. Name is the picture name. The picture is searched in the mission directory, the dtaExt subdirectory of the campaign directory and the dtaExt directory and the data bank (or directory).
<br/><br/>
   * <b>Example:</b> lnbSetPicture [101, [0,1], "iskoda"]<br/>
   * @param oper1 [idc, [row, column], name]
   * @since 5501
   */
  /* Sets the picture in the item with the given position of the 2D listbox. Name is the picture name. The picture is searched in the mission directory, the dtaExt subdirectory of the campaign directory and the dtaExt directory and the data bank (or directory).*/
  public native static void lnbSetPicture(java.util.List oper1);
  /** 
   * Transfers weapons and magazines from the cargo space of object obj to the weapon pool (used in campaigns to transfer weapons to the next mission).
   * @param oper1 obj
   * @since 1.75
   */
  /* Transfers weapons and magazines from the cargo space of object obj to the weapon pool (used in campaigns to transfer weapons to the next mission).*/
  public native static void pickWeaponPool(GameObject oper1);
  /** 
   * Adds text into the footer section of the mpreport.txt file.
   * @param oper1 text
   * @since 1.80
   */
  /* Adds text into the footer section of the mpreport.txt file.*/
  public native static void VBS_addFooter(String oper1);
  /** 
   * Returns the preprocessed content of the given file. The preprocessor is C-like, it supports comments using // or /* and * / and macros defined with #define.
<br/><br/>
   * <b>Example:</b> preprocessFileLineNumbers "myFunction.sqf"<br/>
<br/>
   * <b>Example result:</b> "if a>b then {a} else {b}"<br/>
   * @param oper1 filename
   * @since 2.58
   */
  /* Returns the preprocessed content of the given file. The preprocessor is C-like, it supports comments using // or /* and * / and macros defined with #define.*/
  public native static String preprocessFileLineNumbers(String oper1);
  /** 
   * Loads the file .class from the scripts locations and return a handler used to call its methods.
   * @param oper1 className
   * @since 84775
   */
  /* Loads the file .class from the scripts locations and return a handler used to call its methods.*/
  public native static GameJavaClass jLoad(String oper1);
  /** 
   * Returns the preprocessed content of the given file. The preprocessor is C-like, it supports comments using // or /* and * / and macros defined with #define.
<br/><br/>
   * <b>Example:</b> preprocessFile "myFunction.sqf"<br/>
<br/>
   * <b>Example result:</b> "if a>b then {a} else {b}"<br/>
   * @param oper1 filename
   * @since 1.82
   */
  /* Returns the preprocessed content of the given file. The preprocessor is C-like, it supports comments using // or /* and * / and macros defined with #define.*/
  public native static String preprocessFile(String oper1);
  /** 
   * Convert date to float number.
<br/><br/>
   * <b>Example:</b> time = DateToNumber [year,month,day,hour,minute]<br/>
   * @param oper1 date
   * @since 5501
   */
  /* Convert date to float number.*/
  public native static float DateToNumber(java.util.List oper1);
  /** 
   * Adds an row of strings.
<br/><br/>
   * <b>Example:</b> _index = lnbAddRow [105, ["First column", "second column", ...]]. Returns row index.<br/>
   * @param oper1 [idc, [text, text,...]]
   * @since 5501
   */
  /* Adds an row of strings.*/
  public native static float lnbAddRow(java.util.List oper1);
  /** 
   * Checks whether the camera has finished committing.
<br/><br/>
   * <b>Example:</b> camCommitted _camera<br/>
   * @param oper1 camera
   */
  /* Checks whether the camera has finished committing.*/
  public native static boolean camCommitted(GameObject oper1);
  /** 
   * Exports landscape as XYZ file.
   * @param oper1 filename
   * @since 2.92
   */
  /* Exports landscape as XYZ file.*/
  public native static void diag_exportLandscapeXYZ(String oper1);
  /** 
   * Create location of given type with given size at given position.
   * @param oper1 [type, position, sizeX, sizeZ]
   * @since 2.90
   */
  /* Create location of given type with given size at given position.*/
  public native static GameLocation createLocation(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_cheat1(String oper1);
  /** 
   * Returns the name of the variable assigned to the object in the mission editor. When used on a vehicle, the name of the first crew member is returned (in order: commander, driver, gunner).
<br/><br/>
   * <b>Example:</b> name vehicle player<br/>
   * @param oper1 object
   */
  /* Returns the name of the variable assigned to the object in the mission editor. When used on a vehicle, the name of the first crew member is returned (in order: commander, driver, gunner).*/
  public native static String name(GameObject oper1);
  /** 
   * Return name of global vatiable containing given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return name of global vatiable containing given location.*/
  public native static String name(GameLocation oper1);
  /** 
   * Checks whether the given person, vehicle or building is't dead / destroyed.
<br/><br/>
   * <b>Example:</b> alive player<br/>
   * @param oper1 obj
   */
  /* Checks whether the given person, vehicle or building is't dead / destroyed.*/
  public native static boolean alive(GameObject oper1);
  /** 
   * This function does nothing. It's used to create comments.
<br/><br/>
   * <b>Example:</b> comment "This is a comment."<br/>
   * @param oper1 comment
   * @since 1.85
   */
  /* This function does nothing. It's used to create comments.*/
  public native static void comment(String oper1);
  /** 
   * Enables the GPS receiver (the default is false).
   * @param oper1 show
   */
  /* Enables the GPS receiver (the default is false).*/
  public native static void showGps(boolean oper1);
  /** 
   * Orders a unit to get out from the vehicle (silently).
<br/><br/>
   * <b>Example:</b> doGetOut unitOne<br/>
   * @param oper1 unit
   * @since 2.28
   */
  /* Orders a unit to get out from the vehicle (silently).*/
  public native static void doGetOut(Object oper1);
  /** 
   * Gets the marker color. See <f>setMarkerColor</f>.
<br/><br/>
   * <b>Example:</b> getMarkerColor "MarkerOne"<br/>
   * @param oper1 marker
   * @since 1.21
   */
  /* Gets the marker color. See <f>setMarkerColor</f>.*/
  public native static String getMarkerColor(String oper1);
  /** 
   * Returns a list of all kills caused by the given unit. Items are in format <ar>VBS_Kill</ar>.
   * @param oper1 name
   * @since 1.80
   */
  /* Returns a list of all kills caused by the given unit. Items are in format <ar>VBS_Kill</ar>.*/
  public native static java.util.List VBS_kills(String oper1);
  /** 
   * Creates image of operation field structure (for soldier).
   * @param oper1 [x, z]
   * @since 2.54
   */
  /* Creates image of operation field structure (for soldier).*/
  public native static void diag_operFieldSoldier(java.util.List oper1);
  /** 
   * Return the type of the task.
   * @param oper1 task
   * @since 2.91
   */
  /* Return the type of the task.*/
  public native static String type(GameTask oper1);
  /** 
   * Return type of given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return type of given location.*/
  public native static String type(GameLocation oper1);
  /** 
   * Find the nearest location (to the given position) having it speech non-empty.
   * @param oper1 position
   * @since 5501
   */
  /* Find the nearest location (to the given position) having it speech non-empty.*/
  public native static GameLocation nearestLocationWithDubbing(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_cheat2(String oper1);
  /** 
   * 
   */
  /* */
  public native static java.util.List diag_searchMem(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engStressTex(float oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engStressTex(java.util.List oper1);
  /** 
   * Check if the entity has enabled simulation.
   * @param oper1 entity
   * @since 5500
   */
  /* Check if the entity has enabled simulation.*/
  public native static boolean simulationEnabled(GameObject oper1);
  /** 
   * Unlock the given achievement.
   * @param oper1 name
   * @since 5501
   */
  /* Unlock the given achievement.*/
  public native static boolean unlockAchievement(String oper1);
  /** 
   * Destroy post process effect given by handle
   * @param oper1 effect
   * @since 5501
   */
  /* Destroy post process effect given by handle*/
  public native static void ppEffectDestroy(float oper1);
  /** 
   * Create post process effect specified by name and priority
   * @param oper1 effect
   * @since 5501
   */
  /* Create post process effect specified by name and priority*/
  public native static void ppEffectDestroy(java.util.List oper1);
  /** 
   * Return the list of waypoints for given group.
   * @param oper1 group
   * @since 5129
   */
  /* Return the list of waypoints for given group.*/
  public native static java.util.List waypoints(Object oper1);
  /** 
   * Returns the gunner of the vehicle. If the vehicle is not a vehicle, but a person, the person is returned.
<br/><br/>
   * <b>Example:</b> gunner vehicle player<br/>
   * @param oper1 vehicle
   */
  /* Returns the gunner of the vehicle. If the vehicle is not a vehicle, but a person, the person is returned.*/
  public native static GameObject gunner(GameObject oper1);
  /** 
   * Closes the current overlay without committing.
   * @param oper1 map
   * @since 2.92
   */
  /* Closes the current overlay without committing.*/
  public native static void closeOverlay(GameControl oper1);
  /** 
   * Gets the vehicle attached to the waypoint.
<br/><br/>
   * <b>Example:</b> waypointAttachedVehicle [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the vehicle attached to the waypoint.*/
  public native static GameObject waypointAttachedVehicle(java.util.List oper1);
  /** 
   * Show player's map. If forced, cannot be closed.
<br/><br/>
   * <b>Example:</b> openMap [true,true]<br/>
   * @param oper1 [show, forced]
   * @since 5501
   */
  /* Show player's map. If forced, cannot be closed.*/
  public native static boolean openMap(java.util.List oper1);
  /** 
   * Show player's map. If forced, cannot be closed.
<br/><br/>
   * <b>Example:</b> openMap true<br/>
   * @param oper1 show
   * @since 5501
   */
  /* Show player's map. If forced, cannot be closed.*/
  public native static boolean openMap(boolean oper1);
  /** 
   * Returns the current level of ability of the person.
<br/><br/>
   * <b>Example:</b> skill player<br/>
   * @param oper1 person
   * @since 1.75
   */
  /* Returns the current level of ability of the person.*/
  public native static float skill(GameObject oper1);
  /** 
   * Send diagnostics command to server.
<br/><br/>
   * <b>Example:</b> diag_server "UserQueue off"<br/>
   * @param oper1 command
   * @since 2.69
   */
  /* Send diagnostics command to server.*/
  public native static void diag_serverCmd(String oper1);
  /** 
   * List all registered task types.
   * @param oper1 teamMember
   * @since 2.91
   */
  /* List all registered task types.*/
  public native static java.util.List registeredTasks(GameTeamMember oper1);
  /** 
   * Check if given location has rectangular shape.
   * @param oper1 location
   * @since 2.90
   */
  /* Check if given location has rectangular shape.*/
  public native static boolean rectangular(GameLocation oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_cheat3(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engToggle(String oper1);
  /** 
   * Load the given world, launch an empty mission and execute the given expression. Config (optional) can reference to the config entry replacing description.ext for this mission.
   * @param oper1 [world, expression, config, (ignoreChildWindow)]
   * @since 5500
   */
  /* Load the given world, launch an empty mission and execute the given expression. Config (optional) can reference to the config entry replacing description.ext for this mission.*/
  public native static void playScriptedMission(java.util.List oper1);
  /** 
   * Plays a sound defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> playSound "soundname"<br/>
   * @param oper1 name
   */
  /* Plays a sound defined in the description.ext file.*/
  public native static void playSound(String oper1);
  /** 
   * Plays a sound defined in the description.ext file. Accomodation set to false disable sound volume changes.
<br/><br/>
   * <b>Example:</b> playSound ["soundname", false]<br/>
   * @param oper1 name, enable / disable sound volume accomodation
   */
  /* Plays a sound defined in the description.ext file. Accomodation set to false disable sound volume changes.*/
  public native static void playSound(java.util.List oper1);
  /** 
   * Returns the marker positon in format [x,z,y].
Note: this function is identical to <f>getMarkerPos</f>.
<br/><br/>
   * <b>Example:</b> markerPos "markerOne"<br/>
   * @param oper1 markerName
   * @since 1.50
   */
  /* Returns the marker positon in format [x,z,y].
Note: this function is identical to <f>getMarkerPos</f>.*/
  public native static java.util.List markerPos(String oper1);
  /** 
   * Skips the specified time. Daytime is adjusted, a weather change is estimated and no changes to any units are done. Duration is in hours.
<br/><br/>
   * <b>Example:</b> skipTime 2.5<br/>
   * @param oper1 duration
   */
  /* Skips the specified time. Daytime is adjusted, a weather change is estimated and no changes to any units are done. Duration is in hours.*/
  public native static void skipTime(float oper1);
  /** 
   * Returns true if the trigger has been activated.
   * @param oper1 trigger
   * @since 5501
   */
  /* Returns true if the trigger has been activated.*/
  public native static boolean triggerActivated(GameObject oper1);
  /** 
   * Removes all weapons from the unit.
<br/><br/>
   * <b>Example:</b> removeAllWeapons player<br/>
   * @param oper1 unit
   */
  /* Removes all weapons from the unit.*/
  public native static void removeAllWeapons(GameObject oper1);
  /** 
   * Clear vehicle init field.
<br/><br/>
   * <b>Example:</b> clearVehicleInit soldier3<br/>
   * @param oper1 vehicle
   * @since 5154
   */
  /* Clear vehicle init field.*/
  public native static void clearVehicleInit(GameObject oper1);
  /** 
   * Returns true if the mission editor is operating in real time mode.
<br/><br/>
   * <b>Example:</b> _isRealTime = isRealTime _map<br/>
   * @param oper1 map
   * @since 2.92
   */
  /* Returns true if the mission editor is operating in real time mode.*/
  public native static boolean isRealTime(GameControl oper1);
  /** 
   * Adds count weapons of type name into the weapon pool (used in campaigns to transfer weapons to the next mission).
   * @param oper1 [name, count]
   * @since 1.75
   */
  /* Adds count weapons of type name into the weapon pool (used in campaigns to transfer weapons to the next mission).*/
  public native static void addWeaponPool(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engViewport(float oper1);
  /** 
   * Returns the vehicle in which the given unit is mounted. If there is none, the unit is returned.
<br/><br/>
   * <b>Example:</b> vehicle player != player<br/>
   * @param oper1 unit
   */
  /* Returns the vehicle in which the given unit is mounted. If there is none, the unit is returned.*/
  public native static GameObject vehicle(GameObject oper1);
  /** 
   * The sine of x, the argument is in degrees.
<br/><br/>
   * <b>Example:</b> sin 30<br/>
<br/>
   * <b>Example result:</b> 0.5<br/>
   * @param oper1 x
   */
  /* The sine of x, the argument is in degrees.*/
  public native static float sin(float oper1);
  /** 
   * The person is unassigned from the vehicle. If he is currently inside, the group leader will issue an order to disembark.
<br/><br/>
   * <b>Example:</b> unassignVehicle player<br/>
   * @param oper1 unit
   */
  /* The person is unassigned from the vehicle. If he is currently inside, the group leader will issue an order to disembark.*/
  public native static void unassignVehicle(GameObject oper1);
  /** 
   * Remove backpack from unit.
<br/><br/>
   * <b>Example:</b> removeBackpack player<br/>
   * @param oper1 unit
   * @since 5501
   */
  /* Remove backpack from unit.*/
  public native static void removeBackpack(GameObject oper1);
  /** 
   * Return list of units (drivers) in the formation.
   * @param oper1 person
   * @since 2.92
   */
  /* Return list of units (drivers) in the formation.*/
  public native static java.util.List formationMembers(GameObject oper1);
  /** 
   * Returns true if the specified person is subgroup leader.
   * @param oper1 person
   * @since 2.92
   */
  /* Returns true if the specified person is subgroup leader.*/
  public native static boolean isFormationLeader(GameObject oper1);
  /** 
   * Remove a unit from the list of units available for team switch.
   * @param oper1 person
   * @since 2.92
   */
  /* Remove a unit from the list of units available for team switch.*/
  public native static void removeSwitchableUnit(GameObject oper1);
  /** 
   * The number of elements in the array.
<br/><br/>
   * <b>Example:</b> count [0,0,1,2]<br/>
<br/>
   * <b>Example result:</b> 4<br/>
   * @param oper1 array
   */
  /* The number of elements in the array.*/
  public native static float count(java.util.List oper1);
  /** 
   * Returns count of subentries.
<br/><br/>
   * <b>Example:</b> _count = count (configFile >> "CfgVehicles")<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Returns count of subentries.*/
  public native static float count(GameConfig oper1);
  /** 
   * Returns the nearest building to the given object.
<br/><br/>
   * <b>Example:</b> neareastBuilding player<br/>
   * @param oper1 obj
   */
  /* Returns the nearest building to the given object.*/
  public native static GameObject nearestBuilding(GameObject oper1);
  /** 
   * Return the state of the given task.
   * @param oper1 task
   * @since 2.89
   */
  /* Return the state of the given task.*/
  public native static String taskState(GameTask oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_setLFE(java.util.List oper1);
  /** 
   * Activates the given key for the current user profile. The keys are used to unlock missions or campaigns. See keys, keysLimit and doneKeys in the description.ext file of the missions.
<br/><br/>
   * <b>Example:</b> activateKey "M04"<br/>
   * @param oper1 keyName
   * @since 2.06
   */
  /* Activates the given key for the current user profile. The keys are used to unlock missions or campaigns. See keys, keysLimit and doneKeys in the description.ext file of the missions.*/
  public native static void activateKey(String oper1);
  /** 
   * Check whether driver position of the vehicle turret is locked.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Check whether driver position of the vehicle turret is locked.*/
  public native static boolean lockedDriver(GameObject oper1);
  /** 
   * Breaks block to scope named 'name'. Nil is returned.
   * @param oper1 name
   */
  /* Breaks block to scope named 'name'. Nil is returned.*/
  public native static void breakTo(String oper1);
  /** 
   * Check if given person is the player.
   * @param oper1 person
   * @since 2.92
   */
  /* Check if given person is the player.*/
  public native static boolean isPlayer(GameObject oper1);
  /** 
   * Returns the object damage in the range from 0 to 1.
<br/><br/>
   * <b>Example:</b> getDammage player<br/>
   * @param oper1 obj
   */
  /* Returns the object damage in the range from 0 to 1.*/
  public native static float getDammage(GameObject oper1);
  /** 
   * Returns base entry of config entry.
<br/><br/>
   * <b>Example:</b> _base = inheritsFrom (configFile >> "CfgVehicles" >> "Car")<br/>
   * @param oper1 config
   * @since 2.92
   */
  /* Returns base entry of config entry.*/
  public native static GameConfig inheritsFrom(GameConfig oper1);
  /** 
   * Shows a text hint. The text can contain several lines. \n is used to indicate the end of a line. This hint has to be confirmed.
<br/><br/>
   * <b>Example:</b> hintC "Press W to move forward"<br/>
   * @param oper1 text
   */
  /* Shows a text hint. The text can contain several lines. \n is used to indicate the end of a line. This hint has to be confirmed.*/
  public native static void hintC(String oper1);
  /** 
   * Returns true walk is toggled.
   * @param oper1 soldier
   * @since 5501
   */
  /* Returns true walk is toggled.*/
  public native static boolean isWalking(GameObject oper1);
  /** 
   * Object title - the argument uses format ["text","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The object can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> titleObj ["BISLogo","plain"]<br/>
   * @param oper1 effect
   */
  /* Object title - the argument uses format ["text","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The object can be defined in the description.ext file.*/
  public native static void titleObj(java.util.List oper1);
  /** 
   * Returns the soldier assigned to the given vehicle as a commander.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Returns the soldier assigned to the given vehicle as a commander.*/
  public native static GameObject assignedCommander(GameObject oper1);
  /** 
   * Creates a new trigger on the given position. An object of the given type is created; this type must be a class name in CfgNonAIVehicles or CfgVehicles with simulation=detector.
<br/><br/>
   * <b>Example:</b> trigger = createTrigger["EmptyDetector", position player]<br/>
   * @param oper1 [type, position]
   * @since 1.86
   */
  /* Creates a new trigger on the given position. An object of the given type is created; this type must be a class name in CfgNonAIVehicles or CfgVehicles with simulation=detector.*/
  public native static GameObject createTrigger(java.util.List oper1);
  /** 
   * Sorts the given listbox or combobox by item text.
<br/><br/>
   * <b>Example:</b> lbSort _control<br/>
   * @param oper1 control
   * @since 5148
   */
  /* Sorts the given listbox or combobox by item text.*/
  public native static float lbSort(GameControl oper1);
  /** 
   * Launch actions attached to given (button based) control.
<br/><br/>
   * <b>Example:</b> ctrlActivate _control<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Launch actions attached to given (button based) control.*/
  public native static void ctrlActivate(GameControl oper1);
  /** 
   * Check whether given post process effect is commited
   * @param oper1 effect
   * @since 5501
   */
  /* Check whether given post process effect is commited*/
  public native static boolean ppEffectCommitted(String oper1);
  /** 
   * Check whether given post process effect is commited
   * @param oper1 effect
   * @since 5501
   */
  /* Check whether given post process effect is commited*/
  public native static boolean ppEffectCommitted(float oper1);
  /** 
   * defines name of current scope. Name is visible in debugger, and name is also used as reference in some commands. Scope name can be defined only once per scope.
   * @param oper1 name
   */
  /* defines name of current scope. Name is visible in debugger, and name is also used as reference in some commands. Scope name can be defined only once per scope.*/
  public native static void scopeName(String oper1);
  /** 
   * The base-10 logarithm of x.
<br/><br/>
   * <b>Example:</b> log 10<br/>
<br/>
   * <b>Example result:</b> 1<br/>
   * @param oper1 x
   */
  /* The base-10 logarithm of x.*/
  public native static float log(float oper1);
  /** 
   * Selects the item with the given index of the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> lbSetCurSel [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Selects the item with the given index of the listbox or combobox with id idc of the topmost user dialog.*/
  public native static void lbSetCurSel(java.util.List oper1);
  /** 
   * Returns the text shown in given control.
<br/><br/>
   * <b>Example:</b> _text = ctrlText _control<br/>
   * @param oper1 control
   * @since 2.50
   */
  /* Returns the text shown in given control.*/
  public native static String ctrlText(GameControl oper1);
  /** 
   * Returns the text shown in the control with id idc of the topmost user dialog. This can be used for static texts, buttons, edit lines and active texts.
<br/><br/>
   * <b>Example:</b> _message = ctrlText 100<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Returns the text shown in the control with id idc of the topmost user dialog. This can be used for static texts, buttons, edit lines and active texts.*/
  public native static String ctrlText(float oper1);
  /** 
   * Orders the unit to stop (silently).
Note: the stop command is never finished; the unit will never be ready.
<br/><br/>
   * <b>Example:</b> doStop unitOne<br/>
   * @param oper1 unit
   */
  /* Orders the unit to stop (silently).
Note: the stop command is never finished; the unit will never be ready.*/
  public native static void doStop(Object oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engReloadPS(String oper1);
  /** 
   * Destroys the AI center of the given side.
<br/><br/>
   * <b>Example:</b> deleteCenter East<br/>
   * @param oper1 side
   * @since 1.86
   */
  /* Destroys the AI center of the given side.*/
  public native static void deleteCenter(GameSide oper1);
  /** 
   * Gets the waypoint formation.
<br/><br/>
   * <b>Example:</b> waypointFormation [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint formation.*/
  public native static String waypointFormation(java.util.List oper1);
  /** 
   * Returns a bounding box of given object in model coordinates space. The result is in format [[minX, minZ, minY], [maxX, maxZ, maxY]].
   * @param oper1 object
   * @since 2.92
   */
  /* Returns a bounding box of given object in model coordinates space. The result is in format [[minX, minZ, minY], [maxX, maxZ, maxY]].*/
  public native static java.util.List boundingBox(GameObject oper1);
  /** 
   * Returns the velocity (speed vector) of the vehicle as an array with format [x, z, y].
   * @param oper1 vehicle
   * @since 1.80
   */
  /* Returns the velocity (speed vector) of the vehicle as an array with format [x, z, y].*/
  public native static java.util.List velocity(GameObject oper1);
  /** 
   * Enable / disable team switch.
   * @param oper1 enable
   * @since 2.92
   */
  /* Enable / disable team switch.*/
  public native static void enableTeamSwitch(boolean oper1);
  /** 
   * Return object attached to given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return object attached to given location.*/
  public native static GameObject attachedObject(GameLocation oper1);
  /** 
   * Shows loading screen with the given text, using the given resource. When loading screen is shown, simulation and scene drawing is disabled, scripts run at full speed.
   * @param oper1 [text] or [text, resource]
   * @since 5501
   */
  /* Shows loading screen with the given text, using the given resource. When loading screen is shown, simulation and scene drawing is disabled, scripts run at full speed.*/
  public native static void startLoadingScreen(java.util.List oper1);
  /** 
   * Checks whether the given soldier is able to stand up.
<br/><br/>
   * <b>Example:</b> canStand player<br/>
   * @param oper1 soldier
   */
  /* Checks whether the given soldier is able to stand up.*/
  public native static boolean canStand(GameObject oper1);
  /** 
   * Returns the current state of the given scud launcher. The following states are recognized: 0 - No activity, 1 - Launch preparation, 2 - Launch prepared, 3 - Ignition and 4 - Launched. Note:  non-integral values are used to indicate a transition between states.
<br/><br/>
   * <b>Example:</b> scudState scudOne>=4<br/>
   * @param oper1 scud
   * @since 1.28
   */
  /* Returns the current state of the given scud launcher. The following states are recognized: 0 - No activity, 1 - Launch preparation, 2 - Launch prepared, 3 - Ignition and 4 - Launched. Note:  non-integral values are used to indicate a transition between states.*/
  public native static float scudState(GameObject oper1);
  /** 
   * Sets the color of the item with the given index of the listbox or combobox with id idc of the topmost user dialog to the given color. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> lbSetColor [101, 0, [0, 1, 0, 0.5]]<br/>
   * @param oper1 [idc, index, color]
   * @since 1.50
   */
  /* Sets the color of the item with the given index of the listbox or combobox with id idc of the topmost user dialog to the given color. Color is in format <ar>Color</ar>.*/
  public native static void lbSetColor(java.util.List oper1);
  /** 
   * Defines an action performed when palyer clicked on group marker (3D or in a map)
   * @param oper1 command
   * @since 5501
   */
  /* Defines an action performed when palyer clicked on group marker (3D or in a map)*/
  public native static void onGroupIconOverLeave(Object oper1);
  /** 
   * Deletes an identity created by saveIdentity from the campaign progress file.
<br/><br/>
   * <b>Example:</b> deleteIdentity "playerIdentity"<br/>
   * @param oper1 identityName
   * @since 1.75
   */
  /* Deletes an identity created by saveIdentity from the campaign progress file.*/
  public native static boolean deleteIdentity(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_cheatX(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_cheatXB(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engStressTexMode(float oper1);
  /** 
   * Gets the marker alpha. See <f>setMarkerAlpha</f>.
<br/><br/>
   * <b>Example:</b> markerAlpha "MarkerOne"<br/>
   * @param oper1 markerName
   * @since 5501
   */
  /* Gets the marker alpha. See <f>setMarkerAlpha</f>.*/
  public native static float markerAlpha(String oper1);
  /** 
   * Checks whether the camera has finished preloading.
<br/><br/>
   * <b>Example:</b> camPreloaded _camera<br/>
   * @param oper1 camera
   * @since 2.92
   */
  /* Checks whether the camera has finished preloading.*/
  public native static boolean camPreloaded(GameObject oper1);
  /** 
   * Return the index of the current waypoint.
   * @param oper1 group
   * @since 5500
   */
  /* Return the index of the current waypoint.*/
  public native static float currentWaypoint(GameGroup oper1);
  /** 
   * Returns the object position in format <ar>Position</ar>.
<br/><br/>
   * <b>Example:</b> getPos player<br/>
   * @param oper1 obj
   */
  /* Returns the object position in format <ar>Position</ar>.*/
  public native static java.util.List getPos(GameObject oper1);
  /** 
   * Return (raw) position of given location.
   * @param oper1 location
   * @since 5501
   */
  /* Return (raw) position of given location.*/
  public native static java.util.List getPos(GameLocation oper1);
  /** 
   * Returns description of map sign mouse cursor is over.
   * @param oper1 control
   * @since 5501
   */
  /* Returns description of map sign mouse cursor is over.*/
  public native static java.util.List ctrlMapMouseOver(GameControl oper1);
  /** 
   * Returns terrain height above sea level.
<br/><br/>
   * <b>Example:</b>  z = getTerrainHeightASL  [x,y]<br/>
   * @param oper1 [x,y]
   * @since 1.18
   */
  /* Returns terrain height above sea level.*/
  public native static float getTerrainHeightASL(java.util.List oper1);
  /** 
   * Sets the additional text (invisible) in the item with the given position of the 2D listbox.
<br/><br/>
   * <b>Example:</b> lnbSetData [101, [0,1], "#1"]<br/>
   * @param oper1 [idc, [row, column], data]
   * @since 5501
   */
  /* Sets the additional text (invisible) in the item with the given position of the 2D listbox.*/
  public native static void lnbSetData(java.util.List oper1);
  /** 
   * Defines a sound (voice) that is played the first time when the Plan section in the briefing is selected.
<br/><br/>
   * <b>Example:</b> onBriefingPlan "PlanVoiceOver"<br/>
   * @param oper1 sound
   * @since 1.75
   */
  /* Defines a sound (voice) that is played the first time when the Plan section in the briefing is selected.*/
  public native static void onBriefingPlan(String oper1);
  /** 
   * Defines a sound (voice) that is played the first time when the Group section in the briefing is selected.
<br/><br/>
   * <b>Example:</b> onBriefingGroup "GroupVoiceOver"<br/>
   * @param oper1 sound
   * @since 1.75
   */
  /* Defines a sound (voice) that is played the first time when the Group section in the briefing is selected.*/
  public native static void onBriefingGroup(String oper1);
  /** 
   * Defines an action performed when the user clicks on the map. Command receives:<br/>
<br/>
_pos <t>array</t> position<br/>
_units <t>array</t> selected units<br/>
_shift,_alt <t>bool</t> key state<br/>
<br/>If the click is processed, command should return true.
<br/><br/>
   * <b>Example:</b> onMapSingleClick """SoldierEG"" createUnit [_pos, group player]"<br/>
   * @param oper1 command
   * @since 1.91
   */
  /* Defines an action performed when the user clicks on the map. Command receives:<br/>
<br/>
_pos <t>array</t> position<br/>
_units <t>array</t> selected units<br/>
_shift,_alt <t>bool</t> key state<br/>
<br/>If the click is processed, command should return true.*/
  public native static void onMapSingleClick(Object oper1);
  /** 
   * Returns an array with the type names of all the vehicle's magazines.
<br/><br/>
   * <b>Example:</b> magazines player<br/>
   * @param oper1 vehicle
   * @since 1.75
   */
  /* Returns an array with the type names of all the vehicle's magazines.*/
  public native static java.util.List magazines(GameObject oper1);
  /** 
   * Create a link to the section of diary given by subject. Record is selected based on given object (diary record, task or unit).
<br/><br/>
   * <b>Example:</b> _link = createDiarySubject ["Group", player, "Player"]<br/>
   * @param oper1 [subject, object, text]
   * @since 5500
   */
  /* Create a link to the section of diary given by subject. Record is selected based on given object (diary record, task or unit).*/
  public native static String createDiaryLink(java.util.List oper1);
  /** 
   * Returns the nearest object of the given type to the given position or object.
Pos may be using format [x,y,z, "type"] or [object, "type"].
<br/><br/>
   * <b>Example:</b> nearestObject [player, "StreetLamp"]<br/>
   * @param oper1 pos
   */
  /* Returns the nearest object of the given type to the given position or object.
Pos may be using format [x,y,z, "type"] or [object, "type"].*/
  public native static GameObject nearestObject(java.util.List oper1);
  /** 
   * Returns the current thumb position of the given slider.
<br/><br/>
   * <b>Example:</b> _pos = sliderPosition _control<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Returns the current thumb position of the given slider.*/
  public native static float sliderPosition(GameControl oper1);
  /** 
   * Returns the current thumb position of the slider with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _pos = sliderPosition 101<br/>
   * @param oper1 idc
   * @since 1.79
   */
  /* Returns the current thumb position of the slider with id idc of the topmost user dialog.*/
  public native static float sliderPosition(float oper1);
  /** 
   * Compile expression.
<br/><br/>
   * <b>Example:</b> _function = "a = a + 1"; _compiled = compile _function; call _compiled;<br/>
   * @param oper1 expression
   * @since 2.60
   */
  /* Compile expression.*/
  public native static GameCode compile(String oper1);
  /** 
   * Checks whether the unit is stopped using the "stop" command.
<br/><br/>
   * <b>Example:</b> stopped jeepOne<br/>
   * @param oper1 unit
   */
  /* Checks whether the unit is stopped using the "stop" command.*/
  public native static boolean stopped(GameObject oper1);
  /** 
   * Returns the speed (as an array [line, page]) of the given slider.
<br/><br/>
   * <b>Example:</b> _speed = sliderSpeed _control<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Returns the speed (as an array [line, page]) of the given slider.*/
  public native static java.util.List sliderSpeed(GameControl oper1);
  /** 
   * Returns the speed (as an array [line, page]) of the slider with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _speed = sliderSpeed<br/>
   * @param oper1 idc
   * @since 1.79
   */
  /* Returns the speed (as an array [line, page]) of the slider with id idc of the topmost user dialog.*/
  public native static java.util.List sliderSpeed(float oper1);
  /** 
   * Return the current command type (empty string when no command).
   * @param oper1 vehicle
   * @since 2.92
   */
  /* Return the current command type (empty string when no command).*/
  public native static String currentCommand(GameObject oper1);
  /** 
   * Check if config entry represents text.
<br/><br/>
   * <b>Example:</b> _ok = isText (configFile >> "CfgVehicles")<br/>
<br/>
   * <b>Example result:</b> false<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Check if config entry represents text.*/
  public native static boolean isText(GameConfig oper1);
  /** 
   * Extract number from config entry.
<br/><br/>
   * <b>Example:</b> _array = getNumber (configFile >> "CfgVehicles" >> "Thing" >> "maxSpeed")<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Extract number from config entry.*/
  public native static float getNumber(GameConfig oper1);
  /** 
   * Return whether the person  is hidden (reached the hiding position).
   * @param oper1 person
   * @since 2.92
   */
  /* Return whether the person  is hidden (reached the hiding position).*/
  public native static boolean isHidden(GameObject oper1);
  /** 
   * Return whether leader can issue attack commands.
   * @param oper1 group
   * @since 2.92
   */
  /* Return whether leader can issue attack commands.*/
  public native static boolean attackEnabled(Object oper1);
  /** 
   * Creates a new marker on the given position. The marker name has to be unique. The marker is created on all computers in a network session.
<br/><br/>
   * <b>Example:</b> marker = createMarker [Marker1, position player]<br/>
   * @param oper1 [name, position]
   * @since 1.86
   */
  /* Creates a new marker on the given position. The marker name has to be unique. The marker is created on all computers in a network session.*/
  public native static String createMarker(java.util.List oper1);
  /** 
   * Enables/disables engine's artillery.
   * @param oper1 true/false
   * @since 5501
   */
  /* Enables/disables engine's artillery.*/
  public native static void enableEngineArtillery(boolean oper1);
  /** 
   * Returns the name of the type of the given object.
<br/><br/>
   * <b>Example:</b> typeOf player<br/>
<br/>
   * <b>Example result:</b> "SoldierWB"<br/>
   * @param oper1 object
   * @since 1.91
   */
  /* Returns the name of the type of the given object.*/
  public native static String typeOf(GameObject oper1);
  /** 
   * Dumps the argument value into the debugging output.
<br/><br/>
   * <b>Example:</b> textLog player<br/>
   * @param oper1 anything
   */
  /* Dumps the argument value into the debugging output.*/
  public native static void textLog(Object oper1);
  /** 
   * Set the time interval to wait on player respawn. It is set to mission default on mission start again.
   * @param oper1 time interval
   * @since 1.57
   */
  /* Set the time interval to wait on player respawn. It is set to mission default on mission start again.*/
  public native static void setPlayerRespawnTime(float oper1);
  /** 
   * Returns the soldier assigned to the given vehicle as a driver.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Returns the soldier assigned to the given vehicle as a driver.*/
  public native static GameObject assignedDriver(GameObject oper1);
  /** 
   * Sets the text that will be shown in the control with id idc of the topmost user dialog. This can be used for static texts, buttons, edit lines and active texts.
<br/><br/>
   * <b>Example:</b> ctrlSetText [100, "Hello, world"]<br/>
   * @param oper1 [idc, text]
   * @since 1.50
   */
  /* Sets the text that will be shown in the control with id idc of the topmost user dialog. This can be used for static texts, buttons, edit lines and active texts.*/
  public native static void ctrlSetText(java.util.List oper1);
  /** 
   * The tangens of x, the argument is in degrees.
<br/><br/>
   * <b>Example:</b> tan 45<br/>
<br/>
   * <b>Example result:</b> 1<br/>
   * @param oper1 x
   */
  /* The tangens of x, the argument is in degrees.*/
  public native static float tan(float oper1);
  /** 
   * Checks whether the given vehicle is able to fire. It does not check for ammo, only for damage.
<br/><br/>
   * <b>Example:</b> canFire vehicle player<br/>
   * @param oper1 vehicle
   */
  /* Checks whether the given vehicle is able to fire. It does not check for ammo, only for damage.*/
  public native static boolean canFire(GameObject oper1);
  /** 
   * Transform position from camera coordinate space to world coordinate space.
<br/><br/>
   * <b>Example:</b> _worldPos = positionCameraToWorld _cameraPos<br/>
   * @param oper1 position
   * @since 2.52
   */
  /* Transform position from camera coordinate space to world coordinate space.*/
  public native static java.util.List positionCameraToWorld(java.util.List oper1);
  /** 
   * Gets the marker brush. See <f>setMarkerBrush</f>.
<br/><br/>
   * <b>Example:</b> markerBrush "MarkerOne"<br/>
   * @param oper1 name
   * @since 5501
   */
  /* Gets the marker brush. See <f>setMarkerBrush</f>.*/
  public native static String markerBrush(String oper1);
  /** 
   * Limit maximal bandwidth on server.
<br/><br/>
   * <b>Example:</b> diag_setMaxBandwidth 14400<br/>
   * @param oper1 bandwidth
   * @since 2.69
   */
  /* Limit maximal bandwidth on server.*/
  public native static void diag_setMaxBandwidth(float oper1);
  /** 
   * Returns the position of the task (as specified by destination parameter in config).
   * @param oper1 task
   * @since 2.92
   */
  /* Returns the position of the task (as specified by destination parameter in config).*/
  public native static java.util.List taskDestination(GameTask oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_densTree(float oper1);
  /** 
   * Returns group's HC commander.
<br/><br/>
   * <b>Example:</b> hcLeader group<br/>
   * @param oper1 group
   * @since 5501
   */
  /* Returns group's HC commander.*/
  public native static GameObject hcLeader(GameGroup oper1);
  /** 
   * Removes row with given index from the 2D listbox.
<br/><br/>
   * <b>Example:</b> lnbDeleteRow [105, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Removes row with given index from the 2D listbox.*/
  public native static void lnbDeleteRow(java.util.List oper1);
  /** 
   * Sets the color of the item with the given position of the 2D listbox. Color is in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> lnbSetColor [101, [0,1], [0, 1, 0, 0.5]]<br/>
   * @param oper1 [idc, [row, column], color]
   * @since 5501
   */
  /* Sets the color of the item with the given position of the 2D listbox. Color is in format <ar>Color</ar>.*/
  public native static void lnbSetColor(java.util.List oper1);
  /** 
   * Returns the name of the vehicle's primary weapon (an empty string if there is none).
<br/><br/>
   * <b>Example:</b> primaryWeapon player<br/>
   * @param oper1 vehicle
   * @since 1.75
   */
  /* Returns the name of the vehicle's primary weapon (an empty string if there is none).*/
  public native static String primaryWeapon(GameObject oper1);
  /** 
   * Return the result of the given task.
   * @param oper1 task
   * @since 2.92
   */
  /* Return the result of the given task.*/
  public native static java.util.List taskResult(GameTask oper1);
  /** 
   * Returns group icons params. [color, text,scale, visible]
<br/><br/>
   * <b>Example:</b> getGroupIconParams group<br/>
   * @param oper1 group
   * @since 5501
   */
  /* Returns group icons params. [color, text,scale, visible]*/
  public native static java.util.List getGroupIconParams(GameGroup oper1);
  /** 
   * Returns the object position in format <ar>PositionASL</ar>.
<br/><br/>
   * <b>Example:</b> getPosASL player<br/>
   * @param oper1 obj
   * @since 2.53
   */
  /* Returns the object position in format <ar>PositionASL</ar>.*/
  public native static java.util.List getPosASL(GameObject oper1);
  /** 
   * Opens gear dialog for given unit.
<br/><br/>
   * <b>Example:</b> CreateGearDialog [palyer,"RscDisplayGear"]<br/>
   * @param oper1 [unit,resource]
   * @since 5501
   */
  /* Opens gear dialog for given unit.*/
  public native static void CreateGearDialog(java.util.List oper1);
  /** 
   * Set the input focus on given control.
<br/><br/>
   * <b>Example:</b> ctrlSetFocus _control<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Set the input focus on given control.*/
  public native static void ctrlSetFocus(GameControl oper1);
  /** 
   * Sets camera shake params.
   * @param oper1 [posCoef, rotXCoef, rotYCoef, rotZCoef, interp]
   * @since 5501
   */
  /* Sets camera shake params.*/
  public native static void setCamShakeParams(java.util.List oper1);
  /** 
   * Sets the speed (a click on the arrow results in a move per line. A click on the scale outside the thumb results in a move per page) of the slider with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> sliderSetSpeed [100, 0.5, 2.0]<br/>
   * @param oper1 [idc, line, page]
   * @since 1.79
   */
  /* Sets the speed (a click on the arrow results in a move per line. A click on the scale outside the thumb results in a move per page) of the slider with id idc of the topmost user dialog.*/
  public native static void sliderSetSpeed(java.util.List oper1);
  /** 
   * Returns the number of magazines of type name in the weapon pool (used in campaigns to transfer weapons to the next mission).
   * @param oper1 name
   * @since 1.75
   */
  /* Returns the number of magazines of type name in the weapon pool (used in campaigns to transfer weapons to the next mission).*/
  public native static float queryMagazinePool(String oper1);
  /** 
   * Return a list of the resources belonging to a team.
   * @param oper1 teamMember
   * @since 2.92
   */
  /* Return a list of the resources belonging to a team.*/
  public native static java.util.List resources(GameTeamMember oper1);
  /** 
   * Deletes any unit or vehicle. Only vehicles inserted in the editor or created during a mission can be deleted. The player unit cannot be deleted.
<br/><br/>
   * <b>Example:</b> deleteVehicle tank<br/>
   * @param oper1 object
   * @since 1.34
   */
  /* Deletes any unit or vehicle. Only vehicles inserted in the editor or created during a mission can be deleted. The player unit cannot be deleted.*/
  public native static void deleteVehicle(GameObject oper1);
  /** 
   * Object title  - argument uses format ["text","type",speed] or ["name","type"]. Speed is ignored.
Preload data
The object can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> titleObj ["BISLogo","plain"]<br/>
   * @param oper1 effect
   */
  /* Object title  - argument uses format ["text","type",speed] or ["name","type"]. Speed is ignored.
Preload data
The object can be defined in the description.ext file.*/
  public native static boolean preloadTitleObj(java.util.List oper1);
  /** 
   * Returns whether given control is enabled.
<br/><br/>
   * <b>Example:</b> _ok = ctrlEnabled _control<br/>
   * @param oper1 control
   * @since 2.50
   */
  /* Returns whether given control is enabled.*/
  public native static boolean ctrlEnabled(GameControl oper1);
  /** 
   * Returns whether the control with id idc of the topmost user dialog is enabled.
<br/><br/>
   * <b>Example:</b> _enabled = ctrlEnabled 100<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Returns whether the control with id idc of the topmost user dialog is enabled.*/
  public native static boolean ctrlEnabled(float oper1);
  /** 
   * Return leader of the formation.
   * @param oper1 person
   * @since 2.92
   */
  /* Return leader of the formation.*/
  public native static GameObject formationLeader(GameObject oper1);
  /** 
   * Forces drawing of the cinema borders. This is normally used in cutscenes to indicate the player has no control.
   * @param oper1 show
   */
  /* Forces drawing of the cinema borders. This is normally used in cutscenes to indicate the player has no control.*/
  public native static void showCinemaBorder(boolean oper1);
  /** 
   * Adds camera shake source.
   * @param oper1 [power, duration, frquency]
   * @since 5501
   */
  /* Adds camera shake source.*/
  public native static void addCamShake(java.util.List oper1);
  /** 
   * Defines an action performed when the team switch is finished. Command receives:<br/>
<br/>
_from <t>object</t> previous unit<br/>
_to <t>object</t> current units<br/>
<br/>
   * @param oper1 command
   * @since 5500
   */
  /* Defines an action performed when the team switch is finished. Command receives:<br/>
<br/>
_from <t>object</t> previous unit<br/>
_to <t>object</t> current units<br/>
<br/>*/
  public native static void onTeamSwitch(Object oper1);
  /** 
   * The program will waste given time (in ms) each frame.
<br/><br/>
   * <b>Example:</b> diag_CPUStress 10<br/>
   * @param oper1 time
   * @since 2.92
   */
  /* The program will waste given time (in ms) each frame.*/
  public native static void diag_CPUStress(float oper1);
  /** 
   * Returns the limits (as an array [min, max]) of the given slider.
<br/><br/>
   * <b>Example:</b> _limits = sliderRange _control<br/>
   * @param oper1 control
   * @since 2.92
   */
  /* Returns the limits (as an array [min, max]) of the given slider.*/
  public native static java.util.List sliderRange(GameControl oper1);
  /** 
   * Returns the limits (as an array [min, max]) of the slider with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _limits = sliderRange 100<br/>
   * @param oper1 idc
   * @since 1.79
   */
  /* Returns the limits (as an array [min, max]) of the slider with id idc of the topmost user dialog.*/
  public native static java.util.List sliderRange(float oper1);
  /** 
   * Checks how much fuel is left in the gas tank, in the range from 0 to 1.
<br/><br/>
   * <b>Example:</b> fuel vehicle player<br/>
   * @param oper1 vehicle
   */
  /* Checks how much fuel is left in the gas tank, in the range from 0 to 1.*/
  public native static float fuel(GameObject oper1);
  /** 
   * Gets the waypoint combat mode.
<br/><br/>
   * <b>Example:</b> waypointCombatMode [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint combat mode.*/
  public native static String waypointCombatMode(java.util.List oper1);
  /** 
   * Sets the additional text (invisible) in the item with the given index of the listbox or combobox with id idc of the topmost user dialog to the given data.
<br/><br/>
   * <b>Example:</b> lbSetData [101, 1, "#1"]<br/>
   * @param oper1 [idc, index, data]
   * @since 1.50
   */
  /* Sets the additional text (invisible) in the item with the given index of the listbox or combobox with id idc of the topmost user dialog to the given data.*/
  public native static void lbSetData(java.util.List oper1);
  /** 
   * Returns selected groups in sgroup.
<br/><br/>
   * <b>Example:</b> array = groupSelectedUnits unit<br/>
   * @param oper1 unit
   * @since 5501
   */
  /* Returns selected groups in sgroup.*/
  public native static java.util.List groupSelectedUnits(GameObject oper1);
  /** 
   * Return the parent task of the specified task.
   * @param oper1 task
   * @since 2.92
   */
  /* Return the parent task of the specified task.*/
  public native static GameTask taskParent(GameTask oper1);
  /** 
   * Returns the object speed (in km/h).
<br/><br/>
   * <b>Example:</b> speed player<br/>
   * @param oper1 obj
   */
  /* Returns the object speed (in km/h).*/
  public native static float speed(GameObject oper1);
  /** 
   * MP: returns side score.
<br/><br/>
   * <b>Example:</b> score west<br/>
   * @param oper1 side
   */
  /* MP: returns side score.*/
  public native static float scoreSide(GameSide oper1);
  /** 
   * Returns the position as map coords.
<br/><br/>
   * <b>Example:</b> mapGridPosition player<br/>
   * @param oper1 object
   * @since 5501
   */
  /* Returns the position as map coords.*/
  public native static String mapGridPosition(Object oper1);
  /** 
   * Broadcasts the variable value to all computers.
Only type <t>Number</t> is supported in versions 1.33 and before.
The following types are supported since 1.34:
<t>Number</t>, <t>Boolean</t>, <t>Object</t> and <t>Group</t>.
<br/><br/>
   * <b>Example:</b> publicVariable "CTFscoreOne"<br/>
   * @param oper1 varName
   */
  /* Broadcasts the variable value to all computers.
Only type <t>Number</t> is supported in versions 1.33 and before.
The following types are supported since 1.34:
<t>Number</t>, <t>Boolean</t>, <t>Object</t> and <t>Group</t>.*/
  public native static void publicVariable(String oper1);
  /** 
   * Convert the string to upper case.
<br/><br/>
   * <b>Example:</b> toUpper "ArmA"<br/>
<br/>
   * <b>Example result:</b> "ARMA"<br/>
   * @param oper1 string
   * @since 5195
   */
  /* Convert the string to upper case.*/
  public native static String toUpper(String oper1);
  /** 
   * Removes all special items from the unit.
<br/><br/>
   * <b>Example:</b> removeAllItems player<br/>
   * @param oper1 unit
   * @since 5501
   */
  /* Removes all special items from the unit.*/
  public native static void removeAllItems(GameObject oper1);
  /** 
   * Detach light from object.
   * @param oper1 light
   * @since 2.58
   */
  /* Detach light from object.*/
  public native static void lightDetachObject(GameObject oper1);
  /** 
   * Destroys the camera.
   * @param oper1 camera
   */
  /* Destroys the camera.*/
  public native static void camDestroy(GameObject oper1);
  /** 
   * Returns the current fade factor of control.
<br/><br/>
   * <b>Example:</b> _scale = ctrlFade _control<br/>
   * @param oper1 control
   * @since 2.50
   */
  /* Returns the current fade factor of control.*/
  public native static float ctrlFade(GameControl oper1);
  /** 
   * Return the child tasks of the specified task.
   * @param oper1 task
   * @since 2.92
   */
  /* Return the child tasks of the specified task.*/
  public native static java.util.List taskChildren(GameTask oper1);
  /** 
   * Return the priority of the task.
   * @param oper1 task
   * @since 2.91
   */
  /* Return the priority of the task.*/
  public native static float priority(GameTask oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engReloadVS(String oper1);
  /** 
   * Checks whether the lampost is turned on. Possible values are "ON", "OFF" and "AUTO" (auto means the lamp will be shining during the night automatically).
<br/><br/>
   * <b>Example:</b> lightIsOn nearestObject [player, "StreetLamp"] != "OFF"<br/>
   * @param oper1 lamppost
   * @since 1.04
   */
  /* Checks whether the lampost is turned on. Possible values are "ON", "OFF" and "AUTO" (auto means the lamp will be shining during the night automatically).*/
  public native static String lightIsOn(GameObject oper1);
  /** 
   * Returns the list of soldiers assigned to the given vehicle as a cargo.
   * @param oper1 vehicle
   * @since 5501
   */
  /* Returns the list of soldiers assigned to the given vehicle as a cargo.*/
  public native static java.util.List assignedCargo(GameObject oper1);
  /** 
   * Detaches an object.
<br/><br/>
   * <b>Example:</b> detach player<br/>
   * @param oper1 obj
   * @since 5501
   */
  /* Detaches an object.*/
  public native static void detach(GameObject oper1);
  /** 
   * Returns the additional integer value in the item with the given index of the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _value = lbValue [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Returns the additional integer value in the item with the given index of the listbox or combobox with id idc of the topmost user dialog.*/
  public native static float lbValue(java.util.List oper1);
  /** 
   * Returns all units mounted in the given vehicle. If the vehicle is not a vehicle, but  person, a list containing only persons is returned.
<br/><br/>
   * <b>Example:</b> crew vehicle player<br/>
   * @param oper1 vehicle
   */
  /* Returns all units mounted in the given vehicle. If the vehicle is not a vehicle, but  person, a list containing only persons is returned.*/
  public native static java.util.List crew(GameObject oper1);
  /** 
   * Returns all mission objects with given type. 
<br/><br/>
   * <b>Example:</b> allMissionObjects "Air"<br/>
   * @param oper1 type
   */
  /* Returns all mission objects with given type. */
  public native static java.util.List allMissionObjects(String oper1);
  /** 
   * Make sure sound can start playing without any delay once we need it.
   * @param oper1 sound
   * @since 2.50
   */
  /* Make sure sound can start playing without any delay once we need it.*/
  public native static boolean preloadSound(String oper1);
  /** 
   * Returns the text color of the item with the given position of the 2D listbox. The color is returned in format <ar>Color</ar>.
<br/><br/>
   * <b>Example:</b> _color = lnbColor [101, [0,1]]<br/>
   * @param oper1 [idc, [row, column]
   * @since 5501
   */
  /* Returns the text color of the item with the given position of the 2D listbox. The color is returned in format <ar>Color</ar>.*/
  public native static java.util.List lnbColor(java.util.List oper1);
  /** 
   * Sets the current thumb position of the slider with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> sliderSetPosition [100, 0]<br/>
   * @param oper1 [idc, pos]
   * @since 1.79
   */
  /* Sets the current thumb position of the slider with id idc of the topmost user dialog.*/
  public native static void sliderSetPosition(java.util.List oper1);
  /** 
   * Sets the limits of the slider with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> sliderSetRange [100, 0, 10]<br/>
   * @param oper1 [idc, min, max]
   * @since 1.79
   */
  /* Sets the limits of the slider with id idc of the topmost user dialog.*/
  public native static void sliderSetRange(java.util.List oper1);
  /** 
   * Returns a list of all injuries of the given unit. Items are in format <ar>VBS_Injury</ar>.
   * @param oper1 name
   * @since 1.80
   */
  /* Returns a list of all injuries of the given unit. Items are in format <ar>VBS_Injury</ar>.*/
  public native static java.util.List VBS_injuries(String oper1);
  /** 
   * List all uncompleted tasks.
   * @param oper1 teamMember
   * @since 2.91
   */
  /* List all uncompleted tasks.*/
  public native static java.util.List currentTasks(GameTeamMember oper1);
  /** 
   * Return size of given location (width, height).
   * @param oper1 location
   * @since 2.90
   */
  /* Return size of given location (width, height).*/
  public native static java.util.List size(GameLocation oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engLimitSec(float oper1);
  /** 
   * Sends any text into the debugger console or the logfile.
<br/><br/>
   * <b>Example:</b> echo "Text in logfile"<br/>
   * @param oper1 text
   * @since 2.00
   */
  /* Sends any text into the debugger console or the logfile.*/
  public native static void echo(String oper1);
  /** 
   * Returns vehicle attached to the trigger (for example using <f>triggerAttachVehicle</f>)
   * @param oper1 trigger
   * @since 5101
   */
  /* Returns vehicle attached to the trigger (for example using <f>triggerAttachVehicle</f>)*/
  public native static GameObject triggerAttachedVehicle(GameObject oper1);
  /** 
   * Enable / disable showing of subtitles. Return the previous state.
   * @param oper1 enable
   * @since 5501
   */
  /* Enable / disable showing of subtitles. Return the previous state.*/
  public native static boolean showSubtitles(boolean oper1);
  /** 
   * Returns number of seconds until auto-scroll starts. -2 if scrollbar not present.
<br/><br/>
   * <b>Example:</b> _delay = ctrlAutoScrollDelay _control<br/>
   * @param oper1 control
   * @since 5501
   */
  /* Returns number of seconds until auto-scroll starts. -2 if scrollbar not present.*/
  public native static float ctrlAutoScrollDelay(GameControl oper1);
  /** 
   * The random real value from 0 to x. (0&lt;=random&lt;1)
<br/><br/>
   * <b>Example:</b> random 1<br/>
   * @param oper1 x
   */
  /* The random real value from 0 to x. (0&lt;=random&lt;1)*/
  public native static float random(float oper1);
  /** 
   * Checks whether current user is able to process the specified server command. See also <f>serverCommand</f>
<br/><br/>
   * <b>Example:</b> serverCommandAvailable "#kick"<br/>
   * @param oper1 server command
   */
  /* Checks whether current user is able to process the specified server command. See also <f>serverCommand</f>*/
  public native static boolean serverCommandAvailable(String oper1);
  /** 
   * MP: checks whether the given unit is local on the computer. This can be used when one or more activation fields or scripts need to be performed only on one computer.
In SP all objects are local. Note: all static objects are local on all computers.
<br/><br/>
   * <b>Example:</b> local unitName<br/>
   * @param oper1 obj
   */
  /* MP: checks whether the given unit is local on the computer. This can be used when one or more activation fields or scripts need to be performed only on one computer.
In SP all objects are local. Note: all static objects are local on all computers.*/
  public native static boolean local(GameObject oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engLimitDIP(float oper1);
  /** 
   * Returns container of given control.
<br/><br/>
   * <b>Example:</b> _display = ctrlParent _control<br/>
   * @param oper1 control
   * @since 2.50
   */
  /* Returns container of given control.*/
  public native static GameDisplay ctrlParent(GameControl oper1);
  /** 
   * Check if given difficulty setting is currently enabled. For possible values of flag, see config class Flags in CfgDificulties.
   * @param oper1 flag
   * @since 5127
   */
  /* Check if given difficulty setting is currently enabled. For possible values of flag, see config class Flags in CfgDificulties.*/
  public native static boolean difficultyEnabled(String oper1);
  /** 
   * Disables all user input. This is normally used only in cutscenes to disable the player's controls.
   * @param oper1 disable
   */
  /* Disables all user input. This is normally used only in cutscenes to disable the player's controls.*/
  public native static void disableUserInput(boolean oper1);
  /** 
   * Returns the current mode of the editor.
   * @param oper1 map
   * @since 2.92
   */
  /* Returns the current mode of the editor.*/
  public native static String getEditorMode(GameControl oper1);
  /** 
   * Switch player to given unit.
<br/><br/>
   * <b>Example:</b> selectPlayer aP<br/>
   * @param oper1 unit
   * @since 2.33
   */
  /* Switch player to given unit.*/
  public native static void selectPlayer(GameObject oper1);
  /** 
   * The arccosine of x, the result is in degrees.
<br/><br/>
   * <b>Example:</b> acos 0.5<br/>
<br/>
   * <b>Example result:</b> 60<br/>
   * @param oper1 x
   */
  /* The arccosine of x, the result is in degrees.*/
  public native static float acos(float oper1);
  /** 
   * Suspend execution of script for given uitime.
<br/><br/>
   * <b>Example:</b> uisleep 0.5<br/>
   * @param oper1 delay
   * @since 5501
   */
  /* Suspend execution of script for given uitime.*/
  public native static void uisleep(float oper1);
  /** 
   * Return the unit position rules.
   * @param oper1 person
   * @since 2.92
   */
  /* Return the unit position rules.*/
  public native static String unitPos(GameObject oper1);
  /** 
   * Returns the effective commander (who really commands) of the vehicle.
   * @param oper1 vehicle
   * @since 2.92
   */
  /* Returns the effective commander (who really commands) of the vehicle.*/
  public native static GameObject effectiveCommander(GameObject oper1);
  /** 
   * Sets the picture in the item with the given index of the listbox or combobox with id idc of the topmost user dialog. Name is the picture name. The picture is searched in the mission directory, the dtaExt subdirectory of the campaign directory and the dtaExt directory and the data bank (or directory).
<br/><br/>
   * <b>Example:</b> lbSetPicture [101, 0, "iskoda"]<br/>
   * @param oper1 [idc, index, name]
   * @since 1.50
   */
  /* Sets the picture in the item with the given index of the listbox or combobox with id idc of the topmost user dialog. Name is the picture name. The picture is searched in the mission directory, the dtaExt subdirectory of the campaign directory and the dtaExt directory and the data bank (or directory).*/
  public native static void lbSetPicture(java.util.List oper1);
  /** 
   * Checks whether the soldier's hands are hit (causing inaccurate aiming).
<br/><br/>
   * <b>Example:</b> handsHit leader player<br/>
   * @param oper1 soldier
   */
  /* Checks whether the soldier's hands are hit (causing inaccurate aiming).*/
  public native static float handsHit(GameObject oper1);
  /** 
   * Returns the behaviour mode of the given unit ("CARELESS","SAFE","AWARE","COMBAT" or "STEALTH").
<br/><br/>
   * <b>Example:</b> behaviour player<br/>
   * @param oper1 unit
   */
  /* Returns the behaviour mode of the given unit ("CARELESS","SAFE","AWARE","COMBAT" or "STEALTH").*/
  public native static String behaviour(GameObject oper1);
  /** 
   * Creates list of supported operators and type. Each field of array has format: "x:name" where x is 't' - type, 'n' - nullary operator, 'u' - unary operator, 'b' - binary operator. 'name' is operator's/type's name (in case operator, type of input operands is included). `mask` parameter can be empty string, or one of field. In this case, function returns empty array, if operator is not included in list. `mask` can contain wildcards, for example: *:name, t:*, t:name* or *:*.
<br/><br/>
   * <b>Example:</b> supportInfo "b:select*"<br/>
<br/>
   * <b>Example result:</b> ["b:ARRAY select SCALAR","b:ARRAY select BOOL"]<br/>
   * @param oper1 mask
   * @since 2.00
   */
  /* Creates list of supported operators and type. Each field of array has format: "x:name" where x is 't' - type, 'n' - nullary operator, 'u' - unary operator, 'b' - binary operator. 'name' is operator's/type's name (in case operator, type of input operands is included). `mask` parameter can be empty string, or one of field. In this case, function returns empty array, if operator is not included in list. `mask` can contain wildcards, for example: *:name, t:*, t:name* or *:*.*/
  public native static java.util.List supportInfo(String oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_drawmode(String oper1);
  /** 
   * Find the places with the max. value of _expression in the given area.
   * @param oper1 [_position, _radius, _expression, _precision, _sourcesCount]
   * @since 5500
   */
  /* Find the places with the max. value of _expression in the given area.*/
  public native static java.util.List selectBestPlaces(java.util.List oper1);
  /** 
   * Sets the rendering distance. Default is 900 meters. The accepted range is from 500 to 5000 meters.
<br/><br/>
   * <b>Example:</b> setviewdistance 2000<br/>
   * @param oper1 distance
   */
  /* Sets the rendering distance. Default is 900 meters. The accepted range is from 500 to 5000 meters.*/
  public native static void setViewDistance(float oper1);
  /** 
   * Activates the listed addons. The list of active addons is initialized during this function.
<br/><br/>
   * <b>Example:</b> activateAddons ["BISOFP"]<br/>
   * @param oper1 [addon1, ...]
   * @since 2.32
   */
  /* Activates the listed addons. The list of active addons is initialized during this function.*/
  public native static void activateAddons(java.util.List oper1);
  /** 
   * Checks whether the object is marked for weapons collection.
<br/><br/>
   * <b>Example:</b> marked = isMarkedForCollection truck<br/>
   * @param oper1 object
   * @since 2.13
   */
  /* Checks whether the object is marked for weapons collection.*/
  public native static boolean isMarkedForCollection(GameObject oper1);
  /** 
   * Return the name of the type of the currently using magazine (on the primary turret for vehicles).
   * @param oper1 vehicle
   * @since 5500
   */
  /* Return the name of the type of the currently using magazine (on the primary turret for vehicles).*/
  public native static String currentMagazine(GameObject oper1);
  /** 
   * Extract text from config entry.
<br/><br/>
   * <b>Example:</b> _array = getText (configFile >> "CfgVehicles" >> "Thing" >> "icon")<br/>
   * @param oper1 config
   * @since 2.35
   */
  /* Extract text from config entry.*/
  public native static String getText(GameConfig oper1);
  /** 
   * Gets the radius around the waypoint where is the waypoint completed.
<br/><br/>
   * <b>Example:</b> _radius = waypointCompletionRadius [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the radius around the waypoint where is the waypoint completed.*/
  public native static float waypointCompletionRadius(java.util.List oper1);
  /** 
   * Returns the number of items in the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _n = lbSize _control<br/>
   * @param oper1 control
   * @since 2.91
   */
  /* Returns the number of items in the given listbox or combobox.*/
  public native static float lbSize(GameControl oper1);
  /** 
   * Returns the number of items in the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _n = lbSize 101<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Returns the number of items in the listbox or combobox with id idc of the topmost user dialog.*/
  public native static float lbSize(float oper1);
  /** 
   * Gets the waypoint show/hide status.
<br/><br/>
   * <b>Example:</b> waypointShow [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint show/hide status.*/
  public native static String waypointShow(java.util.List oper1);
  /** 
   * Gets the marker text. See <f>setMarkerText</f>.
<br/><br/>
   * <b>Example:</b> markerText "MarkerOne"<br/>
   * @param oper1 markerName
   * @since 2.92
   */
  /* Gets the marker text. See <f>setMarkerText</f>.*/
  public native static String markerText(String oper1);
  /** 
   * Find display by its IDD.
<br/><br/>
   * <b>Example:</b> _display = findDisplay 1<br/>
   * @param oper1 idd
   * @since 2.54
   */
  /* Find display by its IDD.*/
  public native static GameDisplay findDisplay(float oper1);
  /** 
   * Returns the current position and size of control as [x, y, w, h] array.
<br/><br/>
   * <b>Example:</b> _pos = ctrlPosition _control<br/>
   * @param oper1 control
   * @since 2.50
   */
  /* Returns the current position and size of control as [x, y, w, h] array.*/
  public native static java.util.List ctrlPosition(GameControl oper1);
  /** 
   * The absolute value of x.
<br/><br/>
   * <b>Example:</b> abs -3<br/>
<br/>
   * <b>Example result:</b> 3<br/>
   * @param oper1 x
   */
  /* The absolute value of x.*/
  public native static float abs(float oper1);
  /** 
   * Object background - the argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The object can be defined in the description.ext file.
<br/><br/>
   * <b>Example:</b> cutObj ["TVSet", "plain"]<br/>
   * @param oper1 effect
   */
  /* Object background - the argument uses format ["name","type",speed] or ["name","type"]. If speed is not given, it's assumed to be one.
The object can be defined in the description.ext file.*/
  public native static void cutObj(java.util.List oper1);
  /** 
   * Returns whether water is on given position.
   * @param oper1 [x, y]
   * @since 2.58
   */
  /* Returns whether water is on given position.*/
  public native static boolean surfaceIsWater(java.util.List oper1);
  /** 
   * Terminate the title effect and set duration of the fade out phase to the given time.
<br/><br/>
   * <b>Example:</b> titleFadeIn 1.0<br/>
   * @param oper1 duration
   * @since 5126
   */
  /* Terminate the title effect and set duration of the fade out phase to the given time.*/
  public native static void titleFadeOut(float oper1);
  /** 
   * Returns an array of names of all the vehicle's weapons.
<br/><br/>
   * <b>Example:</b> weapons player<br/>
   * @param oper1 vehicle
   * @since 1.75
   */
  /* Returns an array of names of all the vehicle's weapons.*/
  public native static java.util.List weapons(GameObject oper1);
  /** 
   * The FSM debugger is opened for all FSMs executed by the unit.
<br/><br/>
   * <b>Example:</b> diag_DebugFSM player<br/>
   * @param oper1 unit
   * @since 5153
   */
  /* The FSM debugger is opened for all FSMs executed by the unit.*/
  public native static void diag_debugFSM(GameObject oper1);
  /** 
   * The FSM debugger is opened for the given mission level FSM. FSM handle is the number returned by the execFSM command.
   * @param oper1 FSM handle
   * @since 5500
   */
  /* The FSM debugger is opened for the given mission level FSM. FSM handle is the number returned by the execFSM command.*/
  public native static void diag_debugFSM(float oper1);
  /** 
   * The FSM debugger is opened for the FSM attached to this task.
   * @param oper1 task
   * @since 5153
   */
  /* The FSM debugger is opened for the FSM attached to this task.*/
  public native static void diag_debugFSM(GameTask oper1);
  /** 
   * Store the given amount of armory points.
   * @param oper1 points
   * @since 5500
   */
  /* Store the given amount of armory points.*/
  public native static void setArmoryPoints(float oper1);
  /** 
   * Return the rank of the given unit.
   * @param oper1 unit
   * @since 2.92
   */
  /* Return the rank of the given unit.*/
  public native static String rank(GameObject oper1);
  /** 
   * MP: returns the unit's score.
<br/><br/>
   * <b>Example:</b> score unitOne<br/>
   * @param oper1 unit
   */
  /* MP: returns the unit's score.*/
  public native static float score(GameObject oper1);
  /** 
   * Create trapezoidal map of oper field structure (for soldier).
   * @param oper1 [x, z]
   * @since 2.54
   */
  /* Create trapezoidal map of oper field structure (for soldier).*/
  public native static void diag_operFieldSoldierTrap(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_toggle(String oper1);
  /** 
   * Return how much vehicle wants to reload its weapons.
   * @param oper1 vehicle
   * @since 2.92
   */
  /* Return how much vehicle wants to reload its weapons.*/
  public native static float needReload(GameObject oper1);
  /** 
   * Defines an action performed after the preload screen finished.
   * @param oper1 command
   * @since 5501
   */
  /* Defines an action performed after the preload screen finished.*/
  public native static void onPreloadFinished(Object oper1);
  /** 
   * returns all magazines types and count from the cargo space.
<br/><br/>
   * <b>Example:</b> getMagazineCargo jeepOne<br/>
   * @param oper1 container
   */
  /* returns all magazines types and count from the cargo space.*/
  public native static java.util.List getMagazineCargo(GameObject oper1);
  /** 
   * Breaks block out scope named 'name'. Nil is returned.
   * @param oper1 name
   */
  /* Breaks block out scope named 'name'. Nil is returned.*/
  public native static void breakOut(String oper1);
  /** 
   * Destroy given team.
<br/><br/>
   * <b>Example:</b> deleteTeam _team<br/>
   * @param oper1 team
   * @since 2.90
   */
  /* Destroy given team.*/
  public native static void deleteTeam(GameTeamMember oper1);
  /** 
   * Returns the speed mode of the group ("LIMITED", "NORMAL" or "FULL").
<br/><br/>
   * <b>Example:</b> speedMode group player<br/>
   * @param oper1 grp
   */
  /* Returns the speed mode of the group ("LIMITED", "NORMAL" or "FULL").*/
  public native static String speedMode(Object oper1);
  /** 
   * Returns value representing type of control.
<br/><br/>
   * <b>Example:</b> _type = ctrlType _control<br/>
   * @param oper1 control
   * @since 2.56
   */
  /* Returns value representing type of control.*/
  public native static float ctrlType(GameControl oper1);
  /** 
   * Returns the formation of the group ("COLUMN", "STAG COLUMN", "WEDGE", "ECH LEFT", "ECH RIGHT", "VEE" or "LINE").
<br/><br/>
   * <b>Example:</b> formation group player<br/>
   * @param oper1 grp
   */
  /* Returns the formation of the group ("COLUMN", "STAG COLUMN", "WEDGE", "ECH LEFT", "ECH RIGHT", "VEE" or "LINE").*/
  public native static String formation(Object oper1);
  /** 
   * Return a formation of given team.
<br/><br/>
   * <b>Example:</b> _formation = formation _team<br/>
   * @param oper1 team
   * @since 2.90
   */
  /* Return a formation of given team.*/
  public native static String formation(GameTeamMember oper1);
  /** 
   * Shortcut to textLog format [format, arg1, arg2, ...] (for better performance in retail version).
   * @param oper1 [format, arg1, arg2, ...]
   * @since 5501
   */
  /* Shortcut to textLog format [format, arg1, arg2, ...] (for better performance in retail version).*/
  public native static void textLogFormat(java.util.List oper1);
  /** 
   * Shows a text hint only when using cadet mode. The text can contain several lines. \n is used to indicate the end of a line.
<br/><br/>
   * <b>Example:</b> hintCadet "Press W to move forward"<br/>
   * @param oper1 text
   */
  /* Shows a text hint only when using cadet mode. The text can contain several lines. \n is used to indicate the end of a line.*/
  public native static void hintCadet(Object oper1);
  /** 
   * Sets custom camera apreture (-1 to do it automatically).
   * @param oper1 set
   * @since 2.73
   */
  /* Sets custom camera apreture (-1 to do it automatically).*/
  public native static void setAperture(float oper1);
  /** 
   * Checks whether the engine is on.
<br/><br/>
   * <b>Example:</b> on = isEngineOn vehicle player<br/>
   * @param oper1 vehicle
   * @since 1.90
   */
  /* Checks whether the engine is on.*/
  public native static boolean isEngineOn(GameObject oper1);
  /** 
   * Returns a list of currently selected editor objects.
   * @param oper1 map
   * @since 2.92
   */
  /* Returns a list of currently selected editor objects.*/
  public native static void selectedEditorObjects(GameControl oper1);
  /** 
   * Sets given event handler of in-game UI.
   * @param oper1 [handler name, function]
   * @since 2.91
   */
  /* Sets given event handler of in-game UI.*/
  public native static void inGameUISetEventHandler(java.util.List oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engFlushTexPerf(float oper1);
  /** 
   * Returns what surface is on given position.
   * @param oper1 [x, y]
   * @since 2.58
   */
  /* Returns what surface is on given position.*/
  public native static String surfaceType(java.util.List oper1);
  /** 
   * Gets the waypoint description.
<br/><br/>
   * <b>Example:</b> waypointDescription [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 5500
   */
  /* Gets the waypoint description.*/
  public native static String waypointDescription(java.util.List oper1);
  /** 
   * Returns camera's target.
<br/><br/>
   * <b>Example:</b> camTarget _camera<br/>
   * @param oper1 camera
   * @since 5501
   */
  /* Returns camera's target.*/
  public native static GameObject camTarget(GameObject oper1);
  /** 
   * Returns the name of the variable which contains a reference to this object.
   * @param oper1 object
   * @since 2.91
   */
  /* Returns the name of the variable which contains a reference to this object.*/
  public native static String vehicleVarName(GameObject oper1);
  /** 
   * Returns the combat mode of the given unit ("BLUE","GREEN","YELLOW" or "RED").
<br/><br/>
   * <b>Example:</b> combatMode group player<br/>
   * @param oper1 grp
   */
  /* Returns the combat mode of the given unit ("BLUE","GREEN","YELLOW" or "RED").*/
  public native static String combatMode(Object oper1);
  /** 
   * Process the specified server command.
The Server command string is identical to that used in chat line in MP.
<br/><br/>
   * <b>Example:</b> serverCommand "#kick friendlykiller"<br/>
   * @param oper1 server command
   */
  /* Process the specified server command.
The Server command string is identical to that used in chat line in MP.*/
  public native static boolean serverCommand(String oper1);
  /** 
   * Removes all magazines from the vehicle cargo space. MP synchronized.
<br/><br/>
   * <b>Example:</b> clearMagazineCargoGlobal jeepOne<br/>
   * @param oper1 unit
   */
  /* Removes all magazines from the vehicle cargo space. MP synchronized.*/
  public native static void clearMagazineCargoGlobal(GameObject oper1);
  /** 
   * Adds count magazines of type name into the weapon pool (used in the campaign to transfer weapons to the next mission).
   * @since 1.75
   */
  /* Adds count magazines of type name into the weapon pool (used in the campaign to transfer weapons to the next mission).*/
  public native static void addMagazinePool(java.util.List oper1);
  /** 
   * Returns all group icons.[[id,icon,[offsetx,offsety],[..],..]
<br/><br/>
   * <b>Example:</b> getGroupIcons group<br/>
   * @param oper1 group
   * @since 5501
   */
  /* Returns all group icons.[[id,icon,[offsetx,offsety],[..],..]*/
  public native static java.util.List getGroupIcons(GameGroup oper1);
  /** 
   * Return object's direction vector in world coordinates as [x, z, y].
   * @param oper1 obj
   * @since 2.61
   */
  /* Return object's direction vector in world coordinates as [x, z, y].*/
  public native static java.util.List vectorDir(GameObject oper1);
  /** 
   * Removes all icon from group.
<br/><br/>
   * <b>Example:</b> clearGroupIcons group<br/>
   * @param oper1 group
   * @since 5501
   */
  /* Removes all icon from group.*/
  public native static void clearGroupIcons(GameGroup oper1);
  /** 
   * Returns the additional text (invisible) in an item with the given index of the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> _data = lbData [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Returns the additional text (invisible) in an item with the given index of the listbox or combobox with id idc of the topmost user dialog.*/
  public native static String lbData(java.util.List oper1);
  /** 
   * Adds text into the header section of the mpreport.txt file.
   * @param oper1 text
   * @since 1.80
   */
  /* Adds text into the header section of the mpreport.txt file.*/
  public native static void VBS_addHeader(String oper1);
  /** 
   * Find the nearest locations (from the given position) of certain types, within the specified distance. If &lt;position to sort from&gt; is provided, locations will be ordered by distance from this point.
   * @param oper1 [position, [types], distance, <position to sort from>]
   * @since 2.92
   */
  /* Find the nearest locations (from the given position) of certain types, within the specified distance. If &lt;position to sort from&gt; is provided, locations will be ordered by distance from this point.*/
  public native static java.util.List nearestLocations(java.util.List oper1);
  /** 
   * Removes the item with the given index from the listbox or combobox with id idc of the topmost user dialog.
<br/><br/>
   * <b>Example:</b> lbDelete [101, 0]<br/>
   * @param oper1 [idc, index]
   * @since 1.50
   */
  /* Removes the item with the given index from the listbox or combobox with id idc of the topmost user dialog.*/
  public native static void lbDelete(java.util.List oper1);
  /** 
   * Converts x from degrees to radians.
<br/><br/>
   * <b>Example:</b> rad 180<br/>
<br/>
   * <b>Example result:</b> 3.1415<br/>
   * @param oper1 x
   */
  /* Converts x from degrees to radians.*/
  public native static float rad(float oper1);
  /** 
   * Return the precision of the given entity.
   * @param oper1 entity
   * @since 2.92
   */
  /* Return the precision of the given entity.*/
  public native static float precision(GameObject oper1);
  /** 
   * Gets the waypoint position. The format of waypoint is <ar>Waypoint</ar>.
<br/><br/>
   * <b>Example:</b> getWPPos [groupOne, 1]<br/>
   * @param oper1 waypoint
   * @since 1.21
   */
  /* Gets the waypoint position. The format of waypoint is <ar>Waypoint</ar>.*/
  public native static java.util.List getWPPos(java.util.List oper1);
  /** 
   * Returns whether the control with id idc of the topmost user dialog is visible.
<br/><br/>
   * <b>Example:</b> _visible = ctrlVisible 100<br/>
   * @param oper1 idc
   * @since 1.50
   */
  /* Returns whether the control with id idc of the topmost user dialog is visible.*/
  public native static boolean ctrlVisible(float oper1);
  /** 
   * Dumps the argument's type and value to the debugging output.
<br/><br/>
   * <b>Example:</b> debugLog player<br/>
   * @param oper1 anything
   */
  /* Dumps the argument's type and value to the debugging output.*/
  public native static void debugLog(Object oper1);
  /** 
   * Get unique player id.
   * @param oper1 unit
   * @since 5501
   */
  /* Get unique player id.*/
  public native static String getPlayerUID(GameObject oper1);
  /** 
   * Returns the life state of the given unit.
   * @param oper1 unit
   * @since 5501
   */
  /* Returns the life state of the given unit.*/
  public native static String lifeState(GameObject oper1);
  /** 
   * Returns the number of items in the given listbox or combobox.
<br/><br/>
   * <b>Example:</b> _n = lbSize _control<br/>
   * @param oper1 control
   * @since 2.91
   */
  /* Returns the number of items in the given listbox or combobox.*/
  public native static java.util.List lnbSize(GameControl oper1);
  /** 
   * Returns [X,Y] size of 2D listbox.
<br/><br/>
   * <b>Example:</b> _n = lnbSize 105<br/>
   * @param oper1 idc
   * @since 5501
   */
  /* Returns [X,Y] size of 2D listbox.*/
  public native static java.util.List lnbSize(float oper1);
  /** 
   * Creates a new AI center for the given side.
<br/><br/>
   * <b>Example:</b> center = createCenter East<br/>
   * @param oper1 side
   * @since 1.86
   */
  /* Creates a new AI center for the given side.*/
  public native static GameSide createCenter(GameSide oper1);
  /** 
   * Creates a structured text containing the given image.
<br/><br/>
   * <b>Example:</b> txt1 = image "data\isniper.paa"<br/>
   * @param oper1 filename
   * @since 2.01
   */
  /* Creates a structured text containing the given image.*/
  public native static GameText image(String oper1);
  /** 
   * Return the size of the entity of given type.
   * @param oper1 typeName
   * @since 5160
   */
  /* Return the size of the entity of given type.*/
  public native static float sizeOf(String oper1);
  /** 
   * Return the current task of the unit in the formation.
   * @param oper1 person
   * @since 2.92
   */
  /* Return the current task of the unit in the formation.*/
  public native static String formationTask(GameObject oper1);
  /** 
   * Enable / disable saving of the game. When disabled, the autosave is created (if not forbidden by save == false).
   * @param oper1 enable or [enable, save]
   * @since 5501
   */
  /* Enable / disable saving of the game. When disabled, the autosave is created (if not forbidden by save == false).*/
  public native static void enableSaving(Object oper1);
  /** 
   * Removes all magazines from the vehicle cargo space. MP synchronized.
<br/><br/>
   * <b>Example:</b> clearMagazineCargoGlobal jeepOne<br/>
   * @param oper1 unit
   */
  /* Removes all magazines from the vehicle cargo space. MP synchronized.*/
  public native static void clearBackpackCargoGlobal(GameObject oper1);
  /** 
   * Return importance of given location.
   * @param oper1 location
   * @since 2.90
   */
  /* Return importance of given location.*/
  public native static float importance(GameLocation oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engLimitVSC(float oper1);
  /** 
   * 
   */
  /* */
  public native static void diag_engLimitTri(float oper1);
  /** 
   * Checks whether the player has the watch enabled.
   */
  public native static boolean shownWatch();
  /** 
   * Checks whether the player has the ID card enabled.
(Obsolete).
   */
  public native static boolean shownWarrant();
  /** 
   * Return the region where the game was sold (based on distribution id).
   * @since 5129
   */
  public native static float distributionRegion();
  /** 
   * Returns number of frame currently displayed .
   * @since 5500
   */
  public native static float diag_frameno();
  /** 
   * 
   * @since 5501
   */
  public native static float SafeZoneW();
  /** 
   * 
   * @since 5501
   */
  public native static float SafeZoneH();
  /** 
   * 
   */
  public native static void diag_engFlushVB();
  /** 
   * Disable saving of script containing this command. After this, script can work with the data types which do not support serialization (UI types).
   * @since 5501
   */
  public native static void disableSerialization();
  /** 
   * Checks whether the player has the radio transmitter enabled.
   */
  public native static boolean shownRadio();
  /** 
   * The Enemy side (used for renegades).
   * @since 1.78
   */
  public native static GameSide sideEnemy();
  /** 
   * Returns the vehicle to which the camera is attached.
   * @since 1.56
   */
  public native static GameObject cameraOn();
  /** 
   * Returns the time of the mission start in format [year, month, day, hour, minute, second].
   * @since 1.80
   */
  public native static java.util.List missionStart();
  /** 
   * Return the time (in seconds) when the next weather change will occur.
   * @since 2.92
   */
  public native static float nextWeatherChange();
  /** 
   * Return a list of all groups.
   * @since 5501
   */
  public native static java.util.List allGroups();
  /** 
   * 
   * @since 5501
   */
  public native static float SafeZoneX();
  /** 
   * This is the entity pointed to by a players cursor.
<br/><br/>
   * <b>Example:</b> alive cursorTarget<br/>
   * @since 5500
   */
  public native static GameObject cursorTarget();
  /** 
   * A non-existing control. This value is not equal to anything, including itself.
   * @since 2.92
   */
  public native static GameControl controlNull();
  /** 
   * Create histogram of convex components in OperFields.
   * @since 2.54
   */
  public native static void diag_operFieldsHistogram();
  /** 
   * A non-existing task. This value is not equal to anything, including itself.
<br/><br/>
   * <b>Example:</b> taskNull == taskNull<br/>
<br/>
   * <b>Example result:</b> false<br/>
   * @since 5153
   */
  public native static GameTask taskNull();
  /** 
   * Returns the player's side. This is valid even when the player controlled person is dead (a difference from player side).
   * @since 2.09
   */
  public native static GameSide playerSide();
  /** 
   * Returns the time in the world, in hours.
   */
  public native static float dayTime();
  /** 
   * Checks the current radio volume (set by <f>setRadioVolume</f>).
   */
  public native static float radioVolume();
  /** 
   * Return the actual mission date and time as an array [year, month, day, hour, minute].
   * @since 2.92
   */
  public native static java.util.List date();
  /** 
   * 
   * @since 5501
   */
  public native static float SafeZoneY();
  /** 
   * Return type of camera.
   * @since 5501
   */
  public native static String cameraView();
  /** 
   * Removes all magazines from the weapon pool (this is used in campaigns to transfer weapons to the next mission).
   * @since 1.75
   */
  public native static void clearMagazinePool();
  /** 
   * Nil value. This value can be used to undefine an existing variable.
<br/><br/>
   * <b>Example:</b> variableToDestroy = nil<br/>
   */
  public native static Object nil();
  /** 
   * Returns the map elevation offset from [map]/config.cpp
   */
  public native static float getElevationOffset();
  /** 
   * The unknown side.
   * @since 5501
   */
  public native static GameSide sideUnknown();
  /** 
   * Clean up the content of radio protocol history.
   * @since 2.73
   */
  public native static void clearRadio();
  /** 
   * Check if saving the game is enabled.
   * @since 5501
   */
  public native static boolean savingEnabled();
  /** 
   * Creates a server diagnostics window.
   * @since 2.01
   */
  public native static void diag_server();
  /** 
   * Generate c:\comref.xml file.
   */
  public native static void generateComRef();
  /** 
   * 
   */
  public native static void diag_engStressVMClear();
  /** 
   * A non-existing group. This value is not equal to anything, including itself.
<br/><br/>
   * <b>Example:</b> group player == objNull<br/>
<br/>
   * <b>Example result:</b> false<br/>
   */
  public native static GameGroup grpNull();
  /** 
   * Checks whether the player has the map enabled.
   */
  public native static boolean shownMap();
  /** 
   * Checks whether the player has the notebook enabled.
   */
  public native static boolean shownPad();
  /** 
   * The Civilian side.
   */
  public native static GameSide civilian();
  /** 
   * The Friendly side (used for captives).
   * @since 1.78
   */
  public native static GameSide sideFriendly();
  /** 
   * Creates an autosave game (used for Retry).
   */
  public native static void saveGame();
  /** 
   * Checks whether the map animation has finished.
   * @since 1.27
   */
  public native static boolean mapAnimDone();
  /** 
   * Finish world initialization before mission is launched.
   * @since 2.33
   */
  public native static void finishMissionInit();
  /** 
   * Return the current fog.
   * @since 2.92
   */
  public native static float fog();
  /** 
   * Check if tam switch is currently enabled.
   * @since 2.92
   */
  public native static boolean teamSwitchEnabled();
  /** 
   * Return a list of all units (all persons except agents).
   * @since 5501
   */
  public native static java.util.List allUnits();
  /** 
   * Return a list of all groups.
   * @since 5501
   */
  public native static boolean hcShownBar();
  /** 
   * Real time spent from the start of the game.
   * @since 5501
   */
  public native static float diag_tickTime();
  /** 
   * Removes all weapons from the weapon pool (this is used in campaigns to transfer weapons to the next mission).
   * @since 1.75
   */
  public native static void clearWeaponPool();
  /** 
   * Return a list of agents in the current mission.
<br/><br/>
   * <b>Example:</b> _agents = agents<br/>
   * @since 2.92
   */
  public native static java.util.List agents();
  /** 
   * Show debug window with GeographyInfo QuadTree.
   * @since 2.54
   */
  public native static void diag_geography();
  /** 
   * A non-existing object. This value is not equal to anything, including itself.
<br/><br/>
   * <b>Example:</b> player == objNull<br/>
<br/>
   * <b>Example result:</b> false<br/>
   */
  public native static GameObject objNull();
  /** 
   * Returns the time that elapsed since the mission started (in seconds).
   */
  public native static float time();
  /** 
   * Returns the value of "3D performance" as it is in OFP preferences. This can be used to estimate computer performance. The function can be used to create missions that are able to adapt to computer peformance.
<br/><br/>
   * <b>Example:</b> ? benchmark>2000 : setviewdistance 2000<br/>
   */
  public native static float benchmark();
  /** 
   * Switch player to no unit.
   * @since 5501
   */
  public native static void selectNoPlayer();
  /** 
   * Estimated end of MP game converted to serverTime.
   * @since 5501
   */
  public native static float estimatedEndServerTime();
  /** 
   * Resets all camera shakes.
   * @since 5501
   */
  public native static void resetCamShake();
  /** 
   * A non-existing team member. This value is not equal to anything, including itself.
<br/><br/>
   * <b>Example:</b> _teamMember == teamMemberNull<br/>
<br/>
   * <b>Example result:</b> false<br/>
   */
  public native static GameTeamMember teamMemberNull();
  /** 
   * Creates a client diagnostics window.
   * @since 2.01
   */
  public native static void diag_client();
  /** 
   * Create map of numbers of convex components in OperFields (for soldiers).
   * @since 2.54
   */
  public native static void diag_operFieldsVehicles();
  /** 
   * pi (180 degrees converted to radians)
<br/><br/>
   * <b>Example:</b> pi<br/>
<br/>
   * <b>Example result:</b> 3.1415<br/>
   */
  public native static float pi();
  /** 
   * Returns true if the computer is the server.
   * @since 2.92
   */
  public native static boolean isServer();
  /** 
   * Return the fog forecast.
   * @since 2.92
   */
  public native static float fogForecast();
  /** 
   * Return if mouse vertical axe is inverted.
   * @since 5501
   */
  public native static boolean reversedMouseY();
  /** 
   * Finish loading screen displaying (started by startLoadingScreen).
   * @since 5501
   */
  public native static void endLoadingScreen();
  /** 
   * Return true if multiPlayer.
   * @since 5501
   */
  public native static boolean isMultiplayer();
  /** 
   * Return the content of the (text) clipboard.
   * @since 5500
   */
  public native static String copyFromClipboard();
  /** 
   * Return if groups icon raises onClick and onOver events.
   * @since 5501
   */
  public native static boolean groupIconSelectable();
  /** 
   * Return the global namespace attached to mission.
   * @since 5501
   */
  public native static GameNamespace missionNamespace();
  /** 
   * Return the global namespace attached to user interface.
   * @since 5501
   */
  public native static GameNamespace uiNamespace();
  /** 
   * Returns the rendering distance.
   * @since 5501
   */
  public native static float viewDistance();
  /** 
   * Returns whether a user dialog is present.
   * @since 1.78
   */
  public native static boolean dialog();
  /** 
   * Stops video capturing.
   * @since 5164
   */
  public native static void diag_stopVideo();
  /** 
   * Create text file containing internal development CDkeys.
   * @since 2.54
   */
  public native static void diag_internalKeys();
  /** 
   * This is the person controlled by the player. In MP this value is different on each computer.
<br/><br/>
   * <b>Example:</b> alive player<br/>
   */
  public native static GameObject player();
  /** 
   * Checks whether the player has the compass enabled.
   */
  public native static boolean shownCompass();
  /** 
   * The West side.
   */
  public native static GameSide west();
  /** 
   * Exits the script.
   */
  public native static void exit();
  /** 
   * Clears the map animation.
   * @since 1.27
   */
  public native static void mapAnimClear();
  /** 
   * Plays the map animation.
   * @since 1.27
   */
  public native static void mapAnimCommit();
  /** 
   * Return a list of vehicles in the current mission.
<br/><br/>
   * <b>Example:</b> _vehicles = vehicles<br/>
   * @since 2.92
   */
  public native static java.util.List vehicles();
  /** 
   * Return a list of units accessible through team switch.
   * @since 2.92
   */
  public native static java.util.List switchableUnits();
  /** 
   * Return the player remaining time to respawn.
   * @since 2.92
   */
  public native static float playerRespawnTime();
  /** 
   * Return a list of playable units (occupied by both AI or players).
   * @since 5501
   */
  public native static java.util.List playableUnits();
  /** 
   * Open MP interface.
   * @since 5501
   */
  public native static void openDSInterface();
  /** 
   * Creates a structured text containing a line break.
<br/><br/>
   * <b>Example:</b> txt3 = lineBreak<br/>
   * @since 2.01
   */
  public native static GameText lineBreak();
  /** 
   * Checks whether cheats are enabled (whether the designers' version is running).
   * @since 1.56
   */
  public native static boolean cheatsEnabled();
  /** 
   * Exports the units target lists to the Knowledge Base structures.
   * @since 2.06
   */
  public native static void diag_exportKB();
  /** 
   * The Logic side.
   * @since 1.86
   */
  public native static GameSide sideLogic();
  /** 
   * Launch init.sqs script.
   * @since 2.33
   */
  public native static void runInitScript();
  /** 
   * Return the name of the currently loaded world.
   * @since 2.92
   */
  public native static String worldName();
  /** 
   * Returns average framerate over last 16 frames.
   * @since 5500
   */
  public native static float diag_fps();
  /** 
   * Checks the current sound volume (set by <f>setSoundVolume</f>).
   */
  public native static float soundVolume();
  /** 
   * Return a list of dead units and destroyed vehicles. Dead unit might be inside vehicle.,
   * @since 5501
   */
  public native static java.util.List allDead();
  /** 
   * Return the global namespace attached to config parser.
   * @since 5501
   */
  public native static GameNamespace parsingNamespace();
  /** 
   * Return root of mission description.ext entries hierarchy.
   * @since 2.90
   */
  public native static GameConfig missionConfigFile();
  /** 
   * Stops the program into a debugger.
<br/><br/>
   * <b>Example:</b> halt<br/>
   * @since 2.00
   */
  public native static void halt();
  /** 
   * The East side.
   */
  public native static GameSide east();
  /** 
   * Return the current overcast.
   * @since 2.92
   */
  public native static float overcast();
  /** 
   * Return the current rain.
   * @since 2.92
   */
  public native static float rain();
  /** 
   * Invoke the team switch dialog (force it even when conditions are not met).
   * @since 2.92
   */
  public native static void teamSwitch();
  /** 
   * 
   * @since 5501
   */
  public native static float SafeZoneXAbs();
  /** 
   * returns [width, height, 2D viewport Width, 2D viewport Height, aspect ration, UI scale]
   * @since 5501
   */
  public native static float getResolution();
  /** 
   * Return true if the main map is shown (active).
   * @since 5501
   */
  public native static boolean visibleMap();
  /** 
   * Server time synchronized to clients.
   * @since 5501
   */
  public native static float serverTime();
  /** 
   * Create map of numbers of convex components in OperFields (for soldiers).
   * @since 2.54
   */
  public native static void diag_operFieldsSoldiers();
  /** 
   * Return the stored value of armory points.
   * @since 5500
   */
  public native static float armoryPoints();
  /** 
   * Return group icons are visible.
   * @since 5501
   */
  public native static java.util.List groupIconsVisible();
  /** 
   * Return true for dedicated server.
   * @since 5501
   */
  public native static boolean isDedicated();
  /** 
   * Return root of config entries hierarchy.
   * @since 2.35
   */
  public native static GameConfig configFile();
  /** 
   * 
   */
  public native static void diag_engFlushTex();
  /** 
   * Checks whether the player has the GPS receiver enabled.
   */
  public native static boolean shownGps();
  /** 
   * Checks the current music volume (set by <f>fadeMusic</f>).
   */
  public native static float musicVolume();
  /** 
   * Process commands stored using setVehicleInit.
   * @since 2.33
   */
  public native static void processInitCommands();
  /** 
   * Return the current wind vector.
   * @since 2.92
   */
  public native static java.util.List wind();
  /** 
   * Return the overcast forecast.
   * @since 2.92
   */
  public native static float overcastForecast();
  /** 
   * Load a game from the autosave, if failed, restart the mission.
   * @since 5501
   */
  public native static void loadGame();
  /** 
   * A non-existing display. This value is not equal to anything, including itself.
   * @since 2.92
   */
  public native static GameDisplay displayNull();
  /** 
   * Return a list of teams in the current mission.
<br/><br/>
   * <b>Example:</b> _teams = teams<br/>
   * @since 2.92
   */
  public native static java.util.List teams();
  /** 
   * Analyze the captured frame and show results in the dialog.
   * @since 5501
   */
  public native static void diag_captureDialog();
  /** 
   * A non-existing location. This value is not equal to anything, including itself.
   * @since 5501
   */
  public native static GameLocation locationNull();
  /** 
   * 
   */
  public native static void diag_engStressTexClear();
  /** 
   * Returns true if the mission is played in cadet mode and false in veteran mode.
   */
  public native static boolean cadetMode();
  /** 
   * Returns the current time acceleration factor.
   */
  public native static float accTime();
  /** 
   * The Resistance side.
   */
  public native static GameSide resistance();
  /** 
   * Enables the dialog buttons to be shown during the OnPlayerKilled script.
   */
  public native static void enableEndDialog();
  /** 
   * Forces the mission to terminate.
   */
  public native static void forceEnd();
  /** 
   * Returns the name of the current mission.
   * @since 1.80
   */
  public native static String missionName();
  /** 
   * Initialize the ambient life.
   * @since 2.90
   */
  public native static void initAmbientLife();
  /** 
   * Return the name of the topmost commanding menu.
   * @since 5501
   */
  public native static String commandingMenu();
  /** 
   * Returns minimal framerate. Calculated from the longest frame over last 16 frames.
   * @since 5500
   */
  public native static float diag_fpsmin();
  /** 
   * 
   * @since 5501
   */
  public native static float SafeZoneWAbs();
  /** 
   * Return root of campaign description.ext entries hierarchy.
   * @since 2.90
   */
  public native static GameConfig campaignConfigFile();
  /** 
   * Destroy the top most JVM panel.
   * @since 86290
   */
  public native static void jRemovePanel();
  // conversion from RVEngine.GameObject to Entity
  public native static Entity convert(GameObject value);
}
