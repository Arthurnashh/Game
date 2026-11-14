#ifndef _postProcessT_hpp_
#define _postProcessT_hpp_

#if !defined _RESISTANCE_SERVER && _ENABLE_DX10

#include "engddT.hpp"

//! Abstract ancestor of post processes
class EngineDDT::PostProcess : public RefCount
{
protected:
  //! Engine
  static EngineDDT *_engine;
  //! Preparing of the post process (called each time in Do)
  virtual void Prepare();


  /// helper function for pixel shader compilation
  bool CompilePixelShader(
    const QOStrStream &hlsl, const char *psName, ComRef<ID3D10PixelShader> &ps,
    bool fullPrec=false
  ) const;
  /// helper function for vertex shader compilation
  bool CompileVertexShader(
    const QOStrStream &hlsl, const char *vsName, ComRef<ID3D10VertexShader> &vs
  ) const;
  
public:
  //! Setting of the engine pointer
  static void SetEngine(EngineDDT *engine) {_engine = engine;}
  //! Initialization of the post process
  virtual bool Init(const QOStrStream &hlsl, const QOStrStream &vs) {return true;};
  //! Applying of post process effect
  /*!
    @param isLast when true, no more postprocess effects will be called - if possible, postprocess to backbuffer
  */
  virtual void Do(bool isLast) = 0;
  /// check if the postprocess pass will be executed
  /** some effects sometimes do not execute, depending on parameters */
  virtual bool WillBeDone() const {return true;}
  /// set effect specific parameters
  virtual void SetParameters(const float *pars, int nPars) {}
};

//! Abstract ancestor of simple post processes
class EngineDDT::PPSimple : public PostProcess
{
private:
  typedef PostProcess base;
protected:
  struct Vertex
  {
    Vector3P pos;
    UVPair t0;
  };
  ComRef<IDirect3DVertexBuffer9Old> _vb;
  ComRef<ID3D10InputLayout> _vd;
  ComRef<ID3D10VertexShader> _vs;
  ComRef<ID3D10PixelShader> _ps;
  
  virtual void Prepare();
public:
  virtual bool Init(const QOStrStream &hlsl, const QOStrStream &vs, const char *vsName);
};

//! Final - copying of source to back buffer
class EngineDDT::PPFinal : public PPSimple
{
private:
  typedef PPSimple base;
public:
  virtual bool Init(const QOStrStream &hlsl, const QOStrStream &vs);
  virtual void Do(bool isLast);
};

//! Stencil shadows visualization
class EngineDDT::PPStencilShadowsPri : public PostProcess
{
  typedef PostProcess base;
protected:
  enum {NQualities=3};
  static int _nPlates[NQualities];
  struct Vertex
  {
    Vector3P pos;
    UVPair t0;
    PackedColor color;
  };
  ComRef<IDirect3DVertexBuffer9Old> _vb[NQualities];
  ComRef<ID3D10InputLayout> _vd;
  ComRef<ID3D10VertexShader> _vs;
  ComRef<ID3D10PixelShader> _ps;
  /// constant index
  static const int _shadowRampMulAddIndex = VSC_ShadowRampMulAdd;
  
  virtual void Prepare();

  enum Parameter
  {
    ParRampStart,
    ParRampEnd,
    ParQuality,
    NParameters
  };
  float _pars[NParameters];

protected:
  virtual int GetStencilMask() const {return STENCIL_COUNTER;}
  virtual int GetStencilRef() const {return 0;}
  virtual D3D10_COMPARISON_FUNC GetStencilFunc() const {return D3D10_COMPARISON_LESS;}
  
public:
  virtual bool Init(const QOStrStream &hlsl, const QOStrStream &vs);
  virtual void Do(bool isLast);
  virtual void SetParameters(const float *pars, int nPars);
};

//! Stencil shadows visualization - secondary shadows only
class EngineDDT::PPStencilShadowsSec : public PPStencilShadowsPri
{
  typedef PPStencilShadowsPri base;
  
