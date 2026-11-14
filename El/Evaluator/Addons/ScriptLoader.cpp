#include <El/elementpch.hpp>
#include <El/Evaluator/express.hpp>
#include <el\preprocc\preproc.h>
#include <el\pathname\pathname.h>
#include <malloc.h>
#include <El/MultiThread/Tls.h>
#include "ScriptLoader.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


ScriptLoaderStream *ScriptLoaderClass::FindLibrary(const char *filename)
{
  Pathname dir;    
  for (int i=0;i<searchPaths.Size();i++)
  {
    ScriptLoaderStream *str=new ScriptLoaderStream();
    dir.SetDirectory(searchPaths[i]);
    Pathname out(filename,dir);
    str->open(out);
    if (!str->fail()) 
    {
      str->restoreScriptName=curScriptName;
      curScriptName=out;
      return str;    
    }
    delete str;
  }
  return NULL;
}


QIStream *ScriptLoaderClass::OnEnterInclude(const char *filename)
{
  ScriptLoaderStream *stream=new ScriptLoaderStream ();
  stream->restoreScriptName=curScriptName;
  curScriptName=Pathname(filename,curScriptName);
  if (curScriptName.IsNull()) curScriptName=filename;
  stream->open(curScriptName);
  if (stream->fail()) 
  {    
    curScriptName=stream->restoreScriptName;
    delete stream;
    stream=FindLibrary(filename);
    return stream;
  }  
  return stream;
}

void ScriptLoaderClass::OnExitInclude(QIStream *str)
{
  ScriptLoaderStream *stream=static_cast<ScriptLoaderStream  *>(str);
  curScriptName=stream->restoreScriptName;  
  delete stream;
}


bool ScriptLoaderClass::LoadScript(const char *filename)
{
  script.rewind();
  if (Process(&script,filename)==false) return false;
  script.put(0);
  return true;
}

void ScriptLoaderClass::LoadScriptDirect(const char *_script)
{
  script.rewind();
  script<<_script;
  script.put(0);
}



RString ScriptLoaderClass::GetPreprocError()
{
  const char *errorStr;
  switch (Preproc::error)
  {
    case prNoError:errorStr="0 OK";break;
    case prStreamOpenError: errorStr="1 #include file not found";break;
    case prIncludeError: errorStr="2 #include command syntax error";break;
    case prIncludeMaxRecursion: errorStr="3 #include maximal recursion reached";break;
    case prDefineError: errorStr="4 #define command syntax error";break;
    case prDefineParamError: errorStr="5 #define parametters syntax error";break;
    case prParseExit: errorStr="6 Parser exitted (internal error)";break;
    case prInvalidPreprocessorCommand: errorStr="7 Unknown preprocessor command";break;
    case prUnexceptedEndOfFile: errorStr="8 Unexcepted end of script (didn't you forget parenthesis?)";break;
    case prToManyParameters: errorStr="9 Too many parameters";break;
    case prToFewParameters: errorStr="10 Too few parameters";break;
    case prUnexceptedSymbol: errorStr="11 Unexcepted symbol";break;
    case prEndIfExcepted: errorStr="12 #EndIf excepted";break;
    default: errorStr=(char *)alloca(100);
      sprintf(const_cast<char *>(errorStr),"13 Unexcepted error (%d)", Preproc::error);
      break;
  }
  const char *mask="Preprocess error: %s at %s line %d";
  int len=strlen(errorStr)+strlen(mask)+strlen(Preproc::filename.Data())+50;
  RString res;
  sprintf(res.CreateBuffer(len),mask,errorStr,Preproc::filename.Data(),Preproc::curline);
  return res;
}

void ScriptLoaderClass::SetLibraryPaths(const char *libs) //paths is separated by ;
{
  const char *begin=libs;
  const char *it=strchr(begin,';');
  searchPaths.Clear();
  while (it)
  { 
    RString lib(begin,it-begin);
    begin=it+1;
    it=strchr(begin,';');
    if (lib.GetLength()>0) searchPaths.Append()=lib;
  }
  if (begin[0])
  {
    RString lib=begin;
    searchPaths.Append()=lib;
  }
  char buff[MAX_PATH*4];
  GetModuleFileName(NULL,buff,sizeof(buff));
  *(const_cast<char *>(Pathname::GetNameFromPath(buff))-1)=0;
  searchPaths.Append()=buff;
  GetCurrentDirectory(sizeof(buff),buff);
  searchPaths.Append()=buff;
}


#include <DbgHelp.h>

static TLS(GameState) curGameState;
static Pathname dumpPath;

#pragma comment (lib,"Dbghelp.lib")


#ifdef _SCRIPT_DEBUGGER
static LONG ScriptUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{    
    MINIDUMP_EXCEPTION_INFORMATION expinfo;    
    expinfo.ClientPointers=FALSE;
    expinfo.ExceptionPointers=ExceptionInfo;
    expinfo.ThreadId=GetCurrentThreadId();
    HANDLE hDumpFile=CreateFile(dumpPath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if (hDumpFile)
    {
      MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),hDumpFile,MiniDumpWithFullMemory,&expinfo,NULL,NULL);
      CloseHandle(hDumpFile);
    }
    if (curGameState)
    {
      IScriptDebugger *dbg=curGameState->AttachDebugger(NULL);
      curGameState->AttachDebugger(dbg);
      if (dbg)
      {
        dbg->OnError(*curGameState,"Internal Error: Unhanded Exception");
        ExitProcess(0);
      }
    }
    return EXCEPTION_CONTINUE_SEARCH;
};
#endif

GameValue ScriptLoaderClass::RunScript(GameState &gState)
{
  gState.BeginContext(this);
  GameValue result;
#ifdef _SCRIPT_DEBUGGER
  LPTOP_LEVEL_EXCEPTION_FILTER topFilter=SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ScriptUnhandledExceptionFilter);
#endif
  dumpPath=curScriptName;
  curGameState=&gState;
  dumpPath.SetExtension(".mdmp");
  dumpPath.SetFilename(Pathname::GetExePath().GetTitle());
  result=gState.EvaluateMultiple(script.str(),false);     
#ifdef _SCRIPT_DEBUGGER
  SetUnhandledExceptionFilter(topFilter);
#endif
  gState.EndContext();
  return result;
}

