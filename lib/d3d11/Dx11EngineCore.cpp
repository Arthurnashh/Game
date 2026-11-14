#include "../wpch.hpp"
#include "Dx11EngineCore.hpp"
#include "engdd11.hpp"
#include "Dx11TextBank.hpp"
#include "../SimulWeather/SimulWeatherDX11.hpp"
#include "../clip2D.hpp"
#include "../Shape/shape.hpp"
#include "../Shape/material.hpp"
#include "../camera.hpp"
#include "../scene.hpp"
#include "../lights.hpp"
#include "../object.hpp"
#include "../textbank.hpp"
#include "../tlVertex.hpp"
#include "../engineDll.hpp"
#include "../rtAnimation.hpp"
#include <vector>

// Adapter index selected by the imported full D3D11 renderer.
int D3D11Adapter = -1;

namespace
{
const char *EngineCoreVertexShader =
  "cbuffer EngineDraw : register(b0) { row_major float4x4 worldViewProjection; "
  "uint hasTexture; uint3 padding; };\n"
  "struct VSIn { float3 position : POSITION; float3 normal : NORMAL; "
  "float2 uv : TEXCOORD0; };\n"
  "struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };\n"
  "VSOut main(VSIn input) { VSOut o; o.position = mul(worldViewProjection, "
  "float4(input.position, 1)); o.uv = input.uv; return o; }\n";

const char *EngineCorePixelShader =
  "cbuffer EngineDraw : register(b0) { row_major float4x4 worldViewProjection; "
  "uint hasTexture; uint3 padding; };\n"
  "Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);\n"
  "struct PSIn { float4 position : SV_Position; float2 uv : TEXCOORD0; };\n"
  "float4 main(PSIn input) : SV_Target { return hasTexture != 0 ? "
  "tex0.Sample(samp0, input.uv) : float4(1,1,1,1); }\n";
}

Dx11EngineCore::Dx11EngineCore()
  : _textBank(new Dx11TextBank), _modelToWorld(MIdentity),
    _modelToView(MIdentity), _worldToModel(MIdentity),
    _skinningType(STNone), _lights(NULL),
    _activeShape(NULL), _tlMesh(NULL),
    _activeClip(ClipAll), _activeSpec(0), _activeBias(0),
    _preparedMip(), _preparedMaterial(), _preparedTLMaterial(),
    _preparedSpecFlags(0), _preparedAlphaTestReference(0.0f),
    _preparedFogMode(FM_Fog), _renderFogColor(HWhite),
    _trianglePrepared(false), _window(NULL),
    _width(0), _height(0), _windowed(true), _deviceReady(false),
    _polyPrepared(false), _polyTextured(false), _polyTexture(NULL),
    _textureCacheClock(0)
{
}

Dx11EngineCore::~Dx11EngineCore()
{
  delete _tlMesh;
  _tlMesh = NULL;
  ClearTextureCache();
  delete _textBank;
  _textBank = NULL;
  _renderer.Reset();
  _deviceReady = false;
}

HRESULT Dx11EngineCore::InitializeD3D11(HWND window, UINT width, UINT height,
  bool windowed)
{
  if (!window || !width || !height) return E_INVALIDARG;
  ClearTextureCache();
  _renderer.Reset();
  _window = window;
  _width = width;
  _height = height;
  _windowed = windowed;
  HRESULT hr = _renderer.Initialize(window, width, height,
    EngineCoreVertexShader, "main", EngineCorePixelShader, "main", windowed);
  _deviceReady = SUCCEEDED(hr);
  return hr;
}

HRESULT Dx11EngineCore::BeginD3D11Frame()
{
  if (!_deviceReady) return E_FAIL;
  return _renderer.BeginFrame(0.0f, 0.0f, 0.0f, 1.0f);
}

HRESULT Dx11EngineCore::PresentD3D11(UINT syncInterval)
{
  if (!_deviceReady) return E_FAIL;
  return _renderer.EndFrame(syncInterval);
}

void Dx11EngineCore::Clear(bool clearZ, bool clear, PackedColor color,
  bool clearStencil)
{
  if (!_deviceReady) return;
  const float scale = 1.0f / 255.0f;
  _renderer.Clear(color.R8() * scale, color.G8() * scale,
    color.B8() * scale, color.A8() * scale, clear, clearZ, clearStencil);
}

void Dx11EngineCore::DrawPolyPrepare(const MipInfo &mip, int)
{
  _polyPrepared = false;
  _polyTextured = false;
  if (!_deviceReady) return;
  if (!mip._texture)
  {
    static volatile LONG nullTextureDiagnostics = 0;
    LONG diagnostic = InterlockedIncrement(&nullTextureDiagnostics);
    if (diagnostic <= 24)
      RptF("DX11 2D prepare: null texture, call=%d", (int)diagnostic);
    _polyPrepared = true;
    return;
  }
  HRESULT hr = PrepareTexture(mip._texture, &_polyTexture);
  static volatile LONG textureDiagnostics = 0;
  LONG diagnostic = InterlockedIncrement(&textureDiagnostics);
  if (FAILED(hr))
  {
    RptF("DX11 2D texture upload failed: call=%d name=%s hr=0x%08x",
      (int)diagnostic, (const char *)mip._texture->GetName(), (unsigned)hr);
    return;
  }
  if (diagnostic <= 32)
    RptF("DX11 2D texture ready: call=%d name=%s mip=%d gpu=%p",
      (int)diagnostic, (const char *)mip._texture->GetName(),
      mip._level, _polyTexture);
  _polyPrepared = true;
  _polyTextured = true;
}

HRESULT Dx11EngineCore::PrepareTexture(const Texture *source,
  Dx11Texture **texture)
{
  if (!source || !texture || !_renderer.Device()) return E_INVALIDARG;
  const RStringB &name = source->GetName();
  for (int i = 0; name.GetLength() > 0 && i < TextureCacheSize; ++i)
  {
    TextureCacheEntry &entry = _textureCache[i];
    if (entry.texture && entry.name == name)
    {
      entry.lastUsed = ++_textureCacheClock;
      *texture = entry.texture;
      return S_OK;
    }
  }

  Dx11Texture *loaded = new Dx11Texture;
  HRESULT hr = _textBank->CreateGpuTexture(_renderer.Device(),
    const_cast<Texture *>(source), loaded);
  if (FAILED(hr))
  {
    delete loaded;
    return hr;
  }

  int victim = 0;
  for (int i = 0; i < TextureCacheSize; ++i)
  {
    if (!_textureCache[i].texture) { victim = i; break; }
    if (_textureCache[i].lastUsed < _textureCache[victim].lastUsed) victim = i;
  }
  TextureCacheEntry &entry = _textureCache[victim];
  delete entry.texture;
  entry.name = name;
  entry.texture = loaded;
  entry.lastUsed = ++_textureCacheClock;
  *texture = loaded;
  return S_OK;
}

void Dx11EngineCore::ClearTextureCache()
{
  for (int i = 0; i < TextureCacheSize; ++i)
  {
    delete _textureCache[i].texture;
    _textureCache[i].texture = NULL;
    _textureCache[i].name = RStringB();
    _textureCache[i].lastUsed = 0;
  }
  _polyTexture = NULL;
  _textureCacheClock = 0;
}

