#ifndef _mytree_h_
#define _mytree_h_


#include "componentFern.h"
#include "componentTree.h"
#include "componentTest.h"
#include "componentScriptStem.h"
#include "componentScriptGutter.h"
#include "componentScriptStar.h"
#include "componentScriptTree.h"
#include "componentScriptTreeNode.h"
#include "componentScriptLeaf.h"
#include "componentScriptFunnel.h"
#include "componentScriptBunch.h"
#include "componentScriptEmpty.h"
#include "componentScriptHeel.h"
#include <Es/Strings/rString.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <Es/Containers/array.hpp>
#include "primitivestream.h"
#include "vectorSpace.h"
#include <projects/ObjektivLib/ObjectData.h>
#include <projects/ObjektivLib/LODObject.h>
#include <Es/Threads/PoCritical.hpp>
#include "TextureFactory.h"
#include "IPlantDB.h"
#include <map>

namespace TreeEngine
{
  #define MAX_OLDEST_BRANCHES 32


  struct SBoundingBox;
  struct SComponentChildS;
  class PlantTopologyInfo;

  struct SPolyplaneWithData;
  

  //! Output information about one node
  struct SNodeInformation {
    //! Index of the component associated with the node
    int _componentIndex;
    //! Importancy of the node
    float _importancy;
    //! Actual parameters
    SActualParams _actualParams;

    ///associated polyplane (if known)
    SPolyplaneWithData *_polyplane;         

    ClassIsMovable(SNodeInformation)
  };

  struct SPointInformation {
    Vector3 _point;
    float _importancy;
    ClassIsSimple(SPointInformation)
  };


  struct SPolyplaneWithData : public SVector {
    Ref<CPolyplane> _pPolyplane;
    AutoArray<SPointInformation> _importantPoints;
    bool _enlighted;  
    int _branchTypeId; 
    int _sourceNode;  //source node index
    int _usageCount; //count of branches used
    //SActualParams _ap;
    //int _componentIndex;
  };

  class IPlantPreview
  {
  public:
    ///Called when tree is in preview stable state
    /**
    @param depth current recursion depth
    @return function must return true, to continue generating, or false to stop
    */
    virtual bool Preview(int depth,CPrimitiveStream &branch,CPrimitiveStream &polyplane)=0;
  };


  template Ref<CComponent>;

  class IPolyplaneFactory
  {
  public:
    ~IPolyplaneFactory() {}
    virtual CPolyplane *CreatePolyplane()=0;
  };


  ///Common parameters for tree config
  struct TreeConfigCommon {
    float polyplaneCompareTreshold;
    int trianglesLimit;
    int textureSize;
    mutable int polyplaneCount;
    const DrawParameters &dp;
    float polyplaneBranchSelectTreshold;

    TreeConfigCommon(float polyplaneCompareTreshold,
                     int trianglesLimit,int textureSize,
                     const DrawParameters &dp,
                     float polyplaneBranchSelectTreshold)
               :polyplaneCompareTreshold(polyplaneCompareTreshold)
               ,trianglesLimit(trianglesLimit)
               ,textureSize(textureSize)
               ,dp(dp),polyplaneBranchSelectTreshold(polyplaneBranchSelectTreshold) {}
  };

  ///Configuration of tree, contains all paramaters
  struct TreeConfig: TreeConfigCommon {
    Matrix4Par origin;
    int seed;         
    float age;
    const SNodeInformation *startNode;

    TreeConfig(const TreeConfigCommon &base,Matrix4Par origin,
            int seed,float age,const SNodeInformation *startNode)
                :TreeConfigCommon(base),origin(origin),seed(seed)
                ,age(age),startNode(startNode) {}

    TreeConfig(Matrix4Par origin,
            int seed,float age,const SNodeInformation *startNode,
            float polyplaneCompareTreshold,int trianglesLimit,int textureSize,
             const DrawParameters &dp,float polyplaneBranchSelectTreshold)
                :TreeConfigCommon(polyplaneCompareTreshold,trianglesLimit,
                    textureSize,dp,polyplaneBranchSelectTreshold)
                ,origin(origin),seed(seed)
                ,age(age),startNode(startNode) {}
  };

  ///configuration for plant for one level
  struct PlantLevelConfig: virtual public TreeConfigCommon {
    AutoArray<SNodeInformation> *pNodeInfoList;
    CPointList *pPointList;

    PlantLevelConfig(const TreeConfigCommon &base,
                     AutoArray<SNodeInformation> *pNodeInfoList,
                     CPointList *pPointList)
                     :TreeConfigCommon(base),pNodeInfoList(pNodeInfoList),
                     pPointList(pPointList) {}

  };
  
