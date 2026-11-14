#include <fstream>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <Es/essencePch.hpp>
#include <el/elementpch.hpp>
#include <windows.h>
#include <shellapi.h>
#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>
#include <El/Pathname/Pathname.h>
#include <el/math/math3d.hpp>
#include <Es/Common/fltOpts.hpp>
#include <ES/Strings/rString.hpp>

#include ".\gdiostream.h"
#include <el\TCPIPBasics\IPA.h>
#include <el\TCPIPBasics\WSA.h>
#include <el\TCPIPBasics\Socket.h>
#include "archivestreamwindowsfile.h"
#include "archivestreammemory.h"
#include "archivestreamtcp.h"
#include <malloc.h>
#include <el/RegularExpressions/regexp.h>
#include <io.h>


#define Category "O2Script:IOStream"

DEFINE_FAST_ALLOCATOR(GdIOStream)
DEFINE_FAST_ALLOCATOR(GdIOStreamV)
  
  
  static ctWeakRef<CWSA> wsa;

static BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
  return TRUE;
}

class ArchiveStreamConsole
{
  static int consoleCount;  
  public:
    ArchiveStreamConsole() 
    {GetConsole();
    }
    ~ArchiveStreamConsole() 
    {FreeConsole();
    }
    bool GetConsole()
    {
      if (consoleCount==0) 
      {
        if (GetStdHandle(STD_INPUT_HANDLE)==0 && GetStdHandle(STD_OUTPUT_HANDLE)==0 && GetStdHandle(STD_ERROR_HANDLE)==0)
        {
          AllocConsole();
          ::SetConsoleCtrlHandler(HandlerRoutine,TRUE);
        }
        else
          consoleCount++;
      }
      consoleCount++;
      return true;
    }
    bool FreeConsole()
    {
      consoleCount--;
      if (consoleCount<1)
      {
        ::SetConsoleCtrlHandler(HandlerRoutine,FALSE);
        ::FreeConsole();
        consoleCount=0;
        SetStdHandle(STD_INPUT_HANDLE,0);
        SetStdHandle(STD_OUTPUT_HANDLE,0);
        SetStdHandle(STD_ERROR_HANDLE,0);
        return false;
      }
      return true;
    }
};

int ArchiveStreamConsole::consoleCount=0;

class ArchiveStreamConsoleIn:public ArchiveStreamConsole, public ArchiveStreamWindowsFileIn
{
  public:
    ArchiveStreamConsoleIn():ArchiveStreamConsole(),ArchiveStreamWindowsFileIn(GetStdHandle(STD_INPUT_HANDLE),false) 
    {
    }

};

class ArchiveStreamConsoleOut:public ArchiveStreamConsole, public ArchiveStreamWindowsFileOut
{
  public:
    ArchiveStreamConsoleOut():ArchiveStreamConsole(),ArchiveStreamWindowsFileOut(GetStdHandle(STD_OUTPUT_HANDLE),false) 
    {
    }
    int DataExchange(void *buffer, int maxsize)
    {
      if (_err) return _err;
      for (int i=0;i<maxsize;i+=1024)
      {
        _err=__super::DataExchange((char *)buffer+i,(maxsize-i)>1024?1024:(maxsize-i));
        if (_err) return _err;
      }
      return _err;
    }
};

class ArchiveStreamConsoleError:public ArchiveStreamConsole, public ArchiveStreamWindowsFileOut
{
  public:
    ArchiveStreamConsoleError():ArchiveStreamConsole(),ArchiveStreamWindowsFileOut(GetStdHandle(STD_ERROR_HANDLE),false) 
    {
    }
};

class ArchiveStreamForPipeIn: public ArchiveStreamWindowsFileIn
{
  HANDLE _toclose;
public:
  ArchiveStreamForPipeIn(HANDLE left, HANDLE right): ArchiveStreamWindowsFileIn(left,true),_toclose(right) {}
  ~ArchiveStreamForPipeIn() {CloseHandle(_toclose);}
};

class ArchiveStreamForPipeOut: public ArchiveStreamWindowsFileOut
{
  HANDLE _toclose;
public:
  ArchiveStreamForPipeOut(HANDLE left, HANDLE right): ArchiveStreamWindowsFileOut(left,true),_toclose(right) {}
  ~ArchiveStreamForPipeOut() {CloseHandle(_toclose);}
};



RString GdIOStreamV::GetText() const
{
  if (!_stream->_inBuff.IsNull())
  {
    ArchiveStreamMemoryOut *buff=static_cast<ArchiveStreamMemoryOut *>(_stream->_inBuff.GetRef());
    if (buff->GetBufferSize())
    {
      char sbuff[256];
      int len=__min(255,_stream->_posmax);
      strncpy(sbuff,buff->GetBuffer(),len);
      sbuff[len]=0;
      char sbuff2[256];
      len=__min(255,buff->GetBufferSize()-_stream->_posmax);
      strncpy(sbuff2,buff->GetBuffer()+_stream->_posmax,len);
      sbuff2[len]=0;
      return _stream->_debugText+" \"("+sbuff+")"+sbuff2+"\"";
    }
  }
  return _stream->_debugText;
}

bool GdIOStreamV::IsEqualTo(const GameData *data) const
{
  const GdIOStream *other=dynamic_cast<const GdIOStreamV *>(data)->_stream;
  if (other==NULL) return false;
  return (const ArchiveStream *)other->_streamIn==(const ArchiveStream *)_stream->_streamIn && (const ArchiveStream *)other->_streamOut==(const ArchiveStream *)_stream->_streamOut;
}

bool GdIOStream::CreateConsole()
{
  _streamIn=ArchiveStreamRef::Attach(wknew ArchiveStreamConsoleIn);
  _streamOut=ArchiveStreamRef::Attach(wknew ArchiveStreamConsoleOut);
  _debugText="Standard";
  return true;
}

bool GdIOStream::CreateErrorConsole()
{
  _streamIn=ArchiveStreamRef::Attach(wknew ArchiveStreamConsoleIn);
  _streamOut=ArchiveStreamRef::Attach(wknew ArchiveStreamConsoleError);
  _debugText="Standard+Error";
  return true;
}

