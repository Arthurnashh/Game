#ifndef _postProcess_hpp_
#define _postProcess_hpp_

#include "engdd9.hpp"
#include <El/ParamArchive/serializeClass.hpp>
#include "../paramArchiveExt.hpp"
#include "../global.hpp"

//set and get saturated scalar
#define SETGET_SC_CLAMP(name, param, fMin, fMax) \
	void set##name(float i, bool imm = false)     { saturate(i, fMin, fMax); _pInterp._pars.##param = i; if (imm) _pInterp._lastPars.##param = i; } \
	float get##name() const												{ return _pInterp._pars.##param; } \

#define SETGET_SC(name, param) \
	void set##name(float i, bool imm = false)     { _pInterp._pars.##param = i; if (imm) _pInterp._lastPars.##param = i; } \
	float get##name() const												{ return _pInterp._pars.##param; } \

#define SETGET_VEC(name, param) \
	void set##name(Vector3Par i, bool imm = false){ _pInterp._pars.##param = i; if (imm) _pInterp._lastPars.##param = i; } \
	Vector3Par get##name() const									{ return _pInterp._pars.##param; } \

#define SETGET_INT(name, param) \
	void set##name(int i, bool imm = false)				{ _pInterp._pars.##param = i;  if (imm) _pInterp._lastPars.##param = i; } \
	int get##name() const													{ return _pInterp._pars.##param; } \

#define SETGET_BOOL(name, param) \
	void set##name(bool i, bool imm = false)			{ _pInterp._pars.##param = i;  if (imm) _pInterp._lastPars.##param = i; } \
	bool get##name() const												{ return _pInterp._pars.##param; } \

//-----------------------------------------------------------------------------
/*!
Abstract ancestor of post processes
*/
//-----------------------------------------------------------------------------
class EngineDD9::PostProcess : public RefCount, public SerializeClass
{
protected:
  //! Engine
  static EngineDD9 *_engine;
  //! Preparing of the post process (called each time in Do)
  virtual void Prepare();
  //! Finishing of the post process (called each time at the end of Do)
  virtual void Finish();
  
  //! effect priority - priority define order of applying effect to scene
  int _priority;
  //! enable/disable effect based on video options value(level)
  unsigned char _level;
  /// helper function for pixel shader compilation
  bool CompilePixelShader(
    const LazyLoadFile &hlsl, const char *psName, ComRef<IDirect3DPixelShader9> &ps,
    bool fullPrec=false, D3DXMacroList *d3DXMacroList = NULL

  ) const;
  /// helper function for vertex shader compilation
  bool CompileVertexShader(
    const LazyLoadFile &hlsl, const char *vsName, ComRef<IDirect3DVertexShader9> &vs
  ) const;
  
public:
  //! Setting of the engine pointer
  static void SetEngine(EngineDD9 *engine) {_engine = engine;}
  //! Initialization of the post process
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs) {SetLevel(0xFF); return true;}
  //! Applying of post process effect
  /*!
    @param isLast when true, no more postprocess effects will be called - if possible, postprocess to backbuffer
  */
  virtual void Do(bool isLast) = 0;
#if _HELISIM_RENDER_TO_TEXTURE
  virtual void DoDirect(const RECT *rect, int idx) {}    
#endif
  /// check if the postprocess pass will be executed
  /** some effects sometimes do not execute, depending on parameters */
  virtual bool WillBeDone() const {return true;}
  /* Set effect priority - priority define order of applying pp to scene */
  void SetPriority(int priority) { _priority = priority; }
  /* Get effect priority */
  int GetPriority() const { return _priority; }
  /* Check if effect is enabled */
  virtual bool IsEnabled() { return true; }
  /* Enable / disable effect */
  virtual void Enable (bool enable) {};
  /* Set effect params, when time is 0 then params are set immediately, else pars are linearly interpolated from last used to last set */
  virtual void Commit(float time) {};
  /* Check whether given post process effect is commited */
  virtual bool Committed() const {return false;}
  /* Set effect parameters */
  virtual int SetParams(const AutoArray<float> &pars) {return 0;}
  virtual int SetParams(const float* pars, int nPars) {return 0;}  
  virtual void SetParamsDirect(const float *pars, int nPars) { }
  virtual bool UnInit() {return true;}
  virtual LSError Serialize(ParamArchive &ar) {return LSOK;}
  /* Set effect level mask - enable/disable effect based on value set in video options */
  virtual void SetLevel(unsigned char level) {_level = level;}
  virtual unsigned char GetLevel() const {return _level;}

};

//-----------------------------------------------------------------------------
/*!
Abstract ancestor of simple post processes
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPSimple : public PostProcess
{
private:
  typedef PostProcess base;
protected:
  struct Vertex
  {
    Vector3P pos;
    UVPair t0;
  };
  VertexStaticData9::BufferFuture _vb;
  ComRef<IDirect3DVertexDeclaration9> _vd;
  ComRef<IDirect3DVertexShader9> _vs;
  ComRef<IDirect3DPixelShader9> _ps;
  
  virtual void Prepare();
public:
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const char *vsName);
};


//-----------------------------------------------------------------------------
/*!
Custom edge vertex structure
*/
//-----------------------------------------------------------------------------
struct VertexCustomEdge
{
  Vector3P pos;
  float index;
};


//-----------------------------------------------------------------------------
class EngineDD9::PostProcessCustomEdge : public PostProcess
{
  private:
  typedef PostProcess base;
  
  protected:
  //!{ Custom edge resources
  VertexStaticData9::BufferFuture _vbCustomEdge;
  ComRef<IDirect3DVertexDeclaration9> _vdCustomEdge;
  ComRef<IDirect3DVertexShader9> _vsCustomEdge;
  //!}

  public:
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  virtual void Prepare();
};

//-----------------------------------------------------------------------------
/*
Gaussian blur effect

  This effect is not applied directly to the output - it only calculates and holds
  blurred image for later use.
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPGaussianBlur : public PostProcessCustomEdge
{
private:
  typedef PostProcessCustomEdge base;
protected:

  //!{ Horizontal blur resources
  ComRef<IDirect3DPixelShader9> _psHorizontal;
  ComRef<IDirect3DTexture9> _rtHorizontal;
  ComRef<IDirect3DSurface9> _rtsHorizontal;
  //!}

  //!{ Final blur resources
  ComRef<IDirect3DPixelShader9> _psBlur;
  ComRef<IDirect3DTexture9> _rtBlur;
  ComRef<IDirect3DSurface9> _rtsBlur;
  //!}

  // source texture to blur
  IDirect3DTexture9 *_toBlur;
  
public:
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  void Do(bool isLast, IDirect3DTexture9 *toBlur);
  virtual void Do(bool isLast);
  //! Function to retrieve the blurred texture
  ComRef<IDirect3DTexture9> GetBlurredTexture() const {return _rtBlur;}
  //! function to retrieve first pass RT texture
  ComRef<IDirect3DTexture9> GetFirstRtTexture() const {return _rtHorizontal;}
  //! function to retrieve first pass RT surface
  ComRef<IDirect3DSurface9> GetFirstRt() const {return _rtsHorizontal;}
  //! function to retrieve second pass RT surface
  ComRef<IDirect3DSurface9> GetSecondRt() const {return _rtsBlur;}
};


//-----------------------------------------------------------------------------
/*!
Final - copying of source to back buffer
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPFinal : public PostProcessCustomEdge
{
private:
  typedef PostProcessCustomEdge base;
  
  ComRef<IDirect3DPixelShader9> _ps;
  
public:
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  virtual void Do(bool isLast);
  void Do(bool isLast, bool forceNoSRGBOnRead);
  void Do(IDirect3DTexture9 *rt, float w, float h, bool rescale, bool forceNoSRGBOnRead);
};

//-----------------------------------------------------------------------------
/*!
Final - rescaling of source to back buffer (bicubic filter)
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPRescaleBicubic : public PPSimple
{
private:
  typedef PPSimple base;
  
  ComRef<IDirect3DTexture9> _lookUpTexture;
  
public:
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  virtual void Do(bool isLast);
  void Do(IDirect3DTexture9 *rt, float w, float h);
};

//-----------------------------------------------------------------------------
/*!
container for graphics resources of enforce shape collection
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPEnfShapeCollectionRes: public PostProcess
{
public:
	//just container for debug shapes render
	ComRef<IDirect3DVertexDeclaration9> _vd;
	ComRef<IDirect3DVertexShader9>			_vs;
	ComRef<IDirect3DPixelShader9>				_ps;

	bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
	void Do(bool isLast) {}
	bool UnInit();
	void Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
};

//-----------------------------------------------------------------------------
/*!
Bloom effect
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPBloom: public PostProcess
{
private:
	typedef PostProcess base;
protected:
	static const int KERNEL_SIZE = 9;

	//!{ Custom edge resources
	VertexStaticData9::BufferFuture _vbCustomEdge;
	ComRef<IDirect3DVertexDeclaration9> _vdCustomEdge;
	ComRef<IDirect3DVertexShader9> _vsCustomEdge;
	//!}

	ComRef<IDirect3DPixelShader9> _psBlur;
	ComRef<IDirect3DPixelShader9> _psDownsample2;
	ComRef<IDirect3DPixelShader9> _psDownsample4;
	ComRef<IDirect3DPixelShader9> _psCombine;
	//ComRef<IDirect3DPixelShader9> _ps;

	//!{ Bright resources
	ComRef<IDirect3DTexture9> _rtBright;
	ComRef<IDirect3DSurface9> _rtsBright;
	//!}

	//!{ Horizontal blur resources
	ComRef<IDirect3DTexture9> _rtHorizontal;
	ComRef<IDirect3DSurface9> _rtsHorizontal;
	//!}

	//!{ Final blur resources
	ComRef<IDirect3DTexture9> _rtBlur;
	ComRef<IDirect3DSurface9> _rtsBlur;
	//!}

	Ref<Texture> _gammaTexture;

	//! Custom edge resources creation
	bool InitCustomEdge(const LazyLoadFile &vs);

	float		_imageScale;
	float		_blurScale;
	float		_threshold;

public:
	float GetBlurScale() const
	{
		return _blurScale;
	}

	float GetImageScale() const
	{
		return _imageScale;
	}

	//! Virtual method
	virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
	//! Virtual method
	virtual void Do(bool isLast);
	
  void Do(bool isLast, IDirect3DTexture9 *rt);
	//! Function to retrieve the blurred texture
	IDirect3DTexture9 *GetBlurredTexture() const {return _rtBlur;}
	/// set effect specific parameters
	int SetParams(const float *pars, int nPars) override;
};

//-----------------------------------------------------------------------------
/*!
SSSM creation
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPSSSM : public PPSimple
{
private:
  typedef PPSimple base;
  //! Alternate shadow buffer visualization shader for nVidia HW
  ComRef<IDirect3DPixelShader9> _psNVidia;

	#ifdef TEST_RAIN_MAP
		ComRef<IDirect3DPixelShader9> _psRainMap;
	#endif

protected:
  //! Virtual method
  virtual void Prepare();
public:
  //! Non-virtual version of init
  bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  virtual void Do(bool isLast) {}
  void Do(float minShadowsZ, float maxShadowsZ, bool firstLayer, bool lastLayer);
		
	#ifdef TEST_RAIN_MAP
		void DoRainMapPP();
	#endif
};

//-----------------------------------------------------------------------------
/*!
SSSM Stencil shadows inclusion
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPSSSMStencil : public PPSimple
{
private:
  typedef PPSimple base;
public:
  //! Non-virtual version of init
  bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  //! Virtual method
  virtual void Do(bool isLast);
};

#ifdef SSSMBLUR

//-----------------------------------------------------------------------------
/*!
SSSMB creation
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPSSSMB : public PPGaussianBlur
{
protected:
  //! Shader to perform the final screenspace shadow blur
  ComRef<IDirect3DPixelShader9> _psFinalBlur;
public:
  //! Virtual method
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  //! Virtual method
  virtual void Do(bool isLast);
};
#endif


//-----------------------------------------------------------------------------
/*!
Depth of field effect
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPDOF : public PPSimple
{
private:
  typedef PPSimple base;

protected:
  //! Reference to post process with a texture we require here in this post process
  Ref<PPGaussianBlur> _ppGaussianBlur;

  //true if DistanceDOF distance is used
  bool _useFarDistance;
public:
  //! Non-virtual version of init
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const Ref<PPGaussianBlur> &ppGaussianBlur);
  virtual void Do(bool isLast);
	void Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
};

//-----------------------------------------------------------------------------
/*
Depth of field for undefined focal plane case (we blur only from focal plane distance further)
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPDistanceDOF : public PPDOF
{
private:
  typedef PPDOF base;
  typedef PPSimple grandBase;
public:
  //! Non-virtual version of init
	virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const Ref<PPGaussianBlur> &ppGaussianBlur);
};


// Number of queries (width of the histogram)
static const int NBrightQueries = 16;

//-----------------------------------------------------------------------------
/*!
Manager of the queries
*/
//-----------------------------------------------------------------------------
class EngineDD9::QueryManager
{
protected:
  static EngineDD9 *_engine;
  static const int NItems = 4;

