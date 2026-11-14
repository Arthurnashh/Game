#include "../wpch.hpp"

#if _PROFILE && (USER_Ondra || USER_ondra)
#pragma optimize("",off)
#endif

#if _ENABLE_EDITOR2 && !_VBS2

#include "missionEditor.hpp"
#include "Es/Strings/bstring.hpp"
#include "Es/Files/fileContainer.hpp"
#include <Es/Algorithms/qsort.hpp>
#include "../paramFileExt.hpp"
#include "../landScape.hpp"
#include <El/Evaluator/express.hpp>

#if _VBS2 // _staticProxy
#include "../AI/ai.hpp" 
#endif

// GWorld
#include "../world.hpp"

RString GetUserDirectory();

int EditorArguments::Set(RString name, RString value, bool evaluate)
{
  int index = Add();
  base::Set(index).name = name;
  base::Set(index).value = value;
  if (evaluate)
  {
    GameVarSpace local(false);
    int gshandle = GGameState.BeginContext(&local);
    base::Set(index).evaluated = GGameState.Evaluate(value, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
    GGameState.EndContext(gshandle);
  }
  return index;
};

// read list of parameters from object type definition file
void EditorParams::Load(ParamEntryPar cfg, RString name)
{
  EditorParam defaults[] =
  {
    EditorParam(EPSId, "VARIABLE_NAME", name, "", "", "", false, false, "", false, -1, false, -1.0f, -1.0f, -1.0f, -1.0f, -1)
  };
  int nDefaults = sizeof(defaults) / sizeof(EditorParam);

  int n = cfg.GetEntryCount();
  Realloc(n + nDefaults);
  Resize(n + nDefaults);
  for (int i=0; i<n; i++)
  {
    ParamEntryVal entry = cfg.GetEntry(i);
    EditorParam &param = Set(i);

    ConstParamEntryPtr check = entry.FindEntry("source");
    if (check)
    {
      RString source = *check;
      if (stricmp(source, "link") == 0) param.source = EPSLink;
      else if (stricmp(source, "parent") == 0) param.source = EPSParent;
      else if (stricmp(source, "position") == 0) param.source = EPSPosition;
      else if (stricmp(source, "direction") == 0) param.source = EPSDirection;
      else if (stricmp(source, "counter") == 0) param.source = EPSCounter;
      else if (stricmp(source, "context") == 0) param.source = EPSContext;      
      else
      {
        DoAssert(stricmp(source, "dialog") == 0);
        param.source = EPSDialog;
      }
    }
    else param.source = EPSDialog;

    param.name = entry.GetName();
    param.type = entry >> "type";

    check = entry.FindEntry("subtype");
    if (check) param.subtype = *check;

    check = entry.FindEntry("displayName");
    if (check) param.displayName = *check;
    else param.displayName = param.name;

    check = entry.FindEntry("description");
    if (check) param.description = *check;

    check = entry.FindEntry("activeMode");
    if (check) param.activeMode = *check;

    check = entry.FindEntry("canChange");
    if (check) param.canChange = *check;
    else param.canChange = true;

    check = entry.FindEntry("hidden");
    if (check) param.hidden = *check;
    else param.hidden = false;

    check = entry.FindEntry("passAsEditorObject");
    if (check) param.passAsEditorObject = *check;
    else param.passAsEditorObject = false;

    check = entry.FindEntry("idc");
    if (check) param.idc = *check;
    else param.idc = -1;

    check = entry.FindEntry("shortcutKey");
    if (check) param.shortcutKey = *check;
    else param.shortcutKey = -1;

    check = entry.FindEntry("x");
    if (check) param.x = *check; else param.x = -1.0f;
    check = entry.FindEntry("y");
    if (check) param.y = *check; else param.y = -1.0f;
    check = entry.FindEntry("w");
    if (check) param.w = *check; else param.w = -1.0f;
    check = entry.FindEntry("h");
    if (check) param.h = *check; else param.h = -1.0f;

    check = entry.FindEntry("default");
    if (check)
    {
      param.hasDefValue = true;
      param.defValue = *check;
    }
    else param.hasDefValue = false;

    check = entry.FindEntry("onInit");
    if (check) param.onInit = *check;

    check = entry.FindEntry("onChanged");
    if (check) param.onChanged = *check;

    check = entry.FindEntry("valid");
    if (check) param.valid = *check;
  }

  // default parameters
  for (int i=0; i<nDefaults; i++)
    Set(n + i) = defaults[i];
}

// create object type and read definition file
EditorObjectType::EditorObjectType(RString name, RString filename)
{
  _name = name;
  _nextID = 0;
  _visibleIfTreeCollapsed = false;
  _contextMenuFollowsProxy = false;
  _scope = EOSAll;

  GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

  ParamFile file;
  file.Parse(filename, NULL, NULL, &globals);

  LoadParams((ParamEntryVal)file);
}

// create object type and read definition from config entry
EditorObjectType::EditorObjectType(RString name, ParamEntryVal entry)
{
  _name = name;
  _nextID = 0;
  _visibleIfTreeCollapsed = false;
  _contextMenuFollowsProxy = false;
  _scope = EOSAll;

  LoadParams(entry);
}

// create object type and read definitions from another type
EditorObjectType::EditorObjectType(RString name, const EditorObjectType *type)
{
  _name = name;
  _nextID = 0;
  _params = type->GetParams();
  _create = type->GetCreateScript();
  _update = type->GetUpdateScript();
  _updatePosition = type->GetUpdatePositionScript();
  _delete = type->GetDeleteScript();
  _position = type->GetPosition();
  _proxy = type->GetProxy();
  _shortcutKey = type->GetShortcutKey();
  _shortcutKeyDesc = type->GetShortcutKeyDesc();
  _displayName = type->GetDisplayName();
  _displayNameTree = type->GetDisplayNameTree();
  _visible = type->GetVisible();
  _execDrawMap = type->GetExecDrawMap();
  _visibleIfTreeCollapsed = type->IsVisibleIfTreeCollapsed();
  _scope = type->GetScope();
  _drawMap = type->GetDrawMapScript();
}

void EditorObjectType::LoadParams(ParamEntryVal &file)
{
  // parameters
  if (file.CheckIfEntryExists("Params"))
    _params.Load(file >> "Params", _name);

  int n;

  // create script
  if (file.CheckIfEntryExists("create"))
  {
    ParamEntryVal array = file >> "create";
    n = array.GetSize();
    _create.Realloc(n);
    _create.Resize(n);
    for (int i=0; i<n; i++) _create[i] = array[i];
  }

  // update script
  if (file.CheckIfEntryExists("update"))
  {
    ParamEntryVal array = file >> "update";
    n = array.GetSize();
    _update.Realloc(n);
    _update.Resize(n);
    for (int i=0; i<n; i++) _update[i] = array[i];
  }

  // update position script
  if (file.CheckIfEntryExists("updatePosition"))
  {
    ParamEntryVal array = file >> "updatePosition";
    n = array.GetSize();
    _updatePosition.Realloc(n);
    _updatePosition.Resize(n);
    for (int i=0; i<n; i++) _updatePosition[i] = array[i];
  }

  // delete
  if (file.CheckIfEntryExists("destroy"))
  {
    ParamEntryVal array = file >> "destroy";
    n = array.GetSize();
    _delete.Realloc(n);
    _delete.Resize(n);
    for (int i=0; i<n; i++) _delete[i] = array[i];
  }

  // draw map script
  ParamEntryPtr entry = file.FindEntry("drawMap");
  if (entry)
  {
    int n = entry->GetSize();
    _drawMap.Realloc(n);
    _drawMap.Resize(n);
    for (int i=0; i<n; i++) _drawMap[i] = (*entry)[i];
  }

  entry = file.FindEntry("title");
  if (entry) _title = *entry;
  else _title = _name;

  entry = file.FindEntry("position");
  if (entry) _position = *entry;

  entry = file.FindEntry("displayName");
  if (entry) _displayName = *entry;

  entry = file.FindEntry("displayNameTree");
  if (entry) _displayNameTree = *entry;

  entry = file.FindEntry("visible");
  if (entry) _visible = *entry;

  entry = file.FindEntry("execDrawMap");
  if (entry) _execDrawMap = *entry;

  entry = file.FindEntry("visibleIfTreeCollapsed");
  if (entry) _visibleIfTreeCollapsed = *entry;

  entry = file.FindEntry("contextMenuFollowsProxy");
  if (entry) _contextMenuFollowsProxy = *entry;  

  entry = file.FindEntry("scope");
  if (entry && entry->IsIntValue())
  {
    int val = *entry;
    if (val < NEditorObjectScopes)
      _scope = (EditorObjectScope)val;
  }

  entry = file.FindEntry("proxy");
  if (entry) _proxy = *entry;

  // shortcut key
  _shortcutKey = -1;
  entry = file.FindEntry("shortcutKey");
  if (entry)
  {
    int n = entry->GetSize();
    if (n > 0 && (*entry)[0].IsIntValue())
      _shortcutKey = (*entry)[0];
    if (n > 1 && (*entry)[1].IsTextValue())
      _shortcutKeyDesc = (*entry)[1];
  }
}

void EditorObjectType::RegisterObjectName(RString name)
{
  const char *ext = strrchr(name, '_');
  if (!ext)
  {
    RptF("Invalid object name: %s", (const char *)name);
    return;
  }
  int id = atoi(ext + 1);
  saturateMax(_nextID, id + 1);
}

static inline bool CheckEvaluate(EditorParamSource source)
{
  return source == EPSPosition;
}

static inline bool CheckEvaluate(const EditorParam *param)
{
  return (param->source == EPSPosition || stricmp(param->type, "position") == 0);
}

EditorObject::EditorObject() 
{
  _selected = false; 
  _drawMapScriptChanged = true; 
  _isVisibleInTree = true; 
  _isVisibleOnMap = true; 
  _execDrawMap = true;
  _visibleIfTreeCollapsed = false;
  _parentOverlay = NULL;
  _isInCurrentOverlay = true;
  _scope = EOSAll;
#if _VBS2 // _staticProxy
  _staticProxy = false;
#endif
  _draw3DLine = false;
  _evaluatedPos = VZero;
  UpdateEvaluatedPos();
}

EditorObject::EditorObject(EditorObjectType *type)
: _type(type)
{
  Assert(type);
  _args.Set("VARIABLE_NAME", type->NextVarName(), CheckEvaluate(EPSId));
  _drawMapScriptChanged = true;
  _isVisibleInTree = true; 
  _isVisibleOnMap = true; 
  _execDrawMap = true;
  _visibleIfTreeCollapsed = type->IsVisibleIfTreeCollapsed();
  _selected = false;
  ListAllParameters();
  _parentOverlay = NULL;
  _isInCurrentOverlay = true;
  _scope = type->GetScope();
#if _VBS2 // _staticProxy
  _staticProxy = false;
#endif
  _draw3DLine = false;
  _evaluatedPos = VZero;
  UpdateEvaluatedPos();
}

void EditorObject::ClearSubType() 
{
  if (_subType)
  {
    // remove redundant arguments
    const EditorParams &params = _subType->GetParams();
    for (int i=0; i<params.Size(); i++)
    {
      const EditorParam &param = params[i];
      if (param.source != EPSId)
        RemoveArgument(param.name);
    }
  }

  delete _subType; 
  _subType = NULL; 
  ListAllParameters();
}

void EditorObject::ListAllParameters()
{
  _allParams.Clear();

  // compile a list of pointers to all parameters
  const EditorParams &params = _type->GetParams();
  for (int i=0; i<params.Size(); i++)
    _allParams.Add(params[i]);

  if (_subType)
  {
    const EditorParams &subParams = _subType->GetParams();
    for (int i=0; i<subParams.Size(); i++)
    {
      if (!_allParams.Find(subParams[i].name))
        _allParams.Add(subParams[i]);      
    }
  }
}

void EditorObject::AssignSubType(const EditorObjectType *subType,RString name) 
{
  ClearSubType(); 
  _subType = new EditorObjectType(name,subType);
  ListAllParameters();
}

void EditorObject::SetArguments(const AutoArray<RString> &args)
{
  const EditorParams &params = GetAllParams();
    
  // iterate arguments passed in
  for (int i=0; i<args.Size(); i+=2)
  {
    // confirm param exists
    const EditorParam *param = params.Find(args[i]);
    if (param)
    {
      RString value = args[i+1];
      if (stricmp(param->subtype, "multiple") != 0)
        RemoveArgument(args[i]);
      SetArgument(args[i],value);
    }
  }
}

int EditorObject::SetArgument(RString name, RString value)
{
  _drawMapScriptChanged = true;
  const EditorParam *param = GetAllParams().Find(name);
  bool evaluate = param ? CheckEvaluate(param) : false;
  return _args.Set(name, value, evaluate);
}

void EditorObject::RemoveArgument(RString name)
{
  _drawMapScriptChanged = true;
  for (int i=0; i<_args.Size();)
  {
    if (stricmp(_args[i].name, name) == 0)
      _args.Delete(i);
    else
      i++;
  }
}

bool EditorObject::UpdateArgument(RString name, RString value, bool evaluate, bool updateMapScript)
{
  _drawMapScriptChanged = updateMapScript;
  for (int i=0; i<_args.Size(); i++)
  {
    if (stricmp(_args[i].name, name) == 0)
    {
      _args[i].value = value;
      if (evaluate)
      {
        GameVarSpace local(false);
        int gshandle = GGameState.BeginContext(&local);
        _args[i].evaluated = GGameState.Evaluate(value, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
        GGameState.EndContext(gshandle);
      }
      return true;
    }
  }  
  return false;
}

RString EditorObject::GetParent() const
{
  Assert(_type);
  const EditorParams &params = _type->GetParams();
  for (int i=0; i<params.Size(); i++)
  {
    if (params[i].source == EPSParent) return GetArgument(params[i].name);
  }
  return RString();
}

// write string & end of line to stream
inline void WriteLine(QOStream &out, RString line)
{
  out.write(line, strlen(line));
  out.write("\r\n", 2);
}

// replace " by "" in string
static RString NormalizeString(const char *src)
{
  static const int len = 1024;
  char dst[len];
  int i=0;
  for (const char *ptr=src; *ptr; ptr++)
  {
    if (i >= len - 1)
    {
      Fail("Buffer overflow");
      break;
    }
    dst[i++] = *ptr;
    if (*ptr == '"')
    {
      if (i >= len - 1)
      {
        Fail("Buffer overflow");
        break;
      }
      dst[i++] = *ptr;
    }
  }
  dst[i] = 0;
  return dst;
}

// find argument(s) in argument list, extract its value and check if must be saved to script
static RString ExtractValue(RString name, const EditorParams &params, const EditorArguments &args, bool &required, bool varNames = false)
{
  const EditorParam *param = params.Find(name);
  if (!param)
  {
    ErrF("Undeclared parameter: %s", (const char *)name);
    return "ERROR";
  }  
  // check for to see if editor object (string) should be passed instead of object
  if (param->passAsEditorObject) varNames = true;
  switch (param->source)
  {
  case EPSLink:
  case EPSParent:
    if (stricmp(param->subtype, "single") == 0)
    {
      // no default value
      required = true;
      if (varNames)
        return RString("\"") + args.Get(name) + RString("\"");
      else
        return args.Get(name);
    }
    else if (stricmp(param->subtype, "noneOrSingle") == 0)
    {
      // default NULL
      RString value = args.Get(name);
      if (value.GetLength() > 0) required = true;
      if (varNames)
        return RString("\"") + value + RString("\"");
      else
        return value;
    }
    else
    {
      RString value = "[";
      bool first = true;
      for (int i=0; i<args.Size(); i++)
      {
        if (args[i].name != name) continue;
        if (first) first = false;
        else value = value + ", ";
        if (varNames)
          value = value + RString("\"") + args[i].value + RString("\"");
        else
          value = value + args[i].value;
      }
      if (!first) required = true;
      return value + "]";
    }
  default:
    if (stricmp(param->subtype, "multiple") == 0)
    {
      // type
      int type = 0;
      if (
        stricmp(param->type, "text") == 0 || 
        stricmp(param->type, "code") == 0 ||
        stricmp(param->type, "expression") == 0 ||
        stricmp(param->type, "evalBool") == 0
      )
        type = 1;
      else if (stricmp(param->type, "enum") == 0)
        type = 2;
      else if (stricmp(param->type, "config") == 0)
        type = 3;
      else if (stricmp(param->type, "configEx") == 0)
        type = 3;
      else if (stricmp(param->type, "special") == 0)
        type = 3;
      else if (stricmp(param->type, "picture") == 0)
        type = 3;

      RString value = "[";
      bool first = true;
      for (int i=0; i<args.Size(); i++)
      {
        if (args[i].name != name) continue;
        if (first) first = false;
        else value = value + ", ";
        switch (type)
        {
        case 0:
          value = value + args[i].value;
          break;
        case 1:
          value = value + RString("\"") + NormalizeString(args[i].value) + RString("\"");
          break;
        case 2:
          value = value + RString("\"") + args[i].value + RString("\"");
          break;
        case 3:
          value = value + RString("\"") + args[i].value + RString("\"");
          break;
        }
      }
      if (!first) required = true;
      return value + "]";
    }
    else
    {
      RString value = args.Get(name);

      if (!param->hasDefValue || value != param->defValue) required = true;

      if (stricmp(param->type, "code") == 0)
        return value;
      if (
        stricmp(param->type, "text") == 0 || 
        stricmp(param->type, "code") == 0 ||
        stricmp(param->type, "expression") == 0 ||
        stricmp(param->type, "evalBool") == 0
      )
        return RString("\"") + NormalizeString(value) + RString("\"");
      else if (stricmp(param->type, "enum") == 0)
        return RString("\"") + value + RString("\"");
      else if (stricmp(param->type, "config") == 0)
        return RString("\"") + value + RString("\"");
      else if (stricmp(param->type, "configEx") == 0)
        return RString("\"") + value + RString("\"");
      else if (stricmp(param->type, "special") == 0)
        return RString("\"") + value + RString("\"");
      else if (stricmp(param->type, "picture") == 0)
        return RString("\"") + value + RString("\"");
      else if (stricmp(param->type, "side") == 0)
      {
        TargetSide side = GetEnumValue<TargetSide>(cc_cast(value));
        // return the unary operator defining the correct side
        switch (side)
        {
        case TEast:
          return "east";
        case TWest:
          return "west";
        case TGuerrila:
          return "resistance";
        case TCivilian:
          return "civilian";
        case TLogic:
          return "sideLogic";
        }
      }
      return value;
    }
  }
}

// script definition argument "%!" - required line
typedef void (*OnRequiredFunction)(void *context);
// script definition argument "%NAME"
typedef RString (*OnValueFunction)(RString name, void *context);
// script definition argument "%(EXPRESSION)"
typedef RString (*OnExpressionFunction)(RString name, void *context);

// parse single line from script definition and replace arguments using callback functions
static RString ParseLine
(
  RString format,
  OnRequiredFunction onRequired, OnValueFunction onValue, OnExpressionFunction onExpression,
  void *context
)
{
  static const int len = 1024;
  char dst[len];
  int i=0;
  for (const char *ptr=format; *ptr; ptr++)
  {
    if (*ptr == '%')
    {
      ptr++;
      if (!*ptr)
      {
        Fail("Syntax error");
        break;
      }
      else if (*ptr == '%')
      {
        dst[i++] = *ptr;
        if (i >= len - 1)
        {
          Fail("Buffer overflow");
          break;
        }
      }
      else if (*ptr == '!')
      {
        onRequired(context);
      }
      else if (*ptr == '(')
      {
        ptr++;
        const char *begin = ptr;
        while (*ptr && *ptr != ')') ptr++;
        RString name(begin, ptr - begin);
        if (!*ptr) ptr--;

        // expression
        RString value = onExpression(name, context);
        int n = value.GetLength();
        if (i + n >= len - 1)
        {
          Fail("Buffer overflow");
          break;
        }
        strcpy(dst + i, value); i += n;
      }
      else
      {
        const char *begin = ptr;
        while (*ptr && __iscsym(*ptr)) ptr++;
        RString name(begin, ptr - begin);
        ptr--;

        // value
        RString value = onValue(name, context);
        int n = value.GetLength();
        if (i + n >= len - 1)
        {
          Fail("Buffer overflow");
          break;
        }
        strcpy(dst + i, value); i += n;
      }
    }
    else
    {
      dst[i++] = *ptr;
      if (i >= len - 1)
      {
        Fail("Buffer overflow");
        break;
      }
    }
  }
  dst[i] = 0;
  return dst;
}

struct CreateContext
{
  const EditorParams *params;
  const EditorArguments *args;
  bool some;
  bool required;
};

void CreateOnRequired(void *context)
{
  CreateContext *ctx = (CreateContext *)context;
  ctx->required = true;
}

RString CreateOnValue(RString name, void *context)
{
  CreateContext *ctx = (CreateContext *)context;
  ctx->some = true;
  return ExtractValue(name, *ctx->params, *ctx->args, ctx->required);
}

RString CreateOnExpression(RString name, void *context)
{
  CreateContext *ctx = (CreateContext *)context;
  ctx->some = true;
  return ExtractValue(name, *ctx->params, *ctx->args, ctx->required);
}

RString EditorObject::WriteScript(RString code)
{
  Assert(_type);

  QOStrStream out;

  CreateContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;
  ctx.some = false;
  ctx.required = false;

  RString parsedCode = ParseLine(code, CreateOnRequired, CreateOnValue, CreateOnExpression, &ctx);
  if (ctx.required || !ctx.some) WriteLine(out, parsedCode);

  return RString(out.str(), out.pcount());
}

void EditorObject::WriteCreateScript(QOStream &out)
{
  Assert(_type);

  CreateContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  const AutoArray<RString> *format = &_type->GetCreateScript();
  if (_subType && _subType->GetCreateScript().Size() > 0) format = &_subType->GetCreateScript();

  for (int i=0; i<format->Size(); i++)
  {
    ctx.some = false;
    ctx.required = false;
    RString line = ParseLine(format->Get(i), CreateOnRequired, CreateOnValue, CreateOnExpression, &ctx);
    if (ctx.required || !ctx.some) WriteLine(out, line);
  }
}

void EditorObject::WriteUpdateScript(QOStream &out)
{
  Assert(_type);

  CreateContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  const AutoArray<RString> *format = &_type->GetUpdateScript();
  if (_subType && _subType->GetUpdateScript().Size() > 0) format = &_subType->GetUpdateScript();

  for (int i=0; i<format->Size(); i++)
  {
    ctx.some = false;
    ctx.required = false;
    RString line = ParseLine(format->Get(i), CreateOnRequired, CreateOnValue, CreateOnExpression, &ctx);
    if (ctx.required || !ctx.some) WriteLine(out, line);
  }
}

void EditorObject::WriteUpdatePositionScript(QOStream &out)
{
  Assert(_type);

  CreateContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  const AutoArray<RString> *format = &_type->GetUpdatePositionScript();
  if (_subType && _subType->GetUpdatePositionScript().Size() > 0) format = &_subType->GetUpdatePositionScript();

  for (int i=0; i<format->Size(); i++)
  {
    ctx.some = false;
    ctx.required = false;
    RString line = ParseLine(format->Get(i), CreateOnRequired, CreateOnValue, CreateOnExpression, &ctx);
    if (ctx.required || !ctx.some) WriteLine(out, line);
  }
}

void EditorObject::WriteSelectScript(QOStream &out)
{
  Assert(_type);

  CreateContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  const AutoArray<RString> *format = &_type->GetSelectScript();
  if (_subType && _subType->GetSelectScript().Size() > 0) format = &_subType->GetSelectScript();

  for (int i=0; i<format->Size(); i++)
  {
    ctx.some = false;
    ctx.required = false;
    RString line = ParseLine(format->Get(i), CreateOnRequired, CreateOnValue, CreateOnExpression, &ctx);
    if (ctx.required || !ctx.some) WriteLine(out, line);
  }
}

void EditorObject::WriteDeleteScript(QOStream &out)
{
  Assert(_type);

  CreateContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  const AutoArray<RString> *format = &_type->GetDeleteScript();
  if (_subType && _subType->GetDeleteScript().Size() > 0) format = &_subType->GetDeleteScript();

  for (int i=0; i<format->Size(); i++)
  {
    ctx.some = false;
    ctx.required = false;
    RString line = ParseLine(format->Get(i), CreateOnRequired, CreateOnValue, CreateOnExpression, &ctx);
    if (ctx.required || !ctx.some) WriteLine(out, line);
  }
}

struct GeneralContext
{
  const EditorParams *params;
  const EditorArguments *args;
};

void GeneralOnRequired(void *context)
{
}

RString GeneralOnValue(RString name, void *context)
{
  CreateContext *ctx = (CreateContext *)context;
  bool dummy;
  return ExtractValue(name, *ctx->params, *ctx->args, dummy);
}

RString GeneralOnExpression(RString name, void *context)
{
  CreateContext *ctx = (CreateContext *)context;
  bool dummy;
  return ExtractValue(name, *ctx->params, *ctx->args, dummy, true);
}

RString EditorObject::GetPosition() const
{
  GeneralContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  RString script = _type->GetPosition();
  if (_subType && _subType->GetPosition().GetLength() > 0) script = _subType->GetPosition();

  return ParseLine(script, GeneralOnRequired, GeneralOnValue, GeneralOnExpression, &ctx);
}

void EditorObject::UpdateEvaluatedPos()
{
  if (_proxy)
    _evaluatedPos = _proxy->WorldPosition(_proxy->FutureVisualState());
  else
  {
    Vector3 worldPos;
    if (GetEvaluatedPosition(worldPos))
      _evaluatedPos = worldPos;
  }
}

// returns evaluated position in engine coords (converts position arg to Vector3)
bool EditorObject::GetEvaluatedPosition(Vector3 &pos) const
{
    // update position
  GameValue result = GetArgumentValue("POSITION");
  if (result.GetNil() || result.GetType() != GameArray) return false;
  
  const GameArrayType &array = result;
  if (array.Size() < 2 || array[0].GetType() != GameScalar || array[1].GetType() != GameScalar) return false;
  
  pos[0] = array[0];
  pos[2] = array[1];
  if (array.Size() > 2) 
  {
    if (array.Size() > 3 || array[2].GetType() != GameScalar) return false;    
    pos[1] = (float)array[2] + GLandscape->SurfaceYAboveWater(pos[0],pos[2]);
  }
  else
    pos[1] = GLandscape->SurfaceYAboveWater(pos[0],pos[2]);

  return true;
}

// returns position in engine coords (executes position script for object)
bool EditorObject::GetPosition(GameState *gstate, Vector3 &pos) const
{
  RString statement = GetPosition();      
  GameValue result = gstate->EvaluateMultiple(statement, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace

  if (result.GetType() != GameArray) return false;
  const GameArrayType &array = result;
  if (array.Size() < 2 || array.Size() > 3) return false;
  if (array[0].GetType() != GameScalar) return false;
  if (array[1].GetType() != GameScalar) return false;

  pos[0] = array[0];
  pos[2] = array[1];
  if (array.Size() > 2) 
  {
    if (array.Size() > 3 || array[2].GetType() != GameScalar) return false;    
    pos[1] = (float)array[2] + GLandscape->SurfaceYAboveWater(pos[0],pos[2]);
  }
  else
    pos[1] = GLandscape->SurfaceYAboveWater(pos[0],pos[2]);
  
  return true;
}

RString EditorObject::GetExecDrawMap() const
{
  GeneralContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  RString script = _type->GetExecDrawMap();
  if (_subType && _subType->GetExecDrawMap().GetLength() > 0) script = _subType->GetExecDrawMap();

  return ParseLine(script, GeneralOnRequired, GeneralOnValue, GeneralOnExpression, &ctx);
}

void EditorObject::UpdateExecDrawMap(GameState *gstate)
{
  RString statement = GetExecDrawMap();
  if (statement.GetLength() == 0) 
    _execDrawMap = true;
  else
    _execDrawMap = gstate->EvaluateBool(statement, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
}

RString EditorObject::GetVisible() const
{
  GeneralContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  RString script = _type->GetVisible();
  if (_subType && _subType->GetVisible().GetLength() > 0) script = _subType->GetVisible();

  return ParseLine(script, GeneralOnRequired, GeneralOnValue, GeneralOnExpression, &ctx);
}

void EditorObject::UpdateVisibleOnMap(GameState *gstate)
{
  RString statement = GetVisible();
  if (statement.GetLength() == 0) 
    _isVisibleOnMap = true;
  else
    _isVisibleOnMap = gstate->EvaluateBool(statement, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
}

RString EditorObject::GetProxy() const
{
  GeneralContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  RString script = _type->GetProxy();
  if (_subType && _subType->GetProxy().GetLength() > 0) script = _subType->GetProxy();

  return ParseLine(script, GeneralOnRequired, GeneralOnValue, GeneralOnExpression, &ctx);
}

void EditorObject::SetProxyObject(Ref<Object> proxy) 
{
#if _VBS2 // _staticProxy
  Transport *trans=dyn_cast<Transport>(proxy.GetRef());
  Person *person=dyn_cast<Person>(proxy.GetRef());
  _staticProxy = !proxy || trans || person ? false : true;
#endif
  if (proxy.IsNull())
    _proxy.Free();
  else
    _proxy = proxy;
}

RString EditorObject::GetDisplayName() const
{
  GeneralContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;
  
  RString script = _type->GetDisplayName();
  if (_subType && _subType->GetDisplayName().GetLength() > 0) script = _subType->GetDisplayName();

  return ParseLine(script, GeneralOnRequired, GeneralOnValue, GeneralOnExpression, &ctx);
}

RString EditorObject::GetDisplayNameTree() const
{
  GeneralContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;
  
  RString script = _type->GetDisplayNameTree();
  if (_subType && _subType->GetDisplayNameTree().GetLength() > 0) script = _subType->GetDisplayNameTree();

  return ParseLine(script, GeneralOnRequired, GeneralOnValue, GeneralOnExpression, &ctx);
}

void EditorObject::UpdateDrawMapScript(GameState *state) const
{
  GeneralContext ctx;
  ctx.args = &_args;
  ctx.params = &_allParams;

  _drawMapScript = RString();

  const AutoArray<RString> *script = &_type->GetDrawMapScript();
  if (_subType && _subType->GetDrawMapScript().Size() > 0) 
    script = &_subType->GetDrawMapScript();

  for (int i=0; i<script->Size(); i++)
    _drawMapScript = _drawMapScript + ParseLine(script->Get(i), GeneralOnRequired, GeneralOnValue, GeneralOnExpression, &ctx);

#if USE_PRECOMPILATION
  SourceDoc doc; doc._content = _drawMapScript;
  SourceDocPos pos(doc);
  state->CompileMultiple(doc, pos, _drawMapScriptCompiled, false, GWorld->GetMissionNamespace()); // mission namespace
#endif
  _drawMapScriptChanged = false;
}

bool EditorObject::IsDrawMapScript(GameState *state) const
{
  if (_drawMapScriptChanged) UpdateDrawMapScript(state);
  return _drawMapScript.GetLength() > 0;
}

void EditorObject::ExecuteDrawMapScript(GameState *state) const
{
  if (_drawMapScriptChanged) UpdateDrawMapScript(state);
#if USE_PRECOMPILATION
  state->Execute(_drawMapScript, _drawMapScriptCompiled, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
#else
  state->Execute(_drawMapScript, GameState::EvalContext::_default, GWorld->GetMissionNamespace()); // mission namespace
#endif

  if (state->GetLastError() != EvalOK)
  {
    LogF("Error in the script:");
    LogF(_drawMapScript);
  }
}

static void SaveScript(const AutoArray<RString> *script, RString name, ParamClassPtr &args)
{
  if (script->Size() > 0) 
  {
    ParamEntryPtr array = args->AddArray(name);
    for (int i=0; i<script->Size(); i++)
      array->AddValue(script->Get(i));
  }
}

void EditorObject::Save(ParamEntry &f) const
{
  Assert(_type);

  const EditorParams *params = &_type->GetParams();

  RString varName = _args.Get("VARIABLE_NAME");
  ParamClassPtr cls = f.AddClass(varName);
  
  cls->Add("objectType", _type->GetName());
  ParamClassPtr args = cls->AddClass("Arguments");

  for (int j=0; j<2; j++)
  {
    if (j == 1) 
    {
      if (_subType)
        params = &_subType->GetParams();
      else
        break;
    }
    for (int i=0; i<params->Size(); i++)
    {
      const EditorParam &param = params->Get(i);
      if (param.source == EPSId) continue; // do not store VARIABLE_NAME once again

      RString name = param.name;

      AUTO_STATIC_ARRAY(int, indices, 16);
      for (int i=0; i<_args.Size(); i++)
      {
        if (stricmp(_args[i].name, name) == 0) indices.Add(i);
      }

      if (indices.Size() == 0) continue;
      else if (indices.Size() == 1)
      {
        RString value = _args[indices[0]].value;
        if (param.hasDefValue && value == param.defValue) continue; // do not store default values
        args->Add(name, value);
      }
      else
      {
        // multiple values
        ParamEntryPtr array = args->AddArray(name);
        for (int i=0; i<indices.Size(); i++)
        {
          RString value = _args[indices[i]].value;
          array->AddValue(value);
        }
      }
    }
  }
  
  if (_subType)
  {    
    args = cls->AddClass("DynamicArguments");      
    args->Add("name",_subType->GetName());
    ParamClassPtr paramsClass = args->AddClass("Params");
    for (int i=0; i<params->Size(); i++)
    {
      const EditorParam &param = params->Get(i);
      if (param.source == EPSId) continue; // do not store VARIABLE_NAME once again

      RString name = param.name;
      ParamClassPtr paramClass = paramsClass->AddClass(name);

      if (param.source == EPSLink) paramClass->Add("source", "link");
      else if (param.source == EPSParent) paramClass->Add("source", "parent");
      else if (param.source == EPSPosition) paramClass->Add("source", "position");
      else if (param.source == EPSDirection) paramClass->Add("source", "direction");
      else if (param.source == EPSCounter) paramClass->Add("source", "counter");

      paramClass->Add("type", param.type);      
      if (param.subtype.GetLength() > 0) paramClass->Add("subtype", param.subtype);
      if (param.description.GetLength() > 0) paramClass->Add("description", param.description);      
      if (param.activeMode.GetLength() > 0) paramClass->Add("activeMode", param.activeMode);      
      if (!param.canChange) paramClass->Add("canChange", param.canChange);
      if (param.defValue.GetLength() > 0) paramClass->Add("default", param.defValue);      
      if (param.hidden) paramClass->Add("hidden", param.hidden);
      if (param.shortcutKey > -1) paramClass->Add("shortcutKey", param.shortcutKey);
      if (param.passAsEditorObject) paramClass->Add("passAsEditorObject", param.passAsEditorObject);
      if (param.x > -1) paramClass->Add("x", param.x);
      if (param.y > -1) paramClass->Add("y", param.y);
      if (param.w > -1) paramClass->Add("w", param.w);
      if (param.h > -1) paramClass->Add("h", param.h);
      if (param.idc > -1) paramClass->Add("idc", param.idc);

      if (param.onInit.GetLength() > 0) paramClass->Add("onInit", param.onInit);
      if (param.onChanged.GetLength() > 0) paramClass->Add("onChanged", param.onChanged);
      if (param.valid.GetLength() > 0) paramClass->Add("valid", param.valid);

      // save scripts
      const AutoArray<RString> *script = &_subType->GetCreateScript();
      SaveScript(script,"create",args);

      script = &_subType->GetUpdateScript();
      SaveScript(script,"update",args);

      script = &_subType->GetUpdatePositionScript();
      SaveScript(script,"updatePosition",args);

      script = &_subType->GetSelectScript();
      SaveScript(script,"select",args);

      script = &_subType->GetDeleteScript();
      SaveScript(script,"destroy",args);

      script = &_subType->GetDrawMapScript();
      SaveScript(script,"drawMap",args);

      RString line = _subType->GetPosition();
      if (line.GetLength() > 0) args->Add("position", line);

      line = _subType->GetProxy();
      if (line.GetLength() > 0) args->Add("proxy", line);

      line = _subType->GetVisible();
      if (line.GetLength() > 0) args->Add("visible", line);

      line = _subType->GetExecDrawMap();
      if (line.GetLength() > 0) args->Add("execDrawMap", line);      

      line = _subType->GetDisplayName();
      if (line.GetLength() > 0) args->Add("displayName", line);

      line = _subType->GetDisplayNameTree();
      if (line.GetLength() > 0) args->Add("displayNameTree", line);
    }
  }
}

void EditorObject::Load(EditorObjectType *type, ParamEntryPar f)
{
  Assert(type);
  _type = type;
  _visibleIfTreeCollapsed = _type->IsVisibleIfTreeCollapsed();
  _scope = _type->GetScope();

  // load dynamic arguments (subtype)
  if (f.CheckIfEntryExists("DynamicArguments"))
  {
    ParamEntryVal args = f >> "DynamicArguments";
    RString name;
    if (args.CheckIfEntryExists("name"))
      name = args >> "name";
    _subType = new EditorObjectType(name,args);
  }

  const EditorParams *params = &type->GetParams();

  RString name = f.GetName();
  _args.Set("VARIABLE_NAME", name, CheckEvaluate(EPSId));
  type->RegisterObjectName(name);

  ParamEntryVal args = f >> "Arguments";
  for (int j=0; j<2; j++)
  {
    if (j == 1) 
    {
      if (_subType)
        params = &_subType->GetParams();
      else
        break;
    }
    for (int i=0; i<params->Size(); i++)
    {
      const EditorParam &param = params->Get(i);
      if (param.source == EPSId) continue;

      bool evaluate = CheckEvaluate(&param);

      ConstParamEntryPtr entry = args.FindEntry(param.name);
      if (!entry)
      {
        // use default value
        if (param.hasDefValue)
        {
          _args.Set(param.name, param.defValue, evaluate);
        }
      }
      else if (entry->IsArray())
      {
        // multiple values
        for (int j=0; j<entry->GetSize(); j++)
        {
          _args.Set(param.name, (*entry)[j], evaluate);
        }
      }
      else
      {
        _args.Set(param.name, *entry, evaluate);
      }
    }
  }
  
  ListAllParameters();
  _drawMapScriptChanged = true;
}

void EditorObject::UpdateAzimuth()
{
  if (_proxy)
  {
    float angle = atan2(_proxy->FutureVisualState().Direction().X(), _proxy->FutureVisualState().Direction().Z());
    if (UpdateArgument("AZIMUT", Format("%.8g", angle * (180.0f / H_PI)), true))
    {
      // update icons also
      int nIcons = _icons.Size();
      for (int i=0; i<nIcons; i++)
      {
        if (!_icons[i].is3D)
          _icons[i].angle = angle;
      }
    }
  }
}

static int CmpEditorIcons(const EditorObjectIcon *a, const EditorObjectIcon *b)
{
  return sign(b->priority - a->priority);
}

void EditorObject::AddIcon(const EditorObjectIcon &icon)
{
  if (icon.is3D && icon.line) _draw3DLine = true;
  _icons.Add(icon);

  // sort icons by priority
  QSort(_icons.Data(),_icons.Size(),CmpEditorIcons);
}

void EditorObject::RemoveIcon(RString id)
{
  for (int i=0; i<_icons.Size(); i++)
    if (id.GetLength() == 0 || stricmp(_icons[i].id,id) == 0) 
      _icons.Delete(i--);

  int n = _icons.Size();

  if (_draw3DLine)
  {
    if (n > 0)
      for (int i=0; i<n; i++)
        if (_icons[i].is3D && _icons[i].line)
          return;
    _draw3DLine = false;
  }
}     

static bool AddObjectType(const FileItem &file, RefArray<EditorObjectType> &types)
{
  const char *ptr = file.filename;
  const char *ext = strrchr(ptr, '.');
  Assert(ext);
  Assert(stricmp(ext, ".hpp") == 0);
  RString name(ptr, ext - ptr);
  types.Add(new EditorObjectType(name, file.path + file.filename));

  return false; // continue with enumeration
}

static int CmpTypes(const Ref<EditorObjectType> *ptr1, const Ref<EditorObjectType> *ptr2)
{
  const EditorObjectType *a = *ptr1;
  const EditorObjectType *b = *ptr2;
  
  int shortcutA = a->GetShortcutKey();
  if (shortcutA < 0) shortcutA = 0x10000; // types without shortcut will be the last
  int shortcutB = b->GetShortcutKey();
  if (shortcutB < 0) shortcutB = 0x10000; // types without shortcut will be the last

  int value = shortcutA - shortcutB;
  if (value != 0) return value;
  return strcmpi(a->GetTitle(), b->GetTitle());
}

static void FillArrays(RString fileName, AutoArray<RString> &create, AutoArray<RString> &update)
{
  GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

  ParamFile file;
  file.Parse(fileName, NULL, NULL, &globals);

  // append create script
  if (file.CheckIfEntryExists("create"))
  {
    ParamEntryVal array = file >> "create";
    int n = array.GetSize();
    for (int i=0; i<n; i++) {int j = create.Add(); create[j] = array[i];}
  }

  // append update script
  if (file.CheckIfEntryExists("update"))
  {
    ParamEntryVal array = file >> "update";
    int n = array.GetSize();
    for (int i=0; i<n; i++) {int j = update.Add(); update[j] = array[i];}
  }
}

void EditorWorkspace::Init()
{
  // vbs2 expects editor object .hpp files to be defined in CfgEditorObjects
  ParamEntryVal clsEditorObjects = Pars >> "CfgEditorObjects";
  int n = clsEditorObjects.GetEntryCount();
  for (int i=0; i<n; i++)
  {
    ParamEntryVal entry = clsEditorObjects.GetEntry(i);
    if (entry.IsClass()) continue;    
    
    RString typeName = entry.GetName();
    typeName.Lower();
    if (typeName.Find("prefix") > -1) continue;
    if (typeName.Find("postfix") > -1) continue;

    RString filename = entry;
    _types.Add(new EditorObjectType(entry.GetName(), filename));
  }

  // order types by shortcut key then alphabetical
  QSort(_types.Data(), _types.Size(), CmpTypes);
  _types.Compact();

  // vbs2 expects editor object .hpp files to be defined in CfgEditorObjects
    // there may be multiple prefix and postfix files! this combines them
  // process prefix files
  AutoArray<RString> create;
  AutoArray<RString> update;  
  RString prefixFile;
  for (int i=0; i<n; i++)
  {
    ParamEntryVal entry = clsEditorObjects.GetEntry(i);
    if (entry.IsClass()) continue;    
    RString typeName = entry.GetName();
    typeName.Lower();
    if (typeName.Find("prefix") == -1) continue;
    RString fileName = entry;
    if (i == 0) prefixFile = fileName;
    FillArrays(fileName,create,update);
  }

  // create a type out of the first found prefixFile
  _typePrefix = new EditorObjectType("prefix", prefixFile);  
  _typePrefix->SetCreateScript(create);
  _typePrefix->SetUpdateScript(update);

  // process postfix files
    // go backwards - we want the first postfix to come last so output order is correct
  create.Clear();
  update.Clear();  
  RString postfixFile;
  for (int i=n-1; i>=0; i--)
  {
    ParamEntryVal entry = clsEditorObjects.GetEntry(i);
    if (entry.IsClass()) continue;    
    RString typeName = entry.GetName();
    typeName.Lower();
    if (typeName.Find("postfix") == -1) continue;
    RString fileName = entry;
    postfixFile = fileName;
    FillArrays(fileName,create,update);
  }
  // create a type out of the last postfixFile
  _typePostfix = new EditorObjectType("postfix", postfixFile);  
  _typePostfix->SetCreateScript(create);
  _typePostfix->SetUpdateScript(update);

  _objectPrefix = new EditorObject(_typePrefix);
  _objectPostfix = new EditorObject(_typePostfix);

  _activeOverlay = NULL;
}

void EditorWorkspace::Clear()
{
  _objects.Clear();

  // reload types
  _types.Clear();
  Init();

  //_objectPrefix = new EditorObject(_typePrefix);
  //_objectPostfix = new EditorObject(_typePostfix);
}

void EditorWorkspace::SetSelected(EditorObject *selectObj, bool selected, bool subordinatesAlso, bool onlyHiddenSubordinates, bool isMultiple) 
{
  if (selected)
  {
    if (!selectObj->IsObjInCurrentOverlay()) return;    
    if (selectObj->GetScope() < EOSSelect) return;
  }

  // remove any existing references
  for (int i=0; i<_selected.Size(); i++)
  {
    if (_selected[i] == selectObj)
      _selected.Delete(i--);
  }

  selectObj->SetSelected(selected);

  if (selected)
    _selected.Add(selectObj);

  // also select objects that share the same proxy as selectObj
  if (!isMultiple)
  {
    const Object *selectedProxy = selectObj->GetProxyObject();
    if (selectedProxy)
    {
      for (int i=0; i<NObjects(); i++)
      {
        EditorObject *obj = GetObject(i);
        if (obj->IsSelected() != selected && selectedProxy == obj->GetProxyObject())
          SetSelected(obj,selected,true); // TODO: subordinatesOnly was false, why..? - probably because of team object?
      }
    }
  }

  // subordinates also?
  if (subordinatesAlso)
  {
    RString name = selectObj->GetArgument("VARIABLE_NAME");
    if (name.GetLength() > 0)
    {
      // iterate through subordinates
      for (int i=0; i<NObjects(); i++)
      {
        EditorObject *obj = GetObject(i);
        if (!onlyHiddenSubordinates || (onlyHiddenSubordinates && !obj->IsVisibleInTree()))
          if (stricmp(obj->GetArgument("PARENT"), name) == 0)
            SetSelected(obj,selected,true,onlyHiddenSubordinates);
      }
    }    
  }
}

void EditorWorkspace::ClearSelected() 
{
  for (int i=0; i<_selected.Size(); i++)
  {
    EditorObject *obj = _selected[i];
    obj->SetSelected(false);
  }  
  _selected.Clear();
}

EditorObject *EditorWorkspace::CopyObject(EditorObject *newObj)
{  
  if (newObj)
  {
    EditorObjectType *type = newObj->GetType();
    EditorObject *obj = AddObject(type);
    if (obj)
    {
      *obj = *newObj;       
      return obj;
    }
  }
  return NULL;
}

void EditorWorkspace::CopyObjects(RefArray<EditorObject> &clipboard)
{
  // find selected objects  
  for (int i=0; i<_selected.Size(); i++)
  {
    EditorObject *obj = _selected[i];
    if (obj->GetScope() < EOSDrag || obj->GetScope() == EOSAllNoCopy) continue;
    EditorObject *copiedObj = new EditorObject(obj->GetType());
    clipboard.Add(copiedObj);
    *copiedObj = *obj;
  }
  // order copied objects by link dependancy
  OrderObjects(clipboard);
}

void EditorWorkspace::AddObject(EditorObject *obj) 
{
  _objects.Add(obj);
  obj->SetParentOverlay(_activeOverlay,true);
}

EditorObject *EditorWorkspace::AddObject(EditorObjectType *type)
{
  Assert(type);
  EditorObject *obj = new EditorObject(type);
  AddObject(obj);
  return obj;
}

EditorObjectType *EditorWorkspace::FindObjectType(RString name)
{
  for (int i=0; i<_types.Size(); i++)
  {
    if (stricmp(_types[i]->GetName(), name) == 0) return _types[i];
  }
  return NULL;
}

EditorObject *EditorWorkspace::FindObject(RString id)
{
  // prefix
  EditorObject *obj = _objectPrefix;
  if (stricmp(obj->GetArgument("VARIABLE_NAME"), id) == 0) return obj;

  // objects
  for (int i=0; i<_objects.Size(); i++)
  {
    obj = _objects[i];
    if (stricmp(obj->GetArgument("VARIABLE_NAME"), id) == 0) return obj;
  }

  // postfix
  obj = _objectPostfix;
  if (stricmp(obj->GetArgument("VARIABLE_NAME"), id) == 0) return obj;

  return NULL;
}

struct LinkDependency
{
  EditorObject *obj;
  RefArray<const EditorObject> dependsOn;

  //! dependency resolved - may be removed from list
  void Resolved(const LinkDependency &dep);
};
TypeIsMovable(LinkDependency)

void LinkDependency::Resolved(const LinkDependency &dep)
{
  for (int i=0; i<dependsOn.Size(); i++)
  {
    if (dependsOn[i]!=dep.obj) continue;
    dependsOn.Delete(i--);
  }
}

void EditorWorkspace::OrderObjects(RefArray<EditorObject> &edObjects)
{
  // resolve link dependancies
  AutoArray<LinkDependency> dependencies;
  for (int i=0; i<edObjects.Size(); i++)
  {
    EditorObject *object = edObjects[i];

    LinkDependency &dep = dependencies.Append();
    dep.obj = object;

    // what does this object link to?
    for (int i=0; i<object->GetAllParams().Size(); i++)
    {
      const EditorParam &param = object->GetAllParams()[i];
      if (param.source == EPSLink || param.source == EPSParent)
      {
        AUTO_STATIC_ARRAY(int, indices, 16);

        for (int i=0; i<object->NArguments(); i++)
          if (stricmp(object->GetArgument(i).name, param.name) == 0) 
            indices.Add(i); 

        for (int i=0; i<indices.Size(); i++)
        {
          RString value = object->GetArgument(indices[i]).value;          
          const EditorObject *link = NULL;        
          for (int i=0; i<edObjects.Size(); i++)
            if (stricmp(edObjects[i]->GetArgument("VARIABLE_NAME"), value) == 0) {link = edObjects[i]; break;}
          if (link) dep.dependsOn.Add(link);
        }
      }        
    }    
  }

  // sort objects by dependencies
  RefArray<EditorObject> resolved;
  while(dependencies.Size()>0)
  {
    bool someResolved = false;
    for (int i=0; i<dependencies.Size(); i++)
    {
      const LinkDependency &dep = dependencies[i];
      if (dep.dependsOn.Size()>0) continue;
      
      resolved.Add(dep.obj);
      
      // removed this object from all dependencies lists
      for (int j=0; j<dependencies.Size(); j++)
      {
        dependencies[j].Resolved(dep);
      }
      dependencies.Delete(i--);
      someResolved = true;
    }
    if (!someResolved)
    {
      for (int i=0; i<dependencies.Size(); i++)
      {
        RStringB objName = dependencies[i].obj->GetArgument("VARIABLE_NAME");
        RString msg(Format("Circular link dependency in %s, depends on: ",(const char *)objName));
        for (int j=0; j<dependencies[i].dependsOn.Size(); j++)
        {
          if (j > 0) msg = msg + ", ";
          msg = msg + dependencies[i].dependsOn[j]->GetArgument("VARIABLE_NAME");
        }
        LogF(msg.Data());

        // simply add unresolved object to the end of the list
        resolved.Add(dependencies[i].obj);
      }
      break;
    }
  }
  // reorder objects
  edObjects = resolved;
}

void EditorWorkspace::WriteCreateScript(RString filename) const
{
  QOFStream out(filename);

  // prefix
  // addons stored within prefix are written seperately, as they can create a very long string
  const EditorParam *param = _objectPrefix->GetAllParams().Find("ADDON");
  if (param)
  {
    RString line = "activateAddons [ \r\n";
    for (int i=0; i<_objectPrefix->NArguments(); i++)
    {
      const EditorArgument &arg = _objectPrefix->GetArgument(i);
      if (stricmp(arg.name,"ADDON")) continue;
      line = line + "  \"" + arg.value + "\",\r\n";
    }
    WriteLine(out, RString(line,line.GetLength()-3) + "\r\n];\r\n");
  }
  _objectPrefix->WriteCreateScript(out);
  WriteLine(out, "");

  // objects
  for (int i=0; i<_objects.Size(); i++) 
  {
    _objects[i]->WriteCreateScript(out);
    WriteLine(out, "");
  }

  // postfix
  _objectPostfix->WriteCreateScript(out);
  
  out.close();
}

void EditorWorkspace::Save(RString filename) const
{
  ParamFile f;

  // prefix
  _objectPrefix->Save(f);

  // objects  
  for (int i=0; i<_objects.Size(); i++) _objects[i]->Save(f);

  // postfix
  _objectPostfix->Save(f);

  f.Save(filename);
}

void EditorWorkspace::Load(RString filename)
{
  GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

  ParamFile f;
  f.Parse(filename, NULL, NULL, &globals);

  for (int i=0; i<f.GetEntryCount(); i++)
  {
    ParamEntryVal cls = f.GetEntry(i);
    RString typeName = cls >> "objectType";
    EditorObjectType *type = NULL;
    Ref<EditorObject> obj = new EditorObject();
    if (stricmp(typeName, "prefix") == 0)
    {
      type = _typePrefix;
      _objectPrefix = obj;
    }
    else if (stricmp(typeName, "postfix") == 0)
    {
      type = _typePostfix;
      _objectPostfix = obj;
    }
    else
    {
      type = FindObjectType(typeName);
      AddObject(obj);
    }
    if (!type)
    {
      RptF("Object type %s definition not found", (const char *)typeName);
      continue;
    }
    obj->Load(type, cls);
  }
  _objects.Compact();

  // prefix
  _objectPrefix->Save(f);

  // objects
  for (int i=0; i<_objects.Size(); i++) _objects[i]->Save(f);

  // postfix
  _objectPostfix->Save(f);
}

void EditorOverlay::Save(const RefArray<EditorObject> &objects) const
{
  if (_name.GetLength() == 0) return;

  RString path = GetUserDirectory() + RString("Overlays\\") + _name + RString(".biedi");
  void CreatePath(RString path);
  CreatePath(path);

  ParamFile f;

  // prefix
//  _objectPrefix->Save(f);

  // objects  
  for (int i=0; i<objects.Size(); i++) 
    objects[i]->Save(f);

  // postfix
//  _objectPostfix->Save(f);

  f.Save(path);
}

EditorOverlay *EditorWorkspace::LoadOverlay(RString name)
{
  if (name.GetLength() == 0) return NULL;

  EditorOverlay *overlay = NULL;

  // check if overlay is already loaded
  for (int i=0; i<_overlays.Size(); i++)
    if (stricmp(_overlays[i]->GetName(),name) == 0)
    {
      overlay = _overlays[i];
      break;
    }

  // create empty overlay
  if (!overlay)
  {
    overlay = new EditorOverlay(name);
    _overlays.Add(overlay);    
    // set allowed types?
  }  

  return overlay;
}

void EditorWorkspace::SaveOverlay(const EditorOverlay *overlay)
{
  if (!overlay) return;

  RefArray<EditorObject> objects;
  for (int i=0; i<_objects.Size(); i++) 
  {
    if (_objects[i]->GetParentOverlay() == overlay)
      objects.Add(_objects[i]);   
  }
  OrderObjects(objects);
  overlay->Save(objects);
}

EditorOverlay *EditorWorkspace::CreateOverlay(RString name)
{
  EditorOverlay *overlay = new EditorOverlay(name);
  _overlays.Add(overlay);    
  return overlay;
}

void EditorWorkspace::SetActiveOverlay(EditorOverlay *activeOverlay) 
{
  _activeOverlay = activeOverlay;
  if (!_activeOverlay) _overlays.Clear();

  for (int i=0; i<_objects.Size(); i++)
  {
    EditorObject *obj = _objects[i];
    const EditorOverlay *parentOverlay = obj->GetParentOverlay();

    // if there is no overlay, all objects are in current    
    bool isInCurrent = activeOverlay == NULL;
    if (activeOverlay && parentOverlay == _activeOverlay)
      isInCurrent = true;
    obj->SetParentOverlay(parentOverlay,isInCurrent);
  }
}  

void EditorWorkspace::RenameObjects(RefArray<EditorObject> &objects)
{
  for (int i=0; i<objects.Size(); i++)
  {      
    RString oldName = objects[i]->GetArgument("VARIABLE_NAME");
    EditorObjectType *type = objects[i]->GetType();
    if (type && oldName.GetLength() > 0)
    {        
      RString newName = type->NextVarName();
      objects[i]->RemoveArgument("VARIABLE_NAME");        
      objects[i]->SetArgument("VARIABLE_NAME", newName);

      // loop through all objects in the clipboard, updating names
      for (int i=0; i<objects.Size(); i++)
      {   
        EditorObject *obj = objects[i];
        for (int i=0; i<obj->NArguments(); i++)
        {
          const EditorArgument &arg = obj->GetArgument(i);
          const EditorParam *param = obj->GetAllParams().Find(arg.name);              
          if (param && (param->source == EPSLink || param->source == EPSParent))
          {
            if (stricmp(arg.value,oldName) == 0)
            {
              RString name = arg.name;
              obj->RemoveArgument(name);              
              obj->SetArgument(name, newName);
            }
          }
        } 
      }
    }
  }
}

void EditorWorkspace::MoveObjectToEnd(EditorObject *obj)
{
  int i = _objects.Find(obj);
  if (i > -1)
  {
    _objects.Add(obj);
    _objects.Delete(i);
  }
}

#endif // _ENABLE_EDITOR2
