#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "texturefactory.h"
#include "primitivestream.h"


namespace TreeEngine
{
  DrawParameters DPDefault;

#define CLEAR_COLOR 0x00000000
#define CLEAR_NORMAL 0x008080ff

  extern DWORD dwBranchDecl[];
  extern DWORD dwPolyplaneDecl[];

  static BaseTextureFactory::InitResult InitShaderVx(PDirect3DDevice &pD3DDevice, const char *shaderPath, const char *name, const DWORD *decl,DWORD *handle)
  {
    ComRef<ID3DXBuffer> pCode;
    char *fname=strcat(strcpy((char *)alloca(strlen(shaderPath)+strlen(name)+1),shaderPath),name);
    D3DXAssembleShaderFromFile(fname,0,0,pCode.Init(),0);
    if (pCode.IsNull()) return BaseTextureFactory::InitResult(fname);
    HRESULT hr=pD3DDevice->CreateVertexShader(decl,reinterpret_cast<DWORD *>(pCode->GetBufferPointer()),handle,0);
    if (hr!=S_OK) 
    {
      RString c;
      sprintf(c.CreateBuffer(strlen(fname)+50),"%s (%08X)",fname,hr);
      return BaseTextureFactory::InitResult(c);
    }
    else
      return BaseTextureFactory::InitResult();
  }

  static BaseTextureFactory::InitResult InitShaderPx(PDirect3DDevice &pD3DDevice, const char *shaderPath, const char *name, DWORD *handle)
  {
    ComRef<ID3DXBuffer> pCode;
    char *fname=strcat(strcpy((char *)alloca(strlen(shaderPath)+strlen(name)+1),shaderPath),name);
    D3DXAssembleShaderFromFile(fname,0,0,pCode.Init(),0);
    if (pCode.IsNull()) return BaseTextureFactory::InitResult(fname);
    HRESULT hr=pD3DDevice->CreatePixelShader(reinterpret_cast<DWORD *>(pCode->GetBufferPointer()),handle);
    if (hr!=S_OK) 
    {
      RString c;
      sprintf(c.CreateBuffer(strlen(fname)+50),"%s (%08X)",fname,hr);
      return BaseTextureFactory::InitResult(c);
    }
    else
      return BaseTextureFactory::InitResult();
  }


  BaseTextureFactory::InitResult BaseTextureFactory::Init(PDirect3DDevice &pD3DDevice,const char *shaderPath) 
  {
    // Save device
    _pD3DDevice = pD3DDevice;

    InitResult res;
    // Create shaders for rendering
    res=InitShaderVx(pD3DDevice,shaderPath,"branch.vsh",dwBranchDecl,&_hVSBranch);
    if (!res) goto end;
    res=InitShaderPx(pD3DDevice,shaderPath,"branch.psh",&_hPSBranch);
    if (!res) goto end;
    res=InitShaderVx(pD3DDevice,shaderPath,"triplane.vsh",dwPolyplaneDecl,&_hVSTriplane);
    if (!res) goto end;
    res=InitShaderPx(pD3DDevice,shaderPath,"triplane.psh",&_hPSTriplane);
    if (!res) goto end;
    // Create shaders for branch map generation
    res=InitShaderVx(pD3DDevice,shaderPath,"color.vsh",dwBranchDecl,&_hVSBranchColor);
    if (!res) goto end;
    res=InitShaderPx(pD3DDevice,shaderPath,"color.psh",&_hPSBranchColor);
    if (!res) goto end;
    res=InitShaderVx(pD3DDevice,shaderPath,"normal.vsh",dwBranchDecl,&_hVSBranchNormal);
    if (!res) goto end;
    res=InitShaderPx(pD3DDevice,shaderPath,"normal.psh",&_hPSBranchNormal);
    if (!res) goto end;
    // Create shaders for polyplane map generation
    res=InitShaderVx(pD3DDevice,shaderPath,"color.vsh",dwPolyplaneDecl,&_hVSPolyplaneColor);
    if (!res) goto end;
    res=InitShaderPx(pD3DDevice,shaderPath,"color.psh",&_hPSPolyplaneColor);
    if (!res) goto end;
    res=InitShaderVx(pD3DDevice,shaderPath,"normal.vsh",dwPolyplaneDecl,&_hVSPolyplaneNormal);
    if (!res) goto end;
    res=InitShaderPx(pD3DDevice,shaderPath,"normal.psh",&_hPSPolyplaneNormal);
    if (!res) goto end;
end:
    return res;
  }

