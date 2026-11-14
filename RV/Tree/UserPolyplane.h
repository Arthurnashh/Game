#pragma once

#include "Polyplane.h"

namespace TreeEngine
{

  class CUserPolyplane: public CPolyplane
  {
  protected:
    int _countPlanes;
    int _maxFaceCount;
    RString _name;
    GameValue _userData;
    GameValue _cachedDrawResult;
    AutoArray<ComRef<IDirect3DTexture8> >_colormaps;
    AutoArray<ComRef<IDirect3DTexture8> >_normalmaps;
  public:

    CUserPolyplane():_countPlanes(0),_maxFaceCount(0) {}

    bool SetParameters(
      RString name,
      int countPlanes, 
      int maxFaceCount, 
      GameValuePar userData);

    void Render(CPrimitiveStream &PSBranch, CPrimitiveStream &PSPolyplane,
      CPrimitiveStream &PSStageInit, Matrix4Par Origin,   const CPointList &pointList,
      float SizeCoef, const DrawParameters &dp);
    void Draw(CPrimitiveStream &PSTriplane, Matrix4Par Origin, float NewSizeCoef);
    void UpdatePointList(Matrix4Par Origin, float NewSizeCoef, CPointList &pointList);

    virtual GameValue CallRenderScript(const CPointList &pointList, Matrix4Par Origin, float SizeCoef)=0;
    virtual GameValue CallDrawScript(Matrix4Par Origin, float NewSizeCoef)=0;

    virtual void Save(RString fileName, RString suffix);
    virtual int GetNumberOfTextures() {return _countPlanes;}
    virtual ComRef<IDirect3DTexture8> GetTexture(int index);
    virtual ComRef<IDirect3DTexture8> GetNormalTexture(int index);

  };

  class CScriptedPolyplane: public CUserPolyplane, public GameVarSpace
  {
  protected:
    RString _planeRenderScript;
    RString _planeUseScript;
#if USE_PRECOMPILATION
    CompiledExpression _planeRenderScriptCompiled;
    CompiledExpression _planeUseScriptCompiled;
#endif

    GameState &_gstate;

  public:
    CScriptedPolyplane(GameState &gstate);
    ~CScriptedPolyplane(void);

    ///
    /**@param name Name of scripted polyplane for debugging
       @param countPlanes Total count planes needed by this polyplane 
       @param maxFaceCount Specifies maximum of faces for this polyplane
       @param renderScript specifies script to control plane rendering
       @param useScript specifies script to control usage of polyplanes
       @param userData object contains user data passed to the both of scripts
       */

    bool SetParameters(RString name,int countPlanes, 
      int maxFaceCount, 
      RString renderScript,
      RString useScript,
      GameValuePar userData);

    virtual GameValue CallRenderScript(const CPointList &pointList, Matrix4Par Origin, float SizeCoef);
    virtual GameValue CallDrawScript(Matrix4Par Origin, float NewSizeCoef);

    static void RegisterToGameState(GameState *gstate);
  };

};