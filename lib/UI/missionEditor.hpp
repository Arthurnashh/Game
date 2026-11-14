/*!
\file
Mission editor functions and classes
*/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MISSION_EDITOR_HPP
#define _MISSION_EDITOR_HPP

#if _ENABLE_EDITOR2 && !_VBS2

#include <Es/Strings/rString.hpp>
#include <El/Evaluator/expressImpl.hpp>
#include "El/ParamFile/paramFile.hpp"
#include "../objectId.hpp"
#include "../object.hpp"

DEFINE_ENUM_BEG(EditorObjectScope)
	EOSHidden,      // object is invisible (not shown in tree or editor)
  EOSView,        // object is visible but may not be selected
  EOSSelect,      // object is visible and selectable
  EOSLinkTo,      // object may be linked to (but not from)
  EOSLinkFrom,    // object may be linked to and from
  EOSDrag,        // object may be edited and copied but not dragged
  EOSAllNoTree,   // object may be edited and copied but it is not visible in the object tree
  EOSAllNoSelect, // object may be edited and copied, it is visible in the object tree but not selectable in 2D or 3D
  EOSAllNoCopy,   // object may be fully edited but not copied
  EOSAll,         // object may be edited and copied
  NEditorObjectScopes
DEFINE_ENUM_END(EditorObjectScope)

DEFINE_ENUM_BEG(EditorTypePermission)
	ETSNone,
  ETSLink,
  ETSFull,
  NEditorTypePermissions
DEFINE_ENUM_END(EditorTypePermission)

struct EditorArgument
{
  RString name;
  RString value;
  GameValue evaluated;
};
TypeIsMovableZeroed(EditorArgument)

// TODO: use hash map instead
class EditorArguments : public AutoArray<EditorArgument>
{
private:
  typedef AutoArray<EditorArgument> base;

public:
  int Set(RString name, RString value, bool evaluate);
  RString Get(RString name) const
  {
    for (int i=0; i<Size(); i++)
      if (base::Get(i).name == name) return base::Get(i).value;
    return RString();
  }
  bool Get(EditorArgument &arg) const
  {
    for (int i=0; i<Size(); i++)
      if (base::Get(i).name == arg.name && base::Get(i).value == arg.value) return true;
    return false;
  }
  GameValue GetValue(RString name) const
  {
    for (int i=0; i<Size(); i++)
      if (base::Get(i).name == name) return base::Get(i).evaluated;
    return GameValue();
  }
};

enum EditorParamSource
{
  EPSDialog,
  EPSPosition,
  EPSLink,
  EPSParent,
  EPSId,
  EPSDirection,
  EPSCounter,
  EPSContext
};

/// description of editor parameter
struct EditorParam
{
  EditorParamSource source;
  RString name;
  /// user friendly name, used in dialogs, menues etc.
  RString displayName;
  RString type;
  RString subtype;
  RString description;
  RString activeMode;
  bool canChange;
  bool hasDefValue;
  RString defValue;
  /// new / edit dialog handlers
  RString onInit;
  RString onChanged;
  RString valid;
  bool hidden;
  int shortcutKey;
  bool passAsEditorObject;
  // redefined control settings
  float x,y,w,h;
  int idc;

  EditorParam(
    EditorParamSource src, 
    RString n, 
    RString t, 
    RString subt, 
    RString desc, 
    RString actMode, 
    bool canCh, 
    bool hasDefVal, 
    RString defVal, 
    bool hide, 
    float key, 
    bool eo,
    float xPos,
    float yPos,
    float width,
    float hgt,
    int i
  )
  {
    source = src;
    name = n;
    type = t;
    subtype = subt;
    description = desc;
    activeMode = actMode;
    canChange = canCh;
    hasDefValue = hasDefVal;
    defValue = defVal;
    hidden = hide;
    shortcutKey = key;
    passAsEditorObject = eo;
    x = xPos; y = yPos; w = width; h = hgt;
    idc = i;
  }
};
TypeIsMovableZeroed(EditorParam)

// TODO: use hash map instead
class EditorParams : public AutoArray<EditorParam>
{
public:
  void Load(ParamEntryPar cfg, RString name);
  const EditorParam *Find(RString name) const
  {
    for (int i=0; i<Size(); i++)
      if (Get(i).name == name) return &Get(i);
    return NULL;
  }
};

