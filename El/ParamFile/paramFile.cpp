// parameter file parser

#if _DEBUG
#pragma optimize ("gt",on)
#endif

#include <El/elementpch.hpp>
#include "paramFile.hpp"
// #include "loadStream.hpp"
#include <Es/Framework/appFrame.hpp>
#include <Es/Files/filenames.hpp>
#include <Es/Strings/bString.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/QStream/serializeBin.hpp>
#include <Es/Containers/staticArray.hpp>
#include <ctype.h>

/// growing string
typedef TempString<1024> WordBuf;

#ifdef _DEBUG
#define DIAG_OWNER 0
#define DIAG_UNLOAD 100
#else
#define DIAG_OWNER 0
#define DIAG_UNLOAD 0
#endif

#ifndef _SUPER_RELEASE
#define LOG_CHECKSUM 0
#endif

static bool ProcessLineNumber(QIStream &in);

static void ParsingError(QIStream &in, const char *format, ...)
{
  va_list arglist;
  va_start(arglist, format);

  BString<512> buf;
  vsprintf(buf, format, arglist);
  
  FileLocation loc = in.GetLocation();

  ErrorMessage("File %s, line %d: %s", cc_cast(loc.fileName), loc.line, cc_cast(buf));

  va_end(arglist);
}

static void ParsingWarning(QIStream &in, const char *format, ...)
{
  va_list arglist;
  va_start(arglist, format);

  BString<512> buf;
  vsprintf(buf, format, arglist);

  FileLocation loc = in.GetLocation();

  RptF("File %s, line %d: %s", cc_cast(loc.fileName), loc.line, cc_cast(buf));

  va_end(arglist);
}

/*!
\param buf returned string
\param buf max. returned string string
\param termin list of possible terminators
\param quot if used, returns if the string was quoted or not
*/

static bool GetWord( WordBuf &buf, QIStream &in, const char *termin, bool *quot = NULL )
{
  //char buf[2048];
  buf.Resize(0);

  int c = in.get();
  // LTrim the word
  while (isspace(c)) c = in.get();

  if (c=='"')
  {
    if (quot) *quot = true;
    c=in.get();
    for(;;)
    {
      if (c=='"')
      {
        c=in.get();
        if (c != '"')
        {
          // search for \n
          while (isspace(c)) c = in.get();
          if (c != '\\')
          {
            in.unget();
            buf.Add(0);
            return true; // word parsed
          }
          c = in.get();
          if (c != 'n')
          {
            ParsingError(in, "Config: Unsupported escape sequence \\%c after %s", c, cc_cast(buf));
          }
          c = in.get();
          while (isspace(c)) c = in.get();
          if (c != '"')
          {
            ParsingError(in, "Config: '\"' expected after %s", cc_cast(buf));
          }
          buf.Add('\n');
          c=in.get();
          continue;
        }
      }
      if( c=='\n' || c=='\r' )
      {
        ParsingError(in, "Config: End of line encountered after %s", cc_cast(buf));
      }
      buf.Add(c);
      c=in.get();
    }
  }
  else
  {
    if (quot) *quot = false;
    while (!strchr(termin,c) && c != EOF)
    {
      if( c=='\n' || c=='\r' )
      {
        // word terminated - only white spaces or termin now
        while (true)
        {
          c = in.get();
          // LTrim the word
          while (isspace(c)) c = in.get();

          if (c != '#') break; // valid word character
          if (!ProcessLineNumber(in))
          {
            ParsingError(in, "Config: Wrong # directive");
            buf.Add(0);
            return false;
          }
        } 
        if( !strchr(termin,c) )
        {
          ParsingError(in, "Config: '%c' after %s", c, cc_cast(buf));
        }
        else in.unget();
        goto Return;
      }
      buf.Add(c);
      c=in.get();
    }
    if (c != EOF) in.unget();
    Return:
    // RTrim the word - remove any trailing spaces
    while (buf.Size()>0 && isspace(buf[buf.Size()-1])) buf.Resize(buf.Size()-1);
    buf.Add(0);
    return buf.Size()>1;
  }
  /*NOTREACHED*/
}

static void GetAlphaWord( WordBuf &buf, QIStream &in )
{
  buf.Resize(0);
  int c=in.get();
  while( isspace(c) ) c=in.get();
  while( isalnum(c) || c=='_' )
  {
    buf.Add(c);
    c=in.get();
  }
  in.unget();
  buf.Add(0);
}

// some global identifiers
static RStringB AccessString("access");

ParamEntry::ParamEntry( const RStringB &name )
//:_overload(false)
{
  if( name.GetLength()>0 ) _name=name;
}

inline void NotClass( const char *cName, const char *eName )
{
  ErrorMessage(EMError,"'%s' is not a class ('%s' accessed)",cName,eName);
}
inline void NotClass( const char *cName )
{
  ErrorMessage(EMError,"'%s' is not a class.",cName);
}

inline void NotDefined( const char *cName, const char *eName )
{
  ErrorMessage(EMError,"class '%s' is not defined ('%s' accessed)",cName,eName);
}
inline void NotDefined( const char *cName)
{
  ErrorMessage(EMError,"class '%s' is not defined",cName);
}


inline void NotValue( const char *eName )
{
  ErrorMessage(EMError,"'%s' is not a value",eName);
}

inline void NotArray( const char *aName )
{
  ErrorMessage(EMError,"'%s' is not an array.",aName);
}

enum SpecValueType
{
  SVGeneric, // generic - string
  SVFloat,
  SVInt,
  SVArray,
  SVExpression,
  NSpecValueType,
  SVInt64,
  // note: char is used to contain values of this type
};


//! class used to return error values
/*!
Pointer to global instance of this type is returned
when given config value cannot be found or is not of expected type.
*/

class ParamEntryError: public ParamClass
{
  public:
  ParamEntryError(){}

  virtual bool IsError() const {return true;}
};

//! global instance of value "error"
static ParamEntryError GParamEntryError;


ParamEntryPtr ParamEntry::FindEntry(
  const char *name, IParamVisibleTest &visible
) const
{
  NotClass(GetContext(),name);
  return ParamEntryPtr();
}

/**
lightweigth test - does not load it. Usefull before requesting.
*/

bool ParamEntry::CheckIfEntryExists(
  const char *name, IParamVisibleTest &visible
) const
{
  NotClass(GetContext(),name);
  return false;
}

ParamEntryPtr ParamEntry::FindEntryNoInheritance(
  const char *name, IParamVisibleTest &visible
) const
{
  NotClass(GetContext(),name);
  return ParamEntryPtr();
}

ParamEntryPtr ParamEntry::FindEntryPath(
  const char *path, IParamVisibleTest &visible
) const
{
  NotClass(GetContext(),"");
  return ParamEntryPtr();
}

ConstParamEntryPtr ParamEntry::FindParent() const
{
  NotClass(GetContext(),"");
  return ConstParamEntryPtr();
}

ConstParamEntryPtr ParamEntry::FindBase() const
{
  NotClass(GetContext(),"");
  return ConstParamEntryPtr();
}

bool ParamEntry::Request(const char *name, bool includeBases) const
{
  NotClass(GetContext(),name);
  return true;
}


ParamEntryVal ParamEntry::operator >> ( const char *name ) const
{
  NotClass(GetContext(),name);
  return ParamEntryVal(GParamEntryError);
}

float ParamEntry::GetFloatDef(const char* name, float defVal /* = 0.0f */) const
{
	const ParamEntryPtr entry = FindEntry(name,DefaultAccess);

	if (entry && entry->IsFloatValue())
		return entry->operator float();

	return defVal;
}

int ParamEntry::GetIntDef(const char* name, int defVal /* = 0 */) const
{
	const ParamEntryPtr entry = FindEntry(name,DefaultAccess);

	if (entry && entry->IsIntValue())
		return entry->operator int();

	return defVal;
}

bool ParamEntry::GetBoolDef(const char* name, bool defVal /* = false */) const
{
	const ParamEntryPtr entry = FindEntry(name,DefaultAccess);

	if (entry && entry->IsIntValue())
		return entry->operator bool();

	return defVal;
}


/*
void ParamEntry::AddRefLoaded()
{
  Fail("Entry existence cannot be forced");
}

void ParamEntry::ReleaseLoaded()
{
  Fail("Entry existence cannot be forced");
}
*/

ParamEntry *ParamEntry::Clone() const {NotValue(GetContext()); return NULL;}

ParamEntry::operator RStringB() const {NotValue(GetContext());return RStringBEmpty;}
ParamEntry::operator RString() const {NotValue(GetContext());return RStringBEmpty;}
ParamEntry::operator float() const {NotValue(GetContext());return 0;}
ParamEntry::operator int() const {NotValue(GetContext());return 0;}
ParamEntry::operator bool() const {NotValue(GetContext());return false;}
ParamEntry::operator int64() const {NotValue(GetContext());return 0;}
int64 ParamEntry::GetInt64() const {NotValue(GetContext());return 0;}
int ParamEntry::GetInt() const {NotValue(GetContext());return 0;}
RStringB ParamEntry::GetValueRaw() const {NotValue(GetContext());return 0;}
RStringB ParamEntry::GetValue() const {NotValue(GetContext());return 0;}
void ParamEntry::Add( const RStringB &name, const RStringB &val ) {NotClass(GetContext(),name);}
void ParamEntry::Add( const RStringB &name, float val ) {NotClass(GetContext(),name);}
void ParamEntry::Delete(const RStringB &name) {NotClass(GetContext(),name);}
void ParamEntry::Add(const RStringB &name, int64 val) {NotClass(GetContext(),name);} 


/*!
\patch_internal 1.43 Date 1/29/2002 by Jirka
- Fixed: ParamEntry::Add for integer was missing
*/
void ParamEntry::Add( const RStringB &name, int val ) {NotClass(GetContext(),name);}
ParamClassPtr ParamEntry::AddClass( const RStringB &name, bool guaranteedUnique, bool notdefined )
{
  NotClass(GetContext(),name);
  return ParamClassPtr();
} 
ParamEntryPtr ParamEntry::AddArray( const RStringB &name )
{
  NotClass(GetContext(),name);
  return ParamEntryPtr();
}
void ParamEntry::Clear() {NotArray(GetContext());}

void ParamEntry::AddValue(float val){NotArray(GetContext());}
void ParamEntry::AddValue(int val){NotArray(GetContext());}
void ParamEntry::AddValue(int64 val){NotArray(GetContext());}
//void ParamEntry::AddValue(bool val){NotArray(GetContext());}
void ParamEntry::AddValue(const RStringB &val){NotArray(GetContext());}
//void ParamEntry::AddValue(const char *val){NotArray(GetContext());}
IParamArrayValue *ParamEntry::AddArrayValue(){NotArray(GetContext()); return NULL;}

int ParamEntry::GetEntryCount() const {NotClass(GetContext());return 0;}

void ParamEntry::OptimizeFind() const {NotClass(GetContext());}

ParamEntryVal ParamEntry::GetEntry( int i ) const
{
  NotClass(GetContext());
  return ParamEntryVal(GParamEntryError);
}

void ParamEntry::SetValue( const RStringB &val ) {NotValue(GetContext());}
void ParamEntry::SetValue( float val ) {NotValue(GetContext());}
void ParamEntry::SetValue(int val) {NotValue(GetContext());}
void ParamEntry::SetValue( int64 val ) {NotValue(GetContext());}


class ParamRawValue
{
  RStringB _value;
  ParamFile *_file;
  //float _fValue;
  //int _iValue;

public:
  ParamRawValue() {_file = NULL;}
  
  void CopyValue(const ParamRawValue &source) {_value = source._value;}

  SpecValueType GetValueType() {return SVGeneric;}

  void SetValue(const RStringB &value);
  void SetValue(float val);
  void SetValue(int val);
  void SetValue(int64 val);
  void SetFile(ParamFile *file) {_file = file;}

  //! get value, use localization if necessary
  const RStringB GetValue() const;
  //! get value - no localization
  const RStringB GetValueRaw() const;
  float GetFloat() const;
  int GetInt() const;
  int64 GetInt64() const;

  operator RStringB() const {return _value;}
  operator RString() const {return _value;}
  operator float() const {return GetFloat();}
  operator int() const {return GetInt();}
  //operator const char *() const {return GetValue();}

  bool IsTextValue() const {return true;}
  bool IsFloatValue() const {return false;}
  bool IsIntValue() const {return false;}
  bool IsExpression() const {return false;}
  bool IsInt64Value() const {return false;}

  void Save(QOStream &f, int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);
  void CalculateCheckValue(SumCalculator &sum) const;
};

class ParamRawValueFloat
{ // special case - number detected as float
  float _value;
public:
  ParamRawValueFloat() {}

  void CopyValue(const ParamRawValueFloat &source) {_value = source._value;}

  SpecValueType GetValueType() {return SVFloat;}

  void SetValue( const RStringB &value )
  {
    LogF("Float value set as string %s",(const char *)value);
    Fail("Float value set as string");
  }
  void SetValue( float val ){_value=val;}
  void SetValue( int val ){_value=val;}
  void SetValue( int64 val ){_value=val;}
  void SetFile(ParamFile *file) {}

  RStringB GetValue() const;
  RStringB GetValueRaw() const {return GetValue();}

  float GetFloat() const {return _value;}
  int GetInt() const {return toLargeInt(_value);}
  int64 GetInt64() const {return to64bInt(_value);}

  operator RStringB() const {return GetValue();}
  operator RString() const {return GetValue();}
  operator float() const {return GetFloat();}
  operator int() const {return GetInt();}
  //operator const char *() const {return GetValue();}

  bool IsTextValue() const {return false;}
  bool IsFloatValue() const {return true;}
  bool IsIntValue() const {return false;}
  bool IsInt64Value() const {return false;}
  bool IsExpression() const {return false;}

  void Save(QOStream &f, int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);

  void CalculateCheckValue(SumCalculator &sum) const;
};


class ParamRawValueInt64
{ // special case - number detected as int
  int64 _value;
public:
  ParamRawValueInt64() {}

  void CopyValue(const ParamRawValueInt64 &source) {_value = source._value;}

  SpecValueType GetValueType() {return SVInt64;}

  void SetValue( const RStringB &value )
  {
    RptF("Int64 value set as string %s",(const char *)value);
    Fail("Int64 set as string");
  }
  void SetValue( float val ){_value=to64bInt(val);}
  void SetValue( int val ){_value=val;}
  void SetValue( int64 val ){_value=val;}
  void SetFile(ParamFile *file) {}

  RStringB GetValue() const;
  RStringB GetValueRaw() const {return GetValue();}
  float GetFloat() const {return _value;}
  int GetInt() const {return _value;}
  int64 GetInt64() const {return _value;}

  operator RStringB() const {return GetValue();}
  operator float() const {return GetFloat();}
  operator int() const {return GetInt();}
  operator int64() const {return GetInt64();}
  //operator const char *() const {return GetValue();}

  bool IsTextValue() const {return false;}
  bool IsFloatValue() const {return false;}
  bool IsIntValue() const {return false;}
  bool IsInt64Value() const {return true;}
  bool IsExpression() const {return false;}

  void Save(QOStream &f, int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);

  void CalculateCheckValue(SumCalculator &sum) const;
};



class ParamRawValueInt
{ // special case - number detected as int
  int _value;
public:
  ParamRawValueInt() {}

  void CopyValue(const ParamRawValueInt &source) {_value = source._value;}

  SpecValueType GetValueType() {return SVInt;}

  void SetValue( const RStringB &value )
  {
    RptF("Int value set as string %s",(const char *)value);
    Fail("Int set as string");
  }
  void SetValue( float val ){_value=toLargeInt(val);}
  void SetValue( int val ){_value=val;}
  void SetValue( int64 val ){_value=val;}
  void SetFile(ParamFile *file) {}

  RStringB GetValue() const;
  RStringB GetValueRaw() const {return GetValue();}
  float GetFloat() const {return _value;}
  int GetInt() const {return _value;}
  int64 GetInt64() const {return _value;}

  operator RStringB() const {return GetValue();}
  operator RString() const {return GetValue();}
  operator float() const {return GetFloat();}
  operator int() const {return GetInt();}
  //operator const char *() const {return GetValue();}

  bool IsTextValue() const {return false;}
  bool IsFloatValue() const {return false;}
  bool IsIntValue() const {return true;}
  bool IsInt64Value() const {return false;}
  bool IsExpression() const {return false;}

  void Save(QOStream &f, int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);

  void CalculateCheckValue(SumCalculator &sum) const;
};

class ParamRawValueExpression
{
  RStringB _value;
  ParamFile *_file;
  //float _fValue;
  //int _iValue;

public:
  ParamRawValueExpression() {_file = NULL;}

  void CopyValue(const ParamRawValueExpression &source) {_value = source._value;}

  SpecValueType GetValueType() {return SVExpression;}

  void SetValue(const RStringB &value);
  void SetValue(float val);
  void SetValue(int val);
  void SetValue(int64 val);
  void SetFile(ParamFile *file) {_file = file;}

  //! get value, use localization if necessary
  const RStringB GetValue() const;
  //! get value - no localization
  const RStringB GetValueRaw() const;
  float GetFloat() const;
  int GetInt() const;
  int64 GetInt64() const;

  operator RStringB() const {return _value;}
  operator RString() const {return _value;}
  operator float() const {return GetFloat();}
  operator int() const {return GetInt();}
  //operator const char *() const {return GetValue();}

  bool IsTextValue() const {return true;}
  bool IsFloatValue() const {return false;}
  bool IsIntValue() const {return false;}
  bool IsExpression() const {return true;}
  bool IsInt64Value() const {return false;}

  void Save(QOStream &f, int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);
  void CalculateCheckValue(SumCalculator &sum) const;
};

void ParamEntry::SetValue( int index, const RStringB &string )
{
  ErrorMessage(EMError,"SetValue: '%s' not an array",(const char *)GetContext());
}

void ParamEntry::SetValue( int index, float val )
{
  ErrorMessage(EMError,"SetValue: '%s' not an array",(const char *)GetContext());
}

void ParamEntry::SetValue( int index, int val )
{
  ErrorMessage(EMError,"SetValue: '%s' not an array",(const char *)GetContext());
}

int ParamEntry::GetSize() const 
{
  ErrorMessage(EMError,"Size: '%s' not an array",(const char *)GetContext());
  return 0;
}

void ParamEntry::DeleteValue(int index)
{
  ErrorMessage(EMError,"DeleteValue: '%s' not an array",(const char *)GetContext());
}

#include <Es/Memory/normalNew.hpp>
//! class placeholder - see ParamClass
/*!
This placeholder is used when class is unloaded but its parent is loaded.
Placeholder contains only the name - its size is therefore 8 B 
*/

class ParamClassPlaceholder: public ParamEntry
{
  typedef ParamEntry base;
  
  friend class ParamClass;  
  friend class ParamClassOwnerFilename;
  
  //! id identifying source (including owner) and relative path within it
  Ref<IParamClassSource> _source;
  //! ofset where given class should be loaded from in case of unloading
  int _sourceOffset; // source offset
  int _sourceSize; // source size - including class children

  explicit ParamClassPlaceholder(const RStringB &name);
  explicit ParamClassPlaceholder(const ParamClass &src);
  
  public:

  const ParamClassPlaceholder *GetPlaceholderInterface() const {return this;}
  ParamClassPlaceholder *GetPlaceholderInterface() {return this;}
  
  bool IsClass() const {return true;}
  ParamClass *GetClassInterface()
  {
    LogF("Getting class from %s",(const char *)GetName());
    Fail("Cannot get interface of unloaded class");
    return NULL;
  }

  size_t GetMemorySize() const {return sizeof(*this);}
  
  virtual void SerializeBin(SerializeBinStream &f)
  {
    Fail("Not implemented");
  }
  virtual void SetFile(ParamFile *file)
  {
    //Fail("Not implemented");
  }
  virtual void CalculateCheckValue(SumCalculator &sum) const
  {
    Fail("Not implemented");
  }
  
  int GetEntryCount() const
  {
    Fail("Not implemented");
    return base::GetEntryCount();
  }

