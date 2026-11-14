// preproc.h: interface for the Preproc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PREPROC_H__E6DF0FA5_8BA9_4E88_94E8_F3BDDEAE5346__INCLUDED_)
#define AFX_PREPROC_H__E6DF0FA5_8BA9_4E88_94E8_F3BDDEAE5346__INCLUDED_

#ifdef _MSC_VER
#pragma once
#endif

#include <El/QStream/qStream.hpp>

#include <Es/Containers/hashMap.hpp>


#define TypeHasGenericDestructorEx(Type,destruct,decl) \
	decl void Destruct( Type &dst ) {(&dst)->~destruct();} \
	decl void DestructArray( Type *dst, int n ) \
	{for( int i=0; i<n; i++ ) (&(dst[i]))->~destruct();}

#define TypeIsMovableDeclEx(Type,destruct,decl) \
	TypeHasGenericConstructor(Type,decl) \
	TypeHasGenericDestructorEx(Type,destruct,decl) \
	TypeHasGenericCopy(Type,decl) \
	TypeHasBinaryMove(Type,decl)


enum PreprocessError
  {
  prNoError,
  prStreamOpenError, //OnEnterInclude vratil NULL
  prIncludeError, //Spatna syntaxe include
  prIncludeMaxRecursion, //Maximalni pocet rekurzi byl dosazen
  prDefineError, //Spatna syntaxe define
  prDefineParamError, //Spatna syntaxe parametru
  prParseExit, //Interni
  prInvalidPreprocessorCommand, //preprocesor nerozpoznal prikaz
  prUnexceptedEndOfFile,
  prToManyParameters,
  prToFewParameters,
  prUnexceptedSymbol, 
  prEndIfExcepted,  
  };


enum PreprocLexItem
  {
  lxInclude,
  lxDefine,
  lxIfDef,
  lxIfNDef,
  lxElse,
  lxEndIf,
  lxLeft,
  lxRight,
  lxComma,
  lxHash,
  lxNewFile,
  lxNewLine,
  lxLineBreak,
  lxBeginLineComment,
  lxBeginBlockComment,
  lxText,
  lxUnknown,
  lxEof,
  lxUhozy,
  lxNoUhozy,
  lxLevaZlomena,
  lxPravaZlomena,
  lx2Hash,
  lxUndef,
  };


class PreprocStreamReader;


