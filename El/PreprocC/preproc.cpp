// Preproc.cpp: implementation of the Preproc class.
//
//////////////////////////////////////////////////////////////////////

#include <El/elementpch.hpp>
#include "preproc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
// #define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define LexItem PreprocLexItem

struct LexDef
{
  LexItem item;
  char *text;
};

static LexDef lexdefs[]=
{
  {lxInclude,"include"},
  {lxDefine,"define"},
  {lxIfDef,"ifdef"},
  {lxIfNDef,"ifndef"},
  {lxElse,"else"},
  {lxEndIf,"endif"},
  {lxLeft,"("},
  {lxRight,")"},
  {lxComma,","},
  {lxHash,"#"},	
  {lxNewLine,"\n"},
  {lxBeginLineComment,"//"},
  {lxBeginBlockComment,"/*"},
  {lxLineBreak,"\\\n"},
  {lxUhozy,"\""},
  //	{lxNoUhozy,"\\\""},
  {lxLevaZlomena,"<"},
  {lxPravaZlomena,">"},
  {lx2Hash,"##"},
  {lxUndef,"undef"}
};

static LexItem FindLex(const char *name)
{
  for (int i=0;i<sizeof(lexdefs)/sizeof(LexDef);i++)	
    if (strcmp(name,lexdefs[i].text)==0) return lexdefs[i].item;
  return lxUnknown;
}

static char validIdChar(char chr, bool firstchar)
{
  if (firstchar)
    return (chr>='a' && chr<='z' || chr>='A' && chr<='Z' || chr=='_');
  else
    return validIdChar(chr,true) || (chr>='0' && chr<='9');
}


class PreprocStreamReader
{
  QIStream &_s;
  int _buffer[10];
  int _ugetcnt;
  int _addlines;
public:
  PreprocStreamReader(QIStream &stream):_s(stream) {_ugetcnt=0;_addlines=0;}
  int get()
  {
    if (_ugetcnt) return _buffer[--_ugetcnt];
    else return _s.get();
  }
  void unget(int znk)
  {
    Assert(_ugetcnt<lenof(_buffer));
    _buffer[_ugetcnt++]=znk;
  }
  int gets()
  {
    int i=get();
    while (i=='\r') i=get();
    while (i=='\\')
    {
      int j=get();
      while (j=='\r') j=get();
      if (j!='\n') {unget(j);return i;}
      i=get();
      _addlines++;
      while (i=='\r') i=get();
    }
  return i;
  }

  int GetExtraLinesCount()
  {
    return _addlines;
  }

  void ResetExtraLinesCount()
  {
    _addlines=0;
  }
};


static char *scanName(PreprocStreamReader &in, char *buffer, int size)
{
  int i=in.gets();
  int p=0;
  bool first=true;
  size--;
  while (p<size && validIdChar((char)i,first))
    {buffer[p++]=(char)i;first=false;i=in.gets();}
  if (i!=EOF) in.unget(i);
  buffer[p++]=0;
  return buffer;
}

static char *scanString(PreprocStreamReader &in, char *buffer, int size, const char *terminators)
{
  int i=in.get();
  int p=0;
  bool first=true;
  size--;
  while (p<size && i!=EOF && strchr(terminators,i)==NULL)	
  {buffer[p++]=(char)i;first=false;i=in.get();}
  if (i!=EOF) in.unget(i);
  buffer[p++]=0;
  return buffer;
}

static void SkipWhites(PreprocStreamReader &in)
{
  int i;
  do
  {i=in.get();}
  while (i<33 && i!=EOF && i!='\n');
  if (i!=EOF) in.unget(i);
}

static LexItem GetNext(PreprocStreamReader& in, char *buffer, int size)
{
  int i=in.gets();
  while (i == 0x0d) i = in.gets();
  if (i==EOF) return lxEof;
  if (validIdChar((char)i,true))
  {    
    in.unget(i);
    scanName(in,buffer,size);
    LexItem it=FindLex(buffer);
    if (it==lxUnknown) return lxText;
    else return it;
  }
  buffer[0]=i;
  buffer[1]=0;
  if (i=='\\' || i=='/')
  {
    int i=in.gets();
    while (i == 0x0d) i = in.gets();
    if (!validIdChar((char)i,true))
    {
      buffer[1]=i;
      buffer[2]=0;
      if (FindLex(buffer)==lxUnknown)
      {
        in.unget(i);
        buffer[1]=0;
      }
    }
    else
      in.unget(i);
  }
  else if (i=='#')
  {
    i=in.gets();
    if (i!='#') in.unget(i);else {buffer[1]=i;buffer[2]=0;}
  }
  LexItem it=FindLex(buffer);
  return it;
}

