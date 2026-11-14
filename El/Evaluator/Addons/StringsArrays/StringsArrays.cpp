#include <el/elementpch.hpp>
#include "StringsArrays.h"
#include <malloc.h>
#include <strstream>
#include <el\pathname\pathname.h>
#include <Es/Algorithms/qsort.hpp>
using namespace std;

#define Category "StringsArrays"






static GameValue ListSelect( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  const GameArrayType &array=oper1;
  float index = oper2;
  int sel = toInt(index);
  if (sel<0) sel=array.Size()+sel;
  if( sel<0 || sel>=array.Size() )
  {
    return GameValue();
  }
  return array[sel];
}

static GameValue StringSelect( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  RString str=oper1;
  float index = oper2;
  int sel = toInt(index);
  if (sel<0) sel=str.GetLength()+sel;
  if( sel<0 || sel>=str.GetLength() )
  {
    return RString();
  }
  else
  {
    char buff[2];
    buff[0]=str[sel];
    buff[1]=0;
    return RString(buff);
  }
}

static GameValue ListRangeSelect( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  const GameArrayType &array=oper1;
  const GameArrayType &range=oper2;
  int i;
  if (range.Size()<1 || range.Size()>2)
  {
    state->SetError(EvalType);
    return GameValue();
  }
  for (i=0;i<range.Size();i++) if (range[i].GetType()!=GameScalar) 
  {
    state->SetError(EvalType);
    return GameValue();
  }
  int sel1=toInt(range[0]);  
  int sel2=range.Size()==1?array.Size():toInt(range[1]);
  if (sel1>=0)
  {
    if (sel1>=array.Size()) return GameValue();
    if (sel2<0) sel2=sel1-sel2;
    if (sel2>array.Size()) sel2=array.Size();
  }
  else
  {
    sel1=array.Size()+sel1;
    if (sel1<0) sel1=0;
    if (sel2<0) sel2=sel1-sel2;
    if (sel2>array.Size()) sel2=array.Size();
    if (sel2<sel1)
    {
      sel1=sel1+sel2; // a+b  b  prehozeni
      sel2=sel1-sel2; // a+b  a
      sel1=sel1-sel2; // b    a
    }
  }
  GameArrayType res;
  for (i=sel1;i<sel2;i++)
  {
    res.Append()=array[i];
  }
  return res;
}

static GameValue StringRangeSelect( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  RString array=oper1;
  const GameArrayType &range=oper2;
  int i;
  if (range.Size()<1 || range.Size()>2)
  {
    state->SetError(EvalType);
    return GameValue();
  }
  for (i=0;i<range.Size();i++) if (range[i].GetType()!=GameScalar) 
  {
    state->SetError(EvalType);
    return GameValue();
  }
  int sel1=toInt(range[0]);  
  int sel2=range.Size()==1?array.GetLength():toInt(range[1]);
  if (sel1>=0)
  {
    if (sel1>=array.GetLength()) return GameValue();
    if (sel2<0) sel2=sel1-sel2;
    if (sel2>array.GetLength()) sel2=array.GetLength();
  }
  else
  {
    sel1=array.GetLength()+sel1;
    if (sel1<0) sel1=0;
    if (sel2<0) sel2=sel1-sel2;
    if (sel2>array.GetLength()) sel2=array.GetLength();
    if (sel2<sel1)
    {
      sel1=sel1+sel2; // a+b  b  prehozeni
      sel2=sel1-sel2; // a+b  a
      sel1=sel1-sel2; // b    a
    }
  }
  RString res;
  int sz=sel2-sel1;
  if (sz>0)
  {
    char *buff=(char *)(sz>65535?malloc(sz+1):alloca(sz+1));
    strncpy(buff,(const char *)array+sel1,sz);
    buff[sz]=0;
    res=buff;
    if (sz>65535) free(buff);
  }
  return res;
}

static GameValue lenOfArray( const GameState *state, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  return (float)(arr.Size());
}

static GameValue lenOfString( const GameState *state, GameValuePar oper1)
{
  RString sstr=oper1;
  return (float)(sstr.GetLength());
}

static GameValue CEsc( const GameState *state, GameValuePar oper1)
{
  RString str=oper1;
  istrstream input((char *)(str.Data()),str.GetLength());
  ostrstream output;
  int i=input.get();
  while (i!=EOF)
  {
    if (i=='\\')
    {
      i=input.get();
      switch (i)
      {
      case 't': i='\t';break;
      case 'n': i='\n';break;
      case 'r': i='\r';break;
      case 'x': 
        {
          int val=0;
          i=input.get();
          while (i>='0' && i<='9' || i>='a' && i<='f' || i>='A' && i<='F')
          {
            val*=16;
            if (i>='0' && i<='9') val+=i-'0';
            else if (i>='a' && i<='f') val+=i-'a'+10;
            else if (i>='A' && i<='F') val+=i-'A'+10;
            i=input.get();
          }
          input.unget();
          i=val & 0xFF;
        }
        break;
      case '0': 
        {
          int val=0;
          i=input.get();
          while (i>='0' && i<='7') {val*=8+(i-'0'); i=input.get();}
          input.unget();
          i=val & 0xFF;
        }
       break;
      }      
    } 
   output.put((char)i);  
   i=input.get();
  }
output.put(0);
str=output.str();
output.freeze(false);
return str;
}