bool GdIOStream::CreateFileIO(int mode, const char *filename)
{
  int iomode=0;
  int creat=OPEN_EXISTING;
  switch (mode)
  {
    case 0: iomode=0;creat=OPEN_EXISTING;break;
    case 1: iomode=GENERIC_READ; creat=OPEN_EXISTING;break;
    case 2: iomode=GENERIC_WRITE; creat=CREATE_ALWAYS;break;
    case 3: iomode=GENERIC_READ|GENERIC_WRITE; creat=OPEN_EXISTING;break;
    case 4: iomode=GENERIC_READ|GENERIC_WRITE; creat=OPEN_ALWAYS;break;
  }
  HANDLE h=CreateFile(filename,iomode,0,NULL,creat,0,NULL);
  if (h==NULL || h==INVALID_HANDLE_VALUE) return false;
  HANDLE hdup;
  DuplicateHandle(GetCurrentProcess(),h,GetCurrentProcess(),&hdup,0,FALSE,DUPLICATE_SAME_ACCESS);
  _streamIn=ArchiveStreamRef::Attach(wknew ArchiveStreamWindowsFileIn(h,true));
  _streamOut=ArchiveStreamRef::Attach(wknew ArchiveStreamWindowsFileOut(hdup,true));
  _debugText=filename;
  return true;
}

bool GdIOStream::CreateMemoryStream(const char *inbuffer)
{
  _streamIn=ArchiveStreamRef::Attach(wknew ArchiveStreamMemoryIn(_strdup(inbuffer),strlen(inbuffer),true));
  _streamOut=ArchiveStreamRef::Attach(wknew ArchiveStreamMemoryOut());
  _debugText="MemoryStream";
  return true;
}

bool GdIOStream::CreateTCPIPStream(const char *server, int timeout)
{
  if (wsa.IsNull()) wsa=wknew CWSA(0x101);
  CIPA ipaddr(server,0);
  if (ipaddr.IsAddrValid() && ipaddr.IsPortValid())
  {
    Socket conn;
    if (conn.Create(SOCK_STREAM)==false) return false;
    if (conn.Connect(ipaddr)==false) return false;    
    _streamIn=ArchiveStreamRef::Attach(wknew ArchiveStreamTCPIn(conn,timeout?timeout:INFINITE,false,NULL,NULL));
    _streamOut=ArchiveStreamRef::Attach(wknew ArchiveStreamTCPOut(conn,timeout?timeout:INFINITE,false,NULL,NULL));
    _debugText=RString("connection:")+server;
    return true;
  }
  else
    return false;
}

bool GdIOStream::CreateTCPIPStream(int port, int timeout)
{
  if (wsa.IsNull()) wsa=wknew CWSA(0x101);
  Socket conn;
  CIPA inconn;
  if (conn.Create(SOCK_STREAM)==false) return false;
  if (conn.Bind(CIPA((unsigned long)0,port))==false) return false;
  if (conn.Listen()==false) return false;
  conn.SetNonBlocking(true);
  if (conn.Wait(timeout,Socket::WaitRead)!=Socket::WaitRead) return false;
  Socket exch=conn.Accept(inconn);
  if (!exch.IsValid()) return false;  
  _streamIn=ArchiveStreamRef::Attach(wknew ArchiveStreamTCPIn(exch,timeout?timeout:INFINITE,false,NULL,NULL));
  _streamOut=ArchiveStreamRef::Attach(wknew ArchiveStreamTCPOut(exch,timeout?timeout:INFINITE,false,NULL,NULL));
  char hostname[512];
  inconn.GetHostName(hostname,sizeof(hostname));
  _debugText=RString("connection:")+hostname;
  return true; 
}

struct ProcessWatcherHandles
{
  HANDLE close1,close2;
  HANDLE process;
};


/** Process watcher thread is created for each child process that was started. 
This thread must sleep until process exits. When it happens, ProcessWatcher must 
close handles of both pipes, it terminates any waiting operation. Second end of pipe 
is closed automatically */

static DWORD WINAPI ProcessWatcher(LPVOID info)
{
  ProcessWatcherHandles *pinfo=(ProcessWatcherHandles *)info;
  WaitForSingleObject(pinfo->process,INFINITE); //wait for process exits;
  CloseHandle(pinfo->process);    //close its handle;
  CloseHandle(pinfo->close1);     //close all pipe handles;
  CloseHandle(pinfo->close2);
  return 0;
}

bool GdIOStream::CreatePipeStream(const char *name)
{

  HANDLE pipeInLeft=NULL, pipeInRight=NULL,pipeOutLeft=NULL,pipeOutRight=NULL;
  SECURITY_ATTRIBUTES sa;  
  BOOL result;
  memset(&sa,0,sizeof(sa));
  sa.bInheritHandle=TRUE;
  result=CreatePipe(&pipeInLeft,&pipeInRight,&sa,0);
  if (result) result=CreatePipe(&pipeOutRight,&pipeOutLeft,&sa,0);
  if (result) 
  {
    STARTUPINFO startupinfo;
    PROCESS_INFORMATION processinfo;

    memset(&startupinfo,0,sizeof(startupinfo));
    memset(&processinfo,0,sizeof(processinfo));

    DuplicateHandle(GetCurrentProcess(),pipeInLeft,GetCurrentProcess(),&pipeInLeft,0,FALSE,DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
    DuplicateHandle(GetCurrentProcess(),pipeOutLeft,GetCurrentProcess(),&pipeOutLeft,0,FALSE,DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);  
   
        
    startupinfo.cb=sizeof(startupinfo);
    startupinfo.dwFlags=STARTF_USESTDHANDLES;
    startupinfo.hStdInput=pipeOutRight;
    startupinfo.hStdOutput=pipeInRight;
    startupinfo.hStdError=GetStdHandle(STD_ERROR_HANDLE);
    char *cmdline=strcpy((char *)alloca(strlen(name)+1),name);

    bool result=CreateProcess(NULL,cmdline,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&startupinfo,&processinfo)!=FALSE;    
    ProcessWatcherHandles *hndl=new ProcessWatcherHandles;
    hndl->close1=pipeOutRight;
    hndl->close2=pipeInRight;
    hndl->process=processinfo.hProcess;
    CloseHandle(processinfo.hThread);
    DWORD id;
    HANDLE watcher=CreateThread(NULL,0,ProcessWatcher,hndl,0,&id);
    CloseHandle(watcher);
  }
  if (result)
  {
    _streamIn=ArchiveStreamRef::Attach(wknew ArchiveStreamWindowsFileIn(pipeInLeft,true));
    _streamOut=ArchiveStreamRef::Attach(wknew ArchiveStreamWindowsFileOut(pipeOutLeft,true));
    _debugText=RString("PipeTo:",name);    
    return true;
  }
  else
  {
    CloseHandle(pipeInLeft);
    CloseHandle(pipeInRight);
    CloseHandle(pipeOutLeft);
    CloseHandle(pipeOutRight);
    return false;
  }

//  return true;
}

static GameValue openFile(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[0].GetType()!=GameString || arr[1].GetType()!=GameScalar)
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  GdIOStream *stream=new GdIOStream();
  bool res=stream->CreateFileIO(toInt(arr[1]),(RString)arr[0]);
  if (!res) 
  {
    delete stream;
    return GameValue();
  }
  return GameValue(new GdIOStreamV(stream));
}


