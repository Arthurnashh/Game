#include <El/elementpch.hpp>

#include "express.hpp"

#if !USE_PRECOMPILATION

void GameState::vynech() const
{
  while (isspace(*_e->_pos)) _e->_pos++;
  //while( (*_e->_pos==' ' || *_e->_pos=='\t') ) _e->_pos++;
}

Real GameState::sejmid() const
{

  //const char *Po1;
  //const char *Po2;
  //const struct lconv *Tecka;
  char *end;
  Real cisl = strtod(_e->_pos, &end);
  if (end == _e->_pos)
  {
    SetError(EvalNum);
    return 0;
  }
  _e->_pos = end;
  return cisl;
}

/*!
\patch 1.43 Date 1/31/2002 by Ondra
- New: Scripting: Two quote marks encountered inside of string are parsed as quote mark.
\patch 1.82 Date 8/12/2002 by Ondra
- New: Scripting: Curled braces { and } can be used to enclose string constants.
*/

GameValue GameState::Const() const
{
  //GameValue vrt;
  vynech();
  //if( isalpha(_e->_pos[0]) && !isalpha(_e->_pos[1])) /* promenna */
  if( isalphaext(_e->_pos[0]) ) // variable
  {
    // scan variable name
    char name[MAX_EXPR_LEN];
    char *wName=name;
    int i = 0;
    while( isalnumext(_e->_pos[0]) )
    {
      if (i<MAX_EXPR_LEN-1) *wName++=*_e->_pos, i++;
      _e->_pos++;
    }
    *wName=0;
    strlwr(name);
    // scan nular operators 
    if( *name=='_' )
    {
      // local variable
      GameVarSpace *space=_e->local;
      if( !space )
      {
        SetError(EvalNamespace);
        return GameValue();
      }
      GameValue value;
      if (space->VarGet(name,value)==false)
      {
        // local variables are always reported when missing
        SetError(EvalVar, cc_cast(name));
      }
      return value;
    }
    GameDataNamespace *globals = GetGlobalVariables();
    GameValue check;
    if (_e->_checkOnly || !globals || !globals->VarGet(name, check))
    {
      const GameNular &o = _nulars[name];
      if (_nulars.NotNull(o))
      {
        if( _e->_checkOnly )
        {
          return GameValue(new GameDataNil(o._operator->GetRetType()));
        }
        else
        {
          return o._operator->Evaluate(this);
        }
      }
    }

    if (!VarGoodName(name))
    {
      SetError(EvalBadVar);
      return GameValue(); 
    }
    if( _e->_checkOnly )
    {
      return CreateGameValue(GameVoid);
    }
    return VarGet(name, false, globals);
  }
  else if (*_e->_pos=='"')
  {
    // string
    const char *start=++_e->_pos;
    RString string = "";
    for(;;)
    {
      while( *_e->_pos!='"' )
      {
        if( !*_e->_pos )
        {
          SetError(EvalQuote);
          return RString();
        }
        _e->_pos++;
      }
      // string from start to _e->_pos
      string = string + RString(start,_e->_pos-start);
      // check if another quote mark is encountered
      // if yes, append it, and continue parsing, otherwise we are done
      ++_e->_pos;
      if (*_e->_pos=='"')
      {
        string = string + RString("\"");
        ++_e->_pos;
        start = _e->_pos;
      }
      else
      {
        break;
      }
    }

    return string;
  }
  else if (*_e->_pos=='\'')
  {
    // string
    const char *start=++_e->_pos;
    RString string = "";
    for(;;)
    {
      while( *_e->_pos!='\'' )
      {
        if( !*_e->_pos )
        {
          SetError(EvalSingleQuote);
          return RString();
        }
        _e->_pos++;
      }
      // string from start to _e->_pos
      string = string + RString(start,_e->_pos-start);
      // check if another single quote mark is encountered
      // if yes, append it, and continue parsing, otherwise we are done
      ++_e->_pos;
      if (*_e->_pos=='\'')
      {
        string = string + RString("'");
        ++_e->_pos;
        start = _e->_pos;
      }
      else
      {
        break;
      }
    }

    return string;
  }
  else if (_e->_pos[0]==OPEN_STRING)
  {
    // special case of string value
    const char *start = ++_e->_pos;
    bool closed = false;
    const char *end = start;
    int level = 1;
    for(;;)
    {
      // note: any quotes inside may prevent closing bracket
      if (*end=='"')
      {
        end++;
        // skip anything until another quote mark
        while (*end!=0 && *end!='"')
        {
          end++;
        }
        if (*end!='"')
        {
          SetError(EvalCloseBraces);
          return RString();
        }
      }
      else if (*end=='\'')
      {
        end++;
        // skip anything until another single quote mark
        while (*end!=0 && *end!='\'')
        {
          end++;
        }
        if (*end!='\'')
        {
          SetError(EvalCloseBraces);
          return RString();
        }
      }
      else if (*end==OPEN_STRING)
      {
        level++;
      }
      else if (*end==CLOSE_STRING)
      {
        if (--level==0)
        {
          closed = true;
          break;
        }
      }
      else if (*end==0) break;
      end++;
    }
    if (closed)
    {
      _e->_pos = end+1;
      return RString(start,end-start);
    }
    else
    {
      SetError(EvalCloseBraces);
      return RString();
    }
  }
  else if( _e->_pos[0]=='0' && _e->_pos[1]=='x')
  {
    bool Fg=false;
    int vrt=0;
    _e->_pos += 2;
    while( isxdigit(*_e->_pos) )
    {
      Fg=true;
      vrt*=16;
      if( isdigit(*_e->_pos) ) vrt+=*_e->_pos-'0';
      else vrt+=toupper(*_e->_pos)-('A'-10);
      _e->_pos++;
    }
    if( !Fg ) SetError(EvalNum);
    return float(vrt);
  }
  else if( *_e->_pos=='$' ) /* hex */
  {
    bool Fg=false;
    int vrt=0;
    _e->_pos++;
    while( isxdigit(*_e->_pos) )
    {
      Fg=true;
      vrt*=16;
      if( isdigit(*_e->_pos) ) vrt+=*_e->_pos-'0';
      else vrt+=toupper(*_e->_pos)-('A'-10);
      _e->_pos++;
    }
    if( !Fg ) SetError(EvalNum);
    return float(vrt);
  }
  else return sejmid(); 
}