  static HRESULT CreatePolyplaneTexture(IDirect3DDevice8 *pD3DDevice, int textureSize, IDirect3DTexture8 **txt)
  {
    HRESULT hr=pD3DDevice->CreateTexture(textureSize, textureSize, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, txt);
    if (FAILED(hr))
      LogF("Unable to create texture dimension %d - HRESULT %08X (%d)",textureSize,hr,hr & 0xFFFF);
    return hr;
  }

  static void FillBackgroundWithAvgColor(ComRef<IDirect3DTexture8> &map, int level) {
    HRESULT hr;


    ComRef<IDirect3DSurface8> tmpMapSurface;
    hr = map->GetSurfaceLevel(level, tmpMapSurface.Init());
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Get source texture description
    D3DSURFACE_DESC desc;
    hr = tmpMapSurface->GetDesc(&desc);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Lock the texture
    D3DLOCKED_RECT lr;
    hr = tmpMapSurface->LockRect(&lr, NULL, 0);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Avg color
    int c0, c1, c2;
    c0 = c1 = c2 = 0;
    int colorCount = 0;

    for (unsigned int y = 0; y < desc.Height; y++) {
      D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
      for (unsigned int x = 0; x < desc.Width; x++) {
        if (row[x] != CLEAR_COLOR) {
          c0 += ((unsigned char*)&row[x])[0];
          c1 += ((unsigned char*)&row[x])[1];
          c2 += ((unsigned char*)&row[x])[2];
          colorCount++;
        }
      }
    }

    D3DCOLOR avgColor;
    if (colorCount > 0) {
      ((unsigned char*)&avgColor)[0] = c0 / colorCount;
      ((unsigned char*)&avgColor)[1] = c1 / colorCount;
      ((unsigned char*)&avgColor)[2] = c2 / colorCount;
    }
    else {
      ((unsigned char*)&avgColor)[0] = 0x00;
      ((unsigned char*)&avgColor)[1] = 0x00;
      ((unsigned char*)&avgColor)[2] = 0x00;
    }
    ((unsigned char*)&avgColor)[3] = 0x00;

    for (unsigned int y = 0; y < desc.Height; y++) {
      D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
      for (unsigned int x = 0; x < desc.Width; x++) {
        if (row[x] == CLEAR_COLOR) {
          row[x] = avgColor;
        }
      }
    }

    // Unlock the small texture
    hr = tmpMapSurface->UnlockRect();
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

  }



