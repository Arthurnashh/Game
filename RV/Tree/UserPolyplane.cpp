#include "RVTreeCommon.h"
#include ".\userpolyplane.h"
#include <el/Evaluator/Addons/Matrix/gdMatrix.h>
#include <el/MultiThread/Tls.h>

#define Category "Scripted polyplane"

namespace TreeEngine
{
  CScriptedPolyplane::CScriptedPolyplane(GameState &gstate):_gstate(gstate)
  {
  }

  CScriptedPolyplane::~CScriptedPolyplane(void)
  {
  }

  bool CUserPolyplane::SetParameters(RString name,int countPlanes,int maxFaceCount,GameValuePar userData)
  {
    _countPlanes=countPlanes;
    _maxFaceCount=maxFaceCount;
    _name=name;
    _userData=userData;
    return true;

  }

  bool CScriptedPolyplane::SetParameters(RString name,int countPlanes, int maxFaceCount, RString renderScript, RString useScript, GameValuePar userData)
  {
    CUserPolyplane::SetParameters(name,countPlanes,maxFaceCount,userData);

    _planeRenderScript=renderScript;
    _planeUseScript=useScript;

#if USE_PRECOMPILATION
    {  
      SourceDoc srcDoc("",_planeRenderScript);
      SourceDocPos srcDocPos(srcDoc);
      _gstate.CompileMultiple(srcDoc,srcDocPos,_planeRenderScriptCompiled);
    }
    {  
      SourceDoc srcDoc("",_planeUseScript);
      SourceDocPos srcDocPos(srcDoc);
      _gstate.CompileMultiple(srcDoc,srcDocPos,_planeUseScriptCompiled);
    }
#endif
    return true;
  }

  void CUserPolyplane::Render(CPrimitiveStream &PSBranch,CPrimitiveStream &PSPolyplane, 
    CPrimitiveStream &PSStageInit,
    Matrix4Par Origin,const CPointList &pointList,float SizeCoef, const DrawParameters &dp)
  {
    GameValue res=CallRenderScript(pointList,Origin,SizeCoef);
    //script returns array
    //each line is one plane
    //for each line, it returns row in following form
    /*
    [camera , width , height]
    */
    if (res.GetType()!=GameArray)
    {
      LogF("Error in scripted polyplane %s - array must be returned",_name.Data());
      _countPlanes=0;
    }
    else
    {
      const GameArrayType &arr=res;
      int cnt=arr.Size();
      if (cnt>_countPlanes)
      {
        cnt=_countPlanes;
      }

      // Modify geometry normals
      SBoundingBox bbox;
      pointList.ComputeBoundingBox(Origin, bbox);
      ModifyNormals(PSBranch, PSPolyplane, pointList, bbox.GetVolume(), dp);

      PSBranch.Prepare();
      PSPolyplane.Prepare();

      for (int i=0;i<arr.Size();i++)
      {
        const GameArrayType &subarr=arr[i];
        if (subarr.Size()!=3)
          LogF("Error in scripted polyplane %s,%d - each field must have 3 subfields. [camera, width, height]",_name.Data(),i);
        else if (subarr[0].GetType()!=EvalType_Matrix)
          LogF("Error in scripted polyplane %s,%d - excepting Matrix as first parameter",_name.Data(),i);
        else if (subarr[1].GetType()!=GameScalar || subarr[2].GetType()!=GameScalar )
          LogF("Error in scripted polyplane %s,%d - excepting Scalar as second or third parameter",_name.Data(),i);
        else
        {
          Matrix4 camera=*static_cast<GdMatrix *>(subarr[0].GetData());
          float width=subarr[1];
          float height=subarr[2];
          ComRef<IDirect3DTexture8> colorMap;
          ComRef<IDirect3DTexture8> normalMap;
          RenderPlane(PSBranch,PSPolyplane,colorMap,normalMap,pointList,camera,width,height,dp,NULL);
          RegisterTextureToStage(PSStageInit,i,colorMap,normalMap);
          _colormaps.Add(colorMap);
          _normalmaps.Add(normalMap);
        }
      }
      _SizeCoef = SizeCoef;
      _countPlanes=_colormaps.Size();;
    }
  }