static const RString BinOp="<bin>";
static const RString LstOp="<lst>";

/*!
\patch 1.50 Date 4/16/2002 by Ondra
- New: Scripting language hiding rules changed.
User defined variables now hide functions with identical name.
This ensures old scripts / conditions will certainly work
even when new functions conflict with names of existing variables.
*/

void GameState::VyhCast( int Prio ) const
{
  while( _e->SP>0 )
  {
    RString oper1=_e->_operStack[_e->SP-1];
    if( oper1==BinOp )
    {
      if( _e->SP<=1 ) return;
      RString oper2=_e->_operStack[_e->SP-2];
      if( oper2==BinOp || oper2==LstOp ) return;
      if( Prio>_e->_priorStack[_e->SP-2] ) return;
      
      GameDataNamespace *globals = GetGlobalVariables();

      // binary
      Assert( _e->UB[_e->SP-1]==-1 );
      if( _e->UB[_e->SP-2]==2 )
      {
        // find function with appropriate name

        if( !_e->_checkOnly )
        {
          const GameOperator *op = NULL;

          GameValue check;
          if (!globals || !globals->VarGet(oper2, check))
          {
            const GameOperators &operators = _operators[oper2];
            if (!_operators.IsNull(operators))
            {
              for (int i=0; i<operators.Size(); i++)
              {
                const GameOperator &o = operators[i];
                if(!(o._operator->GetArg1Type() & _e->_stack[_e->SP - 2].GetType())) continue;
                if(!(o._operator->GetArg2Type() & _e->_stack[_e->SP - 1].GetType())) continue;
                op = &o;
                break;
              }
            }
          }

          if (!op)
          {
            TypeErrorOperator
              (
              oper2,
              _e->_stack[_e->SP-2].GetType(),
              _e->_stack[_e->SP-1].GetType()
              );
            return;
          }

          if( _e->_stack[_e->SP-2].GetNil() || _e->_stack[_e->SP-1].GetNil() )
          {
            //LogF("Nil in %s",(const char *)op->_name);
            _e->_stack[_e->SP-2]=GameValue( new GameDataNil(op->_operator->GetRetType()) );
          }
          else
          {
            _e->_stack[_e->SP-2]=op->_operator->Evaluate(this, _e->_stack[_e->SP-2], _e->_stack[_e->SP-1]);
            if (_e->_error) return;
          }
        }
        else
        {

          GameType possible;
          const GameOperators &operators = _operators[oper2];
          if (!_operators.IsNull(operators))
          {
            for (int i=0; i<operators.Size(); i++)
            {
              const GameOperator &o = operators[i];
              if(!(o._operator->GetArg1Type() & _e->_stack[_e->SP - 2].GetType())) continue;
              if(!(o._operator->GetArg2Type() & _e->_stack[_e->SP - 1].GetType())) continue;
              possible |= o._operator->GetRetType();
            }
          }

          if (!possible)
          {
            TypeErrorOperator
              (
              oper2,
              _e->_stack[_e->SP-2].GetType(),
              _e->_stack[_e->SP-1].GetType()
              );
            return;
          }

          _e->_stack[_e->SP-2]=GameValue(new GameDataNil(possible));
        }
      }
      else
      {
        Assert( _e->UB[_e->SP-2]==1 );
        if( !_e->_checkOnly )
        {
          const GameFunction *op = NULL;

          GameValue check;
          if (!globals || !globals->VarGet(oper2, check))
          {
            const GameFunctions &functions = _functions[oper2];
            if (!_functions.IsNull(functions))
            {
              for (int i=0; i<functions.Size(); i++)
              {
                const GameFunction &f = functions[i];
                if (f._operator->GetArgType() & _e->_stack[_e->SP-1].GetType())
                {
                  op = &f;
                  break;
                }
              }
            }
          }

          if (!op)
          {
            TypeErrorFunction
              (
              oper2,
              _e->_stack[_e->SP-1].GetType()
              );
            return;
          }

          if( _e->_stack[_e->SP-1].GetNil() )
          {
            //LogF("Nil in %s",(const char *)op->_name);
            _e->_stack[_e->SP-2]=GameValue( new GameDataNil(op->_operator->GetRetType()) );
          }
          else
          {
            _e->_stack[_e->SP-2]=op->_operator->Evaluate(this, _e->_stack[_e->SP-1]);
            if (_e->_error) return;
          }
        }
        else
        {
          GameType possible;
          const GameFunctions &functions = _functions[oper2];
          if (!_functions.IsNull(functions))
          {
            for (int i=0; i<functions.Size(); i++)
            {
              const GameFunction &f = functions[i];
              if (f._operator->GetArgType() & _e->_stack[_e->SP-1].GetType())
                possible |= f._operator->GetRetType();
            }
          }

          if (!possible)
          {
            TypeErrorFunction
              (
              oper2,
              _e->_stack[_e->SP-1].GetType()
              );
            return;
          }
          _e->_stack[_e->SP-2]=GameValue(new GameDataNil(possible));
        }
      }
      _e->_operStack[_e->SP-2]=BinOp;
      _e->_priorStack[_e->SP-2]=_e->_parPrior+_e->_listPrior;
      _e->UB[_e->SP-2]=-1;
      // remove argument
      _e->_stack[_e->SP-1]=GameValue();
      _e->SP--;
    }
    else
    {
      Assert( oper1==LstOp ); 
      // list
      return;
    }
  }
  if( _e->_error ) return;
}