  ComRef<IDirect3DQuery9> _query[NItems][NBrightQueries];
  /// record value for each query so that we know the absolute (HDR) meaning of the result
  float _value[NItems][NBrightQueries];
  /// for debugging purposes we may want to store one master value with each query set
  float _masterValue[NItems];
  /// new query to be issued
  int _newQuery;
  /// query which should be ready first
  int _oldestQuery;
  /// how many queries were issued
  int _queriesIssued;
  /// gather results as we get them
  int _results[NBrightQueries];
  /// index of the next result to come
  int _currentResultIndex;
  //@{ once results are finalized, copy them here
  int _mostRecentResult[NBrightQueries];
  float _mostRecentResultValue[NBrightQueries];
  float _mostRecentMasterValue;
  //@}

  int Next(int i) {return (i < NItems - 1) ? ++i : 0;}
public:
  static void SetEngine(EngineDD9 *engine) {_engine = engine;}
  QueryManager();
  //! Method to reset all queries
  void ResetQueries();
  bool Issue(const float *value, const float*alpha, float master);
  bool Issue(IDirect3DDevice9 *device, const float *value, const float *alpha, float masterValue);
  float GetNumberOfPixelsDrawn(int *npd, float *value, bool usePixelCount = false);
};

//-----------------------------------------------------------------------------
/*!
Manager of the queries
*/
//-----------------------------------------------------------------------------
class EngineDD9::QueryManagerSinglePass
{
protected:
  static EngineDD9 *_engine;
  static const int NItems = 8;
  ComRef<IDirect3DQuery9> _query[NItems];
  /// record value for each query so that we know the absolute (HDR) meaning of the result
  float _value[NItems][NBrightQueries];
  /// for debugging purposes we may want to store one master value with each query set
  float _masterValue[NItems];
  /// new query to be issued
  int _newQuery;
  /// query which should be ready first
  int _oldestQuery;
  /// how many queries were issued
  int _queriesIssued;
  //@{ once results are finalized, copy them here
  int _mostRecentResult[NBrightQueries];
  float _mostRecentResultValue[NBrightQueries];
  float _mostRecentMasterValue;
  //@}

  int Next(int i) {return (i < NItems - 1) ? ++i : 0;}
public:
  static void SetEngine(EngineDD9 *engine) {_engine = engine;}
  QueryManagerSinglePass();
  //! Method to reset all queries
  void ResetQueries();
  bool Issue(const float *value, float master, IDirect3DTexture9 *alphaTex);
  bool Issue(IDirect3DDevice9 *device, const float *value, float masterValue, IDirect3DTexture9 *alphaTex);
  float GetNumberOfPixelsDrawn(int *npd, float *value);
};

//-----------------------------------------------------------------------------
class EngineDD9::PPDecimate : public PostProcess
{
private:
  typedef PostProcess base;
protected:
  //@{ cascade of low-resolution render targets
  /// 16 render targets is enough for more then 64Kx64K pixels
  /** each pass performs 4x or 2x decimation */
  enum {MaxDecimations=16};
  struct DecimationPass
  {
    ComRef<IDirect3DTexture9> _rt;
    ComRef<IDirect3DSurface9> _rts;
    int _w,_h;
  };
  DecimationPass _dec[MaxDecimations];
  // end of 4x passes (avg only)
  int _decimations;
  //@}

#if _HELISIM_RENDER_TO_TEXTURE

  enum { MaxR2TDecimations = 2 };
  DecimationPass _decR2T[MaxR2TDecimations];

  //@{ RT used for measurements
  ComRef<IDirect3DTexture9> _rtMeasureR2T;
  ComRef<IDirect3DSurface9> _rtsMeasureR2T;
  ComRef<IDirect3DSurface9> _rtsMeasureDepthR2T;
  //@}

#endif

  //!{ Custom edge resources
  VertexStaticData9::BufferFuture _vbCustomEdge;
  ComRef<IDirect3DVertexDeclaration9> _vdCustomEdge;
  ComRef<IDirect3DVertexShader9> _vsCustomEdge;
  //!}

  /// initial luminance calculations
  ComRef<IDirect3DPixelShader9> _psLuminance;
  /// average luminance down-sampling
  ComRef<IDirect3DPixelShader9> _psAvgLuminance;
  /// brightness query pixel shader
  ComRef<IDirect3DPixelShader9> _psQuery;

  //@{ RT used for measurements
  ComRef<IDirect3DTexture9> _rtMeasure;
  ComRef<IDirect3DSurface9> _rtsMeasure;
  ComRef<IDirect3DSurface9> _rtsMeasureDepth;
  //@}

  //! Query manager
  QueryManager _query;

public:
  //! Histogram of the image
  float _histogram[NBrightQueries];
  int _temperatures[NBrightQueries];

  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
#if _HELISIM_RENDER_TO_TEXTURE
  virtual void DoDirect(const RECT *rect, int idx);
#endif
  virtual void Do(bool isLast);
  //   //! Function to retrieve the blurred texture
  //   ComRef<IDirect3DTexture9> GetBlurredTexture() const {return _rtBlur;}
};

//-----------------------------------------------------------------------------
/*!
Thermal visualization
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPFinalThermal : public PPSimple
{
private:
  typedef PPSimple base;
protected:
  //! Decimation post process for histogram retrieving
  PPDecimate _ppDecimate;
  //! Post process with a texture we require here in this post process
  PPGaussianBlur _ppGaussianBlur;

  //! Thermal visualisation shader
  ComRef<IDirect3DPixelShader9> _psThermal;

  ComRef<IDirect3DTexture9> _rtTexture; // temporal render target
  ComRef<IDirect3DSurface9> _rtSurface;

  // last capture frame time
  Time _time;

  //@{ parameters
  float _deltaT;
  //@}

#if _HELISIM_RENDER_TO_TEXTURE

  ComRef<IDirect3DTexture9> _blurRT;
  ComRef<IDirect3DSurface9> _blurRTS;

  ComRef<IDirect3DTexture9> _rtTextureR2T; // temporal render target
  ComRef<IDirect3DSurface9> _rtSurfaceR2T;

#endif

public:
  //! Virtual method
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  //! Virtual method
  int SetParams(const float *pars, int nPars);
  //! Virtual method
  virtual void Do(bool isLast);
#if _HELISIM_RENDER_TO_TEXTURE
  virtual void DoDirect(const RECT *rect, int idx);
#endif
};

//-----------------------------------------------------------------------------
/*!
Glow
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPGlow : public PostProcess
{
private:
  typedef PostProcess base;
protected:
  //! Reference to post process with a texture we require here in this post process
  Ref<PPGaussianBlur> _ppGaussianBlur;
  //@{ cascade of low-resolution render targets
  /// 16 render targets is enough for more then 64Kx64K pixels
  /** each pass performs 4x or 2x decimation */
  enum {MaxDecimations=16};
  struct DecimationPass
  {
    ComRef<IDirect3DTexture9> _rt;
    ComRef<IDirect3DSurface9> _rts;
    int _w,_h;
  };
  DecimationPass _dec[MaxDecimations];
#if _HELISIM_RENDER_TO_TEXTURE
  DecimationPass _decR2T[MaxDecimations];
#endif
  // end of 4x passes (avg only)
  int _decimation4x;
  // end of last 2x passes (avg/min/max only) - index+1
  int _decimation2x;
  //@}
  
  //@{ 1x1 RT used for aperture control - double buffered
  ComRef<IDirect3DTexture9> _rtAperture[2];
  ComRef<IDirect3DSurface9> _rtsAperture[2];
  int _rtApertureCurrent;

  /// histogram generating texute (individual brigtnes thresholds)
  ComRef<IDirect3DTexture9> _histogramTholds;
  //@}

  #if _DEBUG
  /// debugging - lockable sys.mem. surface
  ComRef<IDirect3DSurface9> _rtsDebugAp;
  ComRef<IDirect3DSurface9> _rtsDebugDec;
  
  Color GetContent(IDirect3DSurface9 *surf, IDirect3DSurface9 *temp, D3DFORMAT format);
  Color GetContentAp(IDirect3DSurface9 *surf);
  Color GetContentDec(IDirect3DSurface9 *surf);
  #endif

  //@{ 1x1 RT used for measurements
  ComRef<IDirect3DTexture9> _rtMeasure;
  ComRef<IDirect3DSurface9> _rtsMeasure;
  ComRef<IDirect3DSurface9> _rtsMeasureDepth;
  //@}
  
  //@{ custom edge UV vertex shader + buffer + declaration
	VertexStaticData9::BufferFuture _vbCustomEdge;
	ComRef<IDirect3DVertexDeclaration9> _vdCustomEdge;
	ComRef<IDirect3DVertexShader9> _vsCustomEdge;
  //@}
  
  /// initial luminance calculations
  ComRef<IDirect3DPixelShader9> _psLuminance;
  /// average luminance down-sampling
  ComRef<IDirect3DPixelShader9> _psAvgLuminance;
  /// avg-min-max down-sampling
  ComRef<IDirect3DPixelShader9> _psMaxAvgMinLuminance;
  /// final luminace operator
  ComRef<IDirect3DPixelShader9> _psAssumedLuminance;
  /// brightness query pixel shader
  ComRef<IDirect3DPixelShader9> _psQuery;
  //! Pixel shader for glow
  ComRef<IDirect3DPixelShader9> _psGlow;
  //! Pixel shader for glow + night eye vision
  ComRef<IDirect3DPixelShader9> _psGlowNight;
  //! Pixel shader for glow + night vision (color transformation)
  ComRef<IDirect3DPixelShader9> _psGlowNVG;

  //! Queries
  QueryManager _query;

  //! Query manager
  QueryManagerSinglePass _querySinglePass;

  //@{ parameters
  float _deltaT;
  float _eyeAdaptMin;
  float _eyeAdaptMax;
  //@}

public:
  static void SetEngine(EngineDD9 *engine)
  {
    QueryManager::SetEngine(engine);
    QueryManagerSinglePass::SetEngine(engine);
  }
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, PPGaussianBlur *blur);
  virtual void Do(bool isLast);
#if _HELISIM_RENDER_TO_TEXTURE
  virtual void DoDirect(const RECT *rect, int idx);
#endif
  
  int SetParams(const float *pars, int nPars);

  //! Method to return aperture texture
  ComRef<IDirect3DTexture9> GetApertureTexture() {return _rtAperture[_rtApertureCurrent];};
  //! Method to return decimation texture
  ComRef<IDirect3DTexture9> GetDecimationTexture() {return _dec[_decimation2x-1]._rt;};
  //! Returns cones
  float Cones() const;
  //! Returns EyeSens coefficient
  Color EyeSens(float cones) const;
  //! Method to cancel all queries
  void CancelQueries() {_query.ResetQueries();}

  void SetDefaultToneMapping();
};