  struct PlantConfig: public TreeConfig, public PlantLevelConfig {

      PlantConfig(const TreeConfig &tcfg, const PlantLevelConfig &levelcfg):
            TreeConfig::TreeConfigCommon(tcfg),TreeConfig(tcfg),PlantLevelConfig(levelcfg) {}

  };


  struct PlantDynaParams {
      const SConstantParams &cp;
      SActualParams ap;
      SInternalParams ip;
      int componentIndex;
      float importancy;
      int recursionDepth;
      bool polyplanesAreTemporary;
      mutable int *pPolyplaneType;

      PlantDynaParams(const SConstantParams &cp,
                    const SActualParams &ap,
                    const SInternalParams &ip,
                    int componentIndex,
                    float importancy,
                    int recursionDepth,
                    bool polyplanesAreTemporary,
                    int *pPolyplaneType)
        :cp(cp),ap(ap),ip(ip)
        ,componentIndex(componentIndex)
        ,importancy(importancy)
        ,recursionDepth(recursionDepth)
        ,polyplanesAreTemporary(polyplanesAreTemporary)
        ,pPolyplaneType(pPolyplaneType) {}
  };

  class CPlantType {
  private:
    //! Set of all branches. The last one is the root
    RefArray<CComponent> _components;
    //! Container for polyplanes
    TVectorSpace<SPolyplaneWithData> _polyplaneSpace;

    std::map<int,SRef<IPolyplaneFactory> >_polyplaneTypes;

    //! Returns 2 angles which contains information about matrix orientation
    /*!
      FLY note that the current implementation is horrible...
      \param matrix Orientation we want to retrieve the information from
      \param alpha Angle between the XZ plane and direction vector <-PI/2, PI/2>
      \param beta Rotation around the direction vector <-PI, PI>
    */
    void GetMetricsAngles(Matrix3Par matrix, float &alpha, float &beta);  

    IPlantPreview *_preview;

    ITextureFactory *_texFactory;

    GameState &_gState;

    int _polyplaneDetail;

    Ref<SPolyplaneWithData> CreatePolyplaneByType(int subtreePolyplaneType,int textureSize);

    CPointList _pointList;



  public:
    CPlantType(GameState &gState):_gState(gState),_polyplaneDetail(5000) {}
    //! Initialization
    void Init();
    //! Loading of a plant structure from the param file
    int LoadStructure(
      ITextureFactory *factory,
      RString folder,
      const ParamEntry &entry,
      Ref<CPrimitiveStream> &ps,
      bool resetPolyplanes
      );

    void RegisterPolyplaneFactory(int index, IPolyplaneFactory *factory);
    void ResetPolyplaneTypes();

    bool LoadStructure(ITextureFactory *factory, IPlantControlDB *dbsrc, bool resetPolyplanes);

    static CComponent *CreateComponentByType(const char *type, const char *script=0, GameState *gs=0);

/*    //! Create data of a specified tree
    void CreateStreamData(
      PDirect3DDevice &pD3DDevice,
      DWORD hVSColor,
      DWORD hPSColor,
      DWORD hVSNormal,
      DWORD hPSNormal,
      CPrimitiveStream &psBranch,
      CPrimitiveStream &psPolyplane,
      int seed,
      float detail,
      Matrix4Par origin,
      float age);
//     //! Plants geometry comparsion
//     float ComparePlants(
//       const SConstantParams &cp,
//       SActualParams apA,
//       int indexA,
//       SActualParams apB,
//       int indexB);
*/
    //! Function to generate the most important path
    void GenerateImportantPoints(const SConstantParams &cp, SActualParams ap, SInternalParams ip, int index, AutoArray<SPointInformation> &importantPoints);
    //! Function to generate all interesting points of the tree
    void GenerateAllPoints(const SConstantParams &cp, SActualParams ap, SInternalParams ip, int index, int componentNum, CPointList &pointList);
    float ComparePlantStructures(
      AutoArray<SPointInformation> &ipA,
      AutoArray<SPointInformation> &ipB);
    void CreatePlant(
      CPrimitiveStream &psBranch,
      CPrimitiveStream &psPolyplane,
      PlantConfig &plantConfiguration);

    void CreateStreamData2(
      CPrimitiveStream &psBranch,
      CPrimitiveStream &psPolyplane,
      const PlantDynaParams &dyn,
      const PlantLevelConfig &plantConfiguration);

    void SavePolyplaneSpace(RString fileName, RString suffix);
    void UpdateMergeFile(ParamClassPtr entry, RString className, RString path, RString filePrefix, RString fileSuffix);