void Dx11EngineCore::DrawLinePrepare()
{
  // This temporary path draws solid-color line quads. The textured line
  // coverage resource will be supplied by the DX11 text bank.
  _polyPrepared = _deviceReady;
  _polyTextured = false;
  _polyTexture = NULL;
}

HRESULT Dx11EngineCore::DrawClippedPolygon(const Vertex2DAbs *vertices,
  int nVertices, const Rect2DAbs &clip, float xOffset, float yOffset)
{
  _renderer.SetLinearColorOutput(false);
  if (!_polyPrepared || !_deviceReady || !vertices || nVertices < 3 ||
      nVertices > 32 || Width2D() <= 0 || Height2D() <= 0)
    return E_INVALIDARG;

  ClipFlags orClip = 0;
  ClipFlags andClip = ClipAll;
  for (int i = 0; i < nVertices; ++i)
  {
    ClipFlags flags = 0;
    if (vertices[i].x < clip.x) flags |= ClipLeft;
    else if (vertices[i].x > clip.x + clip.w) flags |= ClipRight;
    if (vertices[i].y < clip.y) flags |= ClipTop;
    else if (vertices[i].y > clip.y + clip.h) flags |= ClipBottom;
    orClip |= flags;
    andClip &= flags;
  }
  if (andClip) return S_FALSE;

  Vertex2DAbs buffers[2][40];
  const Vertex2DAbs *used = vertices;
  int count = nVertices;
  if (orClip)
  {
    Vertex2DAbs *free = buffers[0];
    Vertex2DAbs *next = buffers[1];
    if (orClip & ClipTop)
    {
      count = Clip2D(clip, free, used, count, InsideTopAbs, InterpolateVertexAbs);
      used = free;
      Vertex2DAbs *swap = free; free = next; next = swap;
      if (count < 3) return S_FALSE;
    }
    if (orClip & ClipBottom)
    {
      count = Clip2D(clip, free, used, count, InsideBottomAbs, InterpolateVertexAbs);
      used = free;
      Vertex2DAbs *swap = free; free = next; next = swap;
      if (count < 3) return S_FALSE;
    }
    if (orClip & ClipLeft)
    {
      count = Clip2D(clip, free, used, count, InsideLeftAbs, InterpolateVertexAbs);
      used = free;
      Vertex2DAbs *swap = free; free = next; next = swap;
      if (count < 3) return S_FALSE;
    }
    if (orClip & ClipRight)
    {
      count = Clip2D(clip, free, used, count, InsideRightAbs, InterpolateVertexAbs);
      used = free;
      if (count < 3) return S_FALSE;
    }
  }

  TLVertex converted[40];
  for (int i = 0; i < count; ++i)
  {
    converted[i].pos[0] = used[i].x + xOffset;
    converted[i].pos[1] = used[i].y + yOffset;
    converted[i].pos[2] = used[i].z;
    converted[i].rhw = used[i].w;
    converted[i].color = used[i].color;
    converted[i].specular = PackedColor(0xff000000);
    converted[i].t0.u = used[i].u;
    converted[i].t0.v = used[i].v;
  }
  HRESULT hr = _renderer.DrawTLVertices((UINT)Width2D(), (UINT)Height2D(), converted,
    (UINT)count, _polyTextured ? _polyTexture : NULL);
  static volatile LONG polygonDiagnostics = 0;
  LONG diagnostic = InterlockedIncrement(&polygonDiagnostics);
  if (diagnostic <= 32 || FAILED(hr))
    RptF("DX11 2D polygon: call=%d verts=%d textured=%d hr=0x%08x",
      (int)diagnostic, count, _polyTextured ? 1 : 0, (unsigned)hr);
  return hr;
}

void Dx11EngineCore::BeginInstanceTL(int, const PositionRender &modelToWorld,
  float, int spec, const LightList &lights, const DrawParameters &)
{
  _lights = &lights;
  _activeSpec = spec;
  if (!GScene || !GScene->GetCamera()) return;
  Camera *camera = GScene->GetCamera();
  if (modelToWorld.camSpace)
  {
    // Camera-relative objects already carry a model-to-view transform.
    // Preserve that coordinate space instead of applying the camera inverse
    // to it again in BeginMeshTL.
    _modelToView = modelToWorld.position;
    _modelToWorld = camera->Transform() * modelToWorld.position;
  }
  else
  {
    _modelToWorld = modelToWorld.position;

    // Match the legacy D3D9/D3D10 path: subtract the camera origin before
    // multiplying matrices. A direct view * world product loses precision
    // for DayZ's large world coordinates and can distort projected meshes.
    Matrix4 modelToWorldOffset;
    modelToWorldOffset.SetOrientation(modelToWorld.position.Orientation());
    modelToWorldOffset.SetPosition(modelToWorld.position.Position() -
      camera->Transform().Position());
    Matrix4 viewZero;
    viewZero.SetOrientation(camera->InverseScaled().Orientation());
    viewZero.SetPosition(VZero);
    _modelToView = viewZero * modelToWorldOffset;
  }
  _worldToModel = _modelToWorld.InverseGeneral();
}