  void SetSource(IParamClassSource *source, bool subentries)
  {
    _source=source;
  }

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(ParamClassPlaceholder)

ParamClassPlaceholder::ParamClassPlaceholder(const RStringB &name)
:ParamEntry(name)
{
}

ParamClassPlaceholder::ParamClassPlaceholder(const ParamClass &src)
:ParamEntry(src.GetName()),
_source(src._source),_sourceOffset(src._sourceOffset),_sourceSize(src._sourceSize)
{
}

DEFINE_FAST_ALLOCATOR(ParamClass)

ParamClass::ParamClass()
:ParamEntry(NULL)
{
  _access = PADefault;
  _loadCount = 0;
  _sourceOffset = -1;
  _sourceSize = 0;
#if _VBS3 // saveConfig can save base class
  _noParenthesisIfEmpty = false;
#endif
}
ParamClass::ParamClass( const RStringB &name)
:ParamEntry(name)
{
  _access = PADefault;
  _loadCount = 0;
  _sourceOffset = -1;
  _sourceSize = 0;
#if _VBS3 // saveConfig can save base class
  _noParenthesisIfEmpty = false;
#endif
}

/*!
Make sure all references to other classes are released.
Until this is done, the classes cannot be destroyed

*/
void ParamClass::UnlinkClasses()
{
  for (int i=0; i<_entries.Size(); i++)
  {
    ParamEntry *ee = _entries[i];
    if (ee->GetPlaceholderInterface()) continue;
    ParamClass *e = _entries[i]->GetClassInterface();
    if (!e) continue;
    e->UnlinkClasses();
  }
  _base.Free();
  _parent.Free();
}

bool ParamClass::CheckIntegrity() const
{
  /*
  const ParamClass *topMost = GetRoot();
  int loadCountExpected = topMost->CountLinksToClass(this);
  if (loadCountExpected!=_loadCount)
  {
    LogF
    (
      "%s: Load count not correct, %d!=%d",
      (const char *)GetContext(),
      loadCountExpected,_loadCount
    );
    return false;
  }
  for (int i=0; i<_entries.Size(); i++)
  {
    ParamClass *e = _entries[i]->GetClassInterface();
    if (!e) continue;
    bool ok = e->CheckIntegrity();
    if (!ok)
    {
      LogF("Class %s not correct",(const char *)GetContext(e->GetName()));
      return false;
    }
  }
  */
  return true;
}

int ParamClass::CountLinksToClass(const ParamClass &cls) const
{
  int count = 0;
  if (&cls==_parent) count++;
  if (&cls==_base) count++;
  for (int i=0; i<_entries.Size(); i++)
  {
    ParamClass *e = _entries[i]->GetClassInterface();
    if (!e) continue;
    count += e->CountLinksToClass(cls);
  }
  return count;
}

ParamClass::~ParamClass()
{
  // all our children need to unlink their bases and parents first
  // classes should be unlinked in the whole paramfile
  //UnlinkClasses();
  // make sure class is not locked and can be destructed
  // if any loads are still enforced, it can mean some links into the class still exist
  // this can be OK when unloading only, but it is bad when really destroying
  #if _ENABLE_REPORT
    if (_loadCount!=0)
    {
      LogF("Class %s destroyed with lock count %d",(const char *)GetName(),_loadCount);
      Fail("Class destroyed, but still locked");
    }
  #endif
  // class need to be removed from the cache
  RemoveFromTheCache();
  
  DoAssert(!IsInList());
}

class ParamClassDecl: public ParamClass
{
  typedef ParamClass base;
  
  public:
  ParamClassDecl( const RStringB &name )
  :base(name)
  {}
  
  virtual bool IsDefined() const {return false;}
  virtual void Save(QOStream &f, int indent, const char *eol) const;
  
  //@{ make access functions return error
  virtual ParamEntryVal operator >> ( const char *name ) const
  {
    NotDefined(GetContext(),name);
    return ParamEntryVal(GParamEntryError);
  }

  virtual int GetEntryCount() const {NotDefined(GetContext());return 0;}
  virtual ParamEntryVal GetEntry( int i ) const
  {
    NotDefined(GetContext());
    return ParamEntryVal(GParamEntryError);
  }
  //@}
  
};

class ParamClassDelete: public ParamClass
{
  typedef ParamClass base;

public:
  ParamClassDelete(const RStringB &name)
  : base(name) {}

  virtual bool IsDefined() const {return false;}
  virtual bool IsDelete() const {return true;}
  virtual void Save(QOStream &f, int indent, const char *eol) const;

  //@{ make access functions return error
  virtual ParamEntryVal operator >> ( const char *name ) const
  {
    NotDefined(GetContext(),name);
    return ParamEntryVal(GParamEntryError);
  }

  virtual int GetEntryCount() const {NotDefined(GetContext());return 0;}
  virtual ParamEntryVal GetEntry( int i ) const
  {
    NotDefined(GetContext());
    return ParamEntryVal(GParamEntryError);
  }
  //@}
};



/**
This function does real unloading/freeing the memory.
It is used when item is evicted from the cache.
*/
void ParamClass::ReleaseAll()
{
  // unload all subclasses that can be unloaded
  int countUnloaded = 0;
  for(;;)
  {
    bool somethingUnloaded = false;
    // unload all unloadable entries
    int notUnloaded = 0;
    for (int i=0; i<_entries.Size(); i++)
    {
      ParamEntry *entry = _entries[i];
      if (!entry->IsClass()) continue;
      // if if is already unloaded, there is no need to unload it again
      if (entry->GetPlaceholderInterface()) continue;
      ParamClass *cls = entry->GetClassInterface();
      // only class can be unloaded
      if (!cls) continue;
      // only some classes can be unloaded
      if (!cls->NeedsRestoring()) continue;
      // remove all links from inside the class
      int internalLinks = cls->CountLinksToClass(*cls);
      if (internalLinks!=cls->_loadCount)
      {
        Log
        (
          "Cannot unload %s, links %d %d",
          (const char *)cls->GetContext(),internalLinks,cls->_loadCount
        );
        notUnloaded++;
      }
      else
      {
        // check if there are some external links
        // if the class is still locked, it cannot be unloaded (some link to it is locked)
        somethingUnloaded = true;
        countUnloaded++;
        UnloadEntry(i);
      }
    }
    // if there is nothing more to unload or no unload was done, quit
    if (notUnloaded==0)
    {
      #if DIAG_UNLOAD>=200
      LogF("%s: %d subentries unloaded",(const char *)GetContext(),countUnloaded);
      #endif
      break;
    }
    if (!somethingUnloaded) break;
  }
}

void ParamClass::RefreshInTheCache()
{
  if (IsInList())
  {
    // remove from the cache and insert it back again
    ParamFile *file = _source->GetFile();
    if (!file)
    {
      Fail("No file - no cache");
      return;
    }
    file->CacheRefresh(this);
  }
}

void ParamClass::RemoveFromTheCache()
{
  if (IsInList())
  {
    // we need to remove this from unloadable list
    ParamFile *file = _source->GetFile();
    if (!file)
    {
      Fail("We need a file for unloading");
      return;
    }
    file->CacheRemove(this);
  }
}

/**
Assumes the item is already loaded, removes it from the cache.
*/

void ParamClass::DoLoad()
{
  RemoveFromTheCache();
}

/**
Does not really unload, only moves to the cache.
*/

void ParamClass::DoUnload()
{
  Assert(_loadCount==0);
  // we can add this class into the "unloadable" list
  // caution: _parent is already unlinked
  if (NeedsRestoring())
  {
    ParamFile *file = _source->GetFile();
    if (!file)
    {
      Fail("We need a file for unloading");
      return;
    }
    file->CacheInsert(this);
  }
}

//! information that is used across various serialization functions
struct ParamFileContext
{
  //! string pool
  FindArray<RStringB> _strings;
  //! load format version - used to maintain backward compatibility
  int _version; 
  //! source used for streaming (on demand loading)
  Ref<IParamClassSource> _source;
  
  //! when set, all subclasses are loaded as well
  bool _fullLoad;
  
  //! offset where we should seek when done with loading
  /*!
  this is used to skip string table.
  This member is valid only with config format 7 and newer
  */
  int _endOfParamfile;

  //! transfer name
  void TransferString(SerializeBinStream &f, RStringB &string);
  //! transfer integer value
  void TransferInt(SerializeBinStream &f, int &a);
  //! transfer encoded integer value
  void TransferIndex(SerializeBinStream &f, int &a, int verEncode=3);
  //! transfer string table definition
  void TransferStringTable(SerializeBinStream &f);

  //! load/save format header from the file
  virtual bool TransferFormat(SerializeBinStream &f);
  
  //! load version and other file-level info from the file
  bool LoadHeader(SerializeBinStream &f);
};

const int ActualParamFileVersion = 8;


bool ParamFileContext::TransferFormat(SerializeBinStream &f)
{
#ifdef _WIN32
  if (!f.Version('Par\0'))
#else
  if (!f.Version(StrToInt("\0raP")))
#endif
  {
    f.SetError(f.EBadFileType);
    return false;
  }
  return true;
}

bool ParamFileContext::LoadHeader(SerializeBinStream &f)
{
  Assert(f.IsLoading());
  // supported version:
  // 2..4 - OFP Retail
  // 5..6 - OFP Xbox, config streaming experiments
  // 7,8  - OFP Xbox - streaming working
  _version = f.LoadInt();
  if (_version==0)
  {
    _version = f.LoadInt();
  }
  // some version are not supported
  if (
    _version<2 || _version>ActualParamFileVersion ||
    _version>=5 && _version<=6
  )
  {
    WarningMessage(
      "Bad version %d in ParamFile %s",
      _version,
      _source ? (const char *)_source->GetOwnerDebugName() : "No source"
    );
    f.SetError(f.EBadVersion);
    return false;
  }
  #if _ENABLE_REPORT
  if (_version<ActualParamFileVersion)
  {
    LogF(
      "Config %s uses old binary format %d - rebinarize",
      _source ? (const char *)_source->GetOwnerDebugName() : "No source",
      _version
    );
  }
  #endif
  if (_version>=6)
  {
    if (_version<8)
    {
      int stringtableStart = f.LoadInt();
      _endOfParamfile = f.LoadInt();
      // we need to pass stringtableEnd to the caller
      int curPos = f.TellG();
      f.SeekG(stringtableStart);
      TransferStringTable(f);
      f.SeekG(curPos);
      
    }
    else
    {
      _endOfParamfile = f.LoadInt();
    }
  }
  else
  {
    _endOfParamfile = -1;
  }
  return true;
}


ParamClassOwner::ParamClassOwner(RString owner)
{
  owner.Lower();
  _owner = owner;
}

void ParamClassOwner::SetOwnerName(RString owner)
{
  owner.Lower();
  _owner = owner;
}

RStringB ParamClassOwner::GetOwnerName() const
{
  return _owner;
}

RString ParamClassOwner::GetOwnerDebugName() const
{
  return GetOwnerName()+RString(":")+GetDebugSuffix();
}

ParamClassOwnerFilename::ParamClassOwnerFilename
(
  RString owner, ParamFile *file, RStringB filename
)
:base(owner),_file(file),_name(filename)
{
  _in.AutoOpen(filename);
  // context needs to be set later
  _fullLoad = true;
  _version = -1;
  _endOfParamfile = 0;
}

RString ParamClassOwnerFilename::GetDebugSuffix() const
{
  return _name;
}

bool ParamClassOwnerFilename::CanUnload() const
{
  return true;
}

#if 1
/// context to help accessing streams
class LoadQIStream
{
  QIFStreamB _in;
  
  public:
  explicit LoadQIStream(QIFStreamB &in):_in(in)
  {
    DoAssert(_in.IsFlushed());
    _in.seekg(0);
  }
  ~LoadQIStream()
  {
  }
  QIStream *operator ->() {return &_in;}
  operator QIStream *() {return &_in;}
};
#else
/// context to help accessing streams
class LoadQIStream
{
  QIFStreamB _in;
  
  public:
  explicit LoadQIStream(ParamClassOwnerFilename &in)
  {
    _in.AutoOpen(in.GetFilename());
  }
  ~LoadQIStream()
  {
    //_in.InvalidateBuffer();
  }
  QIStream *operator ->() {return &_in;}
  operator QIStream *() {return &_in;}
};
#endif

bool ParamClassOwnerFilename::Request(int sourceOffset, int sourceSize)
{
  // older configs do not support preloading
  if (sourceSize<=0) return true;
  LoadQIStream in(_in);
  return in->PreRead(FileRequestPriority(100),sourceOffset,sourceSize);
}

void ParamClassOwnerFilename::StoreContext(const ParamFileContext &ctx)
{
  _fullLoad = ctx._fullLoad;
  _version = ctx._version;
  _endOfParamfile = ctx._endOfParamfile;
}

bool ParamClassOwnerFilename::ExtractContext(ParamFileContext &ctx)
{
  Assert(_version>=0);
  if (_version>=8)
  {
    ctx._source = this;
    ctx._fullLoad = _fullLoad;
    ctx._version = _version;
    ctx._endOfParamfile = _endOfParamfile;
    return true;
  }
  return false;
}



void ParamClassOwnerFilename::Load(ParamClass &cls, const char *relPath)
{
  // file can be binary or text
  // load whole file
  // load class from given source and offset
  LoadQIStream in(_in);
  
  SerializeBinStream f(in);
  
  // there was a string index in the context
  // it is no longer used, as it was not compatible with streaming
  ParamFileContext context;
  if (!ExtractContext(context))
  {
    // old & slow streaming - load the context every time we load any class
    if (!context.TransferFormat(f))
    {
      ErrorMessage("Bad format while loading %s",(const char *)cls.GetContext());
      return;
    }
    if (!context.LoadHeader(f))
    {
      ErrorMessage("Error while loading %s",(const char *)cls.GetContext());
      return;
    }
  }
  
  f.SetContext(&context);

  if (context._version<5)
  {
    LogF("Streaming with old version %d may be buggy",context._version);
  }
  // start reading directly from the offset
  DoAssert(cls._sourceOffset>=0);
  f.SeekG(cls._sourceOffset);
  cls.SerializeBin(f);
  
  const ParamClass *root = cls.GetRoot();
  if (root)
  {
    ParamFile *file = unconst_cast(root)->GetFileInterface();
    Assert(file);
    cls.SetFile((file));
  }

  Assert(f.GetError()==SerializeBinStream::EOK);
}

bool ParamClassOwnerFilename::RequestBaseName(RStringB &baseName, const ParamClassPlaceholder &cls)
{
  // file can be binary or text
  // load whole file
  // load class from given source and offset
  LoadQIStream in(_in);
  
  SerializeBinStream f(in);
  
  // what about string index? It has to reside somewhere
  // currently we load the context everytime we load any class
  ParamFileContext context;
  if (!ExtractContext(context))
  {
    // old & slow streaming
    if (!context.TransferFormat(f))
    {
      ErrorMessage("Bad format while loading %s",(const char *)cls.GetContext());
      return true;
    }
    if (!context.LoadHeader(f))
    {
      ErrorMessage("Error while loading %s",(const char *)cls.GetContext());
      return true;
    }
  }
  
  f.SetContext(&context);

  if (context._version<8)
  {
    return true;
  }
  // start reading directly from the offset
  DoAssert(cls._sourceOffset>=0);
  f.SeekG(cls._sourceOffset);
  return ParamClass::RequestBaseName(f,baseName);
}

ParamClassOwnerNoUnload::ParamClassOwnerNoUnload(RString owner, RString filename)
:
#if _DEBUG
  base(owner),_debugSuffix(filename)
#else
  base(owner)
#endif
{
}

RString ParamClassOwnerNoUnload::GetDebugSuffix() const
{
  #if _DEBUG
  return RString("(") + _debugSuffix + RString(" - no unload)");
  #else
  return RString("(no unload)");
  #endif
}

bool ParamClassOwnerNoUnload::RequestBaseName(RStringB &baseName, const ParamClassPlaceholder &cls)
{
  Fail("Not implemented");
  return true;
}

void ParamClassOwnerNoUnload::Load(ParamClass &cls, const char *relPath)
{
  Fail("Not implemented");
}

ParamClassOwnerIndirectNoUnload::ParamClassOwnerIndirectNoUnload(const IParamClassSource *owner)
:_indirect(owner)
{
}

RString ParamClassOwnerIndirectNoUnload::GetDebugSuffix() const
{
  return RString("(") + _indirect->GetDebugSuffix() + RString(" - no unload)");
}

bool ParamClassOwnerIndirectNoUnload::RequestBaseName(RStringB &baseName, const ParamClassPlaceholder &cls)
{
  Fail("Not implemented");
  return true;
}

void ParamClassOwnerIndirectNoUnload::Load(ParamClass &cls, const char *relPath)
{
  Fail("Not implemented");
}

RStringB ParamClassOwnerIndirectNoUnload::GetOwnerName() const
{
  return _indirect->GetOwnerName();
}
RString ParamClassOwnerIndirectNoUnload::GetOwnerDebugName() const
{
  return GetOwnerName()+RString(":")+GetDebugSuffix();
}

void ParamClassOwnerIndirectNoUnload::SetOwnerName(RString owner)
{ 
  Fail("SetOwnerName should never be used on indirect classes");
}

ParamClassOwnerEntry::ParamClassOwnerEntry(RString owner, ParamEntry *baseEntry)
:base(owner),_baseEntry(baseEntry)
{
}

RString ParamClassOwnerEntry::GetDebugSuffix() const
{
  return RString("entry-")+_baseEntry->GetContext();
}

bool ParamClassOwnerEntry::CanUnload() const
{
  return false;
}

void ParamClassOwnerEntry::Load(ParamClass &cls, const char *relPath)
{
  Fail("Not implemented");
}

#include <Es/Memory/normalNew.hpp>


template <class ParamRawValueSpec>
class ParamValueSpec: public ParamEntry,public ParamRawValueSpec
{
  public:
  ParamValueSpec();
  ParamValueSpec(const RStringB &name);

  virtual ParamEntry *Clone() const
  {
    ParamValueSpec<ParamRawValueSpec> *clone = new ParamValueSpec<ParamRawValueSpec>(_name);
    clone->CopyValue(*this);
    return clone;
  }

  RStringB GetValue() const {return ParamRawValueSpec::GetValue();}
  RStringB GetValueRaw() const {return ParamRawValueSpec::GetValueRaw();}
  float GetFloat() const {return ParamRawValueSpec::GetFloat();}
  int GetInt() const {return ParamRawValueSpec::GetInt();}
  int64 GetInt64() const {return ParamRawValueSpec::GetInt64();}

  operator RStringB() const {return ParamRawValueSpec::GetValue();}
  operator RString() const {return ParamRawValueSpec::GetValue();}
  operator float() const {return ParamRawValueSpec::GetFloat();}
  operator int() const{return ParamRawValueSpec::GetInt();}
  operator bool() const{return ParamRawValueSpec::GetInt()!=0;}
  operator int64() const{return ParamRawValueSpec::GetInt64();}

  void SetValue(const RStringB &val){ParamRawValueSpec::SetValue(val);}
  void SetValue(float val){ParamRawValueSpec::SetValue(val);}
  void SetValue(int val){ParamRawValueSpec::SetValue(val);}
  void SetValue(int64 val){ParamRawValueSpec::SetValue(val);}
  void SetFile(ParamFile *file) {ParamRawValueSpec::SetFile(file);}

  bool IsTextValue() const {return ParamRawValueSpec::IsTextValue();}
  bool IsFloatValue() const {return ParamRawValueSpec::IsFloatValue();}
  bool IsIntValue() const {return ParamRawValueSpec::IsIntValue();}
  bool IsInt64Value() const {return ParamRawValueSpec::IsInt64Value();}
  bool IsExpression() const {return ParamRawValueSpec::IsExpression();}
  bool IsArrayValue() const {return false;}

  size_t GetMemorySize() const {return sizeof(*this);}
  void Save( QOStream &f, int indent, const char *eol ) const;

  virtual void SerializeBin(SerializeBinStream &f);
  virtual void CalculateCheckValue(SumCalculator &sum) const;

  USE_FAST_ALLOCATOR
};

template <class ParamRawValueSpec>
class ParamArrayValueSpec: public IParamArrayValue,public ParamRawValueSpec
{
  public:
  ParamArrayValueSpec(const RStringB &val){ParamRawValueSpec::SetValue(val);}
  ParamArrayValueSpec(float val){ParamRawValueSpec::SetValue(val);}
  ParamArrayValueSpec(int val){ParamRawValueSpec::SetValue(val);}
  ParamArrayValueSpec(int64 val){ParamRawValueSpec::SetValue(val);}

  RStringB GetValue() const {return ParamRawValueSpec::GetValue();}
  RStringB GetValueRaw() const {return ParamRawValueSpec::GetValueRaw();}
  int GetInt() const {return ParamRawValueSpec::GetInt();}
  float GetFloat() const {return ParamRawValueSpec::GetFloat();}
  int64 GetInt64() const {return ParamRawValueSpec::GetInt64();}

  void SetValue(const RStringB &val){ParamRawValueSpec::SetValue(val);}
  void SetValue(float val){ParamRawValueSpec::SetValue(val);}
  void SetValue(int val){ParamRawValueSpec::SetValue(val);}
  void SetValue(int64 val){ParamRawValueSpec::SetValue(val);}
  void SetFile(ParamFile *file) {ParamRawValueSpec::SetFile(file);}

  bool IsTextValue() const {return ParamRawValueSpec::IsTextValue();}
  bool IsFloatValue() const {return ParamRawValueSpec::IsFloatValue();}
  bool IsIntValue() const {return ParamRawValueSpec::IsIntValue();}
  bool IsInt64Value() const {return ParamRawValueSpec::IsInt64Value();}
  bool IsExpression() const {return ParamRawValueSpec::IsExpression();}
  bool IsArrayValue() const {return false;}

