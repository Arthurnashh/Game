#include "RVTreeCommon.h"
#include "componentScript.h"
#include "auxiliary.h"

namespace TreeEngine
{



  //! Initializes the component with concrete values
  void CScriptedComponent::Init(  RString folder,
    const ParamEntry &treeEntry,
    const ParamEntry &subEntry,
    const AutoArray<RString> &names
    )
  {
    CComponent::Init(folder, treeEntry, subEntry, names);

    RString fileName = (RString)(subEntry >> "script");
    if ((fileName[1] != ':') && (folder.GetLength() > 0)) fileName = folder + RString("\\") + fileName;
    _script        = LoadAndPreprocessRStringFromFile(fileName);

#if USE_PRECOMPILATION
    SourceDoc srcDoc(fileName,_script);
    SourceDocPos srcDocPos(srcDoc);
    _gState.CompileMultiple(srcDoc,srcDocPos,_compiledScript);
#endif

  }
  
  CScriptedComponent::CScriptedComponent(GameState &gState, const RString &script)
    :_gState(gState),_script(script) 
  {
  
#if USE_PRECOMPILATION
    if (script.GetLength())
    {    
      SourceDoc srcDoc("",_script);
      SourceDocPos srcDocPos(srcDoc);
      _gState.CompileMultiple(srcDoc,srcDocPos,_compiledScript);
    }
#endif
  }


  void CScriptedComponent::RegisterActualAndConstantParameters(const SActualParams &ap,
    const SConstantParams &cp) {

      // -----------------
      // Actual parameters

      // Origin
      _gState.VarSet("_origin", ConvertMatrix4ToGameValue(ap._origin), true);

      // Seed
      _gState.VarSet("_seed", GameValue((float)ap._seed), true);

      // Age
      _gState.VarSet("_age", GameValue(ap._age), true);

      // Counter
      _gState.VarSet("_counter", GameValue((float)ap._counter), true);

      // Trunk
      _gState.VarSet("_trunk", GameValue(ap._trunk), true);

      // DiaOrigin
      _gState.VarSet("_diaOrigin", ConvertVector3ToGameValue(ap._diaOrigin), true);

      // Texture V offset
      _gState.VarSet("_textureVOffset", GameValue(ap._textureVOffset), true);

      // GeometrySpin
      _gState.VarSet("_geometrySpin", GameValue(ap._geometrySpin), true);

      // Optional parameters
      _gState.VarSet("_optional", ap._optional, true);

      // -------------------
      // Constant parameters

      // Detail
      _gState.VarSet("_detail", GameValue(cp._detail), true);

      // Time
      _gState.VarSet("_time", GameValue(cp._time), true);

      // Month
      _gState.VarSet("_month", GameValue(cp._month), true);

      // Age of the plant
      _gState.VarSet("_plantAge", GameValue(cp._plantAge), true);

      // -------------------
      // Constants

      _gState.VarSet("_PT_BLOCK",        GameValue((float)PT_BLOCK), true);
      _gState.VarSet("_PT_BUNCH",        GameValue((float)PT_BUNCH), true);
      _gState.VarSet("_PT_BLOCKBEND",    GameValue((float)PT_BLOCKBEND), true);
      _gState.VarSet("_PT_BLOCKNORM",    GameValue((float)PT_BLOCKNORM), true);
      _gState.VarSet("_PT_BLOCKSIMPLE",  GameValue((float)PT_BLOCKSIMPLE), true);
      _gState.VarSet("_PT_BLOCKSIMPLE2", GameValue((float)PT_BLOCKSIMPLE2), true);
      _gState.VarSet("_PT_BLOCKBEND2",   GameValue((float)PT_BLOCKBEND2), true);
      _gState.VarSet("_PT_BLOCKNORM2",   GameValue((float)PT_BLOCKNORM2), true);
      _gState.VarSet("_PT_UNIPLANE_X", GameValue((float)PT_UNIPLANE_X), true);
      _gState.VarSet("_PT_UNIPLANE_Y", GameValue((float)PT_UNIPLANE_Y), true);
      _gState.VarSet("_PT_UNIPLANE_Z", GameValue((float)PT_UNIPLANE_Z), true);
      _gState.VarSet("_PT_DEFAULT",    GameValue((float)PT_NOTDEFINED), true);
      _gState.VarSet("_PT_NONE",         GameValue((float)PT_NONE), true);

      _gState.VarSet("_AT_Aside",      GameValue((float)AT_Aside), true);
      _gState.VarSet("_AT_Up",         GameValue((float)AT_Up), true);
      _gState.VarSet("_AT_Direction",  GameValue((float)AT_Direction), true);
      _gState.VarSet("_AT_Aside_Rev",      GameValue((float)AT_Aside_Rev), true);
      _gState.VarSet("_AT_Up_Rev",         GameValue((float)AT_Up_Rev), true);
      _gState.VarSet("_AT_Direction_Rev",  GameValue((float)AT_Direction_Rev), true);

      // -------------------
      // External parameters

      for (int i=0; i<_parameters.Size(); i++)
      {
        GameValue value = _gState.Evaluate(_parameters[i]._value);
        _gState.VarSet(_parameters[i]._name, value);
      }
    }