bool Dx11EngineCore::BeginMeshTL(int,
  const VertexTableAnimationContext *animContext, const Shape &shape,
  const EngineShapeProperties &, int spec, const LightList &lights, int)
{
  static volatile LONG beginMeshDiagnostics = 0;
  const LONG beginMeshDiagnostic = InterlockedIncrement(&beginMeshDiagnostics);
  if (!_deviceReady || !GScene || !GScene->GetCamera() ||
      !shape.IsLoadLocked() || shape.NVertex() <= 0 || shape.NSections() <= 0)
  {
    if (beginMeshDiagnostic <= 32)
      RptF("DX11 3D BeginMesh rejected: call=%d device=%d scene=%d locked=%d vertices=%d sections=%d",
        (int)beginMeshDiagnostic, _deviceReady ? 1 : 0,
        GScene && GScene->GetCamera() ? 1 : 0, shape.IsLoadLocked() ? 1 : 0,
        shape.NVertex(), shape.NSections());
    return false;
  }
  if (beginMeshDiagnostic <= 32)
    RptF("DX11 3D BeginMesh: call=%d vertices=%d sections=%d animated=%d skin=%d bones=%d",
      (int)beginMeshDiagnostic, shape.NVertex(), shape.NSections(),
      animContext && animContext->AreVerticesLoaded() ? 1 : 0,
      (int)_skinningType, (int)_skinningMatrices.Size());
  _meshSequenceLock.Lock();
  if (!_lights) _lights = &lights;

  delete _tlMesh;
  _tlMesh = new TLVertexTable;
  Matrix4 pointView = _modelToView;
  _tlMesh->Init(animContext, shape, pointView);

  std::vector<Vector3> skinnedPositions;
  std::vector<Vector3> skinnedNormals;
  const Vector3 *lightingPositions = NULL;
  const Vector3 *lightingNormals = NULL;
  const bool hasAnimatedVertices = animContext && animContext->AreVerticesLoaded();
  if (hasAnimatedVertices)
  {
    const Array<Vector3> &animatedPositions = animContext->GetPosArray(&shape);
    if (animatedPositions.Size() == shape.NVertex())
      lightingPositions = animatedPositions.Data();

    const Array<Vector3Compressed> &animatedNormals = animContext->GetNormArray();
    if (animatedNormals.Size() == shape.NVertex())
    {
      skinnedNormals.resize(shape.NVertex());
      for (int vertexIndex = 0; vertexIndex < shape.NVertex(); ++vertexIndex)
        skinnedNormals[vertexIndex] = animatedNormals[vertexIndex].Get();
      lightingNormals = &skinnedNormals[0];
    }
  }
  else if (_skinningType != STNone && _skinningMatrices.Size() > 0 &&
      shape.GetVertexBoneRef().Size() == shape.NVertex())
  {
    const AutoArray<Vector3> &sourcePositions = shape.GetPosArray();
    const AutoArray<AnimationRTWeight> &vertexBones = shape.GetVertexBoneRef();
    skinnedPositions.resize(shape.NVertex());
    skinnedNormals.resize(shape.NVertex());
    for (int vertexIndex = 0; vertexIndex < shape.NVertex(); ++vertexIndex)
    {
      const AnimationRTWeight &weights = vertexBones[vertexIndex];
      Vector3 skinnedPosition = VZero;
      Vector3 skinnedNormal = VZero;
      float accumulatedWeight = 0.0f;
      bool validWeights = true;
      for (int weightIndex = 0; weightIndex < weights.Size(); ++weightIndex)
      {
        const AnimationRTPair &weight = weights[weightIndex];
        const int boneIndex = GetSubSkeletonIndex(weight.GetSel());
        if (boneIndex < 0 || boneIndex >= _skinningMatrices.Size())
        {
          validWeights = false;
          break;
        }
        const float weightValue = weight.GetWeight();
        Vector3 transformedPosition;
        transformedPosition.SetMultiply(_skinningMatrices[boneIndex],
          sourcePositions[vertexIndex]);
        skinnedPosition += transformedPosition * weightValue;
        Vector3 transformedNormal;
        transformedNormal.SetMultiply(_skinningMatrices[boneIndex].Orientation(),
          shape.Norm(vertexIndex).Get());
        skinnedNormal += transformedNormal * weightValue;
        accumulatedWeight += weightValue;
      }
      skinnedPositions[vertexIndex] = sourcePositions[vertexIndex];
      skinnedNormals[vertexIndex] = shape.Norm(vertexIndex).Get();
      if (validWeights && accumulatedWeight > 1.0e-6f)
      {
        if (fabsf(accumulatedWeight - 1.0f) > 1.0e-5f)
        {
          skinnedPosition *= 1.0f / accumulatedWeight;
          skinnedNormal *= 1.0f / accumulatedWeight;
        }
        if (skinnedNormal.SquareSize() > 1.0e-12f)
          skinnedNormal.Normalize();
        skinnedPositions[vertexIndex] = skinnedPosition;
        skinnedNormals[vertexIndex] = skinnedNormal;
        Vector3 viewPosition;
        viewPosition.SetMultiply(pointView, skinnedPosition);
        _tlMesh->SetTransPosA(vertexIndex, viewPosition);
      }
    }
    lightingPositions = &skinnedPositions[0];
    lightingNormals = &skinnedNormals[0];
  }
  ClipFlags andClip = 0;
  _activeClip = _tlMesh->CheckClipping(*GScene->GetCamera(), ClipAll, andClip);
  if (andClip)
  {
    delete _tlMesh;
    _tlMesh = NULL;
    _meshSequenceLock.Unlock();
    return false;
  }

  _activeShape = &shape;
  _activeSpec = spec;
  _trianglePrepared = false;
  _tlMesh->DoLighting(HWhite, _worldToModel, *_lights, shape, spec,
    lightingPositions, lightingNormals);
  return true;
}

void Dx11EngineCore::SetSkinningType(int, ESkinningType skinningType,
  const Matrix4 *bones, int nBones)
{
  ScopeLockSection lock(_meshSequenceLock);
  _skinningType = skinningType;
  if (!bones || nBones <= 0 || skinningType == STNone)
  {
    _skinningMatrices.Resize(0);
    return;
  }
  _skinningMatrices.Resize(nBones);
  for (int i = 0; i < nBones; ++i)
    _skinningMatrices[i] = bones[i];
}

void Dx11EngineCore::PrepareTriangleTL(int, const PolyProperties *,
  const MipInfo &mip, const TexMaterialLODInfo &mat, int specFlags,
  const TLMaterial &tlMat, int, const EngineShapeProperties &)
{
  _preparedMip = mip;
  _preparedMaterial = mat;
  _preparedTLMaterial = tlMat;
  _preparedSpecFlags = specFlags;
  // IsTransparent is the engine's alpha-tested (cutout) path. The legacy
  // renderer defaults this to an alpha reference of 128 unless the material
  // overrides it; leaving the reference at zero turns leaf and fence cutouts
  // into opaque textured rectangles in DX11.
  _preparedAlphaTestReference = (specFlags & IsTransparent)
    ? 128.0f / 255.0f : 0.0f;
  _preparedFogMode = (specFlags & IsAlphaFog) ? FM_Alpha : FM_Fog;
  if (mat._mat)
  {
    TexMaterial::Loaded loaded(mat._mat);
    _preparedFogMode = loaded.GetFogMode();
    static volatile LONG materialShaderDiagnostics = 0;
    const LONG materialShaderDiagnostic = InterlockedIncrement(
      &materialShaderDiagnostics);
    if (materialShaderDiagnostic <= 64)
      RptF("DX11 material shader: call=%d material=%s pixelShader=%d vertexShader=%d stages=%d texGens=%d fog=%d",
        (int)materialShaderDiagnostic,
        cc_cast(mat._mat->GetName()),
        (int)loaded.GetPixelShaderID(), (int)loaded.GetVertexShaderID(),
        mat._mat->_stageCount + 1, mat._mat->_nTexGen,
        (int)_preparedFogMode);
    if (loaded.SomeRenderFlags() && loaded.GetRenderFlag(RFNoZWrite))
      _preparedSpecFlags |= NoZWrite;
    if (loaded.SomeRenderFlags())
    {
      if (loaded.GetRenderFlag(RFAlphaTest64))
        _preparedAlphaTestReference = 64.0f / 255.0f;
      else if (loaded.GetRenderFlag(RFAlphaTest32))
        _preparedAlphaTestReference = 32.0f / 255.0f;
      else if (loaded.GetRenderFlag(RFAlphaTest128))
        _preparedAlphaTestReference = 128.0f / 255.0f;
    }
  }
  _trianglePrepared = true;
}

void Dx11EngineCore::EndMeshTL(int, const Shape &)
{
  delete _tlMesh;
  _tlMesh = NULL;
  _activeShape = NULL;
  _lights = NULL;
  _trianglePrepared = false;
  _meshSequenceLock.Unlock();
}