    void Reset()
    {
      _components.Clear();
      _polyplaneSpace.Clear();
    }

    void SetPreviewInterface(IPlantPreview *pv) {_preview=pv;}

    void SetPolyplaneDetail(int detail) {_polyplaneDetail=detail;}

    int GetPolyplaneCount() const
    {
      return _polyplaneSpace.GetSize();
    }

    class IEnumTextures
    {
    public:
      virtual bool operator()(const ComRef<IDirect3DTexture8> &texture, float importance,bool temporary)=0;
    };
    void EnumPolyplaneTextures(IEnumTextures &funct, bool enumNormalMaps) const;
    const CComponent *GetComponentAtIndex(int index) const
    {
      if (index<0 || index>=_components.Size()) return 0;
      else return _components[index];
    }

    bool CreateBranchTopology(const SConstantParams &cp, const SComponentChild &childInfo,PlantTopologyInfo &topoInfo, float ageFactor);
    bool CreateBranchTopology(const SConstantParams &cp, const SComponentChild &childInfo,PlantTopologyInfo &topoInfo, float ageFactor, unsigned long atNode, bool mainBranch, int level);

    template<class Functor>
    bool EnumPolyplanes(const Functor &ff) const
    {
      for (int i=0;i<_polyplaneSpace.GetSize();i++)
        if (ff(*_polyplaneSpace.GetVectorArray()[i])) return true;
      return false;
    }

    static void RegisterCommands(GameState &gstate);

    RepresentativeArray GetClusterArray(int mode, float representativesPerm3) const;
    SBoundingBox GetBoundingBox() const;
  };


  class CTreeModel
  {
  protected:
    Ref<CPrimitiveStream> _psBranch;
    Ref<CPrimitiveStream> _psTriplane;
    ITextureFactory *_texFact;
  public:
    CTreeModel(Ref<CPrimitiveStream> psBranch, Ref<CPrimitiveStream> psTriplane, ITextureFactory *texFact):
        _psBranch(psBranch),_psTriplane(psTriplane),_texFact(texFact) {}
    CTreeModel() {}

    int Draw(D3DXMATRIX &matView,  D3DXMATRIX &matProj,  Vector3Par lightDirection, bool sceneBlok=true);
    int GetCountFaces() const
    {
      return (_psBranch->GetIndexCount()+_psTriplane->GetIndexCount())/3;
    }
    void Prepare()
    {
      _psBranch->Prepare();
      _psTriplane->Prepare();
    }
    void SortStages()
    {
      _psBranch->SortStages();
    }

    AutoArray<Vector3> ExportPointsForTexture(IDirect3DTexture8 *texture) const
    {
      AutoArray<Vector3> res1=_psBranch->ExportPointsForTexture(texture);
      AutoArray<Vector3> res2=_psTriplane->ExportPointsForTexture(texture);
      res1.Append(res2);
      return res1;
    }
    AutoArray<Vector3> ExportPointsForStage(bool polyplane, int index) const
    {
      AutoArray<Vector3> res1=(polyplane?_psTriplane:_psBranch)->ExportPointsForStage(index);
      return res1;
    }
    ComRef<IDirect3DTexture8> GetTextureAtIndex(bool polyplane, int index, bool normalMap)
    {
      return (polyplane?_psTriplane:_psBranch)->GetTextureAtIndex(index,normalMap);
    }
    bool IsStageUsed(bool polyplane, int stageIndex) const
    {
      return (polyplane?_psTriplane:_psBranch)->IsStageUsed(stageIndex);
    }
    bool IsUsed() const {
        return _psBranch.NotNull() && _psTriplane.NotNull();
    }
  };

  // Nacteni a spravovani vice typu stromu
  // Spravovani shaderu
  // Spravovani streamu (stromy, triplany)
  class CTreeEngine {
  private:

    //! Mutex to guard critical section
    PoCriticalSection _generate;  //!lock is non signaled during whole generate period
                                  

