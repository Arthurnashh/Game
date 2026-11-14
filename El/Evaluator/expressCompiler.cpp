#include <El/elementpch.hpp>
#include <El/Common/perfProf.hpp>

#include "express.hpp"

#if USE_PRECOMPILATION

#include <El/ParamArchive/paramArchive.hpp>

LSError SourceDoc::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("filename", _filename, 1))
    CHECK(ar.Serialize("content", _content, 1))
    return LSOK;
}

//////////////////////////////////////////////////////////////////////////
//
// Source document position (+ parsing code)
//

SourceDocPos::SourceDocPos()
{
  _pos = 0;
  _sourceLine = 1;
}

SourceDocPos::SourceDocPos(SourceDoc &doc)
{
  _content = doc._content;
  _pos = 0;
  _sourceFile = doc._filename;
  _sourceLine = 1;
}

LSError SourceDocPos::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("sourceFile", _sourceFile, 1))
    CHECK(ar.Serialize("sourceLine", _sourceLine, 1))
    CHECK(ar.Serialize("content", _content, 1))
    CHECK(ar.Serialize("pos", _pos, 1))
    return LSOK;
}

bool SourceDocPos::ProcessLineNumber()
{
  const char *start = cc_cast(_content) + _pos;
  // check for #line directive
  static const char *prefix = "#line";
  static const int prefixLen = strlen(prefix);
  if (strncmp(start, prefix, prefixLen) != 0) return false;
  const char *ptr = start + prefixLen;

  if (!isspace(*ptr)) return false;
  while (isspace(*ptr)) ptr++;

  // line number
  _sourceLine = 0;
  if (!isdigit(*ptr)) return false;
  while (isdigit(*ptr))
  {
    _sourceLine *= 10;
    _sourceLine += *ptr - '0';
    ptr++;
  }

  if (!isspace(*ptr)) return false;
  while (isspace(*ptr)) ptr++;

  // filename
  if (*ptr != '"') return false;
  ptr++;
  const char *end = strchr(ptr, '"');
  if (!end) return false;
  _sourceFile = RString(ptr, end - ptr);
  ptr = end + 1;

  // update read position
  const char *nl = strchr(ptr, '\n');
  if (!nl) return false;
  _pos += nl + 1 - start; // skip \n also
  return true;
}

void SourceDocPos::SkipSpaces()
{
  while (true)
  {
    const char *ptr = cc_cast(_content) + _pos;
    if (*ptr == 0) return;
    if (*ptr == '\n')
    {
      _pos++;
      _sourceLine++;
    }
    else if (*ptr == '#')
    {
      if (!ProcessLineNumber()) return;
    }
    else if (isspace(*ptr))
    {
      _pos++;
    }
    else return;
  }
}

void SourceDocPos::Skip(int chars)
{
  // used only after GetChars and GetWord
  _pos += chars;
}

char SourceDocPos::GetChar()
{
  return *(cc_cast(_content) + _pos);
}

void SourceDocPos::GetChars(char *chars, int count)
{
  const char *src = cc_cast(_content) + _pos;
  for (int i=0; i<count; i++)
  {
    if (*src == 0 || isspace((unsigned char)*src)) break;
    *chars = *src;
    chars++; src++;
  }
  *chars = 0;
}

RString SourceDocPos::GetWord()
{
  const char *src = cc_cast(_content) + _pos;
  const char *ptr = src;
  while (isalnumext(*ptr)) ptr++;
  return RString(src, ptr - src);
}

bool SourceDocPos::ReadString(RString &str)
{
  // read starting quote mark
  const char *src = cc_cast(_content) + _pos;
  char quote = *src;
  src++;
  _pos++;

  const char *ptr = src;
  while (true)
  {
    if (!*ptr)
    {
      return false;
    }
    else if (*ptr == quote)
    {
      // string from start to expression
      str = str + RString(src, ptr - src);
      // skip behind quote
      ptr++;
      _pos += ptr - src;

      // check if another quote mark is encountered
      // no - we are done
      if (*ptr != quote) return true;
      // yes - append it, and continue parsing
      src = ptr;
      ptr++;
    }
    else if (*ptr == '\n')
    {
      ptr++;
      _sourceLine++;
    }
    else if (*ptr == '#')
    {
      // copy string to this position
      str = str + RString(src, ptr - src);
      _pos += ptr - src;
      if (ProcessLineNumber())
      {
        // skip directive
        src = ptr = cc_cast(_content) + _pos;
      }
      else
      {
        // simple #
        src = ptr;
        ptr++;
      }
    }
    else
    {
      ptr++;
    }
  }
}

bool SourceDocPos::ReadCodeString(RString &str)
{
  const char *src = cc_cast(_content) + _pos;

  int level = 1;
  const char *ptr = src;
  while (true)
  {
    if (*ptr == 0)
    {
      return false;
    }
    // note: any quotes inside may prevent closing bracket
    else if (*ptr == '"')
    {
      ptr++;
      while (*ptr != '"')
      {
        if (*ptr == 0)
        {
          return false;
        }
        else if (*ptr == '\n')
        {
          ptr++;
          _sourceLine++;
        }
        else if (*ptr == '#')
        {
          // copy string to this position
          str = str + RString(src, ptr - src);
          _pos += ptr - src;
          if (ProcessLineNumber())
          {
            // skip directive
            src = ptr = cc_cast(_content) + _pos;
          }
          else
          {
            // simple #
            src = ptr;
            ptr++;
          }
        }
        else
        {
          ptr++;
        }
      }
      ptr++;
    }
    // note: any single quotes inside may prevent closing bracket
    else if (*ptr == '\'')
    {
      ptr++;
      while (*ptr != '\'')
      {
        if (*ptr == 0)
        {
          return false;
        }
        else if (*ptr == '\n')
        {
          ptr++;
          _sourceLine++;
        }
        else if (*ptr == '#')
        {
          // copy string to this position
          str = str + RString(src, ptr - src);
          _pos += ptr - src;
          if (ProcessLineNumber())
          {
            // skip directive
            src = ptr = cc_cast(_content) + _pos;
          }
          else
          {
            // simple #
            src = ptr;
            ptr++;
          }
        }
        else
        {
          ptr++;
        }
      }
      ptr++;
    }
    else if (*ptr == OPEN_STRING)
    {
      level++;
      ptr++;
    }
    else if (*ptr == CLOSE_STRING)
    {
      if (--level == 0)
      {
        str = str + RString(src, ptr - src);
        _pos += ptr + 1 - src;
        return true;
      }
      ptr++;
    }
    else if (*ptr == '\n')
    {
      ptr++;
      _sourceLine++;
    }
    else if (*ptr == '#')
    {
      // copy string to this position
      str = str + RString(src, ptr - src);
      _pos += ptr - src;
      if (ProcessLineNumber())
      {
        // skip directive
        src = ptr = cc_cast(_content) + _pos;
      }
      else
      {
        // simple #
        src = ptr;
        ptr++;
      }
    }
    else
    {
      ptr++;
    }
  }
}

bool SourceDocPos::ReadNumber(float &number)
{
  const char *src = cc_cast(_content) + _pos;

  // -------------------------------------------------------------------------------
  // Number in hexadecimal format 0x000

  if (src[0] == '0' && src[1] == 'x')
  {
    const char *ptr = src + 2;
    if (!isxdigit(*ptr)) return false;
    int val = 0;
    do
    {
      val *= 16;
      if (isdigit(*ptr)) val += *ptr - '0';
      else val += toupper(*ptr) - 'A' + 10;
      ptr++;
    } while(isxdigit(*ptr));

    _pos += ptr - src;
    number = val;
    return true;
  }

  // -------------------------------------------------------------------------------
  // Number in hexadecimal format $FF

  if (*src == '$')
  {
    const char *ptr = src + 1;
    if (!isxdigit(*ptr)) return false;
    int val = 0;
    do
    {
      val *= 16;
      if (isdigit(*ptr)) val += *ptr - '0';
      else val += toupper(*ptr) - 'A' + 10;
      ptr++;
    } while(isxdigit(*ptr));

    _pos += ptr - src;
    number = val;
    return true;
  }

  // -------------------------------------------------------------------------------
  // Number in decimal format

  char *ptr;
  number = strtod(src, &ptr);
  if (ptr == src) return false;
  _pos += ptr - src;
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// GameDataCode - encapsulates code scope {}
//

#if USE_PRECOMPILATION
DEFINE_FAST_ALLOCATOR_EXPRESS(GameDataCode)
#endif

GameDataCode::GameDataCode(const GameState *state, SourceDoc &src, SourceDocPos &pos, GameDataNamespace *globals)
{
  _value._compiled = false;

  SourceDocPos start = pos;
  if (!pos.ReadCodeString(_value._string))
  {
    state->SetError(start, EvalCloseBraces);
    return;
  }
  SourceDocPos end = pos;

  GameVarSpace local(state->GetContext(), false);
  int gshandle = state->BeginContext(&local);
  state->CompileMultiple(src, start, _value._code, true, globals); // check and skip trailing }
  if (state->GetLastError() == EvalOK)
  {
    _value._compiled = true;
    pos = start;
  }
  else
  {
    state->ShowError(); // display error on this level

    _value._code.Clear();
    // skip
    pos = end;
  }
  state->EndContext(gshandle);
}

GameDataCode::GameDataCode(const GameState *state, RString expression, GameDataNamespace *globals)
{
  _value._compiled = false;
  _value._string = expression;

  SourceDoc doc; doc._content = expression;
  SourceDocPos pos(doc);
  GameVarSpace local(state->GetContext(), false);
  int gshandle = state->BeginContext(&local);
  state->CompileMultiple(doc, pos, _value._code, false, globals);
  if (state->GetLastError() == EvalOK)
  {
    _value._compiled = true;
  }
  else
  {
    state->ShowError(); // display error on this level

    _value._code.Clear();
  }
  state->EndContext(gshandle);
}

RString GameDataCode::GetText() const
{
  return RString("{") + _value._string + RString("}");
}

bool GameDataCode::IsEqualTo(const GameData *data) const
{
  RString val1 = GetString();
  RString val2 = data->GetString();
  return val1 == val2;
}

jobject GameDataCode::ToJObject(JNIEnv *env, GameValuePar from) const
{
  return ConvertTypes<&GTypeGameCode>::FromGameValue(env, from);
}

#ifndef ACCESS_ONLY
LSError GameDataCode::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar));
  CHECK(ar.Serialize("value", _value._string, 1));
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    SourceDoc scope;
    scope._content = _value._string;
    SourceDocPos scopePos(scope);

    GameState *state = reinterpret_cast<GameState *>(ar.GetParams());
    GameVarSpace local(state->GetContext(), false);
    int gshandle = state->BeginContext(&local);
    state->CompileMultiple(scope, scopePos, _value._code, false, state->GetGlobalVariables());
    if (state->GetLastError() == EvalOK)
      _value._compiled = true;
    else
    {
      _value._compiled = false;
      _value._code.Clear();
    }
    state->EndContext(gshandle);
  }
  return (LSError)0;
}
#endif

#if SCRIPTVM_DEBUGGING

