#include "RVTreeCommon.h"
#include "search.hpp"
#include "pointList.h"
#include "mytree.h"
#include "auxiliary.h"
#include "PlantTopologyInfo.h"
#include "IncidenceTable.h"
#include "UserPolyplane.h"
#include "polyplaneuniplane.h"
#include <el/MultiThread/Tls.h>
#include <vector>
#include <algorithm>




namespace TreeEngine
{
  #define ARROW_COEF 0.5f
  #define ARROW_SHARP 0.5f
  #define LINE_COEF 0.02f
  #define AVG_MAX_TRIANGLES 2000.0f

  #define TOPOLOGY_AGEDEPTHFACTOR 0.2f 

  DWORD dwBranchDecl[] =
  {
    D3DVSD_STREAM(0),
      D3DVSD_REG(D3DVSDE_POSITION,  D3DVSDT_FLOAT3),      // position
      D3DVSD_REG(D3DVSDE_NORMAL,    D3DVSDT_FLOAT3),      // normal
      D3DVSD_REG(D3DVSDE_DIFFUSE,   D3DVSDT_D3DCOLOR),    // diffuse color
      D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),      // texture coordinate
      D3DVSD_END()
  };

  //! Description of component child with shared vertices
  struct SComponentChildS : public SComponentChild, public TLinkBidir<>
  {
    //! Index of shared vertex
    int _sharedVertexIndex;
    //! Number of shared vertices
    int _sharedVertexCount;
  };

  //! Link list of simple component child structure
  struct SComponentChildL : public SComponentChild, public TLinkBidir<>
  {
  };

  void CPlantType::GetMetricsAngles(Matrix3Par matrix, float &alpha, float &beta) {

    // Get the alpha angle
    alpha = asin(matrix.Direction().Y());

    // Get the beta angle
    if ((matrix.Direction().X() == 0) && (matrix.Direction().Z() == 0)) {
      beta = 0.0f;
    }
    else {
      Matrix3 refMatrix;
      refMatrix.SetDirectionAndUp(matrix.Direction(), Vector3(0, 1, 0));
      //beta = matrix.DirectionUp().DotProduct(refMatrix.DirectionUp());
      Matrix3 invRefMatrix = refMatrix.InverseRotation();
      // In originMatrix will be the difference to the refMatrix (originMatrix.Direction() is supposed to be (0, 0, 1))
      Matrix3 originMatrix = invRefMatrix * matrix;
      // Get the angle of the Aside vector (rotated around Direction (Z))
      Vector3 v = originMatrix.DirectionAside();
      if (v.X() > 0.0f) {
        beta = atan(v.Y()/v.X());
      }
      else if (v.X() < 0.0f) {
        if (v.Y() < 0.0f) {
          beta = atan(v.Y()/v.X()) - H_PI;
        }
        else {
          beta = atan(v.Y()/v.X()) + H_PI;
        }
      }
      else {
        if (v.Y() < 0.0f) {
          beta = -H_PI/2;
        }
        else {
          beta = H_PI/2;
        }
      }
    }

  /*
    alpha = matrix.Direction().Y();
    // Get the angle between the direction and X axis in 2D
    float temp;
    float a;
    temp = matrix.Direction().SizeXZ();
    if (temp == 0.0f) {
      a = 0.0f;
    }
    else {
      a = acos(matrix.Direction().X() / temp);
    }
    // Get the rotated Z of the UP vector and store it in upZ
    Vector3 up(
      matrix.DirectionUp().Y(),
      0.0f, 
      matrix.DirectionUp().X() * sin(-a) + matrix.DirectionUp().Z() * cos(-a));
    // Get the angle from the transformed UP vector in the YZ plane
    temp = up.SizeXZ();
    if (temp == 0.0f) {
      beta = 0.0f;
    }
    else {
      beta = acos(up.X() / temp);
    }
  */
  }
  template<class T>
  class SPF: public IPolyplaneFactory
  {
  public:
    CPolyplane *CreatePolyplane()
    {
      return new T();
    }
  };

  template<class T, class P1>
  class SPF1: public IPolyplaneFactory
  {
      P1 p1;
  public:
    SPF1(P1 p1):p1(p1) {}
    CPolyplane *CreatePolyplane()
    {
      return new T(p1);
    }
  };

  class SPNoneF: public IPolyplaneFactory
  {
  public:
    CPolyplane *CreatePolyplane()
    {
      return 0;
    }
  };

  void CPlantType::Init() {
    //float interval[] = {0.15f, H_PI * 0.15f, 0.0f, 0.0f};
    float interval[] = {0.05f, H_PI * 0.15f, 0.0f, 0.0f};

  /*
    Matrix3 m;
    float alpha, beta;
    m.SetDirectionAndUp(Vector3(0, 0, 1), Vector3(1, 0, 0));
    GetMetricsAngles(m, alpha, beta);
  */
    _polyplaneSpace.Init(4, interval);
    _preview=0;
  }
  void CPlantType::ResetPolyplaneTypes()
  {
    _polyplaneTypes.clear();
    RegisterPolyplaneFactory(PT_BUNCH, new SPF<CPolyplaneBunch>());
    RegisterPolyplaneFactory(PT_BLOCKBEND, new SPF<CPolyplaneBlockBend>());
    RegisterPolyplaneFactory(PT_BLOCKNORM, new SPF<CPolyplaneBlockNorm>());
    RegisterPolyplaneFactory(PT_BLOCKSIMPLE, new SPF<CPolyplaneBlockSimple>());
    RegisterPolyplaneFactory(PT_BLOCKSIMPLE2, new SPF<CPolyplaneBlockSimple2>());
    RegisterPolyplaneFactory(PT_BLOCKBEND2, new SPF<CPolyplaneBlockBend2>());
    RegisterPolyplaneFactory(PT_BLOCKNORM2, new SPF<CPolyplaneBlockNorm2>());
    RegisterPolyplaneFactory(PT_BLOCK, new SPF<CPolyplaneBlock>());
    RegisterPolyplaneFactory(PT_UNIPLANE_X, new SPF1<CPolyplaneUniplane, CPolyplaneUniplane::Axe>(CPolyplaneUniplane::axe_x));
    RegisterPolyplaneFactory(PT_UNIPLANE_Y, new SPF1<CPolyplaneUniplane, CPolyplaneUniplane::Axe>(CPolyplaneUniplane::axe_y));
    RegisterPolyplaneFactory(PT_UNIPLANE_Z, new SPF1<CPolyplaneUniplane, CPolyplaneUniplane::Axe>(CPolyplaneUniplane::axe_z));
    RegisterPolyplaneFactory(PT_NONE, new SPNoneF());
  }

  //#include "picture.hpp"

  int CPlantType::LoadStructure(ITextureFactory *factory,
                                RString folder,
                                const ParamEntry &entry,
                                Ref<CPrimitiveStream> &ps,
                                bool resetPolyplanes
                                ) 
  {

    // Delete previousely created polyplanes
    if (resetPolyplanes) _polyplaneSpace.Clear();

    _texFactory=factory;

    // Allocate space for names and components
    AutoArray<RString> Names;
    int n = entry.GetEntryCount();
    if (n == 0) {
      LogF("There are no entries in the plant file.");
      return 0;
    }
    _components.Realloc(n);
    _components.Resize(n);
    Names.Realloc(n);
    Names.Resize(n);

    // Creation of the array of plant components
    for (int i = 0; i < n; i++) {
      const ParamEntryVal &SubEntry = entry.GetEntry(i);
      int index = SubEntry >> "ediIndex";
      if ((index < 0) || (index >= n)) {
        LogF("Index of component out of bounds");
        return 0;
      }
      Names[index] = SubEntry.GetName();
      RString ComponentType = SubEntry >> "type";

      _components[index]=CreateComponentByType(ComponentType,0,&_gState);
      if (_components[index]==0)
      {
        LogF("Error: Component %s is not implemented", ComponentType.Data());
      }
    }


    // Initialization of the array of branches
    for (int i = 0; i < n; i++) {
      const ParamEntryVal &SubEntry = entry.GetEntry(i);
      int index = SubEntry >> "ediIndex";
      if (_components[index] == NULL) {
        LogF("Component was not initialized.");
      }
      else {
        // Init the component
        _components[index]->Init(folder, entry, SubEntry, Names);

        int textureIndex;
        RString textureName;
        RString textureSaveName;
        _components[index]->GetTextureIndexAndName(textureIndex, textureName, textureSaveName);
        // Create and register texture
        ComRef<IDirect3DTexture8> texture;
        if (textureName != RString("")) 
        {
          HRESULT hr=factory->LoadTexure(textureName,texture,false);

/*          HRESULT hr = 
            D3DXCreateTextureFromFileEx(pD3DDevice, textureName,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, D3DX_FILTER_BOX | D3DX_FILTER_MIRROR,
            //D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, D3DX_FILTER_POINT | D3DX_FILTER_MIRROR, D3DX_FILTER_POINT | D3DX_FILTER_MIRROR,
            0, NULL, NULL, texture.Init());*/
          if (SUCCEEDED(hr)) {
            ETextureAdress adressU;
            ETextureAdress adressV;
            _components[index]->GetTextureAdress(adressU, adressV);
            D3DTEXTUREADDRESS d3d_tau;
            switch (adressU) {
            case MIRROR:
              d3d_tau = D3DTADDRESS_MIRROR;
              break;
            default:
              d3d_tau = D3DTADDRESS_WRAP;
            }
            D3DTEXTUREADDRESS d3d_tav;
            switch (adressV) {
            case MIRROR:
              d3d_tav = D3DTADDRESS_MIRROR;
              break;
            default:
              d3d_tav = D3DTADDRESS_WRAP;
            }
            bool alpha=false;
            const char *ext=Pathname::GetExtensionFromPath(textureName);
            ext-=3;
            if (ext>textureName.Data() && _strnicmp(ext,"_CA.",4)==0) alpha=true;
            ps->RegisterTextures(textureIndex, texture, textureName, ComRef<IDirect3DTexture8>(), textureSaveName, d3d_tau, d3d_tav,alpha);
  /*
            int levelCount = texture->GetLevelCount();
            for (int j = 0; j < levelCount; j++) {
              ComRef<IDirect3DSurface8> surface;
              texture->GetSurfaceLevel(j, surface.Init());
              char tname[256];
              sprintf(tname, "%s%d.paa", textureName.Data(), j);

              D3DSURFACE_DESC desc;
              surface->GetDesc(&desc);

              D3DLOCKED_RECT lr;
              surface->LockRect(&lr, NULL, D3DLOCK_READONLY);
              PPixel *pix = (PPixel*) lr.pBits;

              // Create picture
              Picture pic;
              pic.Create(desc.Width, desc.Height);

              int pixNum = 0;
              for (int y = 0; y < desc.Height; y++) {
                for (int x = 0; x < desc.Width; x++) {
                  PPixel pixel = pix[pixNum];
                  pic.SetPixel(x, y, pixel);
                  pixNum++;
                }
              }

              // Unlock the texture
              surface->UnlockRect();

              pic.Save(tname);


              //D3DXSaveSurfaceToFile(tname, D3DXIFF_BMP, surface, NULL, NULL);
            }
  */
          }

        }
      }
    }
    return 1;
  }

  static D3DTEXTUREADDRESS GetTexAddr(IPlantControlDB::TextureDesc::AddrMode mode)
  {
    switch (mode)
    {
    case IPlantControlDB::TextureDesc::Clamp: return D3DTADDRESS_CLAMP;
    case IPlantControlDB::TextureDesc::Wrap: return D3DTADDRESS_WRAP;
    case IPlantControlDB::TextureDesc::Mirror: return D3DTADDRESS_MIRROR;
    default: return D3DTADDRESS_WRAP;
    }
  }

  CComponent *CPlantType::CreateComponentByType(const char *ComponentType, const char *script, GameState *gs)
  {
    if (_stricmp(ComponentType, "fern") == 0)
      return new CCFern();
    else if (_stricmp(ComponentType, "tree") == 0)
      return new CCTree();
    else if (_stricmp(ComponentType, "test") == 0)
      return new CCTest();
    else if (_stricmp(ComponentType, "scriptStem") == 0)
      return new CCScriptStem(*gs,script);
    else if (_stricmp(ComponentType, "scriptGutter") == 0)
      return new CCScriptGutter(*gs,script);
    else if (_stricmp(ComponentType, "scriptStar") == 0)
      return new CCScriptStar(*gs,script);
    else if (_stricmp(ComponentType, "scriptTree") == 0)
      return new CCScriptTree(*gs,script);
    else if (_stricmp(ComponentType, "scriptTreeNode") == 0)
      return new CCScriptTreeNode(*gs,script);
    else if (_stricmp(ComponentType, "scriptLeaf") == 0)
      return new CCScriptLeaf(*gs,script);
    else if (_stricmp(ComponentType, "scriptFunnel") == 0)
      return new CCScriptFunnel(*gs,script);
    else if (_stricmp(ComponentType, "scriptBunch") == 0)
      return new CCScriptBunch(*gs,script);
    else if (_stricmp(ComponentType, "scriptEmpty") == 0)
      return new CCScriptEmpty(*gs,script);
    else if (_stricmp(ComponentType, "scriptSlice") == 0)
      return new CCScriptSlice(*gs,script);
    else if (_stricmp(ComponentType, "scriptHeel") == 0)
      return new CCScriptHeel(*gs,script);
    return 0;

  }


  bool CPlantType::LoadStructure(ITextureFactory *factory, IPlantControlDB *dbsrc, bool resetPolyplanes)    
  {
    // Delete previousely created polyplanes
    if (resetPolyplanes) _polyplaneSpace.Clear();

    _texFactory=factory;

    // Allocate space for names and components
    AutoArray<RString,MemAllocLocal<RString,100> > Names;
    _components.Clear();

    for (int i=dbsrc->GetFirstComponentId();i;)
    {
      IPlantControlDB::ComponentDesc cdesc=dbsrc->GetNextComponent(i);

      RString scr;
      if (cdesc.script.GetLength())
        scr=LoadAndPreprocessRStringFromFile(cdesc.script);
      CComponent *nw=CreateComponentByType(cdesc.type,scr,&_gState);

      if (nw==0)
      {
        LogF("Error: Unknown component type %s - %s", cdesc.type.Data(), cdesc.name.Data());
        continue;
      }      

      Names.Add(cdesc.name);
      nw->SetTextureIndex(cdesc.defIndex);
      nw->SetComponentName(cdesc.name);
      _components.Add(nw);
    }

    for (int i=0;i<_components.Size();i++)
    {
      _components[i]->InitWithDb(dbsrc,Names);
    }

    Names.Compact();
    _components.Compact();

    return true;
  }

  
  void CPlantType::GenerateImportantPoints(const SConstantParams &cp,
                                           SActualParams ap,
                                           SInternalParams ip,
                                           int index,
                                           AutoArray<SPointInformation> &importantPoints)
  {

    int currIndex = index;
    SActualParams currAp = ap;
    SInternalParams currIp = ip;
    importantPoints.Clear();

    do {

      // Get children of both plants both children
      CComponentChildBuffer ccb;
      if (!_components[currIndex]->GetChildren(currAp, currIp, cp, ccb)) {
        LogF("Error in component '" + _components[0]->GetComponentName(currIndex) + RString("'."));
        return;
      };

      if (ccb.GetChildCount() > 0) {
        
        // Find the most important child
        int mostImportantIndex;
        float mostImportantValue;
        mostImportantIndex = 0;
        mostImportantValue = ccb.GetChild(0)->_importancy;
        for (int i = 1; i < ccb.GetChildCount(); i++) {
          if (ccb.GetChild(i)->_importancy > mostImportantValue) {
            mostImportantIndex = i;
            mostImportantValue = ccb.GetChild(i)->_importancy;
          }
        }
        currAp = ccb.GetChild(mostImportantIndex)->_actualParams;
        currIp = ccb.GetChild(mostImportantIndex)->_internalParams;
        currIndex = ccb.GetChild(mostImportantIndex)->_componentIndex;
        Assert(currIndex >= 0);

        // Save child position
        SPointInformation &newPoint = importantPoints.Append();
        newPoint._point = currAp._origin.Position();
        newPoint._importancy = mostImportantValue;
        //importantPoints.Add(currAp._origin.Position());
      }
      else {
        break;
      }
    } while (1);
  }

  /**
  void CPlantType::GenerateAllPoints(const SConstantParams &cp,
                                     SActualParams ap,
                                     SInternalParams ip,
                                     int index,
                                     int componentNum,
                                     CPointList &pointList)
  {
    pointList.Clear();

    // Link list of components
    TListBidir<SComponentChildL> childList;

    // Prepare the first child
    SComponentChildL *pCCL = new SComponentChildL();
    pCCL->_componentIndex = index;
    pCCL->_importancy = FLT_MAX;
    pCCL->_actualParams = ap;
    pCCL->_internalParams = ip;

    // Insert the first child
    childList.Insert(pCCL);

    // Getting the geometry of the tree
    while ((!childList.Empty()) && (pointList.NPoints() < componentNum))
    {

      // Get first item from the queue
      pCCL = childList.Start();    

      // Get children of the branch set the primitive
      CComponentChildBuffer ccb;
      if (!_components[pCCL->_componentIndex]->GetChildren(pCCL->_actualParams, pCCL->_internalParams, cp, ccb))
      {
        LogF("Error in component '" + _components[0]->GetComponentName(pCCL->_componentIndex) + RString("'."));
        while (!childList.Empty()) childList.Start()->Delete();
        return;
      };

      // Draw the component primitive
      CPrimitive *pPrimitive = _components[pCCL->_componentIndex]->GetPrimitive();
      if (pPrimitive)
      {
        pPrimitive->UpdatePointList(pointList);
      }

      // Remove item from list and delete it
      delete pCCL;

      // Insert the children in the queue at the proper place
      for (int i = 0; i < ccb.GetChildCount(); i++)
      {

        // Get the i-th child
        SComponentChild *pCC = ccb.GetChild(i);

        // Test the validity of the component index
        if (pCC->_componentIndex >= 0)
        {

          // Fill the parameters of the i-th child
          pCCL = new SComponentChildL();
          pCCL->_componentIndex = pCC->_componentIndex;
          pCCL->_importancy = pCC->_importancy;
          pCCL->_actualParams = pCC->_actualParams;
          pCCL->_internalParams = pCC->_internalParams;

          // Sort it into the list from the end of the list according to its importancy
          SComponentChildL *pCurr = childList.End();
          while ((childList.NotEnd(pCurr)) && (pCurr->_importancy < pCCL->_importancy))
          {
            pCurr = childList.Retreat(pCurr);
          }
          childList.InsertAfter(pCurr, pCCL);
        }
      }
    }
  }

  */
  float CPlantType::ComparePlantStructures(AutoArray<SPointInformation> &ipA,
                                          AutoArray<SPointInformation> &ipB) {

    if (ipA.Size() > 0) {
      if (ipB.Size() > 0) {
        int maxSize = max(ipA.Size(), ipB.Size());
        int maxIpA = ipA.Size() - 1;
        int maxIpB = ipB.Size() - 1;
        float difference = 0.0f;
        for (int i = 0; i < maxSize; i++) {
          //float avgImportancy = (ipA[min(i, maxIpA)]._importancy + ipB[min(i, maxIpB)]._importancy) * 0.5f;
          difference += /*avgImportancy * */ipA[min(i, maxIpA)]._point.Distance(ipB[min(i, maxIpB)]._point);
        }
        return difference / ((float)maxSize);
      }
      else {
        return FLT_MAX;
      }
    }
    else {
      if (ipB.Size() > 0) {
        return FLT_MAX;
      }
      else {
        return 0;
      }
    }

  }

 static TLS(CPlantType) SCurrentPlant;
                    
  void CPlantType::CreatePlant(CPrimitiveStream &psBranch,
                              CPrimitiveStream &psPolyplane,
                              PlantConfig &config)

  {
    SCurrentPlant=this;
    ResetPolyplaneTypes();
    ///get start component - deprecated style
    int start=_components.Size()-1;
    /// negative start is error
    if (start<0) return;
    /// get tree database
    IPlantControlDB *db=_components[start]->GetParamDatabase();
    /// is there active database
    if (db)
    {
      ///ask database for start component
      RString sname=db->GetStartComponent();
      ///no active component
      db->SetActiveComponentName(0);
      ///find start component in component list
      start=_components[start]->GetIndexOfName(sname);      
    }
    ///negative start - no start component, exit now
    if (start<0) return;

    // Init constant parameters
    SConstantParams cp;
    cp._detail = 1.0f;
    cp._time = 0.0f;
    cp._month = 0.0f;
    cp._plantAge = config.age;

    float startImportancy=FLT_MAX;

    // Init actual parameters
    SActualParams ap;
    if (config.startNode)
    {
      ap=config.startNode->_actualParams;
      start=config.startNode->_componentIndex;
      startImportancy=config.startNode->_importancy;
    }
    else
    {    
      ap._origin = config.origin;
      ap._seed = config.seed;
      ap._age = config.age;
      ap._counter = 0;
      ap._diaOrigin = config.origin.Position();
      ap._textureVOffset = 0.0f;
      ap._geometrySpin = 0.0f;
      ap._branchTypeId=0;
      ap._polyplaneType=PT_NOTDEFINED; //not set
    }

    // Init internal parameters
    SInternalParams ip;
    ip._isRoot = false;

    _pointList.Clear();

    PlantLevelConfig lcfg(config);
    lcfg.pPointList = &_pointList;
    

    // Create the plant data
    CreateStreamData2(psBranch,      
                      psPolyplane,      
                      PlantDynaParams(cp,ap,ip,start,startImportancy,0,false,0),
                      lcfg);
                          
    if (config.pPointList)
        *config.pPointList = _pointList;
  }


  Ref<SPolyplaneWithData> CPlantType::CreatePolyplaneByType(int subtreePolyplaneType,int textureSize)
  {
    Ref<SPolyplaneWithData> polyplaneWithData = new SPolyplaneWithData();

    std::map<int,SRef<IPolyplaneFactory> >::const_iterator iter=_polyplaneTypes.find(subtreePolyplaneType);
    if (iter==_polyplaneTypes.end())
    {
      LogF("Unknown polyplane type: %d",subtreePolyplaneType);
      return 0;
    }
    CPolyplane *p=(*iter).second->CreatePolyplane();
    if (!p)  return 0;
    
    polyplaneWithData->_pPolyplane=p;

    // Initialize the polyplane
    if (FAILED(polyplaneWithData->_pPolyplane->Init(_texFactory,textureSize)))
    {
      LogF("Failed to initialize a polyplane.");
      return 0;
    }
    return polyplaneWithData;
  }

  bool CPlantType::CreateBranchTopology(const SConstantParams &cp, const SComponentChild &childInfo,PlantTopologyInfo &topoInfo, float ageFactor)
  {
    unsigned int root=topoInfo.AddRootNode();
//    LogF("StartTopo");
    return CreateBranchTopology(cp,childInfo,topoInfo,ageFactor,root,true,0);    
  }

  static const char *LevelDots(int level)
  {
    static char buff[200];
    if (level>198) level=198;
    for (int i=0;i<level;i++) buff[i]='.';
    buff[level]=0;
    return buff;
  }
  

  static float AgeOrImportancy(const SComponentChild &child) {
      return (child._actualParams._age + child._importancy);
  }

  bool CPlantType::CreateBranchTopology(const SConstantParams &cp, const SComponentChild &childInfo,PlantTopologyInfo &topoInfo, float ageFactor, unsigned long atNode, bool mainBranch, int level)
  {
    if (level>1000)
    {
      LogF("Too long branch - stack overflow");
      return true;
    }
    if (static_cast<ProgressBar<int> *>(ProgressBarFunctions::GetGlobalProgressBarInstance()->GetTopProgressBar())->StopSignaled())
    {
      LogF("Stop signaled");
      return false;
    }
    topoInfo.SetNodeInfo(atNode,childInfo);
    CComponentChildBuffer ccb;
    if (!_components[childInfo._componentIndex]->GetChildren(childInfo._actualParams, childInfo._internalParams, cp, ccb)) 
    {
      LogF("Error in component '" + _components[0]->GetComponentName(childInfo._componentIndex) + RString("'."));
      return false;
    }


    if (ccb.GetChildCount() == 0)
        return true;

    if (ccb.GetChildCount() == 1) {
        topoInfo.AllocateNodeChildren(atNode,1);
        return CreateBranchTopology(cp,*ccb.GetChild(0),
            topoInfo,ageFactor,topoInfo.GetNodeChildId(atNode,0),
            mainBranch,level+1);
    }
        

    typedef std::pair<float, int> ChildFloatIndex;
    typedef std::vector<ChildFloatIndex> ChildList;
    ChildList childList;
    childList.reserve(ccb.GetChildCount());

    for (int i=0;i<ccb.GetChildCount();i++) 
        childList.push_back(ChildFloatIndex(AgeOrImportancy(*ccb.GetChild(i)),i));

    ChildList::iterator beg = childList.begin(), end = childList.end();
    std::less<ChildFloatIndex> cmp;

    std::make_heap(beg,end,cmp);
    std::pop_heap(beg,end,cmp);
    --end;
    ChildFloatIndex idx = *end;
    float maxAge = idx.first;
    float threshold = maxAge * ageFactor;    
    while (idx.first >= threshold && beg != end) {
        std::pop_heap(beg,end,cmp);
        --end;
        idx = *end;
    }

    if (idx.first < threshold) ++end;
    int acceptCount = childList.end() - end;
    topoInfo.AllocateNodeChildren(atNode,acceptCount );

    for (int i = 0; i < acceptCount; i++,++end) {
        if (CreateBranchTopology(cp,*ccb.GetChild(end->second),topoInfo,
            ageFactor,topoInfo.GetNodeChildId(atNode,i),false,level+1)==false) return false;
    }


    /*
    //    LogF("%s%d",LevelDots(level),ccb.GetChildCount());
    float bestAge=0;
    for (int i=0;i<ccb.GetChildCount();i++) if (ccb.GetChild(i)->_actualParams._age>bestAge)
      bestAge=ccb.GetChild(i)->_actualParams._age;
    for (int i=0;i<ccb.GetChildCount();i++)
    {
      bool isMainBranch=mainBranch && bestAge==ccb.GetChild(i)->_actualParams._age;
      if (CreateBranchTopology(cp,*ccb.GetChild(i),topoInfo,ageFactor,topoInfo.GetNodeChildId(atNode,i),isMainBranch,level+1)==false) return false;
    }
    */
    return true;
  }

  static Matrix4 CalcPolyplaneOrigin(const Matrix4 apOrigin)
  {
    Matrix4 polyplaneOrigin;
    //Matrix4 normalizedApOrigin;
    if ((apOrigin.Direction().X() == 0.0f) && (apOrigin.Direction().Z() == 0.0f)) {
      polyplaneOrigin = apOrigin;
      //normalizedApOrigin = apOrigin;
    }
    else {
      // New polyplane origin
      polyplaneOrigin.SetDirectionAndUp(apOrigin.Direction(), Vector3(0, 1, 0));
      polyplaneOrigin.SetPosition(apOrigin.Position());
    }
    return polyplaneOrigin;
  }

  static void LevelLogF(int level, const char *text, ...)
  {
    char buff[512];
    for (int i=0;i<level;i++) buff[i]='\t';
    va_list vl;
    va_start(vl,text);
    _vsnprintf(buff+level,lenof(buff)-level,text,vl);
    LogF("%s",buff);
  }

  void CPlantType::CreateStreamData2(CPrimitiveStream &psBranch, 
    CPrimitiveStream &psPolyplane, 
    const PlantDynaParams &dyn,
    const PlantLevelConfig &pc
    )
  {

    bool fastSearch=fabs(pc.polyplaneCompareTreshold)<0.001f;
    LevelLogF(dyn.recursionDepth,"Entering to level %d...",dyn.recursionDepth);

#ifdef FAST_POLYPLANE_SEARCH
    fastSearch=true;
#endif

    ProgressBar<int> progressBar(1000); 

    if (progressBar.StopSignaled()) return;

    // Check the depth of recursion
    if (dyn.recursionDepth > 32)
    {
      LevelLogF(dyn.recursionDepth,"Recursion depth limit exceeded.");
      return;
    }

    // Link list of components
    TListBidir<SComponentChildS> childList;

    // Prepare the first child
    SComponentChildS *pCCS = new SComponentChildS();
    pCCS->_componentIndex = dyn.componentIndex;
    pCCS->_importancy = dyn.importancy;
    pCCS->_sharedVertexCount = 0;
    pCCS->_actualParams = dyn.ap;
    pCCS->_internalParams = dyn.ip;

    // Set the polyplane type to default value
    if (dyn.pPolyplaneType) *dyn.pPolyplaneType = PT_BLOCKSIMPLE2;
    bool polyplaneTypeIsSet = false;

    
    // Insert the first child
    childList.Insert(pCCS);    

    // Getting the geometry of the tree
    int trianglesCount = 0;
    int polyplanesCount = 1;    
    int totalcmp=1;
    int PolyStageIndex=0;

    int pbcomplex=_texFactory->GetRenderPlaneComplexFactor();

    progressBar.SetPos(1);
    SComponentChildS *beginCycle=0;

    LevelLogF(dyn.recursionDepth,"Building mesh and nodes...");

    while ((!childList.Empty()) && !progressBar.StopSignaled())
    {    

      //&& (trianglesCount + (polyplanesCount * POLYPLANE_TRIANGLE_COUNT) < trianglesLimit) && ) {

      // Get first item from the queue
      pCCS = childList.Start();   
      // if this item was already processed, and no other items was processed after
      if (beginCycle==pCCS) break; // exit cycle and process remain items as polyplanes.

      // test, if there is enough faces to create this node.
      // if this node has low faces, than polyplanes, it can be realized.
      //otherwise, it is skipped
      //if polyplane forced, it is postponed and will be processed during polyplanize section
      if ((trianglesCount + (polyplanesCount * POLYPLANE_TRIANGLE_COUNT) > pc.trianglesLimit 
          && _components[pCCS->_componentIndex]->EstimateFacesCount()>POLYPLANE_TRIANGLE_COUNT) 
          || (pCCS->_actualParams._forcePolyplane && !pc.dp._ignoreForcedPolyplanes && dyn.recursionDepth==0))
      {
        //Remove item from queue
        pCCS->Delete();
        //if begin of cycle is not marked, mark it now
        if (beginCycle==0) beginCycle=pCCS;
        //insert item at the end of queue
        childList.InsertAfter(childList.End(),pCCS);
      }
      else
      {
        // remove begin of cycle.
        beginCycle=0;

        progressBar.SetMax(totalcmp*pbcomplex+pc.trianglesLimit);
        progressBar.SetPos(trianglesCount);

        // Get children of the branch set the primitive
        CComponentChildBuffer ccb;
        if (!_components[pCCS->_componentIndex]->GetChildren(pCCS->_actualParams, pCCS->_internalParams, dyn.cp, ccb))
        {
          LevelLogF(dyn.recursionDepth,"Error in component '" + _components[0]->GetComponentName(pCCS->_componentIndex) + RString("'."));
          while (!childList.Empty()) childList.Start()->Delete();
          return;
        };

        // Update number of actual polyplanes
        polyplanesCount += ccb.GetChildCount() - 1;

        // Draw the component primitive, fill the slots
        SSlot slots[MAX_SLOT_COUNT];
        for (int i = 0; i < MAX_SLOT_COUNT; i++)
        {
          slots[i]._isRoot = pCCS->_internalParams._isRoot;
        }
        CPrimitive *pPrimitive = _components[pCCS->_componentIndex]->GetPrimitive();
        if (pPrimitive) {
          trianglesCount += pPrimitive->Draw(
            psBranch,
            pCCS->_sharedVertexIndex,
            pCCS->_sharedVertexCount,
            pCCS->_internalParams._isRoot,
            ((float)pc.trianglesLimit) / AVG_MAX_TRIANGLES,
            slots);

          if (pc.pPointList) {
            pPrimitive->UpdatePointList(*pc.pPointList);
          }

          if ((!polyplaneTypeIsSet) && (dyn.pPolyplaneType)) {
            *dyn.pPolyplaneType = pPrimitive->GetPolyplaneType();
            polyplaneTypeIsSet = true;
          }
        }

        // Save actual parameters
        if (pc.pNodeInfoList != NULL) {
          SNodeInformation &newNode = pc.pNodeInfoList->Append();
          newNode._componentIndex = pCCS->_componentIndex;
          newNode._importancy = pCCS->_importancy;
          newNode._actualParams = pCCS->_actualParams;
          newNode._polyplane = 0;
        }

        // Remove item from list and delete it
        delete pCCS;

        // Insert the children in the queue at the proper place
        for (int i = 0; i < ccb.GetChildCount(); i++)
        {
          // Get the i-th child
          SComponentChild *pCC = ccb.GetChild(i);

          // Test the validity of the component index
          if (pCC->_componentIndex >= 0)
          {

            // Fill the parameters of the i-th child
            pCCS = new SComponentChildS();
            pCCS->_componentIndex = pCC->_componentIndex;
            pCCS->_importancy = pCC->_importancy;
            if (pCC->_slotIndex >= 0)
            {
              pCCS->_sharedVertexIndex = slots[pCC->_slotIndex]._sharedVertexIndex;
              pCCS->_sharedVertexCount = slots[pCC->_slotIndex]._sharedVertexCount;
              pCCS->_internalParams._isRoot = slots[pCC->_slotIndex]._isRoot;              
            }
            else
            {
              pCCS->_sharedVertexCount = 0;
              pCCS->_internalParams._isRoot = false;
            }
            pCCS->_actualParams = pCC->_actualParams;

            // Sort it into the list from the end of the list according to its importancy
            SComponentChildS *pCurr = childList.End();
            while ((childList.NotEnd(pCurr)) && (pCurr->_importancy < pCCS->_importancy))
            {
              pCurr = childList.Retreat(pCurr);
            }
            childList.InsertAfter(pCurr, pCCS);
            totalcmp++;
          }
        }
        totalcmp--;
        if (_preview) _preview->Preview(dyn.recursionDepth,psBranch,psPolyplane);
      }
    }

    progressBar.SetMax(totalcmp*pbcomplex+pc.trianglesLimit+pc.trianglesLimit);
    progressBar.SetPos(pc.trianglesLimit);

    // Get the polyplane importancy treshold
    float polyplaneImportancyTreshold  = -1;
    if (!dyn.polyplanesAreTemporary)
    {
      pCCS = childList.First();
      if (pCCS != NULL)
      {
        polyplaneImportancyTreshold = pCCS->_importancy;
      }
    }

    

#ifdef LINDASTUDIO    
    if (!fastSearch)
    {
      float metrics=pc.polyplaneCompareTreshold;
      int enforcePolyCnt=0;
      if (metrics<0.0f) 
      {
        enforcePolyCnt=toInt(floor(-metrics));
        metrics=1000.0f;
      }
      int polyCount=childList.Size();
      LevelLogF(dyn.recursionDepth,"Exploring unused nodes...(%d)",polyCount);
      SRef<SComponentChildS> *childPolyplanes=new SRef<SComponentChildS>[polyCount];
      PlantTopologyInfo *topoList=new PlantTopologyInfo[polyCount];
      int *nodeIndexes = new int[polyCount];
      int nodeBaseIndex=pc.pNodeInfoList?pc.pNodeInfoList->Size():0;

      int i=0;
      {      
        progressBar.AdvanceNext(pbcomplex/4*polyCount); 
        ProgressBar<int> subpb(polyCount);
        //create list of remaining polyplanes
        while (!childList.Empty() && !progressBar.StopSignaled())
        {
          subpb.AdvanceNext(1);
          pCCS=childList.Start();   
          _components[pCCS->_componentIndex]->CloseBranchTip(psBranch, pCCS->_sharedVertexIndex, pCCS->_sharedVertexCount);
          if (pCCS->_actualParams._polyplaneType==PT_NONE)
          {
            delete pCCS;
            continue;
          }
          childPolyplanes[i]=pCCS;
          pCCS->Delete();

          bool res=CreateBranchTopology(dyn.cp,*pCCS,topoList[i],pc.polyplaneBranchSelectTreshold); 
//          LogF("Topology Size: %d",topoList[i].GetCountOfNodes());
          topoList[i].FinalizeTopology(pCCS->_actualParams._origin);
//          topoList[i].dumpTopology("t:\\dump.txt",i,i!=0);

          if (pc.pNodeInfoList != NULL) {
            nodeIndexes[i] = pc.pNodeInfoList->Size();
            SNodeInformation &newNode = pc.pNodeInfoList->Append();
            newNode._componentIndex = pCCS->_componentIndex;
            newNode._importancy = pCCS->_importancy;
            newNode._actualParams = pCCS->_actualParams;
            newNode._polyplane = 0;
          }

          i++;
          if (_preview) _preview->Preview(dyn.recursionDepth,psBranch,psPolyplane);
          if (res==false) 
          {
            polyCount=0;
            break;
          }
        }
      }
      if (polyCount!=i)
      {
        LevelLogF(dyn.recursionDepth,"%d unnecessary node(s) removed.",polyCount-i);
        polyCount=i;
      }
      if (!progressBar.StopSignaled() && polyCount)
      {      
        IncidenceTableBestGroups incTable(polyCount);
        class IncOnDemand: public IInciTableOnDemandCalc
        {
            mutable IncidenceTableBestGroups *table;
            mutable PlantTopologyInfo *topoList;
            float metrics;
            mutable ProgressBar<int> *pb;
            const TopologyWeights &weights;
        public:
            IncOnDemand(IncidenceTableBestGroups *table,
                PlantTopologyInfo *topoList, float metrics,
                const TopologyWeights &weights)
                :table(table),topoList(topoList),
                metrics(metrics),weights(weights) {}
            
            virtual float Get(int a, int b) const
            {
                if (pb) pb->AdvanceCur(1);
                if (a == b) return 0.0f;
                if (a > b) return table->Get(b,a);
                float diff1=topoList[a].GetDifferenceFactor(topoList[b],weights,metrics);
              //            if (diff1>10000.0f) DebugBreak();
                float diff2=topoList[b].GetDifferenceFactor(topoList[a],weights,metrics);
              //            if (diff2>10000.0f) DebugBreak();
                float resDiff=__max(diff1,diff2);
                return resDiff;
            }
            void AttachPb(ProgressBar<int> *pb) {this->pb = pb;}
            void DetachPb() {this->pb = NULL;}
        };

        int grmax;
        int groups;

        TopologyWeights topoweights;
        topoweights.LoadWeightsFromDB(*_components[0]->GetParamDatabase());
        IncOnDemand onDemand(&incTable,topoList,metrics,topoweights);

        {        

            ProgressBar<int> pb(incTable.Size()*incTable.Size());
            onDemand.AttachPb(&pb);
            incTable.SetOnDemand(&onDemand);

            for (int i=0;i<polyCount;i++) if (!progressBar.StopSignaled())
            {
              if (_components[childPolyplanes[i]->_componentIndex]->ForceNewPolyplane())
              {
                LevelLogF(dyn.recursionDepth,"%3d - force new polyplane",i);
                for (int j=0;j<polyCount;j++) incTable.Set(i,j,FLT_MAX/2);
              }
            }
            
            progressBar.AdvanceNext(pc.trianglesLimit);

            if (enforcePolyCnt)
            {           
              LevelLogF(dyn.recursionDepth,"Selecting metrics...");
              float step=metrics;
              grmax=incTable.CreateGroups(metrics);
              groups=incTable.GetTrueGroupCount();
              while (step>0.0001f)
              {
                step/=2.0f;
                if (groups>enforcePolyCnt) metrics+=step;
                if (groups==enforcePolyCnt && step<0.01f) break;
                if (groups<=enforcePolyCnt) metrics-=step;            
                grmax=incTable.CreateGroups(metrics);
                groups=incTable.GetTrueGroupCount();
              }
              LogF("Selected metric: %g",metrics);
            }
            else 
            {
                grmax=incTable.CreateGroups(metrics);
                groups=incTable.GetTrueGroupCount();
            }

              LogF("Creating groups: %g",metrics);
    //        DebugBreak();
            float maxMetric=metrics*100.0f;
            float minMetric=metrics*0.01f;
              while (groups>=20)
              {
                if (dyn.recursionDepth==0 || metrics>maxMetric) break;
                LevelLogF(dyn.recursionDepth,"Too many polyplanes. Relaxing the metric to %0.1f.",metrics);          
                metrics=metrics*1.1f;
                grmax=incTable.CreateGroups(metrics);
                groups=incTable.GetTrueGroupCount();
              }
              while (groups<6 && dyn.recursionDepth>0 && metrics>minMetric)
              {
                LevelLogF(dyn.recursionDepth,"Too less polyplanes for rendering into polyplane. Stricting the metric to %0.1f.",metrics);
                metrics=metrics*0.95f;
                grmax=incTable.CreateGroups(metrics);
                groups=incTable.GetTrueGroupCount();
              }
            onDemand.DetachPb();
          }
        int *groupBranchList=(int *)alloca(sizeof(int)*polyCount);
        int bestPoint;
        int remain=polyCount;
        int counter=0;
        LevelLogF(dyn.recursionDepth,"%d groups prepared...",groups);
        for (int gr=0;gr<grmax && !progressBar.StopSignaled() ;gr++)
        {          
          progressBar.AdvanceNext((progressBar.GetMax()-progressBar.GetPos())/(grmax-gr));
          int pointCnt=incTable.GetGroup(gr,bestPoint,groupBranchList);
          if (pointCnt==0) 
            continue;
          remain-=pointCnt;
          pCCS = childPolyplanes[bestPoint];
          Matrix4 polyplaneOrigin=CalcPolyplaneOrigin(pCCS->_actualParams._origin);
          float sizeCoef=topoList[bestPoint].GetSizeCoeficient();
          float polyplaneVector[4]={0,0,0,0};

          Ref<SPolyplaneWithData> polyplaneWithData;
          // Type of polyplane
          int subtreePolyplaneType = PT_BLOCK;
          // Point list
          CPointList pointList;
          // Get streams with shared VB and IB
          SRef<CPrimitiveStream> tempPsBranch=new CPrimitiveStream;
          tempPsBranch->Init(psBranch);
          SRef<CPrimitiveStream> tempPsPolyplane=new CPrimitiveStream;
          tempPsPolyplane->Init(psPolyplane);

          PlantLevelConfig newconfig(
                TreeConfigCommon (
                    pc.polyplaneCompareTreshold,
                    _polyplaneDetail,64,pc.dp,1),
                    0,&pointList);


          // Recursive calling creates Branch and Polyplane streams
          if (_polyplaneDetail) CreateStreamData2(
                            *tempPsBranch, *tempPsPolyplane,
                            PlantDynaParams(dyn.cp,
                                            pCCS->_actualParams,
                                            pCCS->_internalParams,
                                            pCCS->_componentIndex,
                                            pCCS->_importancy,
                                            dyn.recursionDepth + 1,
                                            true,
                                            &subtreePolyplaneType),
                            newconfig);
          if (pCCS->_actualParams._polyplaneType!=PT_NOTDEFINED) 
                subtreePolyplaneType=pCCS->_actualParams._polyplaneType;

          // Point list must contain some values to create a polyplane         
          if (!pointList.IsNull())
          {
/*            float outSize = PlantTopologyInfo::getSizeCoef(pointList,pCCS->_actualParams._origin.Position());
            if (outSize > sizeCoef)
                sizeCoef = outSize;*/
            // Create new polyplaneWithData
            polyplaneWithData=CreatePolyplaneByType(subtreePolyplaneType,pc.textureSize);
            if (polyplaneWithData.NotNull())
            {
              int pcount=polyplaneWithData->_pPolyplane->SetBaseStage(PolyStageIndex);
              for (int i=0;i<pcount;i++)
              {
                ComRef<IDirect3DTexture8> nl;
                psPolyplane.RegisterTextures(PolyStageIndex+i,nl,"",nl,"",D3DTADDRESS_WRAP,D3DTADDRESS_WRAP);
              }
              PolyStageIndex+=pcount;
              if (!dyn.polyplanesAreTemporary)
              {            
                polyplaneWithData->_branchTypeId=pCCS->_actualParams._branchTypeId;
                polyplaneWithData->_enlighted=pc.dp._enlighted;
                polyplaneWithData->_sourceNode=nodeBaseIndex+bestPoint;                
                polyplaneWithData->_usageCount=pointCnt;

                _polyplaneSpace.AddVector(polyplaneWithData, polyplaneVector);
                LevelLogF(dyn.recursionDepth,"Polyplanes count = %d (group: %d / %d)", _polyplaneSpace.GetSize(),pointCnt,remain+pointCnt);
              }
              else
              {
                polyplaneWithData->_id=counter++;
              }

              _texFactory->RenderLock(true);

//              LevelLogF(dyn.recursionDepth,"Render size coef: %f (%d)",sizeCoef,bestPoint);
//              float renderSizeCoef=sizeCoef;

              // Render it to polyplane
              polyplaneWithData->_pPolyplane->Render(*tempPsBranch, *tempPsPolyplane,psPolyplane,
                polyplaneOrigin, pointList, sizeCoef, pc.dp);

              _texFactory->RenderLock(false);

              // Set it as a temporary by default
              polyplaneWithData->_temporary = 1;
              for (int i=0;i<pointCnt;i++)
              {
                int nodeIndex = groupBranchList[i];
                pCCS=childPolyplanes[nodeIndex]; 

                Matrix4 polyplaneOrigin=CalcPolyplaneOrigin(pCCS->_actualParams._origin);
                float sizeCoef=topoList[groupBranchList[i]].GetSizeCoeficient();

                if (!dyn.polyplanesAreTemporary)
                {
                  // Note that we use the form with 3 divisions instead of 1 division and 2 multiplication
                  // because importancy can be FLT_MAX
                  //          float actualPolyplaneSize = polyplaneWithData->_pPolyplane->GetTextureSize();
                  //          polyplaneWithData->_pPolyplane->UpdateImportancy((pCCS->_importancy / (actualPolyplaneSize * actualPolyplaneSize)) / (polyplaneImportancyTreshold / (polyplaneTextureSize * polyplaneTextureSize)));
                  polyplaneWithData->_pPolyplane->UpdateImportancy(pCCS->_importancy / polyplaneImportancyTreshold );
                }

                // Draw polyplane
                char s[256];
                sprintf(s, "%d", polyplaneWithData->_id);

  //              LevelLogF(dyn.recursionDepth,"Inserting plane: sizeCoef %g (%5.1f%%), seed %d, diff: %g (%d)",sizeCoef,sizeCoef/renderSizeCoef*100,pCCS->_actualParams._seed,incTable.Get(bestPoint,groupBranchList[i]),groupBranchList[i]);
  //              if (groupBranchList[i]==1)
                {
                  polyplaneWithData->_pPolyplane->Draw(psPolyplane, polyplaneOrigin, sizeCoef);
                  if (pc.pPointList) 
                     polyplaneWithData->_pPolyplane->UpdatePointList(polyplaneOrigin, sizeCoef, *pc.pPointList);
                }
                if (!dyn.polyplanesAreTemporary) {
                    polyplaneWithData->_temporary = 0;
                    if (nodeIndexes[nodeIndex] >= 0 && pc.pNodeInfoList != 0) {
                        pc.pNodeInfoList->operator[](nodeIndexes[nodeIndex])._polyplane = polyplaneWithData;
                        
                    }
                }
              }
//              for (int i=0;i<pointCnt;i++) incTable.RemoveRow(groupBranchList[i]);
            }
            else
            {
              incTable.SetPointForbiden(bestPoint);
              if (pointCnt) {gr--;remain+=pointCnt-1;}
            }        
          }
          else
          {
            incTable.SetPointForbiden(bestPoint);
            if (pointCnt) {gr--;remain+=pointCnt-1;}
          }
          
          if (_preview) _preview->Preview(dyn.recursionDepth,psBranch,psPolyplane);

        }
        

      }
        
      delete [] childPolyplanes;
      delete [] topoList;
      delete [] nodeIndexes;
    }
    else
#endif /*LINDASTUDIO*/
    {
      while (!childList.Empty() && !progressBar.StopSignaled()) 
      {
        // Get first item from the queue
        pCCS = childList.Start();
        progressBar.AdvanceNext(pbcomplex);

        // Close graphics
        _components[pCCS->_componentIndex]->CloseBranchTip(psBranch, pCCS->_sharedVertexIndex, pCCS->_sharedVertexCount);
        Matrix4 polyplaneOrigin=CalcPolyplaneOrigin(pCCS->_actualParams._origin);

        // Closest polyplane (or null, if there isn't any)
      Ref<SPolyplaneWithData> polyplaneWithData;
      AutoArray<SPointInformation> importantPoints;
      Matrix4 simplePolyplaneOrigin;
      float sizeCoef=0.001f;
      float polyplaneVector[4];

      if (fastSearch)
      {
        polyplaneVector[0] = log10(pCCS->_actualParams._age);
        polyplaneVector[1] = asin(polyplaneOrigin.Direction().Y());
        polyplaneVector[2] = (float)pc.dp._enlighted+2*pCCS->_actualParams._branchTypeId;
        polyplaneVector[3] = pc.textureSize;
        polyplaneWithData = _polyplaneSpace.GetVector(polyplaneVector);
        sizeCoef = pCCS->_actualParams._age;
        if (sizeCoef <= 0.0f)
        {
          Fail("Error: Age musn't be zero while creating a polyplane");
          sizeCoef = 0.001f;
        }
      }
      else
      {
        // Generate important points of the branch starting from (0, 0, 0)
        SActualParams apZeroBased = pCCS->_actualParams;
        apZeroBased._origin.SetPosition(Vector3(0, 0, 0));
        GenerateImportantPoints(dyn.cp, apZeroBased, pCCS->_internalParams, pCCS->_componentIndex, importantPoints);

        // Get important points focus
        Vector3 sumFocus(0, 0, 0);
        for (int i = 0; i < importantPoints.Size(); i++) {
          sumFocus += importantPoints[i]._point;
        }

        // Get simple polyplane origin
        float importantPointsSize;
        if ((importantPoints.Size() > 0) && (sumFocus.SquareSize() > 0.0f)) {
          simplePolyplaneOrigin.SetDirectionAndUp(sumFocus / importantPoints.Size(), Vector3(0, 1, 0));
          simplePolyplaneOrigin.SetPosition(pCCS->_actualParams._origin.Position());
          importantPointsSize = importantPoints[importantPoints.Size() - 1]._point.Size();
        }
        else {
          simplePolyplaneOrigin = pCCS->_actualParams._origin;
          importantPointsSize = 0.0f;
        }

        // Get size coeficient
        sizeCoef = max(importantPointsSize, 0.001);

        // Transform the focus vector to the X axis
        // (this is the normalized direction of all polyplanes - we've got better chance
        // to find similar polyplane)
        Matrix4 invRotYZ;
        float sizeXZ = simplePolyplaneOrigin.Direction().SizeXZ();
        if (sizeXZ != 0) {
          float sinRotY = simplePolyplaneOrigin.Direction().Z() / sizeXZ;
          float cosRotY = simplePolyplaneOrigin.Direction().X() / sizeXZ;
          Matrix4 rotY;
          rotY.SetRotationY(sinRotY, cosRotY);
          Matrix4 rotZ;
          rotZ.SetRotationZ(asin(simplePolyplaneOrigin.Direction().Y()));
          invRotYZ = (rotY * rotZ).InverseRotation();
        }
        else {
          Matrix4 rotZ;
          rotZ.SetRotationZ(asin(simplePolyplaneOrigin.Direction().Y()));
          invRotYZ = rotZ.InverseRotation();
        }

        // Transform important points
        for (int i = 0; i < importantPoints.Size(); i++) {
          importantPoints[i]._point = invRotYZ * importantPoints[i]._point;
        }

        // Find the polyplane below threshold
        RefArray<SPolyplaneWithData> vectors = _polyplaneSpace.GetVectorArray();
        float curCompareValue=FLT_MAX;
        for (int i = 0; i < vectors.Size(); i++) {
          
          // Calculate storedImportantPointsSize
          float storedImportantPointsSize;
          if (vectors[i]->_importantPoints.Size() > 0) {
            storedImportantPointsSize = vectors[i]->_importantPoints[vectors[i]->_importantPoints.Size() - 1]._point.Size();
          }
          else {
            storedImportantPointsSize = 0.0f;
          }

          // Get the min value to consider the worst case
          float minIPSize = min(importantPointsSize, storedImportantPointsSize);

          // Get the compare value (it should not be NULL, because in that case no difference would be accepted at all)
          //float compareValue =  max(0.2f * minIPSize, 0.01);
          //float compareValue =  max(0.1f * minIPSize, 0.01);
          //float compareValue =  max(0.05f * minIPSize, 0.01);
          float compareValue =  max(pc.polyplaneCompareTreshold * minIPSize, 0.01);

          // Compare structures
          if (vectors[i]->_enlighted == pc.dp._enlighted && vectors[i]->_branchTypeId==pCCS->_actualParams._branchTypeId) {
            float difference = ComparePlantStructures(vectors[i]->_importantPoints, importantPoints);
            if (difference<curCompareValue && difference < compareValue) {
              polyplaneWithData = vectors[i];
              curCompareValue=difference;
            }
          }
        }
      }

      // If the polyplane doesn't exist, then generate it
      if (polyplaneWithData.IsNull() || (_components[pCCS->_componentIndex]->ForceNewPolyplane()))
      {

        // Type of polyplane
        int subtreePolyplaneType;

        // Point list
        CPointList pointList;

        // Get streams with shared VB and IB
        SRef<CPrimitiveStream> tempPsBranch=new CPrimitiveStream;
        tempPsBranch->Init(psBranch);
        SRef<CPrimitiveStream> tempPsPolyplane=new CPrimitiveStream;
        tempPsPolyplane->Init(psPolyplane);
     
          PlantLevelConfig newcfg(TreeConfigCommon (
                    pc.polyplaneCompareTreshold,
                    _polyplaneDetail,64,pc.dp,1),
                    0,&pointList);

        // Recursive calling creates Branch and Polyplane streams
        CreateStreamData2(
          *tempPsBranch, *tempPsPolyplane, 
          PlantDynaParams(dyn.cp,
                         pCCS->_actualParams,
                         pCCS->_internalParams,
                         pCCS->_componentIndex,
                         pCCS->_importancy,
                        dyn.recursionDepth + 1,
                        true,
                        &subtreePolyplaneType), newcfg);


        if (pCCS->_actualParams._polyplaneType!=PT_NOTDEFINED) 
            subtreePolyplaneType=pCCS->_actualParams._polyplaneType;

        // Point list must contain some values to create a polyplane
        if (!pointList.IsEmpty())
        {
          // Create new polyplaneWithData
          polyplaneWithData=CreatePolyplaneByType(subtreePolyplaneType,pc.textureSize);
          if (polyplaneWithData.NotNull())
          {

            // Assign actual parameters and component index
  if (!fastSearch)
  {
            polyplaneWithData->_importantPoints = importantPoints;
            polyplaneWithData->_enlighted = pc.dp._enlighted;
  }
            polyplaneWithData->_branchTypeId=pCCS->_actualParams._branchTypeId;

            int pcount=polyplaneWithData->_pPolyplane->SetBaseStage(PolyStageIndex);
              for (int i=0;i<pcount;i++)
              {
                ComRef<IDirect3DTexture8> nl;
                psPolyplane.RegisterTextures(PolyStageIndex+i,nl,"",nl,"",D3DTADDRESS_WRAP,D3DTADDRESS_WRAP);
              }
              PolyStageIndex+=pcount;
 

            // Save the polyplane
            _polyplaneSpace.AddVector(polyplaneWithData, polyplaneVector);
            LevelLogF(dyn.recursionDepth,"Polyplanes count = %d", _polyplaneSpace.GetSize());

            _texFactory->RenderLock(true);

            // Render it to polyplane
            polyplaneWithData->_pPolyplane->Render(*tempPsBranch, *tempPsPolyplane,psPolyplane,
              polyplaneOrigin, pointList, sizeCoef, pc.dp );

            _texFactory->RenderLock(false);

            // Set it as a temporary by default
            polyplaneWithData->_temporary = 1;
          }
        }
      }

      // Draw the polyplane only in case the polyplaneWithData exists
      if (polyplaneWithData.NotNull())
      {
        
        // Update the polyplane's importancy each time we are about to draw it
        if (!dyn.polyplanesAreTemporary)
        {
          // Note that we use the form with 3 divisions instead of 1 division and 2 multiplication
          // because importancy can be FLT_MAX
//          float actualPolyplaneSize = polyplaneWithData->_pPolyplane->GetTextureSize();
//          polyplaneWithData->_pPolyplane->UpdateImportancy((pCCS->_importancy / (actualPolyplaneSize * actualPolyplaneSize)) / (polyplaneImportancyTreshold / (polyplaneTextureSize * polyplaneTextureSize)));
          polyplaneWithData->_pPolyplane->UpdateImportancy(pCCS->_importancy / polyplaneImportancyTreshold );
        }

        // Draw polyplane
        char s[256];
        sprintf(s, "%d", polyplaneWithData->_id);


        polyplaneWithData->_pPolyplane->Draw( psPolyplane, polyplaneOrigin, sizeCoef);
        if (pc.pPointList) {
          polyplaneWithData->_pPolyplane->UpdatePointList(polyplaneOrigin, sizeCoef, *pc.pPointList);
        }
        if (!dyn.polyplanesAreTemporary) polyplaneWithData->_temporary = 0;
      }

      // Delete the item from the list
      delete pCCS;
      if (_preview) _preview->Preview(dyn.recursionDepth,psBranch,psPolyplane);
    }
    }

    




    //Remo
    while (!childList.Empty()) {
      // Get first item from the queue
      pCCS = childList.Start();
      progressBar.AdvanceNext(1);
      delete pCCS;
    }

    // Return the number of polyplanes
    pc.polyplaneCount = _polyplaneSpace.GetSize();
    LevelLogF(dyn.recursionDepth,"Returning to level %d",dyn.recursionDepth-1);


  }

  void CPlantType::SavePolyplaneSpace(RString fileName, RString suffix) {
    _polyplaneSpace.ClearTemporary();
    RefArray<SPolyplaneWithData> vectors = _polyplaneSpace.GetVectorArray();
    int vectorCount = vectors.Size();
    ProgressBar<int> pb(vectorCount);
    for (int i = 0; i < vectorCount; i++) {
      char s[256];
      pb.AdvanceNext(1);
      sprintf(s, "%d", vectors[i]->_id);
      vectors[i]->_pPolyplane->Save(fileName + RString("_") + RString(s), suffix);
    }
  }

  void CPlantType::UpdateMergeFile(ParamClassPtr entry, RString className, RString path, RString filePrefix, RString fileSuffix)
  {
    RefArray<SPolyplaneWithData> vectors = _polyplaneSpace.GetVectorArray();
    int vectorCount = vectors.Size();
    for (int i = 0; i < vectorCount; i++)
    {

      char classNameAndSuffix[256];
      sprintf(classNameAndSuffix, "%s_%d", className.Data(), vectors[i]->_id);

      char fileName[256];
      sprintf(fileName, "%s_%d", filePrefix.Data(), vectors[i]->_id);

      ParamClassPtr pItemClass;

      if (vectors[i]->_pPolyplane->GetNumberOfTextures() > 0)
      {
        // A
        pItemClass = entry->AddClass(RString(classNameAndSuffix) + RString("_A"));
        pItemClass->Add("x", 0.0f);
        pItemClass->Add("y", 0.0f);
        pItemClass->Add("w", vectors[i]->_pPolyplane->GetTextureSize());
        pItemClass->Add("h", vectors[i]->_pPolyplane->GetTextureSize());
        pItemClass->Add("scale", 0.0f);
        pItemClass->Add("name", path + RString(fileName) + RString("_A") + fileSuffix + RString(".tga"));
        pItemClass->Add("angle", 0.0f);
        pItemClass->Add("importance", vectors[i]->_pPolyplane->GetImportancy());

        if (vectors[i]->_pPolyplane->GetNumberOfTextures() > 1)
        {
          // B
          pItemClass = entry->AddClass(RString(classNameAndSuffix) + RString("_B"));
          pItemClass->Add("x", 0.0f);
          pItemClass->Add("y", 0.0f);
          pItemClass->Add("w", vectors[i]->_pPolyplane->GetTextureSize());
          pItemClass->Add("h", vectors[i]->_pPolyplane->GetTextureSize());
          pItemClass->Add("scale", 0.0f);
          pItemClass->Add("name", path + RString(fileName) + RString("_B") + fileSuffix + RString(".tga"));
          pItemClass->Add("angle", 0.0f);
          pItemClass->Add("importance", vectors[i]->_pPolyplane->GetImportancy());

          if (vectors[i]->_pPolyplane->GetNumberOfTextures() > 2)
          {
            // C
            pItemClass = entry->AddClass(RString(classNameAndSuffix) + RString("_C"));
            pItemClass->Add("x", 0.0f);
            pItemClass->Add("y", 0.0f);
            pItemClass->Add("w", vectors[i]->_pPolyplane->GetTextureSize());
            pItemClass->Add("h", vectors[i]->_pPolyplane->GetTextureSize());
            pItemClass->Add("scale", 0.0f);
            pItemClass->Add("name", path + RString(fileName) + RString("_C") + fileSuffix + RString(".tga"));
            pItemClass->Add("angle", 0.0f);
            pItemClass->Add("importance", vectors[i]->_pPolyplane->GetImportancy());
          }
        }
      }
      
    }
  }

  void CPlantType::EnumPolyplaneTextures(CPlantType::IEnumTextures &funct,bool enumNormalMaps) const
  {
    RefArray<SPolyplaneWithData> vectors = _polyplaneSpace.GetVectorArray();
    int vectorCount = vectors.Size();
    for (int i = 0; i < vectorCount; i++)
    {
      float importancy=vectors[i]->_pPolyplane->GetImportancy();
      bool temporaly=vectors[i]->_temporary!=0;
      for (int j=0,cnt=vectors[i]->_pPolyplane->GetNumberOfTextures();j<cnt;j++)
      {
        if (enumNormalMaps)
        {
          if (funct(vectors[i]->_pPolyplane->GetNormalTexture(j),importancy,temporaly)==false) return;
        }
        else
        {
          if (funct(vectors[i]->_pPolyplane->GetTexture(j),importancy,temporaly)==false) return;
        }
      }
    }
  }