void GameState::CleanStack() const
{
  // remove all values from the stack
  while (_e->SP>0)
  {
    _e->_stack[--_e->SP] = GameValue();
  }
}

/*!
\patch 1.43 Date 1/31/2002 by Ondra
- Fixed: Scripting: Array parsing inside of brackets was wrong.
Example of wrong expression: (+[0])
*/

GameValue GameState::Vyhod() const
{
  _e->_error = EvalOK;
  _e->_errorText = RString();

  _e->_parPrior=0;
  _e->_listPrior=0;
  _e->SP=0;

  const GameFunctions *functions;
  const GameOperators *operators;
  if (*_e->_pos==';' || *_e->_pos==0)
  {
    // detect empty expression
    return NOTHING;
  }
  for(;;)
  {
    vynech();
    if( *_e->_pos=='(' )
    {
      _e->_pos++;
      _e->_parPrior+=zavorky;
      continue;
    }
    if( *_e->_pos=='[' )
    {
      // evaluate list - no recursion?      
      _e->_pos++;
      vynech();
      if( *_e->_pos==']' )
      {
        // empty list construction
        _e->_pos++;

        if (_e->SP>=evalStackLevels)
        {
          SetError(EvalLineLong);
          return NOTHING;
        }
        _e->_stack[_e->SP]=GameArrayType();
        _e->_priorStack[_e->SP]=-1;
        _e->UB[_e->SP]=-1;
        _e->_operStack[_e->SP]=BinOp;
        _e->SP++;

        goto CekejBinar;
      }
      else
      {
        _e->_listPrior+=zavorky;
      }
      continue;
    }
    // check unary operator - 
    functions = NULL;
    if( isalpha(*_e->_pos) )
    {
      // scan identifier
      int idlen=0;
      while( isalnumext(_e->_pos[idlen]) ) idlen++;

      RString name(_e->_pos, idlen);
      name.Lower();

      GameDataNamespace *globals = GetGlobalVariables();

      GameValue check;
      if (_e->_checkOnly || !globals || !globals->VarGet(name, check))
      {
        functions = &_functions[name];
      }
    }
    else
    {
      RString name2(_e->_pos, 2);
      functions = &_functions[name2];
      if (_functions.IsNull(*functions))
      {
        RString name1(_e->_pos, 1);
        functions = &_functions[name1];
      }
    }
    if (!functions || _functions.IsNull(*functions))
    {
      if (_e->SP>=evalStackLevels)
      {
        SetError(EvalLineLong);
        return NOTHING;
      }
      // value on stack top
      _e->_stack[_e->SP]=Const();
      _e->_priorStack[_e->SP]=-1;
      _e->UB[_e->SP]=-1;
      _e->_operStack[_e->SP]=BinOp;
      _e->SP++;

      if( _e->_error ) return NOTHING;
CekejBinar:
      vynech();
      switch( *_e->_pos )
      {
      case ',':
        {
          if (_e->_listPrior<=0)
          {
            if (_e->_checkOnly)
            {
              SetError(EvalOpenB);
              return 0.0f;
            }
            goto Semicolon;
          }
          _e->_pos++;

          VyhCast( _e->_parPrior+_e->_listPrior );

          if( _e->_error ) return NOTHING;
          // leave value on stack and continue with evaluation
          _e->_operStack[_e->SP-1]=LstOp; // no op
          _e->UB[_e->SP-1]=3; // list element
          _e->_priorStack[_e->SP-1]=_e->_parPrior+_e->_listPrior;

          break;
        }
      case ']':
        {
          _e->_pos++;

          VyhCast( _e->_parPrior+_e->_listPrior );
          if( _e->_error ) return NOTHING;

          // change value to list value
          _e->_operStack[_e->SP-1]=LstOp; // no op
          _e->UB[_e->SP-1]=3; // list element
          _e->_priorStack[_e->SP-1]=_e->_parPrior+_e->_listPrior;
          // leave value on stack and continue with evaluation

          _e->_listPrior-=zavorky;
          if( _e->_listPrior<0 )
          {
            SetError(EvalOpenBrackets);
            return 0.0f;
          }
          // pop all arguments with prior>=_listPrior
          GameValue arrayVal = CreateGameValue(GameArray);
          GameArrayType &array = arrayVal;

          int SPP=_e->SP;
          while
            (
            SPP>0 &&
            _e->_priorStack[SPP-1]>=_e->_listPrior+_e->_parPrior+zavorky
            )
          {
            Assert( _e->_operStack[SPP-1]==LstOp );
            Assert( _e->UB[SPP-1]==3 );
            SPP--;
          }
          array.Realloc(_e->SP-SPP);
          //array.Resize(_e->SP-SPP);
          //LogF("Construct array");
          for( int i=SPP; i<_e->SP; i++ )
          {
            //LogF("  %s",(const char *)(_e->_stack[i].GetText()));
            array.Add(_e->_stack[i]);
          }
          _e->SP=SPP;
          if (_e->SP>=evalStackLevels)
          {
            SetError(EvalLineLong);
            return NOTHING;
          }
          // push array value
          _e->_stack[_e->SP]=arrayVal;
          _e->_priorStack[_e->SP]=-1;
          _e->UB[_e->SP]=-1;
          _e->_operStack[_e->SP]=BinOp;
          _e->SP++;
          goto CekejBinar;
        }
      case ')':
        _e->_pos++;
        _e->_parPrior-=zavorky;
        if( _e->_parPrior<0 )
        {
          SetError(EvalOpenB);
          return 0.0f;
        }
        goto CekejBinar;
      case ';':
Semicolon:
        {
          if( _e->_parPrior!=0 )
          {
            SetError(EvalCloseB);
            return 0.0f;
          } 
          VyhCast( 0 );
          if( _e->_error ) return NOTHING;

          Assert( _e->SP==1 );
          Assert( _e->UB[_e->SP-1]==-1 );
          Assert( _e->_operStack[_e->SP-1]==BinOp );
          GameValue value = _e->_stack[--_e->SP];
          _e->_stack[_e->SP] = GameValue();
          return value;
        }
      case 0:
        {
          if( _e->_parPrior!=0 )
          {
            SetError(EvalCloseB);
            return 0.0f;
          } 
          if( _e->_listPrior!=0 )
          {
            SetError(EvalCloseBrackets);
            return 0.0f;
          } 
          VyhCast( 0 );
          if( _e->_error ) return NOTHING;

          //LogF("Expression _e->SP %d",_e->SP);
          Assert( _e->SP==1 );
          GameValue value = _e->_stack[--_e->SP];
          _e->_stack[_e->SP] = GameValue();
          return value;
        }
      default:
        {
          // if first letter is alphaext, scan whole identifier
          int idlen=0;
          operators = NULL;
          if (isalphaext(_e->_pos[0]))
          {
            while( isalnumext(_e->_pos[idlen]) ) idlen++;

            RString name(_e->_pos, idlen);
            name.Lower();

            GameDataNamespace *globals = GetGlobalVariables();

            GameValue check;
            if (_e->_checkOnly || !globals || !globals->VarGet(name, check))
            {
              operators = &_operators[name];
            }
          }
          else
          {
            RString name2(_e->_pos, 2);
            operators = &_operators[name2];
            if (_operators.IsNull(*operators))
            {
              RString name1(_e->_pos, 1);
              operators = &_operators[name1];
            }
          }

          if (!operators || _operators.IsNull(*operators))
          {
            char name[256];
            strncpy(name,_e->_pos,idlen);
            name[idlen] = 0;
            SetError(EvalOper,name);
            return NOTHING;
          }   
          _e->_pos+=strlen(operators->_name);
          int prior=operators->_priority+_e->_parPrior+_e->_listPrior;
          VyhCast( prior );
          if( _e->_error ) return NOTHING;
          _e->_operStack[_e->SP-1]=operators->_name;
          _e->UB[_e->SP-1]=2;
          _e->_priorStack[_e->SP-1]=prior;
          break;
        }
      } // switch
    } // binary or special
    else // unary operator
    {
      _e->_pos+=strlen(functions->_name);
      if (_e->SP>=evalStackLevels)
      {
        SetError(EvalLineLong);
        return NOTHING;
      }
      _e->_stack[_e->SP]=0.0f; // no first argument
      _e->_operStack[_e->SP]=functions->_name;
      _e->UB[_e->SP]=1;
      _e->_priorStack[_e->SP]=unar+_e->_parPrior+_e->_listPrior;
      _e->SP++;
      // save unary operator
    }
  } // while 
  /*NOTREACHED*/
}