  size_t GetMemorySize() const {return sizeof(*this);}
  
  void Save(QOStream &f, int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);

  // may be array of values
  const IParamArrayValue *GetItem(int i) const {return NULL;}
  int GetItemCount() const
  {
    ErrorMessage(EMError,"Value not an array.");
    return 0;
  }
/*
  PackedColor GetPackedColor() const {return PackedBlack;}
  SoundPars GetSoundPars() const {return SoundPars();}
*/

  void AddValue(float val) {ErrorMessage(EMError,"Value not an array.");}
  void AddValue(int val) {ErrorMessage(EMError,"Value not an array.");}
  void AddValue(int64 val) {ErrorMessage(EMError,"Value not an array.");}
  void AddValue(const RStringB &val) {ErrorMessage(EMError,"Value not an array.");}
  IParamArrayValue *AddArrayValue() {ErrorMessage(EMError,"Value not an array."); return NULL;}

  virtual void CalculateCheckValue(SumCalculator &sum) const
  {
    ParamRawValueSpec::CalculateCheckValue(sum);
  }
  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

typedef ParamValueSpec<ParamRawValue> ParamValue;
typedef ParamValueSpec<ParamRawValueFloat> ParamValueFloat;
typedef ParamValueSpec<ParamRawValueInt> ParamValueInt;
typedef ParamValueSpec<ParamRawValueInt64> ParamValueInt64;
typedef ParamValueSpec<ParamRawValueExpression> ParamValueExpression;

typedef ParamArrayValueSpec<ParamRawValue> ParamArrayValue;
typedef ParamArrayValueSpec<ParamRawValueFloat> ParamArrayValueFloat;
typedef ParamArrayValueSpec<ParamRawValueInt> ParamArrayValueInt;
typedef ParamArrayValueSpec<ParamRawValueInt64> ParamArrayValueInt64;
typedef ParamArrayValueSpec<ParamRawValueExpression> ParamArrayValueExpression;

DEFINE_FAST_ALLOCATOR(ParamValue)
DEFINE_FAST_ALLOCATOR(ParamValueFloat)
DEFINE_FAST_ALLOCATOR(ParamValueInt)
DEFINE_FAST_ALLOCATOR(ParamValueInt64)
DEFINE_FAST_ALLOCATOR(ParamValueExpression)

DEFINE_FAST_ALLOCATOR(ParamArrayValue)
DEFINE_FAST_ALLOCATOR(ParamArrayValueFloat)
DEFINE_FAST_ALLOCATOR(ParamArrayValueInt)
DEFINE_FAST_ALLOCATOR(ParamArrayValueInt64)
DEFINE_FAST_ALLOCATOR(ParamArrayValueExpression)

static IParamArrayValue *CreateParamArrayValue(RStringB val)
{
  return new ParamArrayValue(val);
}
static IParamArrayValue *CreateParamArrayValue(float val)
{
  return new ParamArrayValueFloat(val);
}
static IParamArrayValue *CreateParamArrayValue(int val)
{
  return new ParamArrayValueInt(val);
}

static ParamEntry *CreateParamValue(SerializeBinStream &f)
{
  // load type and create value
  Assert( f.IsLoading() );
  char type;
  f.Transfer(type);
  switch (type)
  {
    case SVExpression:
      return new ParamValueExpression();
    case SVGeneric:
      return new ParamValue();
    case SVFloat:
      return new ParamValueFloat();
    case SVInt:
      return new ParamValueInt();
    case SVInt64:
      return new ParamValueInt64();
    default:
      ErrF("Unknown value type %d",type);
      return new ParamValue();
  }

}

template <class ParamRawValueSpec>
ParamValueSpec<ParamRawValueSpec>::ParamValueSpec()
:ParamEntry(NULL)
{
}

template <class ParamRawValueSpec>
ParamValueSpec<ParamRawValueSpec>::ParamValueSpec(const RStringB &name)
:ParamEntry(name)
{
}

template <class ParamRawValueSpec>
void ParamArrayValueSpec<ParamRawValueSpec>::Save(QOStream &f, int indent, const char *eol) const
{
  // check type
  ParamRawValueSpec::Save(f,indent,eol);
}

template <class ParamRawValueSpec>
void ParamArrayValueSpec<ParamRawValueSpec>::SerializeBin(SerializeBinStream &f)
{
  if (f.IsSaving())
  {
    char type = ParamRawValueSpec::GetValueType();
    f.Transfer(type);
  }
  ParamRawValueSpec::SerializeBin(f);
}



// scan some special value types

/*!
\patch_internal 2.01 Date 1/13/2003 by Jirka
- Fixed: Bug in parsing of expressions in confi files (f.e. "0x01 + 2" was parsed like "0x01" only)
*/

static int ScanHex(const char *val, bool &ok)
{
  ok = false;
  if (!strnicmp(val, "0x", 2))
  {
    char c;
    const char *ptr = (const char *)val + 2;
    ok = isxdigit(*ptr) != 0;
    if (!ok) return 0;
    int iValue = 0;
    while (c=*(ptr++), isxdigit(c))
    {
      iValue *= 16;
      if (isdigit(c))   // 0..9
        iValue += c - '0';
      else if (c<='F')  // A..F
        iValue += 10 + c - 'A';
      else              // a..f
        iValue += 10 + c - 'a';
    }
    ok = (c==0);
    return iValue;
  }
  else
  {
    return 0;
  }
}

static float ScanDb(const char *ptr, bool &ok)
{
  ok=false;
  if (ptr[0]!='d' || ptr[1]!='b') return 0;
  ok=true;
  char *end;
  float db = strtod(ptr+2,&end);
  if (*end!=0)
  {
    LogF("invalid db value %s",ptr);
  }
  return pow(10.0f,db*(1.0f/20));
}

static float ScanFloatPlain(const char *ptr, bool &ok)
{
  char *end;
  float db = strtod(ptr,&end);
  ok = (*end==0);
  return db;
}

static int ScanIntPlain(const char *ptr, bool &ok)
{
  errno=0;
  char *end;
  long db = strtol(ptr,&end,10);
  ok = (*end==0);
  if (errno==ERANGE)
    ok = false;
  return (int)db;
}

static int64 ScanInt64Plain(const char *ptr, bool &ok)
{
  char *end;
  int64 db = _strtoi64(ptr,&end,10);
  ok = (*end==0);
  return db;
}

static int ScanInt(const char *ptr, bool &ok)
{
  ok = false;
  if (!*ptr) return 0;
  int val = ScanIntPlain(ptr,ok);
  if (ok) return val;
  val = ScanHex(ptr,ok);
  if (ok) return val;
  return 0;
}

static int64 ScanInt64(const char *ptr, bool &ok)
{
  ok = false;
  if (!*ptr) return 0;
  int64 val = ScanInt64Plain(ptr,ok);
  if (ok) return val;
  val = ScanHex(ptr,ok);
  if (ok) return val;
  return 0;
}

static float ScanFloat(const char *ptr, bool &ok)
{
  ok = false;
  if (!*ptr) return 0;
  float val = ScanFloatPlain(ptr,ok);
  if (ok) return val;
  val = ScanDb(ptr,ok);
  if (ok) return val;
  return 0;
}

const RStringB ParamRawValue::GetValue() const
{
  const char *val = _value;
  if (strncmp(val, "$STR", 4) == 0) return _file->LocalizeString(val + 1);
  else return _value;
}

const RStringB ParamRawValue::GetValueRaw() const
{
  return _value;
}

void ParamRawValue::CalculateCheckValue(SumCalculator &sum) const
{
  Assert(_file);
  sum.Add(_value,_value.GetLength());
#if LOG_CHECKSUM >= 2
  LogF("  add CRC of '%s', result %x", cc_cast(_value), sum.GetResult());
#endif
}


float ParamRawValue::GetFloat() const
{
  bool ok;
  // check for simple cases
  float valF = ScanFloat(_value,ok);
  if (ok) return valF;
  int valI = ScanInt(_value,ok);
  if (ok) return valI;
  // if there is no file, we cannot evaluate expressions
  if (!_file)
  {
    RptF("Cannot evaluate '%s' - no file",(const char *)_value);
    return 0.0f;
  }
  return _file->EvaluateFloat(_value);
}
int ParamRawValue::GetInt() const
{
  bool ok;

  // check for simple cases
  int valI = ScanInt(_value,ok);
  if (ok) return valI;
  float valF = ScanFloat(_value,ok);
  // if there is no file, we cannot evaluate expressions
  if (ok)
  {
    LogF("Warning: rounding float value %g",valF);
    return toLargeInt(valF);
  }
  if (!_file)
  {
    RptF("Cannot evaluate '%s' - no file",(const char *)_value);
    return 0;
  }
  return toLargeInt(_file->EvaluateFloat(_value));
}

int64 ParamRawValue::GetInt64() const
{
  bool ok;

  // check for simple cases
  int64 valI64 = ScanInt64(_value,ok);
  if (ok) return valI64;

  int valI = ScanInt(_value,ok);
  if (ok) return valI;
  float valF = ScanFloat(_value,ok);
  // if there is no file, we cannot evaluate expressions
  if (ok)
  {
    LogF("Warning: rounding float value %g",valF);
    return toLargeInt(valF);
  }
  if (!_file)
  {
    RptF("Cannot evaluate '%s' - no file",(const char *)_value);
    return 0;
  }
  return toLargeInt(_file->EvaluateFloat(_value));
}

void ParamRawValue::SetValue( const RStringB &value )
{
  _value=value;
}

void ParamRawValue::SetValue( float val )
{
  BString<256> buf;
  sprintf(buf,"%f",val);
  _value=buf.cstr();

  #if _ENABLE_REPORT
  // check if ok
  char *end;
  float dummy = strtod(buf, &end);
  (void)dummy;
  if (*end != 0)
  {
    ErrF("Setting invalid value %s", (const char *)buf);
    _value = "0";
  }
  #endif
}

void ParamRawValue::SetValue(int val)
{
  BString<256> buf;
  sprintf(buf,"%d",val);
  _value=buf.cstr();
}

void ParamRawValue::SetValue(int64 val)
{
  BString<256> buf;
  sprintf(buf,"%I64d",val);
  _value=buf.cstr();
}

//////////////////////////////////////////////////////////////////////////

const RStringB ParamRawValueExpression::GetValue() const
{
  int handle = _file->BeginContext();
  RStringB value = _file->EvaluateStringInternal(_value);
  _file->EndContext(handle);
  return value;
}

const RStringB ParamRawValueExpression::GetValueRaw() const
{
  return _value;
}

void ParamRawValueExpression::CalculateCheckValue(SumCalculator &sum) const
{
  Assert(_file);
  sum.Add(_value,_value.GetLength());
#if LOG_CHECKSUM >= 2
  LogF("  add CRC of '%s', result %x", cc_cast(_value), sum.GetResult());
#endif
}


float ParamRawValueExpression::GetFloat() const
{
  return _file->EvaluateFloat(_value);
}
int ParamRawValueExpression::GetInt() const
{
  return toLargeInt(_file->EvaluateFloat(_value));
}

int64 ParamRawValueExpression::GetInt64() const
{
  return to64bInt(_file->EvaluateFloat(_value));
}

void ParamRawValueExpression::SetValue( const RStringB &value )
{
  _value=value;
}

void ParamRawValueExpression::SetValue( float val )
{
  BString<256> buf;
  sprintf(buf,"%f",val);
  _value=buf.cstr();

  #if _ENABLE_REPORT
  // check if ok
  char *end;
  float dummy = strtod(buf, &end);
  (void)dummy;
  if (*end != 0)
  {
    ErrF("Setting invalid value %s", (const char *)buf);
    _value = "0";
  }
  #endif
}

void ParamRawValueExpression::SetValue(int val)
{
  BString<256> buf;
  sprintf(buf,"%d",val);
  _value=buf.cstr();
}

void ParamRawValueExpression::SetValue(int64 val)
{
  BString<256> buf;
  sprintf(buf,"%I64d",val);
  _value=buf.cstr();
}


void ParamRawValueExpression::SerializeBin(SerializeBinStream &f)
{
  // string are very likely to be class names
  // and there is high probability they can be reused
  ParamFileContext *context = (ParamFileContext *)f.GetContext();
  context->TransferString(f,_value);
}

//////////////////////////////////////////////////////////////////////////



#include <Es/Memory/normalNew.hpp>

class ParamRawArray
{
  protected:
  RefArray<IParamArrayValue> _value;

  public:
  void AddValue(float val);
  void AddValue(int val);
  void AddValue(int64 val);
  void AddValue(const RStringB &val);
  void AddExpression(const RStringB &val);
  //void AddValue(const char *val);
  IParamArrayValue *AddArrayValue();

  void Compact(){_value.Compact();}
  void Clear() {_value.Clear();}
  void Reserve(int count){_value.Reserve(count,count);}
  void Copy(const ParamRawArray &src) {_value = src._value;}

  void SetFile(ParamFile *file);

  size_t GetMemoryUsed() const;
  int GetSize() const {return _value.Size();}
  IParamArrayValue &GetValue( int i ) const;
  const IParamArrayValue &operator [] ( int i ) const {return GetValue(i);}

  void SetValue( int index, const RStringB &string );
  void SetValue( int index, float val );
  void DeleteValue( int index );  

  LSError Parse(QIStream &in, ParamFile *file);
  void Save(QOStream &f, int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);
  void CalculateCheckValue(SumCalculator &sum) const;
};

//class ParamArrayValueExpression

class ParamArrayValueArray: public IParamArrayValue,public ParamRawArray
{
  public:
  ParamArrayValueArray(){}

  RStringB GetValue() const {return RStringBEmpty;}
  RStringB GetValueRaw() const {return RStringBEmpty;}
  int GetInt() const {return 0;}
  float GetFloat() const {return 0;}
  int64 GetInt64() const {return 0;}

  void SetValue(const RStringB &val){}
  void SetValue(float val){}
  void SetValue(int val){}
  void SetValue(int64 val){}
  void SetFile(ParamFile *file) {ParamRawArray::SetFile(file);}

  bool IsTextValue() const {return false;}
  bool IsFloatValue() const {return false;}
  bool IsIntValue() const {return false;}
  bool IsInt64Value() const {return false;}

  bool IsExpression() const {return false;}
  bool IsArrayValue() const {return true;}

  size_t GetMemorySize() const {return sizeof(*this);}
  
  void Save(QOStream &f,int indent, const char *eol) const;
  void SerializeBin(SerializeBinStream &f);

  // may be array of values
  const IParamArrayValue *GetItem(int i) const {return &ParamRawArray::operator [](i);}
  int GetItemCount() const {return ParamRawArray::GetSize();}

  void AddValue(float val) {ParamRawArray::AddValue(val);}
  void AddValue(int val) {ParamRawArray::AddValue(val);}
  void AddValue(int64 val) {ParamRawArray::AddValue(val);}
  void AddValue(const RStringB &val) {ParamRawArray::AddValue(val);}
  IParamArrayValue *AddArrayValue() {return ParamRawArray::AddArrayValue();}

  void CalculateCheckValue(SumCalculator &sum) const;

  USE_FAST_ALLOCATOR
};


class ParamArray: public ParamEntry, public ParamRawArray
{
  ParamAccessMode _access;
  //ParamFile *_file;

  public:
  ParamArray( const RStringB &name );

  bool IsArray() const {return true;}

  //void AddValue(const IParamArrayValue &val);

  void SetFile(ParamFile *file){ParamRawArray::SetFile(file);}

  size_t GetMemorySize() const {return sizeof(*this);}
  size_t GetMemoryUsed() const;

  void Compact();
  void Clear();
  void ReserveArrayElements( int count );

  virtual void SetAccessMode(ParamAccessMode mode) {_access=mode;}
  virtual ParamAccessMode GetAccessMode() const {return _access;}

  bool EnableModification()
  {
    if (_access>=PAReadOnly)
    {
      RptF("Attempt to modify read-only item %s",(const char *)GetName());
      return false;
    }
    if (_access>=PAReadAndCreate)
    {
      RptF("Attempt to modify add-only item %s",(const char *)GetName());
      return false;
    }
    return true;
  }
  void AddValue(float val)
  {
    if (!EnableModification()) return;
    ParamRawArray::AddValue(val);
  }
  void AddValue(int val)
  {
    if (!EnableModification()) return;
    ParamRawArray::AddValue(val);
  }
  void AddValue(int64 val)
  {
    if (!EnableModification()) return;
    ParamRawArray::AddValue(val);
  }

  /*
  void AddValue(bool val)
  {
    if (!EnableModification()) return;
    ParamRawArray::AddValue(val);
  }
  */
  void AddValue(const RStringB &val)
  {
    if (!EnableModification()) return;
    ParamRawArray::AddValue(val);
  }
  /*
  void AddValue(const char *val)
  {
    if (!EnableModification()) return;
    ParamRawArray::AddValue(val);
  }
  */
  IParamArrayValue *AddArrayValue()
  {
    if (!EnableModification()) return NULL;
    return ParamRawArray::AddArrayValue();
  }

  int GetSize() const {return ParamRawArray::GetSize();}
  IParamArrayValue &GetValue( int i ) const {return ParamRawArray::GetValue(i);}
  void SetValue(int i, const RStringB &string ){ParamRawArray::SetValue(i,string);}
  void SetValue(int i, float val ){ParamRawArray::SetValue(i,val);}
  void SetValue(int i, int val ){ParamRawArray::SetValue(i,val);}
  void DeleteValue(int index) {ParamRawArray::DeleteValue(index);}
/*
  PackedColor GetPackedColor() const {return ParamRawArray::GetPackedColor();}
  Color GetColor() const {return ParamRawArray::GetColor();}
  operator SoundPars() const {return ParamRawArray::operator SoundPars();}
*/

  const IParamArrayValue &operator [] ( int i ) const
  {
    if (i>=_value.Size())
    {
      ErrorMessage(
        EMError,"Config: '%s' array does not have %d entries.",
        (const char *)GetName(),i+1
      );
    }
    return GetValue(i);
  }

  LSError Parse(QIStream &in, ParamFile *file);
  void Save(QOStream &f, int indent, const char *eol) const;

  virtual void SerializeBin(SerializeBinStream &f);
  virtual void CalculateCheckValue(SumCalculator &sum) const;