//-----------------------------------------------------------------------------
/*!
Calculating of flare intensity into special texture
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPFlareIntensity : public PostProcess
{
private:
  typedef PostProcess base;
protected:
  //!{ Flare intensity resources
  const static int RenderTargetNum = 4;
  ComRef<IDirect3DTexture9> _rtLightValues[RenderTargetNum];
  ComRef<IDirect3DSurface9> _rtsLightValues[RenderTargetNum];
  //!}
public:
  virtual void Prepare();
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  virtual void Do(bool isLast) {};
  void Do(int x, int y, int sizeX, int sizeY);
};


#endif


//-----------------------------------------------------------------------------
/*!
vertex data
*/
//-----------------------------------------------------------------------------
struct VertexP3T2
{
	Vector3P pos;
	UVPair tc;
};


//! create vertex buffer data for post-process screen plane
bool CreateScreenPlane( VertexP3T2 *v, int segsX, int segsY, float ou, float ov );
//! create vertex buffer data for wet distortion PP screen plane
bool CreateScreenPlane2( VertexP3T2 *v, int segsX, int segsY );

//-----------------------------------------------------------------------------
/*!
RotBlur - blurring during camera rotation
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPRotBlur : public PostProcess
{
private:
	typedef PostProcess base;
protected:
	VertexStaticData9::BufferFuture _vb;
	ComRef<IDirect3DVertexDeclaration9> _vd;
	ComRef<IDirect3DVertexShader9> _vs;
	ComRef<IDirect3DPixelShader9> _ps;
	//! num verts in screen plane
	int vertexCount;
	//! num tris in triangle strip
	int stripSize;

	//! post-process enabled
	bool enabled;
	//! cos of threshold angle ( default value = cos( 1.0 * PI / 180 ) )
	float minCos;
	//! blur power ( default value = 0.03 )
	float blurPower;
	//! depth threshold ...areas with greater value (closer to screen) are not blurred  ( default (compromise) value = 1.2 )
	float depthThreshold;
  //! last frame view matrix
  Matrix4 _oldView;
  //! maximal rotational angle
  float _upperBound;
  //! rendering time of last frame
  Time _lastTime;
  
	//! called before Do()
	virtual void Prepare();

public:
	//! is effect enabled
	bool IsEnabled() { return enabled; }
	//! enable/disable effect
	void Enable( bool enable ) { enabled = enable; }
	//! sets minimal rotation angle between frames for applying of blur effect (in degrees; default = 1.0)
	void SetThresholdAngle( float angle = 1.0 );
	//! set blur power ( default value = 0.03 )
	void SetBlurPower( float value ) { blurPower = value; }
	//! get blur power
	float GetBlurPower() const { return blurPower; }

	//! set depth threshold
	void SetDepthThreshold( float value ) { depthThreshold = value; }
	//! get depth threshold
	float GetDepthThreshold() const { return depthThreshold; }

	//! default values of params (brightness, lerpColor, colorizeColor)
	void SetDefaultValues()
	{
		SetThresholdAngle( 1.0 );
    SetDepthThreshold( 0.5f );
	}

	//! Virtual method
	virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
	//bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const Ref<PPGaussianBlur> &ppGaussianBlur, const Camera *camera );
	//! Virtual method
	virtual void Do(bool isLast);

	void Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
	{
		UnInit();
		Init(hlsl, vs);
	}
};

//-----------------------------------------------------------------------------
/*!
RadBlurPar structure store parameters for PPRadialBlur class, these params are used for effect control.

For setting PPRadialBlur params is used EngineDD9::SetRadialBlurrParams() function. 

For applying last set params is necessary used Commit(time) function.

When Commit(time) is called, parameters are linearly interpolated between last used and last set params on the 
basis of elapsed time. When time is set to zero, params are applied immediately. Linear interpolation is
done by class PPEParsInterpolator.                                                                     
*/
//-----------------------------------------------------------------------------
template<class Type>
struct SimulateForOneTraits
{
  static void Simulate(Type &tmp, Type src, float diff)
  {
    tmp = tmp + (src - tmp) * diff;
  }
};

//-----------------------------------------------------------------------------
/*!
specialization for bool type, interpolation for this type is nonsensical
*/
//-----------------------------------------------------------------------------
template<>
struct SimulateForOneTraits<bool>
{
  static void Simulate(bool &tmp, bool src, float diff)
  {
  }
};

//-----------------------------------------------------------------------------
/*!
linear interpolation between src and tmp value on the basis of diff parameter
*/
//-----------------------------------------------------------------------------
struct SimulateForOne
{
  float _diff;

  SimulateForOne(float diff):_diff(diff){}

  template<class Type>
  void operator() (Type &tmp, Type src, const char *id)
  {
    SimulateForOneTraits<Type>::Simulate(tmp, src, _diff);
  }
};

//-----------------------------------------------------------------------------
/*!
implement serialize via as global Serialize call, will redirect to ParamArchive member if needed
not using ::Serialize directly allows for specialization 
*/
//-----------------------------------------------------------------------------
template<class Type>
static LSError SerializeOneFunc(ParamArchive &ar, Type &tmp, const char *id)
{
  return ::Serialize(ar,id,tmp,0,Type());
}

//-----------------------------------------------------------------------------
/*!
serialize one item
*/
//-----------------------------------------------------------------------------
struct SerializeOne
{
  ParamArchive &_ar;
  LSError _error;
  SerializeOne(ParamArchive &ar):_ar(ar),_error(LSOK){}

  template<class Type>
  void operator() (Type &tmp, const char *id)
  {
    if (_error==LSOK)
    {
      _error = SerializeOneFunc<Type>(_ar, tmp, id);
    }
  }
};

//-----------------------------------------------------------------------------
/*!
adapter: call unary functor for a given member
*/
//-----------------------------------------------------------------------------
template <class Pars, class MemberType, class Functor>
struct CallForMember
{
  Functor _func;
  MemberType Pars::*_member;
  
  CallForMember(const Functor &func, MemberType Pars::*member):_func(func),_member(member){}
  void operator () (const char *id)
  {
    _func(_func.GetObj().*_member,id);
  }


};

//-----------------------------------------------------------------------------
// adapter: call binary functor for a given member
template <class Pars, class MemberType, class Functor>
struct CallForMembers
{
  Functor _func;
  MemberType Pars::*_member;
  
  CallForMembers(const Functor &func, MemberType Pars::*member):_func(func),_member(member){}
  void operator () (const char *id)
  {
    _func(_func.GetObj().*_member,_func.GetWith().*_member,id);
  }

};

//-----------------------------------------------------------------------------
/// creative function
template <template <class T, class MT, class F> class Func, class MemberType, class Functor, class Pars>
void CallFunctorForMember(Functor func, MemberType Pars::*member, const char *id)
{
  // create a functor and call it
  Func<Pars, MemberType, Functor>(func,member)(id);
}


#define PAR_MEMBER_LIST(x) CallFunctorForMember<Func>(func,&Par::_##x,#x);
#define PAR_MEMBER_LIST_ENF(x) CallFunctorForMember<Func>(func,&Par::##x,#x);

//-----------------------------------------------------------------------------
/*
  Store parameters for radial post process effect
*/
//-----------------------------------------------------------------------------
struct RadBlurPar
{
  typedef RadBlurPar Par;
  
  //! default value = 0.01
  float _powerX;
  float _powerY;
  //! blur offset - size of unblured center area ( default value = 0.06 )
  float _offsetX;
  float _offsetY;

  
  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST(powerX);
    PAR_MEMBER_LIST(powerY);
    PAR_MEMBER_LIST(offsetX);
    PAR_MEMBER_LIST(offsetY);
  }

};

//-----------------------------------------------------------------------------
/// extend parameters core with expected functionality
template <class Pars>
struct ParsExt: public Pars, public SerializeClass
{
  /// bind one arguments into a functor
  template <class Functor>
  struct BindUnary: public Functor
  {
    Pars &_obj;
    
    BindUnary(Pars &obj, const Functor &func):_obj(obj),Functor(func){}
    Pars &GetObj() const {return _obj;}
  };
  /// bind two arguments into a functor
  template <class Functor>
  struct BindBinary: public Functor
  {
    Pars &_obj;
    const Pars &_with;
    
    BindBinary(Pars &obj, const Pars &with, const Functor &func):_obj(obj),_with(with),Functor(func){}
    Pars &GetObj() const {return _obj;}
    const Pars &GetWith() const {return _with;}
  };
  /// call binary functor for each member
  template<class Func>
  void ForEachWith(Func func, const Pars &with)
  {
    Pars::ForEachMember<CallForMembers>(BindBinary<Func>(*this,with,func));
  }
  /// call unary functor for each member
  template<class Func>
  void ForEach(Func func)
  {
    Pars::ForEachMember<CallForMember>(BindUnary<Func>(*this,func));
  }

  Pars Simulate(const Pars &with, float diff)
  {
    ParsExt<Pars> tmp=*this;
    tmp.ForEachWith(SimulateForOne(diff), with);
    return tmp;
  }

  LSError Serialize(ParamArchive &ar)
  {
    SerializeOne serialize(ar);
    ForEach(serialize);
    return serialize._error;
  }
};

//-----------------------------------------------------------------------------
/// Class provide linear interpolation based on elapsed time, between down and upper bound.
template<class ParFX>
class PPEParsInterpolator : public SerializeClass
{
  Time _startTime;
  Time _endTime;  
  bool _commitDone;
  float _timeRange;

public:
  // upper bound
  ParsExt<ParFX> _pars;
  // down bound
  ParsExt<ParFX> _lastPars;

public:
  PPEParsInterpolator(void):_commitDone(true), _timeRange(0.0f) {}
  // set parameters to default values
  void Initialize(void) { _lastPars = _pars; }

  void Commit(float time)
  {
    _startTime = Glob.time;
    _endTime = _startTime + time;
    _commitDone = false;

    _timeRange = (time > 0.0f) ? (1.0f / time) : 0.0;

    // immediate simulation
    if (time <= 0)
    {
      Simulate();
    }
  }

  bool Committed() const {return _commitDone;}

  
  // set params for effect
  ParFX Simulate(void)
  {
    if (_commitDone)
    {
      return _lastPars;
    }
    else
    {
      // perform interpolation on the basis of the elapsed time
      if (Glob.time < _endTime)
      {
        float diff = (Glob.time - _startTime) * _timeRange;

        // linear interpolated params, interpolation is done between _lastPars and _pars
        ParFX parL = _lastPars.Simulate(_pars, diff);
        
        // return interpolated params 
        return parL;
      }
      // commit done
      else
      {
        _lastPars = _pars;
        _commitDone = true;

        // return final params
        return _pars;
      }
    }
  }

	virtual LSError Serialize(ParamArchive &ar)
  {
    CHECK(ar.Serialize("lastPars",_lastPars, 0));
    CHECK(ar.Serialize("pars",_pars, 0));
    CHECK(ar.Serialize("commitDone", _commitDone, 0));
    CHECK(ar.Serialize("timeRange", _timeRange, 0));

    if (ar.IsLoading())
    {
      Time saveTime;
      CHECK(::Serialize(ar, "saveTime",saveTime,0,Time(0)));
      CHECK(::Serialize(ar, "startTime",_startTime,0,Time(0)));
      CHECK(::Serialize(ar, "endTime",_endTime,0,Time(0)));

      float dt = Glob.time - saveTime;
      // bounds time shift
      _startTime += dt;
      _endTime += dt;
    }

    if (ar.IsSaving())
    {
      // saving time, used for correct time boundaries shift
      Time currTime = Glob.time;
      CHECK(::Serialize(ar, "saveTime",currTime ,0,Time(0)));
      CHECK(::Serialize(ar, "startTime",_startTime,0,Time(0)));
      CHECK(::Serialize(ar, "endTime",_endTime,0,Time(0)));
    }

    return LSOK;
  }
};