bool GameState::PerformAssignment(bool localOnly)
{
  char name[MAX_EXPR_LEN];
  char *wName=name;
  bool forceLocal; 
  {
      const char *nx = TestLocalAssign(_e->_pos);
      forceLocal = nx != _e->_pos;
      _e->_pos = nx;
  }
  
  if( isalphaext(*_e->_pos) )
  {
    int i=0;
    while( isalnumext(*_e->_pos) )
    {
      if (i<MAX_EXPR_LEN-1)
      {
        *wName++=*_e->_pos,i++;
      }
      _e->_pos++;
    }
  }
  *wName=0;
  if( !*name )
  {
    SetError(EvalBadVar);
    return false;
  }
  vynech();
  if( *_e->_pos!='=' )
  {
    SetError(EvalEqu);
    return false;
  }

  strlwr(name);
  if (!LValueGoodName(name, GetGlobalVariables()))
  {
    SetError(EvalBadVar);
    return false;
  }

  _e->_pos++;
  vynech();


  GameValue value=Vyhod();
  CleanStack();
  if( _e->_error ) return false;
  if (_e->_checkOnly)
  {
    if( value.GetType()==GameNothing )
    {
      TypeError(GameAny,value.GetType());
      return false;
    }
    if (IsFakeType(value.GetType()))
    {
      TypeError(GameAny,value.GetType());
    }
  }
  if( !_e->_checkOnly )
  {
    if( value.SharedInstance() && value.GetType()!=GameArray )
    {
      //LogF("Shared data (variable?) duplicated: '%s'",expression);
      value.Duplicate(value);
    }
    if (localOnly || forceLocal)
    {
      VarSetLocal(name,value);
    }
    {
      VarSet(name, value, false, false, GetGlobalVariables());
    }
  }
  return true;
}

int GameState::FindContext(const char *scopeName) const
{
  for (int i=_contextStack.Size()-1;i>=0;i--)
  {
    if (_contextStack[i].NotNull() && _contextStack[i]->local && _contextStack[i]->local->_scopeName==scopeName)
    {
      return i;
    }
  }
  return -1;
}

#endif // !USE_PRECOMPILATION