static GameValue FindString( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  RString tofind=oper1;
  RString whatfind=oper2;
  const char *res=strstr(tofind,whatfind);
  if (res==NULL) return GameValue();
  else
    return (float)(res-tofind.Data());
}

static GameValue FindStringI( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  RString tofind=oper1;
  RString whatfind=oper2;
  tofind.Lower();
  whatfind.Lower();
  const char *res=strstr(tofind,whatfind);
  if (res==NULL) return GameValue();
  else
    return (float)(res-tofind.Data());
}

static GameValue chrFunct( const GameState *state, GameValuePar oper1)
{
  int idx=toInt(oper1);
  char buff[2]={(char)idx,0};
  return RString(buff);
}

static GameValue ascFunct( const GameState *state, GameValuePar oper1)
{
  RString str=oper1;
  if (str.GetLength()==0) return GameValue();
  return (float)(str[0]);
}

static GameValue splitPath( const GameState *state, GameValuePar oper1)
{
  RString str=oper1;
  Pathname path=str;
  GameArrayType arr;
  char buff[]="@:\\";
  buff[0]=path.GetDrive();
  arr.Append()=RString(buff);
  arr.Append()=RString(path.GetDirectory());
  arr.Append()=RString(path.GetTitle());
  arr.Append()=RString(path.GetExtension());
  return arr;
}

static GameValue valFunct( const GameState *state, GameValuePar oper1)
{
  RString str=oper1;
  float f=0;
  sscanf(str,"%f",&f);
  return f;
}

static GameValue hexFunct( const GameState *state, GameValuePar oper1)
{
  RString str=oper1;
  unsigned long res;
  sscanf(str,"%X",&res);
  return (float)res;
}

static GameValue tohexFunct( const GameState *state, GameValuePar oper1)
{
  float f=oper1;
  unsigned long hex=(unsigned long)::toLargeInt(f);
  char buff[50];
  sprintf(buff,"%X",hex);
  return RString(buff);
}

static GameValue tolower( const GameState *state, GameValuePar oper1)
{
  RString a=oper1;
  a.Lower();
  return a;
}

static GameValue toupper( const GameState *state, GameValuePar oper1)
{
  RString a=oper1;
  a.Upper();
  return a;
}

class CompareExp
{
protected:
  const GameState& _state;
  RString _expression;
  mutable bool _exited; // if expresion exited, true. 
public:
  CompareExp(const GameState& state, RString& expression) : _state(state), _expression(expression), _exited(false)  {};
  int operator () (const GameValue *a, const GameValue *b) const
  {
    if (_exited)
      return 0;

    GameVarSpace local(_state.GetContext());
    _state.BeginContext(&local);
       
    // set local varible - will be deleted on EndContext
    _state.VarSetLocal("_x",*a,true);
    _state.VarSetLocal("_y",*b,true);
    GameScalarType ret = _state.EvaluateMultiple(_expression);

#if USE_PRECOMPILATION
    bool exit=_state.GetVMContext()->_breakOut;
#else
    bool exit=local._blockExit;
#endif
    
    if( _state.GetLastError() || exit)
    {
      _exited = true;
      _state.EndContext();
      return 0;
    }       

    _state.EndContext();
    return toInt(ret);
  }
};