Preproc::Preproc():filename("")
{
  out=NULL;
  maclist=NULL;
  recurse=0;
  maxrecurse=20;
}

void Preproc::MacroParam::SetValue(const char *val)
{
  if (!val) val="";
  if (value) free(value);
  value=strDup(val);
}


void Preproc::DefineSymb::AddParam(const char *name, int poradi)
{
  char val[32];
#ifdef _WIN32
  itoa(poradi,val,10);
#else
  sprintf(val,"%d",poradi);
#endif
  MacroParam parm(name),por(val);
  por.SetValue(name);  
  params.Add(parm);
  params.Add(por);
  exceptsParams=true;
}

bool Preproc::DefineSymb::SetParam(int poradi, const char *text,MacroParams& parlist)
{
  char val[32];
#ifdef _WIN32
  itoa(poradi,val,10);
#else
  sprintf(val,"%d",poradi);
#endif
  MacroParam &parname=params[val];
  if (params.IsNull(parname)) return false;
  MacroParam newpar(parname.GetValue());
  newpar.SetValue(text);
  parlist.Add(newpar);
  return true;
}


const char *Preproc::DefineSymb::GetParam(const char *name)
{
  const MacroParam &par=params[name];
  if (params.IsNull(par)) return NULL;
  return par.GetValue();
}

bool Preproc::Process(QOStream *out,const char *name)
{
  this->out=out;
  curline=0;
  error=prNoError;
  filename=name;
  QIStream *in=OnEnterInclude(name);
  if (in==NULL) 
  {
    RptF("Cannot include file %s",name);
    error=prStreamOpenError;
    return false;
  }
  item = lxNewFile;
  AtBeginLine(*out);
  PreprocStreamReader helper(*in);
  bool ret=GlobalScan(helper,out);
  OnExitInclude(in);
  return ret;
}


static int SkipBlockComment(PreprocStreamReader &in, QOStream *out)
{
  int lines=0;
  int last=0;
  int i=in.get();
  while (i!=EOF && (last!='*' || i!='/'))
  {
    if (i=='\n') {lines++; if (out) *out << "\n";}
    last=i;i=in.get();
  }
  return lines;
}

static void SkipLineComment(PreprocStreamReader &in)
{
  int i=in.get();
  while (i!=EOF && i!='\n') i=in.get();
  if (i!=EOF) in.unget(i);
}

bool Preproc::GlobalScan(PreprocStreamReader &in,QOStream *out)
{    
  bool uvozovky=false;
  bool ok=true;  
  for(;;)
  {
    if (item==lxEof) return true;
    if (item==lxUhozy) uvozovky=!uvozovky;
    if (uvozovky) {if (out) (*out)<<text;ReadNext(in);}
    else
    {
      bool newLine = item == lxNewLine;
      if (newLine || item == lxNewFile)
      {
        if (out)
        {
          int addlines=in.GetExtraLinesCount();
          curline+=addlines;
          while (addlines) {*out << "\n"; addlines--;}
          in.ResetExtraLinesCount();
        }
        SkipWhites(in);
        ReadNext(in);
        if (out && newLine) out->put('\n');
        if (item==lxHash)
        {
          ReadNext(in);SkipWhites(in);
          switch (item)
          {
          case lxInclude:if (out) {ReadNext(in); ok=DoIncludeBlock(in);}break;
          case lxDefine:if (out) {ReadNext(in); ok=DoDefineBlock(in);}break;
          case lxIfDef:if (out) {ReadNext(in); ok=DoIfDefBlock(in,true);}break;
          case lxIfNDef:if (out) {ReadNext(in); ok=DoIfDefBlock(in,false);}break;
          case lxEndIf:
          case lxElse:error=prParseExit;ok=false;break;
          case lxUndef:if (out) {ReadNext(in);ok=DoUndefBlock(in);}break;
          default: error=prInvalidPreprocessorCommand;ok=false;break;
          }
          if (!ok) return false;
        }        
      }
      else if (item==lxBeginBlockComment) {curline+=SkipBlockComment(in,out);ReadNext(in);}
      else if (item==lxBeginLineComment) {SkipLineComment(in);ReadNext(in);}
      else if (item==lxText)  {if (out) TryExpandMacro(in,*out);else ReadNext(in);}
      else 
      {if (out) (*out)<<text;ReadNext(in);}
    }
  }
}