#include <Es/Common/win.h>
#include <El/Debugging/debugTrap.hpp>

// TODO: move to AppFrame module
void ProcessMessages()
{
  // why are messages processed here?
  MSG msg;
  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
#if _GAMES_FOR_WINDOWS
    if (!XLivePreTranslateMessage(&msg))
#endif
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  // Note: bad module interface design - cross module dependency with no interface
  GDebugger.ProcessAlive();
  I_AM_ALIVE();
}

RString GetFullPath(RString name)
{
  char buffer[_MAX_PATH];
  if (GetFullPathName(name, _MAX_PATH, buffer, NULL) == 0) return name;
  return buffer;
}


bool GameDataCode::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // go through instructions
  for (int i=0; i<_value._code.Size(); i++)
  {
    if (_value._code[i]->SetBreakpoint(breakpoint)) return true;
  }
  return false;
}

bool GameDataCode::RemoveBreakpoint(DWORD bp)
{
  // go through instructions
  for (int i=0; i<_value._code.Size(); i++)
  {
    if (_value._code[i]->RemoveBreakpoint(bp)) return true;
  }
  return false;
}

bool GameDataCode::EnableBreakpoint(DWORD bp, bool enable)
{
  // go through instructions
  for (int i=0; i<_value._code.Size(); i++)
  {
    if (_value._code[i]->EnableBreakpoint(bp, enable)) return true;
  }
  return false;
}

#endif

//////////////////////////////////////////////////////////////////////////
//
// GameInstruction implementations
//

bool GameInstruction::operator != (const GameInstruction &w) const
{
  return GetDebugName()!=w.GetDebugName();
}

#if SCRIPTVM_DEBUGGING

bool GameInstruction::CanStep(StepUnit unit) const
{
  return unit == SUInstruction;
}

bool GameInstruction::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // by default matching breakpoint is set
  bool matching = stricmp(GetFullPath(_srcPos._sourceFile), breakpoint._file) == 0 &&
    _srcPos._sourceLine - 1 == breakpoint._line;
  if (matching)
  {
    _breakpoint._id = breakpoint._id;
    _breakpoint._enabled = breakpoint._enabled;
    LogF("Breakpoint %d set.", breakpoint._id);
  }
  return matching;
}

bool GameInstruction::RemoveBreakpoint(DWORD bp)
{
  if (_breakpoint._id == bp)
  {
    _breakpoint._id = 0;
    _breakpoint._enabled = false;
    return true;
  }
  return false;
}

bool GameInstruction::EnableBreakpoint(DWORD bp, bool enable)
{
  if (_breakpoint._id == bp)
  {
    _breakpoint._enabled = enable;
    return true;
  }
  return false;
}

#endif // SCRIPTVM_DEBUGGING

DEFINE_FAST_ALLOCATOR_EXPRESS(GameInstructionConst)
DEFINE_FAST_ALLOCATOR_EXPRESS(GameInstructionVariable)
DEFINE_FAST_ALLOCATOR_EXPRESS(GameInstructionOperator)
DEFINE_FAST_ALLOCATOR_EXPRESS(GameInstructionFunction)
DEFINE_FAST_ALLOCATOR_EXPRESS(GameInstructionArray)
DEFINE_FAST_ALLOCATOR_EXPRESS(GameInstructionAssignment)
DEFINE_FAST_ALLOCATOR_EXPRESS(GameInstructionNewExpression)

bool GameInstructionConst::Execute(const GameState &state, VMContext &ctx)
{
  // Copy the value directly to the stack
  ctx._stack.Add(_value);
  return false; // continue with execution
}

#if SCRIPTVM_DEBUGGING
bool GameInstructionConst::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // cannot set breakpoint to constant
  // if constant is code, breakpoint will be set when scope will be entered
  return false;
}
#endif

bool GameInstructionVariable::Execute(const GameState &state, VMContext &ctx)
{
  // Lookup the variable for value and push it to the stack
  GameValue value;

  // If it starts with the '_' sign, then look it up in the local variable space
  if (_name[0] == '_')
  {
    // local variable
    const GameVarSpace *space = state._e->local;
    if (!space)
    {
      state.SetError(_srcPos, EvalNamespace);
      return true; // terminate execution
    }
    space->VarGet(_name, value);
    //if (value.GetNil() && ctx._undefinedIsNil)
    if (value.GetNil() && !state._e->_checkOnly && !ctx._undefinedIsNil)
    {
      state.SetError(_srcPos, EvalVar, cc_cast(_name));
    }
  }
  else
  {
    GameDataNamespace *globals = state.GetGlobalVariables();
    DoAssert(globals);
    if (!globals)
    {
      state.SetError(_srcPos, EvalNamespace);
      return true; // terminate execution
    }

#if !_VBS3 //avoid overriding functions/ operators with variables
    GameValue check;
    if (state._e->_checkOnly || !globals->VarGet(_name, check))
#endif
    {
      // nular operator
      const GameNular &o = state._nulars[_name];
      if (state._nulars.NotNull(o))
      {
        if (state._e->_checkOnly)
        {
          value = GameValue(new GameDataNil(o._operator->GetRetType()));
        }
        else
        {
          value = o._operator->Evaluate(&state);
        }
        ctx._stack.Add(value);
        return false; // continue with execution
      }
    }
    if (!state.VarGoodName(_name))
    {
      state.SetError(_srcPos, EvalBadVar);
      return true; // terminate execution
    }
    // global variable
    if (state._e->_checkOnly)
    {
      value = state.CreateGameValue(GameVoid);
    }
    else
    {
      value = state.VarGet(_name, false, globals);
      if (value.GetNil() && !state._e->_checkOnly && !ctx._undefinedIsNil)
      {
        state.SetError(_srcPos, EvalVar, cc_cast(_name));
      }
    }
  }
  ctx._stack.Add(value);
  return false; // continue with execution
}

bool GameInstructionOperator::Execute(const GameState &state, VMContext &ctx)
{
  int stackBottom = ctx.GetStackBottom();

  // Get 2 items from the stack, do the operation and push back the result
  // If there are not 2 items on the stack then report error
  if (ctx._stack.Size() < stackBottom + 2)
  {
    state.SetError(_srcPos, EvalGen);
    return true; // terminate execution
  }

  GameValue value;
  if (!state._e->_checkOnly)
  {
    const GameOperator *op = NULL;
    for (int i=0; i < _operators->Size(); i++)
    {
      const GameOperator &o = (*_operators)[i];
      if(!(o._operator->GetArg1Type() & ctx._stack[ctx._stack.Size() - 2].GetType())) continue;
      if(!(o._operator->GetArg2Type() & ctx._stack[ctx._stack.Size() - 1].GetType())) continue;
      op = &o;
      break;
    }

    // If operator doesn't exist then report error
    if (!op)
    {
      state.TypeErrorOperator(_srcPos, _operators->_name, ctx._stack[ctx._stack.Size() - 2].GetType(), ctx._stack[ctx._stack.Size() - 1].GetType());
      return true; // terminate execution
    }

    // Do the operation
    if (ctx._stack[ctx._stack.Size() - 2].GetNil() || ctx._stack[ctx._stack.Size() - 1].GetNil())
    {
      // If any of the argument is nil then the result is nil as well
      value = GameValue(new GameDataNil(op->_operator->GetRetType()));
    }
    else
    {
      // Evaluate the operator
      value = op->_operator->Evaluate(&state, ctx._stack[ctx._stack.Size() - 2], ctx._stack[ctx._stack.Size() - 1]);
    }
  }
  else
  {
    GameType possible;
    for (int i=0; i<_operators->Size(); i++)
    {
      const GameOperator &o = (*_operators)[i];
      if (!(o._operator->GetArg1Type() & ctx._stack[ctx._stack.Size() - 2].GetType())) continue;
      if (!(o._operator->GetArg2Type() & ctx._stack[ctx._stack.Size() - 1].GetType())) continue;
      possible |= o._operator->GetRetType();
    }

    if (!possible)
    {
      state.TypeErrorOperator(_srcPos, _operators->_name, ctx._stack[ctx._stack.Size() - 2].GetType(), ctx._stack[ctx._stack.Size() - 1].GetType());
      return true; // terminate execution
    }

    value = GameValue(new GameDataNil(possible));
  }

  // Put the result to the stack
  int n = ctx._stack.Size() - 1;
  ctx._stack.Resize(n);
  ctx._stack[n - 1] = value;
  return false; // continue with execution
}

bool GameInstructionFunction::Execute(const GameState &state, VMContext &ctx)
{
  int stackBottom = ctx.GetStackBottom();

  // Get 1 item from the stack, do the operation and push back the result
  // If there is not 1 item on the stack then report error
  if (ctx._stack.Size() < stackBottom + 1)
  {
    state.SetError(_srcPos, EvalGen);
    return true; // terminate execution
  }

  GameValue value;
  if (!state._e->_checkOnly)
  {
    const GameFunction *f = NULL;
    for (int i=0; i<_functions->Size(); i++)
    {
      const GameFunction &fc = (*_functions)[i];
      if (!(fc._operator->GetArgType() & ctx._stack[ctx._stack.Size() - 1].GetType())) continue;
      f = &fc;
      break;
    }

    // If function doesn't exist then report error
    if (!f)
    {
      state.TypeErrorFunction(_srcPos, _functions->_name, ctx._stack[ctx._stack.Size() - 1].GetType());
      return true; // terminate execution
    }

    // Do the operation
    if (ctx._stack[ctx._stack.Size() - 1].GetNil())
    {
      // If the argument is nil then the result is nil as well
      value = GameValue(new GameDataNil(f->_operator->GetRetType()));
    }
    else
    {
      // Evaluate the operator
      value = f->_operator->Evaluate(&state, ctx._stack[ctx._stack.Size() - 1]);
    }
  }
  else
  {
    GameType possible;
    for (int i=0; i<_functions->Size(); i++)
    {
      const GameFunction &fc = (*_functions)[i];
      if (fc._operator->GetArgType() & ctx._stack[ctx._stack.Size() - 1].GetType())
      {
        possible |= fc._operator->GetRetType();
      }
    }

    if (!possible)
    {
      state.TypeErrorFunction(_srcPos, _functions->_name, ctx._stack[ctx._stack.Size() - 1].GetType());
      return true; // terminate execution
    }

    value = GameValue(new GameDataNil(possible));
  }

  // Put the result to the stack
  ctx._stack[ctx._stack.Size() - 1] = value;
  return false; // continue with execution
}

bool GameInstructionArray::Execute(const GameState &state, VMContext &ctx)
{
  int stackBottom = ctx.GetStackBottom();

  // Get required number of items from the stack, construct an array and push it back
  // If there are not so many items on the stack then report error
  if (ctx._stack.Size() < stackBottom + _size)
  {
    state.SetError(_srcPos, EvalGen);
    return true; // terminate execution
  }

  // Fill out the array
  GameArrayType array;
  array.Realloc(_size);
  array.Resize(_size);
  for (int i=0; i<_size; i++)
  {
    array[i] = ctx._stack[ctx._stack.Size() - _size + i];
  }

  // Put the result to the stack
  int n = ctx._stack.Size() - _size + 1;
  ctx._stack.Resize(n);
  ctx._stack[n - 1] = array;
  return false; // continue with execution
}

