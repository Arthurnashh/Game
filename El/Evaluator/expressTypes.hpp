#ifndef _EXPRESS_TYPES_HPP
#define _EXPRESS_TYPES_HPP

#include "jvmImpl.hpp"

/// list of basic types
#define TYPES_DEFAULT(XX, Category) \
  XX("SCALAR",GameScalar, CreateGameDataScalar, "@STR_EVAL_TYPESCALAR", "Number", "A real number.", Category, "float", "F") \
  XX("BOOL",GameBool, CreateGameDataBool, "@STR_EVAL_TYPEBOOL", "Boolean", "Boolean (<f>true</f> or <f>false</f>).", Category, "boolean", "Z") \
  XX("ARRAY",GameArray, CreateGameDataArray, "@STR_EVAL_TYPEARRAY", "Array", "An array of items, each may be of any type.", Category, "java.util.List", "Ljava/util/List;") \
  XX("STRING",GameString, CreateGameDataString, "@STR_EVAL_TYPESTRING", "String", "An ASCII string.", Category, "String", "Ljava/lang/String;") \
  XX("NOTHING",GameNothing, CreateGameDataNothing, "@STR_EVAL_TYPENOTHING", "Nothing", "Nothing - no value.", Category, "void", "V") \
  XX("ANY",GameAny, CreateGameDataAny, "@STR_EVAL_TYPEANY", "Any", "Anyhing - any value.", Category, "Object", "Ljava/lang/Object;") \
  XX("NAMESPACE",GameNamespace, CreateGameDataNamespace, "@STR_EVAL_TYPENAMESPACE", "Namespace", "Namespace - set of variables.", Category, "GameNamespace", "Lcom/bistudio/JNIScripting/RVEngine$GameNamespace;") \

/// list of combined types
#define TYPES_DEFAULT_COMB(XX, Category) \
  XX("",GameAny, NULL, "", "Anything", "Anything, including nothing.", Category, "Object", "Ljava/lang/Object;") \
  XX("",GameVoid, NULL, "", "Any Value", "Any value.", Category, "Object", "Ljava/lang/Object;") \
  XX("",GameString | GameArray, NULL, "", "String or Array", "<t>String</t> or <t>Array</t>.", Category, "Object", "Ljava/lang/Object;") \
  XX("",GameScalar | GameNothing, NULL, "", "Number or Nothing", "<t>Number</t> or <t>Nothing</t>.", Category, "Object", "Ljava/lang/Object;") \

#if USE_PRECOMPILATION
/// list of basic types
# define TYPES_CODE(XX, Category) \
  XX("CODE", GameCode, CreateGameDataCode, "code", "Code", "Part of code (compiled).", Category, "GameCode", "Lcom/bistudio/JNIScripting/RVEngine$GameCode;")

/// list of combined types
# define TYPES_CODE_COMB(XX, Category) \
  XX("",GameString | GameCode, NULL, "", "String or Code", "<t>String</t> or <t>Code</t>.", Category, "Object", "Ljava/lang/Object;")
#endif

TYPES_DEFAULT(DECLARE_TYPE, "Default")
#if USE_PRECOMPILATION
TYPES_CODE(DECLARE_TYPE, "Default")
#endif

// type (GameValue <-> Java) conversion for primitive types (Scalar, Bool, String, Nothing)
template<>
class ConvertTypes<&GTypeGameScalar>
{
public:
  typedef jfloat javaType;
  static javaType FromGameValue(JNIEnv *env, const GameValue &from)
  {
    return from; // use implicit conversion
  }
  static GameValue ToGameValue(JNIEnv *env, javaType from)
  {
    return GameValue(from);
  }
};
template<>
class ConvertTypes<&GTypeGameBool>
{
public:
  typedef jboolean javaType;
  static javaType FromGameValue(JNIEnv *env, const GameValue &from)
  {
    bool value = from; // use implicit conversion
    return value ? JNI_TRUE : JNI_FALSE;
  }
  static GameValue ToGameValue(JNIEnv *env, javaType from)
  {
    bool value = from != JNI_FALSE;
    return GameValue(value);
  }
};
template<>
class ConvertTypes<&GTypeGameString>
{
public:
  typedef jstring javaType;
  static javaType FromGameValue(JNIEnv *env, const GameValue &from)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    RString str = from; // use implicit conversion
    return env->NewStringUTF(str); 
  }
  static GameValue ToGameValue(JNIEnv *env, javaType from)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    int len = env->GetStringUTFLength(from);
    RString str;
    char *buffer = str.CreateBuffer(len + 1);
    env->GetStringUTFRegion(from, 0, len, buffer);
    return GameValue(str);
  }
};
template<>
class ConvertTypes<&GTypeGameNothing>
{
public:
  typedef void javaType;
  static javaType FromGameValue(JNIEnv *env, const GameValue &from)
  {
  }
  // void parameter not supported
};

// type (GameValue <-> Java) conversion for special types (Array, Any)
template<>
class ConvertTypes<&GTypeGameArray>
{
  static jweak _class;
  static jmethodID _sizeID;
  static jmethodID _getID;
  static jclass InitClass(JNIEnv *env)
  {
    // activate JVM FPU settings
    CHECK_FPU_JVM

    // check the cached class
    jclass result = (jclass)env->NewLocalRef(_class);
    if (result == NULL) 
    {
      // class is not valid, initialize and store
      result = env->FindClass("java/util/List");
      _class = env->NewWeakGlobalRef(result);
      _sizeID = env->GetMethodID(result, "size", "()I");
      _getID = env->GetMethodID(result, "get", "(I)Ljava/lang/Object;");
    }
    return result;
  }

public:
  typedef jobject javaType;
  static javaType FromGameValue(JNIEnv *env, const GameValue &from)
  {
    return from.ToJObject(env);
  }
  static GameValue ToGameValue(JNIEnv *env, javaType from)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    InitClass(env);

    int size = env->CallIntMethod(from, _sizeID);

    GameValue result = GGameState.CreateGameValue(GameArray);
    GameArrayType &array = result;
    array.Realloc(size);
    array.Resize(size);

    for (int i=0; i<size; i++)
    {
      jobject item = env->CallObjectMethod(from, _getID, i);
      array[i] = GameValueFromJObject(env, item);
    }

    return result;
  }
};
template<>
class ConvertTypes<&GTypeGameAny>
{
public:
  typedef jobject javaType;
  static javaType FromGameValue(JNIEnv *env, const GameValue &from)
  {
    return from.ToJObject(env);
  }
  static GameValue ToGameValue(JNIEnv *env, javaType from)
  {
    return GameValueFromJObject(env, from);
  }
};

#define GameVoid GameAny // any value type
#define GTypeGameVoid GTypeGameAny

#if !USE_PRECOMPILATION
const GameType GameCode(GameString);
#endif

#endif // _EXPRESS_TYPES_HPP

