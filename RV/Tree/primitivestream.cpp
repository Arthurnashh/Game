#include "RVTreeCommon.h"
#include "primitivestream.h"
#include <el/MultiThread/ThreadBase.h>

//extern int g_N0;

//--------------------------------------------------------------------------------------


namespace TreeEngine
{
    CPrimitiveStage::CPrimitiveStage() {
    _pTextureS0 << NULL;
    _pTextureS1 << NULL;
    _MinVertexIndex = UINT_MAX;
    _MaxVertexIndex = 0;
  }

  void CPrimitiveStage::Clear(bool freeTextures) 
  {
    _Indices.Resize(0);
    if (freeTextures)
    {
      _pTextureS1 << 0;
      _pTextureS0 << 0;
    }
    _MinVertexIndex = UINT_MAX;
    _MaxVertexIndex = 0;
  }

  void CPrimitiveStage::AddIndex(const unsigned long &Index) {
    _Indices.Add(Index);
    if (Index < _MinVertexIndex) _MinVertexIndex = Index;
    if (Index > _MaxVertexIndex) _MaxVertexIndex = Index;
  }

  int CPrimitiveStage::Size() const {
    return _Indices.Size();
  }

  const unsigned long *CPrimitiveStage::Data() const {
    return _Indices.Data();
  }

  UINT CPrimitiveStage::GetMinVertexIndex() {
    return _MinVertexIndex;
  }

  UINT CPrimitiveStage::GetNumVertices() {
    return _MaxVertexIndex - _MinVertexIndex + 1;
  }

  //--------------------------------------------------------------------------------------
  CPrimitiveStream::CPrimitiveStream() {
  }

  CPrimitiveStream::CPrimitiveStream(ComRef<IDirect3DDevice8> &pD3DDevice) {
    // Save device
    //pD3DDevice->AddRef();
    _pD3DDevice = pD3DDevice;
  }

