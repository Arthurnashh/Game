// Java VM related functions
#include "wpch.hpp"
#include "jvmInit.hpp"

#include <Es/Framework/appFrame.hpp>
#include <El/Evaluator/express.hpp>
#include <El/Evaluator/jniFactory.hpp>
#include <El/Evaluator/jvmImpl.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/ParamArchive/paramArchiveDb.hpp>

#include "UI/missionDirs.hpp"
#include "saveVersion.hpp"

#include "vehicle.hpp"
#include "gameStateExt.hpp"

RString GetWinFontsDirectory();

static JavaVM *GJavaVM = NULL;
static RString GJRE;

bool IsJavaVMInitialized()
{
  return GJavaVM != NULL;
}

// Access to the JNI interface
JNIEnv *GetJNIEnv()
{
  if (GJavaVM == NULL)
  {
    LogF("Java VM not created");
    return NULL;
  }
  JNIEnv *env = NULL;
  CHECK_FPU_JVM
  GJavaVM->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (env == NULL)
  {
    LogF("Java VM not attached to this thread");
  }
  return env;
}

typedef jint JNICALL FuncCreateJavaVM(JavaVM **vm, void **penv, void *args);

// redirect log to the application output
static int JNICALL JVMPrintF(FILE *stream, const char *format, va_list arg)
{
  // activate Engine FPU settings
  SCOPE_FPU_ENGINE

#if _ENABLE_REPORT
  vaLogF(RString("JVM PRINT: ") + format, arg);
#endif
  return 0;
}
static void JNICALL JNI_LogImpl(JNIEnv *env, jobject obj, jstring message)
{
  RString str;
  int len = env->GetStringUTFLength(message);
  char *buffer = str.CreateBuffer(len + 1);
  env->GetStringUTFRegion(message, 0, len, buffer);

  // activate Engine FPU settings
  SCOPE_FPU_ENGINE

  LogF(str);
}

// destruction of native object
static void JNICALL JNI_DestroyPeer(JNIEnv *env, jobject obj, jlong peer)
{
  GameValue *ptr = (GameValue *)peer;
  if (ptr != NULL) delete ptr;
}

static jbyteArray JNICALL JNI_SavePeer(JNIEnv *env, jobject obj, jlong peer)
{
  GameValue *ptr = (GameValue *)peer;

  // Save GameValue to memory
  QOStrStream out;
  {
    // activate Engine FPU settings
    SCOPE_FPU_ENGINE

    ParamArchiveSave ar(WorldSerializeVersion);
    ar.SetParams(&GGameState);
    if (ptr) ptr->Serialize(ar);
    ar.SaveBin(out);
  }

  // Convert to jbyteArray
  int size = out.pcount();
  jbyteArray data = env->NewByteArray(size);
  env->SetByteArrayRegion(data, 0, size, (const jbyte *)out.str());

  return data;
}

static jlong JNICALL JNI_LoadPeer(JNIEnv *env, jobject obj, jbyteArray data)
{
  // Store data to an input stream
  int size = env->GetArrayLength(data);
  if (size == 0) return 0; //not NULL, as warning: converting to non-pointer type ‘jlong’ from NULL
  Ref<QIStreamBuffer> buffer = new QIStreamBuffer(size);
  env->GetByteArrayRegion(data, 0, size, (jbyte *)buffer->DataLock(0, size));
  QIStrStream in;
  in.init(buffer);

  GameValue value;

  // Load GameValue from stream
  ParamArchiveLoad ar;
  if (!ar.LoadBin(in)) return 0;
  ar.SetParams(&GGameState);
  ar.FirstPass();
  if (value.Serialize(ar) != LSOK) return 0;
  ar.SecondPass();
  if (value.Serialize(ar) != LSOK) return 0;
  ar.Close();

  jlong result = (jlong)new GameValue(value);
  return result;
}

static RString GetCurrentDirectory()
{
  RString currentDir;
  int size = GetCurrentDirectory(0, NULL);
  GetCurrentDirectory(size, currentDir.CreateBuffer(size));
  if (currentDir[size - 1] != '\\') currentDir = currentDir + RString("\\");
  return currentDir;
}