//-----------------------------------------------------------------------------
class EngineDD9::PPResources: public RefCount
{
public:
  __forceinline const PPResources *GetType() const { return this; }
  virtual void UnInit() {}
	virtual bool IsValid() const
	{
		return true;
	}
};

//-----------------------------------------------------------------------------
class EngineDD9::PPResourcesExt: public PPResources
{
  // @{ classes using resourcesExt
  friend class EngineDD9::PPSimpleExt;
  friend class EngineDD9::PPChromAber;
  friend class EngineDD9::PPRadialBlur;
  friend class EngineDD9::PPColorsInversion;
  // @}

protected:
  VertexStaticData9::BufferFuture _vb;
  ComRef<IDirect3DVertexDeclaration9> _vd;
  ComRef<IDirect3DVertexShader9> _vs;
  ComRef<IDirect3DPixelShader9> _ps;

public:
  PPResourcesExt() : _vb(VertexStaticData9::BufferFuture::Null) {}
  void UnInit()
  {
    _vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
    _vd.Free();
    _vs.Free();
    _ps.Free();
  }
};

//----------------------------------------------------------------------------- 
class EngineDD9::PPSimpleExt : public PostProcess
{
  friend class EngineDD9::PostProcess;

private:
  typedef PostProcess base;

protected:
  struct Vertex
  {
    Vector3P pos;
    UVPair t0;
  };
  // IUnknown stuff
  Ref<PPResources> _resrc;
  // Set vs, ps stuff
  virtual void Prepare();

public:
  // no IUnknown initialization
  virtual bool Init(PPResources *resources, bool firstInit);
  // first - initialization of IUnknown stuff
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const char *vsName);
  const PPResourcesExt* Resources() { return static_cast<const PPResourcesExt *> (_resrc->GetType()); }
};


//-----------------------------------------------------------------------------
/*!
 RadialBlur - centered radial zoom blur with optional unblurred area around center
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPRadialBlur : public PPSimpleExt//, public SerializeClass
{
private:
	typedef PPSimpleExt base;
protected:
	//! post-process enabled
	bool enabled;
  // down and upper bound for interpolation
  PPEParsInterpolator<RadBlurPar> _pInterp;

public:
	//! is effect enabled
	bool IsEnabled() { return enabled; }
	//! enable/disable effect
	void Enable( bool enable ) { enabled = enable; }

	//! set blur power ( default value = 0.01 )
  void SetBlurPower( float powerX, float powerY ) { _pInterp._pars._powerX = powerX; _pInterp._pars._powerY = powerY; }
	//! set blur offset ( default value = 0.06 )
	void SetBlurOffset( float offsetX, float offsetY ) { _pInterp._pars._offsetX = offsetX; _pInterp._pars._offsetY = offsetY; }
	//
	void SetDefaultValues()
	{
		SetBlurPower( 0.01f, 0.01f );
		SetBlurOffset( 0.06f, 0.06f );
	}

	//! Init - virtual method
	virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit);
  bool Init(PPResources *res, bool firstInit);
  //! Release resources used by effect
  virtual bool UnInit();
	//! Do - virtual method
	virtual void Do(bool isLast);
	
	//@{ implement SerializeClass interface
	//virtual bool IsDefaultValue(ParamArchive &ar) const {return false;}
	virtual LSError Serialize(ParamArchive &ar);
	virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
  //@}

  void Commit(float time) { _pInterp.Commit(time); }
  bool Committed() const {return _pInterp.Committed();}
  int SetParams(const AutoArray<float> &pars);
  int SetParams(const float* pars, int nPars);
};


//-----------------------------------------------------------------------------
/*
  Store parameters for color correction post process effect
*/
//-----------------------------------------------------------------------------
struct ColorPar
{
  typedef ColorPar Par;
  
  //! image brightness ( <0;2>, 0..black, 2..white, 1 ..default
  float _brightness;
  //! image contrast ( default value = 1 )
  float _contrast;
  //! contrast offset ( default value = 0 )
  float _offset;
  //! overlay color blending (rgb ..lerp color, a ..lerp factor)
  Color _lerpColor;
  //! color for colorization;  (rgb ..color, a ..saturation; white is default)
  Color _colorizeColor;
  //! rgb weights for desaturation ( 0.299, 0.587, 0.114 )
  Color _rgbWeights;

  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST(brightness);
    PAR_MEMBER_LIST(contrast);
    PAR_MEMBER_LIST(offset);
    PAR_MEMBER_LIST(lerpColor);
    PAR_MEMBER_LIST(colorizeColor);
    PAR_MEMBER_LIST(rgbWeights);
  }
};

//-----------------------------------------------------------------------------
/*!
PPColors IUnknown* stuff, shared between instances
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPColorResources: public PPResources
{
  friend class EngineDD9::PPColors;

private:
  VertexStaticData9::BufferFuture _vb;
  ComRef<IDirect3DVertexDeclaration9> _vd;
  ComRef<IDirect3DVertexShader9> _vs;
  ComRef<IDirect3DPixelShader9> _ps;
  //! num verts in screen plane
  int _vertexCount;
  //! num tris in triangle strip
  int _stripSize;

public:
  PPColorResources() : _vb(VertexStaticData9::BufferFuture::Null) {}
  void UnInit()
  {
    _vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
    _vd.Free();
    _vs.Free();
    _ps.Free();
  }
};


//-----------------------------------------------------------------------------
/*!
Colors - group of color adjustment effects ( contrast, brightness, saturation, colorization, color overlay )
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPColors : public PostProcess//, public SerializeClass
{
private:
	typedef PostProcess base;

  friend class EngineDD9::PPColorResources;
protected:
	//! post-process enabled
	bool enabled;
  //! class members
  PPEParsInterpolator<ColorPar> _pInterp;
  // IUnknown stuff
  Ref<PPResources> _resrc;

	//! set stuff
	virtual void Prepare();

public:
	//! is effect enabled
	bool IsEnabled() { return enabled; }
	//! enable/disable effect
	void Enable( bool enable ) { enabled = enable; }

	//! set image brightness (0 ..black, 1 ..unchanged, 2 ..white )
	void SetBrightness( float value ) { _pInterp._pars._brightness = value; }
	//! get image brightness
	float GetBrightness() const { return _pInterp._pars._brightness; }

	//! set image contrast (1 ..normal contrast)
	void SetContrast( float value ) { _pInterp._pars._contrast = value; }
	//! get image contrast
	float GetContrast() const { return _pInterp._pars._contrast; }

	//! set image contrast offset (0 ..unchanged )
	void SetOffset( float value ) { _pInterp._pars._offset = value; }
	//! get image contrast offset
	float GetOffset() const { return _pInterp._pars._offset; }

	//! set color for blending ( rgb ..color, a ..blend factor ( 0..original color, 1..blend color ) )
	void SetBlendColor( Color color ) { _pInterp._pars._lerpColor = color; }
	//! get color for blending
	Color GetBlendColor() const { return _pInterp._pars._lerpColor; }

	//! set color for colorization ( rgb ..color, a ..saturation ( 0..original color, 1..BW multiplied by colorize color ) )
	void SetColorizeColor( Color color ) { _pInterp._pars._colorizeColor = color; }
	//! get color for colorization
	Color GetColorizeColor() const { return _pInterp._pars._colorizeColor; }

	//! set rgb weights for desaturation ( 0.299, 0.587, 0.114, NA )
	void SetRGBWeights( Color weights ) { _pInterp._pars._rgbWeights = weights; }
	//! get color for colorization
	Color GetRGBWeights() const { return _pInterp._pars._rgbWeights; }

	//! default values of params (brightness, lerpColor, colorizeColor)
	void SetDefaultValues()
	{
		SetBrightness( 1.0 );
		SetContrast( 1.0 );
		SetOffset( 0.0 );
		SetBlendColor( Color( 0.0f, 0.0f, 0.0f, 0.0f ) );
		SetColorizeColor( HWhite );
    SetRGBWeights( Color( R_EYE, G_EYE, B_EYE ) );
	}
	//!
	bool IsActive()
	{
		return IsEnabled();
	}
	//! Virtual method
	virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit);
	virtual bool Init(PPResources *resources, bool firstInit);

  const PPColorResources* Resources() { return static_cast<const PPColorResources *> (_resrc->GetType()); }

  //! Release resources used by effect
  virtual bool UnInit();
	//! Virtual method
	virtual void Do( bool isLast );

  virtual void DoDirect(const RECT *rect, int idx);

	//virtual void Do1( bool isLast );

  //@{ implement SerializeClass interface
  //virtual bool IsDefaultValue(ParamArchive &ar) const {return false;}
  virtual LSError Serialize(ParamArchive &ar);
  virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
  //@}

  void Commit(float time) { _pInterp.Commit(time); }
  bool Committed() const {return _pInterp.Committed();}
  int SetParams(const AutoArray<float> &pars);
  int SetParams(const float* pars, int count);
};


//-----------------------------------------------------------------------------
struct FourFloats
{
  float _x[4];
  
  float &operator [] (int i) {return _x[i];}
  float operator [] (int i) const {return _x[i];}
  
  operator const float *() const {return _x;}
  
  FourFloats operator + (const FourFloats &w) const
  {
    FourFloats r;
    r._x[0] = _x[0]+w._x[0];
    r._x[1] = _x[1]+w._x[1];
    r._x[2] = _x[2]+w._x[2];
    r._x[3] = _x[3]+w._x[3];
    return r;
  }
  FourFloats operator - (const FourFloats &w) const
  {
    FourFloats r;
    r._x[0] = _x[0]-w._x[0];
    r._x[1] = _x[1]-w._x[1];
    r._x[2] = _x[2]-w._x[2];
    r._x[3] = _x[3]-w._x[3];
    return r;
  }
  FourFloats operator * (float f) const
  {
    FourFloats r;
    r._x[0] = _x[0]*f;
    r._x[1] = _x[1]*f;
    r._x[2] = _x[2]*f;
    r._x[3] = _x[3]*f;
    return r;
  }
};


//-----------------------------------------------------------------------------
/// advertize template specialization
template <>
LSError Serialize(ParamArchive &ar, const RStringB &name, FourFloats &value, int minVersion);

//-----------------------------------------------------------------------------
/*
Store parameters for color correction post process effect
*/
//-----------------------------------------------------------------------------
struct WetDistortPar
{
  typedef WetDistortPar Par;
  
  //! blurriness of distorted image
  float _blurriness;
  //! effect power (top/bottom of screen separately)
  float _powerTop, _powerBottom;
  //! waves speeds .. frequency/PI ( index 0,1 ..horizontal waves; 2,3 ..vertical )
  FourFloats _waveSpeeds;
  //! waves amplitudes (delta texture coordinates) (default: 0.0054, 0.0041, 0.0090, 0.0070 )
  FourFloats _waveAmps;
  /*! coefficients for phase computing;
  [0]..randX, [1]..randY - weight of random vertex data on horizontal/vertical wave phases
  [2]..posX, [3]..posY - weight of vertex X/Y-position on horizontal/vertical wave phases */
  FourFloats _phaseCoefs;

  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST(blurriness);
    PAR_MEMBER_LIST(powerTop);
    PAR_MEMBER_LIST(powerBottom);
    PAR_MEMBER_LIST(waveSpeeds);
    PAR_MEMBER_LIST(waveAmps);
    PAR_MEMBER_LIST(phaseCoefs);
  }
  
};

//-----------------------------------------------------------------------------
class EngineDD9::PPWetDistortResources: public PPResources
{
  friend class EngineDD9::PPWetDistort;

private:
  VertexStaticData9::BufferFuture _vb;
  ComRef<IDirect3DVertexDeclaration9> _vd;
  ComRef<IDirect3DVertexShader9> _vs;
  ComRef<IDirect3DPixelShader9> _ps;
  //! num verts in screen plane
  int _vertexCount;
  //! num tris in triangle strip
  int _stripSize;

public:
  PPWetDistortResources() : _vb(VertexStaticData9::BufferFuture::Null) {}
  void UnInit()
  {
    _vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
    _vd.Free();
    _vs.Free();
    _ps.Free();
  }
};