  CPrimitiveStream::CPrimitiveStream(CPrimitiveStream &PrimitiveStream) {
    _pD3DDevice = PrimitiveStream._pD3DDevice;// _pD3DDevice->AddRef();
    _pVB = PrimitiveStream._pVB;
    _pIB = PrimitiveStream._pIB;
    _VertexSize = PrimitiveStream._VertexSize;
    _IndexCount = PrimitiveStream._IndexCount; // FLY???
    // Copy texture references
    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      _PS[i]._pTextureS0 = PrimitiveStream._PS[i]._pTextureS0;
      _PS[i]._TS0Name = PrimitiveStream._PS[i]._TS0Name;
      _PS[i]._pTextureS1 = PrimitiveStream._PS[i]._pTextureS1;
      _PS[i]._TS1Name = PrimitiveStream._PS[i]._TS1Name;
    }
  }

  CPrimitiveStream::CPrimitiveStream(CPrimitiveStream &PrimitiveStream,bool full) 
  {
    _pD3DDevice = PrimitiveStream._pD3DDevice;
    Init(PrimitiveStream._VertexSize,PrimitiveStream._FVF);
    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) 
    {
      _PS[i]=PrimitiveStream._PS[i];
      _StageIndexPos[i]=PrimitiveStream._StageIndexPos[i];
    }
    _Vertices=PrimitiveStream._Vertices;
    _IndexCount=PrimitiveStream._IndexCount;
  }


  void CPrimitiveStream::Init(CPrimitiveStream &PrimitiveStream) {
    _pD3DDevice = PrimitiveStream._pD3DDevice;
    _VertexSize = PrimitiveStream._VertexSize;
    
    _pVB = PrimitiveStream._pVB;
    _pIB = PrimitiveStream._pIB;


  /*
    HRESULT hr;

    // Create vertex buffer
    hr = _pD3DDevice->CreateVertexBuffer(VB_SIZE * _VertexSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, _pVB.Init());
    assert(SUCCEEDED(hr));

    // Create index buffer
    hr = _pD3DDevice->CreateIndexBuffer(IB_SIZE * sizeof(WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, _pIB.Init());
    assert(SUCCEEDED(hr));
  */

    _IndexCount = 0;
    // Copy texture references
    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      _PS[i]._pTextureS0 = PrimitiveStream._PS[i]._pTextureS0;
      _PS[i]._TS0Name = PrimitiveStream._PS[i]._TS0Name;
      _PS[i]._pTextureS1 = PrimitiveStream._PS[i]._pTextureS1;
      _PS[i]._TS1Name = PrimitiveStream._PS[i]._TS1Name;
      _PS[i]._adressU = PrimitiveStream._PS[i]._adressU;
      _PS[i]._adressV = PrimitiveStream._PS[i]._adressV;
      _PS[i]._isAlpha = PrimitiveStream._PS[i]._isAlpha;
    }
  }

  void CPrimitiveStream::Init(int VertexSize, DWORD FVF) {
    // Save parametres
    _VertexSize = VertexSize;
    _FVF=FVF;

//    HRESULT hr;

    // Create vertex buffer
/*    hr = _pD3DDevice->CreateVertexBuffer(VB_SIZE * _VertexSize, D3DUSAGE_WRITEONLY, FVF, D3DPOOL_MANAGED, _pVB.Init());
    assert(SUCCEEDED(hr));*/

    // Create index buffer
/*    hr = _pD3DDevice->CreateIndexBuffer(IB_SIZE * sizeof(WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, _pIB.Init());
    assert(SUCCEEDED(hr));*/
    
    _IndexCount = 0;
  }

  /*
  void CPrimitiveStream::CopyData(CPrimitiveStream &primitiveStream) {
    _pD3DDevice = primitiveStream._pD3DDevice;
    _VertexSize = primitiveStream._VertexSize;
    _pVB = primitiveStream._pVB;
    _pIB = primitiveStream._pIB;
    _IndexCount = primitiveStream._IndexCount;

    // Copy texture references and stageIndexPos
    int i;
    for (i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      _PS[i]._pTextureS0 = primitiveStream._PS[i]._pTextureS0;
      _PS[i]._TS0Name = primitiveStream._PS[i]._TS0Name;
      _PS[i]._pTextureS1 = primitiveStream._PS[i]._pTextureS1;
      _PS[i]._TS1Name = primitiveStream._PS[i]._TS1Name;
      _StageIndexPos[i] = primitiveStream._StageIndexPos[i];
    }

    // Copy array of vertices
    for (i = 0; i < primitiveStream._Vertices.Size(); i++) {
      _Vertices[i] = primitiveStream._Vertices[i];
    }

  }
  */

  void CPrimitiveStream::Clear(bool freeTextures) {
    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      _PS[i].Clear(freeTextures);
    }
    _IndexCount = 0;
    _Vertices.Resize(0);
  }

  unsigned long CPrimitiveStream::AddVertex(void *pData)
  {
    int OldSize = _Vertices.Size() / _VertexSize;
/*    if (OldSize < VB_SIZE)
    {*/
      for (int i = 0; i < _VertexSize; i++)
      {
        _Vertices.Add(((BYTE*)pData)[i]);
      }
/*    }
    else
    {
      Fail("Error: Too many vertices");
    }
    */
    return OldSize;
  }

  void CPrimitiveStream::GetVertex(int vertexIndex, void *pData) const
  {
    memcpy(pData, (void*)&_Vertices[vertexIndex * _VertexSize], _VertexSize);
  }

  void *CPrimitiveStream::GetVertexData(int vertexIndex)
  {
    assert(vertexIndex * _VertexSize + _VertexSize <= _Vertices.Size());
    return &_Vertices[vertexIndex * _VertexSize];
  }

  void CPrimitiveStream::AddIndex(int Stage, unsigned long Index) {
    if ((Stage < 0) || (Stage >= PRIMITIVESTAGE_COUNT)) {
      LogF("Stage index out of bounds while indices adding.. (%d)", Stage);
      return;
    }
/*    if (_IndexCount < IB_SIZE) {*/
      _PS[Stage].AddIndex(Index);
      _IndexCount++;
/*    }
    else {
      LogF("Too many indices.");
    }*/
  }

  unsigned long CPrimitiveStream::GetNewVertexIndex() {
    return _Vertices.Size() / _VertexSize;
  }

  void CPrimitiveStream::RegisterTextures(int Stage,
                                          ComRef<IDirect3DTexture8> &textureS0,
                                          RString TS0Name,
                                          ComRef<IDirect3DTexture8> &textureS1,
                                          RString TS1Name,
                                          D3DTEXTUREADDRESS adressU,
                                          D3DTEXTUREADDRESS adressV,
                                          bool alpha) {
    if ((Stage < 0) || (Stage >= PRIMITIVESTAGE_COUNT)) {
      LogF("Stage index out of bounds while texture registering. (%d)", Stage);
      return;
    }

    _PS[Stage]._pTextureS0 = textureS0;
    _PS[Stage]._TS0Name = TS0Name;
    _PS[Stage]._pTextureS1 = textureS1;
    _PS[Stage]._TS1Name = TS1Name;

  /*
    if (textureS0 != NULL) {
      _PS[Stage]._pTextureS0 = textureS0;
      _PS[Stage]._pTextureS0->AddRef();
    }
    if (textureS1 != NULL) {
      _PS[Stage]._pTextureS1 = textureS1;
      _PS[Stage]._pTextureS1->AddRef();
    }
    _PS[Stage]._TS0Name = TS0Name;
    _PS[Stage]._TS1Name = TS1Name;
  */

  /*
  //  if (textureS0.NotNull()) {
      _PS[Stage]._pTextureS0 = textureS0;
      _PS[Stage]._pTextureS0->AddRef();
      _PS[Stage]._TS0Name = TS0Name;
  //  }
  //  if (textureS1.NotNull()) {
      _PS[Stage]._pTextureS1 = textureS1;
      _PS[Stage]._pTextureS1->AddRef();
      _PS[Stage]._TS1Name = TS1Name;
  //  }
  */

    _PS[Stage]._adressU = adressU;
    _PS[Stage]._adressV = adressV;
    _PS[Stage]._isAlpha = alpha;
  }

  void CPrimitiveStream::SetTextureContent(int stage,ComRef<IDirect3DTexture8> &textureS0, ComRef<IDirect3DTexture8> &textureS1)
  {
    if ((stage < 0) || (stage >= PRIMITIVESTAGE_COUNT)) {
      LogF("Stage index out of bounds while texture registering. (%d)", stage);
      return;
    }

    _PS[stage]._pTextureS0 = textureS0;
    _PS[stage]._pTextureS1 = textureS1;

  }

  void CPrimitiveStream::Prepare() 
  {
    HRESULT hr;
    BYTE *pData;
    UINT LastIndex = 0;

    int vxCount=_Vertices.Size()/_VertexSize;

    // Finish in case number of vertices or number of indices is zero
    if (vxCount==0 || _IndexCount==0)
    {
      _pVB << 0;
      _pIB << 0;
      return;
    }

    if (vxCount>=65535)
    {
      //setup large vertex buffer instead index buffer
      hr = _pD3DDevice->CreateVertexBuffer(_IndexCount*_VertexSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, _pVB.Init());
      if (FAILED(hr))
      {
        LogF("Unable to setup vertex buffer - HRESULT: %08X",hr);
        return;
      }
            
      hr=_pVB->Lock(0,0,&pData,0);
      for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) 
      {
        if (_PS[i].Size() > 0) 
        {
          {
            assert((_PS[i].Size() % 3) == 0);
            _StageIndexPos[i] = LastIndex;
            for (int j=0;j<_PS[i].Size();j++)
            {
              memcpy(pData+_VertexSize*LastIndex,_Vertices.Data()+_VertexSize*_PS[i].Data()[j],_VertexSize);
              LastIndex++;
            }
          }
        }
      }
      _pVB->Unlock();
      _pIB << 0;
        
    }
    else
    {
      hr = _pD3DDevice->CreateVertexBuffer(_Vertices.Size(), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, _pVB.Init());
      assert(SUCCEEDED(hr));
      hr = _pVB->Lock(0, _Vertices.Size(), &pData, 0);
      assert(SUCCEEDED(hr));
      memcpy(pData, _Vertices.Data(), _Vertices.Size());
      _pVB->Unlock();
      

      hr = _pD3DDevice->CreateIndexBuffer(_IndexCount* sizeof(WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, _pIB.Init());
      assert(SUCCEEDED(hr));

      hr = _pIB->Lock(0, _IndexCount * sizeof(WORD), &pData, 0);
      assert(SUCCEEDED(hr));
      for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) 
      {
        if (_PS[i].Size() > 0) 
        {
            assert((_PS[i].Size() % 3) == 0);
            _StageIndexPos[i] = LastIndex;
            for (int j=0;j<_PS[i].Size();j++)
              ((WORD *)pData)[LastIndex++]=(WORD)_PS[i].Data()[j];
        }
      }
      _pIB->Unlock();
    }
    assert(LastIndex == _IndexCount);
  }

  void CPrimitiveStream::Draw(BOOL SetTextures) {
    HRESULT hr;

    if (_Vertices.Size()==0) return; //nothing to draw;
    if (_pVB.IsNull()) 
    {
      LogF("Cannot draw - primitive stream is not prepared");
      return;
    }
    hr = _pD3DDevice->SetStreamSource(0, _pVB, _VertexSize);
    assert(SUCCEEDED(hr));

    hr = _pD3DDevice->SetIndices(_pIB, 0);
    assert(SUCCEEDED(hr));

    _pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    _pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    _pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);

    _pD3DDevice->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    _pD3DDevice->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    _pD3DDevice->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
    
    _pD3DDevice->SetRenderState( D3DRS_BLENDOP,   D3DBLENDOP_ADD);
    _pD3DDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    _pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    _pD3DDevice->SetRenderState( D3DRS_ALPHAREF, 128 );
    _pD3DDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATER );

    

    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      if (_PS[i].Size() > 0) {
        _pD3DDevice->SetTexture(0, NULL);
        _pD3DDevice->SetTexture(1, NULL);

        if (SetTextures) {
          //assert(_PS[i]._pTextureS0.NotNull());
          //assert(_PS[i]._pTextureS1.NotNull());
          _pD3DDevice->SetTexture(0, _PS[i]._pTextureS0);
          _pD3DDevice->SetTexture(1, _PS[i]._pTextureS1);
        }

        // Filtering
        _pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, _PS[i]._adressU);
        _pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, _PS[i]._adressV);
        _pD3DDevice->SetTextureStageState(1, D3DTSS_ADDRESSU, _PS[i]._adressU);
        _pD3DDevice->SetTextureStageState(1, D3DTSS_ADDRESSV, _PS[i]._adressV);

        if (_PS[i]._isAlpha)
        {
          _pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,   TRUE );
          _pD3DDevice->SetRenderState( D3DRS_ALPHAREF, 0);
        }
        else
        {
          _pD3DDevice->SetRenderState( D3DRS_ALPHAREF, 128);
          _pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,   FALSE );
        }
        

        if (_pIB.IsNull()) //use large vertex buffer
        {
          hr = _pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, _StageIndexPos[i], _PS[i].Size() / 3);
          assert(SUCCEEDED(hr));
        }
        else
        {        
//        if ((_PS[i].Size() % 3) != 0) DebugBreak();
          hr = _pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, _PS[i].GetMinVertexIndex(), _PS[i].GetNumVertices(), _StageIndexPos[i], _PS[i].Size() / 3);
          assert(SUCCEEDED(hr));
        }