/// type of editor object (vehicle, center, group, unit etc.)
class EditorObjectType : public RefCount
{
protected:
  /// internal name
  RString _name;
  /// user friendly name of the type
  RString _title;
  EditorParams _params;

  AutoArray<RString> _create;
  AutoArray<RString> _update;
  AutoArray<RString> _updatePosition;
  AutoArray<RString> _delete;
  AutoArray<RString> _select;
  RString _position;
  RString _proxy;
  /// template to compose object (instance) name
  RString _displayName;
  RString _displayNameTree;
  // object visible on map?
  RString _visible;
  // execute draw map script for object?
  RString _execDrawMap;
  AutoArray<RString> _drawMap;
  int _shortcutKey;
  RString _shortcutKeyDesc;
  int _nextID;
  bool _visibleIfTreeCollapsed;
  bool _contextMenuFollowsProxy;

  // used with real time editor
  EditorObjectScope _scope;

public:
  EditorObjectType() {}
  EditorObjectType(RString name, RString filename);
  EditorObjectType(RString name, ParamEntryVal entry);
  EditorObjectType(RString name, const EditorObjectType *type);

  RString GetName() const {return _name;}
  RString GetTitle() const {return _title;}
  const EditorParams &GetParams() const {return _params;}

  // fill object type variables from editor definition file
  void LoadParams(ParamEntryVal &file);

  const AutoArray<RString> &GetCreateScript() const {return _create;}
  const AutoArray<RString> &GetUpdateScript() const {return _update;}
  const AutoArray<RString> &GetUpdatePositionScript() const {return _updatePosition;}
  const AutoArray<RString> &GetSelectScript() const {return _select;}
  const AutoArray<RString> &GetDeleteScript() const {return _delete;}
  void SetCreateScript(AutoArray<RString> create) {_create = create;}
  void SetUpdateScript(AutoArray<RString> update) {_update = update;}
  RString GetPosition() const {return _position;}
  RString GetProxy() const {return _proxy;}
  RString GetDisplayName() const {return _displayName;}
  RString GetDisplayNameTree() const {return _displayNameTree;}
  RString GetVisible() const {return _visible;}  
  RString GetExecDrawMap() const {return _execDrawMap;}  
  RString GetShortcutKeyDesc() const {return _shortcutKeyDesc;}
  int GetShortcutKey() const {return _shortcutKey;}
  const AutoArray<RString> &GetDrawMapScript() const {return _drawMap;}
  
  bool IsVisibleIfTreeCollapsed() const {return _visibleIfTreeCollapsed;}
  bool ContextMenuFollowsProxy() const {return _contextMenuFollowsProxy;}
  
  EditorObjectScope GetScope() const {return _scope;}

  RString NextVarName() {return Format("%s_%d", (const char *)_name, _nextID++);}
  // update _nextID by this object name
  void RegisterObjectName(RString name);
};

struct EditorObjectIcon
{
  Ref<Texture> icon;
  PackedColor color;
  Vector3 offset;
  float width;
  float height;
  float angle;
  bool maintainSize;
  float minScale;   // minimum scale to maintain size (when map is below this scale, icon size will be maintained)
  RString id;
  bool shadow;
  bool line;
  bool is3D;
  float priority;
  RString text;
  EditorObjectIcon() 
  {
    icon = NULL;
    color = PackedWhite;
    offset = VZero;
    width = 1;
    height = 1;
    angle = 0;
    maintainSize = true;
    minScale = -1;
    shadow = true;
    line = true;
    is3D = false;
    priority = 0.0;
    text = RString();
  }
};
TypeIsMovableZeroed(EditorObjectIcon)

class EditorObject;

// editor object overlay
class EditorOverlay : public RefCount
{
private:
  RString _name;

public:
  void Save(const RefArray<EditorObject> &objects) const;
  EditorOverlay(RString name) {_name = name;}
  RString GetName() const {return _name;}
};

class EditorObject : public RemoveLLinks
{
protected:
  Ref<EditorObjectType> _type;

  // custom editor object type for this object (optional)
  Ref<EditorObjectType> _subType;

  // list of all parameters (main type and subtype)
  EditorParams _allParams;

  EditorArguments _args;
  
  bool _isVisibleInTree;  // is the editor object visible in the object tree?
  bool _isVisibleOnMap;   // is the editor object visible on the map?