//-----------------------------------------------------------------------------
/*!
WetDistort - image distortion & blur
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPWetDistort : public PostProcess//, public SerializeClass
{
  friend class PPWetDistortResources;

private:
	typedef PostProcess base;
protected:
	//! reference to GaussianBlur post process
	Ref<PPGaussianBlur> _ppGaussianBlur;
	//! post-process enabled
	bool enabled;
  //! class parameters
  PPEParsInterpolator<WetDistortPar> _pInterp;
  //! resources stuff
  Ref<PPResources> _resrc;

  Matrix4 _oldView;
  
	virtual void Prepare();

public:
	//! is effect enabled
	bool IsEnabled() { return enabled; }
	//! enable/disable effect
	void Enable( bool enable ) { enabled = enable; }

	//! set effect blurriness <0,1> (values above 1 causes unusual )
	void SetBlurriness( float value ) { _pInterp._pars._blurriness = value; }
	//! get effect blurriness
	float GetBlurriness() { return _pInterp._pars._blurriness; }
	//! set effect power (top/bottom of screen separately) <0,1>
	void SetEffectPower( float top, float bottom ) { _pInterp._pars._powerTop = top; _pInterp._pars._powerBottom = bottom; }
	//! get effect power on top of screen
	float GetPowerOnTop() { return _pInterp._pars._powerTop; }
	//! get effect power on bottom of screen
	float GetPowerOnBottom() { return _pInterp._pars._powerBottom; }
	//! set waves speeds (frequency/PI)
	void SetWaveSpeeds( float horizontal1, float horizontal2, float vertical1, float vertical2 ) 
  {
		_pInterp._pars._waveSpeeds[0] = horizontal1; _pInterp._pars._waveSpeeds[1] = horizontal2; 
    _pInterp._pars._waveSpeeds[2] = vertical1; _pInterp._pars._waveSpeeds[3] = vertical2; 
  }
	//! get waves speeds
	const float* GetWaveSpeeds() { return _pInterp._pars._waveSpeeds; }
	//! set waves amplitudes
	void SetWaveAmplitudes( float horizontal1, float horizontal2, float vertical1, float vertical2 ) 
  {
		_pInterp._pars._waveAmps[0] = horizontal1; _pInterp._pars._waveAmps[1] = horizontal2; 
    _pInterp._pars._waveAmps[2] = vertical1; _pInterp._pars._waveAmps[3] = vertical2; 
  }
	//! get waves amplitudes
	const float* GetWaveAmplitudes() { return _pInterp._pars._waveAmps; }

	//! set waves amplitudes
	void SetPhaseCoefs( float randX, float randY, float posX, float posY )
  {
		_pInterp._pars._phaseCoefs[0] = randX; _pInterp._pars._phaseCoefs[1] = randY; 
    _pInterp._pars._phaseCoefs[2] = posX; _pInterp._pars._phaseCoefs[3] = posY; 
  }
	//! get waves amplitudes
	const float* GetPhaseCoefs() { return _pInterp._pars._phaseCoefs; }

	//! set default values of params
	void SetDefaultValues()
	{
		SetBlurriness( 1 );
		SetEffectPower( 1, 1 );
		SetWaveSpeeds( 4.10, 3.70, 2.50, 1.85 );
		SetWaveAmplitudes( 0.0054, 0.0041, 0.0090, 0.0070 );
		SetPhaseCoefs( 0.5, 0.3, 10.0, 6.0 );		
	}

	//! 
	bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, PPGaussianBlur *ppGaussianBlur, bool firstInit);
  bool Init(PPResources *resources, PPGaussianBlur *ppGaussianBlur, bool firstInit);
  //! Release resources used by effect
  virtual bool UnInit();
	virtual void Do(bool isLast);

  //@{ implement SerializeClass interface
  //virtual bool IsDefaultValue(ParamArchive &ar) const {return false;}
  virtual LSError Serialize(ParamArchive &ar);
  virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
  //@}

  void Commit(float time) { _pInterp.Commit(time); }
  bool Committed() const {return _pInterp.Committed();}
  int SetParams(const AutoArray<float> &pars);
  int SetParams(const float* pars, int nPars);

  const PPWetDistortResources* Resources() { return static_cast<const PPWetDistortResources*> (_resrc->GetType()); }
};


//-----------------------------------------------------------------------------
/*
Store parameters for color correction post process effect
*/
//-----------------------------------------------------------------------------
struct ChromAberPar
{
  typedef ChromAberPar Par;
  
  //! blur power ( default value = 0.01 )
  float _powerX, _powerY;
  //!  (default = false)
  bool _aspectCorrection;

  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST(powerX);
    PAR_MEMBER_LIST(powerY);
    PAR_MEMBER_LIST(aspectCorrection);
  }

  LSError Serialize(ParamArchive &ar)
  {
    CHECK(ar.Serialize("powerX",_powerX,0,0.005f));
    CHECK(ar.Serialize("powerY",_powerY,0,0.005f));
    CHECK(ar.Serialize("aspectCorrection",_aspectCorrection,0,false));

    return LSOK;
  }
};

//-----------------------------------------------------------------------------
/*!
ChromAber - lens chromatic aberration
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPChromAber: public PPSimpleExt
{
private:
	typedef PPSimpleExt base;
protected:
	//! post-process enabled
	bool enabled;
  //! class parameters
  PPEParsInterpolator<ChromAberPar> _pInterp;

public:
	//! is effect enabled
	bool IsEnabled() { return enabled; }
	//! enable/disable effect
	void Enable( bool enable ) { enabled = enable; }

	//! set aberration power ( default value = 0.01 )
	void SetAberration( float aberationPowerX, float aberationPowerY ) { _pInterp._pars._powerX = aberationPowerX; _pInterp._pars._powerY = aberationPowerY; }
	//! enable / disable aspect correction
	void SetAspectCorrection( bool value ) { _pInterp._pars._aspectCorrection = value; }
	//! 
	void SetDefaultValues()
	{
		SetAberration( 0.005f, 0.005f );
		SetAspectCorrection( false );
	}

	//! Init - virtual method
	virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit);
  bool Init(PPResources *resources, bool firstInit);
  //! Release resources used by effect
  virtual bool UnInit();
	//! Do - virtual method
	virtual void Do(bool isLast);

  //@{ implement SerializeClass interface
  //virtual bool IsDefaultValue(ParamArchive &ar) const {return false;}
  virtual LSError Serialize(ParamArchive &ar);
  virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
  //@}

  void Commit(float time) { _pInterp.Commit(time); }
  bool Committed() const {return _pInterp.Committed();}
  int SetParams(const AutoArray<float> &pars);
  int SetParams(const float* pars, int nPars);
};


//-----------------------------------------------------------------------------
/*
Parameters for color inversion post process effect
*/
//-----------------------------------------------------------------------------
struct ColorInversionPar
{
  typedef ColorInversionPar Par;

  //! image inversion (0..no blur, 1..very blurred)
  Color _invColor;

  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST(invColor);
  }
};

//-----------------------------------------------------------------------------
class EngineDD9::PPColorsInversion: public PPSimpleExt
{
private:
  typedef PPSimpleExt base;

protected:
  bool _initialized;
  //! post-process enabled (default is false)
  bool _enabled;
  // parameters
  PPEParsInterpolator<ColorInversionPar> _pInterp;

public:
  //! is effect enabled
  bool IsEnabled() { return _enabled; }
  //! enable/disable effect
  void Enable( bool enable ) { _enabled = enable; }
  //! set blur values
  void SetInversion(Color invColor) { _pInterp._pars._invColor = invColor; }
  //! set default values of params
  void SetDefaultValues() { SetInversion(Color(0,0,0,0)); }
  //! 
  bool Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit );
  bool Init(PPResources *resources, bool firstInit);
  //! release resources used by effect
  virtual bool UnInit();
  //! 
  virtual void Do(bool isLast);

  virtual LSError Serialize(ParamArchive &ar);
  virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}

  void Commit(float time) { _pInterp.Commit(time); }
  bool Committed() const {return _pInterp.Committed();}
  //! set effect parameters, if pars count is correct return 0, else return number of required pars
  int SetParams(const AutoArray<float> &pars);
  int SetParams(const float* pars, int nPars);
};

//-----------------------------------------------------------------------------
class EngineDD9::PPFXAA : public PostProcess
{
  typedef PostProcess base;

  ComRef<IDirect3DTexture9> _backRT;
  ComRef<IDirect3DSurface9> _backSurface;

  ComRef<IDirect3DTexture9> _backRT1;
  ComRef<IDirect3DSurface9> _backSurface1;

  VertexStaticData9::BufferFuture  _vb;
  ComRef<IDirect3DVertexDeclaration9> _vd;
  ComRef<IDirect3DVertexShader9> _vsFxaa;
  AutoArray<ComRef<IDirect3DPixelShader9> > _psFXAAs;
  ComRef<IDirect3DPixelShader9> _psFxaa;
  ComRef<IDirect3DPixelShader9> _psSharpen;
  
  //! effect allowed
  bool _enabled;
  //! 
  int _quality;

public:
  PPFXAA(): _enabled(false) {}

  //! Initialize resources
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs);
  //! Release resources used by effect
  virtual bool UnInit();
  //! Set effect resources
  virtual void Prepare();
  //! Perform post effect
  virtual void Do(bool isLast);
  //! is effect enabled
  bool IsEnabled() { return _enabled; }
  //! enable/disable effect
  virtual void Enable(bool enable) { _enabled = enable; }
  //!
  void SetQuality(int quality);
  //!
  int GetQuality() const;

  ComRef<IDirect3DTexture9> GetSharedRT0() const { return _backRT; }
  ComRef<IDirect3DTexture9> GetSharedRT1() const { return _backRT1; }
};

//-----------------------------------------------------------------------------
class EngineDD9::PPSMAA : public PostProcess
{
  typedef PostProcess base;

  enum Presets {  Low = 0, Medium, High, Ultra, /*Custom, */PresetCount };

  ComRef<IDirect3DTexture9> _backRT;
  ComRef<IDirect3DSurface9> _backSurface;
  ComRef<IDirect3DTexture9> _edgeRT;
  ComRef<IDirect3DSurface9> _edgeSurface;
  ComRef<IDirect3DTexture9> _blendRT;
  ComRef<IDirect3DSurface9> _blendSurface;

  ComRef<IDirect3DVertexDeclaration9> _vd;
  VertexStaticData9::BufferFuture  _vb;

  ComRef<IDirect3DTexture9> _areaTex;
  ComRef<IDirect3DTexture9> _searchTex;

  ComRef<IDirect3DVertexShader9> _edgeDetectionVS;
  AutoArray< ComRef<IDirect3DPixelShader9> > _edgeDetectionPS;

  ComRef<IDirect3DVertexShader9> _blendingWeightVS;
  AutoArray< ComRef<IDirect3DPixelShader9> > _blendingWeightPS;

  ComRef<IDirect3DVertexShader9> _neighborhoodBlendingVS;
  ComRef<IDirect3DPixelShader9> _neighborhoodBlendingPS;

  ComRef<IDirect3DSurface9> _depthSurface;

  bool _enabled;
  Presets _currentPreset; 

public:

  PPSMAA(): _enabled(false) {}
  ~PPSMAA() { UnInit(); }

  //! is effect enabled
  bool IsEnabled() { return _enabled; }
  //! enable/disable effect
  virtual void Enable(bool enable) { _enabled = enable; }
  //! Initialize resources
  virtual bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, ComRef<IDirect3DTexture9> &sharedRT0, ComRef<IDirect3DTexture9> &sharedRT1);
  //! Release resources
  virtual bool UnInit();
  //! Perform post effect
  virtual void Do(bool isLast);
  //! Effect quality
  void SetQuality(int quality);
};


