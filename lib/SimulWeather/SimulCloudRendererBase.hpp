#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARMA3_SIMULCLOUDRENDERERBASE_HPP
#define _ARMA3_SIMULCLOUDRENDERERBASE_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulWeatherIncludes.hpp"
#include "Es/Strings/rString.hpp"
#include "El/Color/colors.hpp"
#include "El/ParamFile/paramFile.hpp"

/**
Base class for Simul Weather cloud renderers used in Arma
*/
class SimulCloudRendererBase: public simul::clouds::BaseCloudRenderer
{
public:
  static const float DefaultMaxFadeDistanceMetres;

  // max vertices for cloud geometry drawing
  static const int MaxVertices = 10000;
  // max vertices for one dynamic vertex buffer  used for drawing
  static const int MaxVBVertices = 20000;
  /// number of textures we need (2 to interpolate, 1 to fill)
  static const int TextureCount = 3;

  /// rendering mode of clouds
  enum RenderMode
  {
    RMPrimitiveUP, //rendering using DrawPrimitiveUP
    RMVertexBuffer, //rendering using vertex buffer
    RMVertexBufferAsync, //rendering using vertex buffer, that is created asynchronously (in vis thread)
    RMCount
  };

  SimulCloudRendererBase();
  virtual ~SimulCloudRendererBase();

  //@{ Implementation of CloudRenderCallback
  virtual void SetCloudTextureSize(unsigned width_x,unsigned length_y,unsigned depth_z);
  virtual void FillCloudTextureBlock(int texture_index,int x,int y,int z,int w,int l,int d,const unsigned *uint32_array);
  virtual void SetIlluminationGridSize(unsigned width_x,unsigned length_y,unsigned depth_z);
  virtual void FillIlluminationBlock(int source_index,int x,int y,int z,int w,int l,int d,const unsigned char *uchar8_array);
  //@}

  //@{ Implementation of BaseCloudRenderer
  virtual bool CreateNoiseTexture(bool) {return false;} //we are using CreateNoiseTexture(RString fileName) instead
  //}

  //! Call this once to set the sky interface that this cloud renderer can use for distance fading.
  void SetSkyInterface(simul::sky::BaseSkyInterface *si);
  //! Call this when the D3D device has been created or reset.
  void SetFadeTableInterface(simul::sky::FadeTableInterface *fti);

  /// initializes renderer (checks for formats, creates resouorces,...), returns if succeeded
  bool Init(); 
  /// deinitializes renderer (checks for formats, creates resouorces,...)
  void DeInit(); 

  /// performs simulation 
  virtual void Simulate(float deltaT);
  /// renders clouds
  /// \param cb command buffer id (-1 for immediate)
  virtual void Render(int cb, bool cubemap = false) = 0;

  /// recalculates lighting of clouds
  virtual void RecalculateLighting();

  /// diag drawing
  virtual void DiagDraw();


  bool GetInitialized() const {return _initialized;}

  simul::clouds::CloudGeometryHelper* GetHelperPtr() {return helper.get();}
  simul::sky::BaseSkyInterface* GetSkyInterface() {return skyInterface;}

  RenderMode GetRenderMode() const {return _renderMode;}
  void SetRenderMode(RenderMode mode) 
  {
    Assert(mode >= RMPrimitiveUP && mode < RMCount);
    if (mode >= RMPrimitiveUP && mode < RMCount)
      _renderMode = mode;
  }

  //! Set the elevation/azimuth grid sizes.
  void SetHelperGrid(unsigned el,unsigned az);
  //! sets helper detail (number of layers and max distance of meters)
  void SetHelperDetail(int numLayers, int maxDistance);
  //! sets near cloud fade of helper
  //! Set a value, usually between 0 and 1, that determines how close cloud layers must be to the camera before starting to fade out.
  //! This value is a multiple for the distance between cloud layers at the nearest point.
  void SetHelperNearCloudFade(float fade);

  //! returns max distance the helper is using for creating geometry (in meters)
  float GetHelperMaxDistance();