  public:
  virtual int GetStencilMask() const {return STENCIL_PROJ_SHADOW|STENCIL_COUNTER;}
  virtual int GetStencilRef() const {return STENCIL_PROJ_SHADOW;}
  virtual D3D10_COMPARISON_FUNC GetStencilFunc() const {return D3D10_COMPARISON_EQUAL;}
};

//! Custom edge vertex structure
struct VertexCustomEdge
{
  Vector3P pos;
  float index;
};

//! Gaussian blur effect
/*!
  This effect is not applied directly to the output - it only calculates and holds
  blurred image for later use.
*/
class EngineDDT::PPGaussianBlur : public PostProcess
{
private:
  typedef PostProcess base;
protected:

  //!{ Custom edge resources
  ComRef<IDirect3DVertexBuffer9Old> _vbCustomEdge;
  ComRef<ID3D10InputLayout> _vdCustomEdge;
  ComRef<ID3D10VertexShader> _vsCustomEdge;
  //!}

  //!{ Horizontal blur resources
  ComRef<ID3D10PixelShader> _psHorizontal;
  ComRef<ID3D10Texture2D> _rtHorizontal;
  // Direct3D 10 REMOVED 
//   ComRef<IDirect3DSurface9> _rtsHorizontal;
  //!}

  //!{ Final blur resources
  ComRef<ID3D10PixelShader> _psBlur;
  ComRef<ID3D10Texture2D> _rtBlur;
  // Direct3D 10 REMOVED 
//   ComRef<IDirect3DSurface9> _rtsBlur;
  //!}

public:
  virtual bool Init(const QOStrStream &hlsl, const QOStrStream &vs);
  virtual void Do(bool isLast);
  //! Function to retrieve the blurred texture
  ComRef<ID3D10Texture2D> GetBlurredTexture() const {return _rtBlur;}
};

//! Field of view effect
class EngineDDT::PPDOF : public PPSimple
{
private:
  typedef PPSimple base;
protected:
  //! Reference to post process with a texture we require here in this post process
  Ref<PPGaussianBlur> _ppGaussianBlur;
public:
  //! Non-virtual version of init
  bool Init(const QOStrStream &hlsl, const QOStrStream &vs, const Ref<PPGaussianBlur> &ppGaussianBlur);
  virtual void Do(bool isLast);
};

//! Field of view for undefined fecal plane case (we blur only from focal plane distance further)
class EngineDDT::PPDistanceDOF : public PPDOF
{
private:
  typedef PPDOF base;
  typedef PPSimple grandBase;
public:
  //! Non-virtual version of init
  bool Init(const QOStrStream &hlsl, const QOStrStream &vs, PPGaussianBlur *ppGaussianBlur);
};

//! Glow
class EngineDDT::PPGlow : public PostProcess
{
public:
  //! Method to cancel all queries
  void CancelQueries() {}
};