#if SCRIPTVM_DEBUGGING
bool GameInstructionArray::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // cannot set breakpoint to array
  return false;
}
#endif


bool GameInstructionAssignment::Execute(const GameState &state, VMContext &ctx)
{
  int stackBottom = ctx.GetStackBottom();

  // Assignment "operator"
  // If there is not 1 item on the stack then report error
  if (ctx._stack.Size() != stackBottom + 1)
  {
    state.SetError(_srcPos, EvalGen);
    return true; // terminate execution
  }

  GameValue value = ctx._stack[ctx._stack.Size() - 1];
  if (!state._e->_checkOnly)
  {
    if (value.SharedInstance() && value.GetType() != GameArray && value.GetType() != GameNamespace)
    {
      //LogF("Shared data (variable?) duplicated: '%s'",expression);
      value.Duplicate(value);
    }
    if (ctx._localOnly || _forceLocal)
    {
      unconst_cast(state).VarSetLocal(_name, value);
    }
    else
    {
      GameDataNamespace *globals = state.GetGlobalVariables();
      DoAssert(globals);
      if (!globals)
      {
        state.SetError(_srcPos, EvalNamespace);
        return true; // terminate execution
      }
      unconst_cast(state).VarSet(_name, value, false, false, globals);
    }
  }
  else
  {
    if (value.GetType() == GameNothing)
    {
      state.TypeError(_srcPos, GameAny, value.GetType());
      return true; // terminate execution
    }
    if (state.IsFakeType(value.GetType()))
    {
      state.TypeError(_srcPos, GameAny, value.GetType());
      return true; // terminate execution
    }
  }
  // update stack
  ctx._stack.Resize(stackBottom);
  return false; // continue with execution
}

bool GameInstructionNewExpression::Execute(const GameState &state, VMContext &ctx)
{
  // New subexpression

  int stackBottom = ctx.GetStackBottom();
  if (ctx._stack.Size() > stackBottom)
  {
    // check current stack 
    if (ctx._stack.Size() > stackBottom + 1)
    {
      state.SetError(_srcPos, EvalGen);
      return true; // terminate execution
    }
    if (!(ctx._stack[stackBottom].GetType() & GameNothing))
    {
      if (state._e->_checkOnly)
      {
        state.TypeError(_srcPos, GameNothing, ctx._stack[stackBottom].GetType());
        return true; // terminate execution
      }     
    }

    // clear stack
    ctx._stack.Resize(stackBottom);
  }

#ifdef _SCRIPT_DEBUGGER
  if (state._debugger) 
  { 
    if (state._debugger->BeforeLineProcessed(const_cast<GameState &>(state)) == false)
    {
      return true; // terminate execution ???
    }
  }
#endif
  return false; // continue with execution
}

int GameInstructionNewExpression::GetStackChange(VMContext &ctx) const
{
  // will be resized to _stackBottom if more than _stackBottom items present
  return std::min(ctx.GetStackBottom() - ctx._stack.Size(), 0);
}

#if SCRIPTVM_DEBUGGING

bool GameInstructionNewExpression::CanStep(StepUnit unit) const
{
  // TODO: check for SULine
  return true;
}

#endif

//////////////////////////////////////////////////////////////////////////
//
// Call stack level implementations
//

CallStackItem::CallStackItem(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, bool enableSerialization)
: _parent(parent), _vars(parentVars, enableSerialization)
#if SCRIPTVM_DEBUGGING
,_sourceLine(0)
#endif
{
  _stackBottom = stackBottom;
  // we want the result remain on the stack when done with the scope
  _stackLast = stackBottom + 1;
#if SCRIPTVM_DEBUGGING
  _debugName = name;
  if (parent)
  {
    _sourceFile = _sourceFile;
    _sourceLine = _sourceLine;
  }
#endif
}

CallStackItem *CallStackItem::CreateObject(ParamArchive &ar)
{
  GameState *state = reinterpret_cast<GameState *>(ar.GetParams());
  Assert(state);

  RString typeName;
  if (ar.Serialize("typeName", typeName, 1) != LSOK) return NULL;
  int parent;
  if (ar.Serialize("parent", parent, 1) != LSOK) return NULL;
  return state->CreateCallStackItem(typeName, parent, true); // when creating in serialization, it can be serialized at sure
}

LSError CallStackItem::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving())
  {
    GameState *state = reinterpret_cast<GameState *>(ar.GetParams());
    Assert(state);
    VMContext *ctx = state->GetVMContext();
    Assert(ctx);

    RString typeName = GetTypeName();
    CHECK(ar.Serialize("typeName", typeName, 1))
      int parent = ctx->FindLevel(this);
    if (parent >= 0) parent--;
    CHECK(ar.Serialize("parent", parent, 1))
  }
  CHECK(ar.Serialize("Variables", _vars._vars, 1)) // local variables
#if SCRIPTVM_DEBUGGING
    CHECK(ar.Serialize("debugName", _debugName, 1, "Frame"))
#endif
    CHECK(ar.Serialize("stackBottom", _stackBottom, 1))
    CHECK(ar.Serialize("stackLast", _stackLast, 1))
    CHECK(ar.Serialize("scopeName", _scopeName, 1))
    return LSOK;
}

bool CallStackItem::OnBreak(const GameState *state)
{
  // default behaviour (for simple scope of code)
  VMContext *context = state->GetVMContext();
  Assert(context);
  if (context->_breakScope.GetLength() == 0)
  {
    if (context->_breakOut)
    {
      // break out of this scope
      context->_breakOut = false;
      return false;
    }
    else
    {
      // handle break in this scope (level up than break was thrown)
      Break(state);
      return true;
    }
  }
  else if (stricmp(context->_breakScope, _scopeName) == 0)
  {
    if (context->_breakOut)
    {
      // break out of this scope
      context->_breakScope = RString();
      context->_breakOut = false;
      return false;
    }
    else
    {
      // handle break in this scope (level up than break was thrown)
      Break(state);
      return true;
    }
  }
  else return false;
}

void CallStackItem::Break(const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);

  // FIX: the _stack size after break should be _stackLast
  if (_stackLast < 1)
  {
    // nothing expected on the stack
    context->_stack.Resize(0);
  }
  else
  {
    // update stack
    if (context->_stack.Size() < _stackLast - 1)
    {
      state->SetError(EvalGen);
      return;
    }
    context->_stack.Resize(_stackLast - 1);

    // add _breakValue to stack
    context->_stack.Add(context->_breakValue);
  }

  context->_break = false;
}

// IDebugScope implementation

const char *CallStackItem::GetName() const
{
#if SCRIPTVM_DEBUGGING
  return _debugName;
#else
  return "Frame";
#endif
}

void CallStackItem::GetDocumentPos(char *file, int fileSize, int &line)
{
#if SCRIPTVM_DEBUGGING
  strncpy(file, _sourceFile, fileSize);
  file[fileSize - 1] = 0;
  line = _sourceLine - 1;
#else
  file[0] = 0;
  line = 0;
#endif
}

int CallStackItem::NVariables() const
{
  int count = 0;
  const GameVarSpace *seek = &_vars;
  while (seek)
  {
    count = seek->_vars.NItems();
    seek = seek->_parent;
  }
  return count;
}

class AddVariableFunc
{
protected:
  const IDebugVariable **_storage;
  int _count;
  int &_index;

public:
  AddVariableFunc(const IDebugVariable **storage, int count, int &filled)
    : _index(filled)
  {
    _storage = storage;
    _count = count;
  }
  bool operator () (const GameVariable &var, const VarBankType *container)
  {
    if (_index >= _count) return true; // stop enumeration
    _storage[_index++] = &var;
    return false; // continue
  }
};

int CallStackItem::GetVariables(const IDebugVariable **storage, int count) const
{
  int filled = 0;
#if !defined(_MSC_VER) || _MSC_VER>1300
  AddVariableFunc func(storage, count, filled);
  const GameVarSpace *seek = &_vars;
  while (seek)
  {
    seek->_vars.ForEachF(func);
    seek = seek->_parent;
  }
#else
  Fail("Not supported with VC 6.0");
#endif
  return filled;
}

IDebugValueRef CallStackItem::EvaluateExpression(const char *code, unsigned int radix)
{
  GameDataNamespace globals(NULL, RString(), false); // no global namespace

  int gshandle = GGameState.BeginContext(&_vars);
  GameValue result = GGameState.Evaluate(code, GameState::EvalContext::_default, &globals);
  GGameState.EndContext(gshandle);
  return result.GetData();
}

IDebugScope *CallStackItem::GetParentScope()
{
  return _parent;
}

void VMContext::DisableSerialization()
{
  // disable serialization of all contained variable spaces
  _enabledSerialization = false;
  for (int i=0; i<_callStack.Size(); i++) _callStack[i]->GetVariables().DisableSerialization();
}

LSError VMContext::Serialize(ParamArchive &ar)
{
  GameState *state = reinterpret_cast<GameState *>(ar.GetParams());
  Assert(state);
  // access to context during call stack serialization
  VMContext *oldContext = state->SetVMContext(this);

  CHECK(ar.Serialize("doc", _doc, 1))
    CHECK(ar.Serialize("lastInstruction", _lastInstruction, 1))

    CHECK(ar.Serialize("callStack", _callStack, 1))
    CHECK(ar.Serialize("stack", _stack, 1))

    CHECK(ar.Serialize("canSuspend", _canSuspend, 1))
    CHECK(ar.Serialize("localOnly", _localOnly, 1))
    CHECK(ar.Serialize("exceptionThrown", _exceptionThrown, 1))
    CHECK(ar.Serialize("break", _break, 1))
    CHECK(ar.Serialize("breakOut", _breakOut, 1))
    CHECK(ar.Serialize("breakScope", _breakScope, 1))
    CHECK(ar.Serialize("exceptionValue", _exceptionValue, 1))
    CHECK(ar.Serialize("breakValue", _breakValue, 1))

    state->SetVMContext(oldContext);

  return LSOK;
}

#if !_SUPER_RELEASE
# include <El/HiResTime/hiResTime.hpp>
/// check if FSM execution took longer than normal
struct GuardEvaluationScopeTime
{
  RString _name;
  SectionTimeHandle _start;
  static const float _limit;

  struct InstructionInfo
  {
    float _time;
    RString _instr;
    ClassIsMovableZeroed(InstructionInfo);
  };
  AutoArray<InstructionInfo> _log;

  GuardEvaluationScopeTime(const RString &name)
    : _name(name)
  {
    if (_name.GetLength() > 0)
    {
      _start = StartSectionTime();
    }
  }
  ~GuardEvaluationScopeTime()
  {
    if (_name.GetLength() > 0)
    {
      float time = GetSectionTime(_start);
      if (time > _limit)
      {
        // summary
        LogF("- evaluation of %s ... %.1f ms (%d instructions)", cc_cast(_name), 1000.0f * time, _log.Size());
        // select expressions with duration >= 1 ms
        static const float expLimit = 0.001f;
        int lastIndex = 0;
        float lastTime = 0;
        for (int i=0; i<_log.Size(); i++)
        {
          float time = _log[i]._time;
          if (time > 0)
          {
            // new expression
            if (time - lastTime >= expLimit)
            {
              // output command
              LogF(" ... single expression taking %.1f ms:", 1000.0f * (time - lastTime));
              for (int j=lastIndex; j<i; j++) LogF("     %s", cc_cast(_log[j]._instr));
            }
            lastIndex = i;
            lastTime = _log[i]._time;
          }
        }
      }
    }
  }