static GameValue openNetworkStream(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[0].GetType()!=GameString || arr[1].GetType()!=GameScalar)
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  GdIOStream *stream=new GdIOStream();
  bool res=stream->CreateTCPIPStream((RString)arr[0],toInt(arr[1]));
  if (!res) 
  {
    delete stream;
    return GameValue();
  }
  return GameValue(new GdIOStreamV(stream));
}


static GameValue openNetworkServer(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[0].GetType()!=GameScalar || arr[1].GetType()!=GameScalar)
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  GdIOStream *stream=new GdIOStream();
  bool res=stream->CreateTCPIPStream(toInt(arr[0]),toInt(arr[1]));
  if (!res) 
  {
    delete stream;
    return GameValue();
  }
  return GameValue(new GdIOStreamV(stream));
}

static GameValue openStandardIO(const GameState *gs)
{
  GdIOStream *stream=new GdIOStream();
  bool res=stream->CreateConsole();
  if (!res) 
  {
    delete stream;
    return GameValue();
  }
  return GameValue(new GdIOStreamV(stream));
}

static GameValue openErrorIO(const GameState *gs)
{
  GdIOStream *stream=new GdIOStream();
  bool res=stream->CreateErrorConsole();
  if (!res) 
  {
    delete stream;
    return GameValue();
  }
  return GameValue(new GdIOStreamV(stream));
}

static GameValue openMemoryStream(const GameState *gs, GameValuePar oper1)
{
  RString input=oper1;
  GdIOStream *stream=new GdIOStream();
  bool res=stream->CreateMemoryStream(input);
  if (!res) 
  {
    delete stream;
    return GameValue();
  }
  return GameValue(new GdIOStreamV(stream));
}

static GameValue openHybridStream(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[0].GetType()!=EvalType_IOStream|| arr[1].GetType()!=EvalType_IOStream)
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  GdIOStream *in=static_cast<GdIOStreamV *>(arr[0].GetData())->GetStream();
  GdIOStream *out=static_cast<GdIOStreamV *>(arr[1].GetData())->GetStream();
  GdIOStream *stream=new GdIOStream();
  stream->SetStreams(in->GetInputStream(),out->GetOutputStream());
  return GameValue(new GdIOStreamV(stream));
}


static GameValue writeStream(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  ctWeakRef<ArchiveStream> ostr=stream->GetOutputStream();
  RString text=oper2;
  const char *data=text;
  ostr->DataExchange((void *)data,text.GetLength());
  return oper1;
}

static GameValue EndOfLine(const GameState *gs)
{
  RString text="\r\n";
  return text;
}


static int GetFormatSafeSize(const char *format, int minimumSize, bool isText)
{
  const char *check=format;
  while (*check) if (isalpha(*check++)) return -1;
  const char *fdigit=format;
  while (*fdigit) if (isdigit(*fdigit)) break;else fdigit++;
  if (!*fdigit) return -1;
  int maxlen=atoi(fdigit);
  const char *addigit=fdigit;
  int fract=-1;
  while (*addigit && *addigit!='.') addigit++;
  if (*addigit=='.')
  {
    addigit++;
    fract=atoi(addigit);
  }
  if (isText && fract!=-1) minimumSize=fract;
  else if (!isText && fract!=-1)
  {
    minimumSize+=fract;
  }
  int size=__max(minimumSize,maxlen);
  return size;    
}

static GameValue inFormNum(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  RString format=oper2;
  int countMax=GetFormatSafeSize(format,512,false);
  if (countMax==-1)
  {
    gs->SetError(EvalForeignError,"Invalid format specified");
    return GameValue();
  }
  char *buffer=(char *)alloca(countMax);
  format=RString(RString("%",format),"f");
  sprintf(buffer,format,(float)oper1);
  return buffer;  
}

static GameValue inFormText(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  RString format=oper2;
  RString text=oper1;
  int countMax=GetFormatSafeSize(format,text.GetLength()+50,true);
  if (countMax==-1)
  {
    gs->SetError(EvalForeignError,"Invalid format specified");
    return GameValue();
  }
  char *buffer=(char *)alloca(countMax);
  format=RString(RString("%",format),"s");
  sprintf(buffer,format,text.Data());
  return buffer;  
}

bool GdIOStream::TestRegExp(const char *pattern, SRef<char> &result)
{
  char *mod_pattern=(char *)alloca(strlen(pattern)+3);
  strcpy(mod_pattern,"^");
  strcat(mod_pattern,pattern);
  RegExp<CharIterator<GdIOStream> > regexp(mod_pattern);
  CharIterator<GdIOStream> textstream(this);  
  _posmax=0;
  bool res=regexp.Match(textstream)!=0;
  if (regexp._errorString[0])
  {
    result=new char[strlen(regexp._errorString)+1];
    strcpy(result,regexp._errorString);
  }  
  if (res) _posmax=regexp.GetEndOfSubExpression(0)-regexp.GetBeginOfSubExpression(0);
  return res;
}

