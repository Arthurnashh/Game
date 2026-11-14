#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARMA3_SIMULCLOUDRENDERERDX9_HPP
#define _ARMA3_SIMULCLOUDRENDERERDX9_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulCloudRendererBase.hpp"
#include <Es/Common/delegate.hpp>
#include "SimulCommonDX9.hpp"
#include "../d3d9/engdd9.hpp"
#include "../d3d9/d3d9Buffers.hpp"

class EngineDD9;



//-----------------------------------------------------------------------------
/*!
DX9 implementation of SimulCloudRenderer
*/
//-----------------------------------------------------------------------------
class SimulCloudRendererDX9: public SimulCloudRendererBase
{
public:
  SimulCloudRendererDX9(EngineDD9* engine);

  //@{ Implementation of CloudRenderCallback
  virtual void FillCloudTextureSequentially(int texture_index,int texel_index,int num_texels,const unsigned *uint32_array);
  virtual void CycleTexturesForward();
  virtual void FillIlluminationSequentially(int source_index,int texel_index,int num_texels,const unsigned char *uchar8_array);
  //@}

  //@{ Implementation of BaseCloudRenderer
  //! Get the list of three textures used for cloud rendering.
  void **GetCloudTextures();
  //@}

  /// setting of textures used for distance fading
  void SetLossTextures(Ref<TextureSysGPUDX9>& t1,Ref<TextureSysGPUDX9>& t2);
  void SetInscatterTextures(Ref<TextureSysGPUDX9>& t1, Ref<TextureSysGPUDX9>& t2);

  //@{ Implementation of SimulCloudRendererBase
  /// renders clouds
  virtual void Render(int cb, bool cubemap = false);
  //@}

private:
  //@{ Implementation of SimulCloudRendererBase
  /// initializes renderer - creates all resources,..., returns if succeeded
  virtual bool InitRenderer();
  /// deinitializes renderer - destroys all resources,...
  virtual void DeInitRenderer();
  /** creates noise texture */
  virtual void CreateNoiseTexture(RString fileName);
  /** creates cloud texture */
  virtual void CreateCloudTexture();
  /** creates illumination texture */
  virtual void CreateIlluminationTexture();
  //@}

  /// setups shader constants for cloud rendering
  void SetupShaderConstants(int cb, bool cubemap);
  /// setups textures for cloud rendering
  void SetupTextures(int cb);
  /// setups render states for cloud rendering
  void SetupRenderStates(int cb);
  /// setups render states AFTER cloud rendering
  void RestoreRenderStates(int cb);
  /// setups shaders for clouod rendering
  void SetupShaders(int cb);
  /// updates helper and renders geometry
  void RenderGeometry(int cb, bool cubemap);

  /// render geometry using DrawPrimitiveUP
  void RenderGeometryPrimitiveUP(int cb, bool cubemap, Camera* camera);
  /// render geometry using vertex buffers
  void RenderGeometryVertexBuffer(int cb, bool cubemap, bool async, Camera* camera);

  /// returns free vertex buffer that we can use for drawing
  Ref<UserVertexBuffer > GetFreeVertexBuffer(int numVertices);

  /// creates noise texture from directX file (dds, tga, ...), returns if succeeded
  bool CreateNoiseTextureFromDXFile(RString fileName);
  /// creates noise texture from arma file (paa), returns if succeeded
  bool CreateNoiseTextureFromFile(RString fileName);

  /// format of textures
  D3DFORMAT _textureFormat;

  /// pointer to rendering engine
  EngineDD9* _engine;

  /// Three cloud textures - 2 to interpolate and one to fill
  Ref<TextureSysGPUDX9> _cloudTextures[TextureCount];
  /// Cloud textures used only for function GetCloudTextures
  IDirect3DVolumeTexture9* _cloudTexturesPtrs[TextureCount];

  /// Illumination texture  
  Ref<TextureSysGPUDX9> _illuminationTexture;

  /// Noise texture loaded from paa file
  Ref<Texture> _noiseTexture;
  /// Created noise texture or texture loaded from dds file
  ComRef<IDirect3DTexture9> _noiseTextureCreated;

  /// textures for distance fading taken from sky renderer
  Ref<TextureSysGPUDX9> _skyLossTexture1;
  Ref<TextureSysGPUDX9> _skyLossTexture2;
  Ref<TextureSysGPUDX9> _skyInscatterTexture1;
  Ref<TextureSysGPUDX9> _skyInscatterTexture2;

  /// array for vertex buffer used for CPU fade
  UserVertexBufferArray _vertexBuffersCPU;
	UserVertexBufferArray _vertexBuffersGPU;

  /// vertices for drawing - used to fill vertex buffers
  AutoArray<Vertex_t> _vertices;
  AutoArray<CPUFadeVertex_t> _cpuFadeVertices;


  RString _noiseTextureFileName;

  /// critical section to protect vertex buffer creation
  CriticalSection _renderVBCritSection;
};

#endif //_ARMA3_SIMUL_WEATHER
#endif //_ARMA3_SIMULCLOUDRENDERERDX9_HPP