/*        ComRef<IDirect3DSurface8> rt;
      D3DLOCKED_RECT lr;
      _pD3DDevice->EndScene();
      _pD3DDevice->GetRenderTarget(rt.Init());
      rt->LockRect(&lr,0,0);
      LogF("Serialize lock...");
      rt->UnlockRect();
      _pD3DDevice->BeginScene();*/

        //g_N0 += _PS[i].Size() / 3;
      }
    }

  }

  const AutoArray<BYTE> &CPrimitiveStream::GetVertices() {
    return _Vertices;
  }

  CPrimitiveStage *CPrimitiveStream::GetPrimitiveStage(int i) {
    return &_PS[i];
  }

  int CPrimitiveStream::GetIndexCount() {
    return _IndexCount;
  }

  void CPrimitiveStream::AddNewTextureNames(StringArray &names) {
    for (int i = 0; i < PRIMITIVESTAGE_COUNT; i++) {
      if (_PS[i].Size() > 0) {
        RString NewName = _PS[i]._TS0Name;
        int j;
        for (j = 0; j < names.Size(); j++) {
          if (names[j] == NewName) break;
        }
        if (j == names.Size()) {
          names.Add(NewName);
        }
      }
    }
  }

  class CompareFace
  {
  public:
    Vector3 *vertices;
    int &numComp;

    CompareFace(Vector3 *vertices, int &numComp):vertices(vertices), numComp(numComp) {}

    enum CompareResult
    {
      before,
      intermediate,
      after,
      unknown
    };

    ///Tests position of first face relative to second face
    /**
     * @param triIndexTest array of 3 indexes of tested face
     * @param triIndexRef array of 3 indexes of ref face
     * @retval before tested face is entire before ref face
     * @retval intermediate tested face has some vertices before and some vertces after the ref face
     * @retval after tested face us enture after reef face
     * @retval unknown error
     */
    static CompareResult PointLocation(const Vector3 &point, float pa, float pb, float pc, float pd)
    {
      // pa*x+pb*y+pc*z+pd
      float r=pa*point.X()+pb*point.Y()+pc*point.Z()+pd;
      if (r>0) return before;
      if (r<0) return after;
      return intermediate;
    }

    CompareResult CompareTwo(const unsigned long *triIndexTest, const unsigned long *triIndexRef)
    {
      const Vector3 &refPos=vertices[triIndexRef[0]];
      Vector3 d1=vertices[triIndexRef[1]]-refPos;
      Vector3 d2=vertices[triIndexRef[2]]-refPos;
      Vector3 normal=d1.CrossProduct(d2);
      float pa=normal.X(),
            pb=normal.Y(),
            pc=normal.Z(),
            pd=-(pa*refPos.X()+pb*refPos.Y()+pc*refPos.Z());

      CompareResult res1=PointLocation(vertices[triIndexTest[0]],pa,pb,pc,pd);
      CompareResult res2=PointLocation(vertices[triIndexTest[1]],pa,pb,pc,pd);
      if (res1==intermediate) res1=res2;
      if (res1!=res2 && res2!=intermediate) return intermediate;
      CompareResult res3=PointLocation(vertices[triIndexTest[2]],pa,pb,pc,pd);
      if (res1==intermediate) res1=res3;
      if (res1!=res3 && res3!=intermediate) return intermediate;
      return res1;
    }


    int operator()(unsigned long **f,unsigned long **l)
    {
      const unsigned long *firstFc=*f;
      const unsigned long *lastFc=*l;

      CompareResult res1=CompareTwo(firstFc,lastFc);
      CompareResult res2=CompareTwo(lastFc,firstFc);
      //both faces has located in same side relative to each other
      //we cannot sort these faces - they are equal
      int ares=-(f>l)+(f<l);
      int zres=0;
      if (res1==res2) zres=0;
      
      //first face is before second
      //in depend on state of second face, first face should be drawn as last
      else if (res1==before) zres=1;

      //first face is after second
      //in depend on state of second face, first face should be drawn as first
      else if (res1==after) zres=-1;

      //first face is intermediate - we must explore state of second face
      else if (res2==before) zres=-1;

      else if (res2==after) zres=1;
      if (zres!=ares)
      {
        numComp++;
      }
      if (zres==0) return ares;

      return zres;
    }
  };
  
  


  void CPrimitiveStage::SortStage(Vector3 *vertices)
  {
    int faces=_Indices.Size()/3;
    unsigned long **fclist=new unsigned long *[faces];

    
    int numComp=0;
    CompareFace comp(vertices,numComp);
    int maxcmp=INT_MAX;
    int retries=3;

    for (int i=0;i<faces;i++) fclist[i]=_Indices.Data()+(i*3);
    for (int i=1;i<faces;i=i*2)
      for (int j=0;j<faces;j=j+2*i)
      {
        int min=j;
        int max=j+2*i;
        if (max>faces) max=faces;
        if (max>min)  QSort(fclist+min,max-min,comp);
        
      }
      
    do 
    {

      numComp=0;
      QSort(fclist,faces,comp);
      if (maxcmp>numComp) 
      {
        maxcmp=numComp;
        retries=5;
      }
      else
      {
        retries--;
      }
    } 
    while(retries>0);

/*    AutoArray<int,MemAllocLocal<int, 256> >groupLow;

    for (int i=0;i<faces;i++)
    {
      int best=i;
      groupLow.Clear();
      groupLow.Add(i);
        for (int k=i+1;k<faces;k++)
        {
          int cnt=0;
          for (int j=0;j<groupLow.Size();j++)
          {
            if (comp(fclist+groupLow[j],fclist+k)<0) break;
            cnt++;
          }
          if (cnt==groupLow.Size())
          {
            best=k;
            groupLow.Add(k);
          }
        }      
      if (best!=i)
      {
        unsigned long *x=fclist[best];
        fclist[best]=fclist[i];
        fclist[i]=x;
      }
    }
/*

    for (int i=0;i<faces;i++)
    {
      unsigned long *f=fclist[i];
      groupLow.Clear();
      groupLow.Add(f);
      for (int j=i+1;j<faces;j++)
      {
        int cnt=0;
        for (int k=0;k<groupLow.Size();k++)
        {
          if (comp(&groupLow[k],fclist+j)>0) cnt++;
        }
        if (cnt==groupLow.Size())
        {
          groupLow.Add(fclist[j]);
        }
      }
      if (groupLow.Size())
      {
        int j,k,l=groupLow.Size()-1;
        for (j=faces-1,k=faces-1;j>i;j--,k--)
        {
          if (fclist[j]==groupLow[l])
          {
            j--;
            l--;
          }
          if (k!=j)
          {
            fclist[k]=fclist[j];
          }
        }
        for (j=groupLow.Size()-1;j>=0;j--,k--)
        {
          fclist[k]=groupLow[j];
        }
      }
    }

/*      
      int swpmax=faces;;
      int swp=0;
      while (comp(fclist+i,fclist+faces-1)>0)
      {
        unsigned long *x=fclist[i];
        fclist[i]=fclist[faces-1];
        fclist[faces-1]=x;
        swp=0;
        for (int j=i+1;j<faces;j++)
        {
          if (comp(fclist+i,fclist+j)>0)
          {
            swp++;
            unsigned long *x=fclist[j-1];
            fclist[j-1]=fclist[j];
            fclist[j]=x;
          }
        }        
      }
    }
     */


    AutoArray<unsigned long> newInd;
    newInd.Resize(_Indices.Size());
    for (int i=0;i<faces;i++)
    {
      unsigned long *p=newInd.Data()+(i*3);
      p[0]=fclist[i][0];
      p[1]=fclist[i][1];
      p[2]=fclist[i][2];
    }

    _Indices=newInd;
    delete [] fclist;

  }


  void CPrimitiveStream::SortStages()
  {
    int vertcnt=_Vertices.Size()/_VertexSize;
    Vector3 *vertices=new Vector3[vertcnt];
    for (int i=0, j=0;i<_Vertices.Size();i+=_VertexSize,j++)
    {
      Vector3 *r=reinterpret_cast<Vector3 *>(_Vertices.Data()+i);
      vertices[j]=*r;
    }
    for (int i=0;i<PRIMITIVESTAGE_COUNT;i++) if (_PS[i].Size()>0 && _PS[i]._isAlpha)
    {
      _PS[i].SortStage(vertices);
    }
    delete [] vertices;
  }

  AutoArray<Vector3> CPrimitiveStream::ExportPointsForTexture(IDirect3DTexture8 *texture) const
  {
    const Array<BYTE> &vertices=GetVertices();
    AutoArray<Vector3> res;

    for (int i=0;i<PRIMITIVESTAGE_COUNT;i++)
      if (_PS[i].Size() && (_PS[i]._pTextureS0==texture || _PS[i]._pTextureS1==texture))
      {
        const unsigned long *indices=_PS[i].Data();
        int size=_PS[i].Size();
        for (int i=0;i<size;i++)
        {
          Vector3 nv;
          memcpy(&nv,vertices.Data()+_VertexSize*indices[i],sizeof(Vector3));
          res.Add(nv);
        }
      }
    return res;
  }

  AutoArray<Vector3> CPrimitiveStream::ExportPointsForStage(int index) const
  {
    const Array<BYTE> &vertices=GetVertices();
    AutoArray<Vector3> res;
    const unsigned long *indices=_PS[index].Data();
    int size=_PS[index].Size();
    for (int i=0;i<size;i++)
     {
        Vector3 nv;
        memcpy(&nv,vertices.Data()+_VertexSize*indices[i],sizeof(Vector3));
        res.Add(nv);
     }      
    return res;
  }

  ComRef<IDirect3DTexture8> CPrimitiveStream::GetTextureAtIndex(int index,bool normalMap)
  {
    if (index>=0 && index<PRIMITIVESTAGE_COUNT) return normalMap?_PS[index]._pTextureS1:_PS[index]._pTextureS0;
    return ComRef<IDirect3DTexture8>();
  }

  bool CPrimitiveStream::IsStageUsed(int index) const
  {
    if (index>=0 && index<PRIMITIVESTAGE_COUNT) return _PS[index].Size()!=0;
    return false;    
  }


}
TypeIsSimpleZeroed(unsigned long *);