  Vector3 GetVectorFromArr(GameValuePar garr)
  {
    if (garr.GetType()!=GameArray) return Vector3(0,0,0);
    const GameArrayType &arr=garr;
    if (arr.Size()!=3) return Vector3(0,0,0);
    return Vector3(arr[0],arr[1],arr[2]);   
  }

  void GetUVFromArr(GameValuePar garr,float &u, float &v)
  {
    u=0,v=0;
    if (garr.GetType()!=GameArray) return;
    const GameArrayType &arr=garr;
    if (arr.Size()!=2) return;
    u=arr[0];
    v=arr[1];
  }

  void CUserPolyplane::Draw(CPrimitiveStream &PSTriplane, Matrix4Par Origin, float NewSizeCoef)
  {
    GameValue res=CallDrawScript(Origin,NewSizeCoef);
    //script returns an array
    //Each field in array is vertex
    //each three vertices is face
    //each vertex has fields [texture,x,y,z,u,v,nx,ny,nz] - nx...nz are optional, and if omitted, program 
    //calculates them
    //

    if (res.GetType()!=GameArray)
    {
      LogF("Error in scripted polyplane %s - array must be returned",_name.Data());    
    }
    else
    {  
      const GameArrayType &arr=res;
      int faces=arr.Size()/3;
      if (faces*3!=arr.Size())
      {
        LogF("Warning in scripted polyplane %s - count of vertices must be multiply by 3",_name.Data());   
      }

      int vertcount=faces*3;
      AutoArray<int,MemAllocLocal<int,100> > textures;
      AutoArray<SPolyplaneVertex,MemAllocLocal<int,300> > vertices;
      vertices.Reserve(vertcount,vertcount);
      textures.Reserve(faces);    
      for (int i=0;i<vertcount;i++)
      { 
        SPolyplaneVertex &vx=vertices.Append();
        if (arr[i].GetType()==GameArray)
        {
          const GameArrayType &vxarr=arr[i];
          if (vxarr.Size()==3 || vxarr.Size()==4)
          {
            vx.Position=GetVectorFromArr(vxarr[1]);
            GetUVFromArr(vxarr[2],vx.U1,vx.V1);
            if (i % 3==0) textures.Add(toInt(vxarr[0]));
            if (vxarr.Size()==4)
              vx.Normal=GetVectorFromArr(vxarr[1]);
            else
              vx.Normal=Vector3(0,0,0);
          }
          else
            LogF("Error in scripted polyplane %s - excepting 6 or 9 parameters in vertex %d",_name.Data(),i);
        }
        else
          LogF("Error in scripted polyplane %s - excepting array at position %d",_name.Data(),i);      
      }
      for (int i=0;i<faces;i++)
      {
        int offsets[]={0,1,2,0,1};
        int offset=i*3;      
        for (int j=0;j<3;j++)
        {
          SPolyplaneVertex &ax=vertices[offset+offsets[j+0]];
          const SPolyplaneVertex &bx=vertices[offset+offsets[j+1]];
          const SPolyplaneVertex &cx=vertices[offset+offsets[j+2]];
          if (ax.Normal==Vector3(0,0,0))
            ax.CalculateNormal(bx,cx);
          ax.GenerateST(bx,cx);
        }
        int stage=textures[i]+_baseStage;
        for (int j=0;j<3;j++)
        {
          PSTriplane.AddIndex(stage,PSTriplane.GetNewVertexIndex());
          PSTriplane.AddVertex(&vertices[offset+j]);
        }
      }
    }
    _cachedDrawResult=res;

  }

  void CUserPolyplane::UpdatePointList(Matrix4Par Origin, float NewSizeCoef, CPointList &pointList)
  {
    GameValue res=_cachedDrawResult.GetNil()?CallDrawScript(Origin,NewSizeCoef):_cachedDrawResult;
    _cachedDrawResult=GameValue();
    //script returns an array
    //Each field in array is vertex
    //each three vertices is face
    //each vertex has fields [texture,x,y,z,u,v,nx,ny,nz] - nx...nz are optional, and if omitted, program 
    //calculates them
    //

    if (res.GetType()!=GameArray)
    {
      LogF("UpdatePointList error - probably in connection to the previous error",_name.Data());    
    }
    else
    {
      const GameArrayType &arr=res;
      for (int i=0,cnt=arr.Size();i<cnt;i++) if (arr[i].GetType()==GameArray)
      {
        const GameArrayType &subarr=arr[i];
        if (subarr.Size()>=6)
        {
          Vector3 vx(subarr[1],subarr[2],subarr[3]);
          pointList.AddPoint(vx);
        }
      }
    }
  }