void Preproc::ReadNext(PreprocStreamReader& in)
{    
  item=GetNext(in,text,sizeof(text));  
  //  printf("%d - %d - %s\n",curline,item,text);
  if (strchr(text,'\n')!=NULL) {curline++;AtBeginLine(*out);} 
}

bool Preproc::DoIncludeBlock(PreprocStreamReader &in)
{
  if (item!=lxUhozy && item!=lxLevaZlomena)
  {
    error=prIncludeError;
    return false;
  }
  const char *del=(item==lxUhozy)?"\"":">";
  scanString(in,text,sizeof(text),del);
  int cline=curline;
  RString p=filename;
  if (Process(out,text)==true)
  {
    ReadNext(in);  
    curline=cline;
    filename=p;
    ReadNext(in);
    return true;
  }
  return false;  
}

void Preproc::DefineDefine(const char *text, const char *value)
{
  DefineSymb defs(text,value);
  deftable.Add(defs);
  DefineSymb &ddf=deftable[text];
  ddf.Unblock();
}

bool Preproc::DoDefineBlock(PreprocStreamReader &in)
{
  DefineSymb defs(text,"");
  deftable.Add(defs);
  DefineSymb &ddf=deftable[text];
  //	SkipWhites(in);
  ReadNext(in);
  if (item==lxLeft)
  {
    ddf.SetEmptyParams();
    ReadDefineParams(in,ddf);
    if (item!=lxRight)
    {
      error=prDefineError;
      return false;
    }
    SkipWhites(in);
    ReadNext(in);
  }
  if (text[0]==32) {SkipWhites(in);ReadNext(in);}
  ReadDefineText(in,ddf);
  ddf.Unblock();
  return true;
}

Preproc::DefineSymb *Preproc::CreateExpandMacro(PreprocStreamReader &in, MacTableList **params)
{
  DefineSymb& smb=deftable[text];
  if (deftable.IsNull(smb) || smb.Blocked()) return NULL;
  ReadNext(in);
  *params=NULL;
  if (smb.HasParams())
  {
    if (item==lxLeft)
    {
      int p=0;
      ReadNext(in);
      if (item!=lxRight)
      {
        *params=new MacTableList();
        RString g;
        bool end;
        do
        {
          end=!LoadMacroParam(g,in);
          if (smb.SetParam(p++,g.Data(),**params)==false)
          {
            error=prToManyParameters;delete *params;return NULL;
          }
        }
        while (!end);
      }
      else 
      {
        ReadNext(in);
      }
      if (smb.SetParam(p,"",**params)==true)
      {
        error=prToFewParameters;delete *params;return NULL;
      }
    }   
    else
    {
      error=prToFewParameters;delete *params;return NULL;
    }
  }
  return &smb;
} 

bool Preproc::LoadMacroParam(RString &p, PreprocStreamReader &in)
{
  error=prNoError;
  int zavorka=0;
  bool uvozovky=false;  
  QOStrStream out;
  for(;;)
  {
    switch (item)
    {
    case lxLeft: if (!uvozovky)zavorka++;out<<text;break;
    case lxRight: if (!uvozovky) zavorka--;
      if (zavorka<0) {ReadNext(in);out.put(0);p=out.str();return false;}
      else out<<text; break;
    case lxUhozy: uvozovky=!uvozovky;out<<text;break;
    case lxComma:
      if (zavorka==0 && uvozovky==false)
      {
        ReadNext(in);
        out.put(0);
        p=out.str();
        return true;
      }
      break;	  
    case lxEof: error=prUnexceptedEndOfFile;return false;
    case lxText: if (!uvozovky) 
                 {
                   if (TryExpandMacro(in,out)==false) return false;
                   continue;
                 }
                 else out<<text;break;
    default : out<<text;
    }
    ReadNext(in);
  }

}