    //! Plant
    CPlantType _plantType;
    //! LOD object for storing of plant data
    LODObject _lo;
    //! One type of tree.
  //  CTreeType _TreeType;
/*    //! 3D Device.
    PDirect3DDevice _pD3DDevice;*/
    //! Stream of branch primitives.
    Ref<CPrimitiveStream> _pPSBranch;
    //! Stream of triplane primitives.
    Ref<CPrimitiveStream> _pPSTriplane;
    //! List of actual parameters
    AutoArray<SNodeInformation> _nodeInfoList;
    //! Pointer to texture factory
    ITextureFactory *_texFactory;

/*    //! VS handle for branch rendering.
    DWORD _hVSBranch;
    //! PS handle for branch rendering.
    DWORD _hPSBranch;
    //! VS handle for triplane rendering.
    DWORD _hVSTriplane;
    //! PS handle for triplane rendering.
    DWORD _hPSTriplane;
    //! VS handle for branch color map generation.
    DWORD _hVSBranchColor;
    //! PS handle for branch color map generation.
    DWORD _hPSBranchColor;
    //! VS handle for branch normal map generation.
    DWORD _hVSBranchNormal;
    //! PS handle for branch normal map generation.
    DWORD _hPSBranchNormal;
    //! VS handle for polyplane color map generation.
    DWORD _hVSPolyplaneColor;
    //! PS handle for polyplane color map generation.
    DWORD _hPSPolyplaneColor;
    //! VS handle for polyplane normal map generation.
    DWORD _hVSPolyplaneNormal;
    //! PS handle for polyplane normal map generation.
    DWORD _hPSPolyplaneNormal;*/
    //! Procedure for visualisation the matrix.
    void DrawMatrix(Matrix4Par Matrix);
    //! Procedure for visualisation the line.
    void DrawLine(Matrix4Par MatrixA, Matrix4Par MatrixB);

  public:
    CTreeEngine(GameState &gState);

    //! Initialization.
    void Init(ITextureFactory *texFactory);
    //! Loading of a tree structure from file.
    void LoadPlant(RString folder, const char *source, int length, bool resetPolyplanes=true);
    //! Loading of a tree structure via ParamFile
    void LoadPlant(RString folder, const ParamEntry &entry, bool resetPolyplanes=true);
    int LoadPlantFromFile(RString folder, RString fileName, bool resetPolyplanes=true);
    //! Returns the branch stream of primitives
    bool LoadPlant(IPlantControlDB *db, bool resetPolyplanes=true);
    

    Ref<CPrimitiveStream> GetPSBranch();
    void AddPolyplaneTextureNames(StringArray &names);
    int PreparePlantData(
        TreeConfig &plantConfiguration,
        IPlantPreview *preview=0);    

    int GetNearestNodeInfo(Vector3Par pos, Vector3Par dir, SNodeInformation &nodeInfo);

    void DeleteLODObjectFirstLevel();
    void DeleteLODObjectLevels();
    void DeleteLODObjectNonSpecLevels();

    void AddLevelToLODObject(float resolution, RString materialName);
    void AddSpecialLOD( int seed,  Matrix4Par origin,  float age,
                        float resolution,  int trianglesLimit,  int sphereDetail,  float sphereSizeCoef,
                        int wrapperComponentNum,  float mass,  ConstParamEntryPtr namedProperties);

    bool LoadLODObject(RString fileName);
    void SaveLODObject(RString fileName);
    void SavePolyplaneSpace(RString fileName, RString suffix);
    void UpdateMergeFile(ParamClassPtr entry, RString className, RString path, RString filePrefix, RString fileSuffix);

    void SetPolyplaneDetail(int detail)
    {
      _plantType.SetPolyplaneDetail(detail);
    }

    CTreeModel GetTreeModel();
    CTreeModel GetTreeModelForPreview();
    
    bool LockObject()
    {
      if (_generate.tryEnter()==false) return false;
      return true;
    }

    void UnlockObject()
    {
      _generate.leave();    
    }
    

    static void PrepareScriptCommands(GameState &gState);
    
    void Reset();
    ITextureFactory *GetCurrentTextureFactory() {return _texFactory;}
    const ITextureFactory *GetCurrentTextureFactory() const  {return _texFactory;}

    int GetPolyplaneCount() const
    {
      return _plantType.GetPolyplaneCount();
    }
  

    int GetNodeCount() const
    {
      return _nodeInfoList.Size();
    }


    void EnumPolyplaneTextures(CPlantType::IEnumTextures &funct, bool enumNormalMaps) const
    {
      _plantType.EnumPolyplaneTextures(funct,enumNormalMaps);
    }



    const CComponent *GetComponentAtIndex(int index) const
    {
      return _plantType.GetComponentAtIndex(index);
    }

    template<class Functor>
      bool EnumPolyplanes(const Functor &ff) const
    {
      return _plantType.EnumPolyplanes(ff);
    }

    bool GetNodeByIndex(int index, SNodeInformation &info) const;

    RepresentativeArray GetClusterArray(int mode, float representativesPerm3) const {
        return _plantType.GetClusterArray( mode,  representativesPerm3);
    }
    SBoundingBox GetBoundingBox() const {
        return _plantType.GetBoundingBox();
    }


};

}
#endif