static jobjectArray ConvertDirectories(JNIEnv *env, const AutoArray<RString> &dirs)
{
  CHECK_FPU_JVM

  // check the cached class
  static jweak stringClass = NULL;
  jclass cls = (jclass)env->NewLocalRef(stringClass);
  if (cls == NULL) 
  {
    // class is not valid, initialize and store
    cls = env->FindClass("java/lang/String");
    stringClass = env->NewWeakGlobalRef(cls);
  }

  // get current directory to convert relative path to absolute
  RString currentDir;
  {
    // activate Engine FPU settings
    SCOPE_FPU_ENGINE

    currentDir = GetCurrentDirectory();
  }

  int n = dirs.Size();
  jobjectArray result = env->NewObjectArray(n, cls, NULL);
  for (int i=0; i<n; i++)
  {
    RString name = dirs[i];
    if (IsRelativePath(name)) name = currentDir + name;
    jstring path = env->NewStringUTF(name);
    env->SetObjectArrayElement(result, i, path);
  }

  return result;
}

static jobjectArray JNICALL JNI_GetReadableDirectories(JNIEnv *env, jobject obj)
{
  CHECK_FPU_JVM

  // accessible directories
  AutoArray<RString> dirs;
  {
    // activate Engine FPU settings
    SCOPE_FPU_ENGINE

    // JRE
    dirs.Add(GJRE);
    // mission directory
    RString dir = GetMissionDirectory();
    if (!dir.IsEmpty()) dirs.Add(dir);
    // campaign directory
    dir = GetBaseDirectory();
    if (!dir.IsEmpty()) dirs.Add(dir + "scripts");
    // user profile
    dir = GetUserDirectory();
    if (!dir.IsEmpty()) dirs.Add(dir);

#ifdef _WIN32
    // resources for UI rendering
    dir = GetWinFontsDirectory();
    if (!dir.IsEmpty()) dirs.Add(dir);
#endif
  }

  return ConvertDirectories(env, dirs);
}

static jobjectArray JNICALL JNI_GetWritableDirectories(JNIEnv *env, jobject obj)
{
  // accessible directories
  AutoArray<RString> dirs;

  {
    // activate Engine FPU settings
    SCOPE_FPU_ENGINE

    // user profile
    RString dir = GetUserDirectory();
    if (!dir.IsEmpty()) dirs.Add(dir);
  }

  return ConvertDirectories(env, dirs);
}

class RVDirectories
{
private:
  static RString GetMissionDirectory()
  {
    RString dir = ::GetMissionDirectory();
    if (dir.IsEmpty()) return dir;

    if (IsRelativePath(dir)) return GetCurrentDirectory() + dir;
    else return dir;
  }
  static RString GetUserDirectory()
  {
    RString dir = ::GetUserDirectory();
    if (dir.IsEmpty()) return dir;

    if (IsRelativePath(dir)) return GetCurrentDirectory() + dir;
    else return dir;
  }
  // helper
  static RString FindClass(RString name)
  {
    // find in mission
    RString dir = ::GetMissionDirectory();
    if (!dir.IsEmpty())
    {
      RString path = dir + name + ".class";
      if (QFBankQueryFunctions::FileExists(path)) return path;
    }
    // find in campaign
    dir = GetBaseDirectory();
    if (!dir.IsEmpty())
    {
      RString path = dir + RString("scripts\\") + name + ".class";
      if (QFBankQueryFunctions::FileExists(path)) return path;
    }
    return RString();
  }
  static Buffer<char> LoadClass(RString name)
  {
    Buffer<char> result;

    RString path = FindClass(name);
    if (path.IsEmpty()) return result;

    // read the .class file
    QIFStreamB stream;
    stream.AutoOpen(path);
    if (stream.fail())
    {
      LogF("Cannot open %s", cc_cast(path));
      return result;
    }
    int toRead = stream.rest();
    if (toRead == 0)
    {
      LogF("%s is empty", cc_cast(path));
      return result;
    }

    result.Init(toRead);

    char *ptr = result.Data();
    while (toRead > 0)
    {
      int read = stream.read(ptr, toRead);
      ptr += read;
      toRead -= read;
    }
    stream.close();

    return result;
  }
  static Buffer<char> LoadClass()
  {
    Buffer<char> result;
    return result;
  }

#define FUNCTIONS_RV_DIRECTORIES(XX) \
  XX("returns the mission directory", RString, GetMissionDirectory) \
  XX("returns the user directory", RString, GetUserDirectory) \
  XX("find the class in the mission or campaign directory and load its data", Buffer<char>, LoadClass, RString) \

public:
  JNI_REGISTER_FUNCTIONS(RVDirectories, FUNCTIONS_RV_DIRECTORIES, NO_FUNCTIONS)
  JNI_EXPORT_FUNCTIONS(RVDirectories, FUNCTIONS_RV_DIRECTORIES, NO_FUNCTIONS)
};