// Direct3D 10 NEEDS UPDATE 
// class EngineDDT::PPGlow : public PostProcess
// {
// private:
//   typedef PostProcess base;
// protected:
//   //! Reference to post process with a texture we require here in this post process
//   Ref<PPGaussianBlur> _ppGaussianBlur;
//   //@{ cascade of low-resolution render targets
//   /// 16 render targets is enough for more then 64Kx64K pixels
//   /** each pass performs 4x or 2x decimation */
//   enum {MaxDecimations=16};
//   struct DecimationPass
//   {
//     ComRef<ID3D10Texture2D> _rt;
//     ComRef<IDirect3DSurface9> _rts;
//     int _w,_h;
//   };
//   DecimationPass _dec[MaxDecimations];
//   // end of 4x passes (avg only)
//   int _decimation4x;
//   // end of last 2x passes (avg/min/max only) - index+1
//   int _decimation2x;
//   //@}
//   
//   //@{ 1x1 RT used for aperture control - double buffered
//   ComRef<ID3D10Texture2D> _rtAperture[2];
//   ComRef<IDirect3DSurface9> _rtsAperture[2];
//   int _rtApertureCurrent;
//   //@}
// 
//   #if _DEBUG
//   /// debugging - lockable sys.mem. surface
//   ComRef<IDirect3DSurface9> _rtsDebugAp;
//   ComRef<IDirect3DSurface9> _rtsDebugDec;
//   
//   Color GetContent(IDirect3DSurface9 *surf, IDirect3DSurface9 *temp, DXGI_FORMAT format);
//   Color GetContentAp(IDirect3DSurface9 *surf);
//   Color GetContentDec(IDirect3DSurface9 *surf);
//   #endif
// 
//   //@{ 1x1 RT used for measurements
//   ComRef<ID3D10Texture2D> _rtMeasure;
//   ComRef<IDirect3DSurface9> _rtsMeasure;
//   ComRef<IDirect3DSurface9> _rtsMeasureDepth;
//   //@}
//   
//   //@{ custom edge UV vertex shader + buffer + declaration
// 	ComRef<IDirect3DVertexBuffer9Old> _vbCustomEdge;
// 	ComRef<ID3D10InputLayout> _vdCustomEdge;
// 	ComRef<ID3D10VertexShader> _vsCustomEdge;
// 	
// 	struct EdgeParams
//   {
//     float u[4][4];
//     float v[4][4];
//     float posOffset[4];
//     
//     void SetupBilinear(int dstW, int dstH, int srcW, int srcH)
//     {
//       // for pos. offset explanation cf. DX docs:
//       // Directly Mapping Texels to Pixels 
//       // Texture Coordinates
//       // TODO: verify if this works well for 2x and 4x decimation case as well
//       posOffset[0] = 0; //-0.5f/dstW;
//       posOffset[1] = 0; //-0.5f/dstH;
//       posOffset[2] = posOffset[3] = 0;
//       // we may perform any decimation, like 2x or 4x decimation
//       int decimationW = srcW/dstW;
//       int decimationH = srcH/dstH;
//       
//       const float sampOffsetU = 1.0/srcW;
//       const float sampOffsetV = 1.0/srcH;
//       static const float baseU[4] = {0, 0, 1, 1};
//       static const float baseV[4] = {1, 0, 1, 0};
//       static const float nU[4] = {0, 0,  1, 1};
//       static const float nV[4] = {0, 1, 0,  1};
//       for (int vertex = 0; vertex < 4; vertex++)
//       {
//         for (int t = 0; t < 4; t++)
//         {
//           u[vertex][t] = baseU[vertex] + sampOffsetU * nU[t]*decimationW;
//           v[vertex][t] = baseV[vertex] + sampOffsetV * nV[t]*decimationH;
//         }
//       }
//     }
//     void SetupPoint(int dstW, int dstH, int srcW, int srcH)
//     {
//       posOffset[0] = 0; //-0.5f/dstW;
//       posOffset[1] = 0; //-0.5f/dstH;
//       posOffset[2] = posOffset[3] = 0;
//       // we may perform any decimation, like 2x or 4x decimation
//       int decimationW = srcW/dstW;
//       int decimationH = srcH/dstH;
//       const float sampOffsetU = 1.0/srcW;
//       const float sampOffsetV = 1.0/srcH;
//       static const float baseU[4] = {0, 0, 1, 1};
//       static const float baseV[4] = {1, 0, 1, 0};
//       static const float nU[4] = {0, 0,  1, 1};
//       static const float nV[4] = {0, 1, 0,  1};
//       for (int vertex = 0; vertex < 4; vertex++)
//       {
//         for (int t = 0; t < 4; t++)
//         {
//           // Cf. DX docs Nearest-Point Sampling for UV offset explanation
//           u[vertex][t] = baseU[vertex] + sampOffsetU * (nU[t]*decimationW + 0.5f);
//           v[vertex][t] = baseV[vertex] + sampOffsetV * (nV[t]*decimationH + 0.5f);
//         }
//       }
//     }
//   };
// 
//   //@}
//   
//   /// initial luminance calculations
//   ComRef<ID3D10PixelShader> _psLuminance;
//   /// average luminance down-sampling
//   ComRef<ID3D10PixelShader> _psAvgLuminance;
//   /// avg-min-max down-sampling
//   ComRef<ID3D10PixelShader> _psMaxAvgMinLuminance;
//   /// final luminace operator
//   ComRef<ID3D10PixelShader> _psAssumedLuminance;
//   /// brightness query pixel shader
//   ComRef<ID3D10PixelShader> _psQuery;
//   //! Pixel shader for glow
//   ComRef<ID3D10PixelShader> _psGlow;
//   //! Pixel shader for glow + night eye vision
//   ComRef<ID3D10PixelShader> _psGlowNight;
//   //! Pixel shader for glow + night vision (color transformation)
//   ComRef<ID3D10PixelShader> _psGlowNVG;
//   
//   /// Number of queries (width of the histogram)
//   static const int NBrightQueries = 16;
//   /// Manager of the queries
//   class QueryManager
//   {
//   protected:
//     static EngineDDT *_engine;
//     static const int NItems = 4;
//     ComRef<IDirect3DQuery9> _query[NItems][NBrightQueries];
//     /// record value for each query so that we know the absolute (HDR) meaning of the result
//     float _value[NItems][NBrightQueries];
//     /// for debugging purposes we may want to store one master value with each query set
//     float _masterValue[NItems];
//     /// new query to be issued
//     int _newQuery;
//     /// query which should be ready first
//     int _oldestQuery;
//     /// how many queries were issued
//     int _queriesIssued;
//     /// gather results as we get them
//     int _results[NBrightQueries];
//     /// index of the next result to come
//     int _currentResultIndex;
//     //@{ once results are finalized, copy them here
//     int _mostRecentResult[NBrightQueries];
//     float _mostRecentResultValue[NBrightQueries];
//     float _mostRecentMasterValue;
//     //@}
//     
//     int Next(int i) {return (i < NItems - 1) ? ++i : 0;}
//   public:
//     static void SetEngine(EngineDDT *engine) {_engine = engine;}
//     QueryManager();
//     //! Method to reset all queries
//     void ResetQueries();
//     bool Issue(const float *value, const float*alpha, float master);
//     float GetNumberOfPixelsDrawn(int *npd, float *value);
//   };
//   //! Queries
//   QueryManager _query;
//   
//   //@{ parameters
//   float _deltaT;
//   float _eyeAdaptMin;
//   float _eyeAdaptMax;
//   //@}
//   
// public:
//   static void SetEngine(EngineDDT *engine) {QueryManager::SetEngine(engine);}
//   virtual bool Init(const QOStrStream &hlsl, const QOStrStream &vs, PPGaussianBlur *blur);
//   virtual void Do(bool isLast);
//   
//   virtual void SetParameters(const float *pars, int nPars);
// 
//   //! Method to return aperture texture
//   ComRef<ID3D10Texture2D> GetApertureTexture() {return _rtAperture[_rtApertureCurrent];};
//   //! Method to return decimation texture
//   ComRef<ID3D10Texture2D> GetDecimationTexture() {return _dec[_decimation2x-1]._rt;};
//   //! Returns cones
//   float Cones() const;
//   //! Returns EyeSens coefficient
//   Color EyeSens(float cones) const;
//   //! Method to cancel all queries
//   void CancelQueries() {_query.ResetQueries();}
// };

//! Calculating of flare intensity into special texture
class EngineDDT::PPFlareIntensity : public PostProcess
{
private:
  typedef PostProcess base;
protected:
  //!{ Flare intensity resources
  const static int RenderTargetNum = 4;
  ComRef<ID3D10Texture2D> _rtLightValues[RenderTargetNum];
  // Direct3D 10 REMOVED 
  //ComRef<IDirect3DSurface9> _rtsLightValues[RenderTargetNum];
  //!}
public:
  virtual bool Init(const QOStrStream &hlsl, const QOStrStream &vs);
  virtual void Do(bool isLast) {};
  void Do(int x, int y, int sizeX, int sizeY);
};

#endif

#endif