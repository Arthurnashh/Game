#pragma once
#ifndef DAYZ_D3D11_ENGINE_CORE_HPP
#define DAYZ_D3D11_ENGINE_CORE_HPP

#include "../engine.hpp"
#include "Dx11Renderer.hpp"
#include "Dx11DeviceAccess.hpp"

class Dx11TextBank;

// Abstract game-engine layer for the D3D11 device and frame lifecycle.
// It deliberately remains abstract until mesh/material, text-bank, video,
// weather, and game-specific Engine operations have D3D11 implementations.
class Dx11EngineCore : public Engine, public Dx11DeviceAccess
{
public:
  Dx11EngineCore();
  virtual ~Dx11EngineCore();

  HRESULT InitializeD3D11(HWND window, UINT width, UINT height,
    bool windowed = true);
  HRESULT BeginD3D11Frame();
  HRESULT PresentD3D11(UINT syncInterval = 1);
  virtual ID3D11Device *D3D11Device() const { return _renderer.Device(); }
  virtual ID3D11DeviceContext *D3D11Context() const { return _renderer.Context(); }

  virtual bool IsDummy() { return false; }
  virtual bool GetTL() const { return true; }
  virtual void Clear(bool clearZ = true, bool clear = true,
    PackedColor color = PackedColor(0), bool clearStencil = true);
  virtual void DrawPolyPrepare(const MipInfo &mip, int specFlags = DefSpecFlags2D);
  virtual void DrawLinePrepare();
  virtual void DrawPolyDo(const Vertex2DAbs *vertices, int nVertices,
    const Rect2DAbs &clip = Rect2DClipAbs);
  virtual void DrawPolyDo(const Vertex2DPixel *vertices, int nVertices,
    const Rect2DPixel &clip = Rect2DClipPixel);
  virtual void DrawLineDo(const Line2DAbs &line, PackedColor c0, PackedColor c1,
    const Rect2DAbs &clip = Rect2DClipAbs);
  virtual void Pause();
  virtual void Restore();
  virtual void FogColorChanged(ColorVal fogColor);
  virtual bool SwitchRes(int x, int y, int w, int h, int bpp);
  virtual RString GetDebugName() const;
  virtual int Width() const { return (int)_width; }
  virtual int Height() const { return (int)_height; }
  virtual int PixelSize() const { return 32; }
  virtual int RefreshRate() const { return 0; }
  virtual bool CanBeWindowed() const { return true; }
  virtual bool IsWindowed() const { return _windowed; }
  virtual AbstractTextBank *TextBank();
  virtual void BeginInstanceTL(int cb, const PositionRender &modelToWorld,
    float minDist2, int spec, const LightList &lights,
    const DrawParameters &dp = DrawParameters::_default);
  virtual bool BeginMeshTL(int cb, const VertexTableAnimationContext *animContext,
    const Shape &shape, const EngineShapeProperties &properties, int spec,
    const LightList &lights, int bias);
  virtual void SetSkinningType(int cb, ESkinningType skinningType,
    const Matrix4 *bones = NULL, int nBones = 0);
  virtual void PrepareTriangleTL(int cb, const PolyProperties *section,
    const MipInfo &mip, const TexMaterialLODInfo &mat, int specFlags,
    const TLMaterial &tlMat, int spec, const EngineShapeProperties &properties);
  virtual void EndMeshTL(int cb, const Shape &shape);
  virtual void DrawSectionTL(int cb, const Shape &shape, int beg, int end,
    int bias, int instancesOffset = 0, int instancesCount = 0,
    const DrawParameters &dp = DrawParameters::_default);
  virtual int GetBias(int cb) { return _activeBias; }
  virtual void SetBias(int cb, int bias) { _activeBias = bias; }
  virtual void GetZCoefs(int cb, float &zAdd, float &zMult)
  {
    zMult = 1.0f - _activeBias * 1.0e-7f;
    zAdd = _activeBias * -2.0e-7f;
  }

protected:
  struct TextureCacheEntry
  {
    RStringB name;
    Dx11Texture *texture;
    unsigned __int64 lastUsed;
    TextureCacheEntry() : texture(NULL), lastUsed(0) {}
  };

  Dx11Renderer _renderer;
  Dx11TextBank *_textBank;
  // BeginMeshTL/DrawSectionTL/EndMeshTL share one transformed-mesh buffer.
  // Keep the full callback sequence exclusive so another render worker
  // cannot delete or replace _tlMesh while clipping/drawing is in progress.
  CriticalSection _meshSequenceLock;
  Matrix4 _modelToWorld;
  Matrix4 _modelToView;
  Matrix4 _worldToModel;
  Matrix4Array _skinningMatrices;
  ESkinningType _skinningType;
  const LightList *_lights;
  const Shape *_activeShape;
  TLVertexTable *_tlMesh;
  ClipFlags _activeClip;
  int _activeSpec;
  int _activeBias;
  MipInfo _preparedMip;
  TexMaterialLODInfo _preparedMaterial;
  TLMaterial _preparedTLMaterial;
  int _preparedSpecFlags;
  float _preparedAlphaTestReference;
  EFogMode _preparedFogMode;
  Color _renderFogColor;
  bool _trianglePrepared;
  HWND _window;
  UINT _width;
  UINT _height;
  bool _windowed;
  bool _deviceReady;
  bool _polyPrepared;
  bool _polyTextured;
  static const int TextureCacheSize = 32;
  TextureCacheEntry _textureCache[TextureCacheSize];
  Dx11Texture *_polyTexture;
  unsigned __int64 _textureCacheClock;

  HRESULT DrawClippedPolygon(const Vertex2DAbs *vertices, int nVertices,
    const Rect2DAbs &clip, float xOffset, float yOffset);
  HRESULT PrepareTexture(const Texture *source, Dx11Texture **texture);
  void ClearTextureCache();
};

#endif
