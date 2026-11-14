#ifndef _EXPRESS_IMPL_HPP
#define _EXPRESS_IMPL_HPP

#include <El/Interfaces/iEval.hpp>
#include <El/Interfaces/iDebugEngine.hpp>

#include <Es/Strings/rString.hpp>
#include <Es/Containers/hashMap.hpp>

#include <El/Enum/enumNames.hpp>

#include <El/Interfaces/iLocalize.hpp>
#include <El/ParamArchive/serializeClass.hpp>
#ifdef _MT_ELES
#include <el/MultiThread/SmartRefCount.h>
#endif

#ifdef _WIN32
#include "Java/jni.h"
#else
// use installed JDK on linux, like /usr/java/jdk1.7.0/include
// Makefile should be configured to use it, -I option
#include <jni.h>
#endif

#if 1
  // single-threaded fast-alloc forces thread unsafe fast allocator also with scalable allocators like JMAlloc
  #define USE_FAST_ALLOCATOR_EXPRESS USE_FAST_ALLOCATOR_SINGLETHREAD
  #define DEFINE_FAST_ALLOCATOR_EXPRESS(className) DEFINE_FAST_ALLOCATOR_SINGLETHREAD(className)
#else
  #define USE_FAST_ALLOCATOR_EXPRESS USE_FAST_ALLOCATOR
  #define DEFINE_FAST_ALLOCATOR_EXPRESS(className) DEFINE_FAST_ALLOCATOR(className)
#endif
//#define NVar ( 'z'-'a'+1 )
//#define GENERATE_DOCUMENT_COMREF 1
//#define DOCUMENT_COMREF 1

#ifndef DOCUMENT_COMREF
  // The c:\comref.xml or c:\comref_SR.xml will be generated when (GENERATE_DOCUMENT_COMREF && DOCUMENT_COMREF) holds
  #define GENERATE_DOCUMENT_COMREF 0
  // DOCUMENT_COMREF should be always set now (in order the GenerateComRef scripting command works properly)
#ifdef _SUPER_RELEASE
  #define DOCUMENT_COMREF 0
  // when ARMA_RELEASE_DOCUMENTATION is set, most diagnostics and debug commands are not included into the generated comref
  #define ARMA_RELEASE_DOCUMENTATION 0
#else
  #define DOCUMENT_COMREF 1
  // when ARMA_RELEASE_DOCUMENTATION is set, most diagnostics and debug commands are not included into the generated comref
  #define ARMA_RELEASE_DOCUMENTATION 0
#endif
#endif

#ifndef _ENABLE_COMREF
# if !_WIN32
#   define _ENABLE_COMREF	0
# elif _SUPER_RELEASE
#   define _ENABLE_COMREF	1
# else
#   define _ENABLE_COMREF	1
# endif
#endif

#if _SCRIPT_DEBUGGER
  #define USE_PRECOMPILATION 0  //script debugger currently doesn't support precompilation
#endif

#ifndef USE_PRECOMPILATION
  #define USE_PRECOMPILATION 1
#endif

#if _SUPER_RELEASE
  #define SCRIPTVM_DEBUGGING	0
#elif !_WIN32
  #define SCRIPTVM_DEBUGGING	0
#else
  #define SCRIPTVM_DEBUGGING	1
#endif

#if SCRIPTVM_DEBUGGING
void ProcessMessages();
RString GetFullPath(RString name);
#endif

class GameType;
struct GameTypeType;
class GameData;
class GameValue;
typedef const GameValue &GameValuePar;
class GameVarSpace;
class GameState;

typedef GameValue (*ProcBinary)(const GameState *state, GameValuePar o1, GameValuePar o2);
typedef GameValue (*ProcUnary)(const GameState *state, GameValuePar o1);
typedef GameValue (*ProcNular)(const GameState *state);

enum GamePriority
{
  nula,
  logicOr, logicAnd, compare, function, functionFirst,
  soucet, soucin, unar, mocnina, zavorky,
  gamePriorityCount
};

enum EvalError
{ // see also: "stringtable.csv" (EVAL_...)
  EvalOK,
  EvalGen, // generic error
  EvalExpo, // exponent out of range or invalid
  EvalNum, // invalid number
  EvalVar, // undefined variable
  EvalBadVar, // undefined variable
  EvalDivZero, // zero divisor
  EvalTg90, // tg 90
  EvalOpenB, // missing (
  EvalCloseB, // missing )
  EvalOpenBrackets, // missing [
  EvalCloseBrackets, // missing ]
  EvalOpenBraces, // missing {
  EvalCloseBraces, // missing }
  EvalEqu, // missing =
  EvalSemicolon, // missing ;
  EvalQuote, // missing "
  EvalSingleQuote, // missing '
  EvalOper, // unknown operator
  EvalLineLong, // line too long
  EvalType, // type mismatch
  EvalNamespace, // no name space
  EvalDim, // array dimension
  EvalUnexpectedCloseB, // unexpected )
  EvalAssertFailed, //assertion failed error
  EvalHaltCalled,  //forced error using halt function
  EvalForeignError, //user error from foreign modules
  EvalScopeNameExists, //unable to define scopeName twice
  EvalScopeNotFound, //scope has not been found
  EvalInvalidTryBlock,  //invalid usage of try block
  EvalUnhandledException,  //Exception has been thrown, but none of catch handles this exceptions
  EvalStackOverflow, //stack overflow during processing BeginContext
  EvalHandled, // error occurred, but was already handled (used when the error occurs in the inner scope)
  EvalAccess, // forbidden function called on client
};