  static void Downsize(ComRef<IDirect3DTexture8> &mapSource, ComRef<IDirect3DTexture8> &mapDest, int srclevel=0, int trglevel=0)
  {
    HRESULT hr;

    // Get descriptions
    D3DSURFACE_DESC sourceDesc;
    hr = mapSource->GetLevelDesc(srclevel, &sourceDesc);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));
    D3DSURFACE_DESC destDesc;
    hr = mapDest->GetLevelDesc(trglevel, &destDesc);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Lock surfaces
    D3DLOCKED_RECT lrSource;
    hr = mapSource->LockRect(srclevel, &lrSource, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));
    D3DLOCKED_RECT lrDest;
    hr = mapDest->LockRect(trglevel, &lrDest, NULL, 0);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Retrieve the size coef
    __int64 sizeCoef = sourceDesc.Width / destDesc.Width;
    __int64 squareSizeCoef = sizeCoef * sizeCoef;

    // Copy data
    for (unsigned int y = 0; y < destDesc.Height; y++) {
      D3DCOLOR *destRow = (D3DCOLOR *)((BYTE*)lrDest.pBits + y * lrDest.Pitch);
      for (unsigned int x = 0; x < destDesc.Width; x++) {
        __int64 c0 = 0, c1 = 0, c2 = 0, c3 = 0;
        for (int j = 0; j < sizeCoef; j++) {
          D3DCOLOR *sourceRow = (D3DCOLOR *)((BYTE*)lrSource.pBits + (y * sizeCoef + j) * lrSource.Pitch);
          for (int i = 0; i < sizeCoef; i++) {
            D3DCOLOR value = sourceRow[x * sizeCoef + i];
            c0 += (value >> 24) & 0xFF;
            c1 += (value >> 16) & 0xFF;
            c2 += (value >>  8) & 0xFF;
            c3 += (value >>  0) & 0xFF;
          }
        }
        D3DCOLOR avgValue = 0;
        avgValue |= (c0 / squareSizeCoef) << 24;
        avgValue |= (c1 / squareSizeCoef) << 16;
        avgValue |= (c2 / squareSizeCoef) <<  8;
        avgValue |= (c3 / squareSizeCoef) <<  0;
        destRow[x] = avgValue;
      }
    }
    mapSource->UnlockRect(srclevel);
    mapDest->UnlockRect(trglevel);
  }


  static void CreateMipLevels(ComRef<IDirect3DTexture8> &map) {

    int levelCount = map->GetLevelCount();
    for (int i = 1; i < levelCount; i++) {
      Downsize(map,map,0,i);
    }
  }


  static void DiscretizeAlpha(ComRef<IDirect3DTexture8> &map) {
    HRESULT hr;

    int levelCount = map->GetLevelCount();
    for (int i = 0; i < levelCount; i++) {

      // Get source texture description
      D3DSURFACE_DESC desc;
      hr = map->GetLevelDesc(i, &desc);
      if (FAILED(hr)) LogF(DXGetErrorString8(hr));


      ComRef<IDirect3DSurface8> tmpMapSurface;
      hr = map->GetSurfaceLevel(i, tmpMapSurface.Init());
      if (FAILED(hr)) LogF(DXGetErrorString8(hr));

      // Lock the texture
      D3DLOCKED_RECT lr;
      hr = tmpMapSurface->LockRect(&lr, NULL, 0);
      if (FAILED(hr)) LogF(DXGetErrorString8(hr));

      int histogram[256];
      for (int i=0; i<256; i++) histogram[i] = 0;
      // 1) build alpha histogram and average alpha
      int aSum = 0;
      for (unsigned int y = 0; y < desc.Height; y++) {
        D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
        for (unsigned int x = 0; x < desc.Width; x++) {
          int a = ((unsigned char*)&row[x])[3];
          histogram[a]++;
          aSum += a;
        }
      }

      // 2) select alpha threshold so that texture does not become more transparent than it was
      // pixels are opaque when pixel.a>=thold
      int aboveThold = 0;
      int thold = 1;
      int desiredSumAlpha = aSum >> 8;
      for (int i=255; i>=0; i--)
      {
        aboveThold += histogram[i];
        // alpha of result picture would be aboveThold/n
        if (aboveThold >= desiredSumAlpha) {thold=i;break;}
      }

      for (unsigned int y = 0; y < desc.Height; y++) {
        D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
        for (unsigned int x = 0; x < desc.Width; x++) {
          if (((unsigned char*)&row[x])[3] >= thold) {
            ((unsigned char*)&row[x])[3] = 255;
          }
          else {
            ((unsigned char*)&row[x])[3] = 0;
          }
        }
      }
      //*/
      //-------------------------------------------------

      hr = tmpMapSurface->UnlockRect();
      if (FAILED(hr)) LogF(DXGetErrorString8(hr));
    }
  }

  struct SourceCoord
  {
    int x,y;
    SourceCoord(int xx, int yy):x(xx),y(yy){}
    int Distance(const SourceCoord &s) const
    {
      return (x-s.x)*(x-s.x) + (y-s.y)*(y-s.y);
    }
    bool Valid() const {return x>=0;}
    bool operator != (const SourceCoord &s) const
    {
      return x!=s.x || y!=s.y;
    }
    ClassIsSimple(SourceCoord);
  };  


  static void FixInvalidNormals(ComRef<IDirect3DTexture8> &map)
  {
    HRESULT hr;

    // Get texture description
    D3DSURFACE_DESC desc;
    hr = map->GetLevelDesc(0, &desc);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Get surface
    ComRef<IDirect3DSurface8> mapSurface;
    hr = map->GetSurfaceLevel(0, mapSurface.Init());
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Lock the texture
    D3DLOCKED_RECT lr;
    hr = mapSurface->LockRect(&lr, NULL, 0);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Remember the source coordinate for each pixel
    Array2D<SourceCoord> source;
    source.Dim(desc.Width, desc.Height);
    for (int y = 0; y < (signed)desc.Height; y++)
    {
      D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
      for (int x = 0; x < (signed)desc.Width; x++)
      {
        if (row[x] == CLEAR_NORMAL)
        {
          // pixel invalid - set as far as possible, anything is better
          source(x,y) = SourceCoord(-(float)desc.Width,-(float)desc.Height);
        }
        else
        {

          source(x,y) = SourceCoord(x,y);
        }
      }
    }

    // Fill out the border of geometry with closest color
    const int maxIter = 32;
    //int maxIter = intMin(desc.Width,desc.Height);
    for(int iter = 1; iter < maxIter; iter++)
    {
      int invIter = maxIter - iter;
      BYTE avgNormal[4];
      avgNormal[0] = 0xFF * iter / maxIter;
      avgNormal[1] = 0x80 * iter / maxIter;
      avgNormal[2] = 0x80 * iter / maxIter;
      avgNormal[3] = 0x00;
      Array2D<SourceCoord> source0 = source;
      int progress = 0;
      int left = 0;
      for (unsigned int y = 0; y < desc.Height; y++)
      {
        D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
        // how many pixels are filled in this step
        for (unsigned int x = 0; x < desc.Width; x++)
        {
          if (row[x] != CLEAR_NORMAL)  continue;

          // check each neighbor, select a nearest one (valid only)
          int bestX = -1, bestY = -1;
          int bestDist = INT_MAX;
          for (int xx=-1; xx<=1; xx++) for (int yy=-1; yy<=1; yy++)
          {
            unsigned int xn = x+xx;
            unsigned int yn = y+yy;
            if (xn>=desc.Width) continue;
            if (yn>=desc.Height) continue;
            if (!source0(xn,yn).Valid()) continue;
            int dist = source0(xn,yn).Distance(SourceCoord(x,y));
            if (dist<=iter*iter && bestDist>dist)
            {
              bestX = xn;
              bestY = yn;
              bestDist = dist;
            }
          }
          left++;
          if (bestX>=0 && source0(bestX,bestY)!=source0(x,y))
          {
            progress++;
            const SourceCoord &coord = source0(bestX,bestY);
            source(x,y) = coord;
            D3DCOLOR *rowSource = (D3DCOLOR *)((BYTE*)lr.pBits + coord.y * lr.Pitch);
            BYTE *c = (BYTE*)&rowSource[coord.x];
            row[x] =
              ((c[0] * invIter / maxIter + avgNormal[0]) << 0) |
              ((c[1] * invIter / maxIter + avgNormal[1]) << 8) |
              ((c[2] * invIter / maxIter + avgNormal[2]) << 16);
            //row[x] = rowSource[coord.x];
          }
        }
      }
      if (left<=0) break;
    }

    // Fill unfilled pixels with average normal
    for (unsigned int y = 0; y < desc.Height; y++)
    {
      D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
      // how many pixels are filled in this step
      for (unsigned int x = 0; x < desc.Width; x++)
      {
        if (row[x] != CLEAR_NORMAL) 
        {
          Vector3 vx(((row[x] & 0xFF0000)>>16)/127.5f-1.0f,
            ((row[x] & 0xFF00)>>8)/127.5f-1.0f,
            (row[x] & 0xFF)/127.5f-1.0f);

          if (vx[2]<0) vx[2] = -vx[2];
          vx.Normalize();

          row[x]=(row[x] & 0xFF000000) |
            (toInt(vx[0]*127.5f+127.5)<<16) |
            (toInt(vx[1]*127.5f+127.5)<<8) |
            (toInt(vx[2]*127.5f+127.5));
        }
        else
        {        
          row[x] =  0x008080FF;
        }
      }
      
    }

    // Unlock the small texture
    hr = mapSurface->UnlockRect();
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));
  }

  void DisableAlpha(ComRef<IDirect3DTexture8> &map, int level) 
  {
    HRESULT hr;

    // Get source texture description
    D3DSURFACE_DESC desc;
    hr = map->GetLevelDesc(level, &desc);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    ComRef<IDirect3DSurface8> tmpMapSurface;
    hr = map->GetSurfaceLevel(0, tmpMapSurface.Init());
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    // Lock the texture
    D3DLOCKED_RECT lr;
    hr = tmpMapSurface->LockRect(&lr, NULL, 0);
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

    for (int y = 0; y < desc.Height; y++) {
      D3DCOLOR *row = (D3DCOLOR *)((BYTE*)lr.pBits + y * lr.Pitch);
      for (int x = 0; x < desc.Width; x++) {
        row[x] |= 0xFF000000;
      }
    }

    // Unlock the small texture
    hr = tmpMapSurface->UnlockRect();
    if (FAILED(hr)) LogF(DXGetErrorString8(hr));

  }

  static bool TestDeviceReset(PDirect3DDevice &dev)
  {
    while (dev.IsLost())
    {
      LogF("NOTE: Device is lost. restarting the device...");
      if (dev.Reset())
      {
        LogF("NOTE: Device restarted successfully");
        return true;
      }
      else
      {
        Fail("ERROR: Device lost and cannot be restarted. Generator can produce unusable results!");
        return false;      
      }
    }
    return false;
  }

  HRESULT BaseTextureFactory::PrepareRenderTarget(unsigned long &sizex, unsigned long &sizey)
  {
    HRESULT hr;
    D3DSURFACE_DESC desc,descZ;
    desc.Size=sizeof(desc);
    if (_renderTarget.NotNull()) _renderTarget->GetDesc(&desc);
    while (_renderTarget.IsNull() || desc.Width<sizex || desc.Height<sizey || desc.Format!=D3DFMT_A8R8G8B8 || desc.MultiSampleType!=D3DMULTISAMPLE_NONE)
    {
      _renderTarget << 0;
      hr = _pD3DDevice->CreateRenderTarget(sizex,sizey,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,TRUE,_renderTarget.Init());
      if (hr) 
      {
        LogF("Failed to create/expand render target in dimensions: %d - reducing (HRESULT %08X)",sizex,hr);
        sizex>>=1;
        sizey>>=1;
        if (sizex<=2 || sizey<=2) 
        {
          LogF("Warning: Unable to render polyplane - NO RENDER TARGET - skipping polyplane");
          return hr;
        }
      }
      else 
      {
        LogF("Render target created/expanded: %d",sizex);
        _renderTarget->GetDesc(&desc);
        break;        
      }
    }

    descZ.Size=sizeof(descZ);
    if (_depthBuffer.NotNull()) _depthBuffer->GetDesc(&descZ);
    if (_depthBuffer.IsNull() || desc.Width != descZ.Width || desc.Height != descZ.Height) {
        hr = _pD3DDevice->CreateDepthStencilSurface(sizex,sizey,D3DFMT_D32,D3DMULTISAMPLE_NONE, _depthBuffer.Init());
        if (hr != 0) hr = _pD3DDevice->CreateDepthStencilSurface(sizex,sizey,D3DFMT_D24X8,D3DMULTISAMPLE_NONE, _depthBuffer.Init());
        if (hr != 0) hr = _pD3DDevice->CreateDepthStencilSurface(sizex,sizey,D3DFMT_D16,D3DMULTISAMPLE_NONE, _depthBuffer.Init());
        if (hr != 0) {
            LogF("Unable to create depth buffer: 0x%08X. Render will continue without the depth buffer",hr);
            _depthBuffer << 0;
        } else
            LogF("Depth buffer activated...");

    }


    ///Select render target
    hr=_pD3DDevice->SetRenderTarget(_renderTarget,_depthBuffer);
    if (hr!=0) {LogF("Cannot activate render target");return hr;}  
    _pD3DDevice->SetRenderState( D3DRS_ZENABLE,        TRUE );

    D3DVIEWPORT8 viewport;
    viewport.X=0;
    viewport.Y=0;
    viewport.Width=sizex;
    viewport.Height=sizey;
    viewport.MinZ=0;
    viewport.MaxZ=1;
    ///render target can be bigger, than requested - adjust the viewport
    hr = _pD3DDevice->SetViewport(&viewport);
    assert(SUCCEEDED(hr));
    return hr;
  }

  HRESULT BaseTextureFactory::PrepareCamera(Matrix4Par Camera, float Width, float Height)
  {
    // Set projection matrix
    D3DXMATRIX matProj;
    D3DXMatrixOrthoRH(&matProj, Width, Height, -100.0f, 100.0f);

    // Get the inverse matrix to camera
    Matrix4 InvCamera;
    InvCamera.SetInvertGeneral(Camera);
    D3DXMATRIX matView;
    ConvertMatrix(matView, InvCamera);

    // Set the shader constants
    D3DXMATRIX matTemp;
    D3DXMatrixTranspose(&matTemp, &matView);
    _pD3DDevice->SetVertexShaderConstant(0, &matTemp, 4);
    D3DXMatrixTranspose(&matTemp, &matProj);
    _pD3DDevice->SetVertexShaderConstant(4, &matTemp, 4);
    return S_OK;
  }

  HRESULT BaseTextureFactory::RenderCoreColorMap(CPrimitiveStream &PSBranch,CPrimitiveStream &PSPolyplane, bool enlighted)
  {
    HRESULT hr;
    if (enlighted) {
      float C8[] = {0.0f, -1.0f, 0.0f, 0.0f}; // Light direction
      _pD3DDevice->SetVertexShaderConstant(8, C8, 1);
      float C9[] = {0.5f, 1.0f, 0.0f, 0.0f};  // Constant vector
      _pD3DDevice->SetVertexShaderConstant(9, C9, 1);
      float pC0[] = {0.4f, 0.4f, 0.4f, 0.0f}; // Ambient light color
      _pD3DDevice->SetPixelShaderConstant(0, pC0, 1);
    }
    else {
      float C8[] = {0.5f, 1.0f, 0.0f, 0.0f};  // Constant vector
      _pD3DDevice->SetVertexShaderConstant(8, C8, 1);
    }

/*    _pD3DDevice->LightEnable( 0, TRUE );
    _pD3DDevice->SetRenderState( D3DRS_LIGHTING, TRUE );*/
    _pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    _pD3DDevice->SetRenderState(D3DRS_ALPHAREF, 128);
    _pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);

    hr = _pD3DDevice->BeginScene();
    assert(SUCCEEDED(hr));

    hr = _pD3DDevice->Clear(0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, CLEAR_COLOR, 1.0f, 0L);
    assert(SUCCEEDED(hr));

    if (enlighted) {
      hr = _pD3DDevice->SetVertexShader(_hVSBranch);
      assert(SUCCEEDED(hr));
      hr = _pD3DDevice->SetPixelShader(_hPSBranch);
      assert(SUCCEEDED(hr));
    }
    else {
      hr = _pD3DDevice->SetVertexShader(_hVSBranchColor);
      assert(SUCCEEDED(hr));
      hr = _pD3DDevice->SetPixelShader(_hPSBranchColor);
      assert(SUCCEEDED(hr));
    }
    PSBranch.Draw();

    if (enlighted) {
      hr = _pD3DDevice->SetVertexShader(_hVSTriplane);
      assert(SUCCEEDED(hr));
      hr = _pD3DDevice->SetPixelShader(_hPSTriplane);
      assert(SUCCEEDED(hr));
    }
    else {
      hr = _pD3DDevice->SetVertexShader(_hVSPolyplaneColor);
      assert(SUCCEEDED(hr));
      hr = _pD3DDevice->SetPixelShader(_hPSPolyplaneColor);
      assert(SUCCEEDED(hr));
    }
    PSPolyplane.Draw();

    hr = _pD3DDevice->EndScene();
    assert(SUCCEEDED(hr));
    return hr;
  }

  HRESULT BaseTextureFactory::RenderCoreNormalMap(CPrimitiveStream &PSBranch,CPrimitiveStream &PSPolyplane)
  {

    float C8[] = {0.5f, 1.0f, 0.0f, 0.0f};  // Constant vector
    _pD3DDevice->SetVertexShaderConstant(8, C8, 1);

    HRESULT hr;
    hr = _pD3DDevice->BeginScene();
    assert(SUCCEEDED(hr));

    hr = _pD3DDevice->Clear(0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, CLEAR_NORMAL, 1.0f, 0L);
    assert(SUCCEEDED(hr));

    hr = _pD3DDevice->SetVertexShader(_hVSBranchNormal);
    assert(SUCCEEDED(hr));
    hr = _pD3DDevice->SetPixelShader(_hPSBranchNormal);
    assert(SUCCEEDED(hr));
    PSBranch.Draw(TRUE);

    hr = _pD3DDevice->SetVertexShader(_hVSPolyplaneNormal);
    assert(SUCCEEDED(hr));
    hr = _pD3DDevice->SetPixelShader(_hPSPolyplaneNormal);
    assert(SUCCEEDED(hr));
    PSPolyplane.Draw(TRUE);

    _pD3DDevice->EndScene();
    assert(SUCCEEDED(hr));
    return hr;
  }

  HRESULT LindaGenTexFactory::RenderPlane( CPrimitiveStream &PSBranch,
    CPrimitiveStream &PSPolyplane,
    ComRef<IDirect3DTexture8> &colorMap,
    ComRef<IDirect3DTexture8> &normalMap,
    Matrix4Par Camera,
    float Width,
    float Height,
    unsigned long sizex,
    unsigned long sizey,
    const DrawParameters &dp)
  {
    do
    {
      HRESULT hr;

      int textureSize=__min(sizex,sizey); //only rectangular textures are available
      if (textureSize<_minTexSize) textureSize=_minTexSize;
      if (textureSize>_maxTexSize) textureSize=_maxTexSize;
      unsigned int originalTextureSize = __min(textureSize << _antialiasingDegree, _maxTexSize);
      sizex=originalTextureSize;
      sizey=originalTextureSize;

      //save active render target
      ComRef<IDirect3DSurface8> activeRenderTarget;
      _pD3DDevice->GetRenderTarget(activeRenderTarget.Init());

      hr=PrepareRenderTarget(sizex,sizey);
      if (hr!=S_OK) return hr;
      originalTextureSize=sizex;

      hr=PrepareCamera(Camera,Width,Height);
      if (hr!=S_OK) return hr;

      // Create original map
      ComRef<IDirect3DTexture8> bigMap;
      if (FAILED(hr = CreatePolyplaneTexture(_pD3DDevice,originalTextureSize, bigMap.Init())))  return hr;

      // helper surface
      ComRef<IDirect3DSurface8> pHelpSurface;

      ///viewport for CopyRect
      RECT rccopy={0,0,originalTextureSize,originalTextureSize};
      POINT copypt={0,0};

      hr = bigMap->GetSurfaceLevel(0, pHelpSurface.Init());
      assert(SUCCEEDED(hr));

      /************************************************************************/
      /* Rendering of color map                                               */
      /************************************************************************/
      if (_renderTarget.IsNull() || bigMap.IsNull()) 
      {
        LogF("Color map is uninitialized.");
      }
      else 
      {
        RenderCoreColorMap(PSBranch,PSPolyplane,dp._enlighted);

        hr =_pD3DDevice->CopyRects(_renderTarget,&rccopy,1,pHelpSurface,&copypt);
        assert(SUCCEEDED(hr));
        PreviewPolyplane(pHelpSurface);
        /************************************************************************/
        /* Post processing for colomap                                          */
        /************************************************************************/
        FillBackgroundWithAvgColor(bigMap, 0);

        hr = _pD3DDevice->CreateTexture(textureSize, textureSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, colorMap.Init());
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) 
        {
          LogF("Failed to create a texture.");
        }
        else
        {        
          // Downsize data from the big texture
          Downsize(bigMap, colorMap);
          CreateMipLevels(colorMap);

          // Discretize alpha in all levels
          if (_discretizeAlpha) DiscretizeAlpha(colorMap);
        }
      }

      /************************************************************************/
      /* Rendering of normal map                                              */
      /************************************************************************/
      if (bigMap.IsNull() || _renderTarget.IsNull()) 
      {
        LogF("Normal map is uninitialized.");
      }
      else 
      {
        RenderCoreNormalMap(PSBranch,PSPolyplane);

        hr =_pD3DDevice->CopyRects(_renderTarget,&rccopy,1,pHelpSurface,&copypt);
        assert(SUCCEEDED(hr));

        PreviewPolyplane(pHelpSurface);
        /************************************************************************/
        /* Post processing for normalmap                                        */
        /************************************************************************/
//        DisableAlpha(bigMap, 0);

        hr = _pD3DDevice->CreateTexture(textureSize, textureSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, normalMap.Init());
        assert(SUCCEEDED(hr));

        if (FAILED(hr)) 
        {
          LogF("Failed to create a texture.");
        }
        else
        {        
        // Downsize data from the big texture
        Downsize(bigMap, normalMap);
        FixInvalidNormals(normalMap);

        hr = normalMap->GetSurfaceLevel(0, pHelpSurface.Init());
        PreviewPolyplane(pHelpSurface);
        // Create the rest of the mip levels
        CreateMipLevels(normalMap);
        if (_discretizeAlpha) DiscretizeAlpha(normalMap);
        }
      }
      hr=_pD3DDevice->SetRenderTarget(activeRenderTarget,0);
      if (_pD3DDevice.IsLost()) {
        _renderTarget << 0;
        _depthBuffer << 0;
      }


    }
    while (TestDeviceReset(_pD3DDevice));
    return 0;
  }


  void BaseTextureFactory::PrepareBranchPreview()
  {
    _pD3DDevice->SetVertexShader(_hVSBranch);
    _pD3DDevice->SetPixelShader(_hPSBranch);
  }
  void BaseTextureFactory::PrepareTriplanePreview()
  {
    _pD3DDevice->SetVertexShader(_hVSTriplane);
    _pD3DDevice->SetPixelShader(_hPSTriplane);
  }

  HRESULT BaseTextureFactory::LoadTexure( const char *filename, ComRef<IDirect3DTexture8> &map,bool linearAlpha)
  {
    HRESULT hr =D3DXCreateTextureFromFileEx(_pD3DDevice, filename,
      D3DX_DEFAULT, D3DX_DEFAULT, 0, 0,
      D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, D3DX_FILTER_BOX | D3DX_FILTER_MIRROR,
      0, NULL, NULL, map.Init());

//    D3DLOCKED_RECT rc;

    return hr;

  }

  HRESULT LindaGenTexFactory::GetLockableLevel(IDirect3DTexture8 *src, ComRef<IDirect3DSurface8> &lockable, int level)
  {
    return src->GetSurfaceLevel(level,lockable.Init());
  }


  HRESULT LindaEditTexFactory::RenderPlane(CPrimitiveStream &PSBranch, CPrimitiveStream &PSPolyplane, ComRef<IDirect3DTexture8> &colorMap, ComRef<IDirect3DTexture8> &normalMap, Matrix4Par Camera, float Width, float Height, unsigned long sizex, unsigned long sizey, const DrawParameters &dp)
  {
    HRESULT hr;

    // Save old render target
    ComRef<IDirect3DSurface8> pOldRenderTarget;
    _pD3DDevice->GetRenderTarget(pOldRenderTarget.Init());
    ComRef<IDirect3DSurface8> pOldDepthStencil;
    _pD3DDevice->GetDepthStencilSurface(pOldDepthStencil.Init());


    int textureSize=__min(sizex,sizey); //only rectangular textures are available

    PrepareCamera(Camera,Width,Height);    

    ComRef<IDirect3DSurface8> pHelpSurface;

//    RECT rccopy={0,0,textureSize,textureSize};
//    POINT copypt={0,0};


    _pD3DDevice->CreateTexture(textureSize,textureSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,colorMap.Init());
    if (dp._normalMap)
        _pD3DDevice->CreateTexture(textureSize,textureSize,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,normalMap.Init());
    else
        normalMap << NULL;


    // Rendering of the color map
    if (colorMap.IsNull()) 
    {
      LogF("Color map is uninitialized.");
    }
    else 
    {
      hr = colorMap->GetSurfaceLevel(0, pHelpSurface.Init());
      assert(SUCCEEDED(hr));

      _pD3DDevice->SetRenderTarget(pHelpSurface,NULL);
      RenderCoreColorMap(PSBranch,PSPolyplane,dp._enlighted);
    }

    if (dp._normalMap) {
        // Rendering of the normal map
        if (normalMap.IsNull()) 
        {
          LogF("Normal map is uninitialized.");
        }
        else {
          hr = normalMap->GetSurfaceLevel(0, pHelpSurface.Init());
          assert(SUCCEEDED(hr));
          
          _pD3DDevice->SetRenderTarget(pHelpSurface,NULL);
          RenderCoreNormalMap(PSBranch,PSPolyplane);
        }
    }

    // Set old render target
    hr = _pD3DDevice->SetRenderTarget(pOldRenderTarget, pOldDepthStencil);
    assert(SUCCEEDED(hr));
    return 0;
  }

  HRESULT LindaEditTexFactory::GetLockableLevel(IDirect3DTexture8 *src, ComRef<IDirect3DSurface8> &lockable, int level)
  {
    return src->GetSurfaceLevel(level,lockable.Init());
  }


  HRESULT LindaGenTexFactory::LoadTexure( const char *filename, ComRef<IDirect3DTexture8> &map,bool linearAlpha)
  {
    HRESULT hr =D3DXCreateTextureFromFileEx(_pD3DDevice, filename,
      D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0,
      D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, D3DX_FILTER_BOX | D3DX_FILTER_MIRROR,
      0, NULL, NULL, map.Init());

//    D3DLOCKED_RECT rc;

    return hr;

  }
}