  struct Context
  {
    const CPointList &pointList;
    CScriptedPolyplane &polyplane;

    Context(const CPointList &pointList, CScriptedPolyplane &polyplane):
    polyplane(polyplane),pointList(pointList) {}
  };

  static TLS(Context) SContext; 

  GameValue CScriptedPolyplane::CallRenderScript(const CPointList &pointList, Matrix4Par Origin, float SizeCoef)
  {
    Context cx(pointList,*this);
    SContext=&cx;

    GameArrayType arr;
    arr.Add(_userData);
    arr.Add(GameValue(new GdMatrix(Origin)));
    arr.Add(SizeCoef);

    VarSet("_this",arr);
    _parent=_gstate.GetContext();
   _gstate.BeginContext(this);
#if USE_PRECOMPILATION  
    GameValue res=_gstate.Evaluate(_planeRenderScript,_planeRenderScriptCompiled);
#else
    GameValue res=_gstate.EvaluateMultiple(_planeRenderScript);
#endif
    _gstate.EndContext();
    SContext=0;
    return res;
  }

  GameValue CScriptedPolyplane::CallDrawScript(Matrix4Par Origin, float NewSizeCoef)
  {
    GameArrayType arr;
    arr.Add(_userData);
    arr.Add(GameValue(new GdMatrix(Origin)));
    arr.Add(NewSizeCoef);

    
    VarSet("_this",arr);
    _parent=_gstate.GetContext();
   _gstate.BeginContext(this);
#if USE_PRECOMPILATION  
    GameValue res=_gstate.Evaluate(_planeUseScript,_planeUseScriptCompiled);
#else
    GameValue res=_gstate.EvaluateMultiple(_planeUseScript);
#endif
    _gstate.EndContext();
    return res;    
  }

  void CUserPolyplane::Save(RString fileName, RString suffix)
  {
    char name[50];
    for (int i=0;i<_colormaps.Size();i++)
    {
      SaveTexture(fileName + "_"+_itoa(i,name,30)+ suffix + ".tga", _colormaps[i], false);
    }
    for (int i=0;i<_normalmaps.Size();i++)
    {
      SaveTexture(fileName + "_"+_itoa(i,name,30)+ suffix + "_NO.tga", _normalmaps[i], true);
    }
  }

  ComRef<IDirect3DTexture8> CUserPolyplane::GetTexture(int index)
  {
    return _colormaps[index];
  }
  ComRef<IDirect3DTexture8> CUserPolyplane::GetNormalTexture(int index)
  {
    return _normalmaps[index];
  }

}

static GameValue VectorReturn(const Vector3 &vx)
{
  GameArrayType arr;
  arr.Add(vx[0]);
  arr.Add(vx[1]);
  arr.Add(vx[2]);
  return arr;
}

using TreeEngine::SContext;
using TreeEngine::SBoundingBox;

#define CHECKCONTEXT if (SContext==0) {state->SetError(EvalForeignError, "No current polyplane (out of context?");return GameValue();}
static GameValue computeFocus(const GameState *state)
{
  CHECKCONTEXT;
  Vector3 vx;
  if (SContext->pointList.ComputeFocus(vx))
  {
    return VectorReturn(vx);
  }
  else
  {
    return GameValue();
  }
}

static GameValue computeFocusOrig(const GameState *state, GameValuePar oper1)
{
  CHECKCONTEXT;
  Matrix4 mx=*static_cast<GdMatrix *>(oper1.GetData());
  Vector3 vx;
  if (SContext->pointList.ComputeFocus(mx,vx))
  {
    return VectorReturn(vx);
  }
  else
  {
    return GameValue();
  }
}