static jobject JNICALL JNI_GameObjectToEntity(JNIEnv *env, jobject obj, jobject jni)
{
  GameValue value = GameValueFromJObject(env, jni);
  if (value.GetType() != GameObject) return NULL;
  Object *object = static_cast<GameDataObject *>(value.GetData())->GetObject();
  Entity *entity = dyn_cast<Entity>(object);
  if (!entity) return NULL;
  return JavaTypes<Entity>::FromNative(env, entity);
}

// helpers for enumeration of operators
static bool IsInvalidOperator(const RString &name)
{
  // avoid language constructions
  if (!isalpha(name[0])) return true;
  if (stricmp(name, "do") == 0) return true;
  if (stricmp(name, "catch") == 0) return true;
  if (stricmp(name, "then") == 0) return true;
  if (stricmp(name, "else") == 0) return true;
  if (stricmp(name, "from") == 0) return true;
  if (stricmp(name, "to") == 0) return true;
  if (stricmp(name, "step") == 0) return true;
  if (stricmp(name, "exitWith") == 0) return true;

  return false;
}
static bool IsInvalidFunction(const RString &name)
{
  if (!isalpha(name[0])) return true;
  if (stricmp(name, "for") == 0) return true;
  if (stricmp(name, "while") == 0) return true;
  if (stricmp(name, "if") == 0) return true;
  if (stricmp(name, "switch") == 0) return true;
  if (stricmp(name, "case") == 0) return true;
  if (stricmp(name, "goto") == 0) return true;
  if (stricmp(name, "default") == 0) return true;
  if (stricmp(name, "assert") == 0) return true;
  if (stricmp(name, "throw") == 0) return true;
  if (stricmp(name, "try") == 0) return true;
  if (stricmp(name, "private") == 0) return true;
  if (stricmp(name, "with") == 0) return true;
  
  return false;
}
static bool IsInvalidNular(const RString &name)
{
  // avoid language constructions
  if (!isalpha(name[0])) return true;
  if (stricmp(name, "true") == 0) return true;
  if (stricmp(name, "false") == 0) return true;

  return false;
}