//-----------------------------------------------------------------------------
/*
parameters for film grain
*/
//-----------------------------------------------------------------------------
struct SSAOPar
{
  typedef SSAOPar Par;

  float m_fIntensity;
  float m_fNearPixelRadius;
  float m_fFarPixelRadius;
	float m_fCosAngle1;
	float m_fCosAngle0;
	float m_fMaxDistance;
  float m_fMaxBlurDistance;
  
	float m_fBlurPasses;
	float m_fDepthBlurPasses;
	float m_fSSAOPasses;

  bool  m_bHalfRes;
  bool  m_bBlurHalfRes;

  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST_ENF(m_fIntensity);
    PAR_MEMBER_LIST_ENF(m_fNearPixelRadius);
    PAR_MEMBER_LIST_ENF(m_fFarPixelRadius);
    PAR_MEMBER_LIST_ENF(m_fCosAngle1);
    PAR_MEMBER_LIST_ENF(m_fCosAngle0);
    PAR_MEMBER_LIST_ENF(m_fMaxDistance);
    PAR_MEMBER_LIST_ENF(m_fMaxBlurDistance);
    PAR_MEMBER_LIST_ENF(m_fBlurPasses);
    PAR_MEMBER_LIST_ENF(m_fDepthBlurPasses);
    PAR_MEMBER_LIST_ENF(m_fSSAOPasses);

    PAR_MEMBER_LIST_ENF(m_bHalfRes);
    PAR_MEMBER_LIST_ENF(m_bBlurHalfRes);
  }

  LSError Serialize(ParamArchive &ar)
  {
    CHECK(ar.Serialize("Intensity", m_fIntensity, 0, 20.0f));
    CHECK(ar.Serialize("NearPixelRadius", m_fNearPixelRadius, 0, 60));
    CHECK(ar.Serialize("FarPixelRadius", m_fFarPixelRadius, 0, 5));
    CHECK(ar.Serialize("CosAngle1", m_fCosAngle1, 0, 0.8));
    CHECK(ar.Serialize("CosAngle0", m_fCosAngle0, 0, 0.2));
    CHECK(ar.Serialize("MaxDistance", m_fMaxDistance, 0, 1.5));
    CHECK(ar.Serialize("MaxBlurDistance", m_fMaxBlurDistance, 0, 0.2));
    CHECK(ar.Serialize("BlurPasses", m_fBlurPasses, 0, 0));
    CHECK(ar.Serialize("DepthBlurPasses", m_fDepthBlurPasses, 0, 3));
		CHECK(ar.Serialize("SSAOPasses", m_fSSAOPasses, 0, 3));
    CHECK(ar.Serialize("HalfRes", m_bHalfRes, 0, false));
    CHECK(ar.Serialize("BlurHalfRes", m_bBlurHalfRes, 0, false));
		
    return LSOK;
  }
  
  void SetHighestSetting()
  {
		m_fIntensity				= 20.0f;
		m_fNearPixelRadius	= 60.0f;
		m_fFarPixelRadius		= 5.0f;
		m_fCosAngle1				= 1.0f;
		m_fCosAngle0				= 0.1f;
		m_fMaxDistance			= 1.5f;
		m_fMaxBlurDistance	= 0.2f;

		m_fBlurPasses				= 1;
		m_fDepthBlurPasses	= 2;
		m_fSSAOPasses				= 3;
		m_bHalfRes					= false;
		m_bBlurHalfRes			= false;
  }

  void SetHighSetting()
  {
		m_fIntensity				= 20.0f;
		m_fNearPixelRadius	= 50.0f;
		m_fFarPixelRadius		= 5.0f;
		m_fCosAngle1				= 1.0f;
		m_fCosAngle0				= 0.1f;
		m_fMaxDistance			= 1.5f;
		m_fMaxBlurDistance	= 0.2f;

		m_fBlurPasses				= 1;
		m_fDepthBlurPasses	= 2;
		m_fSSAOPasses				= 3;
		m_bHalfRes					= true;
		m_bBlurHalfRes			= false;
  }

  void SetMediumSetting()
  {
		m_fIntensity				= 25.0f;
		m_fNearPixelRadius	= 50.0f;
		m_fFarPixelRadius		= 5.0f;
		m_fCosAngle1				= 1.0f;
		m_fCosAngle0				= 0.1f;
		m_fMaxDistance			= 1.5f;
		m_fMaxBlurDistance	= 0.2f;

		m_fBlurPasses				= 3;
		m_fDepthBlurPasses	= 0;
		m_fSSAOPasses				= 3;
		m_bHalfRes					= true;
		m_bBlurHalfRes			= false;
  }
 
  void SetLowSetting()
  {
		m_fIntensity				= 25.0f;
		m_fNearPixelRadius	= 50.0f;
		m_fFarPixelRadius		= 5.0f;
		m_fCosAngle1				= 1.0f;
		m_fCosAngle0				= 0.1f;
		m_fMaxDistance			= 1.5f;
		m_fMaxBlurDistance	= 0.2f;

		m_fBlurPasses				= 2;
		m_fDepthBlurPasses	= 0;
		m_fSSAOPasses				= 2;

		m_bHalfRes					= true;
		m_bBlurHalfRes			= true;
  }
  
  SSAOPar()
  {
    SetHighestSetting();
  } 
};


//-----------------------------------------------------------------------------
/*!
resources
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPSSAOResources: public PPResources
{
  friend class EngineDD9::PPSSAO;

  private:
    int	  _texWidth;
    int   _texHeight;

    int   _SSAOWidth;
    int   _SSAOHeight;

    int   _blurWidth;
    int   _blurHeight;

    VertexStaticData9::BufferFuture  _vb;
    ComRef<IDirect3DVertexDeclaration9> _vd;

    ComRef<IDirect3DVertexShader9> _vsSSAO;
    ComRef<IDirect3DVertexShader9> _vsSSAODown;
		ComRef<IDirect3DVertexShader9> _vsSSAOBlur;

		static const int MIN_SSAO_PASSES		= 2;
		static const int MAX_SSAO_PASSES		= 8;
		static const int SSAO_PASSES_COUNT	= MAX_SSAO_PASSES - MIN_SSAO_PASSES + 1;

    ComRef<IDirect3DPixelShader9>	_psSSAO[SSAO_PASSES_COUNT];
    ComRef<IDirect3DPixelShader9> _psBlurHorz;
    ComRef<IDirect3DPixelShader9>	_psBlurVert;
    ComRef<IDirect3DPixelShader9> _psBlurHorzDepth;
    ComRef<IDirect3DPixelShader9>	_psBlurVertDepth;
    ComRef<IDirect3DPixelShader9>	_psSSAOFinal;
    ComRef<IDirect3DPixelShader9> _psSSAODebug;
    ComRef<IDirect3DPixelShader9> _psSSAODown;

    ComRef<IDirect3DTexture9> _rtSSAO;
    ComRef<IDirect3DSurface9> _rtsSSAO;

    ComRef<IDirect3DTexture9> _rtBlur;
    ComRef<IDirect3DSurface9> _rtsBlur;

    ComRef<IDirect3DTexture9> _rtTemp;
    ComRef<IDirect3DSurface9> _rtsTemp;

  public:
    void UnInit()
    {
      _vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
      _vd.Free();

      _vsSSAO.Free();
      _vsSSAODown.Free();

			for (int i = 0; i < SSAO_PASSES_COUNT; i++)
				_psSSAO[i].Free();

      _psBlurHorz.Free();
      _psBlurVert.Free();
      _psBlurHorzDepth.Free();
      _psBlurVertDepth.Free();
      _psSSAOFinal.Free();
      _psSSAODebug.Free();
      _psSSAODown.Free();

      _rtSSAO.Free();
      _rtsSSAO.Free();

      _rtBlur.Free();
      _rtsBlur.Free();

      _rtTemp.Free();
      _rtsTemp.Free();
    }
};

//-----------------------------------------------------------------------------
/*!
SSAO effect
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPSSAO: public PostProcess
{
  private:
    typedef PostProcess base;
    friend class PPSSAOResources;

    struct Vertex
    {
       Vector3P pos;
       UVPair t0;
    };

    bool _bHalfResOld;
    bool _bBlurHalfResOld;
    bool _bDebug;

    PPEParsInterpolator<SSAOPar> _pInterp;
    SSAOPar _SSAOparams;
    Ref<PPResources> _resrc;
    bool  _enabled;

    bool SetRT(IDirect3DDevice9 *device, PPSSAOResources *res, bool halfres, bool blurhalfres);

  public:
    bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit);
    bool Init(PPResources *resources, bool firstInit);
    void Do(bool isLast) {}
    void Do(float fogEnd, float fogStart, float fogCoef);
    bool UnInit();
		void Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs);

    int SetParams(const float *pars, int nPars, bool imm);
    void SetDefaultValues();
    virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
    LSError Serialize( ParamArchive &ar );

    bool IsEnabled()        { return _enabled; }
    bool Enabled()					{ return _enabled; }
    void Enable(bool enable)	{ _enabled = enable; }
    bool IsActive()				{ return Enabled() && _pInterp._pars.m_fIntensity > 0.01f; }

    void Commit(float time) { _pInterp.Commit(time); }
    bool Committed() const {return _pInterp.Committed();}
    int SetParams(const AutoArray<float> &pars); 

    void SetQuality(int quality);

		SETGET_SC(Intensity, m_fIntensity)
		SETGET_SC(NearPixelRadius, m_fNearPixelRadius)
		SETGET_SC(FarPixelRadius, m_fFarPixelRadius)
		SETGET_SC(CosAngle1, m_fCosAngle1)
		SETGET_SC(CosAngle0, m_fCosAngle0)
		SETGET_SC(MaxDistance, m_fMaxDistance)
		SETGET_SC(MaxBlurDistance, m_fMaxBlurDistance)
		SETGET_SC(BlurPasses, m_fBlurPasses)
		SETGET_SC(DepthBlurPasses, m_fDepthBlurPasses)
		SETGET_SC(SSAOPasses, m_fSSAOPasses)
		
		SETGET_BOOL(HalfRes, m_bHalfRes)
		SETGET_BOOL(BlurHalfRes, m_bBlurHalfRes)
     
    const PPSSAOResources* Resources() { return static_cast<const PPSSAOResources *> (_resrc->GetType()); }
	  void CollectParams() { _SSAOparams = _pInterp.Simulate(); }
};


//-----------------------------------------------------------------------------
/*
parameters for film grain
*/
//-----------------------------------------------------------------------------
struct FilmGrainPar
{
  typedef FilmGrainPar Par;

  float _intensity;
  float _sharpness;
  float _grainSize;
  float _intensityX0;
  float _intensityX1;
  bool  _monochromatic;

  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST(intensity);
    PAR_MEMBER_LIST(sharpness);
    PAR_MEMBER_LIST(grainSize);
    PAR_MEMBER_LIST(intensityX0);
    PAR_MEMBER_LIST(intensityX1);

    PAR_MEMBER_LIST(monochromatic);
  }

  LSError Serialize(ParamArchive &ar)
  {
    CHECK(ar.Serialize("intensity", _intensity, 0, 0.005f));
    CHECK(ar.Serialize("sharpness", _sharpness, 0, 1.25f));
    CHECK(ar.Serialize("grainSize", _grainSize, 0, 2.01f));
    CHECK(ar.Serialize("intensityX0", _intensityX0, 0, 0.75f));
    CHECK(ar.Serialize("intensityX1", _intensityX1, 0, 1.00f));

    CHECK(ar.Serialize("monochromatic", _monochromatic, 0, true));

    return LSOK;
  }
  
  FilmGrainPar()
  {
    _intensity = 0;
		_sharpness	= 0;
		_grainSize	= 0;
		_intensityX0	= 0;
		_intensityX1	= 0;
		_monochromatic = true;
  }
};