char GdIOStream::operator [](int pos) 
{
  if (pos<0) return 0;
  if (_inBuff.IsNull())
    _inBuff=wknew ArchiveStreamMemoryOut();
  if (pos+1>_posmax) _posmax=pos+1;
  ArchiveStreamMemoryOut *membuf=static_cast<ArchiveStreamMemoryOut *>(_inBuff.GetRef());  
  if (membuf->GetBufferSize()>(unsigned long)pos)
  {
    return membuf->GetBuffer()[pos];
  }
  else
  {
    if (_streamIn->IsError()) return 0;
    unsigned long curbuf=membuf->GetBufferSize();
    while (curbuf<=(unsigned long)pos)
    {
      char z;
      _streamIn->ExSimple(z);
      if (_streamIn->IsError()) return 0;
      membuf->ExSimple(z);
      curbuf=membuf->GetBufferSize();
    }
    return membuf->GetBuffer()[pos];
  }
}

static GameValue regexpExplore (const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  RString text=oper2;
  SRef<char> error;
  bool res=stream->TestRegExp(text,error);
  if (error.NotNull())
  {
    RString errorText("Error in regular expression: ",error);
    gs->SetError(EvalForeignError,errorText.Data());
  }  
  return res;
}

static GameValue SkipUntil (const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  RString text=oper2;
  ctWeakRef<ArchiveStream> instr=stream->GetInputStream();
  const char *str=text;
  const char *rdpos=str;
  CharIterator<GdIOStream> z,p;
  stream->ResetInputCmpEnd();
  z.SetInterface(stream);
  int cnt=0;
  while (!instr->IsError())
  {
    if (*z==*str)
    {
      p=z+1;
      rdpos=str+1;
      while (*rdpos && *rdpos==*p) 
      {rdpos++;p++;
      }
      if (*rdpos==0)
      {
        if (cnt) stream->ReadInput(cnt,NULL);
        return oper1;
      }    
    }
    cnt++;
    z++;     
    rdpos=str;    
  }
  return oper1;
}

void GdIOStream::ReadInput(int characters, char *output)
{
  int rd=ReadInputCountChars(characters);
  if (rd<_posmax) _posmax-=rd;
  else _posmax=0;
  ArchiveStreamMemoryOut *membuf=static_cast<ArchiveStreamMemoryOut *>(_inBuff.GetRef());  
  int inbuff=membuf?membuf->GetBufferSize():0;  
  if (inbuff)
  {
    int total=__min(inbuff,rd);
    char *buffer=membuf->GetBuffer();
    if (output)
    {
      for (int i=0;i<total;i++)
      {
        *output++=buffer[i];
      }
    }
    membuf->ShiftBuffer(total);
    rd-=total;    
  }
  if (output)
    while (rd)
    {
      _streamIn->ExSimple(*output);
      output++;
      rd--;
    }
  else
    _streamIn->Reserved(rd);  
}

int GdIOStream::ReadInputCountChars(int characters)
{
  if (characters<1) characters=_posmax+characters;
  if (characters<0) characters=0;
  return characters;
}


static GameValue getStreamData (const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  int amount=toInt(oper2);
  int count=stream->ReadInputCountChars(amount)+1;
  char *buff;
  RString res;
  if (count & ~0xFFFF) buff=new char[count];else buff=(char *)alloca(count);
  stream->ReadInput(amount,buff);
  buff[count-1]=0;
  res=buff;
  if (count & ~0xFFFF) delete[] buff;
  return res;
}
static GameValue ignoreStreamData (const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  int amount=toInt(oper2);
  stream->ReadInput(amount,NULL);
  return oper1;
}

static GameValue eatWaitSpaces(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  CharIterator<GdIOStream> z;
  z.SetInterface(stream);
  stream->ResetInputCmpEnd();
  int cnt=0;
  while (*z && isspace(*z)) 
  {
    if (cnt==256) 
    {
      stream->ReadInput(0,NULL);
      cnt=0;
    }
    cnt++;
    z++;
  }
  stream->ReadInput(-1,NULL);
  stream->ResetInputCmpEnd();
  return oper1;
}

static GameValue eatWaitSpacesEOL(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  CharIterator<GdIOStream> z;
  z.SetInterface(stream);
  int cnt=0;
  while (*z && isspace(*z))
  {
    if (*z=='\n')
    {
      stream->ReadInput(-2,NULL);
      return oper1;
    }
    if (cnt==256) 
    {
      stream->ReadInput(0,NULL);
      cnt=0;
    }
    cnt++;
    z++;
  }
  stream->ReadInput(-1,NULL);
  stream->ResetInputCmpEnd();
  return oper1;
}

static GameValue testForNumber(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  SRef<char> result;
  bool res=stream->TestRegExp("[-+]?[0-9]*[.]?[0-9]+([Ee][-+]?[0-9]+)?",result); //full real number;
  return res;
}

static GameValue testForNumberInt(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  SRef<char> result;
  bool res=stream->TestRegExp("[-+]?[0-9]+",result); //full integer number;
  return res;
}

static GameValue testForIdentifier(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  SRef<char> result;
  bool res=stream->TestRegExp("[_a-zA-Z][_a-zA-Z0-9]*",result); //identifier
  return res;
}

static GameValue testForXMLTag(const GameState *gs, GameValuePar oper1,GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  RString tag=oper2;  
  RString str;
  SRef<char> result;
  int res=0;
  if (tag.GetLength()>0)
  {
    if (tag[0]=='/') 
      res=stream->TestRegExp(str+"<[\x01-\x21]*"+tag+"[\x01-\x21]*>",result)?1:0;      
    else
    {
        bool test=stream->TestRegExp(str+"<[\x01-\x21]*"+tag+"([\x01-\x21]+[_a-zA-Z][-_a-zA-Z0-9:]+=\"[^\"]*\"[\x01-\x21]*)*[\x01-\x21]*/>",result);
      if (test) res=2;
      else 
      {
          test=stream->TestRegExp(str+"<[\x01-\x21]*"+tag+"([\x01-\x21]+[_a-zA-Z][-_a-zA-Z0-9:]+=\"[^\"]*\"[\x01-\x21]*)*[\x01-\x21]*>",result);
        if (test) res=1;
        else
          res=0;
      }
    }
  }
  return (float)res;
}