  void Instruction(GameInstruction *instr)
  {
    if (_name.GetLength() > 0)
    {
      InstructionInfo info;
      info._instr = instr->GetDebugName();
      info._time = -1.0f;
      if (instr->IsNewExpression()) info._time = GetSectionTime(_start);
      _log.Add(info);
    }
  }
};

const float GuardEvaluationScopeTime::_limit = 0.003f;
#endif

VMContextCallback *VMContext::defaultCallback = NULL;  //no processing as default. Application should set it if scripts executions is to be monitored.

#if _DEBUG_GS_CONTEXT
bool VMContext::EvaluateCore(const GameState &state, int minVarLevel, IDebugScript *script, const IEvalInterrupt &funcInterrupt)
{
  int levelBefore = state._contextStack.Size();
  bool retVal = EvaluateCore2(state, minVarLevel, script, funcInterrupt);
  int levelAfter = state._contextStack.Size();
  if (levelBefore!=levelAfter)
  { // Hotfix: call missing EndContexts
    ErrF("EvaluateCore: levelBefore=%d, levelAfter=%d", levelBefore, levelAfter);
    while (levelAfter>levelBefore)
    {
      levelAfter--;
      state.ShowError(); // display error on this level
      state.EndContext(-1); //test should fail, as this is only hotfix
    }
  }
  return retVal;
}
#endif

#if _DEBUG_GS_CONTEXT
bool VMContext::EvaluateCore2(const GameState &state, int minVarLevel, IDebugScript *script, const IEvalInterrupt &funcInterrupt)
#else
bool VMContext::EvaluateCore(const GameState &state, int minVarLevel, IDebugScript *script, const IEvalInterrupt &funcInterrupt)
#endif
{
#if !_SUPER_RELEASE
  RString profileName; // not empty when we want to profile this expression
  // TODO: better control over switching on / off script profiling (enable / disable it for separate scripts)
  if (state.IsProfilingEnabled())
  {
    profileName = _doc._filename;
  }
  GuardEvaluationScopeTime scope(profileName);
#endif
 
  StackArray<int, MemAllocLocal<int, 64> > gsHandles;

  RString checkSourceLast;

  #define CALLBACK_EXPRESSIONS 0
  #if CALLBACK_EXPRESSIONS
  int checkSourceBegLast = 0;
  int checkSourceEndLast = INT_MAX;

  int checkSourceBeg = 0;
  int checkSourceEnd = INT_MAX;
  #endif

  int varLevel = minVarLevel;
  while (true)
  {
    int level = _callStack.Size() - 1;
    if (level < 0) 
    {
      // restore context
      while (varLevel > minVarLevel)
      {
        varLevel--;
        state.ShowError(); // display error on this level
        int gshandle = 0;
        if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
        state.EndContext(gshandle);
      }
      return true;
    }

    if (_exceptionThrown) // handle exception
    {
      while (level >= 0)
      {
        if (_callStack[level]->OnException(&state)) break;
        // level up
        if (varLevel > minVarLevel)
        {
          varLevel--;
          state.ShowError(); // display error on this level
          int gshandle = 0;
          if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
          state.EndContext(gshandle);
        }
        _callStack.Resize(level);
        level--;
      }
      if (level < 0) state.SetError(EvalUnhandledException, cc_cast(_exceptionValue.GetText()));
      if (state.GetLastError() != EvalOK)
      {
        // end evaluation
        // restore context
        while (varLevel > minVarLevel)
        {
          varLevel--;
          state.ShowError(); // display error on this level
          int gshandle = 0;
          if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
          state.EndContext(gshandle);
        }
        return true;
      }
      Assert(!_exceptionThrown);
      level = _callStack.Size() - 1; // level can be changed in OnException
    }
    else if (_break) // handle break
    {
      while (level >= 0)
      {
        if (_callStack[level]->OnBreak(&state)) break;
        // level up
        if (varLevel > minVarLevel)
        {
          varLevel--;
          state.ShowError(); // display error on this level
          int gshandle = 0;
          if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
          state.EndContext(gshandle);
        }
        _callStack.Resize(level);
        level--;
      }
      if (level < 0) 
      {
        // restore context
        while (varLevel > minVarLevel)
        {
          varLevel--;
          state.ShowError(); // display error on this level
          int gshandle = 0;
          if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
          state.EndContext(gshandle);
        }
        return true;
      }
      if (state.GetLastError() != EvalOK)
      {
        // end evaluation
        // restore context
        while (varLevel > minVarLevel)
        {
          varLevel--;
          state.ShowError(); // display error on this level
          int gshandle = 0;
          if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
          state.EndContext(gshandle);
        }
        return true;
      }
      Assert(!_break);
      level = _callStack.Size() - 1; // level can be changed in OnBreak
    }

    // check a new call stack level
    while (level > varLevel)
    {
      varLevel++;
      int gshandle = state.BeginContext(&_callStack[varLevel]->GetVariables());
      gsHandles.Push(gshandle);
    }

    RequiredExecutionControl rec;
    GameInstruction *instr = _callStack[level]->GetNextInstruction(rec, &state);
#if !_SUPER_RELEASE
    if (instr) scope.Instruction(instr);
#endif
    switch (rec)
    {
    case RECInstruction:
      Assert(instr);

      // FIX: set what _stack size is expected after the instruction
      // when break occurs during processing, we need to recover _stack to this size
      _callStack[level]->SetStackLast(_stack.Size() + instr->GetStackChange(*this));

      _lastInstruction = instr->GetSourcePos();
#if SCRIPTVM_DEBUGGING
      if (script)
      {
        _callStack[level]->SetSourcePos(_lastInstruction._sourceFile, _lastInstruction._sourceLine);

        DWORD bp = instr->IsBreakpointFired();
        if (bp) CurrentDebugEngineFunctions->FireBreakpoint(script, bp);
        else if (GetDebugState() == DSStep && level <= _stepLevel && instr->CanStep(_stepUnit))
        {
          // check if break is needed
          CurrentDebugEngineFunctions->Breaked(script);
        }
        // stop execution of program until script is able to run
        while (GetDebugState() == DSBreaked) ProcessMessages();
        if (GetDebugState() == DSStepInit)
        {
          // we could break on the next instruction
          SetDebugState(DSStep);
          switch (_stepKind)
          {
          case SKInto:
            _stepLevel = INT_MAX; break; // finish step in any level
          case SKOver:
            _stepLevel = level; break; // finish step on this level
          case SKOut:
            _stepLevel = level - 1; break; // finish step on upper level
          }
        }
      }
#endif


      {
        RString checkSource = instr->GetSourcePos()._content;

        #if CALLBACK_EXPRESSIONS
        if (cc_cast(checkSource)!=cc_cast(checkSourceLast))
        {
          checkSourceBeg = 0;
          checkSourceEnd = checkSource.GetLength();
        }

        // detect GameInstructionNewExpression
        // TODO: use virtual interface
        if (GameInstructionNewExpression *newExpr=dynamic_cast<GameInstructionNewExpression *>(instr))
        {
          checkSourceBeg = newExpr->GetSourceBeg();
          checkSourceEnd = newExpr->GetSourceEnd();
        }

        if (cc_cast(checkSource)!=cc_cast(checkSourceLast) || checkSourceBeg!=checkSourceBegLast || checkSourceEnd!=checkSourceEndLast)
        {
          RString sourcePart = RString(checkSource.Data()+checkSourceBeg,checkSourceEnd-checkSourceBeg);
          CheckScriptCallback(sourcePart);
          checkSourceLast = checkSource;
          checkSourceBegLast = checkSourceBeg;
          checkSourceEndLast = checkSourceEnd;
        }
        #else
        if (cc_cast(checkSource)!=cc_cast(checkSourceLast))
        {
          CheckScriptCallback(checkSource);
        }
        #endif
      }

      if (instr->Execute(state, *this))
      {
        // end evaluation
        // restore context
        while (varLevel > minVarLevel)
        {
          varLevel--;
          state.ShowError(); // display error on this level
          int gshandle = 0;
          if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
          state.EndContext(gshandle);
        }
        return true;
      }
      else
      {
        // check if the external conditions to yield script execution was met
        bool interrupt = instr->IsNewExpression() && funcInterrupt();
        if (!interrupt) break;
        // continue to RECYield otherwise
      }
    case RECYield:
      if (_canSuspend)
      {
        // restore context
        while (varLevel > minVarLevel)
        {
          varLevel--;
          state.ShowError(); // display error on this level
          int gshandle = 0;
          if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
          state.EndContext(gshandle);
        }
        return false; // done for this simulation step
      }
      RptF("Suspending not allowed in this context");
      state.SetError(EvalGen);
      state.ShowError();
      // error recovery
      state.SetError(EvalOK);
      // continue
    case RECDone:
      while (varLevel > level - 1 && varLevel > minVarLevel)
      {
        varLevel--;
        state.ShowError(); // display error on this level
        int gshandle = 0;
        if (!gsHandles.Empty()) { gshandle = gsHandles.Top(); gsHandles.Pop(); }
        state.EndContext(gshandle);
      }
      _callStack.Resize(level);
      break; // continue on the calling level
    case RECContinue:
      break;
    }
  }
}

int VMContext::AddLevel(CallStackItem *level)
{
  int index = _callStack.Add(level);
#if SCRIPTVM_DEBUGGING
  for (int i=0; i<_breakpoints.Size(); i++)
    _callStack[index]->SetBreakpoint(_breakpoints[i]);
#endif
  return index;
}

#if SCRIPTVM_DEBUGGING

void VMContext::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  _breakpoints.AddUnique(breakpoint);

  for (int i=0; i<_callStack.Size(); i++)
    _callStack[i]->SetBreakpoint(breakpoint);
}

void VMContext::RemoveBreakpoint(DWORD bp)
{
  _breakpoints.DeleteKey(bp);

  for (int i=0; i<_callStack.Size(); i++)
    _callStack[i]->RemoveBreakpoint(bp);
}

void VMContext::EnableBreakpoint(DWORD bp, bool enable)
{
  int index = _breakpoints.FindKey(bp);
  if (index >= 0) _breakpoints[index]._enabled = enable;

  for (int i=0; i<_callStack.Size(); i++)
    _callStack[i]->EnableBreakpoint(bp, enable);
}

#endif // SCRIPTVM_DEBUGGING

DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemSimple)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemData)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemDataBool)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemWith)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemRepeat)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemArrayCount)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemArrayForEach)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemForBASIC)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemForC)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemSwitch)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemTry)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemExitWith)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemWaitUntil)
DEFINE_FAST_ALLOCATOR_EXPRESS(CallStackItemWaitUntilNative)