//-----------------------------------------------------------------------------
class EngineDD9::PPFilmGrainResources: public PPResources
{
  friend class EngineDD9::PPFilmGrain;

private:
  float	_invtexWidth;
  float	_invtexHeight;

  int	_texWidth;
  int   _texHeight;

  VertexStaticData9::BufferFuture  _vb;
  ComRef<IDirect3DVertexDeclaration9> _vd;
  ComRef<IDirect3DVertexShader9>  _vs;
  ComRef<IDirect3DPixelShader9>		_psColor;
  ComRef<IDirect3DPixelShader9>		_psMono;

public:
  PPFilmGrainResources() : _vb(VertexStaticData9::BufferFuture::Null) {}
  void UnInit()
  {
    _vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
    _vd.Free();
    _vs.Free();
    _psColor.Free();
    _psMono.Free();
  }
};

//-----------------------------------------------------------------------------
/*!
Film grain effect
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPFilmGrain: public PostProcess
{
private:
  typedef PostProcess base;
  friend class PPFilmGrainResources;

  struct Vertex
  {
    Vector3P pos;
    UVPair t0;
  };

  PPEParsInterpolator<FilmGrainPar> _pInterp;
  // IUnknown stuff
  Ref<PPResources> _resrc;
  bool  _enabled;

public:
  bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit);
  bool Init(PPResources *resources, bool firstInit);
  void Do(bool isLast);
  bool UnInit();

  int SetParams(const float *pars, int nPars);
  void SetDefaultValues();
  virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
  LSError Serialize( ParamArchive &ar );

  bool IsEnabled()					{ return _enabled; }
  void Enable(bool enable)	{ _enabled = enable; }
  bool IsActive()				{ return IsEnabled() && _pInterp._pars._intensity > 0.00001f; }

  void Commit(float time) { _pInterp.Commit(time); }
  bool Committed() const {return _pInterp.Committed();}
  int SetParams(const AutoArray<float> &pars); 

  void setIntensity(float i)        { saturate(i, 0.0f, 1.0f); _pInterp._pars._intensity = i; }
  void setSharpness(float s)        { saturate(s, 0.f, 20.0f); _pInterp._pars._sharpness= s; }
  void setGrainSize(float g)        { saturate(g, 1.0f, 8.0f); _pInterp._pars._grainSize = g; }
  void setIntensityX0(float i)      { _pInterp._pars._intensityX0 = i;  }
  void setIntensityX1(float i)      { _pInterp._pars._intensityX1 = i;  }
  void SetMonochromatic(bool s)     { _pInterp._pars._monochromatic = s; }

  float getIntensity() const			{ return _pInterp._pars._intensity; }
  float getSharpness() const			{ return _pInterp._pars._sharpness; }
  float getGrainSize() const			{ return _pInterp._pars._grainSize; }
  float getIntensityX0() const			{ return _pInterp._pars._intensityX0; }
  float getIntensityX1() const			{ return _pInterp._pars._intensityX1; }
  bool  getMonochromatic() const	{ return _pInterp._pars._monochromatic; }

  const PPFilmGrainResources* Resources() { return static_cast<const PPFilmGrainResources *> (_resrc->GetType()); }
  // set VertexDecl, VertexShader & VertexBuffer
  void Prepare();
};


//-----------------------------------------------------------------------------
struct GodRaysPar
{
	typedef GodRaysPar Par;

	float _intensity;
	float _normSize;
	float _blurSize;
	float _overBurnIntensity;
	float _overBurnStart;
	float _overBurnEnd;

	int _blurPasses;

	/// list all members
	template <template <class T, class MT, class F> class Func, class Functor>
	void ForEachMember(Functor func)
	{
		PAR_MEMBER_LIST(intensity);
		PAR_MEMBER_LIST(normSize);
		PAR_MEMBER_LIST(blurSize);
		PAR_MEMBER_LIST(overBurnIntensity);
		PAR_MEMBER_LIST(overBurnStart);
		PAR_MEMBER_LIST(overBurnEnd);
		PAR_MEMBER_LIST(blurPasses);
	}

	LSError Serialize(ParamArchive &ar)
	{
		CHECK(ar.Serialize("intensity", _intensity, 0, 0.1f));
		CHECK(ar.Serialize("normSize", _normSize, 0, 0.5f));
		CHECK(ar.Serialize("blurSize", _blurSize, 0, 0.6f));
		CHECK(ar.Serialize("overBurnIntensity", _overBurnIntensity, 0, 0.25f));
		CHECK(ar.Serialize("overBurnStart", _overBurnStart, 0, 0.025f));
		CHECK(ar.Serialize("overBurnEnd", _overBurnEnd, 0, 0.175f));
		CHECK(ar.Serialize("blurPasses", _blurPasses, 0, 3));

		return LSOK;
	}

	void SetDefault()
	{
		_intensity				 = 0.1f;
		_normSize					 = 0.5f;
		_blurSize					 = 0.6f;
		_overBurnIntensity = 0.25f;
		_overBurnStart		 = 0.025f;
		_overBurnEnd			 = 0.175f;
		_blurPasses				 = 3;
	}

	GodRaysPar()
	{
		SetDefault();
	}
};

//-----------------------------------------------------------------------------
class EngineDD9::PPGodRaysResources: public PPResources
{
	friend class EngineDD9::PPGodRays;

private:
	VertexStaticData9::BufferFuture			_vb;
	ComRef<IDirect3DVertexDeclaration9> _vd;
	ComRef<IDirect3DVertexShader9>			_vs;
	ComRef<IDirect3DVertexShader9>			_vsShrink;
	ComRef<IDirect3DPixelShader9>				_psMask;
	ComRef<IDirect3DPixelShader9>				_psMaskNoHor;
	ComRef<IDirect3DPixelShader9>				_psShrink;
	ComRef<IDirect3DPixelShader9>				_psDiletate;
	ComRef<IDirect3DPixelShader9>				_psBlur;
	ComRef<IDirect3DPixelShader9>				_psFinal;

	ComRef<IDirect3DTexture9>	_rtMask;
	ComRef<IDirect3DSurface9> _rtsMask;
	ComRef<IDirect3DSurface9> _rtsMSAAMask;

	ComRef<IDirect3DTexture9>	_rtSmallMask[2];
	ComRef<IDirect3DSurface9> _rtsSmallMask[2];

	Ref<TextureD3D9> _kernelTexture;

public:
	PPGodRaysResources() : _vb(VertexStaticData9::BufferFuture::Null) {}
	void UnInit()
	{
		//TODO!!!
		_vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
		_vd.Free();
		_vs.Free();
	
		_vsShrink.Free();
		_psMask.Free();
		_psMaskNoHor.Free();
		_psDiletate.Free();
		_psShrink.Free();
		_psBlur.Free();
		_psFinal.Free();

		_rtMask.Free();
		_rtsMask.Free();
		_rtsMSAAMask.Free();

		_rtSmallMask[0].Free();
		_rtsSmallMask[0].Free();
		_rtSmallMask[1].Free();
		_rtsSmallMask[1].Free();
	}
	bool IsValid() const;
};

//-----------------------------------------------------------------------------
/*!
godrays effect
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPGodRays: public PostProcess
{
private:
	typedef PostProcess base;
	friend class PPGodRaysResources;

	struct Vertex
	{
		float x,y;
	};

	PPEParsInterpolator<GodRaysPar> _pInterp;
	Ref<PPResources> _resrc;
	bool _enabled;

	static const int _smallMaskLOD = 2;

	struct SmallRTIndex
	{
		private:
			int _full;
			int _free;

		public:
			void Advance()
			{
				_full = (_full + 1) & 1;
				_free = (_full + 1) & 1;
				Assert(_full != _free);
			}

			int FullRT() {return _full;} 
			int FreeRT() {return _free;} 
			SmallRTIndex(): _full(0), _free(1) {}
	} rtIndex;

	void CreateDepthMask(Vector3Par sunCenter, float ratio, float normSize);
	void ShrinkDepthMask(float invWidth, float invHeight);
	void DiletateDepthMask(float invWidth, float invHeight);
	void BlurDepthMask(Vector3Par projScreen, float width, float height, float mult, const GodRaysPar& pars);

public:
	bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit);
	bool Init(PPResources *resources, bool firstInit);
	void Do(bool isLast);
	bool UnInit();
	void Cleanup();
	void Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs);

	void SetTexture(TextureD3D9 *tex);

	int SetParams(const float *pars, int nPars, bool imm = false);
	void SetDefaultValues();
	virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
	LSError Serialize( ParamArchive &ar );

	bool IsEnabled()					{ return _enabled; }
	void Enable(bool enable)	{ _enabled = enable; }
	bool IsActive()						{ return IsEnabled() && _pInterp._pars._intensity > 0.001f; }

	void Commit(float time) { _pInterp.Commit(time); }
	bool Committed() const {return _pInterp.Committed();}
	int SetParams(const AutoArray<float> &pars); 

	SETGET_SC_CLAMP(Intensity, _intensity, 0.0f, 1.0f)
	SETGET_SC_CLAMP(NormSize, _normSize, 0.0f, 1.0f)
	SETGET_SC_CLAMP(BlurSize, _blurSize, 0.0f, 1.0f)
	SETGET_SC_CLAMP(OverBurnIntensity, _overBurnIntensity, 0.0f, 1.0f)
	SETGET_SC_CLAMP(OverBurnStart, _overBurnStart, 0.0f, 1.0f)
	SETGET_SC_CLAMP(OverBurnEnd, _overBurnEnd, 0.0f, 1.0f)

	SETGET_INT(BlurPasses, _blurPasses)

	const PPGodRaysResources* Resources() { return static_cast<const PPGodRaysResources *> (_resrc->GetType()); }
	// set VertexDecl, VertexShader & VertexBuffer
	void Prepare();
};


//-----------------------------------------------------------------------------
struct RainPar
{
	typedef RainPar Par;

	float _intensity;				// intensity of effect -> alpha blend <0, 1>
	float _luminance;				// luminance of effect, this should be remake to color
	float _power;						// power of effect -> number of primitives drawn <0, 1>
	float _radius;					// radius in meters around player
	float _speed;						// speed of rain fall
	float _width;						// width if one drop 
	float _height;					// height of one drop
	float _randMove;				// random move in xz direction
	float _randSpeed;				// random speed of one rain drop, 0 = the same speed for every drop, 1 = speed from <0, _speed>
	float _randGrid;				// grid size for noise texture when random move is enabled
	float _distWidthMul;		// width multiplier for father drops to have them more visible
	float _sunLumIntensity;	// if bigger > 0 the drops by sun are more visible, if < 0 then on opposite side of sun

	int _texDrops;

	Vector3 _dir;

	/// list all members
	template <template <class T, class MT, class F> class Func, class Functor>
	void ForEachMember(Functor func)
	{
		PAR_MEMBER_LIST(intensity);
		PAR_MEMBER_LIST(luminance);
		PAR_MEMBER_LIST(power);
		PAR_MEMBER_LIST(radius);
		PAR_MEMBER_LIST(speed);
		PAR_MEMBER_LIST(width);
		PAR_MEMBER_LIST(height);
		PAR_MEMBER_LIST(randMove);
		PAR_MEMBER_LIST(randSpeed);
		PAR_MEMBER_LIST(randGrid);
		PAR_MEMBER_LIST(distWidthMul);
		PAR_MEMBER_LIST(sunLumIntensity);
		PAR_MEMBER_LIST(dir);

		PAR_MEMBER_LIST(texDrops);
	}

	LSError Serialize(ParamArchive &ar)
	{
		CHECK(ar.Serialize("intensity", _intensity, 0, 30.0f));
		CHECK(ar.Serialize("luminance", _luminance, 0, 0.15f));
		CHECK(ar.Serialize("power", _power, 0, 0.5f));
		CHECK(ar.Serialize("radius", _radius, 0, 50.0f));
		CHECK(ar.Serialize("speed", _speed, 0, 0.1f));
		CHECK(ar.Serialize("width", _width, 0, 0.025));
		CHECK(ar.Serialize("height", _height, 0, 0.1f));
		CHECK(ar.Serialize("randMove", _randMove, 0, 0.1f));
		CHECK(ar.Serialize("randSpeed", _randSpeed, 0, 0.5f));
		CHECK(ar.Serialize("randGrid", _randGrid, 0, 0.0f));
		CHECK(ar.Serialize("distWidthMul", _distWidthMul, 0, 2.0f));
		CHECK(ar.Serialize("sunLumIntensity", _sunLumIntensity, 0, 0.1f));

		//CHECK(ar.Serialize("dir", _dir, 0, Vector3(0.0f, -1.0f, 0.0f)));
	
		CHECK(ar.Serialize("texDrops", _texDrops, 0, 4));

		return LSOK;
	}

	void SetDefault()
	{
		_intensity				= 30.0f;
		_luminance				= 0.2f;
		_power						= 1.0f;
		_radius						= 15.0f;
		_speed						= 20.0f;
		_width						= 0.01;
		_height						= 0.4f;
		_randMove					= 0.2f;
		_randSpeed				= 0.2f;
		_randGrid					= 0.15f;
		_distWidthMul			= 2.0f;
		_sunLumIntensity	= 0.1f;
		_dir							= Vector3(0.25f, -1.0f, 0.0f);

		_texDrops				= 4;
	}

	RainPar()
	{
		SetDefault();
	}
};


//-----------------------------------------------------------------------------
class EngineDD9::PPRainResources: public PPResources
{
	friend class EngineDD9::PPRain;

private:
	//3d rain 
	VertexStaticData9::BufferFuture			_vb;
	IndexStaticData9::BufferFuture		  _ib;
	ComRef<IDirect3DVertexDeclaration9> _vd;
	ComRef<IDirect3DVertexShader9>			_vs;
	ComRef<IDirect3DPixelShader9>				_ps;

	//rain layer
	VertexStaticData9::BufferFuture			_vbLayer;
	ComRef<IDirect3DVertexDeclaration9> _vdLayer;
	ComRef<IDirect3DVertexShader9>			_vsLayer;
	ComRef<IDirect3DPixelShader9>				_psLayer;

	Ref<TextureD3D9>	_rainTexture;
	int								_numOfDrops;

public:
	PPRainResources() : _vb(VertexStaticData9::BufferFuture::Null) {}

	void UnInit()
	{
		_vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
		_ib = IndexStaticData9::BufferFuture(IndexStaticData9::BufferFuture::Null);
		_vd.Free();
		_vs.Free();
		_ps.Free();
	}
};


class CollisionGrid;
class RainDropsSimulator;

//-----------------------------------------------------------------------------
class EngineDD9::PPRain: public PostProcess
{
private:
	typedef PostProcess base;
	friend class PPRainResources;

	static const int MaxNumOfDrops = 40000;

	//todo, better vertex format
	struct Vertex
	{
		unsigned short p[3];
		unsigned short rand;

		unsigned short t[2];
	};

	//layer vertex format
	struct VertexLayer
	{
		Vector3P pos;
		float u, v;
	};

	PPEParsInterpolator<RainPar> _pInterp;
	RainPar _currentPars;

	Ref<PPResources> _resrc;
	bool _enabled, _resNotReady;
	
	int _currNumOfDrops;
	float _actualCamCollision;
	float _globIntensity;
	float _globPower;

	AverageTimeValue	_collisionValue;
	RainDropsSimulator* m_DropsSimulator;

	float ComputePosIntensity(Vector3Par pos);
	float ComputeOcclusionIntensity();

	void RenderRain(const RainPar& pars, float occlusionIntensity);
	void RenderRainDrops(const RainPar& pars);
	void RenderRainLayer(const RainPar& pars);

	bool InitRain(const LazyLoadFile &hlsl, const LazyLoadFile &vs, Ref<PPRainResources> &resrc);
	bool InitLayer(const LazyLoadFile &hlsl, const LazyLoadFile &vs, Ref<PPRainResources> &resrc);

public:
	bool Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit);
	bool Init(PPResources *resources, bool firstInit);
	void Do(bool isLast);
	bool UnInit();
	void Cleanup();
	void Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs);

	void SetTexture(TextureD3D9 *tex);

	int SetParams(const float *pars, int nPars, bool imm = false);
	void SetDefaultValues();
	virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}
	LSError Serialize( ParamArchive &ar );

	bool IsEnabled()					{ return _enabled; }
	void Enable(bool enable)	{ _enabled = enable; }
	bool IsActive()						{ return IsEnabled() && _pInterp._pars._intensity > 0.001f; }

	void Commit(float time) { _pInterp.Commit(time); }
	bool Committed() const {return _pInterp.Committed();}
	int SetParams(const AutoArray<float> &pars); 

	void SetGlobalPower(float power)
	{
		_globPower = floatMinMax(power, 0.0, 1.0f);
	}
	void SetGlobalIntensity(float intensity)
	{
		_globIntensity = floatMinMax(intensity, 0.0, 1.0f);
	}

	const PPRainResources* Resources() { return static_cast<const PPRainResources *> (_resrc->GetType()); }
	void Prepare(float deltaT);

	SETGET_SC(Intensity, _intensity);
	SETGET_SC(Luminance, _luminance);
	SETGET_SC_CLAMP(Power, _power, 0.0f, 1.0f);
	SETGET_SC(Radius, _radius);
	SETGET_SC(Speed, _speed);
	SETGET_SC(Width, _width);
	SETGET_SC(Height, _height);
	SETGET_SC(RandMove, _randMove);
	SETGET_SC_CLAMP(RandSpeed, _randSpeed, 0.0f, 1.0f);
	SETGET_SC_CLAMP(RandGrid, _randGrid, 0.0f, 1.0f);
	SETGET_SC(DistWidthMul, _distWidthMul);
	SETGET_SC(SunLumIntensity, _sunLumIntensity);

	SETGET_VEC(Dir, _dir);
	SETGET_INT(TexDrops, _texDrops);
};


//-----------------------------------------------------------------------------
/*
  Parameters for dynamic blur post process effect
*/
//-----------------------------------------------------------------------------
struct DynamicBlurPar
{
  typedef DynamicBlurPar Par;
  