  USE_FAST_ALLOCATOR
};

size_t ParamArray::GetMemoryUsed() const
{
  return ParamRawArray::GetMemoryUsed();
}

void ParamArray::Compact()
{
  ParamRawArray::Compact();
}
void ParamArray::Clear()
{
  ParamRawArray::Clear();
}
void ParamArray::ReserveArrayElements( int count )
{
  ParamRawArray::Reserve(count);
}


#include <Es/Memory/debugNew.hpp>

static IParamArrayValue *CreateParamArrayValue(SerializeBinStream &f)
{
  // load type and create value
  Assert( f.IsLoading() );
  char type;
  f.Transfer(type);
  switch (type)
  {
    case SVGeneric:
      return new ParamArrayValue("");
    case SVFloat:
      return new ParamArrayValueFloat(0.0f);
    case SVInt:
      return new ParamArrayValueInt(0);
    case SVInt64:
      return new ParamArrayValueInt64(0);
    case SVArray:
      return new ParamArrayValueArray();
    case SVExpression:
      return new ParamArrayValueExpression("");
    default:
      ErrF("Unknown array value type %d",type);
      return new ParamArrayValue("");
  }

}


DEFINE_FAST_ALLOCATOR(ParamArrayValueArray)

/*
PackedColor ParamArrayValueArray::GetPackedColor() const
{
  return ParamRawArray::GetPackedColor();
}

SoundPars ParamArrayValueArray::GetSoundPars() const
{
  return ParamRawArray::operator SoundPars();
}
*/

void ParamArrayValueArray::Save(QOStream &f, int indent, const char *eol) const
{
  ParamRawArray::Save(f,indent+1,eol);
}

void ParamArrayValueArray::SerializeBin(SerializeBinStream &f)
{
  if (f.IsSaving())
  {
    char type = SVArray;
    f.Transfer(type);
  }
  ParamRawArray::SerializeBin(f);
}

void ParamArrayValueArray::CalculateCheckValue(SumCalculator &sum) const
{
  ParamRawArray::CalculateCheckValue(sum);
}


DEFINE_FAST_ALLOCATOR(ParamArray)


ParamArray::ParamArray( const RStringB &name)
:ParamEntry(name) //,_file(NULL)
{
  _access = PADefault;
}


void ParamRawArray::AddValue(float val){_value.Add(new ParamArrayValueFloat(val));}
void ParamRawArray::AddValue(int val){_value.Add(new ParamArrayValueInt(val));}
void ParamRawArray::AddValue(int64 val){_value.Add(new ParamArrayValueInt64(val));}
void ParamRawArray::AddValue(const RStringB &val){_value.Add(new ParamArrayValue(val));}
void ParamRawArray::AddExpression(const RStringB &val){_value.Add(new ParamArrayValueExpression(val));}

IParamArrayValue *ParamRawArray::AddArrayValue()
{
  IParamArrayValue *value = new ParamArrayValueArray();
  _value.Add(value);
  return value;
}

const IParamArrayValue &ParamEntry::operator [] ( int index ) const 
{
  const static ParamArrayValue nil("");
  ErrorMessage(EMError,"[]: '%s' not an array",(const char *)GetContext());
  return nil;
}
IParamClassSource *ParamEntry::GetSource() const
{
  Fail("Not supported");
  return NULL;
}

size_t ParamEntry::GetMemoryUsed() const
{
  // assume names are mostly shared and cost no indirect memory
  return 0;
}

size_t ParamEntry::GetMemorySize() const
{
  // return size of the entry
  return sizeof(*this);
}

RStringB ParamEntry::GetOwnerName() const
{
  Fail("Not supported");
  return RStringBEmpty;
}
RString ParamEntry::GetOwnerDebugName() const
{
  Fail("Not supported");
  return RStringBEmpty;
}
void ParamEntry::SetSource(IParamClassSource *source, bool subentries)
{
  // no owner can be set for single entries, only for classes
  Fail("Cannot set entry owner");
}
void ParamEntry::DisableStreaming(bool subentries)
{
  // no owner can be set for single entries, only for classes
  Fail("Cannot set entry streaming");
}

void ParamEntry::SetSource(const ParamEntry &source)
{
  // no owner can be set for single entries, only for classes
  Fail("Cannot set entry owner");
}

bool ParamEntry::CheckVisible(IParamVisibleTest &visible) const
{
  Fail("ParamEntry does not know if it is visible");
  return true;
}

ParamEntry::~ParamEntry()
{
}


size_t ParamRawArray::GetMemoryUsed() const
{
  size_t size = _value.GetMemoryAllocated();
  for (int i=0; i<_value.Size(); i++)
  {
    IParamArrayValue *val = _value[i];
    size += val->GetMemorySize();
  }
  // we could add size of the 
  return size;
}

void ParamRawArray::SetFile(ParamFile *file)
{
  // note: only ArrayValues of ParamArrays
  // loaded via Parse or SerializeBin have _file member set
  // SetFile is always called recursively after corresponding functions
  //_file = file;
  for (int i=0; i<_value.Size(); i++)
  {
    IParamArrayValue *value = _value[i];
    if (value) value->SetFile(file);
  }
}

IParamArrayValue &ParamRawArray::GetValue( int i ) const
{
  if( i>=_value.Size() )
  {
    //ErrorMessage(EMError,"Config: array does not have %d entries.",i+1);
    static ParamArrayValue errorValue("");
    return errorValue;
  }
  return *_value[i];
}

void ParamRawArray::SetValue( int index, const RStringB &val )
{
  while( index>_value.Size() )
  {
    _value.Add(new ParamArrayValue(""));
  }
  if (index>=_value.Size())
  {
    _value.Access(index);
    _value[index]= CreateParamArrayValue(val);
  }
  else
  {
    _value[index]->SetValue(val);
  }
}

void ParamRawArray::SetValue( int index, float val )
{
  while( index>_value.Size() )
  {
    _value.Add(new ParamArrayValue(""));
  }
  if (index>=_value.Size())
  {
    _value.Access(index);
    _value[index]= CreateParamArrayValue(val);
  }
  else
  {
    _value[index]->SetValue(val);
  }
}

void ParamRawArray::DeleteValue( int index )
{
  if ( index>=_value.Size() ) return;
  _value.Delete(index);
}


/// convert name to index
/**
It would be possible to work with indices only and getting names from ParamClass,
but for this some way to pass ParamClass across MapStringToClass would be necessary
*/
struct ParamEntryFindItem
{ 
  RStringB _name;
  int _index;
  
  ParamEntryFindItem(){}
  ParamEntryFindItem(const RStringB &name, int index):_name(name),_index(index){}
  const char *GetKey() const {return _name;}
};

TypeIsMovableZeroed(ParamEntryFindItem)

template <>
struct MapClassTraits<ParamEntryFindItem>: public MapClassTraitsNoCase<ParamEntryFindItem>
{
};

/// interface to paramentry searching
class ParamEntryIndex: public IParamEntryIndex
{
  MapStringToClass<
    ParamEntryFindItem,AutoArray<ParamEntryFindItem>
  > _index;
  