void Dx11EngineCore::DrawSectionTL(int, const Shape &shape, int beg, int end,
  int bias, int instancesOffset, int instancesCount, const DrawParameters &)
{
  ScopeLockSection meshLock(_meshSequenceLock);
  static volatile LONG sectionDiagnostics = 0;
  const LONG sectionDiagnostic = InterlockedIncrement(&sectionDiagnostics);
  if (!_deviceReady || !_tlMesh || _activeShape != &shape ||
      instancesCount > 0 || instancesOffset > 0)
  {
    if (sectionDiagnostic <= 64)
      RptF("DX11 3D section rejected: call=%d device=%d mesh=%d shapeMatch=%d instances=%d offset=%d",
        (int)sectionDiagnostic, _deviceReady ? 1 : 0, _tlMesh ? 1 : 0,
        _activeShape == &shape ? 1 : 0, instancesCount, instancesOffset);
    return;
  }
  beg = intMax(beg, 0);
  end = intMin(end, shape.NSections());
  if (beg >= end) return;
  HRESULT colorSpaceHr = _renderer.SetLinearColorOutput(true);
  if (FAILED(colorSpaceHr))
    RptF("DX11 scene output switch failed: hr=0x%08x", (unsigned)colorSpaceHr);
  SetBias(0, bias);
  Camera *camera = GScene->GetCamera();
  const Matrix4 &projection = camera->Projection();
  _tlMesh->DoPerspective(*camera, _activeClip);

  for (int sectionIndex = beg; sectionIndex < end; ++sectionIndex)
  {
    const ShapeSection &section = shape.GetSection(sectionIndex);
    TexMaterial *material = section.GetMaterialExt();
    TexMaterial *preparedMaterial = _trianglePrepared
      ? const_cast<TexMaterial *>(_preparedMaterial._mat) : material;
    Texture *sourceTexture = preparedMaterial
      ? preparedMaterial->_stage[0].GetTextureForDraw()
      : (_trianglePrepared ? _preparedMip._texture : section.GetTexture());
    if (!sourceTexture && _trianglePrepared)
      sourceTexture = _preparedMip._texture;
    if (sectionDiagnostic <= 64)
      RptF("DX11 3D section: call=%d section=%d/%d vertices=%d faces=%d..%d prepared=%d material=%s texture=%s spec=0x%x alphaTest=%.3f blend=%d fog=%d",
        (int)sectionDiagnostic, sectionIndex, shape.NSections(),
        shape.NVertex(), (int)section.beg, (int)section.end,
        _trianglePrepared ? 1 : 0,
        preparedMaterial ? cc_cast(preparedMaterial->GetName()) : "<none>",
        sourceTexture ? (const char *)sourceTexture->GetName() : "<none>",
        _preparedSpecFlags, _preparedAlphaTestReference,
        (_preparedSpecFlags & (IsAlpha | IsAlphaFog)) != 0 ? 1 : 0,
        (int)_preparedFogMode);
    Dx11Texture *gpuTexture = NULL;
    if (sourceTexture)
    {
      MipInfo mip = _trianglePrepared && sourceTexture == _preparedMip._texture
        ? _preparedMip : _textBank->UseMipmap(sourceTexture, 0, 0);
      HRESULT textureHr = mip._texture
        ? PrepareTexture(mip._texture, &gpuTexture) : E_FAIL;
      if (!mip._texture || FAILED(textureHr))
      {
        static volatile LONG textureSectionFailures = 0;
        LONG diagnostic = InterlockedIncrement(&textureSectionFailures);
        if (diagnostic <= 64)
          RptF("DX11 section skipped: section=%d texture=%s mip=%d hr=0x%08x",
            sectionIndex,
            (const char *)sourceTexture->GetName(), mip._level,
            (unsigned)textureHr);
        continue;
      }
    }

    const bool useMaterialShader = _trianglePrepared && preparedMaterial;
    const bool isTerrainVertexShader = preparedMaterial &&
      (preparedMaterial->GetVertexShaderID() == VSTerrain ||
       preparedMaterial->GetVertexShaderID() == VSTerrainGrass);
    const bool isTerrainGrass = preparedMaterial &&
      preparedMaterial->GetVertexShaderID() == VSTerrainGrass;
    float terrainLODPars[4] = {};
    Vector3 terrainCameraModel = VZero;
    if (isTerrainVertexShader)
    {
      if (_sceneProps) _sceneProps->GetTerrainLODPars(terrainLODPars);
      const Vector3 cameraPosition = _sceneProps
        ? _sceneProps->GetCameraPosition() : camera->Position();
      terrainCameraModel.SetMultiply(_worldToModel, cameraPosition);
    }
    UINT materialMode = 0;
    UINT materialStageCount = 1;
    UINT materialTextureMask = 0;
    const Dx11Texture *materialTextures[TexMaterial::NStages] = {};
    bool materialTextureSRGB[TexMaterial::NStages] = {};
    UINT materialStageTexGen[TexMaterial::NStages] = {};
    if (useMaterialShader)
    {
      const PixelShaderID pixelShader = preparedMaterial->GetPixelShaderID();
      if (pixelShader == PSTreeAdv || pixelShader == PSTreeAdvSimple)
        materialMode = 1;
      else if (pixelShader == PSTreeAdvAToC ||
               pixelShader == PSTreeAdvSimpleAToC)
        materialMode = 4;
      else if (pixelShader == PSTreeAdvTrunk ||
               pixelShader == PSTreeAdvTrunkSimple)
        materialMode = 5;
      else if (pixelShader == PSSuper || pixelShader == PSSuperExt ||
               pixelShader == PSSuperAToC)
        materialMode = 2;
      else if (pixelShader == PSMulti)
        materialMode = 3;
      else if (pixelShader == PSDetail)
        materialMode = 6;
      else if (pixelShader == PSDetailMacroAS)
        materialMode = 7;
      else if (pixelShader == PSNormalMapDetailSpecularMap ||
               pixelShader == PSNormalMapDetailSpecularDIMap)
        materialMode = 14;
      else if (pixelShader == PSNormalMapDetailMacroASSpecularMap ||
               pixelShader == PSNormalMapDetailMacroASSpecularDIMap)
        materialMode = 10;
      else if (pixelShader == PSNormalMapMacroAS ||
               pixelShader == PSNormalMapDiffuseMacroAS ||
               pixelShader == PSNormalMapMacroASSpecularMap ||
               pixelShader == PSNormalMapMacroASSpecularDIMap)
        materialMode = 11;
      else if (pixelShader == PSTree || pixelShader == PSTreeSimple ||
               pixelShader == PSTreePRT || pixelShader == PSTreeSN)
        materialMode = 8;
      else if (pixelShader == PSTreeAToC)
        materialMode = 12;
      else if (pixelShader == PSGrass || pixelShader == PSNormalMapGrass)
        materialMode = 9;
      else if (pixelShader == PSGrassAToC)
        materialMode = 13;

      materialStageCount = intMax(1, intMin(TexMaterial::NStages,
        preparedMaterial->_stageCount + 1));
      for (UINT stage = 0; stage < materialStageCount; ++stage)
      {
        const int texGen = preparedMaterial->_stage[stage]._texGen;
        materialStageTexGen[stage] = texGen >= 0 &&
          texGen < TexMaterial::NTexGens ? (UINT)texGen : 0;
        Texture *stageSource = preparedMaterial->_stage[stage].GetTextureForDraw();
        // Some legacy material shaders have no texture bound directly to
        // stage 0: the engine supplies their diffuse map through the prepared
        // mip/section texture instead. Keep that base texture in the DX11
        // material table rather than silently rendering white for the base.
        if (stage == 0 && gpuTexture &&
            (!stageSource || stageSource == sourceTexture))
        {
          materialTextures[stage] = gpuTexture;
          materialTextureSRGB[stage] = _textBank->IsSRGB(
            stageSource ? stageSource : sourceTexture);
          materialTextureMask |= 1u << stage;
          continue;
        }
        if (!stageSource) continue;
        MipInfo stageMip = stage == 0 && _trianglePrepared &&
          stageSource == _preparedMip._texture ? _preparedMip :
          _textBank->UseMipmap(stageSource, 0, 0);
        Dx11Texture *stageTexture = NULL;
        if (stageMip._texture && SUCCEEDED(PrepareTexture(stageMip._texture,
            &stageTexture)))
        {
          materialTextures[stage] = stageTexture;
          materialTextureSRGB[stage] = _textBank->IsSRGB(stageSource);
          materialTextureMask |= 1u << stage;
        }
        else if (stage == 0)
        {
          if (sectionDiagnostic <= 64)
            RptF("DX11 material base texture unavailable: material=%s stage=%u texture=%s",
              cc_cast(preparedMaterial->GetName()), stage,
              (const char *)stageSource->GetName());
        }
        else if (sectionDiagnostic <= 64)
        {
          RptF("DX11 auxiliary material texture unavailable: material=%s stage=%u texture=%s",
            cc_cast(preparedMaterial->GetName()), stage,
            (const char *)stageSource->GetName());
        }
      }
      if (sectionDiagnostic <= 64)
        RptF("DX11 material submission: call=%d section=%d material=%s shader=%d mode=%u stages=%u readyMask=0x%04x texGen0=%u texGen1=%u texGen2=%u",
          (int)sectionDiagnostic, sectionIndex,
          cc_cast(preparedMaterial->GetName()),
          (int)preparedMaterial->GetPixelShaderID(), materialMode,
          materialStageCount, materialTextureMask,
          materialStageTexGen[0], materialStageTexGen[1],
          materialStageTexGen[2]);
      if (sectionDiagnostic <= 24)
      {
        const UINT diagnosticStageCount =
          materialStageCount < 3 ? materialStageCount : 3;
        for (UINT stage = 0; stage < diagnosticStageCount; ++stage)
        {
          Texture *stageSource =
            preparedMaterial->_stage[stage].GetTextureForDraw();
          RptF("DX11 material stage probe: call=%d section=%d stage=%u texture=%s srgb=%d texGen=%u uvSource=%d",
            (int)sectionDiagnostic, sectionIndex, stage,
            stageSource ? (const char *)stageSource->GetName() : "<none>",
            materialTextureSRGB[stage] ? 1 : 0,
            materialStageTexGen[stage],
            (int)preparedMaterial->_texGen[materialStageTexGen[stage]]._uvSource);
        }
      }
    }

    const float sectionFogColor[4] = {
      _renderFogColor.R(), _renderFogColor.G(), _renderFogColor.B(),
      _renderFogColor.A()
    };
    const bool useSectionFog = !_trianglePrepared ||
      _preparedFogMode == FM_Fog || _preparedFogMode == FM_FogAlpha;
    Texture *texture0Source = preparedMaterial
      ? preparedMaterial->_stage[0].GetTextureForDraw() : sourceTexture;
    if (!texture0Source) texture0Source = sourceTexture;
    const Color texture0Max = _textBank->GetMaxColor(texture0Source);
    const float texture0MaxColor[4] = {
      texture0Max.R(), texture0Max.G(), texture0Max.B(), texture0Max.A()
    };

    std::vector<TLVertex> triangles;
    std::vector<Dx11MaterialTLVertex> materialTriangles;
    triangles.reserve(6144);
    materialTriangles.reserve(6144);
    const auto flushTriangles = [&]()
    {
      if (useMaterialShader)
      {
        if (materialTriangles.empty()) return;
        // Keep a small CPU-side probe for the first submitted material
        // vertices. It separates lighting/color packing issues from shader
        // texture sampling issues when comparing the DX11 scene with D3D9.
        static volatile LONG materialVertexDiagnostics = 0;
        LONG materialVertexDiagnostic = InterlockedIncrement(
          &materialVertexDiagnostics);
        if (materialVertexDiagnostic <= 24)
        {
          const Dx11MaterialTLVertex &probe = materialTriangles[0];
          RptF("DX11 material vertex probe: call=%d section=%d mode=%u material=%s rgb=%u/%u/%u alpha=%u fog=%u uv0=%.4f,%.4f uv1=%.4f,%.4f uv2=%.4f,%.4f",
            (int)materialVertexDiagnostic, sectionIndex, materialMode,
            cc_cast(preparedMaterial->GetName()),
            (probe.color >> 16) & 0xff, (probe.color >> 8) & 0xff,
            probe.color & 0xff, (probe.color >> 24) & 0xff,
            (probe.specular >> 24) & 0xff,
            probe.uv[0][0], probe.uv[0][1], probe.uv[1][0],
            probe.uv[1][1], probe.uv[2][0], probe.uv[2][1]);
        }
        HRESULT drawHr = _renderer.DrawMaterialTLVertices((UINT)WidthBB(),
          (UINT)HeightBB(), &materialTriangles[0],
          (UINT)materialTriangles.size(), materialTextures,
          materialStageTexGen, materialStageCount, materialMode,
          (_preparedSpecFlags & NoZBuf) == 0,
          (_preparedSpecFlags & NoZWrite) == 0,
          _preparedAlphaTestReference, true, sectionFogColor,
          texture0MaxColor, materialTextureSRGB, useSectionFog,
          (_preparedSpecFlags & (IsAlpha | IsAlphaFog)) != 0);
        if (FAILED(drawHr))
          RptF("DX11 material draw failed: section=%d vertices=%d mode=%u stages=%u hr=0x%08x",
            sectionIndex, (int)materialTriangles.size(), materialMode,
            materialStageCount, (unsigned)drawHr);
        materialTriangles.clear();
        return;
      }
      if (triangles.empty()) return;
      HRESULT drawHr = _renderer.DrawTLVertices((UINT)WidthBB(),
        (UINT)HeightBB(), &triangles[0], (UINT)triangles.size(), gpuTexture,
        (_preparedSpecFlags & NoZBuf) == 0,
        (_preparedSpecFlags & NoZWrite) == 0,
        _preparedAlphaTestReference, true, sectionFogColor, useSectionFog,
        (_preparedSpecFlags & (IsAlpha | IsAlphaFog)) != 0,
        NULL, NULL, 0, _textBank->IsSRGB(sourceTexture));
      if (FAILED(drawHr))
        RptF("DX11 DrawSectionTL batch failed: section=%d vertices=%d hr=0x%08x",
          sectionIndex, (int)triangles.size(), (unsigned)drawHr);
      triangles.clear();
    };

    for (Offset faceOffset = section.beg; faceOffset < section.end;
         shape.NextFace(faceOffset))
    {
      const Poly &face = shape.Face(faceOffset);
      if (face.N() < 3 || face.N() > 32) continue;
      bool invalid = false;
      TLVertex vertices[32];
      Dx11MaterialTLVertex materialVertices[32];
      for (int i = 0; i < face.N(); ++i)
      {
        const int vertexIndex = face.GetVertex(i);
        if (vertexIndex < 0 || vertexIndex >= _tlMesh->NVertex())
        {
          invalid = true;
          break;
        }
        vertices[i] = _tlMesh->GetVertex(vertexIndex);
        // DoPerspective saturates off-screen vertices and replaces near/far
        // clipped vertices with a center-screen placeholder. Re-project the
        // original view-space position without saturation; the D3D11 clipper
        // can then trim crossing triangles instead of losing the whole face.
        Vector3 viewPosition = _tlMesh->TransPosA(vertexIndex);
        if (isTerrainVertexShader && shape.IsCustomPresent())
        {
          // VSTerrain's vertex shader morphs the sampled vertex height between
          // the seven terrain LODs before applying the view/projection
          // matrices. The generic TL path used to discard VertexCustomInfo,
          // leaving terrain patches at LOD0 heights and opening gaps between
          // neighboring patches. Reapply the same distance/LOD weights here
          // until terrain has its dedicated GPU vertex pipeline.
          const Vector3 &modelPosition = shape.Pos(vertexIndex);
          float lodHeights[7] = {};
          float grassHeight = 0.0f;
#if _VBS3_MORETERRAINLODS
          const VertexCustomInfo &custom = shape.Custom(vertexIndex);
          lodHeights[0] = modelPosition.Y();
          for (int lod = 1; lod < VertexCustomItems; ++lod)
            lodHeights[lod] = custom._item[lod];
          grassHeight = custom._item[0];
          const int terrainLODCount = VertexCustomItems;
#else
          const Vector3 &custom = shape.Custom(vertexIndex);
          lodHeights[0] = modelPosition.Y();
          lodHeights[1] = custom.X();
          lodHeights[2] = custom.Y();
          grassHeight = custom.Z();
          const int terrainLODCount = 3;
#endif
          const float dx = terrainCameraModel.X() - modelPosition.X();
          const float dz = terrainCameraModel.Z() - modelPosition.Z();
          const float distanceSquared = floatMax(dx * dx + dz * dz,
            1.0e-12f);
          const float maxLod = floatMax(0.0f, floatMin(
            terrainLODPars[1], (float)(terrainLODCount - 1)));
          float wantedLod = maxLod + terrainLODPars[0] +
            0.5f * (logf(distanceSquared) / logf(2.0f));
          wantedLod = floatMax(0.0f, floatMin(maxLod, wantedLod));

          float height = 0.0f;
          for (int lod = 0; lod < terrainLODCount; ++lod)
          {
            const float weight = floatMax(0.0f,
              1.0f - fabsf(wantedLod - (float)lod));
            if (weight <= 0.0f) continue;
            height += lodHeights[lod] * weight;
          }
          if (isTerrainGrass)
          {
            const float grassFade = floatMin(1.0f,
              floatMax(0.0f, (sqrtf(distanceSquared) - 15.0f) / 15.0f));
            height += grassHeight * grassFade;
          }

          Vector3 morphedModelPosition = modelPosition;
          morphedModelPosition[1] = height;
          viewPosition.SetMultiply(_modelToView, morphedModelPosition);

          static volatile LONG terrainMorphDiagnostics = 0;
          const LONG terrainMorphDiagnostic = InterlockedIncrement(
            &terrainMorphDiagnostics);
          if (terrainMorphDiagnostic <= 16)
            RptF("DX11 terrain LOD morph: call=%d grass=%d lod=%.3f maxLod=%.1f baseY=%.3f resultY=%.3f dist2=%.1f custom=%d",
              (int)terrainMorphDiagnostic, isTerrainGrass ? 1 : 0,
              wantedLod, maxLod, modelPosition.Y(), height,
              distanceSquared, shape.IsCustomPresent() ? 1 : 0);
        }
        const float viewZ = viewPosition.Z();
        if (fabsf(viewZ) < 1.0e-20f)
        {
          invalid = true;
          break;
        }
        const float inverseZ = 1.0f / viewZ;
        vertices[i].pos[0] = projection(0, 2) +
          projection(0, 0) * viewPosition.X() * inverseZ;
        vertices[i].pos[1] = projection(1, 2) +
          projection(1, 1) * viewPosition.Y() * inverseZ;
        vertices[i].rhw = inverseZ;
        if (_preparedFogMode == FM_Fog ||
            _preparedFogMode == FM_FogAlpha)
        {
          const Matrix4 &inverseScale = camera->InvScaleMatrix();
          const float viewX = viewPosition.X() * inverseScale(0, 0);
          const float viewY = viewPosition.Y() * inverseScale(1, 1);
          const float distanceSquared = viewX * viewX + viewY * viewY +
            viewPosition.Z() * viewPosition.Z();
          const int fogAmount = GScene->Fog8(distanceSquared);
          const int fogTransmittance = intMax(0, intMin(255, 255 - fogAmount));
          vertices[i].specular.SetA8(fogTransmittance);
        }
        if (_trianglePrepared)
        {
          if (!useMaterialShader && _preparedMaterial._UVchanged)
          {
            vertices[i].t0.u += _preparedMaterial._offsetU;
            vertices[i].t0.v += _preparedMaterial._offsetV;
          }
          // TLVertexTable performs scene lighting before section materials are
          // known. Reapply the section diffuse/emissive terms here, as the
          // legacy renderer does when it sets the TL material for the draw.
          Color litColor = (Color)vertices[i].color;
          Color diffuseColor = litColor * _preparedTLMaterial.diffuse;
          diffuseColor += _preparedTLMaterial.emmisive;
          diffuseColor.SetA(litColor.A());
          vertices[i].color = PackedColor(diffuseColor);
        }
        if (useMaterialShader)
        {
          Dx11MaterialTLVertex &out = materialVertices[i];
          out.position[0] = vertices[i].pos[0];
          out.position[1] = vertices[i].pos[1];
          out.position[2] = vertices[i].pos[2];
          out.rhw = vertices[i].rhw;
          out.color = vertices[i].color;
          out.specular = vertices[i].specular;
          const UVPairs &uv0 = shape.GetUVArray();
          const UVPairs &uv1 = shape.GetUVArray(1);
          const Vector3 &modelPosition = shape.Pos(vertexIndex);
          for (UINT texGen = 0; texGen < TexMaterial::NTexGens; ++texGen)
          {
            const TexGenInfo &gen = preparedMaterial->_texGen[texGen];
            Vector3 texGenInput;
            switch (gen._uvSource)
            {
              case UVPos:
                texGenInput = modelPosition;
                break;
              case UVWorldPos:
                texGenInput = Vector3(VFastTransform, _modelToWorld,
                  modelPosition);
                break;
              case UVTex1:
                if (vertexIndex < uv1.Size())
                {
                  const UVPair &uv = uv1.UV(vertexIndex);
                  texGenInput = Vector3(uv.u, uv.v, 1.0f);
                }
                else if (vertexIndex < uv0.Size())
                {
                  const UVPair &uv = uv0.UV(vertexIndex);
                  texGenInput = Vector3(uv.u, uv.v, 1.0f);
                }
                else
                  texGenInput = Vector3(vertices[i].t0.u,
                    vertices[i].t0.v, 1.0f);
                break;
              default:
                if (vertexIndex < uv0.Size())
                {
                  const UVPair &uv = uv0.UV(vertexIndex);
                  texGenInput = Vector3(uv.u, uv.v, 1.0f);
                }
                else
                  texGenInput = Vector3(vertices[i].t0.u,
                    vertices[i].t0.v, 1.0f);
                break;
            }
            Vector3 transformedUV;
            if (_preparedMaterial._UVchanged && texGen == 0)
            {
              transformedUV = Vector3(texGenInput.X() +
                  _preparedMaterial._offsetU,
                texGenInput.Y() + _preparedMaterial._offsetV,
                texGenInput.Z());
            }
            else
            {
              transformedUV = gen._uvTransform.FastTransform(texGenInput);
            }
            out.uv[texGen][0] = transformedUV.X();
            out.uv[texGen][1] = transformedUV.Y();
          }
        }
      }
      if (invalid) continue;
      // Keep each polygon's fan triangulation intact while batching many faces
      // into one dynamic-buffer upload and one D3D11 draw call.
      for (int triangle = 1; triangle < face.N() - 1; ++triangle)
      {
        if (useMaterialShader)
        {
          if (materialTriangles.size() + 3 > 65532) flushTriangles();
          materialTriangles.push_back(materialVertices[0]);
          materialTriangles.push_back(materialVertices[triangle]);
          materialTriangles.push_back(materialVertices[triangle + 1]);
        }
        else
        {
          if (triangles.size() + 3 > 65532) flushTriangles();
          triangles.push_back(vertices[0]);
          triangles.push_back(vertices[triangle]);
          triangles.push_back(vertices[triangle + 1]);
        }
      }
    }
    flushTriangles();
  }
}