static GameValue GetXMLTag(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  GameArrayType arr;
  CharIterator<GdIOStream> c;
  char zero=0;
  c.SetInterface(stream);
  ArchiveStreamMemoryOut var,val;
  if (*c!='<') 
  {gs->SetError(EvalForeignError,"XMLTag was not tested");return GameValue();
  }  
  c++;
  while (isspace(*c)) c++;
  while (*c && !isspace(*c) && *c!='>')
  {
    char z=*c++;
    val.ExSimple(z);
  }
  val.ExSimple(zero);
  arr.Append()=RString(val.GetBuffer());val.Rewind();
  while (*c!='>' && *c)
  {
    while (isspace(*c)) c++;
    if (isalpha(*c) || *c=='_' || *c=='-' || *c==':')
    {
      while (*c!='=' && *c) 
      {
        char z=*c++;
        var.ExSimple(z);
      }
      c++;
      if (*c!='"') 
      {gs->SetError(EvalForeignError,"XMLTag was not tested");return GameValue();
      }  
      c++;
      while (*c!='"' && *c)
      {
        char z=*c++;
        val.ExSimple(z);
      }
      var.ExSimple(zero);
      val.ExSimple(zero);
      arr.Append()=RString(var.GetBuffer());
      arr.Append()=RString(val.GetBuffer());
      var.Rewind();
      val.Rewind();
      ++c;
    }
    else if (*c=='/') c++;
    else if (*c=='>') break;
    else 
    {gs->SetError(EvalForeignError,"XMLTag was not tested");return GameValue();
    }  
  }
  if (*c==0)  
  {gs->SetError(EvalForeignError,"XMLTag was not tested");return GameValue();
  }  
  c++;
  stream->ReadInput(0,NULL);
  return arr;
}

static GameValue testEof(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  int err=stream->GetInputStream()->IsError();
  return err==ASTRERR_EOF|| err==ASTRERR_UNCOMPLETEDATA;
}

static GameValue testError(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  int err=stream->GetInputStream()->IsError();
  return err!=0 && !((bool)testEof(gs,oper1));
}

static GameValue deleteFile(const GameState *gs, GameValuePar oper1)
{
  RString name=oper1;
  return DeleteFile(name)!=FALSE;
}

static GameValue backupFile(const GameState *gs, GameValuePar oper1)
{
  RString name=oper1;  
  RString bak=name+".bak";
  if (_access(name,0)==0)
  {
    if (_access(name,02)==0)
    {
      if (_access(bak,0)==0)
      {
      if (DeleteFile(bak)==FALSE) return RString();
      }
    if (MoveFile(name,bak)==FALSE) return RString();
    return bak;
    }
    else return RString();
  }

return bak;  
}

static GameValue createFolder(const GameState *gs, GameValuePar oper1)
{
  RString name=oper1;
  return CreateDirectory(name,NULL)!=FALSE;
}

static GameValue createFolderEx(const GameState *gs, GameValuePar oper1)
{
  RString name=oper1;
  char *buff=(char *)alloca(name.GetLength()*2);
  int pid=1;
  while (Pathname::GetPartFromPath(name,pid,buff,name.GetLength()*2,-1))
  {
    CreateDirectory(buff,NULL);
    pid++;
  }
  if (buff[0]) CreateDirectory(buff,NULL);
  return true;
}

static GameValue copyFile(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  RString from=oper1;
  RString to=oper2;
  return CopyFile(from,to,TRUE)!=FALSE;
}

static GameValue shellRun(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  RString file=oper1;
  RString cmd=oper2;
  int res=(int)ShellExecute(NULL,cmd,file,NULL,NULL,SW_SHOWNORMAL); 
  return res>32;
}



static GameValue shellCmdOpenPipe(const GameState *gs, GameValuePar oper1)
{
  RString cmdline=oper1;
  GdIOStream *iostream=new GdIOStream;
  if (iostream->CreatePipeStream(cmdline)) return GameValue(new GdIOStreamV(iostream));

  delete iostream;
  return GameValue();
}

static GameValue shellCmd(const GameState *gs, GameValuePar oper1)
{
    STARTUPINFO startupinfo;
    PROCESS_INFORMATION processinfo;

    memset(&startupinfo,0,sizeof(startupinfo));
    memset(&processinfo,0,sizeof(processinfo));

    RString cmdline=oper1;
    startupinfo.cb=sizeof(startupinfo);
    char *cmd=strcpy((char *)alloca(cmdline.GetLength()+1),cmdline);

    bool result=CreateProcess(NULL,cmd,NULL,NULL,TRUE,0,NULL,NULL,&startupinfo,&processinfo)!=FALSE;
    
    if (result)
    {
      WaitForSingleObject(processinfo.hProcess,INFINITE);
      DWORD exit;
      GetExitCodeProcess(processinfo.hProcess,&exit);
      CloseHandle(processinfo.hProcess);
      CloseHandle(processinfo.hThread);
      return (float)exit;
    }
    else
      return -1.0f;
}


static GameValue shellCmdNoWait(const GameState *gs, GameValuePar oper1)
{
    STARTUPINFO startupinfo;
    PROCESS_INFORMATION processinfo;

    memset(&startupinfo,0,sizeof(startupinfo));
    memset(&processinfo,0,sizeof(processinfo));

    RString cmdline=oper1;
    startupinfo.cb=sizeof(startupinfo);
    char *cmd=strcpy((char *)alloca(cmdline.GetLength()+1),cmdline);

    bool result=CreateProcess(NULL,cmd,NULL,NULL,TRUE,0,NULL,NULL,&startupinfo,&processinfo)!=FALSE;
    
    if (result)
    {
      CloseHandle(processinfo.hProcess);
      CloseHandle(processinfo.hThread);
      return (float)0;
    }
    else
      return -1.0f;
}

static GameValue getLine(const GameState *gs, GameValuePar oper1)
{
  GameValue nlend=RString("[^\\r\\n]*");
  regexpExplore(gs,oper1,nlend);
  GameValue res=getStreamData(gs,oper1,GameValue((float)0));
  if (res.GetNil()) return res;
  nlend=RString("[\\r]*[\\n]");
  regexpExplore(gs,oper1,nlend);
  ignoreStreamData(gs,oper1,GameValue((float)0));
  return res;
}