CallStackItemSimple::CallStackItemSimple(GameDataNamespace *globalVars, CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, const SourceDoc &content, bool multiple, bool enableSerialization)
: CallStackItem(parent, parentVars, stackBottom, state, name, enableSerialization)
{
  _ip = 0;
  _content = content;
  _multiple = multiple;

  SourceDocPos pos(_content);
  if (multiple)
    state->CompileMultiple(_content, pos, _code, false, globalVars);
  else
    state->Compile(_content, pos, _code, globalVars);
}

CallStackItemSimple::CallStackItemSimple(GameDataNamespace *globalVars, CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, const SourceDoc &content, const CompiledExpression &compiled, bool enableSerialization)
: CallStackItem(parent, parentVars, stackBottom, state, name, enableSerialization)
{
  _ip = 0;
  _content = content;
  _multiple = true;
  _code = compiled;
}

GameInstruction *CallStackItemSimple::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  if (_ip >= _code.Size())
  {
    rec = RECDone;
    return NULL;
  }
  else
  {
    rec = RECInstruction;
    return _code[_ip++];
  }
}

LSError CallStackItemSimple::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar));
  CHECK(ar.Serialize("ip", _ip, 1));
  CHECK(ar.Serialize("content", _content, 1));
  CHECK(ar.Serialize("multiple", _multiple, 1));
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    GameState *state = reinterpret_cast<GameState *>(ar.GetParams());
    SourceDocPos pos(_content);
    if (_multiple)
      state->CompileMultiple(_content, pos, _code, false, state->GetGlobalVariables());
    else
      state->Compile(_content, pos, _code, state->GetGlobalVariables());
  }
  return LSOK;
}

bool CallStackItemSimple::IsDifferent(const CallStackItemSimple &w) const
{
  // compare the two representations
  if (_code.Size()!=w._code.Size())
  {
    LogF("  size does not match");
    return true;
  }
  else
  {
    for (int i=0; i<_code.Size(); i++)
    {
      const GameInstruction *i1 = _code[i];
      const GameInstruction *i2 = w._code[i];
      if (*i1!=*i2)
      {
        LogF("  Subexpression %d instruction %d does not match",i1);
        return true;
      }
    }
  }
  return false;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemSimple::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // go through instructions
  for (int i=0; i<_code.Size(); i++)
  {
    if (_code[i]->SetBreakpoint(breakpoint)) break;
  }
}

void CallStackItemSimple::RemoveBreakpoint(DWORD bp)
{
  // go through instructions
  for (int i=0; i<_code.Size(); i++)
  {
    if (_code[i]->RemoveBreakpoint(bp)) break;
  }
}

void CallStackItemSimple::EnableBreakpoint(DWORD bp, bool enable)
{
  // go through instructions
  for (int i=0; i<_code.Size(); i++)
  {
    if (_code[i]->EnableBreakpoint(bp, enable)) break;
  }
}

#endif

GameInstruction *CallStackItemData::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  const CompiledExpression &code = _code->GetCode();
  if (_ip >= code.Size())
  {
    rec = RECDone;
    return NULL;
  }
  else
  {
    rec = RECInstruction;
    return code[_ip++];
  }
}

static LSError SerializeGameCode(ParamArchive &ar, RString name, Ref<GameDataCode> &code)
{
  if (ar.IsSaving() || ar.GetPass() != ParamArchive::PassFirst)
  {
    GameValue value(code);
    CHECK(ar.Serialize(name, value, 1))
  }
  else
  {
    GameValue value;
    CHECK(ar.Serialize(name, value, 1))
      if (value.GetType() == GameCode) code = static_cast<GameDataCode *>(value.GetData());
      else code = NULL;
  }
  return LSOK;
}

static LSError SerializeGameArray(ParamArchive &ar, RString name, Ref<GameDataArray> &array)
{
  if (ar.IsSaving() || ar.GetPass() != ParamArchive::PassFirst)
  {
    GameValue value(array);
    CHECK(ar.Serialize(name, value, 1))
  }
  else
  {
    GameValue value;
    CHECK(ar.Serialize(name, value, 1))
      if (value.GetType() == GameArray) array = static_cast<GameDataArray *>(value.GetData());
      else array = NULL;
  }
  return LSOK;
}

LSError CallStackItemData::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))
    CHECK(SerializeGameCode(ar, "code", _code))
    CHECK(ar.Serialize("ip", _ip, 1))
    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemData::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  if (_code) _code->SetBreakpoint(breakpoint);
}

void CallStackItemData::RemoveBreakpoint(DWORD bp)
{
  if (_code) _code->RemoveBreakpoint(bp);
}

void CallStackItemData::EnableBreakpoint(DWORD bp, bool enable)
{
  if (_code) _code->EnableBreakpoint(bp, enable);
}

#endif

GameInstruction *CallStackItemDataBool::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  const CompiledExpression &code = _code->GetCode();
  if (_ip >= code.Size())
  {
    // check if result on stack is a boolean
    VMContext *context = state->GetVMContext();
    Assert(context);
    if (context->_stack.Size() == _stackBottom + 1)
    {
      int n = context->_stack.Size() - 1;
      GameValue value = context->_stack[n];
      if (value.GetType() & GameBool)
      {
        // OK
      }
      else
      {
        state->TypeError(GameBool, value.GetType());
      }
    }
    else
    {
      state->SetError(EvalGen);
    }
    rec = RECDone;
    return NULL;
  }
  else
  {
    rec = RECInstruction;
    return code[_ip++];
  }
}

CallStackItemWith::CallStackItemWith(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, GameDataNamespace *ns, GameDataCode *code, bool enableSerialization)
: CallStackItemData(parent, parentVars, stackBottom, state, name, code, enableSerialization)
{
  _state = state;
  _namespace = ns;
  _oldNamespace = state->SetGlobalVariables(ns);
}

CallStackItemWith::~CallStackItemWith()
{
  _state->SetGlobalVariables(_oldNamespace);
}

LSError CallStackItemWith::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItemData::Serialize(ar))

  // only reference need to be serialized (not the contained variables)
  CHECK(ar.Serialize("namespace", _namespace, 1));

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassSecond)
  {
    // set namespace to the GameState
    _state = reinterpret_cast<GameState *>(ar.GetParams());
    _oldNamespace = _state->SetGlobalVariables(static_cast<GameDataNamespace *>(_namespace.GetRef()));
  }
  return LSOK;
}

/*!
\patch 5134 Date 2/26/2007 by Jirka
- Fixed: Limit of iterations in while cycle no longer applied for scripts ran through execVM or spawn
*/

GameInstruction *CallStackItemRepeat::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);
  switch (_phase)
  {
  case PAfterCondition:
    // on the stack is - statement result, condition result
    if (context->_stack.Size() == _stackBottom + 2)
    {
      int n = context->_stack.Size() - 1;
      GameValue value = context->_stack[n];
      context->_stack.Resize(n);
      if (value.GetType() & GameBool)
      {
        bool cond = value;
        if (cond)
        {
          // statement to call stack
          // replace value on the top of stack (statement result)
          CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size() - 1, state, "While body", _statement, context->IsSerializationEnabled());
          context->AddLevel(level);

          _phase = PAfterStatement;
          rec = RECContinue;
          return NULL;
        }
      }
      else
      {
        state->TypeError(GameBool, value.GetType());
      }
    }
    else
    {
      state->SetError(EvalGen);
    }
    break;
  case PAfterStatement:
    // validate stack
    if (context->_stack.Size() == _stackBottom)
    {
      // no result (empty statement)
      context->_stack.Add(NOTHING);
    }
    else if (context->_stack.Size() != _stackBottom + 1)
    {
      state->SetError(EvalGen);
      break;
    }
    // check number of iterations
    _iter++;
    if (_maxIters)
    {
      if (context->_canSuspend)
      {
        if (_iter % _maxIters == 0)
        {
          _phase = PBegin;
          rec = RECYield;
          return NULL;
        }
      }
      else
      {
        if (_iter >= _maxIters) break;
      }
    }
    // continue
  case PBegin:
    Assert(context->_stack.Size() == _stackBottom + 1);
    {
      // condition to call stack
      // place condition result on the top of stack (leave last statement result there)
      CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size(), state, "While condition", _condition, context->IsSerializationEnabled());
      context->AddLevel(level);
    }
    _phase = PAfterCondition;
    rec = RECContinue;
    return NULL;
  case PDone:
    break;
  }
  // finished
  rec = RECDone;
  return NULL;
}

bool CallStackItemRepeat::OnBreak(const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);
  if (context->_breakScope.GetLength() > 0) return CallStackItem::OnBreak(state);

  Assert(!context->_breakOut);
  Break(state);
  _phase = PDone;
  return true;
}

LSError CallStackItemRepeat::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))
    CHECK(SerializeGameCode(ar, "condition", _condition))
    CHECK(SerializeGameCode(ar, "statement", _statement))
    CHECK(ar.Serialize("phase", (int &)_phase, 1))
    CHECK(ar.Serialize("iter", _iter, 1))
    CHECK(ar.Serialize("maxIters", _maxIters, 1))
    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemRepeat::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemRepeat::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemRepeat::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif

GameInstruction *CallStackItemArrayCount::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  if (_breaked)
  {
    rec = RECDone;
    return NULL;
  }

  const GameArrayType &array = _array->GetArray();

  VMContext *context = state->GetVMContext();
  Assert(context);

  int n = context->_stack.Size() - 1;
  if (_i == 0)
  {
    // remove item passed by count function
    Assert(n == _stackBottom);
    context->_stack.Resize(n);
  }
  else
  {
    // check stack - increase result
    if (n == _stackBottom)
    {
      GameValue value = context->_stack[n];
      context->_stack.Resize(n);
      if (value.GetType() & GameBool)
      {
        if ((bool)value) _result++;
      }
      else
      {
        state->TypeError(GameBool, value.GetType());
        rec = RECDone;
        return NULL;
      }
    }
    else if (n + 1 != _stackBottom) // empty expression is ok (like false)
    {
      state->SetError(EvalGen);
      rec = RECDone;
      return NULL;
    }
  }
  if (_i >= array.Size())
  {
    // finished
    // push result to stack
    context->_stack.Add((float)_result);

    rec = RECDone;
    return NULL;
  }

  // code to call stack
  CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size(), state, "Count body", _code, context->IsSerializationEnabled());
  context->AddLevel(level);

  level->GetVariables().VarLocal("_x");
  level->GetVariables().VarSet("_x", array[_i], true);
  _i++;

  rec = RECContinue;
  return NULL;
}

bool CallStackItemArrayCount::OnBreak(const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);
  if (context->_breakScope.GetLength() > 0) return CallStackItem::OnBreak(state);

  Assert(!context->_breakOut);
  Break(state);
  _breaked = true;
  return true;
}

LSError CallStackItemArrayCount::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

    CHECK(SerializeGameCode(ar, "code", _code))
    CHECK(SerializeGameArray(ar, "array", _array))
    CHECK(ar.Serialize("i", _i, 1))
    CHECK(ar.Serialize("result", _result, 1))
    CHECK(ar.Serialize("breaked", _breaked, 1))

    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemArrayCount::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemArrayCount::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemArrayCount::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif

GameInstruction *CallStackItemArrayForEach::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  if (_breaked)
  {
    rec = RECDone;
    return NULL;
  }

  const GameArrayType &array = _array->GetArray();

  VMContext *context = state->GetVMContext();
  Assert(context);

  // validate stack - leave single value on stack (as result)
  if (context->_stack.Size() == _stackBottom)
  {
    context->_stack.Add(NOTHING);
  }
  else if (context->_stack.Size() != _stackBottom + 1)
  {
    state->SetError(EvalGen);
    rec = RECDone;
    return NULL;
  }

  if (_i >= array.Size())
  {
    // finished
    rec = RECDone;
    return NULL;
  }

  // code to call stack
  // replace value on the top of stack (result)
  CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size() - 1, state, "ForEach body", _code, context->IsSerializationEnabled());
  context->AddLevel(level);

  level->GetVariables().VarLocal("_x");
  level->GetVariables().VarSet("_x", array[_i], true);
  level->GetVariables().VarLocal("_foreachindex");
  level->GetVariables().VarSet("_foreachindex", (float)_i, true);
  _i++;

  rec = RECContinue;
  return NULL;
}

bool CallStackItemArrayForEach::OnBreak(const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);
  if (context->_breakScope.GetLength() > 0) return CallStackItem::OnBreak(state);

  Assert(!context->_breakOut);
  Break(state);
  _breaked = true;
  return true;
}

LSError CallStackItemArrayForEach::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))
    CHECK(SerializeGameCode(ar, "code", _code))
    CHECK(SerializeGameArray(ar, "array", _array))
    CHECK(ar.Serialize("i", _i, 1))
    CHECK(ar.Serialize("breaked", _breaked, 1))
    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemArrayForEach::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemArrayForEach::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemArrayForEach::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif

GameInstruction *CallStackItemForBASIC::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  if (_breaked)
  {
    rec = RECDone;
    return NULL;
  }

  VMContext *context = state->GetVMContext();
  Assert(context);

  // access to my variable space
  GameVarSpace *vars = context->GetVariables();

  // not for the first pass
  if (_passed)
  {
    // check stack - always single value should be there
    if (context->_stack.Size() == _stackBottom)
    {
      context->_stack.Add(NOTHING);
    }
    else if (context->_stack.Size() != _stackBottom + 1)
    {
      state->SetError(EvalGen);
      rec = RECDone;
      return NULL;
    }

    // update varValue (can change inside statement)
    GameValue var;
    if (!vars || !vars->VarGet(_varName, var))
    {
      // TODO: error
      rec = RECDone;
      return NULL;
    }
    else if (var.GetType() & GameScalar)
    {
      _varValue = var;
    }
    else
    {
      // TODO: error
      rec = RECDone;
      return NULL;
    }
    _varValue += _step;
  }

  // check ending condition
  if (_step >= 0)
  {
    if (_varValue >= _to)
    {
      rec = RECDone;
      return NULL;
    }
  }
  else
  {
    if (_varValue <= _to)
    {
      rec = RECDone;
      return NULL;
    }
  }

  // prepare execution of statement
  CallStackItem *level = new CallStackItemData(context->GetCallStack(), vars, context->_stack.Size() - 1, state, "For body (BASIC)", _code, context->IsSerializationEnabled()); // replace value on the top of stack
  context->AddLevel(level);
  vars->VarLocal(_varName); // create own instance
  vars->VarSet(_varName, _varValue);

  _passed = true;

  rec = RECContinue;
  return NULL;
}

bool CallStackItemForBASIC::OnBreak(const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);
  if (context->_breakScope.GetLength() > 0) return CallStackItem::OnBreak(state);

  Assert(!context->_breakOut);
  Break(state);
  _breaked = true;
  return true;
}

LSError CallStackItemForBASIC::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

    CHECK(ar.Serialize("varName", _varName, 1))
    CHECK(ar.Serialize("varValue", _varValue, 1))
    CHECK(ar.Serialize("to", _to, 1))
    CHECK(ar.Serialize("step", _step, 1))
    CHECK(SerializeGameCode(ar, "code", _code))
    CHECK(ar.Serialize("passed", _passed, 1))
    CHECK(ar.Serialize("breaked", _breaked, 1))

    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemForBASIC::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemForBASIC::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemForBASIC::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif

GameInstruction *CallStackItemForC::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);

  switch (_phase)
  {
  case PInit:
    {
      // init code must be evaluated in this variable space (to access local variables later)
      const CompiledExpression &code = _init->GetCode();
      if (_ip < code.Size())
      {
        rec = RECInstruction;
        return code[_ip++];
      }
      _phase = PAfterUpdate;
      // validate stack - leave single value on stack (as result)
      if (context->_stack.Size() < _stackBottom)
      {
        state->SetError(EvalGen);
        break;
      }
      else if (context->_stack.Size() == _stackBottom)
      {
        context->_stack.Add(NOTHING);
      }
      // continue
    }
  case PAfterUpdate:
    // validate stack - leave single value on stack (as result)
    if (context->_stack.Size() < _stackBottom + 1)
    {
      state->SetError(EvalGen);
      break;
    }
    context->_stack.Resize(_stackBottom + 1);
    {
      // condition to call stack
      CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size(), state, "For condition (C)", _condition, context->IsSerializationEnabled());
      context->AddLevel(level);
    }
    _phase = PAfterCondition;
    rec = RECContinue;
    return NULL;
  case PAfterCondition:
    if (context->_stack.Size() == _stackBottom + 2)
    {
      int n = context->_stack.Size() - 1;
      GameValue value = context->_stack[n];
      context->_stack.Resize(n);
      if (value.GetType() & GameBool)
      {
        bool cond = value;
        if (cond)
        {
          // statement to call stack
          // replace value on the top of stack (result)
          CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size() - 1, state, "For body (C)", _code, context->IsSerializationEnabled());
          context->AddLevel(level);

          _phase = PAfterStatement;
          rec = RECContinue;
          return NULL;
        }
        // end
      }
      else
      {
        state->TypeError(GameBool, value.GetType());
      }
    }
    else
    {
      state->SetError(EvalGen);
    }
    break;
  case PAfterStatement:
    // validate stack - single value must be on the stack (result)
    if (context->_stack.Size() == _stackBottom)
    {
      context->_stack.Add(NOTHING);
    }
    else if (context->_stack.Size() != _stackBottom + 1)
    {
      state->SetError(EvalGen);
      break;
    }

    {
      // update to call stack
      CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size(), state, "For update (C)", _update, context->IsSerializationEnabled());
      context->AddLevel(level);
    }

    _phase = PAfterUpdate;
    rec = RECContinue;
    return NULL;
  case PDone:
    break;
  }
  // finished
  rec = RECDone;
  return NULL;
}

bool CallStackItemForC::OnBreak(const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);
  if (context->_breakScope.GetLength() > 0) return CallStackItem::OnBreak(state);

  Assert(!context->_breakOut);
  Break(state);
  _phase = PDone;
  return true;
}

LSError CallStackItemForC::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

    CHECK(SerializeGameCode(ar, "init", _init))
    CHECK(SerializeGameCode(ar, "condition", _condition))
    CHECK(SerializeGameCode(ar, "update", _update))
    CHECK(SerializeGameCode(ar, "code", _code))
    CHECK(ar.Serialize("phase", (int &)_phase, 1))
    CHECK(ar.Serialize("ip", _ip, 1))

    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemForC::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  if (_init) _init->SetBreakpoint(breakpoint);
  // others processed by the next level
}

void CallStackItemForC::RemoveBreakpoint(DWORD bp)
{
  if (_init) _init->RemoveBreakpoint(bp);
  // others processed by the next level
}

void CallStackItemForC::EnableBreakpoint(DWORD bp, bool enable)
{
  if (_init) _init->EnableBreakpoint(bp, enable);
  // others processed by the next level
}

#endif

GameInstruction *CallStackItemSwitch::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);

  switch (_phase)
  {
  case PInit:
    {
      // access to my variable space
      GameVarSpace *vars = context->GetVariables();
      Assert(vars);

      // set helper variable
      vars->VarLocal("___switch"); // create own instance
      vars->VarSet("___switch", _switch);

      // start execution of statement
      CallStackItem *level = new CallStackItemData(context->GetCallStack(), vars, context->_stack.Size(), state, "Switch body", _code, context->IsSerializationEnabled());
      context->AddLevel(level);

      _phase = PAfterStatement;
      rec = RECContinue;
      return NULL;
    }
  case PAfterStatement:
    {
      // validate stack
      if (context->_stack.Size() < _stackBottom)
      {
        state->SetError(EvalGen);
        break;
      }
      context->_stack.Resize(_stackBottom);

      // access to my variable space
      GameVarSpace *vars = context->GetVariables();
      Assert(vars);

      // find result branch
      GameValue result;
      if (!vars->VarGet("___switch", result))
      {
        state->SetError(EvalForeignError, "Invalid switch block");
        break;
      }
      if (!state->IsSwitchType(result.GetType()))
      {
        state->SetError(EvalForeignError,"Unable to evaluate switch block");
        break;
      }

      GameDataSwitch *sw =  static_cast<GameDataSwitch *>(result.GetData());
      GameValuePar code = sw->_mode == sw->swTriggered ? sw->_selBranch : sw->_defBranch;
      if (code.GetType() == GameCode)
      {
        // start execution of selected branch
        GameDataCode *statement = static_cast<GameDataCode *>(code.GetData());
        CallStackItem *level = new CallStackItemData(context->GetCallStack(), vars, context->_stack.Size(), state, "Switch case", statement, context->IsSerializationEnabled());
        context->AddLevel(level);

        _phase = PDone;
        rec = RECContinue;
        return NULL;
      }
      else
      {
        GameVarSpace local(state->GetContext(), false);
        int gshandle = state->BeginContext(&local);
        GameValue result;
        if (code.GetNil()) result = GameValue(true);
        else 
        {
          RString codeExpr = code.operator GameStringType();
          if (codeExpr.IsEmpty()) 
            LogF("Warning: Empty expression evaluated!");
          result = state->EvaluateMultiple(codeExpr, GameState::EvalContext::_default, state->GetGlobalVariables()); // propagate current global variables
        }
        // TODO: check errors
        state->EndContext(gshandle);
        // store result on the stack
        context->_stack.Add(result);
        break;
      }
    }
  case PDone:
    // validate stack
    if (context->_stack.Size() == _stackBottom)
    {
      // no result (empty statement)
      context->_stack.Add(NOTHING);
    }
    else if (context->_stack.Size() != _stackBottom + 1)
    {
      state->SetError(EvalGen);
    }
    break;
  }
  // finished
  rec = RECDone;
  return NULL;
}

LSError CallStackItemSwitch::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

    CHECK(ar.Serialize("switch", _switch, 1))
    CHECK(SerializeGameCode(ar, "code", _code))
    CHECK(ar.Serialize("phase", (int &)_phase, 1))

    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemSwitch::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemSwitch::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemSwitch::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif


GameInstruction *CallStackItemTry::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);

  switch (_phase)
  {
  case PInit:
    {
      // access to my variable space
      // start execution of statement - replace value on the top of stack (result)
      CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size() - 1, state, "Try body", _tryCode, context->IsSerializationEnabled());
      context->AddLevel(level);

      _phase = PAfterTry;
      rec = RECContinue;
      return NULL;
    }
  case PAfterTry:
  case PDone:
    {
      Assert(!context->_exceptionThrown);
      // validate stack
      if (context->_stack.Size() == _stackBottom)
      {
        // no result (empty statement)
        context->_stack.Add(NOTHING);
      }
      else if (context->_stack.Size() != _stackBottom + 1)
      {
        state->SetError(EvalGen);
        break;
      }
      break; // done
    }
  }
  // finished
  rec = RECDone;
  return NULL;
}

bool CallStackItemTry::OnException(const GameState *state)
{
  if (_phase == PDone) return false;

  VMContext *context = state->GetVMContext();
  Assert(context);
  context->_exceptionThrown = false; // handled

  Assert(_phase == PAfterTry);
  if (context->_stack.Size() < _stackBottom)
  {
    state->SetError(EvalGen);
    return true;
  }
  context->_stack.Resize(_stackBottom);

  // start execution of statement
  CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size(), state, "Catch body", _catchCode, context->IsSerializationEnabled());
  context->AddLevel(level);
  level->GetVariables().VarLocal("_exception");
  level->GetVariables().VarSet("_exception", context->_exceptionValue, true);
  _phase = PDone;
  return true;
}

LSError CallStackItemTry::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

    CHECK(SerializeGameCode(ar, "tryCode", _tryCode))
    CHECK(SerializeGameCode(ar, "catchCode", _catchCode))
    CHECK(ar.Serialize("phase", (int &)_phase, 1))

    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemTry::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemTry::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemTry::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif

GameInstruction *CallStackItemExitWith::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);

  switch (_phase)
  {
  case PInit:
    {
      // start execution of statement - replace value on the top of stack (result)
      CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size() - 1, state, "ExitWith body", _code, context->IsSerializationEnabled());
      context->AddLevel(level);

      _phase = PDone;
      rec = RECContinue;
      return NULL;
    }
  case PDone:
    // validate stack
    if (context->_stack.Size() == _stackBottom)
    {
      // no result (empty statement)
      context->ThrowBreak(true, NOTHING);
    }
    else if (context->_stack.Size() == _stackBottom + 1)
    {
      int n = context->_stack.Size() - 1;
      context->ThrowBreak(true, context->_stack[n]);
      context->_stack.Resize(n);
    }
    else
    {
      state->SetError(EvalGen);
    }
    break; // done
  }
  rec = RECDone;
  return NULL;
}

LSError CallStackItemExitWith::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

    CHECK(SerializeGameCode(ar, "code", _code))
    CHECK(ar.Serialize("phase", (int &)_phase, 1))

    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemExitWith::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemExitWith::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemExitWith::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif

GameInstruction *CallStackItemWaitUntil::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  VMContext *context = state->GetVMContext();
  Assert(context);

  switch (_phase)
  {
  case PInit:
    {
      // start execution of statement - replace value on the top of stack (result)
      CallStackItem *level = new CallStackItemData(context->GetCallStack(), context->GetVariables(), context->_stack.Size() - 1, state, "WaitUntil condition", _condition, context->IsSerializationEnabled());
      context->AddLevel(level);

      _phase = PAfterCondition;
      rec = RECContinue;
      return NULL;
    }
  case PAfterCondition:
    // validate stack
    if (context->_stack.Size() == _stackBottom + 1)
    {
      GameValue value = context->_stack[context->_stack.Size() - 1];
      if (value.GetType() & GameBool)
      {
        // check condition result
        bool cond = value;
        if (cond)
        {
          rec = RECDone;
        }
        else
        {
          rec = RECYield;
          _phase = PInit;
        }
        return NULL;
      }
      else
      {
        state->TypeError(GameBool, value.GetType());
      }
    }
    else
    {
      state->SetError(EvalGen);
    }
    break;
  }
  rec = RECDone;
  return NULL;
}

LSError CallStackItemWaitUntil::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

    CHECK(SerializeGameCode(ar, "condition", _condition))
    CHECK(ar.Serialize("phase", (int &)_phase, 1))

    return LSOK;
}

#if SCRIPTVM_DEBUGGING

void CallStackItemWaitUntil::SetBreakpoint(const VMBreakpointInfo &breakpoint)
{
  // processed by the next level
}

void CallStackItemWaitUntil::RemoveBreakpoint(DWORD bp)
{
  // processed by the next level
}

void CallStackItemWaitUntil::EnableBreakpoint(DWORD bp, bool enable)
{
  // processed by the next level
}

#endif