HWND AppInit(HINSTANCE hInst, HINSTANCE hPrev, int sw, bool fullscreen,
  int x, int y, int width, int height);
BOOL AppDone(HWND hwndApp, HINSTANCE hInstance);

// First game-startup integration for the new backend. The DX11 renderer is
// intentionally opt-in while the remaining game-specific render operations
// are ported; the default engine selection remains unchanged.
class Dx11GameEngine : public Dx11EngineCore
{
public:
  Dx11GameEngine() : _instance(NULL), _ownsWindow(false), _gamma(1.0f),
    _legacyMesh(NULL) {}
  virtual ~Dx11GameEngine()
  {
    if (_ownsWindow && _window) AppDone(_window, _instance);
    _window = NULL;
  }

  void OwnWindow(HINSTANCE instance)
  {
    _instance = instance;
    _ownsWindow = true;
  }

  virtual int GetMaxBonesCount() const { return 0; }
  virtual bool SwitchRefreshRate(int refresh) { return refresh == 0; }
  virtual bool SwitchWindowed(bool windowed, int, int, int)
  {
    return windowed == IsWindowed();
  }
  virtual void ListResolutions(FindArray<ResolutionInfo> &ret)
  {
    ret.Clear();
    DEVMODE mode = {};
    mode.dmSize = sizeof(mode);
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode))
    {
      ResolutionInfo info;
      info.w = (int)mode.dmPelsWidth;
      info.h = (int)mode.dmPelsHeight;
      info.bpp = (int)mode.dmBitsPerPel;
      ret.AddUnique(info);
    }
    ResolutionInfo windowed = {0, 0, 0};
    ret.AddUnique(windowed);
  }
  virtual void ListRefreshRates(FindArray<int> &ret)
  {
    ret.Clear();
    ret.Add(0);
  }
  virtual void SetGamma(float gamma) { _gamma = gamma; }
  virtual float GetGamma() const { return _gamma; }
  virtual float GetDefaultGamma() const { return 1.0f; }
  virtual void PrepareMesh(int, int)
  {
    _legacyMesh = NULL;
  }
  virtual bool BeginMesh(int, TLVertexTable &mesh, int)
  {
    if (!_deviceReady || mesh.NVertex() <= 0) return false;
    _legacyMesh = &mesh;
    return true;
  }
  virtual void EndMesh(int, TLVertexTable &mesh)
  {
    if (_legacyMesh == &mesh) _legacyMesh = NULL;
  }
  virtual void PrepareTriangle(int, const MipInfo &mip,
    const TexMaterial *, int specFlags)
  {
    DrawPolyPrepare(mip, specFlags);
  }
  virtual void DrawPolygon(const VertexIndex *indices, int nVertices)
  {
    if (!_legacyMesh || !_polyPrepared || !indices || nVertices < 3 ||
        nVertices > 4096)
      return;

    std::vector<TLVertex> vertices((size_t)nVertices);
    for (int i = 0; i < nVertices; ++i)
    {
      const int vertexIndex = indices[i];
      if (vertexIndex < 0 || vertexIndex >= _legacyMesh->NVertex()) return;
      vertices[i] = _legacyMesh->GetVertex(vertexIndex);
    }
    _renderer.SetLinearColorOutput(true);
    _renderer.DrawTLVertices((UINT)WidthBB(), (UINT)HeightBB(),
      &vertices[0], (UINT)vertices.size(), _polyTextured ? _polyTexture : NULL,
      true, true, 0.0f, false, NULL, false, true, NULL, NULL, 0,
      _polyTextured);
  }
  virtual void DrawSection(const FaceArray &faces, Offset beg, Offset end)
  {
    if (!_legacyMesh || !_polyPrepared) return;
    for (Offset faceIndex = beg; faceIndex < end; faces.Next(faceIndex))
    {
      const Poly &face = faces[faceIndex];
      DrawPolygon(face.GetVertexList(), face.N());
    }
  }
  virtual void DrawDecal(int, Vector3Par screen, float rhw, float sizeX,
    float sizeY, PackedColor color, const MipInfo &mip,
    const TexMaterial *, int specFlags)
  {
    if (!_deviceReady || Width2D() <= 0 || Height2D() <= 0 ||
        sizeX <= 0.0f || sizeY <= 0.0f)
      return;

    const float width = (float)Width2D();
    const float height = (float)Height2D();
    float x0 = screen.X() - sizeX;
    float x1 = screen.X() + sizeX;
    float y0 = screen.Y() - sizeY;
    float y1 = screen.Y() + sizeY;
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = 0.0f, v1 = 1.0f;

    if (x0 < 0.0f) { u0 = -x0 / (2.0f * sizeX); x0 = 0.0f; }
    if (x1 > width)
    {
      u1 = 1.0f - (x1 - width) / (2.0f * sizeX);
      x1 = width;
    }
    if (y0 < 0.0f) { v0 = -y0 / (2.0f * sizeY); y0 = 0.0f; }
    if (y1 > height)
    {
      v1 = 1.0f - (y1 - height) / (2.0f * sizeY);
      y1 = height;
    }
    if (x0 >= x1 || y0 >= y1) return;

    Vertex2DAbs vertices[4];
    vertices[0].x = x0; vertices[0].y = y0;
    vertices[1].x = x1; vertices[1].y = y0;
    vertices[2].x = x1; vertices[2].y = y1;
    vertices[3].x = x0; vertices[3].y = y1;
    vertices[0].u = u0; vertices[0].v = v0;
    vertices[1].u = u1; vertices[1].v = v0;
    vertices[2].u = u1; vertices[2].v = v1;
    vertices[3].u = u0; vertices[3].v = v1;
    for (int i = 0; i < 4; ++i)
    {
      vertices[i].z = screen.Z();
      vertices[i].w = rhw;
      vertices[i].color = color;
    }

    DrawPolyPrepare(mip, specFlags);
    DrawPolyDo(vertices, 4, Rect2DAbs(0.0f, 0.0f, width, height));
  }
  virtual void DrawLine(const TLVertexTable &mesh, int beg, int end, int)
  {
    if (!_deviceReady || beg < 0 || end < 0 ||
        beg >= mesh.NVertex() || end >= mesh.NVertex())
      return;

    const TLVertex &v0 = mesh.GetVertex(beg);
    const TLVertex &v1 = mesh.GetVertex(end);
    const float dx = v1.pos[0] - v0.pos[0];
    const float dy = v1.pos[1] - v0.pos[1];
    const float lengthSquared = dx * dx + dy * dy;
    const float inverseLength = lengthSquared > 0.0f
      ? 1.0f / sqrtf(lengthSquared) : 1.0f;
    const float halfWidth = 1.5f;
    const float px = dy * inverseLength * halfWidth;
    const float py = -dx * inverseLength * halfWidth;

    TLVertex quad[4] = {v0, v0, v1, v1};
    quad[0].pos[0] -= px; quad[0].pos[1] -= py;
    quad[1].pos[0] += px; quad[1].pos[1] += py;
    quad[2].pos[0] += px; quad[2].pos[1] += py;
    quad[3].pos[0] -= px; quad[3].pos[1] -= py;
    quad[0].t0.u = quad[0].t0.v = 0.0f;
    quad[1].t0.u = quad[1].t0.v = 0.0f;
    quad[2].t0.u = quad[2].t0.v = 0.0f;
    quad[3].t0.u = quad[3].t0.v = 0.0f;

    _renderer.SetLinearColorOutput(true);
    _renderer.DrawTLVertices((UINT)WidthBB(), (UINT)HeightBB(), quad, 4,
      NULL, true);
  }
  virtual void Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect,
    const Rect2DAbs &clip)
  {
    if (!pars.mip.IsOK() || rect.w <= 0 || rect.h <= 0) return;
    Vertex2DAbs vertices[4];
    vertices[0].x = rect.x;           vertices[0].y = rect.y;
    vertices[1].x = rect.x + rect.w;  vertices[1].y = rect.y;
    vertices[2].x = rect.x + rect.w;  vertices[2].y = rect.y + rect.h;
    vertices[3].x = rect.x;           vertices[3].y = rect.y + rect.h;
    vertices[0].u = pars.uTL; vertices[0].v = pars.vTL;
    vertices[1].u = pars.uTR; vertices[1].v = pars.vTR;
    vertices[2].u = pars.uBR; vertices[2].v = pars.vBR;
    vertices[3].u = pars.uBL; vertices[3].v = pars.vBL;
    vertices[0].color = pars.colorTL;
    vertices[1].color = pars.colorTR;
    vertices[2].color = pars.colorBR;
    vertices[3].color = pars.colorBL;
    DrawPolyPrepare(pars.mip, pars.spec);
    DrawPolyDo(vertices, 4, clip);
  }
  virtual float ZShadowEpsilon() const { return 0.01f; }
  virtual float ZRoadEpsilon() const { return 0.005f; }
  virtual float ObjMipmapCoef() const { return 1.5f; }
  virtual SimulWeatherBase *CreateSimulWeather()
  {
    return new SimulWeatherDX11(this, this);
  }
  virtual bool ZBiasExclusion() const { return false; }
  virtual int AFrameTime() const { return 16; }
  virtual RString GetDebugName() const { return "Direct3D 11 game engine"; }

  virtual void InitDraw(bool clear = false, Color color = HBlack)
  {
    Engine::InitDraw(clear, color);
    if (_deviceReady) BeginD3D11Frame();
  }
  virtual void FinishDraw(int wantedDurationVSync, bool allBackBuffers = false,
    RString fileName = RString())
  {
    if (_deviceReady)
    {
      HRESULT hr = PresentD3D11(1);
      if (FAILED(hr)) RptF("DX11 Present failed: 0x%08x", (unsigned)hr);
    }
    Engine::FinishDraw(wantedDurationVSync, allBackBuffers, fileName);
  }