static GameValue SortBy( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{  
  RString expression=oper2;
  GameArrayType &array=(oper1.GetData())->GetArray();  

  CompareExp comp(*state,expression);
  QSort<GameValue, CompareExp>(array.Data(), array.Size(), comp);

  return GameValue(array);
}

// Compare strings... 
static GameValue StringCmp( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  RString str1=oper1;
  RString str2=oper2;

  return GameValue((GameScalarType) strcmp(str1, str2));
}

#define OPERATORS_DEFAULT(XX, Category) \
  XX(GameAny,"@", mocnina, ListSelect, GameArray, GameScalar,"array","idx", "Shortcut for select: array @ 1 is the same as array select 1", "[\"a\",\"b\",true,3,8] @ 1", "\"b\"", "", "", Category) \
  XX(GameString,"@", mocnina, StringSelect, GameString, GameScalar,"string","idx", "Select one character from string identified by index", "\"Hello\" @ 2", "\"l\"", "", "", Category) \
  XX(GameArray,"@", mocnina, ListRangeSelect, GameArray, GameArray,"array","[from,to]", "Various methods how to select subarray of array. Selection is defined from ,to but last element of array is excluded. Second parameter is optional, if not specified, remain of array is selected. If first parameter is negative, index is counted from end of array. If second parameter is negative, it specified length of subarray (in absolute form)", "[\"a\",\"b\",true,3,8] @ [1,-3]", "[\"b\",true,3]", "", "", Category) \
  XX(GameString,"@", mocnina, StringRangeSelect, GameString, GameArray,"string","[from,to]", "Various methods how to select substring of string. It use the same style as selecting subarrays.Note: If you selecting abslute range, last character is excluded, see example:", "\"Hello world\" @ [2,7]", "llo w", "", "", Category) \
  XX(GameScalar,"find", function, FindString, GameString, GameString,"string","what", "Finds substring in string.", "\"one two three\" find \"two\"", "4", "", "", Category) \
  XX(GameScalar,"findi", function, FindStringI, GameString, GameString,"string","what", "Finds substring in string case insensitive.", "\"one two three\" findi \"TWO\"", "4", "", "", Category) \
  XX(GameArray, "sortBy",function, SortBy, GameArray, GameString, "Array","Block", "Sorts the array. The block defines a compare function, it has two parameters _x, _y and must return negative number -- if _x is less than _y, 0 -- if _x == _y and positive number ->if _x is bigger than _y. The sortBy function modifies input array.", "_arr = [3,2,1]; _arr sortBy {private [\"_ret\"];  _ret = 0; if (_x < _y) then {_ret = -1;}; if (_x > _y) then {_ret = 1;};  _ret};", "_arr is [1,2,3]", "", "", Category) \
  XX(GameScalar, "strCmp",function, StringCmp, GameString, GameString, "String","String", "Compares two strings. Returns a negative number if the first string is less than the second string, 0 if the strings are identical and a positive number otherwise.", "[\"a\" strCmp \"b\",\"a\" strCmp \"a\",\"b\" strCmp \"a\"]", "[-1,0,1]", "", "", Category) 

#define FUNCTIONS_DEFAULT(XX, Category) \
  XX(GameScalar, "count", lenOfString, GameString, "string", "Returns count of characters in string.", "count \"Test\"", "4", "", "", Category) \
  XX(GameString, "CEsc", CEsc, GameString, "string", "Creates string from source usign C/C++ escapes", "CEsc \"\\r\\n", "new line", "", "", Category) \
  XX(GameString, "chr", chrFunct, GameScalar, "a", "Returns character identified by ascii code", "chr 65", "\"A\"", "", "", Category) \
  XX(GameScalar, "asc", ascFunct, GameString, "a", "Returns ascii code of first character", "asc \"A\"", "65", "", "", Category) \
  XX(GameArray, "splitPath", splitPath, GameString, "path", "Splits path into araay", "splitPath \"C:\\test\\anFile.txt\"", "[\"c:\",\"\\test\\\",\"anFile\",\".txt\"]", "", "", Category) \
  XX(GameScalar, "val", valFunct, GameString, "a", "Converts number in string into Number type. Doesn't evaluates any expression. Returns 0 if error", "val \"124\"", "124.0", "", "", Category) \
  XX(GameScalar, "fromHex", hexFunct, GameString, "a", "Converts hex number in string into Number type. Doesn't evaluates any expression. Returns 0 if error", "hex \"12EC7\"", "77511.0", "", "", Category) \
  XX(GameString, "toHex", tohexFunct, GameScalar, "a", "Converts number into hexadecimal string.", "tohex 77511 ", "\"12EC7\"", "", "", Category) \
  XX(GameString, "tolower", tolower, GameString, "a", "Converts number into lowercase string.", "lowercase \"AHoj\"", "\"ahoj\"", "", "", Category) \
  XX(GameString, "toupper", toupper, GameString, "a", "Converts number into uppercase string.", "uppercase \"AHoj\"", "\"AHOJ\"", "", "", Category) \


//#define NULARS_DEFAULT(XX, Category) 

static GameOperator BinaryFuncts[]=
{
  OPERATORS_DEFAULT(REGISTER_OPERATOR, Category)
};

static GameFunction UnaryFuncts[]=
{
  FUNCTIONS_DEFAULT(REGISTER_FUNCTION, Category)
};

/*static GameNular NularyFuncts[]=
{
  NULARS_DEFAULT(REGISTER_NULAR, Category)
};*/


#define TYPES_DEFAULT(XX, Category) \

/*static GameTypeType TypeDef[]=
{
    TYPES_DEFAULT(REGISTER_TYPE,Category)
};*/

#if DOCUMENT_COMREF
static ComRefFunc DefaultComRefFunc[] =
{
//  NULARS_DEFAULT(COMREF_NULAR, Category)
  FUNCTIONS_DEFAULT(COMREF_FUNCTION, Category)
  OPERATORS_DEFAULT(COMREF_OPERATOR, Category)
};

/*static ComRefType DefaultComRefType[] =
{
  TYPES_DEFAULT(COMREF_TYPE, Category)
/*  TYPES_DEFAULT_COMB(COMREF_TYPE, "Default")
};*/
#endif

void GdStringsArrays::RegisterToGameState(GameState *gState)
{
  //gState->NewType(TypeDef[0]);
  gState->NewOperators(BinaryFuncts,lenof(BinaryFuncts));
  gState->NewFunctions(UnaryFuncts,lenof(UnaryFuncts));
  //gState->NewNularOps(NularyFuncts,lenof(NularyFuncts));
#if DOCUMENT_COMREF
  gState->AddComRefFunctions(DefaultComRefFunc,lenof(DefaultComRefFunc));
//  gState->AddComRefTypes(DefaultComRefType,lenof(DefaultComRefType));
#endif
}
