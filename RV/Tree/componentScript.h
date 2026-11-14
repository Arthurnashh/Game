#pragma  once

#include "component.h"

namespace TreeEngine
{


  class CScriptedComponent: public CComponent 
  {
  protected:
    //! optional script that controls the component
    RString _script;
#if USE_PRECOMPILATION
    CompiledExpression _compiledScript;
#endif

    GameState &_gState;
  public:
    CScriptedComponent(GameState &gState, const RString &script);

    //! Initializes the component with concrete values
    virtual void Init(  RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names
      );

    void RegisterActualAndConstantParameters(const SActualParams &ap,
      const SConstantParams &cp);

    int ExecuteScript(const SActualParams &ap,
                       const SConstantParams &cp,
                       CComponentChildBuffer *ccb=0,
                       CPrimitive *prim=0);

    static void RegisterCommands(GameState &gState);

  };

}