// creation of RVEngine.java interface file
static RString NormalizeComment(RString comment)
{
  // replace "*/" by "* /"
  RString result;
  const char *ptr = comment;
  const char *next = strchr(ptr, '*');
  while (next)
  {
    next++;
    result = result + RString(ptr, next - ptr);
    if (*next == '/') result = result + " ";
    ptr = next;
    next = strchr(ptr, '*');
  }
  return result + RString(ptr);
}
static void CreateComment(QOStream &out, RString description, RString loper, RString roper, RString since, RString example, RString exampleResult)
{
  out << "  /** \n";

  // description
  out << "   * ";
  out << NormalizeComment(description);
  out << "\n";
  // example
  if (!example.IsEmpty())
  {
    out << "<br/><br/>\n";
    out << "   * <b>Example:</b> ";
    out << example;
    out << "<br/>\n";
  }
  if (!exampleResult.IsEmpty())
  {
    out << "<br/>\n";
    out << "   * <b>Example result:</b> ";
    out << exampleResult;
    out << "<br/>\n";
  }
  // loper
  if (!loper.IsEmpty())
  {
    out << "   * @param oper1 ";
    out << loper;
    out << "\n";
  }
  // roper
  if (!roper.IsEmpty())
  {
    out << "   * @param oper2 ";
    out << roper;
    out << "\n";
  }
  // since
  if (!since.IsEmpty())
  {
    out << "   * @since ";
    out << since;
    out << "\n";
  }

  out << "   */\n";
}
static void ExportOperator(const GameOperators &ops, const GameOperatorsType *list, void *context)
{
  QOFStream &out = *(QOFStream *)context;
  for (int i=0; i<ops.Size(); i++)
  {
    RString name = ops._opName;
    if (name.GetLength() == 0 || IsInvalidOperator(name)) continue;

    // comment
    const GameOperator op = ops[i];
#if _ENABLE_COMREF
    CreateComment(out, op._description, op._loper, op._roper, op._since, op._example, op._exampleResult);
#endif
    // function declaration
    out << "  public native static ";
    out << op._operator->GetRetType().GetJavaType();
    out << " ";
    out << name;
    out << "(";
    out << op._operator->GetArg1Type().GetJavaType();
    out << " oper1, ";
    out << op._operator->GetArg2Type().GetJavaType();
    out << " oper2);\n";
  }
}
static void ExportFunction(const GameFunctions &ops, const GameFunctionsType *list, void *context)
{
  QOFStream &out = *(QOFStream *)context;
  for (int i=0; i<ops.Size(); i++)
  {
    RString name = ops._opName;
    if (name.GetLength() == 0 || IsInvalidFunction(name)) continue;

    // comment
    const GameFunction op = ops[i];
#if _ENABLE_COMREF
    CreateComment(out, op._description, op._roper, RString(), op._since, op._example, op._exampleResult);
    out << "  /* ";
    out << NormalizeComment(op._description);
    out << "*/\n";
#endif
    // function declaration
    out << "  public native static ";
    out << op._operator->GetRetType().GetJavaType();
    out << " ";
    out << name;
    out << "(";
    out << op._operator->GetArgType().GetJavaType();
    out << " oper1);\n";
  }
}
static void ExportNular(const GameNular &op, const GameNularsType *list, void *context)
{
  QOFStream &out = *(QOFStream *)context;

  RString name = op._opName;
  if (name.GetLength() == 0 || IsInvalidNular(name)) return;

  // comment
#if _ENABLE_COMREF
  CreateComment(out, op._description, RString(), RString(), op._since, op._example, op._exampleResult);
#endif
  // function declaration
  out << "  public native static ";
  out << op._operator->GetRetType().GetJavaType();
  out << " ";
  out << name;
  out << "();\n";
}
void ExportJavaInterface(RString jniSources)
{
  QOFStream out(jniSources + "/RVEngine.java");

  out << "package com.bistudio.JNIScripting;\n\n"; \
  out << "public class RVEngine\n{\n";

  const AutoArray<const GameTypeType *> &types = GGameState.GetTypes();
  for (int i=0; i<types.Size(); i++)
  {
    RString name = types[i]->_javaType;
    static const char *prefix = "Game";
    if (strncmp(name, prefix, strlen(prefix)) == 0)
    {
#if _ENABLE_COMREF
      out << "  /** \n";
      out << "   * ";
      out << NormalizeComment(types[i]->_description);
      out << "\n";
      out << "   */\n";
#endif
      out << "  public static class ";
      out << name;
      out << " extends NativeObject {}\n";
    }
  }
  out << "\n";
  const GameOperatorsType &operators = GGameState.GetOperators();
  operators.ForEach(ExportOperator, &out);
  const GameFunctionsType &functions = GGameState.GetFunctions();
  functions.ForEach(ExportFunction, &out);
  const GameNularsType &nulars = GGameState.GetNulars();
  nulars.ForEach(ExportNular, &out);

  // explicit functions
  out << "  // conversion from RVEngine.GameObject to Entity\n";
  out << "  public native static Entity convert(GameObject value);\n";

  out << "}\n";

  out.close();

  RVDirectories::ExportFunctions(jniSources);
  Entity::ExportFunctions(jniSources);
}

