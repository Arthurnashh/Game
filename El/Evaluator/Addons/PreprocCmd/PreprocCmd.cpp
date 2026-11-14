#include <el/elementpch.hpp>
#include <el/PreprocC/Preproc.h>
#include "PreprocCmd.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define Category "PreprocCmd"

struct LastPreprocError
{
  const char *text;
  RString filename;
  int line;
  PreprocessError error;  
};

static LastPreprocError SLastPreprocError;

class ScriptPreproc: public Preproc, public GameVarSpace
{
  const GameState *gs;
  GameValuePar controlScript;
  int _level;

  class StrStream:public QIStrStream
  {
    RString text;
  public:
    StrStream(RString text):text(text),QIStrStream(text,text.GetLength()) {}
  };
public:
  ScriptPreproc(const GameState *gs,  GameValuePar controlScript):
      GameVarSpace(gs->GetContext()),
        gs(gs),
        controlScript(controlScript),
        _level(0) {}

        virtual QIStream *OnEnterInclude(const char *filename)
        {
          GameVarSpace::VarSet("_this",GameStringType(filename));
          gs->BeginContext(this);
          GameValue res=gs->EvaluateMultiple((RString)controlScript);
          if (res.GetNil())
          {
            gs->EndContext();
            return 0;
          }
          else
          {
            _level++;
            gs->EndContext();
            RString text=res;
            QIStrStream *stream=new StrStream(text);
            return stream;
          }
        }

        virtual void OnExitInclude(QIStream *stream)
        {
          _level--;
          delete stream;
        }

        int GetLevel() const {return _level;}
};

static GameValue preprocFile(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{

  ScriptPreproc preproc(gs,oper2);
  QOStrStream out;
  if (preproc.Process(&out,(RString)oper1)==false)
  {
    SLastPreprocError.error=preproc.error;
    SLastPreprocError.filename=preproc.filename;
    SLastPreprocError.line=preproc.curline;
    switch (preproc.error)
    {
    case prNoError: SLastPreprocError.text="No error";break;
    case prStreamOpenError: SLastPreprocError.text="Unable to read specified file";break;
    case prIncludeError: SLastPreprocError.text="#include syntax error ";break;
    case prIncludeMaxRecursion: SLastPreprocError.text="Include - max recursions reached";break;
    case prDefineError: SLastPreprocError.text="#define syntax error";break;
    case prDefineParamError: SLastPreprocError.text="#define syntax parameters error";break;
    case prParseExit: SLastPreprocError.text="Unexcepted exit";break;
    case prInvalidPreprocessorCommand: SLastPreprocError.text="Invalid preprocessor command";break;
    case prUnexceptedEndOfFile: SLastPreprocError.text="Unexcepted end of file";break;
    case prToManyParameters: SLastPreprocError.text="Too many parameters";break;
    case prToFewParameters: SLastPreprocError.text="Too few parameters";break;
    case prUnexceptedSymbol: SLastPreprocError.text="Symbol is not excepted";break;
    case prEndIfExcepted: SLastPreprocError.text="Missing EndIF";break;
    default: SLastPreprocError.text="Error has no description (undetermined error)";break;
    }
    return GameValue();
  }
  else
  {
    out.put(0);
    RString result=out.str();
    return result;
  }
}


static GameValue readTextFile(const GameState *gs, GameValuePar oper1)
{
  RString filename=oper1;
  HANDLE h=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0);
  if (h==INVALID_HANDLE_VALUE || h==0)
  {
    return GameValue();
  }

  DWORD size=GetFileSize(h,0);
  DWORD rd;
  RString buffer;
  char *b=buffer.CreateBuffer(size+1);
  ReadFile(h,b,size,&rd,0);
  b[size]=0;
  CloseHandle(h);
  if (rd!=size) return GameValue();
  return buffer;
}

static GameValue preprocError(const GameState *gs)
{
  GameArrayType arr;
  arr.Add(GameValue((float)((int)SLastPreprocError.error)));
  arr.Add(GameValue((float)SLastPreprocError.line));
  arr.Add(SLastPreprocError.filename);
  arr.Add(SLastPreprocError.text);
  return arr;
}

static GameValue preprocCurLevel(const GameState *gs)
{
  GameVarSpace *vspc=gs->GetContext();
  ScriptPreproc *scrp=dynamic_cast<ScriptPreproc *>(vspc);
  if (scrp==0) 
  {
    gs->SetError(EvalForeignError,"Function 'preprocCurLevel' is useful only inside control script");
    return GameValue();
  }

  return (float)scrp->GetLevel();
}

#define OPERATORS_DEFAULT(XX, Category) \
  XX(GameScalar, "preprocFile",function, preprocFile, GameString, GameCode, "String","Code", \
    "Calls preprocessor to preprocess file. Function takes filename on the left"\
    " and control script on the right. Control script must convert this"\
    " name to the string that contains the script to preprocess."\
    " Control script can read this string from file. Control script "\
    " returns readed script or NIL if error happens. Control script "\
    " is called for given name and for any #include found inside.",\
    "", "", "", "", Category) 

#define FUNCTIONS_DEFAULT(XX, Category) \
  XX(GameString, "readTextFile", readTextFile, GameString, "string", "Reads text file into the string. Source must be text file, otherwise result is unpredictable. Function returns NIL, if it cannot read from the file.", "_text=readTextFile \"brief.txt\"", "", "", "", Category) 

#define NULARS_DEFAULT(XX, Category) \
  XX(GameArray, "preprocError",preprocError, "Returns error code of the last preproc command in form [code, line, text]", "", "", "", "", Category)\
  XX(GameScalar, "preprocCurLevel",preprocCurLevel, "If used in control script, it returns current level of recursion inclusion", "", "", "", "", Category)


static GameOperator BinaryFuncts[]=
{
  OPERATORS_DEFAULT(REGISTER_OPERATOR, Category)
};

static GameFunction UnaryFuncts[]=
{
  FUNCTIONS_DEFAULT(REGISTER_FUNCTION, Category)
};

static GameNular NularyFuncts[]=
{
  NULARS_DEFAULT(REGISTER_NULAR, Category)
};

#if DOCUMENT_COMREF
static ComRefFunc DefaultComRefFunc[] =
{
  NULARS_DEFAULT(COMREF_NULAR, Category)
    FUNCTIONS_DEFAULT(COMREF_FUNCTION, Category)
    OPERATORS_DEFAULT(COMREF_OPERATOR, Category)
};
#endif

void GdPreprocCmd::RegisterToGameState(GameState *gState)
{
  gState->NewOperators(BinaryFuncts,lenof(BinaryFuncts));
  gState->NewFunctions(UnaryFuncts,lenof(UnaryFuncts));
  gState->NewNularOps(NularyFuncts,lenof(NularyFuncts));
#if DOCUMENT_COMREF
  gState->AddComRefFunctions(DefaultComRefFunc,lenof(DefaultComRefFunc));
#endif
}