private:
  HINSTANCE _instance;
  bool _ownsWindow;
  float _gamma;
  TLVertexTable *_legacyMesh;
};

Engine *CreateEngineD3D11(CreateEnginePars &pars)
{
  RptF("DX11: creating imported full renderer");
  Engine *engine = DX11::EngineDD11::CreateEngine(pars);
  if (!engine) RptF("DX11: imported renderer initialization failed");
  return engine;
}

void Dx11EngineCore::DrawPolyDo(const Vertex2DAbs *vertices, int nVertices,
  const Rect2DAbs &clip)
{
  DrawClippedPolygon(vertices, nVertices, clip, 0.0f, 0.0f);
}

void Dx11EngineCore::DrawPolyDo(const Vertex2DPixel *vertices, int nVertices,
  const Rect2DPixel &clip)
{
  if (!vertices || nVertices < 3 || nVertices > 32) return;
  Vertex2DAbs converted[32];
  for (int i = 0; i < nVertices; ++i)
  {
    converted[i].x = vertices[i].x;
    converted[i].y = vertices[i].y;
    converted[i].z = vertices[i].z;
    converted[i].w = vertices[i].w;
    converted[i].u = vertices[i].u;
    converted[i].v = vertices[i].v;
    converted[i].color = vertices[i].color;
  }
  const Rect2DAbs clipAbs(clip.x, clip.y, clip.w, clip.h);
  DrawClippedPolygon(converted, nVertices, clipAbs, (float)Left2D(),
    (float)Top2D());
}