// registration of scripting functions to Java
struct RegisterContext
{
  JNIEnv *env;
  jclass cls;
};
static void RegisterOperator(const GameOperators &ops, const GameOperatorsType *list, void *context)
{
  RegisterContext *ctx = (RegisterContext *)context;

  for (int i=0; i<ops.Size(); i++)
  {
    RString name = ops._opName;
    if (name.GetLength() == 0 || IsInvalidOperator(name)) continue;
    const GameOperator op = ops[i];

    JNINativeMethod func;
    func.name = unconst_cast(cc_cast(name));
    RString signature = op.GetSignature(); // keep existence until RegisterNatives called
    func.signature = unconst_cast(cc_cast(signature));
    func.fnPtr = op._jniFunc;
    ctx->env->RegisterNatives(ctx->cls, &func, 1);
    if (ctx->env->ExceptionCheck())
    {
      ctx->env->ExceptionDescribe();
      ctx->env->ExceptionClear();
    }
  }
}
static void RegisterFunction(const GameFunctions &ops, const GameFunctionsType *list, void *context)
{
  RegisterContext *ctx = (RegisterContext *)context;

  for (int i=0; i<ops.Size(); i++)
  {
    RString name = ops._opName;
    if (name.GetLength() == 0 || IsInvalidFunction(name)) continue;
    const GameFunction op = ops[i];

    JNINativeMethod func;
    func.name = unconst_cast(cc_cast(name));
    RString signature = op.GetSignature(); // keep existence until RegisterNatives called
    func.signature = unconst_cast(cc_cast(signature));
    func.fnPtr = op._jniFunc;
    ctx->env->RegisterNatives(ctx->cls, &func, 1);
    if (ctx->env->ExceptionCheck())
    {
      ctx->env->ExceptionDescribe();
      ctx->env->ExceptionClear();
    }
  }
}
static void RegisterNular(const GameNular &op, const GameNularsType *list, void *context)
{
  RegisterContext *ctx = (RegisterContext *)context;

  RString name = op._opName;
  if (name.GetLength() == 0 || IsInvalidNular(name)) return;

  JNINativeMethod func;
  func.name = unconst_cast(cc_cast(name));
  RString signature = op.GetSignature(); // keep existence until RegisterNatives called
  func.signature = unconst_cast(cc_cast(signature));
  func.fnPtr = op._jniFunc;
  ctx->env->RegisterNatives(ctx->cls, &func, 1);
  if (ctx->env->ExceptionCheck())
  {
    ctx->env->ExceptionDescribe();
    ctx->env->ExceptionClear();
  }
}
static void RegisterJavaInterface(JNIEnv *env)
{
  SCOPE_FPU_JVM

  RegisterContext ctx;
  ctx.env = env;
  ctx.cls = env->FindClass("com/bistudio/JNIScripting/RVEngine");
  if (ctx.cls == NULL) return;

  const GameOperatorsType &operators = GGameState.GetOperators();
  operators.ForEach(RegisterOperator, &ctx);
  const GameFunctionsType &functions = GGameState.GetFunctions();
  functions.ForEach(RegisterFunction, &ctx);
  const GameNularsType &nulars = GGameState.GetNulars();
  nulars.ForEach(RegisterNular, &ctx);

  // explicit registration
  {
    JNINativeMethod func;
    func.name = "convert";
    func.signature = "(Lcom/bistudio/JNIScripting/RVEngine$GameObject;)Lcom/bistudio/JNIScripting/Entity;";
    func.fnPtr = (void *)&JNI_GameObjectToEntity;
    env->RegisterNatives(ctx.cls, &func, 1);
    if (env->ExceptionCheck())
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
    }
  }

  RVDirectories::RegisterFunctions(env);
  Entity::RegisterFunctions(env);
}