static GameValue getBuffer(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  ArchiveStream *ostr=stream->GetOutputStream();
  ArchiveStreamMemoryOut *mostr=dynamic_cast<ArchiveStreamMemoryOut *>(ostr);
  if (mostr==NULL) return GameValue();
  RString res;
  int size=mostr->GetBufferSize();
  char *buff=res.CreateBuffer(size+1);
  memcpy(buff,mostr->GetBuffer(),size);
  buff[size]=0;
  return res;
}

static GameValue seekOutput(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  __int64 pos=(__int64)(toLargeInt(oper2));
  stream->GetOutputStream()->Seek(pos,ArchiveStream::begin);
  return GameValue();
}

static GameValue seekInput(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  __int64 pos=(__int64)(toLargeInt(oper2));
  stream->GetInputStream()->Seek(pos,ArchiveStream::begin);
  return GameValue();
}


static GameValue tellInput(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  __int64 pos=stream->GetInputStream()->Tell();
  return (float)pos;
}

static GameValue tellOutput(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  __int64 pos=stream->GetOutputStream()->Tell();
  return (float)pos;
}

static GameValue fileSize(const GameState *gs, GameValuePar oper1)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  __int64 pos=stream->GetOutputStream()->Tell();
  stream->GetInputStream()->Seek(0,ArchiveStream::end);
  __int64 sz=stream->GetOutputStream()->Tell();
  stream->GetInputStream()->Seek(pos,ArchiveStream::begin);
  return (float)sz;
}

static GameValue renameFile( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  RString a=oper1;
  RString b=oper2;
  int res=rename(a,b);
  return res==0;
}

static GameValue homeFolder(const GameState *state)
{
  Pathname pth=Pathname::GetExePath();
  return RString(pth.GetDirectoryWithDrive());
}


static GameValue hostingApp(const GameState *state)
{
  Pathname pth=Pathname::GetExePath();
  return RString(pth.GetFullPath());
}

static GameValue GetCurrentDir(const GameState *state)
{
  RString rs;
  int needsz=GetCurrentDirectory(0,0)+1;
  GetCurrentDirectory(needsz,rs.CreateBuffer(needsz));
  return rs;
}

static GameValue SetCurrentDir(const GameState *state, GameValuePar oper1)
{
  RString rs=oper1;
  return SetCurrentDirectory(rs)!=0;
}

/*
static GameValue getBinaryData(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  GdIOStream *stream=static_cast<GdIOStreamV *>(oper1.GetData())->GetStream();
  int count=toLargeInt(oper2);
  GameArrayType res;
  for (int i=0;i<count;i++)
  {
    //code below did nothing, it caused compilation warning
    //unsigned char z;
  }
  return GameValue();
}

static GameValue putBinaryData(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  return GameValue();
}
*/

#define OPERATORS_DEFAULT(XX, Category) \
  XX(EvalType_IOStream, "<<",function, writeStream,EvalType_IOStream, GameString, "stream","text", "Writes text into the stream.", "stream<<\"Hello \"<<\"world.\"", "", "", "", Category) \
  XX(GameString, "inForm",functionFirst, inFormNum ,GameScalar, GameString, "number","\"format\"", "Formats number to string. format is string, and use the same form as format in printf function in C/C++ language (format specification for %f)", "3.15 inForm \"10.3\"", "\"000003.150\"", "", "", Category) \
  XX(GameString, "inForm",functionFirst, inFormText ,GameString, GameString, "text","\"format\"", "Formats string to string. format is string, and use the same form as format in printf function in C/C++ language (format specification for %s)", "\"Ahoj\" inForm \"-10.3\"", "\"Aho       \"", "", "", Category) \
  XX(GameBool, "exploreFor",function, regexpExplore ,EvalType_IOStream, GameString, "stream","\"pattern\"", "Explores input stream for pattern. Regular expression is used. It doesn't affect state of input stream, so multiple explores can be called, until request pattern match. More informations about regular expressions at www.regexp.cz", "stream exploreFor \"-?[0-9]+\"", "returns true, if streams contain any integer number", "", "", Category) \
  XX(GameBool, "skipUntil",function, SkipUntil,EvalType_IOStream, GameString, "stream","\"pattern\"", "Explores input stream for pattern. Regular expression is used. It doesn't affect state of input stream, so multiple explores can be called, until request pattern match. More informations about regular expressions at www.regexp.cz", "stream exploreFor \"-?[0-9]+\"", "returns true, if streams contain any integer number", "", "", Category) \
  XX(GameString, "get", function, getStreamData,EvalType_IOStream,GameScalar, "stream","count", "Get count of characters from stream. If count &lt;=0 than gets 'last tested characters'+count. For example, -1 will get 5 characters, if 6 characters was tested.","if (stream exploreFor \"-?[0-9]+\") then {result=stream get 0;}; ","number in stream","", "", Category) \