static const EnumName NativeEventNames[]=
{
  EnumName(NEInitScript, "InitScript"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(NativeEvent dummy)
{
  return NativeEventNames;
}

GameInstruction *CallStackItemWaitUntilNative::GetNextInstruction(RequiredExecutionControl &rec, const GameState *state)
{
  //VMContext *context = state->GetVMContext();
  //Assert(context);

  bool CheckNativeEvent(NativeEvent condition);
  if (CheckNativeEvent(_condition))
  {
    rec = RECDone;
  }
  else
  {
    rec = RECYield;
  }
  return NULL;
}

LSError CallStackItemWaitUntilNative::Serialize(ParamArchive &ar)
{
  CHECK(CallStackItem::Serialize(ar))

  CHECK(ar.SerializeEnum("condition", _condition, 1, NEInitScript))

  return LSOK;
}


//////////////////////////////////////////////////////////////////////////
//
// Compilation
//

void GameState::CompileConstants(SourceDoc &src, SourceDocPos &pos, CompiledExpression &polishNotation) const
{
  // Skip possible spaces
  pos.SkipSpaces();

  SourceDocPos srcPos = pos; // position of identifier in source document

  char c = pos.GetChar();

  // -------------------------------------------------------------------------------
  // Unary operator (or just function - if you want)

  // Get the function
  const GameFunctions *functions = NULL;
  if (isalpha(c))
  {
    // Unary operator is a name
    RString name = pos.GetWord();
    name.Lower();
#if !_VBS3 //avoid overriding of functions by variables
    GameDataNamespace *globals = GetGlobalVariables();
    DoAssert(globals);
    if (!globals)
    {
      SetError(pos, EvalNamespace);
      return;
    }

    GameValue check;
    if (!globals->VarGet(name, check))
#endif
    {
      functions = &_functions[name];
    }
  }
  else
  {
    // Unary operator is 1 or 2 (special) characters
    char name[3];
    pos.GetChars(name, 2);
    if (name[0])
    {
      functions = &_functions[name];
      if (_functions.IsNull(*functions) && name[1])
      {
        name[1] = 0;
        functions = &_functions[name];
      }
    }
  }

  if (functions && _functions.NotNull(*functions))
  {
    pos.Skip(functions->_name.GetLength());

    // Compile the argument of the function
    CompileConstants(src, pos, polishNotation);

    // Write the function pointer
    polishNotation.Add(new GameInstructionFunction(srcPos, functions));
    return;
  }

  // -------------------------------------------------------------------------------
  // Variables

  if (isalphaext(c))
  {
    // Get the name of the variable
    RString name = pos.GetWord();
    pos.Skip(name.GetLength());

    // Lower the name
    name.Lower();

    // Write the variable name
    polishNotation.Add(new GameInstructionVariable(srcPos, name));
    return;
  }

  // -------------------------------------------------------------------------------
  // Parenthesis ()

  if (c == ')')
  {
    SetError(pos, EvalUnexpectedCloseB);
    return;
  }
  if (c == '(')
  {
    pos.Skip(1);
    const GameOperators *dummy = NULL;
    CompileBinaryOperator(src, pos, polishNotation, nula, dummy);
    pos.SkipSpaces();
    if (pos.GetChar() != ')')
    {
      SetError(pos, EvalCloseB);
      return;
    }
    pos.Skip(1);
    return;
  }

  // -------------------------------------------------------------------------------
  // Array []

  if (c == ']')
  {
    SetError(pos, EvalOpenBrackets);
    return;
  }
  if (c == '[')
  {
    int arraySize = 0;

    pos.Skip(1);
    pos.SkipSpaces();
    if (pos.GetChar() != ']')
    {
      while (true)
      {
        const GameOperators *dummy = NULL;
        CompileBinaryOperator(src, pos, polishNotation, nula, dummy);
        arraySize++;
        pos.SkipSpaces();
        if (pos.GetChar() != ',') break;
        pos.Skip(1);
      }
      if (pos.GetChar() != ']')
      {
        SetError(pos, EvalCloseBrackets);
        return;
      }
    }
    pos.Skip(1);

    // Write the array size
    polishNotation.Add(new GameInstructionArray(srcPos, arraySize));
    return;
  }

  // -------------------------------------------------------------------------------
  // String alias block of code {}

  if (c == CLOSE_STRING)
  {
    SetError(pos, EvalOpenBraces);
    return;
  }
  if (c == OPEN_STRING)
  {
    pos.Skip(1);

    // Write the string
    Ref<GameDataCode> code = new GameDataCode(this, src, pos, GetGlobalVariables());
    if (!code->IsCompiled())
    {
      SetError(pos, EvalHandled); // error was handled in the inner scope
      return;
    }
    GameValue value(code);
    polishNotation.Add(new GameInstructionConst(srcPos, value));
    return;
  }

  // -------------------------------------------------------------------------------
  // String in quotation marks "" (Two of them in consequence mean one inside the string)

  if (c == '"')
  {
    // string
    RString str;
    if (!pos.ReadString(str))
    {
      SetError(srcPos, EvalQuote);
      return;
    }

    // Write the string
    polishNotation.Add(new GameInstructionConst(srcPos, str));
    return;
  }

  // -------------------------------------------------------------------------------
  // String in single quotation marks '' (Two of them in consequence mean one inside the string)

  if (c == '\'')
  {
    // string
    RString str;
    if (!pos.ReadString(str))
    {
      SetError(srcPos, EvalSingleQuote);
      return;
    }

    // Write the string
    polishNotation.Add(new GameInstructionConst(srcPos, str));
    return;
  }

  // -------------------------------------------------------------------------------
  // Number

  float number;
  if (!pos.ReadNumber(number))
  {
    SetError(pos, EvalNum);
    return;
  }

  // Write the number
  polishNotation.Add(new GameInstructionConst(srcPos, number));
}

void GameState::CompileBinaryOperator(SourceDoc &src, SourceDocPos &pos, CompiledExpression &polishNotation, GamePriority priority, const GameOperators *&operators) const
{
  char c = pos.GetChar();

  // Detect an empty expression
  if (c == ';' || c == 0) return;

  // If there is no such a high priority then evaluate constants, parenthesis... and fill out the operator pointer
  if (priority >= gamePriorityCount)
  {
    // Evaluate constants
    CompileConstants(src, pos, polishNotation);

    // If there was an error in constant evaluation, then return immediately
    if (_e->_error != EvalOK) return;

    // Skip possible spaces
    pos.SkipSpaces();

    // Get the operator
    operators = NULL;
    if (isalphaext(pos.GetChar()))
    {
      // Unary operator is a name
      RString name = pos.GetWord();
      name.Lower();
      operators = &_operators[name];
    }
    else
    {
      // Unary operator is 1 or 2 (special) characters
      char name[3];
      pos.GetChars(name, 2);
      if (name[0])
      {
        operators = &_operators[name];
        if (_operators.IsNull(*operators) && name[1])
        {
          name[1] = 0;
          operators = &_operators[name];
        }
      }
    }
    // Return value of the constant
    return;
  }

  // Evaluate all operators of the same priority (it's also possible there are no such operators at all)
  CompileBinaryOperator(src, pos, polishNotation, GamePriority(priority + 1), operators);
  // If there was an error in evaluation, then return immediately
  if (_e->_error != EvalOK) return;

  // Read all operators of the same priority and add the right side
  while (operators && _operators.NotNull(*operators) && (operators->_priority == priority))
  {
    SourceDocPos srcPos = pos; // position of identifier in source document

    // Move the cursor behind the operator
    pos.Skip(operators->_name.GetLength());

    // Detect an empty expression
    char c = pos.GetChar();
    if (c == ';' || c == 0)
    {
      // ??? other error message
      SetError(pos, EvalNum);
      return;
    }

    // Evaluate the right side of the operator
    const GameOperators *newOperators = NULL;
    CompileBinaryOperator(src, pos, polishNotation, GamePriority(priority + 1), newOperators);
    // If there was an error in evaluation, then return immediately
    if (_e->_error != EvalOK) return;

    // Write the binary operator
    polishNotation.Add(new GameInstructionOperator(srcPos, operators));
    operators = newOperators;
  }
}

bool GameState::CompileAssignment(SourceDoc &src, SourceDocPos &pos, CompiledExpression &compiled)
{
  // name of variable
  RString name = pos.GetWord();
  if (name.GetLength() == 0)
  {
    SetError(pos, EvalBadVar);
    return false;
  }
  bool forceLocal;
  //test for word "local"
  if (stricmp(name,LOCAL_VAR_ASSIGN) == 0) {
      forceLocal = true;
  pos.Skip(name.GetLength());
      pos.SkipSpaces();
      if (pos.GetChar() == '_') //is it really local variable?
      {
        name = pos.GetWord(); 
        pos.Skip(name.GetLength());
        pos.SkipSpaces();
      }  else {
          forceLocal = false; //no, turn of force local and process default
      }

  } else {
    pos.Skip(name.GetLength());
  pos.SkipSpaces();
    forceLocal = false;
  }

  // =
  if (pos.GetChar() != '=')
  {
    SetError(pos, EvalEqu);
    return false;
  }
  // position of assignment in source text
  SourceDocPos srcPos = pos;
  pos.Skip(1);

  pos.SkipSpaces();

  name.Lower();

  // expression
  _e->_error = EvalOK;
  _e->_errorText = RString();
  const GameOperators *dummy = NULL;
  CompileBinaryOperator(src, pos, compiled, nula, dummy);
  if (_e->_error) return false;

  // add assignment to compiled expression
  compiled.Add(new GameInstructionAssignment(srcPos, name,forceLocal));
  return true;
}

void GameState::Compile(SourceDoc &src, SourceDocPos &pos, CompiledExpression &compiled, GameDataNamespace *globals) const
{
  if (!globals)
  {
    // check if default namespace is enabled
    if (_defaultGlobalsEnabled) globals = &GDefaultNamespace;
    else
    {
      ErrF("Global namespace not passed during: %s", cc_cast(src._content));
    }
  }

  GameDataNamespace *oldGlobals = SetGlobalVariables(globals);

  _e->_error = EvalOK;
  _e->_errorText = RString();

  compiled.Clear();

  if (pos.GetChar())
  {
    const GameOperators *dummy = NULL;
    CompileBinaryOperator(src, pos, compiled, nula, dummy);
    if (!_e->_checkOnly)
    {
      ShowError();
    }
  }

  // patch - avoid new expression on the end (which can clear the return value)
  while (compiled.Size() > 0)
  {
    int i = compiled.Size() - 1;
    if (compiled[i]->IsNewExpression()) compiled.Resize(i);
    else break;
  }

  compiled.Compact();

  SetGlobalVariables(oldGlobals);
}

void GameState::CompileMultiple(SourceDoc &src, SourceDocPos &pos, CompiledExpression &compiled, bool inner, GameDataNamespace *globals) const
{
  if (!globals)
  {
    // check if default namespace is enabled
    if (_defaultGlobalsEnabled) globals = &GDefaultNamespace;
    else
    {
      ErrF("Global namespace not passed during: %s", cc_cast(src._content));
    }
  }

  GameDataNamespace *oldGlobals = SetGlobalVariables(globals);

  _e->_error = EvalOK;
  _e->_errorText = RString();

  compiled.Clear();

  // command should be one or more assignments
  // scan for variable name (all before '=')
  while (pos.GetChar() != (inner ? CLOSE_STRING : 0))
  {
    pos.SkipSpaces();
    if (inner)
    {
      if (pos.GetChar() == CLOSE_STRING) break;
    }

    // add mark that new expression begins
    GameInstructionNewExpression *expression = new GameInstructionNewExpression(pos);
    compiled.Add(expression);
    int beg = pos._pos;

    // check if expression is assignment
    // assignment must start with identifier
    if (CheckAssignment(cc_cast(src._content) + pos._pos))
    {
      if (!const_cast<GameState *>(this)->CompileAssignment(src, pos, compiled))
      {
        break;
      }
    }
    else // non assignment
    {
      // check for empty command
      const GameOperators *dummy = NULL;
      CompileBinaryOperator(src, pos, compiled, nula, dummy);
      if (_e->_error) break;
    } // single command executed

    // mark what part of the source corresponds to the expression
    expression->SetSourceRange(beg,pos._pos);
    pos.SkipSpaces();

    char c = pos.GetChar();
    if (inner)
    {
      if (c == CLOSE_STRING) break;
    }
    else
    {
      if (c == 0) break;
    }
    if (c != ';' && (c != ',' || _e->_checkOnly))
    {
      SetError(pos, EvalSemicolon);
      break;
    }
    pos.Skip(1);
  }
  if (inner)
  {
    // skip trailing '}'
    if (pos.GetChar() == CLOSE_STRING)
    {
      pos.Skip(1);
    }
  }

  if (!_e->_checkOnly)
  {
    ShowError();
  }

  // patch - avoid new expression on the end (which can clear the return value)
  while (compiled.Size() > 0)
  {
    int i = compiled.Size() - 1;
    if (compiled[i]->IsNewExpression()) compiled.Resize(i);
    else break;
  }

  compiled.Compact();

  SetGlobalVariables(oldGlobals);
}

//////////////////////////////////////////////////////////////////////////
//
// Evaluation
//

static RString ShortLine(RString longLine)
{
  // no line break 
  // avoid too long lines
  if (const char *lf = strchr(longLine,'\n'))
  {
    longLine = RString(longLine,lf-longLine)+"...";
  }

  const int maxLine = 40;
  if (longLine.GetLength()>maxLine)
  {
    longLine = RString(longLine,maxLine);
  }
  return longLine;
}

static RString ExpEval = "Expression evaluation";

GameValue GameState::Evaluate(RString expression, const CompiledExpression &compiled, const EvalContext &ctx, GameDataNamespace *globals, RString expressionName) const
{
  PROFILE_SCOPE_EX(gsEva,eval);
  if (PROFILE_SCOPE_NAME(gsEva).IsActive())
  {
    PROFILE_SCOPE_NAME(gsEva).AddMoreInfo(ShortLine(expression));
  }
  
  if (expression.IsEmpty() && !compiled.Size())
    return GameValue(true); //not only optimization. We want to avoid EvaluateCore call with an empty expression

  if (!globals)
  {
    // check if default namespace is enabled
    if (_defaultGlobalsEnabled) globals = &GDefaultNamespace;
    else
    {
      ErrF("Global namespace not passed during: %s", cc_cast(expression));
    }
  }

  _e->_error = EvalOK;
  _e->_errorText = RString();

  // create evaluation context
  VMContext context(false);
  context._doc._filename = expressionName;
  context._doc._content = expression;
  CallStackItem *level = new CallStackItemSimple(globals, NULL, GetContext(), 0, this, ExpEval, context._doc, compiled, context.IsSerializationEnabled()); // root
  context.Clear();
  context.AddLevel(level);
  context._localOnly = ctx._localOnly;
  context._undefinedIsNil = ctx._undefinedIsNil;

  GameDataNamespace *oldGlobals = SetGlobalVariables(globals);

  // evaluate
  VMContext *oldContext = SetVMContext(&context);
  bool done = context.EvaluateCore(*this);
  if (!_e->_checkOnly) ShowError(); // error position is given in context of whole document
  SetVMContext(oldContext);

  SetGlobalVariables(oldGlobals);

  if (_e->_error != EvalOK) return NOTHING;

  if (done)
  {
    // If there is not 0 or 1 item on the stack left, then report error
    if (context._stack.Size() > 1)
    {
      // position of the last statement
      VMContext *oldContext = SetVMContext(&context);
      SetError(context._lastInstruction, EvalGen);
      if (!_e->_checkOnly) ShowError(); // error position is given in context of whole document
      SetVMContext(oldContext);
      return NOTHING;
    }

    if (context._stack.Size() == 0) return NOTHING;
    return context._stack[0];
  }
  else
  {
    // not finished
    return NOTHING;
  }
}

bool GameState::EvaluateBool(RString expression, const CompiledExpression &compiled, const EvalContext &ctx, GameDataNamespace *globals, RString expressionName) const
{
  GameValue value = Evaluate(expression, compiled, ctx, globals, expressionName);
  if (!(value.GetType() & GameBool))
  {
    if (_e->_checkOnly)
    {
      TypeError(GameBool, value.GetType());
    }
    return false;
  }
  return bool(value);
}

void GameState::Execute(RString expression, const CompiledExpression &compiled, const EvalContext &ctx, GameDataNamespace *globals, RString expressionName)
{
  GameValue value = Evaluate(expression, compiled, ctx, globals, expressionName);
  if (!(value.GetType() & GameNothing) && !((value.GetType() & GameBool) && value))
  {
    if (_e->_checkOnly)
    {
      TypeError(GameNothing, value.GetType());
    }
  }
}

CallStackItem *GameState::CreateCallStackItem(RString typeName, int parentIndex, bool enableSerialization)
{
  for (int i=0; i<_callStackItems.Size(); i++)
  {
    if (typeName == _callStackItems[i].typeName)
    {
      GameVarSpace *vars = NULL;
      CallStackItem *stack = NULL;
      if (parentIndex >= 0)
      {
        VMContext *ctx = GetVMContext();
        Assert(ctx);
        vars = ctx->GetVariables(parentIndex);
        stack = ctx->GetCallStack(parentIndex);
      }
      return (*_callStackItems[i].createFunction)(stack, vars, this, enableSerialization);
    }
  }
  RptF("Unknown call stack item type: %s", cc_cast(typeName));
  return NULL;
}

#endif // USE_PRECOMPILATION