// Macros to define and register scripting types and functions
#if _ENABLE_COMREF
/// declare new GameType identifier
# define DECLARE_TYPE(name, type, createfunc, tname, comrefname, description, category, javaType, signature) \
  extern GameTypeType GType##type; \
  const GameType type(&GType##type);
/// define GameType identifier through static GameTypeType structure
# define DEFINE_TYPE(name, type, createfunc, tname, comrefname, description, category, javaType, signature) \
  GameTypeType GType##type(name, createfunc, tname, comrefname, description, category, javaType, signature); \
/// register a new type to the GameState
# define REGISTER_TYPE(name, type, createfunc, tname, comrefname, description, category, javaType, signature) \
  state.NewType(&GType##type);

# define REGISTER_NULAR(returnType, name, func, description, example, exampleResult, since, changed, category) GameNular(returnType, name, func, description, example, exampleResult, since, changed, category, (void *)&JNI_##func),
# define REGISTER_FUNCTION(returnType, name, func, roperType, roper, description, example, exampleResult, since, changed, category) GameFunction(returnType, name, func, roperType, roper, description, example, exampleResult, since, changed, category, (void *)&JNI_##func##_##roperType),
# define REGISTER_OPERATOR(returnType, name, priority, func, loperType, roperType, loper, roper, description, example, exampleResult, since, changed, category) GameOperator(returnType, name, priority, func, loperType, roperType, loper, roper, description, example, exampleResult, since, changed, category, (void *)&JNI_##func##_##loperType##_##roperType),

# define TODO_TYPE_DOCUMENTATION , "", "", "", NULL, ""
# define TODO_NULAR_DOCUMENTATION , "", "", "", "", "", "", NULL
# define TODO_FUNCTION_DOCUMENTATION , "", "", "", "", "", "", "", NULL
# define TODO_OPERATOR_DOCUMENTATION , "", "", "", "", "", "", "", "", NULL

#else
/// declare new GameType identifier
# define DECLARE_TYPE(name, type, createfunc, tname, comrefname, description, category, javaType, signature) \
  extern GameTypeType GType##type; \
  const GameType type(&GType##type);
/// define GameType identifier through static GameTypeType structure
# define DEFINE_TYPE(name, type, createfunc, tname, comrefname, description, category, javaType, signature) \
  GameTypeType GType##type(name, createfunc, tname, javaType, signature); \
/// register a new type to the GameState
# define REGISTER_TYPE(name, type, createfunc, tname, comrefname, description, category, javaType, signature) \
  state.NewType(&GType##type);

# define REGISTER_NULAR(returnType, name, func, description, example, exampleResult, since, changed, category) GameNular(returnType, name, func, (void *)&JNI_##func),
# define REGISTER_FUNCTION(returnType, name, func, roperType, roper, description, example, exampleResult, since, changed, category) GameFunction(returnType, name, func, roperType, (void *)&JNI_##func##_##roperType),
# define REGISTER_OPERATOR(returnType, name, priority, func, loperType, roperType, loper, roper, description, example, exampleResult, since, changed, category) GameOperator(returnType, name, priority, func, loperType, roperType, (void *)&JNI_##func##_##loperType##_##roperType),

# define TODO_TYPE_DOCUMENTATION , NULL, ""
# define TODO_NULAR_DOCUMENTATION , NULL
# define TODO_FUNCTION_DOCUMENTATION , NULL
# define TODO_OPERATOR_DOCUMENTATION , NULL
#endif

/// create a bridge function from JNI to scripting function
# define JNI_NULAR(returnType, name, func, description, example, exampleResult, since, changed, category) \
  static ConvertTypes<&GType##returnType>::javaType JNICALL JNI_##func(JNIEnv *env, jobject obj) \
{ \
  SCOPE_FPU_ENGINE \
  GameValue result = func(&GGameState); \
  return ConvertTypes<&GType##returnType>::FromGameValue(env, result); \
  }
# define JNI_FUNCTION(returnType, name, func, roperType, roper, description, example, exampleResult, since, changed, category) \
  static ConvertTypes<&GType##returnType>::javaType JNICALL JNI_##func##_##roperType(JNIEnv *env, jobject obj, ConvertTypes<&GType##roperType>::javaType par) \
{ \
  GameValue val = ConvertTypes<&GType##roperType>::ToGameValue(env, par); \
  SCOPE_FPU_ENGINE \
  GameValue result = func(&GGameState, val); \
  return ConvertTypes<&GType##returnType>::FromGameValue(env, result); \
  }
# define JNI_OPERATOR(returnType, name, priority, func, loperType, roperType, loper, roper, description, example, exampleResult, since, changed, category) \
  static ConvertTypes<&GType##returnType>::javaType JNICALL JNI_##func##_##loperType##_##roperType(JNIEnv *env, jobject obj, ConvertTypes<&GType##loperType>::javaType par1, ConvertTypes<&GType##roperType>::javaType par2) \
{ \
  GameValue val1 = ConvertTypes<&GType##loperType>::ToGameValue(env, par1); \
  GameValue val2 = ConvertTypes<&GType##roperType>::ToGameValue(env, par2); \
  SCOPE_FPU_ENGINE \
  GameValue result = func(&GGameState, val1, val2); \
  return ConvertTypes<&GType##returnType>::FromGameValue(env, result); \
  }

/// multiple possible types (common only when some variable is undefined, while validating expressions)
class CompoundGameType: public FindArrayKey<const GameTypeType *>, public SerializeClass, public CountInstances<CompoundGameType>
{
public:
  CompoundGameType() {}
  CompoundGameType(const GameTypeType *id)
  {
    Realloc(1); Resize(1); Set(0) = id;
  }
  CompoundGameType(const GameTypeType *id1, const GameTypeType *id2)
  {
    Realloc(2); Resize(2); Set(0) = id1; Set(1) = id2;
  }
  CompoundGameType operator |(const CompoundGameType &with) const;
  CompoundGameType operator &(const CompoundGameType &with) const;
  bool operator ==(const CompoundGameType &with) const;
  void operator |=(const CompoundGameType &with);
  void operator &=(const CompoundGameType &with);
  operator bool() const {return Size() > 0;}

  LSError Serialize(ParamArchive &ar);
};

/// scripting type identifier (can combine multiple simple types)
/**
Optimized for a typical case where type is known (sigle type only)
*/
class GameType: public SerializeClass
{
  const GameTypeType *_singleType;
  SRef<CompoundGameType> _multipleTypes;

  public:
  GameType() {_singleType = NULL;}
  GameType(const GameType &src)
  :_singleType(src._singleType),
  _multipleTypes(src._multipleTypes.NotNull() ? new CompoundGameType(*src._multipleTypes) : NULL)
  {}
  void operator = (const GameType &src)
  {
    _singleType = src._singleType;
    _multipleTypes = src._multipleTypes.NotNull() ? new CompoundGameType(*src._multipleTypes) : NULL;
  }
  
  GameType(const GameTypeType *id) {_singleType = id;}
  GameType operator |(const GameType &with) const;
  GameType operator &(const GameType &with) const;
  bool operator ==(const GameType &with) const;
  bool operator !=(const GameType &with) const {return !(operator ==(with));}
  void operator |=(const GameType &with);
  void operator &=(const GameType &with);
  operator bool() const
  {
    // we assume canonical form (no empty _multipleTypes)
    Assert(_multipleTypes.IsNull() || _multipleTypes->Size()>0);
    return _singleType!=NULL || _multipleTypes.NotNull();
  }
  // Linux port: default operator ! did not use bool(), causing critical errors
  //    Error in expression <-1>, Error position: <-1>, Error -: Type Number, expected
  bool operator !() const { return !(operator bool()); }
  
  int Size() const
  {
    if (_multipleTypes) return _multipleTypes->Size();
    return _singleType ? 1 : 0;
  }
  
  const GameTypeType *operator [] (int index) const
  {
    if (_multipleTypes) return _multipleTypes->Get(index);
    Assert(index == 0);
    return _singleType;
  }

  RString GetJavaType() const;
  RString GetSignature() const;

  LSError Serialize(ParamArchive &ar);
  void Canonicalize();
};


typedef GameData *CreateGameDataFunction(ParamArchive *ar);

struct GameTypeType: public CountInstances<GameTypeType>
{
  RString _name;
  CreateGameDataFunction *_createFunction;
  RString _typeName;
#if _ENABLE_COMREF
  RString _comRefName;
  RString _description;
  RString _category;
#endif
  RString _javaType;
  RString _signature;

#if _ENABLE_COMREF
  /// create a simple type
  GameTypeType(RStringB name, CreateGameDataFunction *function, RString typeName, RString comRefName, RString description, RString category, RString javaType, RString signature)
  {
    _name = name; _createFunction = function; _typeName = typeName;
    _comRefName = comRefName; _description = description; _category = category;
    _javaType = javaType; _signature = signature;
  }
//   /// create a combined type
//   GameTypeType(const GameType &type, RStringB name, CreateGameDataFunction *function, RString typeName, RString comRefName, RString description, RString category)
//   {
//     Fail("Not supported");
//     _name = name; _createFunction = function; _typeName = typeName;
//     _comRefName = comRefName; _description = description; _category = category;
//   }
  RString FormatHelp() const;
#else
  /// create a simple type
  GameTypeType(RStringB name, CreateGameDataFunction *function, RString typeName, RString javaType, RString signature)
  {
    _name = name; _createFunction = function; _typeName = typeName;
    _javaType = javaType; _signature = signature;
  }
//   /// create a combined type
//   GameTypeType(const GameType &type, RStringB name, CreateGameDataFunction *function, RString typeName)
//   {
//     Fail("Not supported");
//     _name = name; _createFunction = function; _typeName = typeName;
//   }
#endif
  GameTypeType();
  GameData *CreateGameData(ParamArchive *ar) {return _createFunction(ar);}
  RString GetJavaType() const {return _javaType;}
  RString GetSignature() const {return _signature;}
};

#ifndef ACCESS_ONLY
class ParamArchive;

#endif

typedef float GameScalarType;
typedef bool GameBoolType;
typedef RString GameStringType;
typedef AutoArray<GameValue> GameArrayType;

extern const GameArrayType GameArrayEmpty;
extern GameArrayType GameArrayDummy;

#ifdef _MT_ELES
class GameData: public MultiThread::SmartRefCount, public IDebugValue
#else
class GameData: public RefCount, public IDebugValue
#endif
{
  public:

#ifdef _MT_ELES
    //force MTAware for ref count.
  GameData():SmartRefCount(true) {}
#endif

  virtual const GameType &GetType() const = 0;
  virtual ~GameData(){}

  virtual GameBoolType GetBool() const {Fail("Bad conversion: bool");return false;}
  virtual GameScalarType GetScalar() const {Fail("Bad conversion: scalar");return 0;}
  virtual GameStringType GetString() const {Fail("Bad conversion: string");return "";}
  virtual const GameArrayType &GetArray() const {Fail("Bad conversion: array");return GameArrayEmpty;}
  virtual GameArrayType &GetArray() {Fail("Bad conversion: array");return GameArrayDummy;}
  virtual GameData *Clone() const {return NULL;}
  //! if there are any shared data (see GameDataArray), we may wish to set them as read only
  virtual void SetReadOnly(bool val) {}
  //! check read only status
  virtual bool GetReadOnly() const {return false;}

  virtual RString GetText() const = 0;
  virtual bool IsEqualTo(const GameData *data) const = 0;

  virtual const char *GetTypeName() const {return "unknown";}
  virtual bool GetNil() const {return false;}

  virtual jobject ToJObject(JNIEnv *env, GameValuePar from) const = 0;

  /// by default the data type supports serialization
  virtual bool SupportSerialization() const {return true;}

  #ifndef ACCESS_ONLY
  static GameData *CreateObject(ParamArchive &ar);
  virtual LSError Serialize(ParamArchive &ar);
  #endif

  // IDebugValue
  virtual int IAddRef();
  virtual int IRelease();

  virtual void GetValueType(char *buffer, int len) const;
  virtual void GetValueValue(unsigned int radix, char *buffer, int len) const;
  virtual bool IsExpandable() const;
  virtual int NItems() const;
  virtual IDebugValueRef GetItem(int i) const;
};

class GameState;

#include <Es/Memory/normalNew.hpp>

class GameDataNil: public GameData
{
  GameType _type;

public:
  GameDataNil(const GameType &type) : _type(type){}
  const GameType &GetType() const {return _type;}

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  bool GetNil() const {return true;}
  const char *GetTypeName() const {return "void";}

  virtual GameBoolType GetBool() const {return false;}
  virtual GameScalarType GetScalar() const {return 0;}
  virtual GameStringType GetString() const {return "";}
  virtual const GameArrayType &GetArray() const {return GameArrayEmpty;}
  virtual GameArrayType &GetArray() {return GameArrayDummy;}
  
  GameData *Clone() const {return new GameDataNil(*this);}

  virtual jobject ToJObject(JNIEnv *env, GameValuePar from) const {return NULL;}

#ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameDataScalar : public GameData
{
  typedef GameData base;

  GameScalarType _value;

  static jweak _class;
  static jmethodID _constructorID;
  static jclass InitClass(JNIEnv *env);

public:
  GameDataScalar():_value(0){}
  GameDataScalar( GameScalarType value ):_value(value){}
  ~GameDataScalar(){}

  const GameType &GetType() const;
  GameScalarType GetScalar() const {return _value;}

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "float";}
  GameData *Clone() const {return new GameDataScalar(*this);}

  virtual jobject ToJObject(JNIEnv *env, GameValuePar from) const;

#ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameDataNothing: public GameData
{
  typedef GameData base;

public:
  GameDataNothing(){}
  ~GameDataNothing(){}

  const GameType &GetType() const;

  RString GetText() const {return "nothing";}
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "nothing";}
  GameData *Clone() const {return new GameDataNothing(*this);}

  virtual jobject ToJObject(JNIEnv *env, GameValuePar from) const {return NULL;}

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameDataString: public GameData
{
  typedef GameData base;

  GameStringType _value;

public:
  GameDataString():_value(){}
  GameDataString( GameStringType value ):_value(value){}
  ~GameDataString(){}

  const GameType &GetType() const;
  GameStringType GetString() const {return _value;}

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "string";}
  GameData *Clone() const {return new GameDataString(*this);}

  virtual jobject ToJObject(JNIEnv *env, GameValuePar from) const;

#ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameDataBool: public GameData
{
  typedef GameData base;

  GameBoolType _value;

  static jweak _class;
  static jmethodID _constructorID;
  static jclass InitClass(JNIEnv *env);

public:
  GameDataBool():_value(false){}
  GameDataBool( GameBoolType value ):_value(value){}
  ~GameDataBool(){}

  const GameType &GetType() const;
  GameBoolType GetBool() const {return _value;}
  GameScalarType GetScalar() const {return _value;}

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "bool";}
  GameData *Clone() const {return new GameDataBool(*this);}

  virtual jobject ToJObject(JNIEnv *env, GameValuePar from) const;

#ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameDataArray: public GameData
{
  typedef GameData base;

  //! actual array data
  GameArrayType _value;
  //! some array may be read only
  bool _readOnly;

  static jweak _class;
  static jmethodID _constructorID;
  static jmethodID _addID;
  static jclass InitClass(JNIEnv *env);

public:
  GameDataArray();
  GameDataArray(const GameArrayType &value);
  ~GameDataArray();

  const GameType &GetType() const;
  const GameArrayType &GetArray() const {return _value;}
  GameArrayType &GetArray() {return _value;}
  //! change or crate given element
  void SetReadOnly(bool val){_readOnly=val;}
  bool GetReadOnly()const {return _readOnly;}

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "array";}
  GameData *Clone() const;

  virtual jobject ToJObject(JNIEnv *env, GameValuePar from) const;

#ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
#endif

  USE_FAST_ALLOCATOR_EXPRESS

    // IDebugValue
    virtual bool IsExpandable() const;
  virtual int NItems() const;
  virtual IDebugValueRef GetItem(int i) const;
};

class GameValue : public SerializeClass
{
  friend class GameState;
  friend class GameVariable;

  // only one of these should be valid
  protected:
  Ref<GameData> _data;

  public:
  explicit GameValue( GameData *data );

  public:
  GameValue();

  GameValue( GameBoolType value );
  GameValue( GameScalarType value );
  GameValue( const GameArrayType &value );
  GameValue( GameStringType value );
  GameValue( const char *value );

  void operator = ( bool value );
  void operator = ( float value );
  void operator = ( const GameArrayType &value );
  void operator = ( GameStringType value );
  void operator = ( const char *value );

  // special handling of copy
  GameValue( const GameValue &value );
  void operator = ( const GameValue &value );

  void Duplicate( const GameValue &value );
  bool SharedInstance() const {return _data ? _data->RefCounter()>1 : false;}

  // access
  GameData *GetData() const {return _data;}
  const GameType &GetType() const;
  bool GetNil() const {return _data ? _data->GetNil() : true;}

  operator GameBoolType() const {return _data ? _data->GetBool() : false;}
  operator GameScalarType() const {return _data ? _data->GetScalar() : 0;}
  operator const GameArrayType &() const {return _data ? _data->GetArray() : GameArrayEmpty;}
  operator GameArrayType &() {return _data ? _data->GetArray() : GameArrayDummy;}
  operator GameStringType() const {return _data ? _data->GetString() : "";}

  bool GetReadOnly() const {return _data ? _data->GetReadOnly() : false;}
  void SetReadOnly(bool val) {if (_data) _data->SetReadOnly(val);}

  RString GetText() const;
  RString GetDebugText() const;
  bool IsEqualTo(const GameValue &value) const;
  /** used for default values in Serialize */
  bool operator ==(const GameValue &value) const {return IsEqualTo(value);}

  jobject ToJObject(JNIEnv *env) const
  {
    if (GetNil()) return NULL;
    return _data->ToJObject(env, *this);
  }

  /// Check if contained data supports serialization (unassigned value is OK)
  bool SupportSerialization() const {return _data ? _data->SupportSerialization() : true;}

  #ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
  #else
  LSError Serialize(ParamArchive &ar) {return LSOK;}
  #endif
};

// helper class to encode/decode GameValue to a Java object
class CachedJavaClass
{
private:
  const char *_name;
  jweak _class;
  jmethodID _constructorID;
  jfieldID _peerID;

  jclass Init(JNIEnv *env);

public:
  CachedJavaClass(const char *name)
    : _name(name), _class(NULL), _constructorID(NULL), _peerID(NULL) {}

  // convert GameValue -> jobject
  jobject FromGameValue(JNIEnv *env, const GameValue &from);
  // convert jobject -> GameValue
  GameValue ToGameValue(JNIEnv *env, jobject from);
};

// by default, encode GameValue to a Java object as a pointer
template <const GameTypeType *type>
class ConvertTypes
{
public:
  typedef jobject javaType;
  static CachedJavaClass _cache;

  static javaType FromGameValue(JNIEnv *env, const GameValue &from)
  {
    return _cache.FromGameValue(env, from);
  }
  static GameValue ToGameValue(JNIEnv *env, javaType from)
  {
    return _cache.ToGameValue(env, from);
  }
};

class GameDataSwitch: public GameData
{
public:
  enum SwitchMode {swNoTriggered,swTriggerNext,swTriggered};

  SwitchMode _mode;
  GameValue _test;
  GameValue _selBranch;
  GameValue _defBranch;

  const GameType &GetType() const;
  const char *GetTypeName() const {return "Switch <invisible>";}
  GameData *Clone() const {return new GameDataSwitch(*this);}
  virtual RString GetText() const  {return _test.GetText();}
  virtual bool IsEqualTo(const GameData *data) const {return false;}

  jobject ToJObject(JNIEnv *env, GameValuePar from) const;

  USE_FAST_ALLOCATOR_EXPRESS
};

#include <Es/Memory/debugNew.hpp>

TypeIsMovable(GameValue)

class GameVariable : public IDebugVariable
{
public:
  RString _name;
  GameValue _value;
  bool _readOnly;

  GameVariable()
  :_name(),_readOnly(false)
  {
  }
  GameVariable( RString name, GameValuePar value, bool readOnly=false )
  :_name(name),_readOnly(readOnly)
  {
    _value=value;
  };
  const char *GetKey() const {return _name;}
  LSError Serialize(ParamArchive &ar);
  
  RString GetName() const {return _name;}
  RString GetValueText() const {return _value.GetText();}

  bool IsReadOnly() const {return _readOnly;}

  // IDebugVariable implementation
  virtual void GetVarName(char *buffer, int len) const;
  virtual IDebugValueRef GetVarValue() const;
};

TypeIsMovable(GameVariable);

typedef MapStringToClass<GameVariable, AutoArray<GameVariable> > VarBankType;

//typedef double Real;
typedef float Real;

const int evalStackLevels=256; // stack levels

class NularOperator : public RefCount
{
protected:
  ProcNular _proc;
  GameType _retType;

public:
  NularOperator(ProcNular proc, const GameType &retType) : _proc(proc), _retType(retType) {}
  const GameType &GetRetType() const {return _retType;}
  GameValue Evaluate(const GameState *state) {return _proc(state);}
};

class UnaryOperator : public RefCount
{
protected:
  ProcUnary _proc;
  GameType _retType;
  GameType _argType;

public:
  UnaryOperator(ProcUnary proc, const GameType &retType, const GameType &argType)
    : _proc(proc), _retType(retType), _argType(argType) {}
  const GameType &GetRetType() const {return _retType;}
  const GameType &GetArgType() const {return _argType;}
  GameValue Evaluate(const GameState *state, GameValuePar oper) {return _proc(state, oper);}
};

class BinaryOperator : public RefCount
{
protected:
  ProcBinary _proc;
  GameType _retType;
  GameType _arg1Type;
  GameType _arg2Type;

public:
  BinaryOperator(ProcBinary proc, const GameType &retType, const GameType &arg1Type, const GameType &arg2Type)
    : _proc(proc), _retType(retType), _arg1Type(arg1Type), _arg2Type(arg2Type) {}
  const GameType &GetRetType() const {return _retType;}
  const GameType &GetArg1Type() const {return _arg1Type;}
  const GameType &GetArg2Type() const {return _arg2Type;}
  GameValue Evaluate(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {return _proc(state, oper1, oper2);}
};

struct GameOpName
{
  RString _opName;
  GameOpName(){}
  GameOpName(RString name):_opName(name){}
};

struct GameNular: public GameOpName
{
  RString _name;
  Ref<NularOperator> _operator;
#if _ENABLE_COMREF
  RString _description;
  RString _example;
  RString _exampleResult;
  RString _since;
  RString _changed;
  RString _category;
#endif
  void *_jniFunc;

  GameNular(){_jniFunc = NULL;}
#if _ENABLE_COMREF
  GameNular(const GameType &retType, RString name, ProcNular proc,
    RString description, RString example, RString exampleResult, RString since, RString changed, RString category, void *jniFunc)
    : GameOpName(name), _name(name), _description(description), _example(example), _exampleResult(exampleResult),
    _since(since), _changed(changed), _category(category), _jniFunc(jniFunc)
  {
    _name.Lower();
    _operator = new NularOperator(proc, retType);
  }
  RString FormatHelp() const;
#else
  GameNular(const GameType &retType, RString name, ProcNular proc, void *jniFunc)
    : GameOpName(name), _name(name), _jniFunc(jniFunc)
  {
    _name.Lower();
    _operator = new NularOperator(proc, retType);
  }
#endif
  GameNular(RString name, NularOperator *oper)
    : GameOpName(name), _name(name), _operator(oper)
  {
    _jniFunc = NULL;
    _name.Lower();
  }
  const char *GetKey() const {return _name;}
  RString GetSignature() const
  {
    return "()" + _operator->GetRetType().GetSignature();
  }
};
TypeIsMovableZeroed(GameNular)

typedef MapStringToClass<GameNular, AutoArray<GameNular> > GameNularsType;

struct GameFunction: public GameOpName
{
  RString _name;
  Ref<UnaryOperator> _operator;
#if _ENABLE_COMREF
  RString _roper;
  RString _description;
  RString _example;
  RString _exampleResult;
  RString _since;
  RString _changed;
  RString _category;
#endif
  void *_jniFunc;

  GameFunction(){_jniFunc = NULL;}
#if _ENABLE_COMREF
  GameFunction(const GameType &retType, RString name, ProcUnary proc, const GameType &argType,
    RString roper, RString description, RString example, RString exampleResult, RString since, RString changed, RString category, void *jniFunc)
    : GameOpName(name), _name(name), _roper(roper), _description(description), _example(example), _exampleResult(exampleResult),
    _since(since), _changed(changed), _category(category), _jniFunc(jniFunc)
  {
    _name.Lower();
    _operator = new UnaryOperator(proc, retType, argType);
  }
  RString GetOperName() const;
  RString FormatHelp() const;
#else
  GameFunction(const GameType &retType, RString name, ProcUnary proc, const GameType &argType, void *jniFunc)
    : GameOpName(name), _name(name), _jniFunc(jniFunc)
  {
    _name.Lower();
    _operator = new UnaryOperator(proc, retType, argType);
  }
#endif
  GameFunction(RString name, UnaryOperator *oper)
    : GameOpName(name),_name(name),_operator(oper)
  {
    _jniFunc = NULL;
    _name.Lower();
  }
  RString GetSignature() const
  {
    return "(" + _operator->GetArgType().GetSignature() + ")" + _operator->GetRetType().GetSignature();
  }
};
TypeIsMovableZeroed(GameFunction)

struct GameFunctions : public AutoArray<GameFunction>, public GameOpName
{
  RString _name;
  GameFunctions(){}
  GameFunctions(RString name)
  :GameOpName(name),_name(name)
  {_name.Lower();}
  const char *GetKey() const {return _name;}
};
TypeIsMovable(GameFunctions)

typedef MapStringToClass<GameFunctions, AutoArray<GameFunctions> > GameFunctionsType;

struct GameOperator: public GameOpName
{
  RString _name;
  GamePriority _priority;
  Ref<BinaryOperator> _operator;
#if _ENABLE_COMREF
  RString _loper;
  RString _roper;
  RString _description;
  RString _example;
  RString _exampleResult;
  RString _since;
  RString _changed;
  RString _category;
#endif
  void *_jniFunc;

  GameOperator(){_jniFunc = NULL;}
#if _ENABLE_COMREF
  GameOperator(const GameType &retType, RString name, GamePriority priority, ProcBinary proc, const GameType &argType1, const GameType &argType2,
    RString loper, RString roper, RString description, RString example, RString exampleResult, RString since, RString changed, RString category, void *jniFunc)
    : GameOpName(name), _name(name), _priority(priority),
    _loper(loper), _roper(roper), _description(description), _example(example), _exampleResult(exampleResult),
    _since(since), _changed(changed), _category(category), _jniFunc(jniFunc)
  {
    _name.Lower();
    _operator = new BinaryOperator(proc, retType, argType1, argType2);
  }
  RString GetLOperName() const;
  RString GetROperName() const;
  RString FormatHelp() const;
#else
  GameOperator(const GameType &retType, RString name, GamePriority priority, ProcBinary proc, const GameType &argType1, const GameType &argType2, void *jniFunc)
    : GameOpName(name), _name(name), _priority(priority), _jniFunc(jniFunc)
  {
    _name.Lower();
    _operator = new BinaryOperator(proc, retType, argType1, argType2);
  }
#endif
  GameOperator(RString name, BinaryOperator *oper, GamePriority priority)
    : GameOpName(name), _name(name), _operator(oper), _priority(priority)
  {
    _jniFunc = NULL;
    _name.Lower();
  }
  RString GetSignature() const
  {
    return "(" + _operator->GetArg1Type().GetSignature() + _operator->GetArg2Type().GetSignature() + ")" + _operator->GetRetType().GetSignature();
  }
};
TypeIsMovableZeroed(GameOperator)

struct GameOperators : public AutoArray<GameOperator>, public GameOpName
{
  RString _name;
  GamePriority _priority;
  GameOperators(){}
  GameOperators(RString name, GamePriority priority)
  :GameOpName(name),_name(name),_priority(priority)
  {_name.Lower();}
  const char *GetKey() const {return _name;}
};
TypeIsMovable(GameOperators)

typedef MapStringToClass<GameOperators, AutoArray<GameOperators> > GameOperatorsType;

typedef MapStringToClass<RString, AutoArray<RString> > GameNameType;


class GameVarSpace: public SerializeClass, public IEvaluatorVariables
{
  public:
  VarBankType _vars;
  const GameVarSpace *_parent;
  /// this variable space can be serialized
  bool _enabledSerialization;

#if !USE_PRECOMPILATION // for compiled code, this is stored in VMContext

    /// Flag about exiting block
    /** this flag should be set, when command needs force to break current block (and stops all loops)
    For example "break" command. Owner command-implementation function can test this flag, if block has been 
    exited, and use this information (for example, stops looping this block) 

    EvaluateMultiple tests this flag at the end of command. When flag is set true, function exits and 
    assumes current value as return;
    */
    bool _blockExit; 
    /// Name of current scope - optional, can be used to identify scope in source
    RString _scopeName;
#endif

  //! no local variables inherited
  GameVarSpace(bool enableSerialization = false);
  //! inherit all local variables from some parent scope
  GameVarSpace(GameVarSpace *parent, bool enableSerialization = false);

  ~GameVarSpace();

  void Reset();

  static bool IsNull(const GameVariable &value) {return VarBankType::IsNull(value);}
  static bool NotNull(const GameVariable &value) {return VarBankType::NotNull(value);}
  static GameVariable &Null() {return VarBankType::Null();}

  /// this variable space can be serialized
  bool IsSerializationEnabled() const {return _enabledSerialization;}
  /// set that this variable space cannot be serialized
  void DisableSerialization() {_enabledSerialization = false;}

  //! set variable in this and all parent scopes
  /**
  @return function returns true, when new local variable has been added. Function returns false, 
  when no variable has been added. This helps to detect using of undeclared variables. Only
  assignment to undeclared variable is reported by returning true.
  */
  bool VarSet(const char *name, GameValuePar value, bool readOnly=false);
  //! get variable in this and all parent scopes
  bool VarGet(const char *name, GameValue &ret) const;
  //! create undefined variable in the innermost scope
  void VarLocal( const char *name);

#if _ENABLE_CHEATS
  /// Direct access to variables for diagnostics only
  VarBankType &GetVariables() {return _vars;}
#endif

#if !ACCESS_ONLY
  virtual LSError Serialize(ParamArchive &ar);
#else
  virtual LSError Serialize(ParamArchive &ar) {return LSOK;}
#endif
};

#include <Es/Memory/normalNew.hpp>

/// Space of global variables, may be extended to store scripts etc.
class GameDataNamespace : public GameData, public SerializeClass, public IEvaluatorNamespace
{
protected:
  /// set of global variables
  VarBankType _variables;
  /// unique identifier of static namespaces
  RString _name;
  /// this variable space can be serialized
  bool _enabledSerialization;

public:
  /// constructor for static namespaces
  GameDataNamespace(GameState *state, RString name, bool enableSerialization = false, bool addRef = true);
  // TODO: constructor for dynamic namespaces

  const GameType &GetType() const;
  RString GetName() const {return _name;}

  static bool IsNull(const GameVariable &value) {return VarBankType::IsNull(value);}
  static bool NotNull(const GameVariable &value) {return VarBankType::NotNull(value);}
  static GameVariable &Null() {return VarBankType::Null();}

  /// this variable space can be serialized
  bool IsSerializationEnabled() const {return _enabledSerialization;}
  /// set that this variable space cannot be serialized
  void DisableSerialization() {_enabledSerialization = false;}

  /// clear all variables
  void Reset();
  /// set variable
  /**
  @return function returns true, when new local variable has been added. Function returns false, 
  when no variable has been added. This helps to detect using of undeclared variables. Only
  assignment to undeclared variable is reported by returning true.
  */
  bool VarSet(const char *name, GameValuePar value, bool readOnly = false);
  /// get variable
  bool VarGet(const char *name, GameValue &ret) const;
  /// delete variable
  void VarDelete(const char *name) {_variables.Remove(name);}

  /// direct access to variable space
  const VarBankType &GetVariables() const {return _variables;}
  /// direct access to variable space (serialization)
  VarBankType &GetVariables() {return _variables;}

  /// direct access to variable space
  const GameVariable &GetVariable(const char *name) const
  {
    Assert(RString::IsLowCase(name));
    return _variables[name];
  }

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "namespace";}
  GameData *Clone() const;

  jobject ToJObject(JNIEnv *env, GameValuePar from) const;

#if !ACCESS_ONLY
  virtual LSError Serialize(ParamArchive &ar);
#else
  virtual LSError Serialize(ParamArchive &ar) {return LSOK;}
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

#include <Es/Memory/debugNew.hpp>

#if USE_PRECOMPILATION

/// Source document
struct SourceDoc : public SerializeClass
{
  RString _filename;
  RString _content;

  
  LSError Serialize(ParamArchive &ar);
  SourceDoc(const RString filename=RString(), const RString content=RString()):_filename(filename),_content(content) {}
};

/// Position in source document
struct SourceDocPos : public SerializeClass
{
  RString _sourceFile;
  int _sourceLine;

  RString _content;
  int _pos;

  SourceDocPos();
  SourceDocPos(SourceDoc &doc);

  LSError Serialize(ParamArchive &ar);

  // skip all white spaces
  void SkipSpaces();
  // skip given number of characters
  void Skip(int chars);
  // read single character (without moving of _pos)
  char GetChar();
  // read multiple characters (without moving of _pos)
  void GetChars(char *chars, int count);
  // read single identifier (without moving of _pos)
  RString GetWord();
  // read quoted string (and move _pos)
  bool ReadString(RString &str);
  // read string in {} (and move _pos)
  bool ReadCodeString(RString &str);
  // read number (and move _pos)
  bool ReadNumber(float &number);

protected:
  bool ProcessLineNumber();
};

#if SCRIPTVM_DEBUGGING
enum DebugState
{
  DSNone,
  DSAttached,
  DSEntered,
  DSRunning,
  DSBreaked,
  DSStepInit,
  DSStep
};

struct VMBreakpoint
{
  DWORD _id;
  bool _enabled;

  VMBreakpoint()
  {
    _id = 0;
    _enabled = false;
  }
};

struct VMBreakpointInfo
{
  RString _file;
  int _line;
  DWORD _id;
  bool _enabled;
};
TypeIsMovableZeroed(VMBreakpointInfo)

struct BreakpointInfoTraits
{
  typedef DWORD KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  /// get a key from an item
  static KeyType GetKey(const VMBreakpointInfo &a) {return a._id;}
};

#endif //SCRIPTVM_DEBUGGING

enum RequiredExecutionControl
{
  RECInstruction, // execution will continue with the next instruction
  RECContinue, // continue with the next instruction
  RECDone, // current level is done
  RECYield // execution is interrupted (suspended) for now
};

class GameInstruction;

/// Single level of call stack
class CallStackItem : public RefCount, public IDebugScope
{
protected:
  /// parent call stack item
  CallStackItem *_parent;
  /// local variables
  GameVarSpace _vars;
  /// number of values on the data stack when item was created
  int _stackBottom; 
  /// number of values on the data stack before the current instruction processing
  int _stackLast;
  RString _scopeName;

#if SCRIPTVM_DEBUGGING
  // source context
  RString _debugName;
  RString _sourceFile;
  int _sourceLine;
#endif

public:
  CallStackItem(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, bool enableSerialization = false);
  GameVarSpace &GetVariables() {return _vars;}
  int GetStackBottom() const {return _stackBottom;}
  void SetStackLast(int stackSize) {_stackLast = stackSize;}
  bool SetScopeName(RString name)
  {
    if (_scopeName.GetLength() == 0)
    {
      _scopeName = name; return true;
    }
    return stricmp(_scopeName, name) == 0;
  }
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state) = 0;
  virtual bool OnException(const GameState *state) {return false;}
  virtual bool OnBreak(const GameState *state);

  virtual RString GetTypeName() const = 0;
  static CallStackItem *CreateObject(ParamArchive &ar);
  virtual LSError Serialize(ParamArchive &ar);

#if SCRIPTVM_DEBUGGING
  void SetSourcePos(RString sourceFile, int sourceLine) {_sourceFile = sourceFile; _sourceLine = sourceLine;}

  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint) = 0;
  virtual void RemoveBreakpoint(DWORD bp) = 0;
  virtual void EnableBreakpoint(DWORD bp, bool enable) = 0;
#endif

  // IDebugScope implementation
  virtual const char *GetName() const;
  virtual int NVariables() const;
  virtual int GetVariables(const IDebugVariable **storage, int count) const;
  virtual IDebugValueRef EvaluateExpression(const char *code, unsigned int radix);
  virtual void GetDocumentPos(char *file, int fileSize, int &line);
  virtual IDebugScope *GetParentScope();

protected:
  // implementation
  void Break(const GameState *state);
};

class GameDataCode;

/// interface to functor telling when to interrupt evaluation
class IEvalInterrupt
{
public:
  virtual ~IEvalInterrupt() {}
  virtual bool operator() () const = 0;
};

/// default implementation if IEvalInterrupt (never interrupt)
class EvalDoNotInterrupt : public IEvalInterrupt
{
public:
  virtual bool operator() () const {return false;}
};


// On Load Callback
class VMContextCallback
{
public:
  virtual bool operator() (RString script) =0;
};

/// Virtual machine evaluation context
struct VMContext : public SerializeClass
{
protected:
  // make sure only controlled access to _callStack occurs
  /** Preallocated to handle typical code nesting depth */
  RefArray<CallStackItem, MemAllocLocal<Ref<CallStackItem>,64> > _callStack;
#if SCRIPTVM_DEBUGGING
  // This variable must be aligned - used for multiprocess synchronization
  DebugState _debugState;
  // List of all active breakpoints
  FindArrayKey<VMBreakpointInfo, BreakpointInfoTraits> _breakpoints;

  // type of step
  StepKind _stepKind;
  // unit of step
  StepUnit _stepUnit;
  // minimal level where can be step finished
  int _stepLevel;
#endif

  /// serialization of contained variables is enabled
  bool _enabledSerialization;

  /// callback to check scripts execution (currently used by BattlEye)
  VMContextCallback *_callback;

public:
  /// callback which should be used as default
  static VMContextCallback *defaultCallback;

  SourceDoc _doc;
  SourceDocPos _lastInstruction; // position of the last executed instruction

  bool _isUIContext; // the script is run as a result of some UI Control Event

  /** Preallocated to handle typical expression depth */
  AutoArray<GameValue, MemAllocLocal<GameValue,32> > _stack;
  
  /// User friendly name of the script, usable in profiling / debugging
  RString _displayName;

  bool _canSuspend;
  bool _localOnly;
  /// what to do when undefined variable is encountered
  /**
  nil can be returned, or error can be reported
  */
  bool _undefinedIsNil;
  // exception and break info
  bool _exceptionThrown;
  bool _break;
  bool _breakOut;
  RString _breakScope;
  GameValue _exceptionValue;
  GameValue _breakValue;

  VMContext(bool enableSerialization = false)
    : _enabledSerialization(enableSerialization)
  {
    _canSuspend = false;
    _localOnly = false;
    _exceptionThrown = false;
    _break = false;
    _breakOut = false;
    _undefinedIsNil = true;
    _callback = VMContext::defaultCallback;
    _isUIContext = false;
#if SCRIPTVM_DEBUGGING
    _debugState = DSNone;
#endif
  }
  int GetStackBottom() const
  {
    int level = _callStack.Size() - 1;
    if (level < 0) return 0;
    return _callStack[level]->GetStackBottom();
  }
  CallStackItem *GetCallStack()
  {
    int level = _callStack.Size() - 1;
    if (level < 0) return NULL;
    return _callStack[level];
  }
  CallStackItem *GetCallStack(int index)
  {
    if (index < 0) return NULL;
    return _callStack[index];
  }
  GameVarSpace *GetVariables()
  {
    int level = _callStack.Size() - 1;
    if (level < 0) return NULL;
    return &_callStack[level]->GetVariables();
  }
  GameVarSpace *GetVariables(int index)
  {
    if (index < 0) return NULL;
    return &_callStack[index]->GetVariables();
  }
  RString GetDisplayName() const
  {
    if (_displayName.GetLength() > 0) return _displayName;
    return _doc._filename;
  }
  bool SetScopeName(RString name)
  {
    int level = _callStack.Size() - 1;
    if (level < 0) return true;
    return _callStack[level]->SetScopeName(name);
  }

  void Clear() {_callStack.Clear();}
  int AddLevel(CallStackItem *level);
  int FindLevel(const CallStackItem *level) const
  {
    for (int i=0; i<_callStack.Size(); i++)
    {
      if (_callStack[i] == level) return i;
    }
    return -1;
  }

  /// enable/disabled undefined variable reporting
  void ReportUndefinedVariables(bool report) {_undefinedIsNil = report;}

  void ThrowException(GameValuePar value)
  {
    _exceptionThrown = true;
    _exceptionValue = value;
  }
  void ThrowBreak(bool breakOut, GameValuePar breakValue = GameValue(), RString breakScope = RString())
  {
    _break = true;
    _breakOut = breakOut;
    _breakScope = breakScope;
    _breakValue = breakValue;
  }
  
  bool EvaluateCore(const GameState &state, int minVarLevel = 0, IDebugScript *script = NULL, const IEvalInterrupt &funcInterrupt = EvalDoNotInterrupt());
#define _DEBUG_GS_CONTEXT 0
#if _DEBUG_GS_CONTEXT
  bool EvaluateCore2(const GameState &state, int minVarLevel = 0, IDebugScript *script = NULL, const IEvalInterrupt &funcInterrupt = EvalDoNotInterrupt());
#endif

  void CheckScriptCallback(RString expression)
  {
    if (_callback) (*_callback)(expression);
  }

  /// serialization of contained variables is enabled
  bool IsSerializationEnabled() const {return _enabledSerialization;}
  /// disable serialization of contained variables
  void DisableSerialization();
  LSError Serialize(ParamArchive &ar);

#if SCRIPTVM_DEBUGGING
  void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  void RemoveBreakpoint(DWORD bp);
  void EnableBreakpoint(DWORD bp, bool enable);

  DebugState GetDebugState() const {return _debugState;}
  void SetDebugState(DebugState state)
  {
    // _debugState is aligned, so calling this from other process should be safe
    _debugState = state;
  }
  void SetStep(StepKind kind, StepUnit unit)
  {
    _stepKind = kind;
    _stepUnit = unit;
    _stepLevel = INT_MAX;
  }
#endif
};

/// Interface for single instruction
class GameInstruction : public RefCount
{
protected:
  // Position in source document
  SourceDocPos _srcPos;
#if SCRIPTVM_DEBUGGING
  VMBreakpoint _breakpoint;
#endif

public:
  GameInstruction(const SourceDocPos &srcPos)
    : _srcPos(srcPos) {}
  // returns true if exit whole scope is needed
  virtual bool Execute(const GameState &state, VMContext &ctx) = 0;
  /// how data stack changes after the instruction is processed
  virtual int GetStackChange(VMContext &ctx) const = 0;
  // returns position in source code
  const SourceDocPos &GetSourcePos() const {return _srcPos;}
  virtual bool IsNewExpression() const {return false;}
  /// text of the instruction (for disassembly, profiling etc.)
  virtual RString GetDebugName() const = 0;
  
  bool operator != (const GameInstruction &w) const;
  
#if SCRIPTVM_DEBUGGING
  virtual bool CanStep(StepUnit unit) const;

  virtual bool SetBreakpoint(const VMBreakpointInfo &breakpoint);
  bool RemoveBreakpoint(DWORD bp);
  bool EnableBreakpoint(DWORD bp, bool enable);

  // check if breakpoint matched
  DWORD IsBreakpointFired() const
  {
    if (!_breakpoint._enabled) return 0;
    return _breakpoint._id;
  }
#endif
};

#include <Es/Memory/normalNew.hpp>

class GameInstructionConst : public GameInstruction
{
protected:
  GameValue _value;

public:
  GameInstructionConst(const SourceDocPos &srcPos, const GameValue &value)
    : GameInstruction(srcPos), _value(value) {}
  bool Execute(const GameState &state, VMContext &ctx);
  // 0 arguments, 1 result
  int GetStackChange(VMContext &ctx) const {return +1;}

  RString GetDebugName() const {return RString("const ") + _value.GetText();}

//   virtual bool operator != (const GameInstruction &w) const
//   {
//     // left is known to be const, not dispatch based on right
//     if (dynamic_cast<GameInstructionConst *>(&w))
//   }
  
#if SCRIPTVM_DEBUGGING
  virtual bool SetBreakpoint(const VMBreakpointInfo &breakpoint);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameInstructionVariable : public GameInstruction
{
protected:
  RString _name;

public:
  GameInstructionVariable(const SourceDocPos &srcPos, RString name)
    : GameInstruction(srcPos), _name(name) {}
  bool Execute(const GameState &state, VMContext &ctx);
  // 0 arguments, 1 result
  int GetStackChange(VMContext &ctx) const {return +1;}

  RString GetDebugName() const {return RString("var ") + _name;}

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameInstructionOperator : public GameInstruction
{
protected:
  const GameOperators *_operators;

public:
  GameInstructionOperator(const SourceDocPos &srcPos, const GameOperators *operators)
    : GameInstruction(srcPos), _operators(operators) {}
  bool Execute(const GameState &state, VMContext &ctx);
  // 2 arguments, 1 result
  int GetStackChange(VMContext &ctx) const {return -1;}

  RString GetDebugName() const {return RString("operator ") + _operators->_name;}

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameInstructionFunction : public GameInstruction
{
protected:
  const GameFunctions *_functions;

public:
  GameInstructionFunction(const SourceDocPos &srcPos, const GameFunctions *functions)
    : GameInstruction(srcPos), _functions(functions) {}
  bool Execute(const GameState &state, VMContext &ctx);
  // 1 arguments, 1 result
  int GetStackChange(VMContext &ctx) const {return 0;}

  RString GetDebugName() const {return RString("function ") + _functions->_name;}

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameInstructionArray : public GameInstruction
{
protected:
  int _size;

public:
  GameInstructionArray(const SourceDocPos &srcPos, int size)
    : GameInstruction(srcPos), _size(size) {}
  bool Execute(const GameState &state, VMContext &ctx);
  // _size arguments, 1 result
  int GetStackChange(VMContext &ctx) const {return -_size + 1;}

  RString GetDebugName() const {return Format("array (%d items)", _size);}

#if SCRIPTVM_DEBUGGING
  virtual bool SetBreakpoint(const VMBreakpointInfo &breakpoint);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameInstructionAssignment : public GameInstruction
{
protected:
  // variable name
  RString _name;
  bool _forceLocal;

public:
  GameInstructionAssignment(const SourceDocPos &srcPos, RString name, bool forceLocal)
    : GameInstruction(srcPos), _name(name),_forceLocal(forceLocal) {}
  bool Execute(const GameState &state, VMContext &ctx);
  // 1 argument, no result
  int GetStackChange(VMContext &ctx) const {return -1;}

  RString GetDebugName() const {return Format("%s = ...%s", cc_cast(_name),_forceLocal ? " (local)" : "");}

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameInstructionNewExpression : public GameInstruction
{
  // position into the string: beginning and end
  int _expressionBeg,_expressionEnd;

public:
  GameInstructionNewExpression(const SourceDocPos &srcPos)
    : GameInstruction(srcPos),_expressionBeg(0),_expressionEnd(0) {}
  bool Execute(const GameState &state, VMContext &ctx);
  // need to be calculated
  int GetStackChange(VMContext &ctx) const;
  virtual bool IsNewExpression() const {return true;}

  RString GetDebugName() const {return RString("new command");}

#if SCRIPTVM_DEBUGGING
  virtual bool CanStep(StepUnit unit) const;
#endif

  void SetSourceRange(int beg, int end){_expressionBeg=beg,_expressionEnd=end;}
  int GetSourceBeg() const {return _expressionBeg;}
  int GetSourceEnd() const {return _expressionEnd;}

  USE_FAST_ALLOCATOR_EXPRESS
};

#include <Es/Memory/debugNew.hpp>

typedef RefArray<GameInstruction> CompiledExpression;

struct GameCodeType
{
  GameStringType _string;
  CompiledExpression _code;
  bool _compiled;
};

#include <Es/Memory/normalNew.hpp>

class GameDataCode : public GameData
{
  typedef GameData base;

  GameCodeType _value;

public:
  GameDataCode() {_value._compiled= false;}
  GameDataCode(const GameCodeType &value) : _value(value) {}
  GameDataCode(const GameState *state, SourceDoc &src, SourceDocPos &pos, GameDataNamespace *globals);
  GameDataCode(const GameState *state, RString expression, GameDataNamespace *globals);
  ~GameDataCode(){}

  const GameType &GetType() const;
  GameStringType GetString() const {return _value._string;}
  const CompiledExpression &GetCode() const {return _value._code;}
  bool IsCompiled() const {return _value._compiled;}
  bool IsEmpty() const {return _value._compiled ? _value._code.Size()==0 : _value._string.IsEmpty();}
  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "code";}
  GameData *Clone() const {return new GameDataCode(*this);}

  jobject ToJObject(JNIEnv *env, GameValuePar from) const;

#ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
#endif

#if SCRIPTVM_DEBUGGING
  bool SetBreakpoint(const VMBreakpointInfo &breakpoint);
  bool RemoveBreakpoint(DWORD bp);
  bool EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - simple code
class CallStackItemSimple : public CallStackItem
{
protected:
  CompiledExpression _code; // compiled code
  int _ip; // instruction pointer

  SourceDoc _content;
  bool _multiple;

public:
  CallStackItemSimple(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Frame", enableSerialization) {} // for serialization
  CallStackItemSimple(GameDataNamespace *globalVars, CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, const SourceDoc &content, bool multiple, bool enableSerialization);
  CallStackItemSimple(GameDataNamespace *globalVars, CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, const SourceDoc &content, const CompiledExpression &compiled, bool enableSerialization);
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

  bool IsDifferent(const CallStackItemSimple &w) const;
  
#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - simple code encapsulated to GameDataCode
class CallStackItemData : public CallStackItem
{
protected:
  Ref<GameDataCode> _code; // compiled code
  int _ip; // instruction pointer

public:
  CallStackItemData(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Frame", enableSerialization) {} // for serialization
  CallStackItemData(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, GameDataCode *code, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, name, enableSerialization)
  {_ip = 0; _code = code;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - code argument of OR, AND expressions
// Warning: removes 2 arguments from the stack
class CallStackItemDataBool : public CallStackItemData
{
public:
  CallStackItemDataBool(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItemData(parent, parentVars, stackBottom, state, enableSerialization) {} // for serialization
  CallStackItemDataBool(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, GameDataCode *code, bool enableSerialization)
    : CallStackItemData(parent, parentVars, stackBottom, state, name, code, enableSerialization) {}

  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);
  virtual RString GetTypeName() const;

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - with do command
class CallStackItemWith : public CallStackItemData
{
protected:
  const GameState *_state;
  /// namespace passed to with (not GameDataNamespace because of serialization)
  Ref<GameData> _namespace;
  /// backup of the original namespace
  Ref<GameDataNamespace> _oldNamespace;

public:
  CallStackItemWith(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItemData(parent, parentVars, stackBottom, state, enableSerialization) {} // for serialization
  CallStackItemWith(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, RString name, GameDataNamespace *ns, GameDataCode *code, bool enableSerialization);
  ~CallStackItemWith();

  virtual LSError Serialize(ParamArchive &ar);
  virtual RString GetTypeName() const;

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - while do command
class CallStackItemRepeat : public CallStackItem
{
protected:
  enum Phase
  {
    PBegin,
    PAfterCondition,
    PAfterStatement,
    PDone // used for break
  };

  Ref<GameDataCode> _condition; // compiled code
  Ref<GameDataCode> _statement; // compiled code
  Phase _phase;
  int _iter, _maxIters;

public:
  CallStackItemRepeat(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "While command", enableSerialization) {} // for serialization
  CallStackItemRepeat(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    GameDataCode *condition, GameDataCode *statement, int maxIters, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "While command", enableSerialization)
  {_phase = PBegin; _iter = 0; _condition = condition; _statement = statement; _maxIters = maxIters;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);
  virtual bool OnBreak(const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - count command
class CallStackItemArrayCount : public CallStackItem
{
protected:
  Ref<GameDataCode> _code; // compiled code
  Ref<GameDataArray> _array;
  int _i, _result;
  bool _breaked;

public:
  CallStackItemArrayCount(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Count command", enableSerialization) {} // for serialization
  CallStackItemArrayCount(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    GameDataCode *code, GameDataArray *array, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Count command", enableSerialization)
  {_i = 0; _result = 0; _code = code; _array = array; _breaked = false;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);
  virtual bool OnBreak(const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - forEach command
class CallStackItemArrayForEach : public CallStackItem
{
protected:
  Ref<GameDataCode> _code; // compiled code
  Ref<GameDataArray> _array;
  int _i;
  bool _breaked;

public:
  CallStackItemArrayForEach(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "ForEach command", enableSerialization) {} // for serialization
  CallStackItemArrayForEach(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    GameDataCode *code, GameDataArray *array, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "ForEach command", enableSerialization)
  {_i = 0; _code = code; _array = array; _breaked = false;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);
  virtual bool OnBreak(const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - BASIC form of for command (for "var" from 10 to 1 step -1 do {...})
class CallStackItemForBASIC : public CallStackItem
{
protected:
  RString _varName;
  float _varValue;
  float _to;
  float _step;
  Ref<GameDataCode> _code; // compiled code
  bool _passed;
  bool _breaked;

public:
  CallStackItemForBASIC(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "For command (BASIC)", enableSerialization) {} // for serialization
  CallStackItemForBASIC(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    RString varName, float from, float to, float step, GameDataCode *code, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "For command (BASIC)", enableSerialization)
  {_varName = varName; _varName.Lower(); _varValue = from; _to = to; _step = step; _code = code; _passed = false; _breaked = false;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);
  virtual bool OnBreak(const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - C form of for command (for [{init}, {cond}, {update}] do {...})
class CallStackItemForC : public CallStackItem
{
protected:
  enum Phase
  {
    PInit,
    PAfterCondition,
    PAfterStatement,
    PAfterUpdate,
    PDone // used for break
  };

  Ref<GameDataCode> _init; // compiled code
  Ref<GameDataCode> _condition; // compiled code
  Ref<GameDataCode> _update; // compiled code
  Ref<GameDataCode> _code; // compiled code

  Phase _phase;
  int _ip; // instruction pointer for _init

public:
  CallStackItemForC(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "For command (C)", enableSerialization) {} // for serialization
  CallStackItemForC(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    GameDataCode *init, GameDataCode *condition, GameDataCode *update, GameDataCode *code, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "For command (C)", enableSerialization)
  {_init = init; _condition = condition; _update = update; _code = code; _phase = PInit; _ip = 0;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);
  virtual bool OnBreak(const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - switch command
class CallStackItemSwitch : public CallStackItem
{
protected:
  enum Phase
  {
    PInit,
    PAfterStatement,
    PDone
  };

  GameValue _switch;
  Ref<GameDataCode> _code; // compiled code

  Phase _phase;

public:
  CallStackItemSwitch(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Switch command", enableSerialization) {} // for serialization
  CallStackItemSwitch(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    const GameValue &sw, GameDataCode *code, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Switch command", enableSerialization)
  {_switch = sw; _code = code; _phase = PInit;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - try catch command
class CallStackItemTry : public CallStackItem
{
protected:
  enum Phase
  {
    PInit,
    PAfterTry,
    PDone
  };

  Ref<GameDataCode> _tryCode; // compiled code
  Ref<GameDataCode> _catchCode; // compiled code

  Phase _phase;

public:
  CallStackItemTry(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Try command", enableSerialization) {} // for serialization
  CallStackItemTry(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    GameDataCode *tryCode, GameDataCode *catchCode, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "Try command", enableSerialization)
  {_tryCode = tryCode; _catchCode = catchCode; _phase = PInit;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);
  virtual bool OnException(const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};

/// Call stack level - exitWith command
class CallStackItemExitWith : public CallStackItem
{
protected:
  enum Phase
  {
    PInit,
    PDone
  };

  Ref<GameDataCode> _code; // compiled code
  Phase _phase;

public:
  CallStackItemExitWith(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "ExitWith command", enableSerialization) {} // for serialization
  CallStackItemExitWith(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    GameDataCode *code, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "ExitWith command", enableSerialization)
  {_code = code; _phase = PInit;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};



/// Call stack level - waitUntil command
class CallStackItemWaitUntil : public CallStackItem
{
protected:
  enum Phase
  {
    PInit,
    PAfterCondition
  };

  Ref<GameDataCode> _condition; // compiled code
  Phase _phase;

public:
  CallStackItemWaitUntil(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "WaitUntil command", enableSerialization) {} // for serialization
  CallStackItemWaitUntil(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    GameDataCode *condition, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "WaitUntil command", enableSerialization)
  {_condition = condition; _phase = PInit;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};



enum NativeEvent
{
  NEInitScript,
  NNativeEvent,
};

class CallStackItemWaitUntilNative : public CallStackItem
{
public:
protected:

  NativeEvent _condition; // compiled code

public:
  CallStackItemWaitUntilNative(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "WaitUntil command", enableSerialization) {} // for serialization
  CallStackItemWaitUntilNative(CallStackItem *parent, GameVarSpace *parentVars, int stackBottom, const GameState *state,
    NativeEvent condition, bool enableSerialization)
    : CallStackItem(parent, parentVars, stackBottom, state, "WaitUntil command", enableSerialization)
  {_condition = condition;}
  virtual GameInstruction *GetNextInstruction(RequiredExecutionControl &rec, const GameState *state);

  virtual LSError Serialize(ParamArchive &ar);

  virtual RString GetTypeName() const;

#if SCRIPTVM_DEBUGGING
  virtual void SetBreakpoint(const VMBreakpointInfo &breakpoint){}
  virtual void RemoveBreakpoint(DWORD bp){}
  virtual void EnableBreakpoint(DWORD bp, bool enable){}
#endif

  USE_FAST_ALLOCATOR_EXPRESS
};


#include <Es/Memory/debugNew.hpp>

typedef CallStackItem *CreateCallStackItemFunction(CallStackItem *parent, GameVarSpace *parentVars, const GameState *state, bool enableSerialization);

struct CallStackItemType
{
  CreateCallStackItemFunction *createFunction;
  RString typeName;

  CallStackItemType(){createFunction=NULL;}
  CallStackItemType(CreateCallStackItemFunction *func, RString name)
  {
    createFunction = func;
    typeName = name;
  }
};
TypeIsMovableZeroed(CallStackItemType)

// macros for call stack items registration
#define DEFINE_CALL_STACK_ITEM(name, type) \
  RString type::GetTypeName() const {return name;} \
  CallStackItem *Create##type(CallStackItem *parent, GameVarSpace *parentVars, const GameState *state, bool enableSerialization) {return new type(parent, parentVars, 0, state, enableSerialization);}

#define REGISTER_CALL_STACK_ITEM(name, type) \
  CallStackItemType(&Create##type, name),

#endif

#include <Es/Memory/normalNew.hpp>

class GameEvaluator: public RefCount
{
  friend class GameState;
  friend class GameInstructionVariable;
  friend class GameInstructionOperator;
  friend class GameInstructionFunction;
  friend class GameInstructionAssignment;
  friend class GameInstructionNewExpression;

  GameVarSpace *local;  // local variables
  int           handle; // for debug purposes to test the Begin/EndContext matching pairs
  
  bool _checkOnly;
  bool _isUIcontext;
  EvalError _error;
  RString _errorText; // user friendly error text
#if _DEBUG_GS_CONTEXT
  DWORD callStackRET;  // remember the caller address ([ebp+40]) (used to debug BeginContext)
#endif

#if USE_PRECOMPILATION
  SourceDocPos _errorPos; // error position during compilation
#else //USE_PRECOMPILATION
  const char *_pos,*_pos0;
  // offsets of subexpression
  int _subexpBeg;
  GameValue _stack[evalStackLevels]; // value stack
  int UB[evalStackLevels]; // unary/binary flag
  int _priorStack[evalStackLevels]; // prioriry stack
  RString _operStack[evalStackLevels];
  int SP; // stack pointer
  int _parPrior; // parenthesis priority
  int _listPrior; // parenthesis priority
  int _errorCarretPos;
#endif
  
  GameEvaluator( GameVarSpace *vars=NULL );
  ~GameEvaluator();

#ifdef _SCRIPT_DEBUGGER
    friend class IScriptDebugger;
#endif

#if USE_PRECOMPILATION
  USE_FAST_ALLOCATOR_EXPRESS
#endif
};

#include <Es/Memory/debugNew.hpp>

#if DOCUMENT_COMREF
struct ComRefType
{
  GameType type;
  RString name;
  RString description;
  RString category;

  ComRefType(const GameType &t, RString n, RString d, RString c)
  {
    type = t;
    name = n;
    description = d;
    category = c;
  }
};
TypeIsMovableZeroed(ComRefType)

struct ComRefArray
{
  RString name;
  RString format;
  RString description;
  RString category;

  ComRefArray(RString n, RString f, RString d, RString c)
  {
    name = n;
    format = f;
    description = d;
    category = c;
  }
};
TypeIsMovableZeroed(ComRefArray)

struct ComRefFunc
{
  GameType returnType;
  RString name;
  RString priority;
  RString func;
  GameType loperType;
  GameType roperType;
  RString loper;
  RString roper;
  RString description;
  RString example;
  RString exampleResult;
  RString since;
  RString changed;
  RString category;

  ComRefFunc
  (
    const GameType &rT, RString n, RString p, RString f, const GameType &loT, const GameType &roT,
    RString lo, RString ro, RString d, RString e, RString eR,
    RString s, RString ch, RString c
  )
  {
    returnType = rT;
    name = n;
    priority = p;
    func = f;
    loperType = loT;
    roperType = roT;
    loper = lo;
    roper = ro;
    description = d;
    example = e;
    exampleResult = eR;
    since = s;
    changed = ch;
    category = c;
  }
};
TypeIsMovableZeroed(ComRefFunc)
#endif

#ifdef _SCRIPT_DEBUGGER
class IScriptDebugger;
#endif

static GameValue supportInfo(const GameState *state, GameValuePar oper1);

class GameDataException;

class GameState
{
  ///Function gets information about installed addons. It needs the full access into gamestate
  friend GameValue supportInfo(const GameState *state, GameValuePar oper1);
  friend class GameInstructionVariable;
  friend class GameInstructionOperator;
  friend class GameInstructionFunction;
  friend class GameInstructionAssignment;
  friend class GameInstructionNewExpression;

private:
  /// registered scripting types
  AutoArray<const GameTypeType *> _typeNames;
  int _numBasicTypes;

  GameFunctionsType _functions;
  GameOperatorsType _operators;
  GameNularsType _nulars;

#if _DEBUG_GS_CONTEXT
  // make it public to ease the debug purpose implementation
  public:
#endif 

  mutable RefArray<GameEvaluator, MemAllocLocal<Ref<GameEvaluator>,64> > _contextStack;

#if _DEBUG_GS_CONTEXT
  private:
#endif 

  // evaluator temporary storage
  mutable GameEvaluator *_e;

  // current global variable space
  // TODO: stack of namespaces
  mutable Ref<GameDataNamespace> _globals;

  // registered statically created namespaces, needed for serialization (exist during the whole app live, so need not stronger reference)
  AutoArray<GameDataNamespace *> _staticNamespaces;

  // recovery when no global namespace is given is enabled
  bool _defaultGlobalsEnabled;

  /// show evaluator errors on-screen
  bool _showScriptErrors;

#if !_SUPER_RELEASE
  /// profiling of expression evaluation is enabled (works only for precompiled expressions)
  bool _profilingEnabled;
#endif

#if USE_PRECOMPILATION
  /// temporary storage for evaluation context - to enable access to context for Game functions without changing their declaration
  mutable VMContext *_context;
  AutoArray<CallStackItemType> _callStackItems;
#endif

  static LocalizeStringFunctions *_defaultLocalizeFunctions;
  GameDataException *_exceptionHandler;  ///<Current exception handler for throw function

public:
  // Access for registration in Java
  const AutoArray<const GameTypeType *> &GetTypes() const {return _typeNames;}
  const GameFunctionsType &GetFunctions() const {return _functions;}
  const GameOperatorsType &GetOperators() const {return _operators;}
  const GameNularsType &GetNulars() const {return _nulars;}

  void NewNularOp(const GameNular &f);
  void NewFunction(const GameFunction &f);
  void NewOperator(const GameOperator &f);

  void NewNularOps(const GameNular *f, int count);
  void NewFunctions(const GameFunction *f, int count);
  void NewOperators(const GameOperator *f, int count);

  //! append all nular op names passed by filter to given string array
  void AppendNularOpList
  (
    AutoArray<RStringS> &dic, bool (*filter)(const char *word)
  ) const;
  //! append all function names passed by filter to given string array
  void AppendFunctionList
  (
    AutoArray<RStringS> &dic, bool (*filter)(const char *word)
  ) const;
  //! append all operator names passed by filter to given string array
  void AppendOperatorList
  (
    AutoArray<RStringS> &dic, bool (*filter)(const char *word)
  ) const;

  /// register a scripting type
  void NewType(const GameTypeType *type);

  bool IsUIContext() const 
  { 
    // the code is run as a result of some UI Control Event (VMScript of EvaluateContext knows)
    if (GetVMContext()) return ( GetVMContext()->_isUIContext || _e->_isUIcontext );
    else return _e->_isUIcontext; 
  }

#if _ENABLE_COMREF
  /// access to documentation
  const GameNular *FindNular(RString name) const;
  /// access to documentation
  const GameFunctions *FindFunctions(RString name) const;
  /// access to documentation
  const GameOperators *FindOperators(RString name) const;
#endif
  /// find the type with given name
  const GameTypeType *FindType(const char* name) const;
  int FindTypeIndex(const GameTypeType* type) const;
  bool IsBasicType(int index) const { return index < _numBasicTypes; }
  const GameTypeType* GetType(int index) const { return _typeNames[index]; }

#if DOCUMENT_COMREF
protected:
  AutoArray<ComRefType> _comRefType;
  AutoArray<ComRefArray> _comRefArray;
  AutoArray<ComRefFunc> _comRefFunc;

public:
  void AddComRefType(const ComRefType &type) {_comRefType.Add(type);}
  void AddComRefArray(const ComRefArray &array) {_comRefArray.Add(array);}
  void AddComRefFunction(const ComRefFunc &func) {_comRefFunc.Add(func);}
  void AddComRefTypes(const ComRefType *type, size_t len) {for (size_t i=0;i<len;i++) _comRefType.Add(type[i]);}
  void AddComRefArrays(const ComRefArray *array, size_t len) {for (size_t i=0;i<len;i++) _comRefArray.Add(array[i]);}
  void AddComRefFunctions(const ComRefFunc *func, size_t len) {for (size_t i=0;i<len;i++) _comRefFunc.Add(func[i]);}
  void ClearComRefFunctions() {_comRefFunc.Clear();}
  void ClearComRefArrays() {_comRefArray.Clear();}
  void ClearComRefTypes() {_comRefType.Clear();}
  bool ComRefDocument(const char *file="c:\\comref.xml");
  bool GenerateUltraeditWordFile(const char *targetfile, const char *langName, const char *extensionList, bool preprocessor=true);

protected:
  RString ComRefTypeName(const GameType &type) const;

#endif

public:
  GameState();
  ~GameState();

  void ShowScriptErrors(bool set) {_showScriptErrors = set;}

  /// check if default global namespace is enabled
  bool AreDefaultGlobalsEnabled() const {return _defaultGlobalsEnabled;}

  /// mark the evaluator to global namespace need to be passed explicitly
  void DisableDefaultGlobals() {_defaultGlobalsEnabled = false;}

#if !_SUPER_RELEASE
  bool IsProfilingEnabled() const {return _profilingEnabled;}
  void EnableProfiling(bool enable) {_profilingEnabled = enable;}
#endif

  void Init();
  void Compact();
  void Reset();

  GameDataNamespace *GetGlobalVariables() const {return _globals;}
  GameDataNamespace *SetGlobalVariables(GameDataNamespace *globals) const
  {
    GameDataNamespace *oldGlobals = _globals;
    _globals = globals;
    return oldGlobals;
  }

  void RegisterStaticNamespace(GameDataNamespace *ns) {_staticNamespaces.Add(ns);}
  GameDataNamespace *FindStaticNamespace(RString name);

  bool IdtfGoodName( const char *name ) const;  
  bool VarGoodName( const char *name ) const;
  bool LValueGoodName(const char *name, const GameDataNamespace *globals = NULL) const;
  GameValue VarGet(const char *name, bool errorWhenUndefined = false, const GameDataNamespace *globals = NULL, bool localsOnly = false) const;
  GameValue VarGetLocal(const char *name, bool errorWhenUndefined = false) const
  {
    return VarGet(name, errorWhenUndefined, NULL, true);
  }
  bool VarReadOnly(const char *name, const GameDataNamespace *globals = NULL) const;
  //! Creates a variable and assigns it a value
  /*!
    If the name of the variable starts with the _ char, then it will be local
    for the script.
    \param name Name of the value
    \param value Initial value of the variable
    \param readOnly Variable is read only if true
    \param forceLocal Variable is local even if its name doesn't start with the _ char, if true
  */
  void VarSet(const char *name, GameValuePar value, bool readOnly = false, bool forceLocal = false, GameDataNamespace *globals = NULL);
  void VarSetLocal(const char *name, GameValuePar value, bool readOnly=false, bool forceLocal=false) const;
  //! create undefined variable in the innermost scope
  void VarLocal( const char *name);
  void VarDelete(const char *name, GameDataNamespace *globals = NULL);

  bool IsSwitchType(const GameType &type) const;
  bool IsFakeType(const GameType &type) const;

  RString GetInternalTypeName(const GameType &type) const;

  bool IsVisible( const GameVariable &var ) const;
  
  // runtime errors
  void SetError(EvalError error, ...) const;
  void TypeError(const GameType &exp, const GameType &was, const char *name = NULL ) const;
  // report argument types incompatible
  void TypeErrorOperator(const char *name, const GameType &left, const GameType &right) const;
  void TypeErrorFunction(const char *name, const GameType &right) const;
  // report change of World state on client
  void AccessError(const char *name) const;

  // debug function
  const char *GetExpressionText() const;

  EvalError GetLastError() const;
  RString GetLastErrorText() const;
  int GetLastErrorPos(RString expression) const;

  // evaluate an expression
  //! get actual context (started by BeginContext)
  GameVarSpace *GetContext() const;

  //! start context with given local variable space
  int BeginContext( GameVarSpace *vars, bool isUIContext=false ) const;
  //! close context
  void EndContext( int gshandle ) const;

#if !USE_PRECOMPILATION
  GameVarSpace *GetContext(int index) const
  {
    if (index<0 || index>=_contextStack.Size() || _contextStack[index].IsNull()) return NULL;
    return _contextStack[index]->local;
  }

  int FindContext(const char *scopeName) const;
#endif

#if USE_PRECOMPILATION
  // runtime errors (position in source file is passed)
  void SetError(const SourceDocPos &pos, EvalError error, ...) const;
  void TypeError(const SourceDocPos &pos, const GameType &exp, const GameType &was, const char *name = NULL ) const;
  // report argument types incompatible
  void TypeErrorOperator(const SourceDocPos &pos, const char *name, const GameType &left, const GameType &right) const;
  void TypeErrorFunction(const SourceDocPos &pos, const char *name, const GameType &right) const;

  VMContext *GetVMContext() const {return _context;}
  VMContext *SetVMContext(VMContext *context) const
  {
    VMContext *oldContext = _context;
    _context = context;
    return oldContext;
  }
#endif

  struct EvaluateTerm
  {
    char *terminationBy;
  };

  /// context (arguments) common to all Evaluate/Execute functions
  struct EvalContext
  {
    bool _localOnly;
    bool _undefinedIsNil;

    EvalContext()
    {
      _localOnly = false;
      _undefinedIsNil = true;
    }
    EvalContext(bool localOnly, bool undefinedIsNil=true)
    :_localOnly(localOnly),
    _undefinedIsNil(undefinedIsNil)
    {

    }

    static const EvalContext _default;
    static const EvalContext _reportUndefined;
  };
  // Common evaluation
  GameValue Evaluate(const char *expression, const EvalContext &ctx=EvalContext::_default, GameDataNamespace *globals = NULL) const;
  GameValue EvaluateMultiple(const char *expression, const EvalContext &ctx = EvalContext::_default, GameDataNamespace *globals = NULL) const;
  bool EvaluateMultipleBool(const char *expression, const EvalContext &ctx = EvalContext::_default, GameDataNamespace *globals = NULL) const;
  bool EvaluateBool(const char *expression, const EvalContext &ctx = EvalContext::_default, GameDataNamespace *globals = NULL) const;
  //! execute command or list of commands
  void Execute(const char *expression, const EvalContext &ctx = EvalContext::_default, GameDataNamespace *globals = NULL);

#if USE_PRECOMPILATION
  // Compiler evaluation
protected:
  void CompileConstants(SourceDoc &src, SourceDocPos &pos, CompiledExpression &polishNotation) const;
  void CompileBinaryOperator(SourceDoc &src, SourceDocPos &pos, CompiledExpression &polishNotation, GamePriority priority, const GameOperators *&operators) const;
  bool CompileAssignment(SourceDoc &src, SourceDocPos &pos, CompiledExpression &compiled);

public:
  /// compile single expression from a string
  void Compile(RString express, CompiledExpression &compiled, GameDataNamespace *globals = NULL) const
  {
    SourceDoc doc(RString(),express);
    SourceDocPos pos(doc);
    Compile(doc, pos, compiled, globals);
  }
  /// compile multiple expressions from a string
  void CompileMultiple(RString express, CompiledExpression &compiled, GameDataNamespace *globals = NULL) const
  {
    SourceDoc doc(RString(),express);
    SourceDocPos pos(doc);
    CompileMultiple(doc, pos, compiled, false, globals);
  }
  
  /// compile single expression
  void Compile(SourceDoc &src, SourceDocPos &pos, CompiledExpression &compiled, GameDataNamespace *globals = NULL) const;
  /// compile multiple expressions
  void CompileMultiple(SourceDoc &src, SourceDocPos &pos, CompiledExpression &compiled, bool inner = false, GameDataNamespace *globals = NULL) const;

  /// evaluate compiled expression (generic return type)
  GameValue Evaluate(RString expression, const CompiledExpression &compiled, const EvalContext &ctx = EvalContext::_default, GameDataNamespace *globals = NULL, RString expressionName = RString()) const;
  /// evaluate compiled expression (return type bool)
  bool EvaluateBool(RString expression, const CompiledExpression &compiled, const EvalContext &ctx = EvalContext::_default, GameDataNamespace *globals = NULL, RString expressionName = RString()) const;
  /// execute compiled expression (return type NOTHING)
  void Execute(RString expression, const CompiledExpression &compiled, const EvalContext &ctx = EvalContext::_default, GameDataNamespace *globals = NULL, RString expressionName = RString());

  /// helper function for serialization of VMScript
  CallStackItem *CreateCallStackItem(RString typeName, int parent, bool enableSerialization);
  void NewCallStackItemType(const CallStackItemType &type) {_callStackItems.Add(type);}

#else
  // Interpreter evaluation
protected:
  void VyhCast( int Prio ) const; // evaluate to priority
  GameValue Const() const; // term evaluation
  Real sejmid() const;
  void vynech() const;

  GameValue Vyhod() const; // partial evaluation
  void CleanStack() const;

  //! execute one assignment command
  bool PerformAssignment(bool localOnly = false);
public:
#endif

  void ShowError() const;
  
  // check syntax
  bool CheckEvaluate(const char *expression, GameDataNamespace *globals = NULL) const;
  bool CheckEvaluateBool(const char *expression, GameDataNamespace *globals = NULL) const;
  bool CheckExecute(const char *expression, GameDataNamespace *globals = NULL) const;

  // serialization support

  virtual GameData *CreateGameData(const GameType &type, ParamArchive *ar) const;
  virtual GameValue CreateGameValue(const GameType &type) const;
  virtual RString GetTypeName(const GameType &type) const;

  // Function returns maximally allowed number of iteration for one "for", "while" etc. command
  virtual int MaxIterations() const {return 10000;};

  ///Called when Echo command is used
  virtual void OnEcho(const char *text) const {LogF(text);};

#if !USE_PRECOMPILATION
  ///Function returns pointer to current exception handler
  GameDataException *GetCurrentExceptionHandler() const {return _exceptionHandler;}
  ///Function sets current exception handler. Function returns previous exception handler
  GameDataException *SetCurrentExceptionHandler(GameDataException *exceptionHandler) 
  {
    GameDataException *old=_exceptionHandler;
    _exceptionHandler=exceptionHandler;
    return old;
  }
  ///Throws an exception
  /**
  Function changes state of object, that will cause to call catch blok after current command is processed.
  This function should be called inside each command, that can throw an exception/
  */
  bool ThrowException(GameValuePar exception);
#endif

  #ifdef _SCRIPT_DEBUGGER
protected: 
  mutable IScriptDebugger *_debugger;
public: 
  IScriptDebugger *AttachDebugger(IScriptDebugger *debugger=NULL)
  {
    IScriptDebugger *old=_debugger;
    _debugger=debugger;
    return old;
  }
  void DebuggerContext(IScriptDebugger *debugger) const ;
  void Halt() const;
  void Echo(const char *text) const;
  #endif
};

#if DOCUMENT_COMREF
#define COMREF_TYPE(name, type, createfunc, tname, comrefname, description, category, javaType, signature) ComRefType(type, comrefname, description, category),
#define COMREF_ARRAY(name, format, description, category) ComRefArray(name, format, description, category),
#define COMREF_OPERATOR(returnType, name, priority, func, loperType, roperType, loper, roper, description, example, exampleResult, since, changed, category) ComRefFunc(returnType, name, #priority, #func, loperType, roperType, loper, roper, description, example, exampleResult, since, changed, category),
#define COMREF_FUNCTION(returnType, name, func, roperType, roper, description, example, exampleResult, since, changed, category) ComRefFunc(returnType, name, "", #func, GameType(), roperType, "", roper, description, example, exampleResult, since, changed, category),
#define COMREF_NULAR(returnType, name, func, description, example, exampleResult, since, changed, category) ComRefFunc(returnType, name, "", #func, GameType(), GameType(), "", "", description, example, exampleResult, since, changed, category),
#endif

// global evaluator
extern GameState GGameState;
// the default global namespace (used in simple tools)
extern GameDataNamespace GDefaultNamespace;

#ifdef _SCRIPT_DEBUGGER
#include "IScriptDebugger.h"
#endif

// helper functions / definitions
inline bool isalphaext( char c )
{
  return isalpha((unsigned char)c) || c=='_';
}

inline bool isalnumext( char c )
{
  return isalnum((unsigned char)c) || c=='_';
}

#define OPEN_STRING '{'
#define CLOSE_STRING '}'

#define NOTHING GameValue()

#define MAX_EXPR_LEN 1024

const char *CheckAssignment(const char *p);
#define LOCAL_VAR_ASSIGN "local"
#define LOCAL_VAR_ASSIGN_SZ 5
const char *TestLocalAssign(const char *p);

#endif // _EXPRESS_IMPL_HPP