/*  XX(GameArray, "getBin", function, getBinaryData,EvalType_IOStream,GameScalar, "stream","count", "Gets count binary characters. Data are stored in array of scalars","","","", "", Category) */\
/*  XX(GameBool, "putBin", function, putBinaryData,EvalType_IOStream,GameArray, "stream","data", "Puts array of binary characters. Data are stored in array of scalars","","","", "", Category) */\
  XX(EvalType_IOStream, "ignore", function,ignoreStreamData,EvalType_IOStream,GameScalar, "stream","count", "ignores count of characters from stream. If count &lt;=0 than ignores 'last tested characters'+count. For example, -1 will ignores 5 characters, if 6 characters was tested.","if (stream exploreFor \"-?[0-9]+\") then {stream ignore 0;}; //ignore number ","","", "", Category) \
  XX(GameBool, "testXMLTag", function,testForXMLTag,EvalType_IOStream,GameString, "stream","tag", "Test, if input contains xml tag &lt;tag [param=\"value\"]&gt; If tag starts with '/', it will test for end tag. Return is integer with defined meaning: 0 - test failed, 1 - test passed, and result is open or close tag, 2 - test passed, and result is single tag (not for close tag)","res=testXMLTag \"name\"","1 = stream contains <name ... &gt;","", "", Category) \
  XX(GameBool, "copyFileTo", function,copyFile,GameString,GameString, "file1","file2", "Copies one file into another. Function will fail, if target file exists. Use deleteFile, to delete target file, or use backupFile to create backup of target file.","_result=\"test1.txt\" copyFileTo \"test2.txt\"","Returns true, if successful. ","", "", Category) \
  XX(GameBool, "shell", function,shellRun,GameString,GameString, "file1","command", "Executes command on shell with specified file (URLs is also allowed). `command` is command defined in registers. Use empty string \"\" for default (\"open\"). Return is true, if shell executes command without error","\"note.txt\" shell \"print\";","true, if successfull. Prints note.txt on printer, using application registered in explorer. ","", "", Category) \
  XX(GameNothing, "seekOutput", function,seekOutput,EvalType_IOStream,GameScalar, "stream","position", "This command moves writting pointer (current writting position) in static stream (file or memory stream) at specified position. Position is calculated in bytes from the beginning of stream. Note: File-streams shares one pointer for both directions. Moving writting pointer will also move reading pointer. Note: non-static stream doesn't support this command. If it used, sets stream into error state.","","","", "", Category) \
  XX(GameNothing, "seekInput", function,seekInput,EvalType_IOStream,GameScalar, "stream","position", "This command moves reading pointer (current reading position) in static stream (file or memory stream) at specified position. Position is calculated in bytes from the beginning of stream. Note: File-streams shares one pointer for both directions. Moving reading pointer will also move reading pointer. Note: non-static stream doesn't support this command. If it used, sets stream into error state.","","", "", "",Category) \
  XX(GameBool,"renameFileTo", function, renameFile, GameString, GameString,"a","b", "Renames one file into another.", "", "", "", "", Category) \
  

#define FUNCTIONS_DEFAULT(XX, Category) \
  XX(EvalType_IOStream, "openFile", openFile, GameArray ,"[\"filename\",mode]", "Opens or creates a file specified by filename. Mode contain one of following values: 0 - test existence of file (no open), 1 - open existing file for reading, 2 - create new file for writting, 3 - open existing file for reading and writting, 4 - open (create if not exists) file for reading and writting", "", "", "", "", Category) \
  XX(EvalType_IOStream, "openNetworkStream", openNetworkStream, GameArray, "[\"server\",timeout]", "Creates network stream to another computer, server is string in format address:port. Timeout specified waiting for connection/data in miliseconds.", "", "", "", "",Category) \
  XX(EvalType_IOStream, "openNetworkServer", openNetworkServer, GameArray, "[port,timeout]", "Creates server for accepting connection. Another computer may connect to the current computer. Script halts processing for timeout period and waits for connection. Timeout also specified any other waitings", "", "", "","", Category) \
  XX(EvalType_IOStream, "openMemoryStream", openMemoryStream, GameString, "\"input\"", "Creates stream in memory, but acts as any other I/O device. You can specify input text, that will be read. Data written into the stream may be later read using getBuffer function.", "", "", "", "",Category) \
  XX(EvalType_IOStream, "openHybridStream", openHybridStream, GameArray, "[input,output]", "Creates stream that combines two other streams. Data readed using this stream is readed from `input` stream, and data written to this stream is written to the `output` stream.", "file=openFile [\"pokus\",1];console=openStandardIO;hybrid=openHybridStream [file,console]", "bidirectional stream that reads data from file a writes data to standard output", "", "",Category) \
  XX(EvalType_IOStream, "eatWS",eatWaitSpaces, EvalType_IOStream, "stream", "Ignores all waitspace (spaces, tabs, new-lines etc), unitil any other character found, or end of file reached.", "eatWS stream exploreFor \"[_a-zA-Z][_a-zA-Z0-9]*\"", "true, if stream contain identifier. Before test, all waitspaces is skipped", "", "",Category) \
  XX(EvalType_IOStream, "eatWSEOL",eatWaitSpacesEOL, EvalType_IOStream, "stream", "Ignores all waitspace u(spaces, tabs, etc), unitil any other character found, or end of line or end of file reached. End of line delimiter is not skipped, but it can be skipped by ignore command", "eatWSEOL stream exploreFor ignore 0", "eats whitespace until new line, then new line delimiter is skipped", "", "",Category) \
  XX(GameBool,"testNumber",testForNumber,EvalType_IOStream,"stream","Tests input stream for number, returns true, if input stream contains number. It doesn't read the number, so if script calls this function repeatly, always return last same result, until number is readed by function get","if (testNumber stream) then {_value=val (_stream get 0);}","","","",Category) \
  XX(GameBool,"testNumberInt",testForNumberInt,EvalType_IOStream,"stream","Tests input stream for integer number, returns true, if input stream contains number. It doesn't read the number, so if script calls this function repeatly, always return last same result, until number is readed by function get","if (testNumberInt stream) then {_value=val (_stream get 0);}","","","",Category) \
  XX(GameBool,"testIdentifier",testForIdentifier,EvalType_IOStream,"stream","Tests input stream for identifier. Identitfier is world starting with alpha character or underscore '_' and can contain any alphanumeric characters or underscores","","","","",Category) \
  XX(GameArray,"getXMLTag",GetXMLTag,EvalType_IOStream,"stream","Return parsed XML tag. Before using this function, you must test stream for presence of tag! use testXMLTag function.","if (stream testXMLTag \"name\") then {tag=getXMLTag stream;}","array [\"nameOfTag\",\"firstAttrib\",\"firstAttribValue\",\"secondAttrib\",\"secondAtrribValue\", ... etc.]","","",Category) \
  XX(GameBool,"eof",testEof,EvalType_IOStream,"stream","Returns true, if stream reached end.","","","","",Category) \
  XX(GameBool,"!",testError,EvalType_IOStream,"stream","Returns true, if an error has been reported during operation with stream.","","","","",Category) \
  XX(GameBool,"deleteFile",deleteFile,GameString,"file","Deletes file. Returns true, if operation was successful","deleteFile \"test.txt\"","","","",Category) \
  XX(GameString,"backupFile",backupFile,GameString,"file","Renames file to file.bak. It useful before saving new version of file.Returns name of backup file. Note: It returns backup name even if original file doesn't exists (this means when there is nothing to backup). Operator returns empty string, when backup cannot be created due an error (file is R/O, cannot delete old backup, cannot move original file, disk integrity error etc.)","_backup= backupFile nameOf object;if (_backup!="") then {save object;};", "","","",Category) \
  XX(GameBool,"createFolder",createFolder,GameString,"folderName","Creates new folde. You can specify full pathname for new folder, but all referenced folders specified in path must exist (of course, except last - name of folder)","_result = createFolder \"p:\\ofp\\testing\\\"", "","","",Category) \
  XX(GameBool,"createFolderEx",createFolderEx,GameString,"folderName","Creates new folder. You can specify full pathname for new folder. If specified path doesn't exists, creates it","_result = createFolderEx \"p:\\ofp\\testing\\\"", "","","",Category) \
  XX(GameScalar,"shellCmd",shellCmd,GameString,"command","Executes shell command as if were written in shell's command line. Function waits until command is finished","_result = shellCmd \"notepad.exe\"", "-1 if file not found, or cannot be started, otherwise returns application's exit code ","","",Category) \
  XX(GameScalar,"shellCmdNoWait",shellCmdNoWait,GameString,"command","Executes shell command as if were written in shell's command line. Function continues immediatelly","", "-1 if file not found, or cannot be started, otherwise returns application's exit code ","","",Category) \
  XX(EvalType_IOStream,"shellCmdOpenPipe",shellCmdOpenPipe,GameString,"command","Executes shell command as if were written in shell's command line. Creates bi-directional pipe to the new process. Script can use this pipe to send commands to child process and read its results","_pipe=shellCmdOpenPipe \"dir /b\"", "stream contains files in current folder, each on new line","","",Category) \
  XX(GameString,"getLine",getLine,EvalType_IOStream,"stream","Reads whole line. Return string without new-line delimiter.","_line=getLine console;", "whole line","","",Category) \
  XX(GameString,"getBuffer",getBuffer,EvalType_IOStream,"stream","Retrieves buffer output from memory stream. Data sent through memory stream are stored in buffer. The buffer can be converted to the String using this command. Note: Function returns NIL if parameter is not memory stream","_data=getBuffer stream", "content of buffer of the memory stream","","",Category) \
  XX(GameScalar,"tellInput",tellInput,EvalType_IOStream,"stream","Retrieves current reading pointer for static stream. Returns -1, if stream is non-static","", "","","",Category) \
  XX(GameScalar,"tellOuput",tellOutput,EvalType_IOStream,"stream","Retrieves current writting pointer for static stream. Returns -1, if stream is non-static","", "","","",Category) \
  XX(GameScalar,"fileSize",fileSize,EvalType_IOStream,"stream","Retrieves size of the file in the bytes. Returns -1, if stream is non-static","", "","","",Category) \
  XX(GameBool,"SetCD",SetCurrentDir,GameString,"dir","Changes current working directory. You can use absolute or relative path. Returns true, if directory was changed","", "","","",Category) \