static GameValue computeBoundingBox(const GameState *state, GameValuePar oper1)
{
  CHECKCONTEXT;
  Matrix4 mx=*static_cast<GdMatrix *>(oper1.GetData());
  SBoundingBox bbox;
  if (SContext->pointList.ComputeBoundingBox(mx,bbox))
  {
    GameArrayType arr;
    arr.Add(bbox._MinDir);arr.Add(bbox._MaxDir);
    arr.Add(bbox._MinUp);arr.Add(bbox._MaxUp);
    arr.Add(bbox._MinAside);arr.Add(bbox._MaxAside);
    return arr;
  }
  else
    return GameValue();
}

static GameValue boundingBoxFocus(const GameState *state, GameValuePar oper1)
{
  const GameArrayType& arr=oper1;
  if (arr.Size()==6)
  {
    SBoundingBox bbox;
    bbox._MinDir=arr[0];bbox._MaxDir=arr[1];
    bbox._MinUp=arr[2];bbox._MaxUp=arr[3];
    bbox._MinAside=arr[4];bbox._MaxAside=arr[5];
    Vector3 vx=bbox.GetFocus();
    return VectorReturn(vx);
  }
  else 
  {
    state->TypeError(GameArray,GameArray,"Excepted 6 parameters");
    return GameValue();
  }
}

static GameValue boundingBoxDimension(const GameState *state, GameValuePar oper1)
{
  const GameArrayType& arr=oper1;
  if (arr.Size()==6)
  {
    SBoundingBox bbox;
    bbox._MinDir=arr[0];bbox._MaxDir=arr[1];
    bbox._MinUp=arr[2];bbox._MaxUp=arr[3];
    bbox._MinAside=arr[4];bbox._MaxAside=arr[5];
    Vector3 vx=bbox.GetDimension();
    return VectorReturn(vx);
  }
  else 
  {
    state->TypeError(GameArray,GameArray,"Excepted 6 parameters");
    return GameValue();
  }
}

static GameValue bestSize(const GameState *state, GameValuePar oper1)
{
  CHECKCONTEXT;
  const GameArrayType &arr=oper1;
  if (arr.Size()==2)
  {
    if (arr[0].GetType()==GameArray)
    {
      if (arr[1].GetType()==EvalType_Matrix)
      {
        const GameArrayType &focux=arr[0];
        if (focux.Size()==3)
        {
          Vector3 vx(focux[0],focux[1],focux[2]);
          Matrix4 mx(*static_cast<GdMatrix *>(arr[1].GetData()));
          float x,y,z;
          SContext->pointList.BestSize(vx,mx,x,y,z);
          GameArrayType res;
          res[0]=x;
          res[1]=y;
          res[2]=z;
          return res;
        }
        else
          state->TypeError(GameArray,GameArray,"Excepted [x,y,z] as first parameter");
      }
      else
        state->TypeError(EvalType_Matrix,arr[1].GetType(),"parameter 2");
    }
    else
      state->TypeError(GameArray,arr[1].GetType(),"parameter 1");
  }
  state->TypeError(GameArray,arr[1].GetType(),"Excepted 2 parameters");
  return GameValue();
}

static GameValue bestOrientation(const GameState *state)
{
  CHECKCONTEXT;
  Matrix4 bo;
  bo.SetOrientation(SContext->pointList.BestOrientation());
  bo.SetPosition(Vector3(0, 0, 0));
  GdMatrix *mx=new GdMatrix(bo);  
  return GameValue(mx);  
}

static GameValue bestOrientationAndSize(const GameState *state)
{
  CHECKCONTEXT;
  Vector3 vx;
  GdMatrix *mx=new GdMatrix(SContext->pointList.BestOriginAndSize(vx[0],vx[1],vx[2]));
  GameArrayType res;
  res.Add(GameValue(mx));
  res.Add(VectorReturn(vx));
  return res;
}

static GameValue geomNPoints(const GameState *state)
{
  CHECKCONTEXT;
  return (float)(SContext->pointList.NPoints());
}

static GameValue geomGetPoint(const GameState *state, GameValuePar oper1)
{
  CHECKCONTEXT;
  int index=toInt(oper1);
  if (index<0 || index>=SContext->pointList.NPoints())
  {
    state->SetError(EvalForeignError,"Index out of range");
    return GameValue();
  }
  else
  {
    return VectorReturn(SContext->pointList.GetPoint(index));
  }
}