void Preproc::ReadDefineParams(PreprocStreamReader &str, DefineSymb &def)
{
  SkipWhites(str);
  ReadNext(str);
  int count=0;
  while (item==lxText)
  {
    def.AddParam(text,count);
    count++;
    SkipWhites(str);
    ReadNext(str);
    if (item==lxComma) 
    {
      SkipWhites(str);
      ReadNext(str);
    }
  }
}

void Preproc::ReadDefineText(PreprocStreamReader &str, DefineSymb &def)
{
  QOStrStream sout;
  while (item!=lxNewLine && item!=lxEof)
  {
    if (item==lxBeginLineComment) SkipLineComment(str);
    else if (item==lxBeginBlockComment) curline+=SkipBlockComment(str, out);
    else if (item!=lxLineBreak) {sout<<text;}
    ReadNext(str);
  }
  sout.put(0);
  def.SetValue(sout.str());  
}

bool Preproc::TryExpandMacro(PreprocStreamReader& in, QOStream &out)
{
  if (text[0]=='_')
  {
    if (strcmp(text,"__FILE__")==0) 
      {out<<"\""<<filename<<"\"";ReadNext(in);return true;}
    if (strcmp(text,"__LINE__")==0)
    #ifdef _WIN32
      {out<<itoa(curline,text,10);ReadNext(in);return true;}
    #else
      {sprintf(text,"%d", curline);out<<text;ReadNext(in);return true;}
    #endif 
  }
  const MacroParam &par=maclist->GetFromList(text);
  if (maclist->NotNull(par)) 
    {out<<par.GetValue();ReadNext(in);return true;}
  LexItem last=lxUnknown;
  MacTableList *params; //parametry tohoto makra
  DefineSymb *fnd=CreateExpandMacro(in,&params);
  bool uvozovky=false;
  if (fnd==NULL) {out<<text;ReadNext(in);return error==prNoError;}
  if (params) maclist=maclist->Add(params); //vloz parametry do listu parametru
  fnd->Block();
  const char *expand=fnd->GetValue();
  QIStrStream local(expand,strlen(expand));
  PreprocStreamReader localhelper(local);
  LexItem it=item;
  char tt[128];
  strncpy(tt,text,128);
  ReadNext(localhelper);
  while (item!=lxEof)
  {	
    if (item==lxUhozy)
      uvozovky=!uvozovky;
    if (uvozovky) {out<<text;ReadNext(localhelper);}
    else if (item==lxText)
    {
      LexItem itsave=item;
      if (last==lxHash) out.put('"');
      TryExpandMacro(localhelper,out);	  
      if (last==lxHash) out.put('"');
      last=itsave;
      //	  ReadNext(local);
    }
    else 
    {
      if (item!=lx2Hash && item!=lxHash) out<<text;
      last=item;
      ReadNext(localhelper);
    }
  }
  item=it;
  strncpy(text,tt,128);
  fnd->Unblock();
  if (params) maclist=maclist->Remove();  
  return error==prNoError;
}

bool Preproc::DoIfDefBlock(PreprocStreamReader &in, bool cond)
{	
  if (item!=lxText)
  {
    error=prUnexceptedSymbol;return false;
  }
  DefineSymb &def=deftable[text];
  bool skip=deftable.IsNull(def);
  if (!cond) skip=!skip;
  ReadNext(in);
  if (skip) 	
    GlobalScan(in,NULL);else GlobalScan(in,out);
  if (error!=prParseExit)
  {return false;}
  error=prNoError;
  if (item==lxElse)
  {
    ReadNext(in);
    if (skip)
      GlobalScan(in,out);else GlobalScan(in,NULL);
    if (error!=prParseExit)
    {return false;}
  }
  if (item!=lxEndIf)
  {
    error=prEndIfExcepted;
    return false;
  }
  ReadNext(in);
  return true;
}

bool Preproc::DoUndefBlock(PreprocStreamReader &str)
{
  if (item!=lxText)
  {
    error=prUnexceptedSymbol;return false;
  }
  deftable.Remove(text);
  ReadNext(str);
  return true;
}