  public:
  ParamEntryIndex(const ParamClass *cls);
  virtual int Find(const char *name) const;
  virtual void Delete(const char *name);
  virtual void Add(const char *name, int index);
  virtual ~ParamEntryIndex() {}
};

ParamEntryIndex::ParamEntryIndex(const ParamClass *cls)
{
  _index.Reserve(cls->GetEntryCount());
  for (int i=0; i<cls->GetEntryCount(); i++)
  {
    RStringB name = cls->GetEntryName(i);
    _index.Add(ParamEntryFindItem(name,i));
  }
  _index.Compact();
}

int ParamEntryIndex::Find(const char *name) const
{
  const ParamEntryFindItem &item = _index.Get(name);
  if (_index.IsNull(item))
  {
    return -1;
  }
  return item._index;
}

void ParamEntryIndex::Delete(const char *name)
{
  _index.Remove(name);
}

void ParamEntryIndex::Add(const char *name, int index)
{
  _index.Add(ParamEntryFindItem(name, index));
}

void ParamClass::OptimizeFind() const
{
  if (NeedsRestoring())
  {
    RptF("%s: Cannot optimize entry find for restorable classes",cc_cast(GetContext()));
    return;
  }
  unconst_cast(this)->CreateEntryIndex();
}

void ParamClass::CreateEntryIndex()
{
  _index = new ParamEntryIndex(this);
}

int ParamClass::FindIndex(const char *name) const
{
  if (_index)
  {
    return _index->Find(name);
  }
  for( int i=0; i<_entries.Size(); i++ )
  {
    if (!_entries[i]) continue;
    if (strcmpi(_entries[i]->GetName(),name)==0) return i;
  }
  return -1;
}

size_t ParamClass::GetMemorySize() const
{
  return sizeof(*this);
}

size_t ParamClass::GetMemoryUsed() const
{
  // calculate all subclasses?
  size_t size = base::GetMemoryUsed();
  size += _entries.GetMemoryAllocated();

  for (int i=0; i<_entries.Size(); i++)
  {
    // check how much memory uses given object
    const ParamEntry *e = _entries[i];
    size += e->GetMemoryUsed();
    size += e->GetMemorySize();
  }
  return size;
}

/** asynchronous loading of entry content, not including base class */

bool ParamClass::RequestEntry(int index) const
{
  ParamEntry *entry = _entries[index];
  
  ParamClassPlaceholder *entryPH = entry->GetPlaceholderInterface();
  if (!entryPH)
  {
    // only placeholders need to be loaded
    return true;
  }
  return entryPH->_source->Request(entryPH->_sourceOffset,entryPH->_sourceSize);
}

/** asynchronous loading of base class name - used for entry preloading */

bool ParamClass::RequestEntryBaseName(int index, RStringB &baseName) const
{
  ParamEntry *entry = _entries[index];
  
  ParamClassPlaceholder *cls = entry->GetPlaceholderInterface();
  if (!cls)
  {
    // only placeholders need to be loaded
    return true;
  }
  
  // verify id is correct
  // create an empty (unloaded) ParamClass
  DoAssert(cls->_sourceOffset>=0);
  // we have the class, but it is empty
  // we need to fill it with value entries and class placeholders
  return cls->_source->RequestBaseName(baseName,*cls);
}

/**
Load entry (class) - convert a placeholder into a real entry 
*/

void ParamClass::LoadEntry(int index)
{
  // convert a placeholder into a real entry
  ParamEntry *entry = _entries[index];
  
  ParamClassPlaceholder *entryPH = entry->GetPlaceholderInterface();
  if (!entryPH)
  {
    // only placeholders need to be loaded
    return;
  }
  
  // verify id is correct
  // create an empty (unloaded) ParamClass
  ParamClass *cls = new ParamClass(entryPH->GetName());
  cls->_source = entryPH->_source;
  cls->_sourceOffset = entryPH->_sourceOffset;
  cls->_sourceSize = entryPH->_sourceSize;
  cls->_parent = ParamClassLockedPtr(this);
  DoAssert(cls->_sourceOffset>=0);
  _entries[index] = cls;
  #if DIAG_UNLOAD>=100
    LogF("Loading class %s from %d",(const char *)cls->GetContext(),cls->_sourceOffset);
  #endif
  // we have the class, but it is empty
  // we need to fill it with value entries and class placeholders
  cls->_source->Load(*cls,cls->GetName());
}
/**
Unload entry (class) - replace it with a corresponding placeholder
*/
void ParamClass::UnloadEntry(int index)
{
  ParamEntry *entry = _entries[index];
  ParamClass *cls = entry->GetClassInterface();
  if (!cls)
  {
    Fail("Cannot unload non-class");
    return;
  }
  #if DIAG_UNLOAD>=200
    LogF("Class %s unloaded",(const char *)cls->GetContext());
  #endif
  cls->UnlinkClasses();
  if (cls->_loadCount!=0)
  {
    LogF("Error: Cannot unload %s",(const char *)cls->GetContext());
    Fail("Bug - need to link parents and bases again");
    return;
  }
  // remove the class from the cache - it probably is there
  cls->RemoveFromTheCache();
  Assert(!cls->IsInList());
  
  ParamClassPlaceholder *placeh = new ParamClassPlaceholder(*cls);
  //placeh->_source = cls->_source;
  //placeh->_sourceOffset = cls->_sourceOffset;
  DoAssert(cls->_sourceOffset>=0);
  Assert(placeh->_source == cls->_source);
  Assert(placeh->_sourceOffset == cls->_sourceOffset);
  Assert(placeh->_sourceSize == cls->_sourceSize);
  _entries[index] = placeh;
}

/**
@param loadAsNeeded when false, may return placeholder instead of entries which are unloaded
*/
ParamEntryPtr ParamClass::Find(
  const char *name, bool parent, bool base,
  IParamVisibleTest &visible
) const
{
  // TODO: if anyone is operating on the class, the class should be locked
  //DoAssert(_loadCount>0);
  int i=FindIndex(name);
  if( i>=0 )
  {
    // check if this entry can be seen
    // if yes, we are done
    if (visible(*this,*_entries[i]))
    {
      unconst_cast(this)->LoadEntry(i);
      ParamEntry *entry = _entries[i];
      if (entry->IsClass())
      {
        return ParamEntryPtr(entry->GetClassInterface()); // entry locked while creating return value
      }
      else
      {
        return ParamEntryPtr(this,entry); // entry locked while creating return value
      }
    }
    LogF
    (
      "Entry %s in %s is not visible",
      (const char *)_entries[i]->GetName(),
      (const char *)GetName()
    );
  }
  if( base && _base!=NULL )
  {
    // FIX: do not search in the parent of base class (otherwise base::parent was checked before parent)
    // ParamEntryPtr found = _base->Find(name,parent,base,visible); 
    ParamEntryPtr found = _base->Find(name,false,base,visible); 
    if (found) return found;
  }
  if( parent && _parent!=NULL )
  {
    return _parent->Find(name,parent,base,visible);
  }
  return ParamEntryPtr();
}

ParamEntryPtr ParamClass::FindEntry( const char *name, IParamVisibleTest &visible) const
{
  Assert(strlen(name) > 0);
  return Find(name,false,true,visible);
}

bool ParamClass::CheckIfEntryExists(
  const char *name, IParamVisibleTest &visible
) const
{
  Assert(strlen(name) > 0);
  // search both bases and parents
  int i=FindIndex(name);
  if( i>=0 )
  {
    // check if this entry can be seen
    // if yes, we are done
    if (visible(*this,*_entries[i]))
    {
      return true;
    }
    LogF
    (
      "Entry %s in %s is not visible",
      (const char *)_entries[i]->GetName(),
      (const char *)GetName()
    );
  }
  if (_base!=NULL)
  {
    return _base->CheckIfEntryExists(name,visible);
  }
  if (_parent!=NULL)
  {
    return _parent->CheckIfEntryExists(name,visible);
  }
  return false;
}

ParamEntryPtr ParamClass::FindEntryNoInheritance(
  const char *name, IParamVisibleTest &visible
) const
{
  Assert(strlen(name) > 0);
  return Find(name,false,false,visible);
}

static RString PathFirstFolder(const char *path)
{
  const char *next = strchr(path,'/');
  if (!next) return "";
  return RString(path,next-path);
}

ParamEntryPtr ParamClass::FindEntryPath(
  const char *entryPath, IParamVisibleTest &visible
) const
{
  const char *path = entryPath;
  ParamEntryPtr entry = ParamEntryPtr(this);
  while (strlen(path)>0)
  {
    RString base = PathFirstFolder(path);
    if (base.GetLength()<=0)
    {
      ParamEntryPtr nEntry = entry->FindEntry(path);
      if (!nEntry) break;
      if (!nEntry->IsClass()) break;
      entry = nEntry;
      break;
    }
    else
    {
      ParamEntryPtr nEntry = entry->FindEntry(base);
      if (!nEntry) break;
      if (!nEntry->IsClass()) break;
      entry = nEntry;
      path += strlen(base)+1;
    }
  }
  return entry;
}

ConstParamEntryPtr ParamClass::FindParent() const
{
  return ConstParamEntryPtr(_parent);
}

ConstParamEntryPtr ParamClass::FindBase() const
{
  return ConstParamEntryPtr(_base);
}

RString ParamClassId::GetDebugName() const
{
  return _parent->GetContext()+"\\"+_name;
}

bool ParamClass::NeedsRestoring() const
{
  // if we are root, we cannot be restored / unloaded
  if (!_parent) return false;
  return _source && _source->CanUnload();
  //Fail("Not implemented");
  //return false;
}

bool ParamClass::RequestEntry(IParamClassSource *src, const char *name) const
{
  int index = FindIndex(name);
  // if entry does not exist, we pretend it is ready, and leave error handling to restore
  if (index<0)
  {
    // TODO: entry may exist in a parent class or a base class
    Fail("Base class requested not found");
    return true;
  }
  if (RequestEntry(index))
  {
    RStringB baseName;
    bool baseNameReady = RequestEntryBaseName(index,baseName);
    if (!baseNameReady)
    {
      // base name not ready yet
      return false;
    }
    // make sure base class is ready as well
    if (baseName.GetLength()>0)
    {
      bool baseReady = RequestEntry(src,baseName);
      if (!baseReady)
      {
        return false;
      }
    }
    // if entry is ready, load it
    unconst_cast(this)->LoadEntry(index);
    // update entry position in the cache?
    return true;
  }
  return false;
}

ParamClassPtr ParamClass::Restore(IParamClassSource *src, const char *name)
{
  // make sure given member is loaded
  // first try finding it
  // restoring is implemented inside of Find
  ParamEntryPtr entry = Find(name,false,false,DefaultAccess);
  if (entry)
  {
    Assert(entry->IsClass());
    return ParamClassPtr(entry->GetClassInterface());
  }
  return ParamClassPtr();
  
}

ParamClass *ParamClass::RestoreLinkDeep(const LinkId &id)
{
  // create a class based on the link
  // we need to know the pointer to the parent
  // because class will be owned by the parent
  Assert(id._parent);
  ParamClassPtr cls = id._parent->Restore(id._source,id._name);
  cls->DeepLockLink();
  return cls.GetPointer();
}

ParamClass *ParamClass::RestoreLinkShallow(const LinkId &id)
{
  // create a class based on the link
  // we need to know the pointer to the parent
  // because class will be owned by the parent
  Assert(id._parent);
  ParamClassPtr cls = id._parent->Restore(id._source,id._name);
  cls->ShallowLockLink();
  return cls.GetPointer();
}

bool ParamClass::RequestRestoreLinkDeep(const LinkId &id, bool noRequest)
{
  // TODO: respect noRequest
  return id._parent->RequestEntry(id._source,id._name);
}

bool ParamClass::RequestRestoreLinkShallow(const LinkId &id, bool noRequest)
{
  // TODO: respect noRequest
  return id._parent->RequestEntry(id._source,id._name);
}

void ParamClass::DeepLockLink()
{
  //Fail("Not implemented");
  // lock the class and all subclasses
  AddRefLoad();
  for (int i=0; i<_entries.Size(); i++)
  {
    // do we know all classes should already be loaded?
    //Assert(!_entries[i]->GetPlaceholderInterface());
    LoadEntry(i);
    ParamEntry *entry = _entries[i];
    ParamClass *entryC = entry->GetClassInterface();
    if (!entryC) continue;
    entryC->DeepLockLink();
  }
  if (_base)
  {
    _base->DeepLockLink();
  }
  
}
void ParamClass::DeepUnlockLink()
{
  // when deep unlocking, we know everything in the scope was deep locked
  // addon/config loading on the fly could present a problem here
  // newly loaded addon needs to respect the lock of the class where is it loaded to
  if (_base)
  {
    _base->DeepUnlockLink();
  }
  for (int i=0; i<_entries.Size(); i++)
  {
    // do we know all classes should already be loaded?
    Assert(!_entries[i]->GetPlaceholderInterface());
    ParamEntry *entry = _entries[i];
    ParamClass *entryC = entry->GetClassInterface();
    if (!entryC) continue;
    entryC->DeepUnlockLink();
  }
  
  ReleaseLoad();
}

class ParamEntryAllVisible: public IParamVisibleTest
{
  public:
  bool operator () (const ParamEntry &entry) {return true;}
  bool operator () (const ParamEntry &parent, const ParamEntry &entry) {return true;}
};

static ParamEntryAllVisible DefAccess;

IParamVisibleTest &DefaultAccess = DefAccess;

IParamClassSource *ParamClass::GetSource() const
{
  return _source;
}

RStringB ParamClass::GetOwnerName() const
{
  if (!_source)
  {
    Fail("No owner");
    return RStringB();
  }
  return _source->GetOwnerName();
}

RString ParamClass::GetOwnerDebugName() const
{
  if (!_source)
  {
    Fail("No owner");
    return RString();
  }
  return _source->GetOwnerDebugName();
}

void ParamClass::DisableStreaming(bool subentries)
{
  if (_loadCount==0 && _source && _source->CanUnload())
  {
    // if the class is cached, remove it from the cache
    RemoveFromTheCache();
  }
  _source=NULL;
  if (!subentries) return;
  for (int i=0; i<_entries.Size(); i++)
  {
    LoadEntry(i);
    ParamEntry *entry = _entries[i];
    if (entry->IsClass())
    {
      // append entry name to the path
      // if entry is a placeholder, one source is enough
      _entries[i]->DisableStreaming(subentries);
    }
  }
}
void ParamClass::SetSource(IParamClassSource *source, bool subentries)
{
  _source=source;
  // _sourceOffset = -1;
  if (_loadCount==0 && _source && _source->CanUnload())
  {
    ParamFile *file = source->GetFile();
    // class need not to be loaded, add it into the class cache
    file->CacheInsert(this);
  }
  if (!subentries) return;
  for (int i=0; i<_entries.Size(); i++)
  {
    ParamEntry *entry = _entries[i];
    if (entry->IsClass())
    {
      // append entry name to the path
      // if entry is a placeholder, one source is enough
      _entries[i]->SetSource(source,subentries);
    }
  }
}

void ParamClass::SetSource(const ParamEntry &entry)
{
  if (!entry.IsClass())
  {
    Fail("Cannot get a source from non-class");
    return;
  }
  const ParamClass *cls = entry.GetClassInterface();
  if (cls)
  {
    _source = cls->_source;
    _sourceOffset = cls->_sourceOffset;
    _sourceSize = cls->_sourceSize;
  }
  else
  {
    Fail("Not a class?");
  }
}

bool ParamClass::CheckVisible(IParamVisibleTest &visible) const
{
  if (_parent)
  {
    if (!visible(*_parent,*this)) return false;
    return _parent->CheckVisible(visible);
  }
  else
  {
    return visible(*this);
  }
}

bool ParamClass::RequestAllEntries(bool includeSubclasses) const
{
  bool ret = true;
  for(int i=0; i<_entries.Size(); i++)
  {
    const ParamEntry *entry = _entries[i];
    
    if (!entry->GetPlaceholderInterface())
    {
      const ParamClass *entryClass = entry->GetClassInterface();
      if (includeSubclasses && entryClass)
      {
        if (!entryClass->RequestAllEntries())
        {
          ret = false;
        }
      }
      continue;
    }
    if (!RequestEntry(i))
    {
      ret = false;
    }
  }
  return ret;
}

bool ParamClass::Request(const char *name, bool includeSubclasses) const
{
  // if class does not support streaming, it is always ready
  if (!_source) return true;
  bool ready = RequestEntry(_source,name);
  if (!ready) return false;
  // class is now loaded - check bases if necessary
  // how can we know what the base class of the source is without loading it?
  
  bool ret = true;

  ParamEntryPtr requestedEntry = FindEntry(name);
  if (requestedEntry.IsNull()) return true;
  
  // class itself is loaded, but its children may be unloaded
  ParamClass *requestedClass = requestedEntry->GetClassInterface();
  for (const ParamClass *load=requestedClass; load; load=includeSubclasses ? load->_base.GetPointer() : NULL)
  {
    // make sure all members are loaded - we may need any of them
    if (!load->RequestAllEntries())
    {
      ret = false;
    }
  }
  return ret; 
}


ParamEntryVal ParamClass::operator >> ( const char *name ) const
{
  ParamEntryPtr entry = FindEntry(name,DefaultAccess);
  if( entry )
  {
    if (entry->IsClass())
    {
      return ParamEntryVal(*entry->GetClassInterface());
    }
    else
    {
      return ParamEntryVal(*this,*entry);
    }
  }
  ErrorMessage(EMError,"No entry '%s'.",(const char *)GetContext(name));
  return ParamEntryVal(GParamEntryError);
}

float ParamClass::GetFloatDef(const char* name, float defVal /* = 0.0f */) const
{
	ParamEntryPtr entry = FindEntry(name,DefaultAccess);

	if (entry && entry->IsFloatValue())
		return entry->operator float();

	return defVal;
}

int ParamClass::GetIntDef(const char* name, int defVal /* = 0 */) const
{
	ParamEntryPtr entry = FindEntry(name,DefaultAccess);

	if (entry && entry->IsIntValue())
		return entry->operator int();

	return defVal;
}

bool ParamClass::GetBoolDef(const char* name, bool defVal /* = false */) const
{
	ParamEntryPtr entry = FindEntry(name,DefaultAccess);

	if (entry && entry->IsIntValue())
		return entry->operator bool();

	return defVal;
}

ParamEntry::ParamEntry(const ParamEntry &src)
{
  Fail("Not supported");
}

void ParamEntry::operator =(const ParamEntry &src)
{
  Fail("Not supported");
}

ParamClassPtr ParamClass::GetClass( const char *name ) const
{
  ParamEntryPtr entry=Find(name,false,true,DefaultAccess);
  if (!entry)
  {
    ErrorMessage(EMError,"No section '%s' in '%s'",(const char *)name,(const char *)GetName());
    return ParamClassPtr(&GParamEntryError);
  }
  const ParamClass *section = entry->GetClassInterface();
  if( !section )
  {
    ErrorMessage(EMError,"No section '%s' in '%s'",(const char *)name,(const char *)GetName());
    return ParamClassPtr(&GParamEntryError);
  }
  return ParamClassPtr(section);
}

RStringB ParamClass::GetEntryName( int i ) const
{
  return _entries[i]->GetName();
}

ParamEntryVal ParamClass::GetEntry( int i ) const
{
  // TODO: if anyone is operating on the class, the class should be locked
  //DoAssert(_loadCount>0);
  unconst_cast(this)->LoadEntry(i);
  const ParamEntry *entry = _entries[i];
  return ParamEntryVal(*this,*entry);
}

bool ParamClass::IsDerivedFrom(ParamEntryPar parent) const
{
  const ParamClass *base=this;
  while( base )
  {
    if( base==&parent ) return true;
    base=base->_base;
  }
  return false;
}

void ParamClass::Add( const RStringB &name, float val )
{
  Delete(name);

  ParamEntry *nEntry=new ParamValueFloat(name);
  nEntry->SetValue(val);
  NewEntry(nEntry,true);
}

void ParamClass::Add( const RStringB &name, int val )
{
  ParamEntryPtr entry=FindEntryNoInheritance(name,DefaultAccess);
  if( !entry )
  {
    ParamEntry *nEntry=new ParamValueInt(name);
    nEntry->SetValue(val);
    NewEntry(nEntry,true);
    return;
  }
  entry.GetPointer()->SetValue(val);
}

void ParamClass::Add( const RStringB &name, int64 val )
{
  ParamEntryPtr entry=FindEntryNoInheritance(name,DefaultAccess);
  if( !entry )
  {
    ParamEntry *nEntry=new ParamValueInt64(name);
    nEntry->SetValue(val);
    NewEntry(nEntry,true);
    return;
  }
  entry.GetPointer()->SetValue(val);
}

void ParamClass::Delete(const RStringB &name)
{
  int index = FindIndex(name);
  if (index >= 0)
  {
    ParamClass *e = _entries[index]->GetClassInterface();
    if (e)
    {
      // make sure we can unlink safely
      if (_source && _source->CanUnload())
      {
        UnloadEntry(index);
      }
      else
      {
        e->UnlinkClasses();
      }
    }
    _entries.DeleteAt(index);
    if (_index)
    {
      _index->Delete(name);
    }
  }
  //_entries.DeleteKey(name);
}

/*!
\patch_internal 1.44 Date 2/13/2002 by Ondra
- Fixed: Config protection turned off during config reload.
*/

void ParamClass::SetAccessModeForAll(ParamAccessMode mode)
{
  _access = mode;
  // traverse all entries
  for (int i=0; i<GetEntryCount(); i++)
  {
    _entries[i]->SetAccessModeForAll(mode);
  }
}

void ParamClass::AccessDenied(const char *name)
{
  if (_access>=PAReadOnly)
  {
    LogF("Trying to modify read-only entry %s",(const char *)GetContext(name));
  }
  else if (_access>=PAReadAndCreate)
  {
    LogF("Trying to modify add-only entry %s",(const char *)GetContext(name));
  } 
}


void ParamClass::Add( const RStringB &name, const RStringB &val )
{
  if (_access>=PAReadOnly)
  {
    AccessDenied(name);
    return;
  }
  ParamEntryPtr entry=FindEntryNoInheritance(name,DefaultAccess);
  if( !entry )
  {
    ParamEntry *nEntry=new ParamValue(name);
    nEntry->SetValue(val);
    NewEntry(nEntry,true);
    return;
  }
  else if (_access>=PAReadAndCreate)
  {
    AccessDenied(name);
    return;
  }
  entry.GetPointer()->SetValue(val);
}

/*!
\patch 1.82 Date 8/22/2002 by Ondra
- Optimized: Large text config loading and game saving is now faster.
\patch_internal 1.82 Date 8/22/2002 by Ondra
- Optimized: name is AddClass is sometimes guaranteed to be unique
and there is no to check if entry with given name already exists.
Example: ParamArchive serialization of arrays.
*/

ParamClassPtr ParamClass::AddClass( const RStringB &name, bool guaranteedUnique, bool notdefined)
{
  if (_access>=PAReadOnly)
  {
    AccessDenied(name);
    return ParamClassPtr();
  }
  ParamEntryPtr entry;
  if (guaranteedUnique)
  {
    #if _DEBUG
      entry=FindEntryNoInheritance(name,DefaultAccess);
      if (entry)
      {
        Fail("Guaranteed unique entry already present");
        RptF("  entry %s",(const char *)name);
      }
    #endif
  }
  else
  {
    entry=FindEntryNoInheritance(name,DefaultAccess);
  }
  if( !entry )
  {
    ParamClass *nEntry=notdefined?new ParamClassDecl(name):new ParamClass(name);
    nEntry->_parent = ParamClassLockedPtr(this);
    NewEntry(nEntry,true);
    return ParamClassPtr(nEntry);
  }
  DoAssert(entry->IsClass());
  return ParamClassPtr(entry->GetClassInterface());
}

ParamEntryPtr ParamClass::AddArray( const RStringB &name)
{
  if (_access>=PAReadOnly)
  {
    AccessDenied(name);
    return ParamEntryPtr();
  }
  ParamEntryPtr entry=FindEntryNoInheritance(name,DefaultAccess);
  if( !entry )
  {
    ParamEntry *nEntry=new ParamArray(name);
    NewEntry(nEntry,true);
    return ParamEntryPtr(this,nEntry);
  }
  else
  {
    DoAssert(entry->IsArray());
    entry->Clear();
    return entry;
  }
}

void ParamClass::SetFile(ParamFile *file)
{
  for (int i=0; i<_entries.Size(); i++)
  {
    ParamEntry *entry = _entries[i];
    if (entry) entry->SetFile(file);
  }
}

const ParamClass *ParamClass::GetFile() const
{
  if (_parent) return _parent->GetFile();
  return this;
}

void ParamClass::NewEntry(ParamEntry *entry, bool guaranteedUnique)
{
  int index = -1;
  #if !_RELEASE
    if (guaranteedUnique)
    {
      index=FindIndex(entry->GetName());
      if (index>=0)
      {
        Fail("Guaranteed unique Entry already exists");
        ParamEntry *entry=_entries[index];
        RptF
        (
          "Config: '%s' already defined in '%s'.",
          (const char *)entry->GetName(),(const char *)GetName()
        );
      }
    }
    else
    {
      index=FindIndex(entry->GetName());
    }
  #endif
  if( index>=0 )
  {
    // ??? why error message when not guaranteed unique
    Fail("Entry already exists");
    ParamEntry *entry=_entries[index];
    ErrorMessage
    (
      "Config: '%s' already defined in '%s'.",
      (const char *)entry->GetName(),(const char *)GetName()
    );
    //_entries.Replace(index,entry);
    _entries[index]=entry;
  }
  else
  {
    index = _entries.Add(entry);
    if (_index)
    {
      _index->Add(entry->GetName(), index);
    }
  }
}

RString ParamEntry::GetContext( const char *member ) const
{
  BString<512> buf1;
  bool first=false;
  if( member ) strcpy(buf1,member),first=true;
  else strcpy(buf1,"");
  const ParamEntry *src=this;
  while( src )
  {
    BString<512> buf2;
    buf2 = buf1;
    if( src->GetName() ) strncpy(buf1,src->GetName(),sizeof(buf1));
    else {strcpy(buf1,"");Fail("Bad context");}
    if( first ) strcat(buf1,"."),first=false;
    else strcat(buf1,"/");
    strcat(buf1,buf2);
    const ParamClass *cls = src->GetClassInterface();
    if( !cls ) break;
    src=cls->_parent;
  }
  return buf1.cstr();
}

LSError ParamClass::Parse( QIStream &in, ParamFile *file)
{
  int c;
  // parse section content
  for(;;)
  {
    c=in.get();
    while( isspace(c) ) c=in.get();
    if (in.fail())
    {
      ParsingError(in, "%s: File read error", (const char *)GetContext());
      return LSBadFile;
    }
    if (in.eof())
    {
      if (this == file) return LSOK; // correct on the file level
      ParsingError(in, "%s: Missing '}'", (const char *)GetContext());
      return LSStructure;
    }
    if (c == '#')
    {
      if (!ProcessLineNumber(in))
      {
        ParsingError(in, "%s: Wrong # directive",(const char *)GetContext());
        return LSStructure;
      }
      continue;
    }
    if( c=='}' )
    {
      c=in.get();
      while( (isspace(c) || c==';') && c!=EOF) c=in.get();
      if (c!=EOF) in.unget();
      break; // section end reached
    }
    if (c!=EOF) in.unget();
    WordBuf word;
    GetAlphaWord(word,in);
    // word is entry name
    SRef<ParamEntry> newEntry;
    if (!strcmp(word, "delete"))
    {
      // "delete" may be forgotten now
      GetAlphaWord(word, in);

      // section header
      int c=in.get();
      while( isspace(c) ) c=in.get();
      ParamClass *section;
      if (c == ';')
      {
        section = new ParamClassDelete(word);
        section->_parent = ParamClassLockedPtr(this);
      }
      else
      {
        ParsingError(in, "%s: '%c' encountered instead of ';'", (const char *)GetContext(), c);
        return LSStructure;
      }
      newEntry=section;
    }
    else if( !strcmp(word,"class") )
    {
      // "class" may be forgotten now
      GetAlphaWord(word,in);
      
      // section header
      int c=in.get();
      while( isspace(c) ) c=in.get();
      ParamClass *section;
      if (c==';')
      {
        // forward declaration detected
        section = new ParamClassDecl(word);
        section->_parent=ParamClassLockedPtr(this);
      }
      else
      {
        section = new ParamClass(word);
        section->_parent=ParamClassLockedPtr(this);
        if (c==':')
        {
          // base class
          WordBuf base;
          GetAlphaWord(base,in);
          ParamEntryPtr entry=Find(base,true,true,DefaultAccess); // search parents and bases of my parent
          if( !entry )
          {
            ParsingError(in, "%s: Undefined base class '%s'",(const char *)GetContext(word),(const char *)base);
            delete section;
            return LSStructure;
          }
          ParamClass *baseClass = entry->GetClassInterface();
          if( !baseClass )
          {
            ParsingError(in, "%s: '%s' is not class",(const char *)GetContext(word),(const char *)base);
            return LSStructure;
          }
          Assert(baseClass!=section);
          section->_base = ParamClassLockedPtr(baseClass);
          c=in.get();
        }
        // find opening brace
        while( c!='{' )
        {
          if( !isspace(c) )
          {
            ParsingError(in, "%s: '%c' encountered instead of '{'",(const char *)GetContext(),c);
            delete section;
            return LSStructure;
          }
          c=in.get();
        }
        // parse section content
        LSError res=section->Parse(in, file);
        if (res!=LSOK) return res;
      }
      newEntry=section;
    }
    else if (!strcmp(word, "enum"))
    {
      // "enum" may be forgotten now
      GetAlphaWord(word, in);
      // enum name not used

      // find opening brace
      int c = in.get();
      while (c != '{')
      {
        if (!isspace(c))
        {
          ParsingError(in, "%s: '%c' encountered instead of '{'", (const char *)GetContext(), c);
          return LSStructure;
        }
        c = in.get();
      }
      int enumValue = 0;
      do
      {
        GetAlphaWord(word, in);
        RString name = word;
        c = in.get();
        while (isspace(c)) c = in.get();
        if (c == '=')
        {
          c = in.get();
          while (isspace(c)) c=in.get();
          in.unget();
          GetWord(word, in,",}");
          c = in.get();
          enumValue = toLargeInt(file->EvaluateFloatInternal(word));
        }
        file->VarSetFloatInternal(name, enumValue, true, true);
        enumValue++;
      } while (c == ',');
      if (c == '}')
      {
        c = in.get();
        while (isspace(c) || c == ';') c = in.get();
        in.unget();
      }
      else
      {
        ParsingError(in, "%s: '%c' encountered instead of '}'", (const char *)GetContext(), c);
        return LSStructure;
      }
    }
    else if (!strcmp(word, "__EXEC"))
    {
      // find opening brace
      int c = in.get();
      while (c != '(')
      {
        if (!isspace(c))
        {
          ParsingError(in, "%s: '%c' encountered instead of '('", (const char *)GetContext(), c);
          return LSStructure;
        }
        c = in.get();
      }
      GetWord(word, in, ")");
      c = in.get();
      if (c == ')')
      {
        c = in.get();
        while (isspace(c) || c == ';') c = in.get();
        in.unget();
      }
      else
      {
        ParsingError(in, "%s: '%c' encountered instead of ')'", (const char *)GetContext(), c);
        return LSStructure;
      }
      file->ExecuteInternal(word);
      // GGameState.Execute(word);
    }
    else
    {
      // word should be value or array
      c=in.get();
      if( c=='[' )
      {
        // word is array name
        ParamArray *array=new ParamArray(word);
        //array->SetFile(file);
        c=in.get();
        while( isspace(c) ) c=in.get();
        if( c!=']' )
        {
          ParsingError(in, "Config: %s: '%c' encountered instead of ']'",(const char *)GetContext(word),c);
          return LSStructure;
        }
        c=in.get();
        while( isspace(c) ) c=in.get();
        if( c!='=' )
        {
          ParsingError(in, "Config: %s: '%c' encountered instead of '='",(const char *)GetContext(word),c);
          return LSStructure;
        }
        LSError res=array->Parse(in,file);
        if (res!=LSOK) return res;
        c=in.get();
        while( isspace(c) ) c=in.get();
        if( c!=';' )
        {
          ParsingError(in, "%s: '%c' encountered instead of ';'",(const char *)GetContext(array->GetName()),c);
          return LSStructure;
        }
        newEntry=array;
      }
      else
      {
        while( isspace(c) ) c=in.get();
        if( c!='=' )
        {
          WordBuf errorContext;
          GetWord(errorContext,in,"\n");
          RptF("Error context %s",cc_cast(errorContext));
          ParsingError(in, "'%s': '%c' encountered instead of '='",(const char *)GetContext(word),c);
          return LSStructure;
        }
        RStringB valueName = word;
        c=in.get();
        while( isspace(c) ) c=in.get();
        bool expression = c=='@';
        if (!expression)
        {
          in.unget();
        }
        bool quot;
        GetWord(word, in, ";}\n\r", &quot);
        c = in.get();
        if (c == '}')
        {
          in.unget(); // handle '}' outside
          ParsingWarning(in, "'%s': Missing ';' prior '}'", (const char *)GetContext(valueName));
        }
        else if (c != ';')
        {
          if (c!='\n' && c!='\r')
          {
            if (!quot)
            {
              ParsingError(in, "'%s': '%c' encountered instead of ';'",(const char *)GetContext(valueName),c);
              return LSStructure;
            }
            // we cannot handle the character - return it back
            in.unget();
          }
          ParsingWarning(in, "'%s': Missing ';' at the end of line",(const char *)GetContext(valueName));
        }

        ParamEntry *value=NULL;
        if (expression)
        {
          value = new ParamValueExpression(valueName);
          value->SetValue(word);          
        }
        else if (!quot)
        {
          if (strncmp(word, "__EVAL", 6) == 0)
          {
            strcpy(word, cc_cast(file->EvaluateStringInternal(word + 6)));
          }

          // check if value is integer or float
          // check for integer: convert using all letters must be 
          bool ok = false;
          int val=ScanInt(word,ok);
          if (ok)
          {
            value = new ParamValueInt(valueName);
            value->SetValue(val);
          }
          else
          {
            int64 val = ScanInt64(word,ok);
            if (ok)
            {
              value = new ParamValueInt64(valueName);
              value->SetValue(val);
            }
            else
            {
              float val = ScanFloat(word,ok);
              if (ok)
              {
                value = new ParamValueFloat(valueName);
                value->SetValue(val);
              }
            }
          }
        }
        if (!value)
        {
          value = new ParamValue(valueName);
          value->SetValue(word);
        }

        //value->SetFile(file);
        // done recursivelly in the end of parsing
        newEntry=value;
      }
    }
    // check for overload
    if (newEntry)
    {
      int baseIndex=FindIndex(newEntry->GetName());
      if( baseIndex<0 )
      {
        if (newEntry->GetName()==AccessString)
        {
          newEntry->SetFile(file);
          _access = (ParamAccessMode)newEntry->GetInt();
        }
        _entries.Add(newEntry);
      }
      else
      {
        ParsingError(in, "%s: Member already defined.",(const char *)GetContext(newEntry->GetName()));
        // if the new entry is a class, we need to prepare it for destruction
        ParamClass *newClass = newEntry->GetClassInterface();
        if (newClass)
        {
          newClass->UnlinkClasses();
        }
      }
    }
  }

  _entries.Compact();
  // class parsed
  // check access protection mode
  CheckInheritedAccess();
  return LSOK;
}

//! Updates class from source class
/*!
\return true when all members were overloaded by the new class
\patch_internal 1.01 Date 06/13/2001 by Jirka - fixed error in config reload
- when new entry was added to class and this entry was in base class, base class entry was updated
- instead new entry must be added to updated class
\patch_internal 1.11 Date 08/03/2001 by Ondra
- New: access protection and CRC verification of ParamFiles.
\patch 1.43 Date 1/23/2002 by Ondra
- Fixed: Addons could change config entries that should not be changed,
namely base class definition of any class, leading to damaged functionality
or bogus "... uses modified config file" messages.
\patch 1.53 Date 4/26/2002 by Ondra
- Fixed: Addons could make main config unusable
by omitting base class of redefined class.
\patch 1.63 Date 5/30/2002 by Ondra
- Changed: Increased addon safety:
Addons can now only add classes, not single entries, in ReadAndCreate config areas.
*/

bool ParamClass::Update(const ParamClass &source, bool protect, bool canUpdateBase)
{
  bool containsDeleteEntry = false;
  return Update(source, containsDeleteEntry, protect, canUpdateBase);
}

bool ParamClass::Update(const ParamClass &source, bool &containsDeleteEntry, bool protect, bool canUpdateBase)
{
#ifdef DEBUG_UI_RSC
  if (_parent && !stricmp(cc_cast(GetName()),"Controls") && !stricmp(cc_cast(_parent->GetName()),"RscDisplayNotFreeze"))
  {
    RptF("Updating class: %s", cc_cast(GetName()));
  }
#endif
  Assert(source.IsDefined());
  ParamAccessMode access = _access;
  if (!protect) access = PAReadAndWrite;
  if (access>=PAReadOnly)
  {
    if (source.GetEntryCount()>0)
    {
      BString<256> buf;
      sprintf
      (
        buf,
        "** Update **, by %s",
        (const char *)source.GetContext()
      );
      AccessDenied(buf);
    }
    return false;
  }
  // update base first


  bool baseOverloadedThis = false;

  //dstCls->_parent = ParamClassLockedPtr(this);
  if(source._base)
  {
    // if we have no parent, we cannot have a base class
    RString baseName = source._base->GetName();
    ParamEntryPtr baseEntry = _parent->Find
    (
      baseName, true, true, DefaultAccess
    );
    // FIX: class inherited from class with the same name
    if (baseEntry == this)
    {
      baseEntry = NULL;
      if (_parent->_base) baseEntry = _parent->_base->Find(baseName, true, true, DefaultAccess);
      if (!baseEntry && _parent->_parent) baseEntry = _parent->_parent->Find(baseName, true, true, DefaultAccess);
    }

    if (!baseEntry || !baseEntry->IsClass())
    {
      RptF(
        "Error updating '%s' by '%s', base '%s'",
        cc_cast(GetContext()),cc_cast(source.GetContext()),cc_cast(baseName)
      );
      ErrorMessage(
        "%s: Cannot find base class '%s'",
        (const char *)GetContext(),
        (const char *)source._base->GetName()
      );
      return false;
    }
    // check if changing base is enabled
    if (baseEntry.GetPointer()!=_base)
    {
      #if 1
      if (!canUpdateBase)
      {
        // changing of base class is quite unusual situation and can lead to unwanted config problems 
        RptF
        (
          "Updating base class %s->%s, by %s",
          _base ? (const char *)_base->GetName() : "",
          (const char *)baseEntry->GetName(),
          (const char *)source.GetContext()
        );
      }
      #endif
      if (GetAccessMode()<PAReadAndCreate)
      {
        // base can be changed only if no content protection is applied
        _base = ParamClassLockedPtr(baseEntry->GetClassInterface());
        Assert((ParamClass *)_base!=this); //FIXED BY BREDY error C2593: 'operator !=' is ambiguous
                                            //Assert(_base!=this); 
        baseOverloadedThis = true;
      }
      else
      {
        BString<256> buf;
        sprintf
        (
          buf,
          "** Update base %s->%s **, by %s",
          _base ? (const char *)_base->GetName() : "",
          (const char *)baseEntry->GetName(),
          (const char *)source.GetContext()
        );
        AccessDenied(buf);
      }
    }
    else
    {
      // base class is the same as it was - we can assume new owner took ownership
      baseOverloadedThis = true;
    }
  }
  else
  {
    if (_base)
    {
      if (!canUpdateBase)
      {
        // changing of base class is quite unusual situation and can lead to unwanted config problems 
        RptF
        (
          "Updating base class %s->%s, by %s",
          (const char *)_base->GetName(),
          "",
          (const char *)source.GetContext()
        );
      }

      if (GetAccessMode()<PAReadAndCreate)
      {
        _base = ParamClassLockedPtr(NULL);
        baseOverloadedThis = true;
      }
      else
      {
        BString<256> buf;
        sprintf
        (
          buf,
          "** Update base %s-><null> **, by %s",
          _base ? (const char *)_base->GetName() : "",
          (const char *)source.GetContext()
        );
        AccessDenied(buf);
      }
    }
    else
    {
      // base class is the same as it was - we can assume new owner took ownership
      baseOverloadedThis = true;
    }
  }
  
  // detect singular case: all entries and base are overloaded
  // in this case owner can be the new 
  // track if all entries were defined by the source class
  int countEntriesOverloaded = 0;
  for (int i=0; i<source.GetEntryCount(); i++)
  {
    // TODO: unloaded entries might sometimes be merged without loading
    unconst_cast(source).LoadEntry(i);
    ParamEntryVal srcEntryVal = source.GetEntry(i);
    const ParamEntry &srcEntry = srcEntryVal;
    // FIX - do not update base class, instead add new class
    ParamEntryPtr dstEntry = Find(srcEntry.GetName(), false, false, DefaultAccess);
    if (srcEntry.IsDelete())
    {
      if (dstEntry)
      {
        if (access >= PAReadAndCreate)
        {
          if (dstEntry.NotNull())
            AccessDenied("**Update** entry" + dstEntry->GetName());
          else
            AccessDenied("**Update**");

          return false;
        }
        bool canDelete = true;
        ParamClass *dstClass = dstEntry->GetClassInterface();
        if (dstClass)
        {
          int remainingLinks = dstClass->_loadCount;
          for (int i=0; i<dstClass->GetEntryCount(); i++)
          {
            ParamEntryVal entry = dstClass->GetEntry(i);
            if (entry.IsClass()) remainingLinks--;
          }
          canDelete = remainingLinks <= 1; // dstEntry
        }
        if (canDelete)
        {
          dstEntry = NULL;
          Delete(srcEntry.GetName());
        }
        else
        {
          RptF("Cannot delete class %s, it is referenced somewhere (used as a base class probably).", cc_cast(dstEntry->GetName()));
        }
        // for deleted entries DisableStreaming() should be used
        DisableStreaming(false);
        containsDeleteEntry = true;
      }
    }
    else if (srcEntry.IsClass())
    {
      bool defined=srcEntry.IsDefined();
      if (!defined && FindEntry(srcEntry.GetName()).IsNull())
      {
        LogF("Note: '%s' is declared, but definition was not found. Creating empty class (source: %s)",srcEntry.GetName().Data(),source.GetRoot()->GetName().Data());
        defined = true;   //force class is defined. It will cause creation of new empty class.
      }
      if (defined) 
      {
        int originalEntryCount = 0;
        bool isNewEntry = false;
        if (!dstEntry)
        {
          ParamClass *nEntry = new ParamClass(srcEntry.GetName());
          GetContext();
          nEntry->SetSource(srcEntry);
          NewEntry(nEntry,true);
          dstEntry = ParamEntryPtr(nEntry);
          isNewEntry = true;
        }
        else if (!dstEntry->IsClass())
        {
          ErrF("Cannot update non class from class %s",cc_cast(srcEntry.GetContext()));
          return false;
        }
        else if (!dstEntry->IsDefined())
        {
          // if destination class is not defined, we cannot update it
          RptF("Cannot update undefined class %s",cc_cast(dstEntry->GetName()));
          dstEntry->SetSource(srcEntry);
        }
        else if (dstEntry->GetEntryCount()==0)
        {
          // entry existed, but was empty
          // consider its existence to be a forward declaration
          // and take its ownership
         #if DIAG_OWNER>=200
          LogF
            (
            "Note: '%s' taking ownership of '%s' from '%s'",
            (const char *)srcEntry.GetOwnerDebugName(),
            (const char *)dstEntry->GetContext(),
            (const char *)dstEntry->GetOwnerDebugName()
            );
        #endif
          dstEntry->SetSource(srcEntry);
        }
        else if (srcEntry.IsDefined() && srcEntry.GetEntryCount()>0)
        {
          // class already exists and is being modified by different addon
          // warn if multiple owners share one class
          // this can cause delay loading problems
          originalEntryCount = dstEntry->GetEntryCount();
          /*
          LogF
          (
          "Warning: class '%s' shared between '%s' and '%s'",
          (const char *)dstEntry->GetContext(),
          (const char *)srcEntry.GetOwnerDebugName(),
          (const char *)dstEntry->GetOwnerDebugName()
          );
          */
        }

        const ParamClass *srcCls = srcEntry.GetClassInterface();
        ParamClass *dstCls = dstEntry->GetClassInterface();
        dstCls->_parent = ParamClassLockedPtr(this);
        // check if all entries were overloaded
        if (srcEntry.IsDefined())
        {
          #if _ENABLE_REPORT
          RString oldOwner = ( originalEntryCount>0 && dstCls->GetSource() ) ? dstCls->GetOwnerDebugName() : "";
          #endif
          bool deleteEntryInside = false;
          bool allEntriesOverloaded = dstCls->Update(*srcCls,deleteEntryInside,protect,isNewEntry);
          containsDeleteEntry = containsDeleteEntry || deleteEntryInside;
          if (allEntriesOverloaded)
          {
            countEntriesOverloaded++;
            if (srcEntry.GetEntryCount()>0 && originalEntryCount>0)
            {
              #if DIAG_OWNER>=200
              LogF
                (
                "Note: class '%s' ownership overloaded by '%s' (was '%s')",
                (const char *)dstEntry->GetContext(),
                (const char *)srcEntry.GetOwnerDebugName(),
                (const char *)oldOwner
                );
              #endif
              if (!deleteEntryInside) dstEntry->SetSource(srcEntry);
            }
          }
          else
          {
            // merging two non-empty classes
            if (srcEntry.GetEntryCount()>0 && originalEntryCount>0)
            {
              // change ownership to no-unload if necessary 
              if (dstEntry->GetSource() && dstEntry->GetSource()->CanUnload())
              {
                #if DIAG_OWNER>=100
                LogF
                  (
                  "Warning: class '%s' shared between '%s' and '%s'",
                  (const char *)dstEntry->GetContext(),
                  (const char *)srcEntry.GetOwnerDebugName(),
                  (const char *)oldOwner
                  );
                #endif
                IParamClassSource *noUnloadSrc = new ParamClassOwnerIndirectNoUnload(dstEntry->GetSource());
                dstEntry->SetSource(noUnloadSrc,false);
              }
              else
              {
              #if DIAG_OWNER>=300
                LogF
                  (
                  "Note: class '%s' shared once more by '%s' (owner '%s')",
                  (const char *)dstEntry->GetContext(),
                  (const char *)srcEntry.GetOwnerDebugName(),
                  (const char *)oldOwner
                  );
              #endif
              }
            }
          }
        }
      }
    }
    else if (srcEntry.IsArray())
    {
      if (!dstEntry)
      {
        ParamArray *nEntry = new ParamArray(srcEntry.GetName());
        NewEntry(nEntry,true);
        dstEntry = ParamEntryPtr(this,nEntry);
      }
      else if (!dstEntry->IsArray())
      {
        RptF(
          "%s: Cannot update non array from array",
          cc_cast(source.GetContext(dstEntry->GetName()))
        );
        return false;
      }
      else if (access>=PAReadAndCreate)
      {
        if (dstEntry.NotNull())
          AccessDenied("**Update** entry" + dstEntry->GetName());
        else
          AccessDenied("**Update**");
        return false;
      }

      const ParamArray *srcArr = static_cast<const ParamArray *>(&srcEntry);
      ParamArray *dstArr = static_cast<ParamArray *>(dstEntry.GetPointer());
      dstArr->Copy(*srcArr);
      countEntriesOverloaded++;
    }
    else if (access>=PAReadAndCreate)
    {
      if (dstEntry.NotNull())
        AccessDenied("**Update** entry" + dstEntry->GetName());
      else
        AccessDenied("**Update**");
    }
    else
    {
      // FIX: allow to update entry of different type
      if (dstEntry)
      {
        if (access >= PAReadAndCreate)
        {
          if (dstEntry.NotNull())
            AccessDenied("**Update** entry" + dstEntry->GetName());
          else
            AccessDenied("**Update**");

          return false;
        }
        Delete(srcEntry.GetName());
      }
      ParamEntry *entry = srcEntry.Clone();
      if (entry)
      {
        NewEntry(entry, true);
        // dstEntry = ParamEntryPtr(this, entry);
      }

      countEntriesOverloaded++;
    }
  }
#ifdef DEBUG_UI_RSC
  if (_parent && !stricmp(cc_cast(GetName()),"Controls") && !stricmp(cc_cast(_parent->GetName()),"RscDisplayNotFreeze"))
  {
    RptF("class: %s updated, _entries = %d", cc_cast(GetName()), _entries.Size());
  }
#endif
  // new entries might be added during update - compact the array
  _entries.Compact();
#ifdef DEBUG_UI_RSC
  if (_parent && !stricmp(cc_cast(GetName()),"Controls") && !stricmp(cc_cast(_parent->GetName()),"RscDisplayNotFreeze"))
  {
    for (int i=0; i<_entries.Size(); i++)
      RptF(" ... entry: %s", cc_cast(_entries[i]->GetName()));
  }
#endif
  bool allOverloaded = countEntriesOverloaded==GetEntryCount() && baseOverloadedThis;
  return allOverloaded;
}

void ParamClass::CalculateCheckValue(SumCalculator &sum) const
{
  #if LOG_CHECKSUM
    LogF("** Calculate CRC of '%s'",(const char *)GetName());
  #endif
  // recursive get crc
  ParamAccessMode mode = GetAccessMode();
  for (int i=0; i<_entries.Size(); i++)
  {
    unconst_cast(this)->LoadEntry(i);
    const ParamEntry *entry = _entries[i];
    // scan all classes
    // other entries scan depending on class access mode
    if (mode>=PAReadOnlyVerified || entry->IsClass())
    {
      entry->CalculateCheckValue(sum);
    #if LOG_CHECKSUM
      LogF
      (
        "CRC after %s = %x",(const char *)entry->GetName(),
        sum.GetResult()
      );
    #endif
    }
  }
  // in 1.50 fix CRC calculation - class name should be calculated only once
  if (mode>=PAReadOnlyVerified)
  {
    sum.Add(GetName(),GetName().GetLength());
    #if LOG_CHECKSUM
      LogF("add CRC of '%s'",(const char *)GetName());
    #endif
  }
  if (_base && mode>=PAReadOnlyVerified)
  {
    sum.Add(_base->GetName(),_base->GetName().GetLength());
    #if LOG_CHECKSUM
      LogF("add CRC of base '%s'",(const char *)_base->GetName());
    #endif
  }
}

bool ParamClass::HasChecksum() const
{
  // recursive get crc
  ParamAccessMode mode = GetAccessMode();
  if (mode>=PAReadOnlyVerified) return true;
  for (int i=0; i<_entries.Size(); i++)
  {
    unconst_cast(this)->LoadEntry(i);
    const ParamEntry *entry = _entries[i];
    // scan all classes and check if some of them is verified
    if (entry->IsClass())
    {
      bool ret = entry->HasChecksum();
      if (ret) return true;
    }
  }
  return false;
}

/*!
Count classes that may be checked using CalculateCheckValue 
*/
int ParamClass::GetNumberOfClassesForChecking() const
{
  int count = 0;
  for (int i=0; i<_entries.Size(); i++)
  {
    unconst_cast(this)->LoadEntry(i);
    const ParamEntry *entry = _entries[i];
    // scan all classes
    // other entries scan depending on class access mode
    if (!entry->IsClass()) continue;
    const ParamClass *cEntry = entry->GetClassInterface();
    count += cEntry->GetNumberOfClassesForChecking();
  }
  // check if this class is suitable for checking
  if (GetAccessMode()>=PAReadOnlyVerified || count>0)
  {
    count++;
  }
  return count;
}

/*!
Select class that may be checked using CalculateCheckValue 
index should be between 0 and GetNumberOfClassesForChecking
*/
const ParamClass *ParamClass::SelectClassForChecking(int index) const
{
  if (index<0) return 0;
  Assert (index<GetNumberOfClassesForChecking());
  for (int i=0; i<_entries.Size(); i++)
  {
    unconst_cast(this)->LoadEntry(i);
    const ParamEntry *entry = _entries[i];
    // scan all classes
    // other entries scan depending on class access mode
    if (!entry->IsClass()) continue;
    const ParamClass *cEntry = entry->GetClassInterface();
    if (!cEntry) continue;
    int nCheckInCEntry = cEntry->GetNumberOfClassesForChecking();
    if (index<nCheckInCEntry)
    {
      const ParamClass *select = cEntry->SelectClassForChecking(index);
      if (select) return select;
    }
    index -= nCheckInCEntry;
  }
  if (index==0) return this;
  return NULL;
}

void ParamClass::Diagnostics( int indent )
{
}

DEFINE_FAST_ALLOCATOR(ParamFile)

ParamFile::ParamFile()
{
}

ParamFile::~ParamFile()
{
  //LogF("Destruct paramfile %s",(const char *)GetName());
  Clear();
}

#define DIAG_OPEN 0

#if DIAG_OPEN
static int ParamFileOpen=0;
#endif


void ParamFileContext::TransferIndex(SerializeBinStream &f, int &a, int verEncode)
{
  if (_version>=verEncode)
  {
    // index encoded
    // we expect for most cfg files 2B should be enough
    TransferInt(f,a);
  }
  else
  {
    // plain string index
    f.Transfer(a);
  }
}

void ParamFileContext::TransferStringTable(SerializeBinStream &f)
{
  // no central table before version 5
  if (_version<=5 || _version>=8) return;
  if (f.IsSaving())
  {
    f.SaveInt(_strings.Size());
  }
  else
  {
    int size = f.LoadInt();
    _strings.Realloc(size);
    _strings.Resize(size);
  }
  for (int i=0; i<_strings.Size(); i++)
  {
    f.Transfer(_strings[i]);
  }
}

void ParamFileContext::TransferString(SerializeBinStream &f, RStringB &string)
{
  if (_version>=8)
  {
    f.Transfer(string);
    return;
  }
  
  if (f.IsSaving())
  {
    f.Transfer(string);
    /*
    f.Transfer(stringB);
    // check if name is already in table
    RStringB stringB = string;
    int index = _strings.Find(stringB);
    if (index>=0)
    {
      // already there - transfer only index
      TransferIndex(f,index);
    }
    else
    {
      // transfer new index and string defition
      RStringB stringB = string;
      index = _strings.Add(stringB);
      TransferIndex(f,index);
      // all strings are saved together since version 5
      Assert(_version>=5);
      //f.Transfer(stringB);
    }
    */
  }
  else
  {
    // transfer index
    int index = -1;
    TransferIndex(f,index);
    if (index<0 || index>_strings.Size())
    {
      Fail("FileStructure error");
      f.SetError(f.EFileStructure);
      return;
    }
    if (index<_strings.Size())
    {
      // old string - use it
      string = _strings[index];
    }
    else
    {
      // new string - define and use it
      if (_version<5)
      {
        // all strings should be already defined
        RStringB stringB;
        f.Transfer(stringB);
        Assert (index==_strings.Size());
        _strings.Access(index);
        _strings[index] = stringB;
        string = stringB;
      }
      else
      {
        Fail("FileStructure error");
        f.SetError(f.EFileStructure);
        return;
      }
    }
  }
}

void ParamFileContext::TransferInt(SerializeBinStream &f, int &a)
{
  // encoded integer (dynamic byte length)
  // TODO: terminator based on signed format (MSB propagated)?
  // use dynamic length int format
  if (f.IsLoading())
  {
    unsigned int val = 0;
    int offset = 0;
    while (f.GetError()==f.EOK)
    {
      unsigned char c = f.LoadChar();
      // transfer 7 bits ber byte
      val |= (c&0x7f)<<offset;
      // check terminator
      if ((c&0x80)==0)
      {
        // extend MSB?
        break;
      }
      offset += 7;
    }
    a = val;
  }
  else 
  {
    unsigned int val = a;
    for(;;)
    {
      unsigned char c = val&0x7f;
      val >>= 7;
      // check MSB?
      if (val)
      {
        f.SaveChar(c|0x80);
      }
      else
      {
        // no more bits left
        f.SaveChar(c);
        break;
      }
    }
  }
}

static void IntentionalCrash()
{
  // cause access violation
  *(volatile int *)NULL;
}


void ParamFile::Clear()
{
  #if DIAG_OPEN
  if( _entries.Size()>0 )
  {
    LogF("%d: Clear paramfile %s",ParamFileOpen,(const char *)GetName());
    --ParamFileOpen;
  }
  #endif
  //Assert(CheckIntegrity());
  UnlinkClasses();
  Assert( !_parent );
  Assert( !_base );
  _entries.Clear();
  _name=RStringBEmpty;

  _evaluator = NULL;
  RemoveFromTheCache();
  _source.Free();
}

#define OUTPUT_PREPROC  0

#if OUTPUT_PREPROC
#include <Es/Common/win.h>
#endif

LSError ParamFile::ParsePlainText(const char *name, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  SetName(name);
  if (!QFBankQueryFunctions::FileExists(name)) return LSOK;

//  Preprocessor preprocessor;
  QOStrStream out;
  if (!Preprocess(out, name))
//  if (!preprocessor.Process(&out, name))
  {
//    ErrorMessage("Preprocessor failed on file %s - error %d.", name, preprocessor.error);
    return LSStructure;
  }

  QIStrStream *stream = new QIStrStream;
  stream->init(out.str(), out.pcount());
  QIPreprocessedStream in(stream);
  ParsePlainText(in, parentVariables, globalVariables);
  #if DIAG_OPEN
  if( _entries.Size()>0 )
  {
    ParamFileOpen++;
    LogF("%d: Parsed paramfile %s",ParamFileOpen,name);
  }
  #endif
  return in.fail() ? in.error() : LSOK;
}

void ParamFile::ParsePlainText(QIStream &in, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  // TODO: change locality of variables defined by enum and EXEC / EVAL
  // TODO: evaluator should load / save global variables, not local ones
  CreateEvaluator(parentVariables, globalVariables, true);

  // parse section content
  int number = 1;
  for(;;)
  {
    char buf[4096];
    in.readLine(buf,sizeof(buf));
    if (buf[0]!=0 && buf[0]!='#')
    {
    BString<256> nameBuf;
    // entry name is autogenerated
    sprintf(nameBuf,"Line%d",number++);
    RStringB valueName = (const char *)nameBuf;
    ParamEntry *value=NULL;
      // trim trailing and leading spaces
      char *bufEnd = strchr(buf,0);
      while (bufEnd>buf && isspace(bufEnd[-1])) *--bufEnd=0;
      const char *name = buf;
      while (isspace(*name)) name++;
      value = new ParamValue(valueName);
      value->SetValue(name);

    SRef<ParamEntry> newEntry = value;
    // check for overload
    if (newEntry)
    {
      int baseIndex=FindIndex(newEntry->GetName());
      if( baseIndex<0 )
      {
        _entries.Add(newEntry);
      }
      else
      {
        ParsingError(in, "%s: Member already defined.",(const char *)GetContext(newEntry->GetName()));
      }
    }
  }
    if( in.eof() || in.fail() )
    {
      break;
    }
  }

  _entries.Compact();
  // class parsed

  SetSource(new ParamClassOwnerNoUnload("",""));
}

LSError ParamFile::Parse(const char *name, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  LSError lsres;
  SetName(name);
  if (!QFBankQueryFunctions::FileExists(name)) return LSOK;

//  Preprocessor preprocessor;
  QOStrStream out;
//  if (!preprocessor.Process(&out, name))
  if (!Preprocess(out, name))
  {
//  ErrorMessage("Preprocessor failed on file %s - error %d.", name, preprocessor.error);
    return LSStructure;
  }

  QIStrStream *stream = new QIStrStream;
  stream->init(out);
  QIPreprocessedStream in(stream);
  lsres = Parse(in, source, parentVariables, globalVariables);

  // FIX: Clear is called in Parse
  SetName(name);
  // default ownership is not unloadable
  //SetSource(new ParamClassOwnerNoUnload("",name));

  // top-level should be used only as a parent, never for inheritance
  // as a parent it can be used once by each entry (in _parent)
  // therefore lock (load) count should not exceeed entry count
  Assert(_loadCount<=_entries.Size());
  //Assert(CheckIntegrity());
  #if DIAG_OPEN
  if( _entries.Size()>0 )
  {
    ParamFileOpen++;
    LogF("%d: Parsed paramfile %s",ParamFileOpen,name);
  }
  #endif
  return in.fail() ? in.error() : lsres;
}

LSError ParamFile::Parse(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  LSError lsres=LSOK;
  SetName(name);
  if (!bank.FileExists(name)) return LSOK;

  QOStrStream out;
  if (!Preprocess(out, bank, name))
  {
    //  ErrorMessage("Preprocessor failed on file %s - error %d.", name, preprocessor.error);
    return LSStructure;
  }

  QIStrStream *stream = new QIStrStream;
  stream->init(out);
  QIPreprocessedStream in(stream);
  lsres = Parse(in, new ParamClassOwnerNoUnload("",name), parentVariables, globalVariables);

  // FIX: Clear is called in Parse
  SetName(name);

  return in.fail() ? in.error() : lsres;
}

void ParamFile::SetSource(IParamClassSource *source)
{
  // if given source can serve to unloading, register ParamFile with the global unloader
  RegisterUnloading(source);
  ParamClass::SetSource(source,true);
}

bool ParamClass::CanBeUnloaded() const
{
  if (!NeedsRestoring()) return false;
  DoAssert(_sourceOffset>=0);
  return CountLinksToClass(*this) == _loadCount;
}

ParamClass *ParamClass::GetClassForUnloading()
{
  bool canUnload = CanBeUnloaded();
  
  if (canUnload)
  {
    return this;
  }
  for (int i=0; i<_entries.Size(); i++)
  {
    ParamEntry *entry = _entries[i];
    if (!entry->IsClass()) continue;
    if (entry->GetPlaceholderInterface()) continue;
    ParamClass *entryC = entry->GetClassInterface();
    if (!entryC)
    {
      Fail("Cannot get class interface");
      continue;
    }
    ParamClass *cls = entryC->GetClassForUnloading();
    if (cls)
    {
      return cls;
    }
  }
  return NULL;
}

void ParamFile::CacheRefresh(ParamClass *cls)
{
  _cache.Delete(cls);
  _cache.Insert(cls);
}

/**
While class is not loaded, it can be released at any time.
*/

void ParamFile::CacheInsert(ParamClass *cls)
{
  _cache.Insert(cls);
}

/**
While class is loaded, it can not be released and should not therefore be treaded as cached.
*/
void ParamFile::CacheRemove(ParamClass *cls)
{
  _cache.Delete(cls);
}

const int ParamEntrySize = 200;

size_t ParamFile::FreeOneItem()
{
  ParamClass *cls = _cache.Last();
  if (cls)
  {
    int oldCount = _cache.Size();
    const ParamClass *parent = cls->GetParent();
    int index = parent->_entries.FindKey(cls);
    if (index>=0)
    {
      // unload corresponding entry
#ifdef DEBUG_UI_RSC
      RptF("... FreeOneItem - %s/%s", cc_cast(parent->GetName()), cc_cast(cls->GetName()));
#endif
      unconst_cast(parent)->UnloadEntry(index);
      size_t unloaded = (oldCount-_cache.Size())*ParamEntrySize;
      return unloaded>0 ? unloaded : 1;
    }
    else
    {
      Fail("Class not found");
      // unload all entries within the class
      cls->ReleaseAll();
      return 1;
    }
    
  }
  return 0;
}
float ParamFile::Priority() const
{
  return 0.2;
}

size_t ParamFile::MemoryControlled() const
{
  #if _DEBUG
  size_t total = GetMemoryUsed();
  size_t perClass = _cache.Size()> 0 ? total/_cache.Size() : 0;
  (void)perClass;
  #endif
  // estimate saving based on class count - this is based on perClass experimental results
  size_t size = _cache.Size()*ParamEntrySize;
  return size;
}
RString ParamFile::GetDebugName() const
{
  return GetFilenameExt(_name);
}


void Indent( QOStream &f, int indent )
{
  while( --indent>=0 ) f<<"\t";
}

inline bool IsNumerical(const char *strValue, float *ret=NULL )
{
  char *endptr = NULL;
  float fValue = strtod(strValue, &endptr);
  if( ret ) *ret=fValue;
  return (*strValue != 0 && *endptr == 0);
}

/*!
\patch_internal 1.43 Date 1/29/2002 by Jirka
- Fixed: Disable interpretation of strings containing number as number in ParamFile
*/

void ParamRawValue::Save(QOStream &f, int indent, const char *eol) const
{
  const char *strValue = _value;
  f << "\"";
  while (*strValue)
  {
    if (*strValue == '"') f << "\"\"";
    else if (*strValue == '\r') {}
    else if (*strValue == '\n') f << "\" \\n \"";
    else f.put(*strValue);
    strValue++;
  }
  f << "\"";
}

void ParamRawValueExpression::Save(QOStream &f, int indent, const char *eol) const
{
  const char *strValue = _value;
  f << "@\"";
  while (*strValue)
  {
    if (*strValue == '"') f << "\"\"";
    else if (*strValue == '\r') {}
    else if (*strValue == '\n') f << "\" \\n \"";
    else f.put(*strValue);
    strValue++;
  }
  f << "\"";
}

void ParamRawValue::SerializeBin(SerializeBinStream &f)
{
  // string are very likely to be class names
  // and there is high probability they can be reused
  ParamFileContext *context = (ParamFileContext *)f.GetContext();
  context->TransferString(f,_value);
  //f.Transfer(_value);
}

void ParamRawValueFloat::Save(QOStream &f, int indent, const char *eol) const
{
  BString<256> buffer;
  sprintf(buffer, "%.8g", _value);
  f << buffer;
}

RStringB ParamRawValueFloat::GetValue() const
{
  BString<256> buf;
  sprintf(buf, "%g", _value);
  return (const char *)buf;
}

void ParamRawValueFloat::SerializeBin(SerializeBinStream &f)
{
  f.Transfer(_value);
}

void ParamRawValueFloat::CalculateCheckValue(SumCalculator &sum) const
{
  sum.Add(&_value,sizeof(_value));
#if LOG_CHECKSUM >= 2
  LogF("  add CRC of %g, result %x", _value, sum.GetResult());
#endif
}

void ParamRawValueInt64::SerializeBin(SerializeBinStream &f)
{
  // before version 3 integers were not encoded
  f.Transfer(_value);
}

void ParamRawValueInt64::CalculateCheckValue(SumCalculator &sum) const
{
  sum.Add(&_value,sizeof(_value));
#if LOG_CHECKSUM >= 2
  LogF("  add CRC of %d, result %x", _value, sum.GetResult());
#endif
}

void ParamRawValueInt64::Save(QOStream &f, int indent, const char *eol) const
{
  BString<256> buffer;
  sprintf(buffer, "%I64d", _value);
  f << buffer;
}

RStringB ParamRawValueInt64::GetValue() const
{
  BString<256> buf;
  sprintf(buf,"%I64d",_value);
  return (const char *)buf;
}
//#endif

void ParamRawValueInt::SerializeBin(SerializeBinStream &f)
{
  // before version 3 integers were not encoded
  f.Transfer(_value);
}

void ParamRawValueInt::CalculateCheckValue(SumCalculator &sum) const
{
  sum.Add(&_value,sizeof(_value));
#if LOG_CHECKSUM >= 2
  LogF("  add CRC of %d, result %x", _value, sum.GetResult());
#endif
}

void ParamRawValueInt::Save(QOStream &f, int indent, const char *eol) const
{
  BString<256> buffer;
  sprintf(buffer, "%d", _value);
  f << buffer;
}

RStringB ParamRawValueInt::GetValue() const
{
  BString<256> buf;
  sprintf(buf,"%d",_value);
  return (const char *)buf;
}


LSError ParamRawArray::Parse(QIStream &in, ParamFile *file)
{
  bool isFirst=true;
  int c=in.get();
  while( isspace(c) ) c=in.get();
  if( c!='{' )
  {
    //ErrorMessage("Config: %s: '%c' encountered instead of '{'",(const char *)GetContext(GetName()),c);
    ParsingError(in, "Config: '%c' encountered instead of '{'",c);
    return LSStructure;
  }
  for(;;)
  {
    // check for sub-array
    c=in.get();
    while( isspace(c) ) c=in.get();
    in.unget();
    if (c=='{')
    {
      // sub-array
      Ref<ParamArrayValueArray> sub = new ParamArrayValueArray();
      IParamArrayValue *val = sub;
      LSError lsres=sub->Parse(in,file);
      if (lsres!=LSOK) return lsres;
      _value.Add(val);
      c=in.get();
    }
    else if (c == '#')
    {
      in.get(); // # will be no more needed 
      if (!ProcessLineNumber(in))
      {
        ParsingError(in, "Config: Wrong # directive");
        return LSStructure;
      }
      continue;
    }
    else
    {
      WordBuf word;
      // check for expression
      c=in.get();
      while( isspace(c) ) c=in.get();
      if (c=='@')
      {
        // get expression - one word
        bool someWord = GetWord(word,in,",;}");
        c=in.get();
        // c may be one of ,;} or \n or \r
        if( c==',' || c==';' || someWord )
        {
          if (c == ';')
          {
            ParsingWarning(in, "Config: ';' used as a separator in the array");
          }

          AddExpression(word);
        }
      }
      else
      {
        in.unget();
        bool quoted = false;
        bool someWord = GetWord(word,in,",;}",&quoted);
        c=in.get();
        // c may be one of ,;} or \n or \r
        if( c==',' || c==';' || someWord )
        {
          if (c == ';')
          {
            ParsingWarning(in, "Config: ';' used as a separator in the array");
          }

          if (!quoted)
          {
            if (strncmp(word, "__EVAL", 6) == 0)
            {
              strcpy(word, cc_cast(file->EvaluateStringInternal(word + 6)));
            }
            // autodetect value type
            bool ok = false;
            int val = ScanInt(word,ok);
            if (ok) AddValue(val);
            else
            {
              float val = ScanFloat(word,ok);
              if (ok) AddValue(val);
              else AddValue(word);
            }
          }
          else
          {
            AddValue(word);
          }
          isFirst=false;
        }
      }
    } // if subarray else 
    if( c<0 )
    {
      //ErrorMessage("%s: EOF encountered.",(const char *)GetContext(GetName()));
      ParsingError(in, "EOF encountered.");
      return LSStructure;
    }
    while( isspace(c) ) c=in.get();
    if( c=='}' ) break; // array terminated
    // , or ; should follow
    if( c!=',' && c!=';' )
    {
      //ErrorMessage("Config: %s: '%c' encountered instead of ','",(const char *)GetContext(word),c);
      ParsingError(in, "Config: '%c' encountered instead of ','",c);
      return LSStructure;
    }
  }
  c = in.get();
  while( isspace(c) ) c=in.get();
  in.unget();
  Compact();
  return LSOK;
}

void ParamRawArray::Save( QOStream &f, int indent, const char *eol ) const
{
  // check if some item is string
  // (item may be numerical or string)
  bool someString = false;
  for( int i=0; i<_value.Size(); i++ )
  {
    
    if
    (
      dynamic_cast<ParamArrayValueArray *>(_value[i].GetRef()) ||
      !IsNumerical(_value[i]->GetValue())
    )
    {
      someString = true;
      break;
    }

  }
  if( someString )
  {
    // array of string values (at least one string)
    f << eol;
    Indent(f,indent);
    f << "{" << eol;
    for( int i=0; i<_value.Size(); i++ )
    {
      Indent(f,indent+1);
      _value[i]->Save(f,indent,eol);
      if( i<_value.Size()-1 ) f << ",";
      f << eol;
    }
    Indent(f,indent);
    f << "}";
  }
  else
  {
    // array of numeric values
    f << "{";
    for( int i=0; i<_value.Size(); i++ )
    {
      _value[i]->Save(f,indent,eol);
      if( i<_value.Size()-1 ) f << ",";
    }
    f << "}";
  }
}


void ParamRawArray::SerializeBin(SerializeBinStream &f)
{
  ParamFileContext *context = (ParamFileContext *)f.GetContext();
  // make table of names
  if (f.IsSaving())
  {
    int n = _value.Size();
    context->TransferIndex(f,n,4);
    for (int i=0; i<n; i++)
      _value[i]->SerializeBin(f);
  }
  else
  {
    Assert(f.IsLoading());
    int n = 0;
    context->TransferIndex(f,n,4);
    _value.Realloc(n);
    _value.Resize(n);
    for (int i=0; i<n; i++)
    {
      // CreateValue - create value of appropriate type
      _value[i] = CreateParamArrayValue(f);
      _value[i]->SerializeBin(f);
    }
  }
}

LSError ParamArray::Parse(QIStream &in, ParamFile *file)
{
  return ParamRawArray::Parse(in,file);
}
void ParamArray::Save(QOStream &f, int indent, const char *eol) const
{
  Indent(f,indent);
  f << _name << "[]=";
  ParamRawArray::Save(f,indent,eol);
  f << ";" << eol;
}
void ParamArray::SerializeBin(SerializeBinStream &f)
{
  ParamFileContext *context = (ParamFileContext *)f.GetContext();
  // make table of names
  context->TransferString(f,_name);
  ParamRawArray::SerializeBin(f);
}

void ParamRawArray::CalculateCheckValue(SumCalculator &sum) const
{
  for (int i=0; i<_value.Size(); i++)
  {
    _value[i]->CalculateCheckValue(sum);
  }
}

void ParamArray::CalculateCheckValue(SumCalculator &sum) const
{
  ParamRawArray::CalculateCheckValue(sum);
}


template <class ParamRawValueSpec>
void ParamValueSpec<ParamRawValueSpec>::Save( QOStream &f, int indent, const char *eol ) const
{
  Indent(f,indent);
  f << _name << "=";
  ParamRawValueSpec::Save(f,indent,eol);
  f << ";" << eol;
}

template <class ParamRawValueSpec>
void ParamValueSpec<ParamRawValueSpec>::SerializeBin(SerializeBinStream &f)
{
  if (f.IsSaving())
  {
    char type = ParamRawValueSpec::GetValueType();
    f.Transfer(type);
  }
  // Loading - type processed by CreateParamValue
  
  ParamFileContext *context = (ParamFileContext *)f.GetContext();
  // make table of names
  context->TransferString(f,_name);
  ParamRawValueSpec::SerializeBin(f);
}

template <class ParamRawValueSpec>
void ParamValueSpec<ParamRawValueSpec>::CalculateCheckValue(SumCalculator &sum) const
{
  ParamRawValueSpec::CalculateCheckValue(sum);
}


void ParamClass::Save( QOStream &f, int indent, const char *eol) const
{
  Indent(f,indent);
  f << "class " << _name;
  // TODO: save parents (base class name)
  if (_base)
  {
    f << ": " << _base->GetName();
  }
#if _VBS3 // saveConfig can save base class
  else if (_baseClass.GetLength() > 0)
  {
    f << ": " << _baseClass;
  }
  if (GetEntryCount() == 0 && _noParenthesisIfEmpty)
  {
    f << ";" << eol;
    return;
  }
#endif
  f << eol;
  Indent(f,indent);
  f << "{" << eol;
  for( int i=0; i<GetEntryCount(); i++ )
  {
    unconst_cast(this)->LoadEntry(i);
    GetEntry(i).Save(f,indent+1,eol);
  }
  Indent(f,indent);
  f << "};" << eol;
}

void ParamClassDecl::Save(QOStream &f, int indent, const char *eol) const
{
  Indent(f,indent);
  f << "class " << _name << ";" << eol;
}

void ParamClassDelete::Save(QOStream &f, int indent, const char *eol) const
{
  Indent(f, indent);
  f << "delete " << _name << ";" << eol;
}

void ParamEntry::Compact()
{
}

void ParamEntry::ReserveArrayElements(int count)
{
}

/*!
This function is usefull when you know in advance how many
entries will given class contain.
With large classes you can avoid reallocation during class growing.
Even when using Compact after adding all entries,
if the number provided is accurate, you can avoid resizing during Compact.
*/

void ParamEntry::ReserveEntries(int count)
{
}



bool ParamEntry::HasChecksum() const
{
  // by default no entry has checksum
  // only class may be checksumed
  return false;
}

void ParamClass::CheckInheritedAccess()
{
  if (_access==PADefault)
  {
    // no access given - inherit it
    // prefer base class
    if(_base && _base->GetAccessMode()>PADefault)
    {
      // base must be closed - no need to check its parent
      _access = _base->GetAccessMode();
    }
    else if (_parent)
    {
      ParamClass *parent = _parent;
      do
      {
        if (parent->GetAccessMode()>PADefault)
        {
          _access = parent->GetAccessMode();
          break;
        }
        // before checking parent of parent check base of parent
        if (parent->_base && parent->_base->GetAccessMode()>PADefault)
        {
          _access = parent->_base->GetAccessMode();
          break;
        }
        parent = parent->_parent;
      }
      while (parent);
    }
  }
}

void ParamClass::SetBase(RStringB base)
{
 if (base.GetLength() > 0)
  {
    Assert(_parent);
    // search parents and bases of my parent
    ParamEntryPtr entry;
    // if the base class is the same class as we, it must be defined before us
    int baseIndex = _parent->FindIndex(base);
    // check our index
    int thisIndex = _parent->FindIndex(GetName());
    if (baseIndex>=0 && (baseIndex<thisIndex || thisIndex<0))
    {
      _parent->LoadEntry(baseIndex);
      // base found and possible (defined before us)
      entry = ParamEntryPtr(_parent,_parent->_entries[baseIndex]);
    }
    else
    {
      // try searching for a base in base class and parent class
      if (_parent->_base)
      {
        entry = _parent->_base->Find(base, true, true, DefaultAccess);
      }
      if (!entry && _parent->_parent)
      {
        entry = _parent->_parent->Find(base, true, true, DefaultAccess);
      }
    }
    if (entry)
    {
      _base = ParamClassLockedPtr(entry->GetClassInterface());
      Assert((ParamClass *)_base!=this); //FIXED BY BREDY error C2593: 'operator !=' is ambiguous
                                           //Assert(_base!=this); 
    }
    else
    {
      RptF("Cannot find base %s for %s",cc_cast(base),cc_cast(GetContext()));
      _base = ParamClassLockedPtr(NULL);
    }
  }
  else
  {
    _base = ParamClassLockedPtr(NULL);
  }
}

/*!
\patch_internal 1.24 Date 09/26/2001 by Ondra
- Optimized: better memory usage for binary config files.
*/


void ParamClass::MoveEntryToEnd(int entryIndex)
{
  Assert(entryIndex>=0 && entryIndex<_entries.Size());
  SRef<ParamEntry> entry=_entries[entryIndex];
  _entries.DeleteAt(entryIndex);
  ParamEntry *ptr=entry;
  _entries.Append()=entry;

  for (int i=0,cnt=_entries.Size()-1;i<cnt;++i)
  {
      LoadEntry(i);
      ParamEntryPtr entry=ParamEntryPtr(this,_entries[i]);
      if (entry->IsClass() && entry->IsDefined())
      {
        if (entry->GetClassInterface()->FindBase().GetPointer()==ptr)
        {
          MoveEntryToEnd(i);
          --i;
          while (cnt>i && _entries[cnt]!=ptr) cnt--;
        }        
      }
  }
}

bool ParamClass::SetBaseEx(RStringB base, bool nomoveClass)
{
if (base.GetLength() > 0)
  {
    Assert(_parent);
    // search parents and bases of my parent
    ParamEntryPtr entry;
    // if the base class is the same class as we, it must be defined before us
    int baseIndex = _parent->FindIndex(base);
    // check our index
    int thisIndex = _parent->FindIndex(GetName());
    if (thisIndex==baseIndex || baseIndex<0 || thisIndex<0 || (baseIndex>thisIndex && nomoveClass)) 
    {
      // try searching for a base in base class and parent class
      if (_parent->_base)
      {
        entry = _parent->_base->Find(base, true, true, DefaultAccess);
      }
      if (!entry && _parent->_parent)
      {
        entry = _parent->_parent->Find(base, true, true, DefaultAccess);      
      }
      if (!(!entry))
      {
        ParamClassPtr basenode=entry->GetClassInterface();
        ParamClassPtr parent=_parent.GetPointer();
        ParamClassPtr node=this;                        
zkustoznova:
        while (!(!parent) && parent!=basenode->GetParent()) 
        {
          node=parent;
          parent=parent->GetParent();
        }
        if (!parent) 
        {
          ParamEntryPtr e=basenode->GetParent();
          if (!(!e))
          {
            basenode=e->GetClassInterface();
            parent=_parent;
            node=this;
            goto zkustoznova;
          }
          else
            entry=0;
        }
        else
        {
          thisIndex=parent->FindIndex(node->GetName());
          baseIndex=parent->FindIndex(basenode->GetName());
          if (thisIndex<baseIndex)
          {
            parent->MoveEntryToEnd(thisIndex);
            return SetBaseEx(base,true);
          }
        }
      }
    }
    else
    {
      if (thisIndex<baseIndex) {_parent->MoveEntryToEnd(thisIndex);return SetBaseEx(base,true);}
      _parent->LoadEntry(baseIndex);
      // base found and possible (defined before us)
      entry = ParamEntryPtr(_parent,_parent->_entries[baseIndex]);
    }
    if (entry)
    {
      _base = ParamClassLockedPtr(entry->GetClassInterface());
      return true;
    }
    else
    {
      _base = ParamClassLockedPtr(NULL);
      return false;
    }
  }
  else
  {
    _base = ParamClassLockedPtr(NULL);
    return true;
  }
}

void ParamClass::SerializeBin(SerializeBinStream &f)
{
  enum
  {
    idClass,
    idValue,
    idArray,
    idClassDecl,
    idClassDelete,
  };

  ParamFileContext *context = (ParamFileContext *)f.GetContext();
  // make table of names
  if (context->_version<7)
  {
    context->TransferString(f,_name);
  }
  
  if (f.IsSaving())
  {
    // class is transferred in two passes
    // in first pass only a fixup is saved, with a pointer to the real content
    RStringB base = "";
    if (_base) base = _base->GetName();
    if (context->_version>=6)
    {
      context->TransferString(f,base);
    }
    else
    {
      f.Transfer(base);
    }
    int n = _entries.Size();
    context->TransferIndex(f,n,4);
    AUTO_STATIC_ARRAY(int,classStartFilePos,64);
    classStartFilePos.Resize(n);
    // save entries and class headers
    for (int i=0; i<n; i++)
    {
      ParamEntry *entry = _entries[i];
      if (entry->IsClass())
      {
        if (entry->IsDelete())
        {
          f.SaveChar(idClassDelete);
          RStringB name = entry->GetName();
          context->TransferString(f, name);
        }
        else if (entry->IsDefined())
        {
          f.SaveChar(idClass);
          RStringB name = entry->GetName();
          context->TransferString(f,name);
          // reserve a place for the pointer to the real content
          classStartFilePos[i] = f.CreateFixUp();
        }
        else
        {
          f.SaveChar(idClassDecl);
          RStringB name = entry->GetName();
          context->TransferString(f,name);
        }
      }
      else if (entry->IsArray())
      {
        f.SaveChar(idArray);
        entry->SerializeBin(f);
      }
      else // value
      {
        f.SaveChar(idValue);
        entry->SerializeBin(f);
      }
    }
    int classContentFilePos = f.CreateFixUp();
    // save class content
    for (int i=0; i<n; i++)
    {
      LoadEntry(i);
      ParamEntry *entry = _entries[i];
      if (entry->IsClass() && entry->IsDefined())
      {
        // check class starting point
        Assert(classStartFilePos[i]>=0);
        f.FixUp(classStartFilePos[i]);
        entry->SerializeBin(f);
      }
    }
    // write to the position reserved to hold end of class pointer
    f.FixUp(classContentFilePos);
  }
  else
  {
    SetSource(context->_source,false);
    
    Assert(f.IsLoading());
    RStringB base;

    if (context->_version>=6)
    {
      context->TransferString(f,base);
    }
    else
    {
      f.Transfer(base);
    }

    SetBase(base);

    int n;
    context->TransferIndex(f,n,4);
    _entries.Realloc(n);
    
    AUTO_STATIC_ARRAY(int,classStart,64);
    classStart.Resize(n+1);
    for (int i=0; i<n; i++)
    {
      classStart[i] = -1;
    }
    classStart[n] = -1;
    
    for (int i=0; i<n; i++)
    {
      int id = f.LoadChar();
      switch (id)
      {
        case idClass:
          {
            ParamClass *cls = new ParamClass();
            cls->_parent = ParamClassLockedPtr(this);
            if (context->_version>=7)
            {
              RStringB name;
              context->TransferString(f,name);
              cls->SetName(name);
              classStart[i] = f.LoadInt();
            }
            else
            {
              cls->_sourceOffset = f.TellG();
              cls->SerializeBin(f);
              #if DIAG_UNLOAD>=300
              LogF("%s: Source offset %d",(const char *)cls->GetContext(),cls->_sourceOffset);
              #endif
            }
            _entries.Add(cls);
          }
          break;
        case idClassDecl:
          {
            RStringB name;
            context->TransferString(f, name);
            ParamClassDecl *cls = new ParamClassDecl(name);
            cls->_parent = ParamClassLockedPtr(this);
            _entries.Add(cls);
          }
          break;
        case idClassDelete:
          {
            RStringB name;
            context->TransferString(f, name);
            ParamClassDelete *cls = new ParamClassDelete(name);
            cls->_parent = ParamClassLockedPtr(this);
            _entries.Add(cls);
          }
          break;
        case idArray:
          {
            ParamArray *array = new ParamArray("");
            array->SerializeBin(f);
            _entries.Add(array);
          }
          break;
        default:
          ErrF("Unknown entry type %x in %s",id,cc_cast(GetContext()));
          break;
        case idValue:
          {
            ParamEntry *entry = CreateParamValue(f);
            entry->SerializeBin(f);
            if (entry->GetName()==AccessString)
            {
              _access = (ParamAccessMode)entry->GetInt();
              DoAssert(_access>PADefault);
            }
            _entries.Add(entry);
          }
          break;
      }
    }
    if (context->_version>=5)
    {
      int classContentEnd = f.LoadInt();
      // start i+1 is used to indicate end of i
      classStart[n] = classContentEnd;
      // we need to fill the gaps now - between classes there may be a non-class entries
      int classEnd = classContentEnd;
      for (int i=n-1; i>=0; i--)
      {
        if (classStart[i]>=0)
        {
          classEnd = classStart[i];
        }
        else
        {
          classStart[i] = classEnd;
        }
      }
      Assert(_entries.Size()==n);
      for (int i=0; i<_entries.Size(); i++)
      {
        ParamEntry *entry = _entries[i];
        if (!entry->IsClass() || !entry->IsDefined())
        {
          continue;
        }
        // if possible, we should load fixup only
        // class itself should be loaded only when needed
        if (context->_source && context->_source->CanUnload() && !context->_fullLoad)
        {
          // do not load - only insert a placeholder
          Assert(classStart[i]>0);
          Assert(classStart[i+1]>classStart[i]);
          ParamClassPlaceholder *placeholder = new ParamClassPlaceholder(entry->GetName());
          placeholder->SetSource(context->_source,false);
          placeholder->_sourceOffset = classStart[i];
          placeholder->_sourceSize = classStart[i+1]-classStart[i];
          _entries[i] = placeholder;
          #if 0 //DIAG_UNLOAD>=300
          LogF(
            "%s: Placeholder offset %d",(const char *)GetContext(placeholder->GetName()),placeholder->_sourceOffset
          );
          #endif
        }
        else
        {
          Assert(classStart[i]>0);
          f.SeekG(classStart[i]);
          ParamClass *cls = entry->GetClassInterface();
          cls->_sourceOffset = classStart[i];
          entry->SerializeBin(f);
          cls->_sourceSize = f.TellG()-classStart[i];
          Assert(f.TellG()==classStart[i+1]);
          #if 0 //DIAG_UNLOAD>=300
          LogF(
            "%s: Streaming entry offset %d",(const char *)GetContext(entry->GetName()),cls->_sourceOffset
          );
          #endif
        }
      }
      // we should skip all classes
      f.SeekG(classContentEnd);
    }
    // check access attribute
    CheckInheritedAccess();
  }
}

bool ParamClass::RequestBaseName(SerializeBinStream &f, RStringB &baseName)
{
  Assert(f.IsLoading());
  
  ParamFileContext *context = (ParamFileContext *)f.GetContext();
  // make table of names
  if (context->_version<8)
  {
    return true; // no base class for old formats - pretend it is ready
  }
  // it is very unlikely any entry name would be longer than 1024 B
  // preloading is done by 4KB blocks anyway, so using smaller buffer has little sense
  // we could implement a specific preloading for strings, but it seems like an overkill
  const int preloadNameSize = 1024;
  f.PreReadSequential(preloadNameSize);
  f.Transfer(baseName);
  Assert(baseName.GetLength()<preloadNameSize)
  return true;
}

void ParamClass::Compact()
{
  _entries.Compact();
}

void ParamClass::ReserveEntries(int count)
{
  _entries.Reserve(count,count);
}


void ParamFile::Save( QOStream &f, int indent, const char *eol ) const
{
  for( int i=0; i<GetEntryCount(); i++ )
  {
    GetEntry(i).Save(f,indent,eol);
  }
}

LSError ParamFile::Save( const char *name) const
{
  QOFStream f;
  f.open(name);
  Save(f,0,"\r\n");
  f.close();
  return f.error();
}

LSError ParamFile::Parse(QIStream &in, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  LSError lsres=LSOK;
  // caution: be carefull to avoid memory leak in case source is changed
  Ref<IParamClassSource> sourceRef = source;
  
  Clear();
  
  // TODO: change locality of variables defined by enum and EXEC / EVAL
  // TODO: evaluator should load / save global variables, not local ones
  CreateEvaluator(parentVariables, globalVariables, true);
  
  // read all class definitions
  int handle = BeginContext();
  // variable context defined
  // int this context all enum values are defined
  lsres=ParamClass::Parse(in, this);
  EndContext(handle);

  SetFile(this);
  // convert source to non-streaming one
  if (sourceRef && sourceRef->CanUnload())
  {
    LogF(
      "Strange - source %s of .cpp file can unload",
      (const char *)sourceRef->GetOwnerDebugName()
    );
    sourceRef = new ParamClassOwnerIndirectNoUnload(source);
  }
  SetSource(sourceRef);
  //LogF("Config %s cannot be streamed",(const char *)GetName());
  //SetSource(new ParamClassOwnerNoUnload("",""));
  if( in.fail() || in.eof() )
  {
    return in.fail()?LSBadFile:lsres;
  }
  ErrorMessage
  (
    "Config %s: some input after EndOfFile.",(const char *)_name
  );
  return LSStructure;
}

LSError ParamFile::Reload(IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  ParamFile source;
  LSError res=source.Parse(_name, _source, parentVariables, globalVariables);
  if (res!=LSOK) return res;
  _evaluator = source._evaluator;
  // no protection during reload
  if (!Update(source,false)) return LSUnknownError;
  SetFile(this);
  return LSOK;
}

void ParamFile::SerializeBin(SerializeBinStream &f, IParamClassSource *source, ParamFileContext &context)
{
  // caution: be careful to avoid memory leak in case source is changed
  Ref<IParamClassSource> sourceRef = source;
  context._source = sourceRef;

  if (!context.TransferFormat(f))
  {
    return;
  }
  
  void *oldContext = f.GetContext();
  f.SetContext(&context);
  if (f.IsLoading())
  {
    if (!context.LoadHeader(f))
    {
      f.SetContext(oldContext);
      return;
    }
    
    // if there is a source, use it to cache file-related data
    if (source)
    {
      source->StoreContext(context);
    }
    
    if (context._version<5 && sourceRef && sourceRef->CanUnload())
    {
      // some old binarized config files may not be streamed
      RptF(
        "Config %s may not be streamed, old version %d",
        (const char *)sourceRef->GetOwnerDebugName(),context._version
      );
      context._source = new ParamClassOwnerIndirectNoUnload(sourceRef);
    }
    
    ParamClass::SerializeBin(f);
      
    if (context._version>=6)
    {
      f.SeekG(context._endOfParamfile);
    }
  }
  else
  {
    context._version = ActualParamFileVersion;
    // the only versions rejected by retail OFP are 0 and 1
    // we therefore mark this a version 0 and save real version number as another int
    f.SaveInt(0);
    f.SaveInt(context._version);
    // save stringtable fixup
    //int stringtableBegFixup = f.CreateFixUp();
    int paramFileEndEndFixup = f.CreateFixUp();

    ParamClass::SerializeBin(f);

    //f.FixUp(stringtableBegFixup);
    //context.TransferStringTable(f);
    f.FixUp(paramFileEndEndFixup);
  }
  
  f.SetContext(oldContext);

  RegisterUnloading(context._source);
}
void ParamFile::RegisterUnloading(IParamClassSource *source)
{
  Unregister();
  if (source && source->CanUnload())
  {
    RegisterFreeOnDemandMemory(this);
  }
}

ParamFile::ErrorCode ParamFile::ParseBinEx(const char *name, IParamClassSource *source, ParamFileContext &context, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  Clear();

  SetName(name);
  if (!QFBankQueryFunctions::FileExists(name)) return SerializeBinStream::ENoFile;

  QIFStreamB in;
  in.AutoOpen(name);
  in.PreReadSequential();

  SerializeBinStream f(&in);

  SerializeBin(f, source, context);
  if (f.GetError() != SerializeBinStream::EOK) return f.GetError();

  SetFile(this);
  //SetSource(new ParamClassOwnerNoUnload("",name));

  //Assert(CheckIntegrity());

  // load variables
  LoadVariables(f, parentVariables, globalVariables);

#if DIAG_OPEN
  if (_entries.Size() > 0)
  {
    ParamFileOpen++;
    LogF("%d: Parsed paramfile %s",ParamFileOpen,name);
  }
#endif

  return f.GetError();
}

ParamFile::ErrorCode ParamFile::ParseBinEx(QIStream &in, IParamClassSource *source, ParamFileContext &context, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  Clear();

  in.PreReadSequential();

  SerializeBinStream f(&in);

  SerializeBin(f, source, context);
  if (f.GetError() != SerializeBinStream::EOK) return f.GetError();

  SetFile(this);
  //SetSource(new ParamClassOwnerNoUnload("",name));

  //Assert(CheckIntegrity());

  // load variables
  LoadVariables(f, parentVariables, globalVariables);

#if DIAG_OPEN
  if (_entries.Size() > 0)
  {
    ParamFileOpen++;
    LogF("%d: Parsed paramfile %s",ParamFileOpen,name);
  }
#endif

  return f.GetError();
}

ParamFile::ErrorCode ParamFile::ParseBinEx(const char *name, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  ParamFileContext context;
  context._fullLoad = true;
  //context._fullLoad = false;

  return ParseBinEx(name, source, context, parentVariables, globalVariables);
}


ParamFile::ErrorCode ParamFile::ParseBinEx(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  Clear();

  SetName(name);

  QIFStreamB in;
  in.open(bank, name);
  
  if (in.fail() || in.eof()) return SerializeBinStream::EGeneric;
  
  SerializeBinStream f(&in);

  // when loading from temporary bank, on demand loading is not possible
  Ref<IParamClassSource> source = new ParamClassOwnerNoUnload("",name);
  
  ParamFileContext context;
  context._fullLoad = true;
  //context._fullLoad = false;

  SerializeBin(f, source, context);
  if (f.GetError() != SerializeBinStream::EOK) return f.GetError();

  SetFile(this);
  //SetSource(new ParamClassOwnerFilename("",name));

  //Assert(CheckIntegrity());

  LoadVariables(f, parentVariables, globalVariables);

#if DIAG_OPEN
  if (_entries.Size() > 0)
  {
    ParamFileOpen++;
    LogF("%d: Parsed paramfile %s",ParamFileOpen,name);

  }
#endif
  
  return f.GetError();
}

bool ParamFile::ParseBin(const char *name, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  ParamFileContext context;
  context._fullLoad = true;
  //context._fullLoad = false;

  ErrorCode err = ParseBinEx(name, source, context, parentVariables, globalVariables);
  return err==SerializeBinStream::EOK;
}

bool ParamFile::ParseBin(QIStream &f, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  ParamFileContext context;
  context._fullLoad = true;
  //context._fullLoad = false;

  ErrorCode err = ParseBinEx(f, source, context, parentVariables, globalVariables);
  return err==SerializeBinStream::EOK;
}

bool ParamFile::ParseSigned(const char *name, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  return ParseBin(name, source, parentVariables, globalVariables);
}

bool ParamFile::ParseSigned(QIStream &f, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  return ParseBin(f, source, parentVariables, globalVariables);
}

bool ParamFile::ParseBin(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  ErrorCode err = ParseBinEx(bank, name, parentVariables, globalVariables);
  return err==SerializeBinStream::EOK;
}

bool ParamFile::SaveBin( const char *name )
{
  QOFStream out;
  out.open(name);

  SerializeBinStream f(&out);

  ParamFileContext context;
  context._fullLoad = true;
  //context._fullLoad = false;

  // no source needed when saving
  SerializeBin(f, NULL, context);

  // save variables
  SaveVariables(f, NULL, NULL); // TODO: global namespace need to be passed to save global variables

  out.close();

  return f.GetError() == SerializeBinStream::EOK && !out.fail();
}

bool ParamFile::SaveBin(QOStream &out)
{
  SerializeBinStream f(&out);

  ParamFileContext context;
  context._fullLoad = true;
  //context._fullLoad = false;

  // no source needed when saving
  SerializeBin(f, NULL, context);

  // save variables
  SaveVariables(f, NULL, NULL); // TODO: global namespace need to be passed to save global variables

  return f.GetError() == SerializeBinStream::EOK && !out.fail();
}

bool ParamFile::SaveSigned( const char *name )
{
  return SaveBin(name);
}

bool ParamFile::ParseBinOrTxt(const char *name, IParamClassSource *source, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  // make sure source is not release during ParseBin
  Ref<IParamClassSource> addRef = source;
  ErrorCode ret = ParseBinEx(name, source, parentVariables, globalVariables);
  if (ret==SerializeBinStream::EOK) return true;
  if (ret!=SerializeBinStream::EBadFileType) return false;
  return Parse(name, source, parentVariables, globalVariables) == LSOK;
}

bool ParamFile::ParseBinOrTxt(QFBank &bank, const char *name, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  ErrorCode ret = ParseBinEx(bank, name, parentVariables, globalVariables);
  if (ret==SerializeBinStream::EOK) return true;
  if (ret!=SerializeBinStream::EBadFileType) return false;
  return Parse(bank, name, parentVariables, globalVariables) == LSOK;
}

static bool ProcessLineNumber(QIStream &in)
{
  // check for #line info
  WordBuf word;
  GetAlphaWord(word, in);
  if (strcmp(word, "line") != 0)
  {
    // unexpected directive
    return false;
  }
  GetWord(word, in, " \n\r");
  bool ok = false;
  int line = ScanInt(word, ok);
  if (!ok)
  {
    // wrong line number
    return false;
  }
  GetWord(word, in, "\n\r");
  in.SetLocation(word, line - 1);
  return true;
}