//#define OPERATORS_DEFAULT(XX, Category) \
//  XX(GameAny,"@", mocnina, ListSelect, GameArray, GameScalar,"array","idx", "Shortcut for select: array @ 1 is the same as array select 1", "[\"a\",\"b\",true,3,8] @ 1", "\"b\"", "", "", Category) \

#define FUNCTIONS_DEFAULT(XX, Category) \
  XX(GameArray, "geomComputeFocusOrig", computeFocusOrig, EvalType_Matrix, "origin", "Computes focus of geometry transformed by the origin.", "", "[x,y,z]", "", "", Category) \
  XX(GameArray, "geomComputeBoundingBox", computeBoundingBox, EvalType_Matrix, "origin", "Computes bounding box of the geometry transformed by the origin.", "", "[minDir,maxDir,minUp,maxUp,minAside,maxAside]", "", "", Category) \
  XX(GameArray, "geomBoundingBoxFocus", boundingBoxFocus, GameArray, "boundingBox", "Computes focus of bounding box", "", "[x,y,z]", "", "", Category) \
  XX(GameArray, "geomBoundingBoxDimension", boundingBoxDimension, GameArray, "boundingBox", "Computes dimension of bounding box", "", "[x,y,z]", "", "", Category) \
  XX(GameArray, "geomBestSize", bestSize, GameArray, "[focus,origin]", "Computes best size of geometry for give origin", "", "[xs,ys,zs]", "", "", Category) \
  XX(GameArray, "geomGetPoint", geomGetPoint, GameScalar, "pointIndex", "Retrieves point from current geometry. Index is zerobased and must be below geomNPoints.", "", "", "", "", Category) \


#define NULARS_DEFAULT(XX, Category) \
  XX(GameArray, "geomComputeFocus", computeFocus, "Computes focus of current geometry.", "", "[x,y,z]", "", "", Category) \
  XX(EvalType_Matrix, "geomBestOrientation", bestOrientation,  "Computes best orientation of the geometry", "", "", "", "", Category) \
  XX(GameArray, "geomBestOrientationAndSize", bestOrientationAndSize,  "Computes best orientation and size of geometry", "", "[matrix,[xs,ys,zs]]", "", "", Category) \
  XX(GameArray, "geomNPoints", geomNPoints,  "Returns count of points of current geometry", "", "[matrix,[xs,ys,zs]]", "", "", Category) \

/*static GameOperator BinaryFuncts[]=
{
OPERATORS_DEFAULT(REGISTER_OPERATOR, Category)
};*/

static GameFunction UnaryFuncts[]=
{
  FUNCTIONS_DEFAULT(REGISTER_FUNCTION, Category)
};

static GameNular NularyFuncts[]=
{
  NULARS_DEFAULT(REGISTER_NULAR, Category)
};


#define TYPES_DEFAULT(XX, Category) \

/*static GameTypeType TypeDef[]=
{
TYPES_DEFAULT(REGISTER_TYPE,Category)
};*/

#if DOCUMENT_COMREF
static ComRefFunc DefaultComRefFunc[] =
{
  NULARS_DEFAULT(COMREF_NULAR, Category),
  FUNCTIONS_DEFAULT(COMREF_FUNCTION, Category)
    //    OPERATORS_DEFAULT(COMREF_OPERATOR, Category)
};

/*static ComRefType DefaultComRefType[] =
{
TYPES_DEFAULT(COMREF_TYPE, Category)
/*  TYPES_DEFAULT_COMB(COMREF_TYPE, "Default")
};*/
#endif

namespace TreeEngine
{


  void CScriptedPolyplane::RegisterToGameState(GameState *gState)
  {
    //gState->NewType(TypeDef[0]);
    //  gState->NewOperators(BinaryFuncts,lenof(BinaryFuncts));
    gState->NewFunctions(UnaryFuncts,lenof(UnaryFuncts));
    gState->NewNularOps(NularyFuncts,lenof(NularyFuncts));
#if DOCUMENT_COMREF
    gState->AddComRefFunctions(DefaultComRefFunc,lenof(DefaultComRefFunc));
    //  gState->AddComRefTypes(DefaultComRefType,lenof(DefaultComRefType));
#endif

  }
}