#define NULARS_DEFAULT(XX, Category) \
  XX(EvalType_IOStream, "openStandardIO",openStandardIO, "Opens stream for standard input and output. If no standard I/O is assigned, creates console, and redirect I/O into console. Note: Always assign result to variable, and access stream with variable.", "console=openStandardIO; console<<\"Hallo world\"", "", "", "", Category) \
  XX(EvalType_IOStream, "openErrorIO",openErrorIO, "Opens stream for standard input and error output . If no standard I/O is assigned, creates console, and redirect I/O into console. Note: Always assign result to variable, and access stream with variable.Use error stream to displaying error messages, that shouldn't be included in standard output", "errorcons=openErrorIO; errorcons<<\"An error has occured.\"", "", "", "", Category) \
  XX(GameString, "eoln",EndOfLine, "Returns end of line for streams. It also can be used with strings. Result is \\r\\n", "stream << \"line1\" << eoln << \"line2\" << eoln", "", "", "", Category) \
  XX(GameString, "homeFolder",homeFolder, "Returns path of hosting module", "homeFolder", "c:\\bis\\Oxygen\\", "", "", Category) \
  XX(GameString, "hostingApp",hostingApp, "Returns filename of hosting application. It useful with shell commands etc.", "hostingApp", "c:\\bis\\Oxygen\\Objektiv2.exe", "", "", Category) \
  XX(GameString,"GetCD",GetCurrentDir,"Retrieves current working directory","", "","","",Category) \

static GameNular NularyFuncts[]=
{
  NULARS_DEFAULT(REGISTER_NULAR, Category)
};

static GameOperator BinaryFuncts[]=
{
  OPERATORS_DEFAULT(REGISTER_OPERATOR, Category)
};

static GameFunction UnaryFuncts[]=
{
  FUNCTIONS_DEFAULT(REGISTER_FUNCTION, Category)
};

static GameData *CreateIOStream(ParamArchive *ar) 
{
    return new GdIOStreamV();
}

TYPES_IOSTREAM(DEFINE_TYPE, "IOStream")

#if DOCUMENT_COMREF
static ComRefFunc DefaultComRefFunc[] =
{
  NULARS_DEFAULT(COMREF_NULAR, Category)
    FUNCTIONS_DEFAULT(COMREF_FUNCTION, Category)
      OPERATORS_DEFAULT(COMREF_OPERATOR, Category)
};

static ComRefType DefaultComRefType[] =
{
  TYPES_DEFAULT(COMREF_TYPE, Category)
};
#endif



void GdIOStreamV::RegisterToGameState(GameState *gState)
{
  GameState &state = *gState; // helper to make macro works
  TYPES_IOSTREAM(REGISTER_TYPE, "IOStream")
  gState->NewOperators(BinaryFuncts,lenof(BinaryFuncts));
  gState->NewFunctions(UnaryFuncts,lenof(UnaryFuncts));
  gState->NewNularOps(NularyFuncts,lenof(NularyFuncts));
  #if DOCUMENT_COMREF
  gState->AddComRefFunctions(DefaultComRefFunc,lenof(DefaultComRefFunc));
  gState->AddComRefTypes(DefaultComRefType,lenof(DefaultComRefType));
  #endif
}