    int CScriptedComponent::ExecuteScript(const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer *ccb,
      CPrimitive *prim)
    {
      
      // Initialize param database with active component
      if (_paramDB) _paramDB->SetActiveComponentName(GetComponentName());
      // Notify the profiler, that component processing is starting
      if (_paramDB) _paramDB->Profile_ComponentStart();
      
      // Create space for variables
      GameVarSpace vars;
      _gState.BeginContext(&vars);

      if (ccb)
      // Register componentChildBuffer object
        _gState.VarSet("_childBuffer", GameValue(new GameDataComponentChildBuffer(ccb)), true);

      // Register component object
      _gState.VarSet("_this", GameValue(new GameDataComponent(this)), true);

      if (prim)
      // Register primitive object
      _gState.VarSet("_primitive", GameValue(new GameDataPrimitive(prim)), true);

      // Register actual and constant parameters
      RegisterActualAndConstantParameters(ap, cp);

      // Execute the script
#if USE_PRECOMPILATION
      _gState.Execute(_script,_compiledScript);

#else
      _gState.Execute(_script);
#endif

      EvalError executeResult = _gState.GetLastError();

      // Destroy space for variables
      _gState.EndContext();

      // Notify the profiler, that component processing is finished
      if (_paramDB) _paramDB->Profile_ComponentStop();

      // Return
      return (executeResult == EvalOK);

    }

    GameValue Component_ImportParam(const GameState *state, GameValuePar oper) 
    {
      // retrieve current component variable
      GameValue _this=state->VarGet("_this");
      // retrieve current component game data class
      GameDataComponent *compData=static_cast<GameDataComponent *>(_this.GetData());
      // retrieve current component pointer
      CComponent *comp=compData->GetComponent();
      // retrieve active param database
      IPlantControlDB *db=comp->GetParamDatabase();
      // get name of parameter
      RString name=oper;
      
      // retrieve result
      return db->GetDBItemScript(name);
    }

    GameValue Component_SetNamespace(const GameState *state, GameValuePar oper)
    {
      // retrieve current component variable
      GameValue _this=state->VarGet("_this");
      // retrieve current component game data class
      GameDataComponent *compData=static_cast<GameDataComponent *>(_this.GetData());
      // retrieve current component pointer
      CComponent *comp=compData->GetComponent();
      // retrieve active param database
      IPlantControlDB *db=comp->GetParamDatabase();
      // get name of parameter
      RString name=oper;

      // set active namespace
      db->SetNamespace(name);
      return oper;
    }

    void CScriptedComponent::RegisterCommands(GameState &gState)
    {
      gState.NewFunction(GameFunction(GameAny,      "importParam",            Component_ImportParam,            GameString TODO_FUNCTION_DOCUMENTATION));
      gState.NewFunction(GameFunction(GameNothing,  "importSetNamespace",     Component_SetNamespace,           GameString TODO_FUNCTION_DOCUMENTATION));
    }

}