/// implementation of subset of c preprocessor
class Preproc: private NoCopy
  {
  protected:
  QOStream *out;
  
  public:

  class Symbol
	{
	char *key;
	public:
	  Symbol() {key=strDup("00NULL00");}
	  Symbol(const char *x) {key=strDup(x);}
	  Symbol(const Symbol &other) {key=strDup(other.key);}
	  Symbol& operator=(const Symbol &other)
    {
      char *nkey=strDup(other.key);
      free(key);
      key = nkey;
      return *this;
    }
	  virtual ~Symbol() {free(key);key = NULL;}
	  const char *GetKey() const {return key;}

      bool operator == ( const Symbol &src ) const
		{
		return strcmp(key,src.key)==0;
		}
      bool operator != ( const Symbol &src ) const
		{
		return strcmp(key,src.key)!=0;
		}
	};

	
	class MacroParam:public Symbol
	  {
	  char *value;
	  public:
		MacroParam():Symbol() {value=NULL;}
		MacroParam(const char *x) : Symbol(x) {value=NULL;}
		MacroParam(const MacroParam &other):Symbol(other) {value=strDup(other.value);}
		MacroParam& operator=(const MacroParam &other)
    {
      Symbol::operator=(other);
      char *nvalue=strDup(other.value);
      free(value);
      value = nvalue;
      return *this;
    }
		virtual ~MacroParam() {free(value); value = NULL;}
  		void SetValue(const char *val);	  
		const char *GetValue() const {return value;}	

		ClassIsMovable(MacroParam)
	  };


	typedef MapStringToClass<MacroParam, AutoArray<MacroParam> > MacroParams;

	class DefineSymb:public MacroParam
	  {
	  MacroParams params;
	  int blocked;
      bool exceptsParams;
	  public:
		DefineSymb():MacroParam() {blocked=1;exceptsParams=false;}
		DefineSymb(const char *x,const char *expand):MacroParam(x)
		  {
		  SetValue(expand);
		  blocked=1;
          exceptsParams=false;
		  }
          DefineSymb(const DefineSymb &other):MacroParam(other),params(other.params),blocked(other.blocked),exceptsParams(other.exceptsParams) {}		
		DefineSymb& operator=(const DefineSymb &other) 
		  {params=other.params;MacroParam::operator=(other);return *this;}

	  public: 
		const char *GetParam(const char *name);
		void AddParam(const char *name, int poradi);
  		bool SetParam(int poradi, const char *text, MacroParams& parlist);
        void SetEmptyParams() {exceptsParams=true;}
		void Block() {blocked++;}
		void Unblock() {if (blocked>0) blocked--;}
		bool Blocked() {return blocked!=0;}
        bool HasParams() {return exceptsParams;}
     

		ClassIsMovable(DefineSymb)
	  };

  typedef MapStringToClass<DefineSymb, AutoArray<DefineSymb> > DefTable;

  class MacTableList:public MacroParams
	{
	public:
	  MacTableList *next;
	  MacTableList():MacroParams() {next=NULL;}
	  const MacroParam& GetFromList(const char *name)
		{
		if (this==NULL) return _null;
		const MacroParam &out=Get(name);
		if (IsNull(out) && next!=NULL) return next->GetFromList(name);
		return out;
		}
	  MacTableList *Add(MacTableList *item) {item->next=this;return item;}
	  MacTableList *Remove() {MacTableList *out=next;next=NULL;delete this;return out;}
	  virtual ~MacTableList() {delete next;}
	};

  protected:
	int recurse;
	DefTable deftable;  //tabulka #define
	MacTableList *maclist; //seznam parametru pro expanzi makra (lokalni vyhledavaci tabulky)
	
	PreprocLexItem item;
	char text[128];
  public:
	int curline;
	RString filename;  
	PreprocessError error;
	int maxrecurse;
	
  public:
  Preproc();
	bool Process(QOStream *out,const char *name);
	/// introdude a global define
  void DefineDefine(const char *name, const char *value);

  protected:
	virtual void AtBeginLine(QOStream &out) {}
	virtual QIStream *OnEnterInclude(const char *filename)=0;
	virtual void OnExitInclude(QIStream *stream)=0;

  private:
	bool DoIfDefBlock(PreprocStreamReader &in, bool cond);
	bool TryExpandMacro(PreprocStreamReader& in,QOStream &out);
	void ReadDefineText(PreprocStreamReader &str, DefineSymb& def);
	void ReadDefineParams(PreprocStreamReader &str,DefineSymb& def);
	bool LoadMacroParam(RString& p,PreprocStreamReader &in);
    DefineSymb * CreateExpandMacro(PreprocStreamReader &in, MacTableList **params); //text obsahuje jmeno makra
  // process define content
	bool DoDefineBlock(PreprocStreamReader &in);
	bool DoIncludeBlock(PreprocStreamReader& in);
	void ReadNext(PreprocStreamReader& in);
	bool GlobalScan(PreprocStreamReader& in,QOStream *out);
	bool DoUndefBlock(PreprocStreamReader &str);
  };


//TypeIsMovableDeclEx(Preproc::MacroParam,MacroParam,inline)
//TypeIsMovableDeclEx(Preproc::DefineSymb,DefineSymb,inline)



#endif // !defined(AFX_PREPROC_H__E6DF0FA5_8BA9_4E88_94E8_F3BDDEAE5346__INCLUDED_)