// Performance test
static float PerfTestCpp(float interval)
{
  float result = 0;
  for (float t=0; t<360; t+=interval)
  {
    float rad = t * H_PI / 180.0;
    result += sin(rad);
  }
  return result;
}
static float JVMSin(JNIEnv *env, jobject obj, float x)
{
  SCOPE_FPU_ENGINE
  float rad = x * H_PI / 180.0f;
  return sin(rad);
}
void PerfTestJavaVM()
{
  float interval = 0.001f;

  {
    volatile int start = GlobalTickCount();
    volatile float res = PerfTestCpp(interval);
    volatile int timeMs = GlobalTickCount() - start;
    RptF("C++ test duration %d ms, result %f", timeMs, res);
  }

  {
    SCOPE_FPU_JVM

    JNIEnv *env = GetJNIEnv();
    jclass cls = env->FindClass("com/bistudio/JNIScripting/PerfTest");

    if (cls != NULL)
    {
      // Register optimized scripting function
      JNINativeMethod helper;
      helper.name = "sin";
      helper.signature = "(F)F";
      helper.fnPtr = (void *)&JVMSin;
      env->RegisterNatives(cls, &helper, 1);
      if (env->ExceptionCheck())
      {
        env->ExceptionDescribe();
        env->ExceptionClear();
      }

      // Try to invoke function
      jmethodID id = env->GetStaticMethodID(cls, "perfTestJava", "(F)F");
      if (id != NULL)
      {
        volatile int start = GlobalTickCount();
        volatile float res = env->CallStaticFloatMethod(cls, id, interval);
        volatile int timeMs = GlobalTickCount() - start;
        RptF("Java test duration %d ms, result %f", timeMs, res);
      }
      id = env->GetStaticMethodID(cls, "perfTestJNI", "(F)F");
      if (id != NULL)
      {
        volatile int start = GlobalTickCount();
        volatile float res = env->CallStaticFloatMethod(cls, id, interval);
        volatile int timeMs = GlobalTickCount() - start;
        RptF("JNI (optimized) test duration %d ms, result %f", timeMs, res);
      }
      id = env->GetStaticMethodID(cls, "perfTestJNIRV", "(F)F");
      if (id != NULL)
      {
        volatile int start = GlobalTickCount();
        volatile float res = env->CallStaticFloatMethod(cls, id, interval);
        volatile int timeMs = GlobalTickCount() - start;
        RptF("JNI test duration %d ms, result %f", timeMs, res);
      }
    }
  }
  {
    GameDataNamespace globals(NULL, RString(), false);
    GameVarSpace locals(false);

    RString script = "_result = 0; for [{_t=0},{_t<360},{_t=_t+_interval}] do { _result = _result + sin _t; }; _result";
    CompiledExpression compiled;
    GGameState.CompileMultiple(script, compiled, &globals);

    volatile int start = GlobalTickCount();
    int gshandle = GGameState.BeginContext(&locals);
    GGameState.VarSetLocal("_interval", interval, true);
    GameValue resGV = GGameState.Evaluate(script, compiled, GameState::EvalContext::_default, &globals);
    GGameState.EndContext(gshandle);
    volatile int timeMs = GlobalTickCount() - start;

    float res = resGV;
    RptF("SQF test duration %d ms, result %f", timeMs, res);
  }
}