void Dx11EngineCore::DrawLineDo(const Line2DAbs &line, PackedColor c0,
  PackedColor c1, const Rect2DAbs &clip)
{
  if (!_polyPrepared || !_deviceReady) return;
  const float dx = line.end.x - line.beg.x;
  const float dy = line.end.y - line.beg.y;
  const float lengthSquared = dx * dx + dy * dy;
  const float inverseLength = lengthSquared > 0.0f ? 1.0f / sqrtf(lengthSquared) : 1.0f;
  const float px = dy * inverseLength * 1.5f;
  const float py = -dx * inverseLength * 1.5f;
  Vertex2DAbs vertices[4];
  vertices[0].x = line.beg.x - px; vertices[0].y = line.beg.y - py;
  vertices[1].x = line.beg.x + px; vertices[1].y = line.beg.y + py;
  vertices[2].x = line.end.x + px; vertices[2].y = line.end.y + py;
  vertices[3].x = line.end.x - px; vertices[3].y = line.end.y - py;
  for (int i = 0; i < 4; ++i)
  {
    vertices[i].z = 0.5f;
    vertices[i].w = 1.0f;
    vertices[i].u = vertices[i].v = 0.0f;
  }
  vertices[0].color = vertices[1].color = c0;
  vertices[2].color = vertices[3].color = c1;
  DrawClippedPolygon(vertices, 4, clip, 0.0f, 0.0f);
}

void Dx11EngineCore::Pause()
{
  ClearTextureCache();
  _renderer.Reset();
  _polyTextured = false;
  _deviceReady = false;
}

void Dx11EngineCore::Restore()
{
  if (_window && _width && _height)
    InitializeD3D11(_window, _width, _height, _windowed);
}

void Dx11EngineCore::FogColorChanged(ColorVal fogColor)
{
  // Match the HDR-scaled fog constant used by the D3D9 and D3D10 renderers.
  // The base Engine keeps the unscaled scene fog in _fogColor.
  _renderFogColor = fogColor * GetHDRFactor();
  _renderFogColor.Saturate();
}

bool Dx11EngineCore::SwitchRes(int, int, int width, int height, int)
{
  if (!_deviceReady || width <= 0 || height <= 0) return false;
  HRESULT hr = _renderer.Resize((UINT)width, (UINT)height);
  if (FAILED(hr)) return false;
  _width = (UINT)width;
  _height = (UINT)height;
  return true;
}

RString Dx11EngineCore::GetDebugName() const
{
  return "Direct3D 11 engine core (abstract)";
}

AbstractTextBank *Dx11EngineCore::TextBank()
{
  return _textBank;
}