  bool _execDrawMap;      // execute drawMap script for this object?

  AutoArray<EditorObjectIcon> _icons;
  bool _draw3DLine;

  // pointer to proxy object
  Ref<Object> _proxy;

#if _VBS2 // _staticProxy
  // is the proxy object static (ie not a unit or vehicle)?
  bool _staticProxy;
#endif

  // drawMap script cache
  mutable RString _drawMapScript;
#if USE_PRECOMPILATION
  mutable CompiledExpression _drawMapScriptCompiled;
#endif
  mutable bool _drawMapScriptChanged;

  // object selected?
  bool _selected;

  // overlay that this object belongs to (if any)
  const EditorOverlay *_parentOverlay;
  bool _isInCurrentOverlay;

  // used with real time editor
  EditorObjectScope _scope;

  // is the object visible even if tree is collapsed?
  bool _visibleIfTreeCollapsed;

  // evaluated position of the object (for drawing)
  Vector3 _evaluatedPos;

public:
  EditorObject();
  EditorObject(EditorObjectType *type);

  // return parent overlay
  void SetParentOverlay(const EditorOverlay *overlay, bool isCurrent = false) {_parentOverlay = overlay; _isInCurrentOverlay = isCurrent;}
  const EditorOverlay *GetParentOverlay() {return _parentOverlay;}
  bool IsObjInCurrentOverlay() const {return _isInCurrentOverlay;}

  // 2D and 3D icons
  void AddIcon(const EditorObjectIcon &icon);
  EditorObjectIcon &GetIcon(int i) {return _icons[i];}
  void RemoveIcon(RString id);
  int NIcons() {return _icons.Size();}
  bool Draw3DLine() {return _draw3DLine;}

  bool IsSelected() {return _selected;}
  void SetSelected(bool selected) {_selected = selected;}

  // object position, pre-evaluated
  void UpdateEvaluatedPos();
  // returns proxy position (if available), otherwise evaluated position
  Vector3 GetEvaluatedPos() const {return _evaluatedPos;}

  // object visibility
  bool IsVisibleInTree() const {return _isVisibleInTree;} // object visible in tree?
  void SetVisibleInTree(bool isVisible) {_isVisibleInTree = isVisible;}

  RString GetVisible() const; // object's visibility script (from editor object definition)
  void UpdateVisibleOnMap(GameState *gstate);

  // is the object visible even if tree is collapsed?
  bool IsVisibleIfTreeCollapsed() {return _visibleIfTreeCollapsed;}
  void SetVisibleIfTreeCollapsed(bool v) {_visibleIfTreeCollapsed = v;}

  EditorObjectScope GetScope() const {return _scope;} // editor object scope (may relate to visibility)
  void SetScope(EditorObjectScope scope) {_scope = scope;}

  // is the object visible?
  bool IsVisible() const {return (_isVisibleInTree || _visibleIfTreeCollapsed) && _scope > EOSHidden && _isVisibleOnMap;}
  bool IsVisibleIn3D() const {return _scope > EOSHidden && _isVisibleOnMap;}

  // editor object type related
  const EditorObjectType *GetType() const {return _type;}
  EditorObjectType *GetType() {return _type;}

  // subtype related
  const EditorParams &GetAllParams() const {return _allParams;}  
  void ListAllParameters();
  void ClearSubType();
  EditorObjectType *GetSubType() {return _subType;}
  void AssignSubType(const EditorObjectType *subType, RString name);

  // edit object arguments
  int NArguments() const {return _args.Size();}
  const EditorArgument &GetArgument(int i) const {return _args[i];}  
  void RemoveArgument(int i) {_drawMapScriptChanged = true; _args.Delete(i);}
  void SetArguments(const AutoArray<RString> &args);
  int SetArgument(RString name, RString value);
  RString GetArgument(RString name) const {return _args.Get(name);}
  bool HasArgument(EditorArgument &arg) {return _args.Get(arg);}
  GameValue GetArgumentValue(RString name) const {return _args.GetValue(name);}
  void RemoveArgument(RString name);
  // update an arguments value
  bool UpdateArgument(RString name, RString value, bool evaluate, bool updateMapScript = true);
  RString GetParent() const;

  // returns objects execDrawMap script from object type definition
  RString GetExecDrawMap() const;
  void UpdateExecDrawMap(GameState *gstate);
  bool ExecDrawMap() {return _execDrawMap;}