/*
  void CPlantType::CreateSpecialLOD(ObjectData *o,
                                    Matrix4Par origin,
                                    int seed,
                                    float age,
                                    int trianglesLimit,
                                    int sphereDetail,
                                    float sphereSizeCoef,
                                    int wrapperComponentNum)
  {

    // Test whether there are any components
    if (_components.Size() == 0) return;

    // Parallels and meridians
    const int nParallels = 3;
    const int nMeridians = 3;

    // Init constant parameters
    SConstantParams cp;
    cp._detail = 1.0f;
    cp._time = 0.0f;
    cp._month = 0.0f;
    cp._plantAge = age;

    // Init actual parameters
    SActualParams ap;
    ap._origin = origin;
    ap._seed = seed;
    ap._age = age;
    ap._counter = 0;
    ap._diaOrigin = origin.Position();
    ap._textureVOffset = 0.0f;
    ap._geometrySpin = 0.0f;

    // Link list of components
    TListBidir<SComponentChildL> childList;

    // Prepare the first child
    SComponentChildL *pCCL = new SComponentChildL();
    pCCL->_componentIndex = _components.Size() - 1;
    pCCL->_importancy = FLT_MAX;
    pCCL->_actualParams = ap;

    // Insert the first child
    childList.Insert(pCCL);

    // Getting the geometry of the tree
    int trianglesCount = 0;
    int polyplanesCount = 1;
    int bodyId = 1;
    while ((!childList.Empty()) && (trianglesCount + (polyplanesCount * CPrimitive::GetSphereTriangles(nParallels, nMeridians)) < trianglesLimit))
    {

      // Get first item from the queue
      pCCL = childList.Start();    

      // Get children of the branch set the primitive
      CComponentChildBuffer ccb;
      if (!_components[pCCL->_componentIndex]->GetChildren(pCCL->_actualParams, pCCL->_internalParams, cp, ccb))
      {
        LogF("Error in component '" + _components[0]->GetComponentName(pCCL->_componentIndex) + RString("'."));
        while (!childList.Empty()) childList.Start()->Delete();
        return;
      };

      // Update number of actual polyplanes
      polyplanesCount += ccb.GetChildCount() - 1;

      // Draw the component primitive
      CPrimitive *pPrimitive = _components[pCCL->_componentIndex]->GetPrimitive();
      if (pPrimitive)
      {
        trianglesCount += pPrimitive->DrawBody(o, bodyId);
      }

      // Remove item from list and delete it
      delete pCCL;

      // Insert the children in the queue at the proper place
      for (int i = 0; i < ccb.GetChildCount(); i++)
      {

        // Get the i-th child
        SComponentChild *pCC = ccb.GetChild(i);

        // Test the validity of the component index
        if (pCC->_componentIndex >= 0)
        {

          // Fill the parameters of the i-th child
          pCCL = new SComponentChildL();
          pCCL->_componentIndex = pCC->_componentIndex;
          pCCL->_importancy = pCC->_importancy;
          pCCL->_actualParams = pCC->_actualParams;

          // Sort it into the list from the end of the list according to its importancy
          SComponentChildL *pCurr = childList.End();
          while ((childList.NotEnd(pCurr)) && (pCurr->_importancy < pCCL->_importancy))
          {
            pCurr = childList.Retreat(pCurr);
          }
          childList.InsertAfter(pCurr, pCCL);
        }
      }
    }

    // Get spheres from the rest of the branches
    while (!childList.Empty())
    {

      // Get first item from the queue
      pCCL = childList.Start();

      // Generate the points from the rest of the tree
      CPointList pointList;
      GenerateAllPoints(cp, pCCL->_actualParams, pCCL->_internalParams, pCCL->_componentIndex, wrapperComponentNum, pointList);

      // Get the sphere parameters and draw it
      float sX, sY, sZ;
      Matrix4 sOrigin = pointList.BestOriginAndSize(sX, sY, sZ);
      CPrimitive::DrawSphere(o, bodyId, sOrigin, sX * sphereSizeCoef, sY * sphereSizeCoef, sZ * sphereSizeCoef, sphereDetail + 2, sphereDetail + 2);

      // Delete the item from the list
      delete pCCL;
    }
  }
  */
  /*
  void CTreeType::LoadTreeStructure(PDirect3DDevice &pD3DDevice, DWORD hVSNormal, const ParamEntry &Entry) {
    AutoArray<RString> Names;
    int n = Entry.GetEntryCount();
    if (n == 0) return;
    _Branches.Realloc(n);
    _Branches.Resize(n);
    Names.Realloc(n);
    Names.Resize(n);
    int i;

    // Creation of the array of branches
    for (i = 0; i < n; i++)
    {
      const ParamEntryVal &SubEntry = Entry.GetEntry(i);
      Names[i] = SubEntry.GetName();
      RString BranchType = SubEntry >> "type";

      if (stricmp(BranchType, "tree") == 0)
      {
        _Branches[i] =  new CBTree();
      }
      else if (stricmp(BranchType, "leaf") == 0)
      {
        _Branches[i] =  new CBLeaf();
      }
      else if (stricmp(BranchType, "stem") == 0)
      {
        _Branches[i] =  new CBStem();
      }
      else if (stricmp(BranchType, "simpleGutter") == 0)
      {
        _Branches[i] =  new CBSimpleGutter();
      }
      else if (stricmp(BranchType, "cane") == 0)
      {
        _Branches[i] =  new CBCane();
      }
      else if (stricmp(BranchType, "bunch") == 0)
      {
        _Branches[i] =  new CBBunch();
      }
      else if (stricmp(BranchType, "splitter") == 0)
      {
        _Branches[i] =  new CBSplitter();
      }
      else if (stricmp(BranchType, "leafStalk") == 0)
      {
        _Branches[i] =  new CBLeafStalk();
      }
    }

    // Initialization of the array of branches
    for (i = 0; i < n; i++)
    {
      //const ParamEntryVal &SubEntry = Entry.GetEntry(i);
      //_Branches[i]->Init(pD3DDevice, hVSNormal, i, SubEntry, Names);
    }
  }
  */
  #if 0
  void CTreeType::CreateStreamData(
                    CPrimitiveStream &PSBranch,
                    CPrimitiveStream &PSTriplane,
                    int Seed,
                    float Detail,
                    Matrix4Par Origin,
                    float Age,
                    SBranchBone *pBranchBone,
                    int BranchBoneCount) {
    if (_Branches[_Branches.Size() - 1] != NULL) {
      // pBranchBone
      // ConsideredBoneIndex (= Index rootu - je treba ho najit)
  /*
      SBranchBone RootBone;
      RootBone.Seed = Seed;
      int RootBoneIndex = FindInSortedArray(pBranchBone, BranchBoneCount, &RootBone, BranchBoneCompareSeed);
      assert((RootBoneIndex >= 0) && (RootBoneIndex < BranchBoneCount));
  */

      SParamBranch PB;
      PB._Age = Age;
      PB._Origin = Origin;
      PB._Seed = Seed;
      PB._TreeOrigin = Origin.Position();
      SParamDraw PD;
      PD._Color = SFColor(1, 1, 1, 1);
      PD._Detail = Detail;
      PD._DisableTriplanes = true;
      PD._pBBoxCounter = NULL;
      PD._SharedSideNum = 0;
      PD._SharedVertexIndex = 0;
      PD._TextureLength = 0.0f;
      _Branches[_Branches.Size() - 1]->Draw(PSBranch, PSTriplane, _Branches, PB, PD);
    }
  }
  #endif

  /*
  int CTreeType::GetOldestBranches(
                    int Seed,
                    Matrix4Par Origin,
                    float Age,
                    int MaxBranchesNum,
                    SBranchNode *pBranchNode) {

    int i;

    // Verify maximal count of oldest branches
    assert(MaxBranchesNum <= MAX_OLDEST_BRANCHES);

    // --- Get Oldest branches and save a parent for each ---
    // Branch queue
    SSubtree BranchQueue[MAX_OLDEST_BRANCHES * MAX_BRANCH_CHILDERN];
    int ParentQueue[MAX_OLDEST_BRANCHES * MAX_BRANCH_CHILDERN];
    int BranchQueueFirstItem = 0;
    int BranchQueueItemsCount;

    // Put first items into the queues
    BranchQueue[0]._Branch = _Branches.Size() - 1;
    BranchQueue[0]._ParamBranch._Age = Age;
    BranchQueue[0]._ParamBranch._Origin = Origin;
    BranchQueue[0]._ParamBranch._Seed = Seed;
    BranchQueue[0]._ParamBranch._TreeOrigin = Origin.Position();
    ParentQueue[0] = -1;
    BranchQueueItemsCount = 1;

    // Modify the root
    _Branches[BranchQueue[0]._Branch]->ModifyBranch(BranchQueue[0]._ParamBranch);

    // Main cycle - oldest branches will accumulate at the beginning of the queue
    while ((BranchQueueItemsCount > 0) && (BranchQueueFirstItem < MaxBranchesNum)) {

      // Get the oldest branch
      SSubtree *pB = &BranchQueue[BranchQueueFirstItem];

      // Get new childern
      int ChildernStartIndex = BranchQueueFirstItem + BranchQueueItemsCount;
      int ChildernCount = _Branches[pB->_Branch]->GetChildern(_Branches, pB->_ParamBranch, &BranchQueue[ChildernStartIndex]);
      for (i = ChildernStartIndex; i < ChildernStartIndex + ChildernCount; i++) {
        ParentQueue[i] = BranchQueueFirstItem;
      }
      BranchQueueItemsCount += ChildernCount - 1;
      BranchQueueFirstItem++;
      
      // Swap the oldest branch with the first branch
      int OldestBranch = BranchQueueFirstItem;
      // Find it
      for (i = BranchQueueFirstItem + 1; i < BranchQueueFirstItem + BranchQueueItemsCount; i++) {
        if (BranchQueue[i]._ParamBranch._Age > BranchQueue[OldestBranch]._ParamBranch._Age) OldestBranch = i;
      }
      // Swap it
      if (BranchQueueFirstItem != OldestBranch) {
        // Exchange items
        SSubtree TempBranch;
        TempBranch = BranchQueue[BranchQueueFirstItem];
        BranchQueue[BranchQueueFirstItem] = BranchQueue[OldestBranch];
        BranchQueue[OldestBranch] = TempBranch;
        int TempParent;
        TempParent = ParentQueue[BranchQueueFirstItem];
        ParentQueue[BranchQueueFirstItem] = ParentQueue[OldestBranch];
        ParentQueue[OldestBranch] = TempParent;
        // Update parents by childern from interval <BranchQueueFirstItem + 1, OldestBranch - 1> ...
        for (i = BranchQueueFirstItem + 1; i < OldestBranch; i++) {
          if (ParentQueue[i] == BranchQueueFirstItem) ParentQueue[i] = OldestBranch;
          if (ParentQueue[i] == OldestBranch) ParentQueue[i] = BranchQueueFirstItem;
        }
        // ... and from interval <OldestBranch + 1, BranchQueueFirstItem + BranchQueueItemsCount - 1>
        for (i = OldestBranch + 1; i < BranchQueueFirstItem + BranchQueueItemsCount; i++) {
          if (ParentQueue[i] == BranchQueueFirstItem) ParentQueue[i] = OldestBranch;
          if (ParentQueue[i] == OldestBranch) ParentQueue[i] = BranchQueueFirstItem;
        }
      }
    }

    // --- Put indexes to items from  Queue into the 2D array according to their parents ---
    // Each row is MAX_BRANCH_CHILDERN row and contains childern of parent specified
    // by the row index
    // Example:
    // Root (1, 2)
    // 1    (3, 4)
    // 2    (5, 6)
    // 3    ()
    // 4    ()
    // 5    ()
    // 6    ()
    
    int QueueIndex[MAX_OLDEST_BRANCHES * MAX_BRANCH_CHILDERN];
    int ChildernNumber[MAX_OLDEST_BRANCHES];

    // Zero number of childern for each possible parent
    for (i = 0; i < BranchQueueFirstItem; i++) ChildernNumber[i] = 0;

    // Fill out the 2D array. Skip the root - it doesn't have any parent.
    for (i = 1; i < BranchQueueFirstItem; i++) {
      int RowIndex = ParentQueue[i];
      QueueIndex[MAX_BRANCH_CHILDERN * RowIndex + ChildernNumber[RowIndex]] = i;
      ChildernNumber[RowIndex]++;
    }

    // --- Serialize items ---
    // Example:
    // (Root, 1, 2), (1, 3, 2), (2, 5, 2), (3, 0, 0), (4, 0, 0), (5, 0, 0), (6, 0, 0)

    int IndexOfChildernInSerializedArray[MAX_OLDEST_BRANCHES];

    // First we set all ChildIndexes to point to the row of QueueIndex
    // (simly copy the value from QueueIndex array)

    // Put in the root
    pBranchNode[0].Branch = BranchQueue[0];
    pBranchNode[0].FirstChildIndex = 0;
    pBranchNode[0].NumChildern = ChildernNumber[0];
    IndexOfChildernInSerializedArray[0] = 0;

    // Put in the rest nodes
    int BranchNodeIndex = 1;
    for (int RowIndex = 0; RowIndex < BranchQueueFirstItem; RowIndex++) {
      // Save the starting index
      IndexOfChildernInSerializedArray[RowIndex] = BranchNodeIndex;
      // Add the row
      for (i = 0; i < ChildernNumber[RowIndex]; i++) {
        int QI = QueueIndex[MAX_BRANCH_CHILDERN * RowIndex + i];
        pBranchNode[BranchNodeIndex].Branch = BranchQueue[QI];
        pBranchNode[BranchNodeIndex].FirstChildIndex = QI;
        pBranchNode[BranchNodeIndex].NumChildern = ChildernNumber[QI];
        BranchNodeIndex++;
      }
    }  

    // In this loop we convert QueueIndex to point to our new array
    for (i = 0; i < BranchQueueFirstItem; i++) {
      pBranchNode[i].FirstChildIndex = IndexOfChildernInSerializedArray[pBranchNode[i].FirstChildIndex];
    }

    return BranchQueueFirstItem;
  }

  void CTreeType::CreateBranchBoneArray(SBranchNode *pBranchNode, int BranchNodeCount, SBranchBone *pBranchBone) {
    
    // Fill the array
    for (int i = 0; i < BranchNodeCount; i++) {
      pBranchBone[i].Seed = pBranchNode[i].Branch._ParamBranch._Seed;
      pBranchBone[i].Index = i;
    }

    // Sort the array according to seed
    ShortSort(&pBranchBone[0], &pBranchBone[BranchNodeCount - 1], BranchBoneCompareSeed);

    // Test if there are 2 or more branches with the same seed
    for (int i = 1; i < BranchNodeCount; i++) {
      assert(pBranchBone[i].Seed != pBranchBone[i - 1].Seed);
    }
  }
  */
  //--------------------------------------------------------------------------------------

  #if 0
  void CTreeEngine::DrawMatrix(Matrix4Par Matrix) {
    SBranchVertex BV;
    WORD Index;
    int Stage = PRIMITIVESTAGE_COUNT - 1;
    _pPSBranch->RegisterTextures(Stage, ComRef<IDirect3DTexture8>(), "", ComRef<IDirect3DTexture8>(), "", D3DTADDRESS_MIRROR, D3DTADDRESS_MIRROR);

    // Aside
    // E
    BV.Position = Matrix.Position() + Matrix.Direction() * ARROW_COEF;
    BV.Normal = Matrix.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(0, 0, 255);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // B
    BV.Position = Matrix.Position() + (Matrix.DirectionAside() - Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = Matrix.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(0, 0, 255);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // A
    BV.Position = Matrix.Position() + (Matrix.DirectionAside() + Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = Matrix.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(0, 0, 255);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // -Aside
    // E
    BV.Position = Matrix.Position() + Matrix.Direction() * ARROW_COEF;
    BV.Normal = -Matrix.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // D
    BV.Position = Matrix.Position() + (-Matrix.DirectionAside() + Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = Matrix.Position() + (-Matrix.DirectionAside() - Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // Up
    // E
    BV.Position = Matrix.Position() + Matrix.Direction() * ARROW_COEF;
    BV.Normal = Matrix.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(0, 255, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // A
    BV.Position = Matrix.Position() + (Matrix.DirectionAside() + Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = Matrix.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(0, 255, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // D
    BV.Position = Matrix.Position() + (-Matrix.DirectionAside() + Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = Matrix.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(0, 255, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // -Up
    // E
    BV.Position = Matrix.Position() + Matrix.Direction() * ARROW_COEF;
    BV.Normal = -Matrix.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = Matrix.Position() + (-Matrix.DirectionAside() - Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // B
    BV.Position = Matrix.Position() + (Matrix.DirectionAside() - Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // -Direction
    // A
    BV.Position = Matrix.Position() + (Matrix.DirectionAside() + Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // B
    BV.Position = Matrix.Position() + (Matrix.DirectionAside() - Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = Matrix.Position() + (-Matrix.DirectionAside() - Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // A
    BV.Position = Matrix.Position() + (Matrix.DirectionAside() + Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = Matrix.Position() + (-Matrix.DirectionAside() - Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // D
    BV.Position = Matrix.Position() + (-Matrix.DirectionAside() + Matrix.DirectionUp()) * ARROW_SHARP * ARROW_COEF;
    BV.Normal = -Matrix.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);
  }

  void CTreeEngine::DrawLine(Matrix4Par MatrixA, Matrix4Par MatrixB) {
    SBranchVertex BV;
    WORD Index;
    int Stage = PRIMITIVESTAGE_COUNT - 1;
    _pPSBranch->RegisterTextures(Stage, ComRef<IDirect3DTexture8>(), "", ComRef<IDirect3DTexture8>(), "", D3DTADDRESS_MIRROR, D3DTADDRESS_MIRROR);

    // Aside
    // E
    BV.Position = MatrixB.Position();
    BV.Normal = MatrixA.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(0, 0, 255);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // B
    BV.Position = MatrixA.Position() + (MatrixA.DirectionAside() - MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = MatrixA.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(0, 0, 255);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // A
    BV.Position = MatrixA.Position() + (MatrixA.DirectionAside() + MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = MatrixA.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(0, 0, 255);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // -Aside
    // E
    BV.Position = MatrixB.Position();
    BV.Normal = -MatrixA.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // D
    BV.Position = MatrixA.Position() + (-MatrixA.DirectionAside() + MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = MatrixA.Position() + (-MatrixA.DirectionAside() - MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.DirectionAside();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // Up
    // E
    BV.Position = MatrixB.Position();
    BV.Normal = MatrixA.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(0, 255, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // A
    BV.Position = MatrixA.Position() + (MatrixA.DirectionAside() + MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = MatrixA.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(0, 255, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // D
    BV.Position = MatrixA.Position() + (-MatrixA.DirectionAside() + MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = MatrixA.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(0, 255, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // -Up
    // E
    BV.Position = MatrixB.Position();
    BV.Normal = -MatrixA.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = MatrixA.Position() + (-MatrixA.DirectionAside() - MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // B
    BV.Position = MatrixA.Position() + (MatrixA.DirectionAside() - MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.DirectionUp();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // -Direction
    // A
    BV.Position = MatrixA.Position() + (MatrixA.DirectionAside() + MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // B
    BV.Position = MatrixA.Position() + (MatrixA.DirectionAside() - MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = MatrixA.Position() + (-MatrixA.DirectionAside() - MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // A
    BV.Position = MatrixA.Position() + (MatrixA.DirectionAside() + MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // C
    BV.Position = MatrixA.Position() + (-MatrixA.DirectionAside() - MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);

    // D
    BV.Position = MatrixA.Position() + (-MatrixA.DirectionAside() + MatrixA.DirectionUp()) * LINE_COEF;
    BV.Normal = -MatrixA.Direction();
    BV.Diffuse = D3DCOLOR_XRGB(255, 0, 0);
    BV.U1 = BV.V1 = 0.0f;
    Index = _pPSBranch->AddVertex(&BV);
    _pPSBranch->AddIndex(Stage, Index);
  }

  #endif




  void CTreeEngine::Init(ITextureFactory *texFactory) 
  {
    
    // Save device
    _generate.Lock();

    // Init the plant type
    _plantType.Init();


    // Create stream for branches
    ComRef<IDirect3DDevice8> tmp;
    tmp << texFactory->GetRenderDevice();
    _pPSBranch = new CPrimitiveStream(tmp);
    _pPSBranch->Init(sizeof(SBranchVertex), 0);

    // Create stream for triplanes
    _pPSTriplane = new CPrimitiveStream(tmp);
    _pPSTriplane->Init(sizeof(SPolyplaneVertex), 0);

    _texFactory=texFactory;

    _generate.Unlock();
  }


  void CTreeEngine::LoadPlant(RString folder, const char *source, int length,bool resetPolyplanes) {
    QIStrStream stream;
    stream.init(source, length);
    ParamFile pf;
    pf.Parse(stream);

    _pPSBranch->Clear(resetPolyplanes);
    _pPSTriplane->Clear(resetPolyplanes);

    _plantType.LoadStructure(_texFactory, folder, pf, _pPSBranch,resetPolyplanes);
  }

  void CTreeEngine::LoadPlant(RString folder, const ParamEntry &entry, bool resetPolyplanes) {
    _pPSBranch->Clear(resetPolyplanes);
    _pPSTriplane->Clear(resetPolyplanes);
    _plantType.LoadStructure(_texFactory, folder, entry, _pPSBranch,resetPolyplanes);
  }

  int CTreeEngine::LoadPlantFromFile(RString folder, RString fileName, bool resetPolyplanes) {
    _pPSBranch->Clear(resetPolyplanes);
    _pPSTriplane->Clear(resetPolyplanes);

    ParamFile pf;
    if (pf.Parse(folder + RString("\\") + fileName) != LSOK) return 0;
    return _plantType.LoadStructure(_texFactory, folder, pf, _pPSBranch, resetPolyplanes);
  }

  bool CTreeEngine::LoadPlant(IPlantControlDB *db, bool resetPolyplanes)
  {
    _pPSBranch->Clear(resetPolyplanes);
    _pPSTriplane->Clear(resetPolyplanes);

    if (!_plantType.LoadStructure(_texFactory,db,resetPolyplanes)) return false;
    for (int i=db->GetFirstTextureId();i;)
    {
      IPlantControlDB::TextureDesc tex=db->GetNextTexture(i);

      ComRef<IDirect3DTexture8> texture;
      if (tex.textureName!= RString("")) 
      {
        HRESULT hr=_texFactory->LoadTexure(tex.textureName,texture,tex.linearAlpha);
        if (SUCCEEDED(hr))
        {        
          _pPSBranch->RegisterTextures(tex.textureIndex,texture,tex.textureName,ComRef<IDirect3DTexture8>(),RString(),
            GetTexAddr(tex.addr_u),GetTexAddr(tex.addr_v),tex.linearAlpha); 
        }
      }
    }
    return true;
  }


  Ref<CPrimitiveStream> CTreeEngine::GetPSBranch() {
    return _pPSBranch;
  }


  void CTreeEngine::AddPolyplaneTextureNames(StringArray &names) {
    _pPSTriplane->AddNewTextureNames(names);
  }


  CTreeEngine::CTreeEngine(GameState &gState):_plantType(gState)
  {
  }

  int CTreeEngine::PreparePlantData(TreeConfig &plantConfiguration,
                                    IPlantPreview *preview)
  {

      _generate.Lock();     //generating started
      _texFactory->StartGenerating();


    ProgressBar<int> overall(1);
    overall.AdvanceNext(1);

    _pPSBranch->Clear(false);
    _pPSTriplane->Clear(false);
    // Create stream data
    _nodeInfoList.Clear();
    _plantType.SetPreviewInterface(preview);
    PlantConfig pcfg(plantConfiguration,PlantLevelConfig(plantConfiguration,&_nodeInfoList,0));
    _plantType.CreatePlant(*_pPSBranch, *_pPSTriplane,pcfg);

    if (overall.StopSignaled())
      LogF("Stopped.");
    // Prepare VB and IB of Branches



    _pPSBranch->Prepare();  
    _pPSTriplane->Prepare();

    int result=_pPSBranch->GetIndexCount() / 3 + _pPSTriplane->GetIndexCount() / 3;

    _generate.Unlock();       //generation is completted, unlock state
    _texFactory->StopGenerating();

    return result;
  }

  int CTreeEngine::GetNearestNodeInfo(Vector3Par pos,
                                      Vector3Par dir,
                                      SNodeInformation &nodeInfo) {

    // Get nearest item
    int nearestIndex = -1;
    float nearestDistance2 = FLT_MAX;
    for (int i = 0; i < _nodeInfoList.Size(); i++) {
      Vector3 node = _nodeInfoList[i]._actualParams._origin.Position();
      if (node.DotProduct(dir) - pos.DotProduct(dir) > 0.0f) {
        float newDistance2 = (pos - node + (((node - pos) * dir) / (dir * dir)) * dir).SquareSize();
        if (newDistance2 < nearestDistance2) {
          nearestDistance2 = newDistance2;
          nearestIndex = i;
        }
      }
    }

    // Return
    if (nearestIndex >= 0) {
      nodeInfo = _nodeInfoList[nearestIndex];
      return 1;
    }
    return 0;
  }

  bool CTreeEngine::GetNodeByIndex(int index, SNodeInformation &info) const
  {
    if (index<0 || index>=_nodeInfoList.Size()) return false;
    info=_nodeInfoList[index];
    return true;
  }

  void CTreeEngine::DeleteLODObjectFirstLevel() {
    // Delete level 0 - a simple hack to solve the problem the LODObject cannot be empty
    int level = _lo.FindLevelExact(0.0f);
    if (level >= 0) _lo.DeleteLevel(level);
    //_lo.DeleteLevel(0);
  }

  void CTreeEngine::DeleteLODObjectLevels() {
    while(_lo.DeleteLevel(_lo.NLevels() - 1)) {};
  }

  void CTreeEngine::DeleteLODObjectNonSpecLevels() {
    for (int i = 0; i < _lo.NLevels();) {
      if (_lo.NLevels() <= 1) return;
      if (_lo.Resolution(i) < 900.0f) {
        _lo.DeleteLevel(i);
      }
      else {
        i++;
      }
    }
  }

  void CTreeEngine::AddLevelToLODObject(float resolution, RString materialName)
  {
    
    // Create level of the LOD object
    ObjectData *pod = new ObjectData();

    // Copy Branches

    // Get vertices
    AutoArray<BYTE> vertices = _pPSBranch->GetVertices();
    SBranchVertex *pBV = (SBranchVertex*)vertices.Data();
    int bvCount = vertices.Size() / sizeof(SBranchVertex);

  /*
    // Copy vertices
    pod->ReservePoints(bvCount);
    for (int i = 0; i < bvCount; i++) {
      pod->Point(i).SetPoint(pBV[i].Position);
    }

    // Copy normals
    VecT *pNormal;
    for (int i = 0; i < bvCount; i++) {
      pNormal = pod->NewNormal();
      *pNormal = -pBV[i].Normal;
    }
  */

    // Copy vertices
    PosT *pPoint;
    for (int i = 0; i < bvCount; i++) {
      Vector3 point = pBV[i].Position;
      //int temp;
      if (0 > pod->FindPoint(point)) {
        pPoint = pod->NewPoint();
        pPoint->SetPoint(pBV[i].Position);
      }
    }

    // Copy normals
    VecT *pNormal;
    for (int i = 0; i < bvCount; i++) {
      Vector3 normal = -pBV[i].Normal;
      //int temp;
      if (0 > pod->FindNormal(normal)) {
        pNormal = pod->NewNormal();
        *pNormal = normal;
      }
    }

    // Cycle through all the stages
    pod->ReserveFaces(_pPSBranch->GetIndexCount() / 3);
    int faceNumber = 0;
    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      CPrimitiveStage *pPStage = _pPSBranch->GetPrimitiveStage(i);
      for (int j = 0; j < pPStage->Size(); j += 3) {
        FaceT fc(pod,faceNumber);
        //strcpy(pod->Face(faceNumber).texture, pPStage->_TS1Name);
        fc.SetTexture(pPStage->_TS1Name);
        fc.SetMaterial(materialName + RString("_branch.rvmat"));
        fc.SetN(3);

        int pointIndex;
        int normalIndex;
        SBranchVertex *pCurrBV;

        pCurrBV = &pBV[pPStage->Data()[j + 0]];
        if (0 > (pointIndex = pod->FindPoint(pCurrBV->Position))) {
          LogF("Point doesn't exist.");
          pointIndex = 0;
        }
        if (0 > (normalIndex = pod->FindNormal(-pCurrBV->Normal))) {
          LogF("Point doesn't exist.");
          pointIndex = 0;
          normalIndex = 0;
        }
        fc.SetPoint(0,pointIndex);
        fc.SetNormal(0,normalIndex);
        fc.SetU(0,pBV[pPStage->Data()[j + 0]].U1);
        fc.SetV(0,pBV[pPStage->Data()[j + 0]].V1);

        pCurrBV = &pBV[pPStage->Data()[j + 1]];
        if (0 > (pointIndex = pod->FindPoint(pCurrBV->Position))) {
          LogF("Point doesn't exist.");
          pointIndex = 0;        
        }
        if (0 > (normalIndex = pod->FindNormal(-pCurrBV->Normal))) {
          LogF("Point doesn't exist.");
          pointIndex = 0;
          normalIndex = 0;
        }
        fc.SetPoint(2,pointIndex);
        fc.SetNormal(2,normalIndex);
        fc.SetU(2,pBV[pPStage->Data()[j + 1]].U1);
        fc.SetV(2,pBV[pPStage->Data()[j + 1]].V1);

        pCurrBV = &pBV[pPStage->Data()[j + 2]];
        if (0 > (pointIndex = pod->FindPoint(pCurrBV->Position)))
        {
          LogF("Point doesn't exist.");
          pointIndex = 0;
        }
        if (0 > (normalIndex = pod->FindNormal(-pCurrBV->Normal))) {
          LogF("Point doesn't exist.");
          pointIndex = 0;
          normalIndex = 0;
        }
        fc.SetPoint(1,pointIndex);
        fc.SetNormal(1,normalIndex);
        fc.SetU(1,pBV[pPStage->Data()[j + 2]].U1);
        fc.SetV(1,pBV[pPStage->Data()[j + 2]].V1);

        faceNumber++;
      }
    }

    // Copy Polyplanes

    // Get vertices
    AutoArray<BYTE> polyplaneVertices = _pPSTriplane->GetVertices();
    SPolyplaneVertex *pPV = (SPolyplaneVertex*)polyplaneVertices.Data();
    int pvCount = polyplaneVertices.Size() / sizeof(SPolyplaneVertex);

    // Start values
    int startPoint = pod->NPoints();
    int startNormal = pod->NNormals();

    // Copy vertices
    for (int i = 0; i < pvCount; i++) {
      pPoint = pod->NewPoint();
      pPoint->SetPoint(pPV[i].Position);
      //pPoint->flags |= POINT_HALFLIGHT;
    }

    // Copy normals
    for (int i = 0; i < pvCount; i++) {
      pNormal = pod->NewNormal();
      *pNormal = -pPV[i].Normal;
    }

    // Cycle through all the stages
    pod->ReserveFaces(_pPSTriplane->GetIndexCount() / 3);
    int oldFaceNumber = faceNumber;
    faceNumber = 0;
    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      CPrimitiveStage *pPStage = _pPSTriplane->GetPrimitiveStage(i);
      for (int j = 0; j < pPStage->Size(); j += 3) {
        FaceT fc(pod,oldFaceNumber + faceNumber);
        //strcpy(pod->Face(oldFaceNumber + faceNumber).texture, pPStage->_TS0Name);
        fc.SetTexture(pPStage->_TS0Name);
        fc.SetMaterial(materialName + RString("_polyplane.rvmat"));
        fc.SetN(3);

        fc.SetPoint(0,startPoint + pPStage->Data()[j + 0]);
        fc.SetNormal(0, startNormal + pPStage->Data()[j + 0]);
        fc.SetU(0,pPV[pPStage->Data()[j + 0]].U1);
        fc.SetV(0,pPV[pPStage->Data()[j + 0]].V1);

        fc.SetPoint(2,startPoint + pPStage->Data()[j + 1]);
        fc.SetNormal(2, startNormal + pPStage->Data()[j + 1]);
        fc.SetU(2,pPV[pPStage->Data()[j + 1]].U1);
        fc.SetV(2,pPV[pPStage->Data()[j + 1]].V1);

        fc.SetPoint(1,startPoint + pPStage->Data()[j + 2]);
        fc.SetNormal(1, startNormal + pPStage->Data()[j + 2]);
        fc.SetU(1,pPV[pPStage->Data()[j + 2]].U1);
        fc.SetV(1,pPV[pPStage->Data()[j + 2]].V1);

        // Add sharp edges
        pod->AddSharpEdge(fc.GetPoint(0),fc.GetPoint(2));
        pod->AddSharpEdge(fc.GetPoint(2),fc.GetPoint(1));
        pod->AddSharpEdge(fc.GetPoint(1),fc.GetPoint(0));

        faceNumber++;
      }
    }

    // Insert level to LOD object (remove if there exists one already)
    int level = _lo.FindLevelExact(resolution);
    if (level >= 0) _lo.DeleteLevel(level);
    _lo.InsertLevel(pod, resolution);
  }

/*  void CTreeEngine::AddSpecialLOD(int seed,
                                  Matrix4Par origin,
                                  float age,
                                  float resolution,
                                  int trianglesLimit,
                                  int sphereDetail,
                                  float sphereSizeCoef,
                                  int wrapperComponentNum,
                                  float mass,
                                  ConstParamEntryPtr namedProperties)
  {

    // Create level of the LOD object
    ObjectData *pod = new ObjectData();

    // Create the special LOD data
    _plantType.CreateSpecialLOD(pod, origin, seed, age, trianglesLimit, sphereDetail, sphereSizeCoef, wrapperComponentNum);

    // Assign mass to all the points
    if (mass > 0)
    {
      int nPoints = pod->NPoints();
      float partialMass = mass / nPoints;
      for (int i = 0; i < nPoints; i++)
      {
        pod->SetPointMass(i, partialMass);
      }
    }

    // Add the list of named properties
    if (namedProperties.NotNull() && (namedProperties->IsArray()))
    {
      int entryCount = namedProperties->GetSize() / 2;
      for (int i = 0; i < entryCount; i++)
      {
        pod->SetNamedProp((*namedProperties)[i * 2].GetValue().Data(), (*namedProperties)[i * 2 + 1].GetValue().Data());
      }
    }

    // Insert level to LOD object (remove if there exists one already)
    int level = _lo.FindLevelExact(resolution);
    if (level >= 0) _lo.DeleteLevel(level);
    _lo.InsertLevel(pod, resolution);
  }

  bool CTreeEngine::LoadLODObject(RString fileName) {
    return _lo.Load(Pathname(fileName.Data()), NULL, NULL) >= 0;
  }

  void CTreeEngine::SaveLODObject(RString fileName) {
    _lo.Save(Pathname(fileName.Data()), 1, true, NULL, NULL);
  /*
    ofstream f;
    f.open(fileName,ios::out|ios::binary);
    _lo.Save(f, true);
    f.close();
  *//*
  }
      */
  void CTreeEngine::SavePolyplaneSpace(RString fileName, RString suffix) {
    _plantType.SavePolyplaneSpace(fileName, suffix);
  }

  void CTreeEngine::UpdateMergeFile(ParamClassPtr entry, RString className, RString path, RString filePrefix, RString fileSuffix) {
    _plantType.UpdateMergeFile(entry, className, path, filePrefix, fileSuffix);
  }

  CTreeModel CTreeEngine::GetTreeModel()
  {
    return CTreeModel(_pPSBranch,_pPSTriplane,_texFactory);
  }

  CTreeModel CTreeEngine::GetTreeModelForPreview()
  {
    Ref<CPrimitiveStream> branch=new CPrimitiveStream(*_pPSBranch.GetRef(),true);
    Ref<CPrimitiveStream> triplane=new CPrimitiveStream(*_pPSTriplane.GetRef(),true);
    return CTreeModel(branch,triplane,_texFactory);
  }


  int CTreeModel::Draw(D3DXMATRIX &matView,
                            D3DXMATRIX &matProj,
                            Vector3Par lightDirection,bool sceneBlock) {
    HRESULT hr;
    
    if (_psBranch.IsNull() || _psTriplane.IsNull() || _texFact==0) return 0;
    
    
    ComRef<IDirect3DDevice8> pD3DDevice;
    pD3DDevice << _texFact->GetRenderDevice();


    // Drawing
	  if (sceneBlock)
    {
      hr = pD3DDevice->BeginScene();
      assert(SUCCEEDED(hr));
    }

    // Rendering
    D3DXMATRIX matTemp;

    // Branch rendering
/*
    pD3DDevice->SetVertexShader(_hVSBranch);
    pD3DDevice->SetPixelShader(_hPSBranch);*/
    _texFact->PrepareBranchPreview();
    D3DXMatrixTranspose(&matTemp, &matView);
    pD3DDevice->SetVertexShaderConstant(0, &matTemp, 4);
    D3DXMatrixTranspose(&matTemp, &matProj);
    pD3DDevice->SetVertexShaderConstant(4, &matTemp, 4);
    float BranchC8[] = {lightDirection.X(), lightDirection.Y(), lightDirection.Z(), 0.0f};
    pD3DDevice->SetVertexShaderConstant(8, BranchC8, 1);
    //float BranchC9[] = {0.5f, 1.0f, 0.0f, 0.0f};
    //pD3DDevice->SetVertexShaderConstant(9, BranchC9, 1);
    float BranchpC0[] = {0.4f, 0.4f, 0.4f, 0.0f}; // Ambient light color
    pD3DDevice->SetPixelShaderConstant(0, BranchpC0, 1);
    _psBranch->Draw();

    DWORD state;
    pD3DDevice->GetRenderState(D3DRS_FILLMODE,&state);
    BOOL setTextures=state==D3DFILL_SOLID;

    // Polyplane rendering
/*    pD3DDevice->SetVertexShader(_hVSTriplane);
    pD3DDevice->SetPixelShader(_hPSTriplane);*/
    _texFact->PrepareTriplanePreview();
    D3DXMatrixTranspose(&matTemp, &matView);
    pD3DDevice->SetVertexShaderConstant(0, &matTemp, 4);
    D3DXMatrixTranspose(&matTemp, &matProj);
    pD3DDevice->SetVertexShaderConstant(4, &matTemp, 4);
    float C8[] = {-lightDirection.X(), -lightDirection.Y(), -lightDirection.Z(), 0.0f};
    pD3DDevice->SetVertexShaderConstant(8, C8, 1);
    float C9[] = {0.5f, 1.0f, 0.0f, 0.0f};
    pD3DDevice->SetVertexShaderConstant(9, C9, 1);
    float pC0[] = {0.4f, 0.4f, 0.4f, 0.0f}; // Ambient light color
    pD3DDevice->SetPixelShaderConstant(0, pC0, 1);
    _psTriplane->Draw(setTextures);

    if (sceneBlock)
    {    
      // Finalizing
      pD3DDevice->EndScene();
    }

    //int a = _pPSTriplane->GetIndexCount() / 36;
    int result=_psBranch->GetIndexCount() / 3 + _psTriplane->GetIndexCount() / 3;  

    return result;
  }


  void CTreeEngine::Reset()
  {
    _pPSBranch->Clear(false);
    _pPSTriplane->Clear(true);
    // Create stream data
    _nodeInfoList.Clear();
    _plantType.Reset();  
    

  }

  void CTreeEngine::PrepareScriptCommands(GameState &gState)
  {
    CComponentChildBuffer::RegisterCommands(gState);
    CComponent::RegisterCommands(gState);
    CScriptedComponent::RegisterCommands(gState);
    CPrimitive::RegisterCommands(gState);
    CPBunch::RegisterCommands(gState);
    CPFunnel::RegisterCommands(gState);
    CPGutter::RegisterCommands(gState);
    CPLeaf ::RegisterCommands(gState);
    CPStar::RegisterCommands(gState);
    CPTest::RegisterCommands(gState);
    CPTree::RegisterCommands(gState);
    CPTreeNode::RegisterCommands(gState);
    CPStem::RegisterCommands(gState);
    CPHeel::RegisterCommands(gState);
    CScriptedPolyplane::RegisterToGameState(&gState);
    CPlantType::RegisterCommands(gState);
    O2ScriptClass::PrepareGameState(gState);
    
  }

  void CPlantType::RegisterPolyplaneFactory(int index, IPolyplaneFactory *factory)
  {
    _polyplaneTypes.erase(index);
    _polyplaneTypes.insert(std::pair<int,SRef<IPolyplaneFactory> >(index,SRef<IPolyplaneFactory>(factory)));
  }

  class ScriptedPolyplaneFactory: public IPolyplaneFactory
  {
    RString _name;
    int _countPlanes;
    int _maxFaces;
    RString _renderScript;
    RString _useScript;
    GameValue _userData;
    GameState &_gstate;
    
  public:
    ScriptedPolyplaneFactory(GameState &gstate,RString name, int countPlanes,
      int maxFaces, RString renderScript, RString useScript,
      GameValue userData):_gstate(gstate),_name(name),_countPlanes(countPlanes),
      _maxFaces(maxFaces),_renderScript(renderScript),_useScript(useScript),
      _userData(userData.GetData()->Clone()) {}

    CPolyplane *CreatePolyplane()
    {
      CScriptedPolyplane *p=new CScriptedPolyplane(_gstate);
      p->SetParameters(_name,_countPlanes,_maxFaces,_renderScript,_useScript,_userData);
      return p;
    }
  };


  static GameValue RegisterPolyplane(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
#if USE_PRECOMPILATION
    const GameType scripttype(GameCode);
#else
    const GameType scripttype(GameString);
#endif
    int index=toInt(oper1);
    const GameArrayType &arr=oper2;
    if (arr.Size()==1 || arr.Size()==2)
    {
      if (arr[0].GetType()==GameArray)
      {
        const GameArrayType &polytype=arr[0];
        GameValue userData;
        if (arr.Size()==2) userData=arr[1];
        if (polytype.Size()==5)
        {
          if (polytype[0].GetType()==GameString)
          {
            if (polytype[1].GetType()==GameScalar)
            {
              if (polytype[2].GetType()==GameScalar)
              {
                if (polytype[3].GetType()==scripttype)
                {
                  if (polytype[4].GetType()==scripttype)
                  {
                    if (SCurrentPlant!=0)
                    {
                      SCurrentPlant->RegisterPolyplaneFactory(index,
                        new ScriptedPolyplaneFactory(*const_cast<GameState *>(state),
                        polytype[0],toInt(polytype[1]),toInt(polytype[2]),
                        polytype[3],polytype[4],userData));
                      return GameValue();
                    }
                    else
                      state->SetError(EvalForeignError,"Command is not allowed here");
                  }
                  else
                    state->TypeError(scripttype,polytype[4].GetType(),"parameter 1,5");
                }
                else
                  state->TypeError(scripttype,polytype[3].GetType(),"parameter 1,4");
              }
              else
                state->TypeError(GameScalar,polytype[2].GetType(),"parameter 1,3");
            }
            else
              state->TypeError(GameScalar,polytype[1].GetType(),"parameter 1,2");
          }
          else
            state->TypeError(GameString,polytype[0].GetType(),"parameter 1,1");
        }
        else
          state->TypeError(GameString,arr[0].GetType(),"parameter 1");
      }
      else
        state->TypeError(GameArray,GameArray,"Excepting one or two parameters");
    }
  return GameValue();
  }

  void CPlantType::RegisterCommands(GameState &gstate)
  {
    gstate.NewOperator(GameOperator(GameNothing,"registerPolyplaneType",function,&RegisterPolyplane,GameScalar,GameArray TODO_OPERATOR_DOCUMENTATION));
  }


  RepresentativeArray CPlantType::GetClusterArray(int mode, float representativesPerm3) const {
    RepresentativeArray res;

    if (mode == 0)  //mode 0 = empty
        return res;
    if (mode == 1)  {//mode 1 
        res.Clear();
        for (int i = 0; i<_pointList.NPoints(); i++) {
            Representative x;
            x._sumPosition = _pointList.GetPoint(i);
            x._sumCenterDistance = representativesPerm3/2;
            x._n = 1;
            res.Add(x);
        }
        return res;
        
    }
    _pointList.FindRepresentatives(1.0f/representativesPerm3,res);
      
    if (mode == 3) {
        //TODO:Duplicate polyplanes;
        RefArray<SPolyplaneWithData> &polyplanes = _polyplaneSpace.GetVectorArray();
        for (int i = 0; i<polyplanes.Size();i++) {
            const SPolyplaneWithData *pol = polyplanes[i];
            const CPolyplane *cpol  = pol->_pPolyplane;
            const RepresentativeArray &ra = cpol->GetRepresentatives();
            res.Append(ra);
        }    
    }
    return res;  
  }


  SBoundingBox CPlantType::GetBoundingBox() const {
      SBoundingBox bbox;
      Matrix4P ident(MIdentity);
      _pointList.ComputeBoundingBox(ident,bbox);
      return bbox;
  }
}