  /// initializes from cfg
  void InitFromCfg(ParamEntryPar entry);
  /// setups keyframes based on given keyframe
  void InitKeyframes(int numKeyframes, simul::clouds::CloudKeyframer::Keyframe& keyframe);

  /// applies values of src keyframe to dest keyframe (not all values - we don't want to overwrite daytime for example)
  void ApplyKeyframe(simul::clouds::CloudKeyframer::Keyframe& dest, const simul::clouds::CloudKeyframer::Keyframe& src);

  // Distance for fade texture lookups:
  void SetMaxFadeDistanceKm(float distKm)
  {
    _maxFadeDistanceMetres=distKm*1000.f;
  }

  /** fills vertices with cloud geometry 
  \param cloudRenderer cloud renderer used for filling
  \param verticesToFill array of vertices we want to fill
  \param maxVertices max. possible number of vertices
  \param vertexSize size of one vertex in bytes
  \param cameraMatrix camera matrix
  \param cemaraProjMatrix camera projection matrix
  \param cubemap ?
  \param useCpuFade if CPU fade mode is used
  \param enableLightning if lightning is enabled
  \return number of vertices that were created
  */
  static int FillVertices(SimulCloudRendererBase* cloudRenderer, void* vertices, int maxVertices, int vertexSize, 
    const Matrix4& cameraMatrix, const Matrix4& cameraProjMatrix, 
    bool cubemap, bool useCpuFade, bool enableLightning);

  void SetFog(float start, float end) {_fogStart = start; _fogEnd = end;}

  /// vertices for drawing
  /// TEMPORARY SOLUTION!
  struct float2
  {
    float x,y;
    void operator=(const float*f)
    {
      x=f[0];
      y=f[1];
    }
  };
  struct float3
  {
    float x,y,z;
    void operator=(const float*f)
    {
      x=f[0];
      y=f[1];
      z=f[2];
    }
  };
  struct Vertex_t
  {
    float3 position;
    float3 texCoords;
    float layerFade;
    float2 texCoordsNoise;
    float3 sunlightColour;
  };
  struct CPUFadeVertex_t : public Vertex_t
  {
    float3 loss;
    float3 inscatter;
  };


private:
  /// initializes renderer - creates all resources,..., returns if succeeded
  virtual bool InitRenderer() = 0;
  /// deinitializes renderer - destroys all resources,...
  virtual void DeInitRenderer() = 0;
  /** creates noise texture */
  virtual void CreateNoiseTexture(RString fileName) = 0;
  /** creates cloud texture */
  virtual void CreateCloudTexture() = 0;
  /** creates illumination texture */
  virtual void CreateIlluminationTexture() = 0;

  /// updates camera position based on scene camera
  void UpdateCameraPos();

  /// if the renderer was initialized
  bool _initialized;
  /// rendering mode
  RenderMode _renderMode;

	float _baseChurn;

#if _ENABLE_CHEATS
  struct DiagInfo
  {
    Vector3 sunOcclusionCamPos;
    Vector3 sunOcclusionDir;
  };
  DiagInfo _diagInfo;
#endif

protected:

  /// illumination texture dimensions
  unsigned _illuminationTexWidth;
  unsigned _illuminationTexHeight;
  unsigned _illuminationTexDepth;

  /// color that is used to modify cloud lighting in shader
  Color _cloudLighting;

  // multiplier for wind speed
  float _windSpeedCoef;

  // global coef, that is used to multiply direct lighting of clouds
  float _directLightCoef;
  // global coef, that is used to multiply indirect lighting of clouds
  float _indirectLightCoef;

  /// max. fade distance in metres
  float _maxFadeDistanceMetres;
  /// arma fog for clouds start (in meters)
  float _fogStart;
  /// arma fog for clouds end (in meters)
  float _fogEnd;
};

TypeIsMovable(SimulCloudRendererBase::Vertex_t);
TypeIsMovable(SimulCloudRendererBase::CPUFadeVertex_t);

#endif //_ARMA3_SIMUL_WEATHER
#endif //_ARMA3_SIMULCLOUDRENDERERBASE_HPP