  // parse a single line of code
  RString WriteScript(RString code);

  void WriteCreateScript(QOStream &out);
  void WriteUpdateScript(QOStream &out);
  void WriteUpdatePositionScript(QOStream &out);
  void WriteSelectScript(QOStream &out);
  void WriteDeleteScript(QOStream &out);

  // get position script from editor definition file
  RString GetPosition() const;

  // execute the position script (from editor definition)
  bool GetPosition(GameState *gstate, Vector3 &pos) const;

  // get the actual value of the position attribute
  bool GetEvaluatedPosition(Vector3 &pos) const;

  RString GetProxy() const;
  RString GetDisplayName() const;
  RString GetDisplayNameTree() const;

  bool IsDrawMapScript(GameState *state) const;
  void ExecuteDrawMapScript(GameState *state) const;

  void Save(ParamEntry &f) const;
  void Load(EditorObjectType *type, ParamEntryPar f);

  // set proxy object
  void SetProxyObject(Ref<Object> proxy);
  Ref<Object> GetProxyObject() const {return _proxy;}

#if _VBS2 // _staticProxy
  bool IsStaticProxy() {return _staticProxy;}
#endif

  // update position and direction arguments based upon proxy
  void UpdateAzimuth();

protected:
  void UpdateDrawMapScript(GameState *state) const;
};

class EditorWorkspace
{
protected:
  RefArray<EditorObjectType> _types;
  RefArray<EditorObject> _objects;
  RefArray<EditorObject> _selected;

  // Fixed objects
  Ref<EditorObjectType> _typePrefix;
  Ref<EditorObjectType> _typePostfix;
  Ref<EditorObject> _objectPrefix;
  Ref<EditorObject> _objectPostfix;

  // loaded editor object overlays
  RefArray<EditorOverlay> _overlays;
  EditorOverlay *_activeOverlay;

public:
  EditorWorkspace() {Init();}

  // overlays allow a user to create template missions that 'overlay' the main
  // mission (contains 'fake' editor objects)
  EditorOverlay *CreateOverlay(RString name);
  void SetActiveOverlay(EditorOverlay *activeOverlay);
  EditorOverlay *GetActiveOverlay() {return _activeOverlay;} 

  void SaveOverlay(const EditorOverlay *overlay);
  EditorOverlay *LoadOverlay(RString filename);

  EditorObject *GetPrefixObject() {return _objectPrefix;}
  EditorObject *GetPostfixObject() {return _objectPostfix;}
  int NObjects() const {return _objects.Size();}
  const EditorObject *GetObject(int i) const {return _objects[i];}
  EditorObject *GetObject(int i) {return _objects[i];}

  // add an object that has already been created
  void AddObject(EditorObject *obj);  

  // create and add a new object of a certain type
  EditorObject *AddObject(EditorObjectType *type);

  EditorObject *CopyObject(EditorObject *newObj);
  void CopyObjects(RefArray<EditorObject> &clipboard);
  void RenameObjects(RefArray<EditorObject> &objects);

  void DeleteObject(EditorObject *obj) {_objects.Delete(obj);}

  // select an object and subordinates - by default only select hidden suborindates
  void SetSelected(
    EditorObject *selectObj, 
    bool selected, 
    bool subordinatesAlso = true, 
    bool onlyHiddenSubordinates = true,
    bool isMultiple = false
  );
  void ClearSelected();
  RefArray<EditorObject> &GetSelected() {return _selected;}

  int NObjectTypes() const {return _types.Size();}
  const EditorObjectType *GetObjectType(int i) const {return _types[i];}
  EditorObjectType *FindObjectType(RString name);
  // EditorObject *FindObject(EditorObjectType *type, ...);
  EditorObject *FindObject(RString id);

  // order objects by link dependencies  
  void OrderObjects(RefArray<EditorObject> &edObjects);
  void OrderAllObjects() {ClearSelected(); OrderObjects(_objects);}

  // move an object to the end of the objects list
  void MoveObjectToEnd(EditorObject *obj);

  void WriteCreateScript(RString filename) const;
  void Clear();
  void Save(RString filename) const;
  void Load(RString filename);

protected:
  void Init();
};

#endif // _ENABLE_EDITOR2

#endif