// Initialization / destruction
void InitJavaVM(RString jre, int debugPort, bool suspend)
{
#ifdef _WIN32
  // Load jvm.dll dynamically and find JNI_CreateJavaVM function
  HMODULE lib = LoadLibrary(jre + "/bin/client/jvm.dll");
  if (!lib)
  {
    LogF("Java VM not found");
    return;
  }
  FuncCreateJavaVM *createFunc = (FuncCreateJavaVM *)GetProcAddress(lib, "JNI_CreateJavaVM");
  if (!createFunc)
  {
    LogF("Java VM corrupted");
    return;
  }
#endif

  // Create the Java VM
  JavaVMOption options[5];
  RString debugCmd, classPathCmd, libPathCmd; // storage to persist until createFunc is called
  int nOptions = 0;

#ifdef _WIN32
  libPathCmd = RString("-Djava.library.path=") + jre + RString("/lib");
  options[nOptions++].optionString = unconst_cast(cc_cast(libPathCmd));
#else
  //on linux one should use LD_LIBRARY_PATH env. variable
  //example: export LD_LIBRARY_PATH=/usr/java/jdk1.7.0/jre/lib/i386/client
  classPathCmd = RString("-Djava.class.path=jre/lib/ext/jniscripting.jar");
  options[nOptions++].optionString = unconst_cast(cc_cast(classPathCmd));
#endif

  options[nOptions].optionString = "vfprintf";
  options[nOptions++].extraInfo = (void *)JVMPrintF; //error: invalid conversion from int (*)(FILE*, const char*, char*) to void*

  if (debugPort > 0)
  {
    options[nOptions++].optionString = "-verbose:class,jni";
    debugCmd = Format("-agentlib:jdwp=transport=dt_socket,server=y,address=%d,suspend=%c", debugPort, suspend ? 'y' : 'n');
    options[nOptions++].optionString = unconst_cast(cc_cast(debugCmd));
  }

  JavaVMInitArgs args;
  args.version = JNI_VERSION_1_6;
  args.ignoreUnrecognized = JNI_TRUE;
  args.options = options;
  args.nOptions = nOptions;

  // store setting prior JVM creation
  LocalFPUSettings::InitEngine();

  JNIEnv *env = NULL;
  {
    // restore the FPU state after JVM invocation
    SCOPE_FPU_ENGINE

#ifdef _WIN32
    jint result = (*createFunc)(&GJavaVM, (void **)&env, &args);
#else
    jint result = JNI_CreateJavaVM(&GJavaVM, (void **)&env, &args);
#endif
    if (result != 0)
    {
      GJavaVM = NULL;
      RptF("Java Create failed");
      return;
    }
    else RptF("JavaVM Created");
    // store setting after JVM creation
    LocalFPUSettings::InitJVM();
  }

  // enable access to JRE
  GJRE = jre;

  // Initialize the logging system
  // need to be prior security manager installed (setIO permission disabled)
  {
    SCOPE_FPU_JVM
    jclass cls = env->FindClass("com/bistudio/JNIScripting/Logging");
    if (env->ExceptionCheck())
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
    }
    if (cls)
    {
      // register callback
      JNINativeMethod helper;
      helper.name = "logImpl";
      helper.signature = "(Ljava/lang/String;)V";
      helper.fnPtr = (void *)&JNI_LogImpl;
      env->RegisterNatives(cls, &helper, 1);

      jmethodID id = env->GetStaticMethodID(cls, "init", "()V");
      env->CallStaticVoidMethod(cls, id);
    }
  }

  // Install restrictions to protect PC content outside game directory
  {
    SCOPE_FPU_JVM
    jclass cls = env->FindClass("com/bistudio/JNIScripting/FilesAccess");
    if (env->ExceptionCheck())
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
    }
    if (!cls)
    {
      GJavaVM->DestroyJavaVM();
      GJavaVM = NULL;
      return;
    }

    {
      JNINativeMethod helper;
      helper.name = "getReadableDirectories";
      helper.signature = "()[Ljava/lang/String;";
      helper.fnPtr = (void *)&JNI_GetReadableDirectories;
      env->RegisterNatives(cls, &helper, 1);
      if (env->ExceptionCheck())
      {
        env->ExceptionDescribe();
        env->ExceptionClear();
      }
    }
    {
      JNINativeMethod helper;
      helper.name = "getWritableDirectories";
      helper.signature = "()[Ljava/lang/String;";
      helper.fnPtr = (void *)&JNI_GetWritableDirectories;
      env->RegisterNatives(cls, &helper, 1);
      if (env->ExceptionCheck())
      {
        env->ExceptionDescribe();
        env->ExceptionClear();
      }
    }

    jmethodID id = env->GetStaticMethodID(cls, "init", "()Z");
    jboolean ok = env->CallStaticBooleanMethod(cls, id);
    if (env->ExceptionCheck())
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
    }
    if (!ok)
    {
      GJavaVM->DestroyJavaVM();
      GJavaVM = NULL;
      return;
    }
  }

  // Registration of scripting functions
  // helper functions
  {
    SCOPE_FPU_JVM
    jclass cls = env->FindClass("com/bistudio/JNIScripting/NativeObject");
    if (cls)
    {
      JNINativeMethod methods[] =
      {
        {"destroyPeer", "(J)V", (void *)&JNI_DestroyPeer},
        {"savePeer", "(J)[B", (void *)&JNI_SavePeer},
        {"loadPeer", "([B)J", (void *)&JNI_LoadPeer}
      };
      env->RegisterNatives(cls, methods, lenof(methods));
      if (env->ExceptionCheck())
      {
        env->ExceptionDescribe();
        env->ExceptionClear();
      }
    }
  }
  // scripting functions
  RegisterJavaInterface(env);
}
void DestroyJavaVM()
{
  if (GJavaVM != NULL)
  {
    SCOPE_FPU_JVM
    GJavaVM->DestroyJavaVM();
    GJavaVM = NULL;
  }
}