  //! image blurriness (0..no blur, 10..very blurred)
  float _blurriness;

  /// list all members
  template <template <class T, class MT, class F> class Func, class Functor>
  void ForEachMember(Functor func)
  {
    PAR_MEMBER_LIST(blurriness);
  }

};

class EngineDD9::PPDynBlurResources: public PPResources
{
  friend class EngineDD9::PPDynamicBlur;

protected:
  //! screen plane vertex buffer
  VertexStaticData9::BufferFuture _vb;
  //! screen plane vertex declaration
  ComRef<IDirect3DVertexDeclaration9> _vd;
  //! shaders
  ComRef<IDirect3DVertexShader9> _vsBlur;
  ComRef<IDirect3DPixelShader9> _psBlur;
  ComRef<IDirect3DVertexShader9> _vsFinal;
  ComRef<IDirect3DPixelShader9> _psFinal;
  //! num verts in screen plane
  int _vertexCount;
  //! num tris in triangle strip
  int _stripSize;

public:
  PPDynBlurResources() : _vb(VertexStaticData9::BufferFuture::Null) {}
  void UnInit()
  {
    _vb = VertexStaticData9::BufferFuture(VertexStaticData9::BufferFuture::Null);
    _vd.Free();
    _vsBlur.Free();
    _psBlur.Free();
    _vsFinal.Free();
    _psFinal.Free();
  }
};

//-----------------------------------------------------------------------------
/*!
dynamic blur effect 
*/
//-----------------------------------------------------------------------------
class EngineDD9::PPDynamicBlur: public PostProcess//, public SerializeClass
{
  friend class EngineDD9::PPDynBlurResources;

private:
  typedef PostProcess base;
protected:
  //! reference to GaussianBlur post process
  Ref<PPGaussianBlur> _ppGaussianBlur;
  // IUnknown stuff
  Ref<PPResources> _resrc;
  bool _initialized;

  //! post-process enabled (default is false)
  bool _enabled;
  // parameters
  PPEParsInterpolator<DynamicBlurPar> _pInterp;
  
public:
  //! is effect enabled
  bool IsEnabled() { return _enabled; }
  //! enable/disable effect
  void Enable( bool enable ) { _enabled = enable; }
  //! is active = enabled && non default params
  bool IsActive()
  {
    if( IsEnabled() )
      return true;
    else
      return false;
  }
  //! set blur values
  void SetBlurrines(float blurrines) { _pInterp._pars._blurriness = blurrines; }
  //! set default values of params
  void SetDefaultValues() { SetBlurrines(0.0f); }
  //! 
  bool Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, PPGaussianBlur *ppGaussianBlur, bool firstInit );
  bool Init(PPResources *resources, PPGaussianBlur *ppGaussianBlur, bool firstInit);
  //! Release resources used by effect
  virtual bool UnInit();
  //! 
  virtual void Do( bool isLast );

  virtual LSError Serialize(ParamArchive &ar);
  virtual void LoadDefaultValues(ParamArchive &ar) {SetDefaultValues();}

  void Commit(float time) { _pInterp.Commit(time); }
  bool Committed() const {return _pInterp.Committed();}
  //! set effect parameters, if pars count is correct return 0, else return number of required pars
  int SetParams(const AutoArray<float> &pars);
  int SetParams(const float* pars, int nPars);
  const PPDynBlurResources* Resources() { return static_cast<const PPDynBlurResources*> (_resrc->GetType()); }
};

//-----------------------------------------------------------------------------
class EngineDD9::ISpecialEffect: public RefCount, public SerializeClass
{
protected:
  //! Engine
  static EngineDD9* _engine;
  //! PPE handles
  AutoArray<int> _hndls;
  //! PPE handles validity
  bool _handlesValid;
  //! Time to next update
  Time _timeStamp;
  //! Time of last change
  Time _lastUpdate;
  bool _active;

public:
  ISpecialEffect(): _handlesValid(false) {}
  ~ISpecialEffect() { DestroyHndls(); }

public:
  static void SetEngine(EngineDD9 *engine) {_engine = engine;}
  virtual bool Init() = 0;
  virtual void Update() = 0;
  virtual void SetParams(const float *pars, int nPars) = 0;
  virtual LSError Serialize(ParamArchive &ar);

public:
  void DestroyHndls();
  void Reset();
};

//-----------------------------------------------------------------------------
/*!
Soldier - hit
*/
//-----------------------------------------------------------------------------
class EngineDD9::HitSpecEffect: public ISpecialEffect
{
  typedef ISpecialEffect base;

private:
  enum { DBPriority = 410 };

public:
  bool Init();
  void SetParams(const float *pars, int nPars);
  void Update();
};

//-----------------------------------------------------------------------------
/*!
Soldier movement - most noticeable in sprint
*/
//-----------------------------------------------------------------------------
class EngineDD9::MovementSpecEffect: public ISpecialEffect
{
  typedef ISpecialEffect base;

private:
  AutoArray<float> _pars;
  enum { RBPriority = 120 };

public:
  bool Init();
  void SetParams(const float *pars, int nPars);
  void Update();
};

//-----------------------------------------------------------------------------
/*!
Soldier tired
*/
//-----------------------------------------------------------------------------
class EngineDD9::TiredSpecEffect: public ISpecialEffect
{
  typedef ISpecialEffect base;

private:
  AutoArray<float> _pars;
  AutoArray<float> _parsCa;
  enum { WPriority = 310, CCPriority = 1510 };

public:
  bool Init();
  void SetParams(const float *pars, int nPars);
  void Update();
};

