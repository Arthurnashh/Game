//# pragma optimize( "", off ) 

#include "../wpch.hpp"
#if !defined _SERVER && !_DISABLE_GUI
#include "postProcess.hpp"
#include "engdd9.hpp"
#include "txtd3d9.hpp"
#include "../textbank.hpp"
#include "../diagModes.hpp"
#include <Es/Strings/bString.hpp>
#include "../Shape/shape.hpp"
#include "../appFrameExt.hpp"
#include "../global.hpp"
#include <El/Math/mathStore.hpp>
#include "../keyInput.hpp"
#include "../dikCodes.h"
#include "ShaderSources_3_0/common.h"
#include "../scene.hpp"
#include "../camera.hpp"

//because rain needs to cooperate with camera inside vehicle
#include "../world.hpp"
//...and sealevel
#include "../landscape.hpp"

#include <El/Evaluator/express.hpp>
#include <El/Modules/modules.hpp>
#include <El/Common/randomGen.hpp>
#include <El/Credits/credits.hpp>

#include "system/enf_timer.h"


#if _ENABLE_COMPILED_SHADER_CACHE
#include "ShaderCompileCache.h"
#endif

EngineDD9 *EngineDD9::PostProcess::_engine;
extern bool FULL_STENCIL;

#define SET_SC(name, i)		if (nPars > i)			set##name(pars[i], imm);
#define SET_BOOL(name, i)	if (nPars > i)			set##name(fabs(pars[i]) < 0.01f, imm);
#define SET_VEC(name, i)	if (nPars > i + 2)	set##name(Vector3(pars[i], pars[i+1], pars[i+2]), imm);


/*!
enable pp effect profiling, TODO: put to graph
*/
//#define ENABLE_GPU_PP_MEASURE

#ifdef ENABLE_GPU_PP_MEASURE
	#define GPU_START_PROFILE(device, name) { GPUMeasure m(device, true, name);
	#define GPU_END_PROFILE	}
#else
	#define GPU_START_PROFILE(device, name) 
	#define GPU_END_PROFILE	
#endif



//-----------------------------------------------------------------------------
/*!
class for simple GPU time measure 
*/
//-----------------------------------------------------------------------------
class GPUMeasure
{
private:
	IDirect3DQuery9*					m_pEvent;
	ComRef<IDirect3DDevice9>& m_pDevice;
	bool											m_bRunning;
	enf::CStr									m_pStr;

	//-----------------------------------------------------------------------------
	void WaitForGPU()
	{
		m_pEvent->Issue(D3DISSUE_END);
		while (m_pEvent->GetData( NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
			;
	}

public:
	//-----------------------------------------------------------------------------
	GPUMeasure(ComRef<IDirect3DDevice9>& device, bool immStart = true, enf::CStr name = NULL): m_pDevice(device), m_bRunning(false), m_pStr(name), m_pEvent(NULL)
	{
		if (immStart) 
			Start();
	}

	//-----------------------------------------------------------------------------
	~GPUMeasure()
	{
		if (m_bRunning)
			Stop(true);

		if (m_pEvent)
			m_pEvent->Release();
	}

	//-----------------------------------------------------------------------------
	void Start(enf::CStr name = NULL)
	{
		if (!m_pDevice || m_bRunning)
			return;

		if (!m_pEvent)
		{
			if (m_pDevice->CreateQuery(D3DQUERYTYPE_EVENT, NULL) != D3D_OK)
				return;

			m_pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pEvent);
		}

		if (!name.IsEmpty())
			m_pStr = name;

		m_bRunning = true;

		WaitForGPU();
		enf::Timer::PushTime();
	}

	//-----------------------------------------------------------------------------
	float Stop(bool log = true)
	{
		WaitForGPU();
		float time = enf::Timer::PopTime();

		if (log)
			LogF("%s%3.3f", m_pStr.IsEmpty() ? "Last GPU time: " : m_pStr.cstr(), time);

		m_bRunning = false;
		return time;
	}
};


//-----------------------------------------------------------------------------
class ProceduralTexture : protected RefCount
{
private:
	short			m_iWidth;
	short			m_iHeight;
	int				m_iMipMaps;
	D3DFORMAT m_Format;

	ComRef<IDirect3DTexture9> _texture;	

protected:
	virtual void Fill(const D3DSURFACE_DESC &desc, const D3DLOCKED_RECT& lRect, const D3DSURFACE_DESC &prevDesc, const D3DLOCKED_RECT& prevLRect) = 0;

public:
	ProceduralTexture(int width, int height, D3DFORMAT format, int mipmaps): m_Format(format)
	{
		m_iWidth  = intMax(width, 1);
		m_iHeight = intMax(height, 1);

		int m = intMax(enf::GetHighestBit(m_iWidth), enf::GetHighestBit(m_iHeight));
		int test = 1 << m;
		
		m = (m_iWidth <= test) && (m_iHeight <= test) ? m : m + 1;
		m_iMipMaps = intMinMax(m_iMipMaps, 1, m);
	}

	ENF_INLINE int GetWidth() const
	{
		return m_iWidth;
	}

	ENF_INLINE int GetHeight() const
	{
		return m_iHeight;
	}

	ENF_INLINE IDirect3DTexture9* Get() const
	{
		return _texture;
	}

	ENF_INLINE void Free()
	{
		if (!DecRef() && _texture)
		{
			_texture.Free();
		}
	}

	void Create(IDirect3DDevice9* dev)
	{
		if (!dev)
			return;

		AddRef();
		
		if (RefCounter() > 1)
		{
			return;
		}

		//is referenced by somebody else
		Assert(_texture.IsNull())

		ComRef<IDirect3DTexture9> texture;
		if (FAILED(dev->CreateTexture(m_iWidth, m_iHeight, m_iMipMaps, 0, m_Format, D3DPOOL_DEFAULT, texture.Init(), NULL)))
		{
			RptF("Error: Cannot create procedural  texture");
			return;
		}

		ComRef<IDirect3DTexture9> sysMem;
		if (FAILED(dev->CreateTexture(m_iWidth, m_iHeight, m_iMipMaps, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, sysMem.Init(), NULL)))
		{
			RptF("Error: Cannot create procedural sysmem texture");
			return;
		}

		//for each mipmap 
		int mipmap		= m_iMipMaps;
		int currLevel = 0;

		ComRef<IDirect3DSurface9> prevSurface;
		D3DSURFACE_DESC prevDesc;
		D3DLOCKED_RECT prevLR;

		memset(&prevDesc, 0, sizeof(D3DSURFACE_DESC));
		memset(&prevLR, 0, sizeof(D3DLOCKED_RECT));

		while (mipmap--)
		{
			ComRef<IDirect3DSurface9> surface;

			if (FAILED(sysMem->GetSurfaceLevel(currLevel++, surface.Init())))
			{
				RptF("Error: Cannot get procedural texture surface ");	
				return;
			}

			D3DSURFACE_DESC desc;
			if (FAILED(surface->GetDesc(&desc)))
			{
				RptF("Error: Cannot get procedural texture surface description");
				return;
			}

			D3DLOCKED_RECT lr;
			if (FAILED(surface->LockRect(&lr, NULL, 0)))
			{
				RptF("Error: Cannot lock procedural  texture surface");
				return;
			}

			Fill(desc, lr, prevDesc, prevLR);

			surface->UnlockRect();
			
			if (!prevSurface.IsNull())
				prevSurface->UnlockRect();

			prevSurface = surface;
			prevDesc = desc;
			prevLR = lr;
		}

		if (!prevSurface.IsNull())
			prevSurface->UnlockRect();

		dev->UpdateTexture(sysMem, texture);

		_texture = texture;
	}
};


//-----------------------------------------------------------------------------
class NoiseRotationTextureRGBA8 : public ProceduralTexture
{
	void Fill(const D3DSURFACE_DESC &desc, const D3DLOCKED_RECT& lRect, const D3DSURFACE_DESC &prevDesc, const D3DLOCKED_RECT& prevLRect) override
	{
		for (int y = 0; y < (int) desc.Height; y++)
		{
			unsigned char *data = (unsigned char*) ((char*) lRect.pBits + y*lRect.Pitch);

			for (int x = 0; x < (int) desc.Width; x++, data += 4)
			{
				float ranVal = GRandGen.RandomValue();
				float angle = ranVal*3.14159265f*2.0f;

				float s = sin(angle);
				float c = cos(angle);

				//random matrix -> in memory it's ARGB -> [-c, c, s, s] to have correct rotation transformation
				//in a way x' = dot(xy, rotMatrix.xy) and y' = dot(xy, rotMatrix.zw)
				data[0] = (unsigned char) ((-c*0.5f + 0.5f)*255.0f);
				data[1] = (unsigned char) (( c*0.5f + 0.5f)*255.0f);
				data[2] = (unsigned char) (( s*0.5f + 0.5f)*255.0f);
				data[3] = (unsigned char) (( s*0.5f + 0.5f)*255.0f);
			}
		}
	}

public:
	NoiseRotationTextureRGBA8(int width, int height): ProceduralTexture(width, height, D3DFMT_A8R8G8B8, 1)
	{}
};


//-----------------------------------------------------------------------------
class NoiseReflectionTextureLA8 : public ProceduralTexture
{
	void Fill(const D3DSURFACE_DESC &desc, const D3DLOCKED_RECT& lRect, const D3DSURFACE_DESC &prevDesc, const D3DLOCKED_RECT& prevLRect) override
	{
		for (int y = 0; y < (int) desc.Height; y++)
		{
			unsigned char *data = (unsigned char*) ((char*) lRect.pBits + y*lRect.Pitch);

			for (int x = 0; x < (int) desc.Width; x++, data += 2)
			{
				float angle = GRandGen.RandomValue()*3.14159265f*2.0f;

				float s = sin(angle);
				float c = cos(angle);

				data[0] = (unsigned char) (( c*0.5f + 0.5f)*255.0f);
				data[1] = (unsigned char) (( s*0.5f + 0.5f)*255.0f);
			}
		}
	}

public:
	NoiseReflectionTextureLA8(int width, int height): ProceduralTexture(width, height, D3DFMT_A8L8, 1)
	{}
};


//-----------------------------------------------------------------------------
class NoiseUniformTextureRGBA8 : public ProceduralTexture
{
	void Fill(const D3DSURFACE_DESC &desc, const D3DLOCKED_RECT& lRect, const D3DSURFACE_DESC &prevDesc, const D3DLOCKED_RECT& prevLRect) override
	{
		for (int y = 0; y < (int) desc.Height; y++)
		{
			unsigned char *data = (unsigned char*) ((char*) lRect.pBits + y*lRect.Pitch);

			for (int x = 0; x < (int) desc.Width; x++, data += 4)
			{
				data[0] = (unsigned char) (GRandGen.RandomValue()*255.0f);
				data[1] = (unsigned char) (GRandGen.RandomValue()*255.0f);
				data[2] = (unsigned char) (GRandGen.RandomValue()*255.0f);
				data[3] = (unsigned char) (GRandGen.RandomValue()*255.0f);
			}
		}
	}

public:
	NoiseUniformTextureRGBA8(int width, int height): ProceduralTexture(width, height, D3DFMT_A8R8G8B8, 1)
	{}
};


NoiseReflectionTextureLA8		GNoiseReflectionTexture(128, 128);
NoiseRotationTextureRGBA8		GNoiseRotationTexture(128, 128);
NoiseUniformTextureRGBA8		GNoiseUniformTexture(128, 128);


//-----------------------------------------------------------------------------
void EngineDD9::PostProcess::Prepare()
{
  const int cb = -1;
  _engine->FlushAndFreeAllQueues();
  // we want all render states to be set as needed
  // postprocess coordinates are given in 3D, but there is no camera space
  _engine->SwitchTL(cb,TLViewspace);

  // Set the src blend and dest blend somehow (they are not going to be set in the DoPrepareTriangle and the temporary setting returning would set a undefined value)
  _engine->D3DSetRenderState(cb, D3DRS_SRCBLEND,D3DBLEND_ONE);
  _engine->D3DSetRenderState(cb, D3DRS_DESTBLEND,D3DBLEND_ONE);

  // Set standard render states (it is possible to change it in every descendant, but
  // it must return it also back)
  // No alpha blending - we will do everything in the pixel shader
  // Use point filter
  EngineShapeProperties prop;
  _engine->DoPrepareTriangle(cb,NULL,TexMaterialLODInfo(NULL),0,NoZBuf|NoZWrite|ClampU|ClampV|FilterPoint,prop);
  _engine->DoShadowsStatesSettings(cb,SS_Transparent,false);
}

//-----------------------------------------------------------------------------
void EngineDD9::PostProcess::Finish()
{
  int cb = -1;
  // Restore usual rendering settings
  _engine->SwitchTL(cb, TLDisabled);
  
  #if _DEBUG || _PROFILE
    if (!_engine->_hlRecording && Glob.config._renderThread==Config::ThreadingEmulated)
    {
      _engine->KickOffCBDebug();
    }
  #endif
}

//-----------------------------------------------------------------------------
bool EngineDD9::PostProcess::CompilePixelShader(
  const LazyLoadFile &hlsl, const char *psName, ComRef<IDirect3DPixelShader9> &ps,
  bool fullPrec, D3DXMacroList *d3DXMacroList
) const
{
  // define macros for the shaders
  D3DXMacroList defines;
  if (d3DXMacroList == NULL)
    _engine->CreateBasicDefines(defines);
  else
    defines = *d3DXMacroList;

  D3DXCompileShader_Result result;  
  
  HRESULT hr;
  DWORD flags = fullPrec ? 0 : D3DXSHADER_PARTIALPRECISION;

  hr = D3DXCompileShader_Cached(hlsl, defines.Data(), &_engine->_hlslInclude, psName, _engine->_caps._psVersion, flags, result);
  if (FAILED(hr))
  {
    LogF("Error: %x in D3DXCompileShader (%s).", hr, psName);
    if (result.errorMsgs.NotNull())
    {
      LogF("%s", (const char*)result.errorMsgs->GetBufferPointer());
    }
    return false;
  };
  if (FAILED(_engine->GetDirect3DDevice()->CreatePixelShader((const DWORD*)result.shader->GetBufferPointer(), ps.Init())))
  {
    LogF("Error: %s shader creation failed.",psName);
    return false;
  }

  return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PostProcess::CompileVertexShader(
  const LazyLoadFile &hlsl, const char *vsName, ComRef<IDirect3DVertexShader9> &vs
) const
{
  // define macros for the shaders
  D3DXMacroList defines;
  _engine->CreateBasicDefines(defines);
  
  D3DXCompileShader_Result result;  
  DWORD flags = DebugVS ? D3DXSHADER_SKIPOPTIMIZATION|D3DXSHADER_DEBUG : 0;
  
  HRESULT hr;
  hr = D3DXCompileShader_Cached(hlsl, defines.Data(), &_engine->_hlslInclude, vsName, _engine->_caps._vsVersion, flags, result);
  if (FAILED(hr))
  {
    LogF("Error: %x in D3DXCompileShader (%s).", hr, vsName);
    if (result.errorMsgs.NotNull())
    {
      LogF("%s", (const char*)result.errorMsgs->GetBufferPointer());
    }
    return false;
  };
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexShader((const DWORD*)result.shader->GetBufferPointer(), vs.Init())))
  {
    LogF("Error: %s shader creation failed.",vsName);
    return false;
  }

  return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPEnfShapeCollectionRes::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
	if (!PostProcess::Init(hlsl,vs)) 
		return false;

	// Create the VD
	const D3DVERTEXELEMENT9 vertexDecl[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};

	ComRef<IDirect3DVertexDeclaration9> vd;
	if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
	{
		LogF("Error: DebugShape vertex declaration creation failed.");
		return false;
	}
	_vd = vd;

 	if (!CompileVertexShader(vs, "VSEnfDebugShape", _vs))
		return false;

	if (!CompilePixelShader(hlsl, "PSEnfDebugShape", _ps, false))
		return false;

	return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPEnfShapeCollectionRes::UnInit()
{
	_vd.Free();
	_vs.Free();
	_ps.Free();

	return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPEnfShapeCollectionRes::Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
	UnInit();
	Init(hlsl, vs);
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSimple::Prepare()
{
  base::Prepare();

  const int cb = -1;
  // Set the stream source
  _engine->SetStreamSource(cb,_vb, sizeof(Vertex), 4);

  // Set the vertex declaration
  _engine->SetVertexDeclaration(cb,_vd);

  // Set the vertex shader
  _engine->SetVertexShader(cb,_vs);

  // Set the pixel shader
  _engine->SetPixelShader(cb,_ps);
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPSimple::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const char *vsName)
{
  if (!base::Init(hlsl,vs)) return false;

  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  Vertex *v;
  float offsetU = 1.0f / _engine->Width() * 0.5f; //See the Nearest-Point Sampling in the DX9 documentation
  float offsetV = 1.0f / _engine->Height() * 0.5f;
  vb->Lock(0, 0, (void**)&v, 0);
  v[0].pos = Vector3P(-0.5, -0.5, 0.5); v[0].t0.u = 0 + offsetU; v[0].t0.v = 1 + offsetV;
  v[1].pos = Vector3P(-0.5,  0.5, 0.5); v[1].t0.u = 0 + offsetU; v[1].t0.v = 0 + offsetV;
  v[2].pos = Vector3P( 0.5, -0.5, 0.5); v[2].t0.u = 1 + offsetU; v[2].t0.v = 1 + offsetV;
  v[3].pos = Vector3P( 0.5,  0.5, 0.5); v[3].t0.u = 1 + offsetU; v[3].t0.v = 0 + offsetV;

  vb->Unlock();

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }

  // note: vertex shader is stored in PS.hlsl for simple effects
  if (!CompileVertexShader(hlsl,vsName,_vs))
  {
    return false;
  }

  _vb = VertexStaticData9::BufferFuture(vb);
  _vd = vd;

  return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPFinal::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs)) return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessCopy", _ps))
  {
    return false;
  }
  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFinal::Do(bool isLast)
{
  Do(isLast,true);
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFinal::Do(bool isLast,  bool forceNoSRGBOnRead)
{
  // not much to do if current render target is back-buffer (common situation, happens whenever we were doing 3D rendering)
  if (_engine->_currRenderTargetSurface!=_engine->_backBuffer)
  {
    PROFILE_SCOPE_GRF_EX(ppFin,pp,SCOPE_COLOR_YELLOW);

    // Remember the current render target (to be used as a texture later)
    IDirect3DTexture9 *rt = _engine->GetCurrRenderTargetAsTexture();
    DoAssert(rt);

    // Use the back buffer as render target
    _engine->SwitchToBackBufferRenderTarget();

    bool rescale = _engine->_wRT!=_engine->_w || _engine->_hRT!=_engine->_h;

    Do(rt, _engine->_wRT, _engine->_hRT, rescale, forceNoSRGBOnRead);
  }

  Finish();
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFinal::Do(IDirect3DTexture9 *rt, float w, float h, bool rescale, bool forceNoSRGBOnRead)
{
  PROFILE_SCOPE_GRF_EX(ppFin,pp,SCOPE_COLOR_YELLOW);

  DoAssert(rt);
  const int cb = -1;

  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();
  
  // StretchRect not possible here?
  // Prepare drawing
  Prepare();

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Use the texture
  SetTextureTemp tRenderTarget(0, rt);
  
  // note: stage0 is set to point sampling by default (see FilterPoint in EngineDD9::PostProcess::Prepare)
  D3DTEXTUREFILTERTYPE filter = rescale ? D3DTEXF_LINEAR : D3DTEXF_POINT;
  SetSamplerStateTemp sampFilterMag0(0, D3DSAMP_MAGFILTER, filter);
  SetSamplerStateTemp sampFilterMin0(0, D3DSAMP_MINFILTER, filter);

  BOOL rtIsSRGB = _engine->_caps._rtIsSRGB && _engine->_caps._canReadSRGB && !forceNoSRGBOnRead;
  SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE, rtIsSRGB);

  PushVertexShaderConstant pushC0(0);
  PushVertexShaderConstant pushC1(1);
  PushVertexShaderConstant pushC2(2);
  PushVertexShaderConstant pushC3(3);
  PushVertexShaderConstant pushC4(4);
  PushVertexShaderConstant pushC5(5);
  PushVertexShaderConstant pushC6(6);
  PushVertexShaderConstant pushC7(7);
  PushVertexShaderConstant pushC8(8);

  // Prepare the UV coordinates offset coefficients
  static const float baseU[4] = {0, 0, 1, 1};
  static const float baseV[4] = {1, 0, 1, 0};

  // older version, well tuned
  const float tgtOffsetU = 0;
  const float tgtOffsetV = 0;
  const float srcOffsetU = rescale ? 0 : 0.5f / w;
  const float srcOffsetV = rescale ? 0 : 0.5f / h;

  //IDirect3DTexture9 *glow = _engine->_pp._bloom->GetBlurredTexture();

  //D3DSURFACE_DESC glowDesc;
  //glow->GetLevelDesc(0, &glowDesc);

  float U[4][4];
  float V[4][4];
  for (int vertex = 0; vertex < 4; vertex++)
  {
    // Texture 0 - source image
    U[vertex][0] = baseU[vertex] + srcOffsetU;
    V[vertex][0] = baseV[vertex] + srcOffsetV;
    // Texture 1
    U[vertex][1] = 0;
    V[vertex][1] = 0;
    // Texture 2
    U[vertex][2] = 0;
    V[vertex][2] = 0;
    // Texture 3
    U[vertex][3] = 0;
    V[vertex][3] = 0;
  }
  const float posOffset[4] = {tgtOffsetU,tgtOffsetV,0,0};
  _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
  _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);
  _engine->SetVertexShaderConstantF(cb, 8, posOffset, 1);
  _engine->SetPixelShader(cb,_ps);
  
  // Drawing
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Final: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE


  if (!inScene) _engine->D3DEndScene();
}


//-----------------------------------------------------------------------------
/*
	Generate coefficients used in filtering process. Coefficients define offsets and weights for neighbouring
	samples used for sample determination. More information about coefficients can be found in GPU Gems 2,
	page 316.
*/
//-----------------------------------------------------------------------------
static void GenCoefficients(AutoArray<float> &vec, int itemsNum)
{
	const float wMult = 1.0f / 6.0f;

  float invDivider = 1.0f/ (itemsNum - 1);
	for (int i = 0; i < itemsNum; i++)
	{
		float alpha = i * invDivider; // alpha [0, 1], fractional part of texel coords
		float alpha2 = alpha * alpha;
		float alpha3 = alpha2 * alpha;

		// B-spline kernel - Coons cubics
		float w0 =		-alpha3 + 3.0f * alpha2 - 3.0f * alpha + 1.0f;		// w0 = -1*a^3 + 3*a^2 - 3*a + 1
		float w1 =  3.0f * alpha3 - 6.0f * alpha2 + 4.0f;						// w1 =  3*a^3 - 6*a^2 + 0*a + 4
		//w1 =  3.0f * alpha3 - 6.0f * alpha2 + 3.0f * alpha;
		float w2 = -3.0f * alpha3 + 3.0f * alpha2 + 3.0f * alpha + 1.0f;		// w2 = -3*a^3 + 3*a^2 + 3*a + 0
		//w2 = - 3.0f * alpha3 + 3.0f * alpha2;
		float w3 =		 alpha3;											// w3 =  1*a^3 + 0*a^2 + 0*a + 0

		w0 *= wMult;
		w1 *= wMult;
		w2 *= wMult;
		w3 *= wMult;

		// weights
		float g0 = w0 + w1;
		float g1 = w2 + w3;

		// offsets
		float h0 = 1 - (w1 / g0) + alpha;
		float h1 = 1 + (w3 / g1) - alpha;

		//std::cout << h1 << ", " << h0 << ", " << g1 << ", " << g0 << std::endl;

		vec.Add(h1);
		vec.Add(h0);
		vec.Add(g1);
		vec.Add(g0);
	}
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPRescaleBicubic::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs,"VSPostProcessRescaleBicubic")) return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessRescaleBicubic", _ps))
  {
    return false;
  }

  // create lookup texture
  {
    const int width = 128;
	  const int lookupTextureSize = width * 4;
	  
    IDirect3DDevice9 *dev = _engine->_d3DDevice.GetRef();
    #define FLOAT_COEFS 1
    
    #if FLOAT_COEFS
      D3DFORMAT coefFormat = D3DFMT_A16B16G16R16F;
    #else
  	  // it seems all coefficients are 0..1, 8b precision should be enough for blending weights
      D3DFORMAT coefFormat = D3DFMT_A8B8G8R8;
    #endif
    
     if ( FAILED(dev->CreateTexture(
		  width, 1, /*levels*/ 1, /*usage*/ 0, coefFormat, D3DPOOL_DEFAULT, _lookUpTexture.Init(), NULL
		  )) ) { return false; }
		
	  ComRef<IDirect3DTexture9> tmpTexture;

	  // temporary texture created in system pool used data filling
	  if ( FAILED(dev->CreateTexture( width, 1, /*levels*/ 1, /*usage*/ 0, coefFormat, D3DPOOL_SYSTEMMEM, tmpTexture.Init(), NULL)) ) \
			return false;

	  AutoArray<float> vec;
	  vec.Realloc(lookupTextureSize);

	  //prepare coefficients vector (h0, h1, g0 & g1)
	  GenCoefficients(vec, width);

	  // lock and fill texture by coefficients - convert to float16
	  D3DLOCKED_RECT lockRect;
	  if ( FAILED(tmpTexture->LockRect(0, &lockRect, 0, 0)) ) { return false; }
	  #if FLOAT_COEFS
	    D3DXFloat32To16Array((D3DXFLOAT16 *)lockRect.pBits,vec.Data(), lookupTextureSize);
	  #else
	    // caution: what about endians?
	    BYTE *coefs = (BYTE *)lockRect.pBits;
	    for (int i=0; i<lookupTextureSize; i++)
	    {
	      int coefI = toInt(vec[i]*255);
	      saturate(coefI,0,255);
	      coefs[i] = coefI;
	    }
	  #endif
	  tmpTexture->UnlockRect(0);

	  // update texture
	  if ( FAILED(dev->UpdateTexture(tmpTexture, _lookUpTexture)) ) 
			 return false;
  }
  
  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPRescaleBicubic::Do(bool isLast)
{ 
  Assert(_engine->_wRT<_engine->_w || _engine->_hRT<_engine->_h);

  // StretchRect not possible here?
  // Prepare drawing
  Prepare();

  // Remember the current render target (to be used as a texture later)
  IDirect3DTexture9 *rt = _engine->GetCurrRenderTargetAsTexture();
  DoAssert(rt);

  // Use the back buffer as render target
  _engine->SwitchToBackBufferRenderTarget();

  Do(rt, _engine->_wRT, _engine->_hRT);
  
  Finish();
}


//-----------------------------------------------------------------------------
void EngineDD9::PPRescaleBicubic::Do(IDirect3DTexture9 *rt, float w, float h)
{
  const int cb = -1;

  PROFILE_SCOPE_GRF_EX(ppBic,pp,SCOPE_COLOR_YELLOW);

  DoAssert(rt);

  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  BOOL rtIsSRGB = _engine->_caps._rtIsSRGB && _engine->_caps._canReadSRGB;
  SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE, rtIsSRGB);

  PushVertexShaderConstant pushC0(0);
  PushVertexShaderConstant pushC1(1);
  PushVertexShaderConstant pushC2(2);
  PushVertexShaderConstant pushC3(3);
  PushVertexShaderConstant pushC4(4);
  PushVertexShaderConstant pushC5(5);
  PushVertexShaderConstant pushC6(6);
  PushVertexShaderConstant pushC7(7);
  PushVertexShaderConstant pushC8(8);

  PushPixelShaderConstant pushP0(0);
  PushPixelShaderConstant pushP1(1);

  // sampler 0 - texture to filtering
  SetTextureTemp tRenderTarget(0, rt);
  SetSamplerStateTemp s0Mag(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp s0Min(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp s0Mip(0, D3DSAMP_MIPFILTER, 0);
  SetSamplerStateTemp s0U(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp s0V(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // sampler 1 - lookup texture
  SetTextureTemp t1(1, _lookUpTexture);
  SetSamplerStateTemp s1Mag(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp s1Min(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp s1Mip(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);

  D3DXVECTOR4 srcTexSize(w, h, 0.0f, 0.0f);
  D3DXVECTOR4 texelSize(1.0f / w, 0.0f, 0.0f, 1.0f / h);

  _engine->SetPixelShaderConstantF(cb, 0, (const float *)&srcTexSize, 1);
  _engine->SetPixelShaderConstantF(cb, 1, (const float *)&texelSize, 1);
  _engine->SetPixelShader(cb,_ps);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Bicubic rescale: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  if (!inScene) _engine->D3DEndScene();
}


//-----------------------------------------------------------------------------
bool EngineDD9::PostProcessCustomEdge::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexCustomEdge) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  void *data;
  vb->Lock(0, 0, &data, 0);
  VertexCustomEdge *v = (VertexCustomEdge *)data;
  v[0].pos = Vector3P(-0.5, -0.5, 0.5); v[0].index = 0;
  v[1].pos = Vector3P(-0.5,  0.5, 0.5); v[1].index = 1;
  v[2].pos = Vector3P( 0.5, -0.5, 0.5); v[2].index = 2;
  v[3].pos = Vector3P( 0.5,  0.5, 0.5); v[3].index = 3;
  vb->Unlock();

  _vbCustomEdge = VertexStaticData9::BufferFuture(vb);

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDeclCustomEdge[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
    D3DDECL_END()
  };
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDeclCustomEdge, _vdCustomEdge.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }

  // Create the VS
  if (!CompileVertexShader(vs,"VSPostProcessCustomEdge4T", _vsCustomEdge))
  {
    return false;
  }


  // Success
  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PostProcessCustomEdge::Prepare()
{
  base::Prepare();
  
  int cb = -1;
  // Set the vertex shader + stream source
  _engine->SetStreamSource(cb,_vbCustomEdge, sizeof(VertexCustomEdge), 4);
  _engine->SetVertexDeclaration(cb,_vdCustomEdge);
  _engine->SetVertexShader(cb,_vsCustomEdge);

}

static const int GBSizeDivisor = 4;

//-----------------------------------------------------------------------------
bool EngineDD9::PPGaussianBlur::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs)) return false;

  // Create pixel shaders
  {
    if (!CompilePixelShader(hlsl,"PSPostProcessGaussianBlurH", _psHorizontal, true)) return false;
    if (!CompilePixelShader(hlsl,"PSPostProcessGaussianBlurV", _psBlur, true)) return false;
  }

  // Create horizontal blur and final blur render targets
  {
    int fullWidth = _engine->Width();
    int fullHeight = _engine->Height();
    int partWidth = fullWidth / GBSizeDivisor;
    int partHeight = fullHeight / GBSizeDivisor;
    HRESULT err;
    err = _engine->CreateTextureRT(partWidth, fullHeight, 1, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtHorizontal, _rtsHorizontal, true);
    if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
    err = _engine->CreateTextureRT(partWidth, partHeight, 1, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtBlur, _rtsBlur, true);
    if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
  }

  _toBlur = 0;
  // Success
  return true;
}


//-----------------------------------------------------------------------------
// perform blur on specified texture (toBlur) instead back buffer
void EngineDD9::PPGaussianBlur::Do(bool isLast, IDirect3DTexture9 *toBlur)
{
  Assert(_toBlur == 0);
  _toBlur = toBlur; // set blur source
  Do(isLast);
  _toBlur = 0;  // remove
}


//-----------------------------------------------------------------------------
void EngineDD9::PPGaussianBlur::Do(bool isLast)
{
  // Draw
  PROFILE_SCOPE_GRF_EX(ppGSB,pp,SCOPE_COLOR_YELLOW);

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  const int cb = -1;
  
  // Perform blur in 2 steps
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Gauss blur: ");
  {
    // Protect VS constants from changing
    PushVertexShaderConstant pushC0(0);
    PushVertexShaderConstant pushC1(1);
    PushVertexShaderConstant pushC2(2);
    PushVertexShaderConstant pushC3(3);
    PushVertexShaderConstant pushC4(4);
    PushVertexShaderConstant pushC5(5);
    PushVertexShaderConstant pushC6(6);
    PushVertexShaderConstant pushC7(7);
    PushVertexShaderConstant pushC8(8);

    // Prepare the UV coordinates offset coefficients
    static const float baseU[4] = {0, 0, 1, 1};
    static const float baseV[4] = {1, 0, 1, 0};
    int fullWidth = _engine->Width();
    int fullHeight = _engine->Height();
    int partWidth = fullWidth / GBSizeDivisor;
    int partHeight = fullHeight / GBSizeDivisor;

    // No posOffset needed
    const float posOffset[4] = {0, 0, 0, 0};
    _engine->SetVertexShaderConstantF(cb,8, posOffset, 1);

    IDirect3DTexture9 *rt = (_toBlur == 0) ? _engine->CreateRenderTargetCopyAsTexture() : _toBlur;

    // Set temporarily render target and depth stencil to horizontally shrunk texture
    _engine->SetTemporaryRenderTarget(_rtHorizontal, _rtsHorizontal, NULL);

    // We need to set the viewport as it was set to previous values inside the SetTemporaryRenderTarget.
    // We cannot use the SetDepthRange method here, because we need to use custom Width and Height
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData,0,sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = partWidth;
      viewData.Height = fullHeight;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      CALL_D3D_MEMBER(_engine,SetViewport,viewData);
    }

    // Set temporary render states
    SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

    // Use SRGB conversion in texture read
    SetSamplerStateTemp ssSRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);

    // Write in the same space as you read // _VBS3_TI
    SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, _engine->_caps._rtIsSRGB);

    // Use linear sampling and clamping for the texture on stage 0
    SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // First step - render horizontally shrunk texture
    {
      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / partWidth;
      const float srcOffsetV = 0.5f / fullHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);

      // Set the pixel shader for final blur
      _engine->SetPixelShader(cb, _psHorizontal);

      // Set current render target as temporary texture (we know we don't render into it)
      SetTextureTemp tTex0(0, rt);

      // Prepare PS constants
      float invW = 1.0f / _engine->Width();
      SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invW, invW, invW, invW));

			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Horizontal: ");
				_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }

    // Rewrite render target and depth stencil surfaces by the blurred ones
    _engine->SetRenderTargetAndDepthStencil(_rtsBlur, _rtBlur, NULL);

    // Second step - render finally blurred texture from the horizontally shrunked one
    {
      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / partWidth;
      const float srcOffsetV = 0.5f / partHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);

      // Set the pixel shader for final blur
      _engine->SetPixelShader(cb, _psBlur);

      // Set current render target as temporary texture (we know we don't render into it)
      SetTextureTemp tTex0(0, _rtHorizontal);

      // Prepare PS constants
      float invH = 1.0f / _engine->Height();
      SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invH, invH, invH, invH));

      // Drawing
			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Vertical: ");
				_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }

    // Restore the original render target and depth stencil buffer
    _engine->RestoreOldRenderTarget();
  }
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  _engine->SwitchTL(cb,TLDisabled);
}



static const int BloomSizeDivisor = 8;

//-----------------------------------------------------------------------------
bool EngineDD9::PPBloom::InitCustomEdge(const LazyLoadFile &vs)
{
  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexCustomEdge) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  void *data;
  vb->Lock(0, 0, &data, 0);
  VertexCustomEdge *v = (VertexCustomEdge *)data;
  v[0].pos = Vector3P(-1.0, -1.0, 1.0); v[0].index = 0;
  v[1].pos = Vector3P(-1.0,  1.0, 1.0); v[1].index = 1;
  v[2].pos = Vector3P( 1.0, -1.0, 1.0); v[2].index = 2;
  v[3].pos = Vector3P( 1.0,  1.0, 1.0); v[3].index = 3;
  vb->Unlock();

  _vbCustomEdge = VertexStaticData9::BufferFuture(vb);

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDeclCustomEdge[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
    D3DDECL_END()
  };
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDeclCustomEdge, _vdCustomEdge.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }

  // Create the VS
  if (!CompileVertexShader(vs,"VSPostProcessCustomEdge4T", _vsCustomEdge))
  {
    return false;
  }

  // Create horizontal blur and final blur render targets
  {
    int fullWidth = _engine->Width();
    int fullHeight = _engine->Height();
    int partWidth = fullWidth / BloomSizeDivisor;
    int partHeight = fullHeight / BloomSizeDivisor;
    HRESULT err;
    err = _engine->CreateTextureRT(fullWidth/2, fullHeight/2, 1, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtBright, _rtsBright, true);
    if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
    err = _engine->CreateTextureRT(partWidth, partHeight, 1, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtHorizontal, _rtsHorizontal, true);
    if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
    err = _engine->CreateTextureRT(partWidth, partHeight, 1, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtBlur, _rtsBlur, true);
    if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
  }

  // Success
  return true;
}


//-----------------------------------------------------------------------------
int EngineDD9::PPBloom::SetParams(const float *pars, int nPars)
{
  if(nPars > 0)
    _blurScale = pars[0];

  if(nPars > 1)
    _imageScale = pars[1];

  if(nPars > 2)
    _threshold = pars[2];

  return 0;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPBloom::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs)) return false;

  _imageScale = 0.8f;
  _blurScale = 0.25f;

  // Create custom edge vertex resources
  if (!InitCustomEdge(vs)) return false;

  // Create pixel shaders
  {
    if (!CompilePixelShader(hlsl,"PSPostProcessGaussBlur", _psBlur, true)) return false;
    if (!CompilePixelShader(hlsl,"PSPostProcessBloomDownsample2", _psDownsample2, true)) return false;
    if (!CompilePixelShader(hlsl,"PSPostProcessBloomDownsample4", _psDownsample4, true)) return false;
    if (!CompilePixelShader(hlsl,"PSPostProcessBloomCombine", _psCombine, true)) return false;
  }

  // Success
  return true;
}


//-----------------------------------------------------------------------------
float ComputeGaussianValue( float x, float mean, float std_deviation )
{
  // The gaussian equation is defined as such:
  /*    
  -(x - mean)^2
  -------------
  1.0               2*std_dev^2
  f(x,mean,std_dev) = -------------------- * e^
  sqrt(2*pi*std_dev^2)

  */

  return ( 1.0f / sqrt( 2.0f * 3.14159265f * std_deviation * std_deviation ) ) 
    * expf( (-((x-mean)*(x-mean)))/(2.0f * std_deviation * std_deviation) );
}


typedef float Vec4[4];

//-----------------------------------------------------------------------------
static void GaussKernel(Vec4* hkernel, Vec4* vkernel, int kernelSize, int width, int height)
{
	const float GAUSS_MEAN = 0.0f;
	const float GAUSS_DEVIATION = 0.5f;

  float	biwidth = 1.0f / (float)width;
  float biheight = 1.0f / (float)height;
  float hsize = (float)(kernelSize - 1) * 0.5f;

  for(int i = 0; i < kernelSize; i++ )
  {
    float f = ((float)i - hsize) / hsize;
    float gauss = ComputeGaussianValue( f, GAUSS_MEAN, GAUSS_DEVIATION );
    hkernel[i][0] = ((float)i - hsize) * biwidth;
    hkernel[i][1] = 0.0f;
    hkernel[i][2] = 0.0f;
    hkernel[i][3] = gauss;

    vkernel[i][0] = 0.0f;
    vkernel[i][1] = ((float)i - hsize) * biheight;
    vkernel[i][2] = 0.0f;
    vkernel[i][3] = gauss;
  }

  // the kernel needs to be normalized, so that we do not change image brightness
  float kernelSum = 0;
  for (int i=0; i<kernelSize; i++)
  {
    kernelSum += hkernel[i][3];
  }
  float normF = 1.0f/kernelSum;
  for (int i=0; i<kernelSize; i++)
  {
    hkernel[i][3] *= normF;
    vkernel[i][3] = hkernel[i][3];
  }
}


//-----------------------------------------------------------------------------
void EngineDD9::PPBloom::Do(bool isLast)
{ 
  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();
  
  // Remember the current render target
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
  Do(isLast,rt);

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}


//-----------------------------------------------------------------------------
void EngineDD9::PPBloom::Do(bool isLast, IDirect3DTexture9 *rt)
{
  // Draw
  PROFILE_SCOPE_GRF_EX(ppBlo,pp,SCOPE_COLOR_YELLOW);

  const int cb = -1;
  
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Bloom: ");

  // Protect VS constants from changing
  PushVertexShaderConstant pushC0(0);
  PushVertexShaderConstant pushC1(1);
  PushVertexShaderConstant pushC2(2);
  PushVertexShaderConstant pushC3(3);
  PushVertexShaderConstant pushC4(4);
  PushVertexShaderConstant pushC5(5);
  PushVertexShaderConstant pushC6(6);
  PushVertexShaderConstant pushC7(7);
  PushVertexShaderConstant pushC8(8);

  PushPixelShaderConstant ppushC0(0);
  PushPixelShaderConstant ppushC1(1);
  PushPixelShaderConstant ppushC2(2);
  PushPixelShaderConstant ppushC3(3);
  PushPixelShaderConstant ppushC4(4);
  PushPixelShaderConstant ppushC5(5);
  PushPixelShaderConstant ppushC6(6);
  PushPixelShaderConstant ppushC7(7);
  PushPixelShaderConstant ppushC8(8);
  PushPixelShaderConstant ppushC9(9);
  PushPixelShaderConstant ppushC10(10);
  PushPixelShaderConstant ppushC11(11);
  PushPixelShaderConstant ppushC12(12);
  PushPixelShaderConstant ppushC13(13);
  PushPixelShaderConstant ppushC14(14);
  PushPixelShaderConstant ppushC15(15);

  // Use clamping for the textures
  SetSamplerStateTemp ss0(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss1(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss2(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss3(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  float scales[16][4];
  int fullWidth = _engine->Width();
  int fullHeight = _engine->Height();
  int brightWidth = fullWidth / 2;
  int brightHeight = fullHeight / 2;
  int partWidth = fullWidth / BloomSizeDivisor;
  int partHeight = fullHeight / BloomSizeDivisor;

  {
    // Set the vertex shader + stream source
    _engine->SetStreamSource(cb, _vbCustomEdge, sizeof(VertexCustomEdge), 4);
    _engine->SetVertexDeclaration(cb, _vdCustomEdge);
    _engine->SetVertexShader(cb, _vsCustomEdge);

    // Prepare the UV coordinates offset coefficients
    static const float baseU[4] = {0, 0, 1, 1};
    static const float baseV[4] = {1, 0, 1, 0};

    // Set temporarily render target and depth stencil to 2x2 shrunk texture
    ComRef<IDirect3DSurface9> zeroDepth;
    _engine->SetTemporaryRenderTarget(_rtBright, _rtsBright, zeroDepth);

    // We need to set the viewport as it was set to previous values inside the SetTemporaryRenderTarget.
    // We cannot use the SetDepthRange method here, because we need to use custom Width and Height
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData,0,sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = brightWidth;
      viewData.Height = brightHeight;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      CALL_D3D_MEMBER(_engine,SetViewport,viewData);
    }

    // Set temporary render states
    SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

    // Use SRGB conversion in texture read
    SetSamplerStateTemp ssSRGBTEXTURE0(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
    SetSamplerStateTemp ssSRGBTEXTURE1(0, D3DSAMP_SRGBTEXTURE, FALSE);

    // Write in the same space as you read // _VBS3_TI
    SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, _engine->_caps._rtIsSRGB);

    // First step - render bright pass
    {
      // Use point sampling for the textures
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / fullWidth;
      const float srcOffsetV = 0.5f / fullHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);
      
      // No posOffset needed
      const float posOffset[4] = {0, 0, 0, 0};
      _engine->SetVertexShaderConstantF(cb, 8, posOffset, 1);

      // Set the pixel shader for bright pass
      _engine->SetPixelShader(cb, _psDownsample2);

      // Set current render target as temporary texture (we know we don't render into it)
      SetTextureTemp tTex0(0, rt);
      SetTextureTemp tTex2(1, _engine->_pp._glow->GetApertureTexture()); // aperture control texture
    //	SetTextureTemp tTex1(1, ((TextureD3D9*)_gammaTexture.GetRef())->GetHandle());

      // Prepare PS constants - PSSceneOffsets2;
      int index = 0;
      for(int y = -1; y < 1; y++)
      {
        for(int x = -1; x < 1; x++)
        {
          scales[index][0] = (1.0f / (float)fullWidth) * ((float)y + 0.5f);
          scales[index][1] = (1.0f / (float)fullHeight) * ((float)x + 0.5f);
          scales[index][2] = 0;
          scales[index][3] = 0;
          index++;
        }
      }
      _engine->SetPixelShaderConstantF(cb, 0, scales[0], 4);
      float treshold[4];
      treshold[0] = treshold[1] = treshold[2] = treshold[3] = _threshold;
      _engine->SetPixelShaderConstantF(cb, 4, treshold, 1);


			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Bright pass: ");
	      _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }

    // Second step - 4x4 downscale
    {
      ComRef<IDirect3DSurface9> zeroDepth;
      _engine->SetRenderTargetAndDepthStencil(_rtsBlur, _rtBlur, zeroDepth);

      // We need to set the viewport as it was set to previous values inside the SetTemporaryRenderTarget.
      // We cannot use the SetDepthRange method here, because we need to use custom Width and Height
      {
        D3DVIEWPORT9 viewData;
        memset(&viewData,0,sizeof(viewData));
        viewData.X = 0;
        viewData.Y = 0;
        viewData.Width  = partWidth;
        viewData.Height = partHeight;
        viewData.MinZ = 0.0f;
        viewData.MaxZ = 1.0f;
        CALL_D3D_MEMBER(_engine,SetViewport,viewData);
      }

      // Use point sampling for the textures
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / brightWidth;
      const float srcOffsetV = 0.5f / brightHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);
      // No posOffset needed
      const float posOffset[4] = {0, 0, 0, 0};
      _engine->SetVertexShaderConstantF(cb, 8, posOffset, 1);

      // Set the pixel shader for downsample pass
      _engine->SetPixelShader(cb, _psDownsample4);

      // Set current render target as temporary texture (we know we don't render into it)
      SetTextureTemp tTex0(0, _rtBright);

      // Prepare PS constants - PSSceneOffsets4;
      int index = 0;
      index = 0;
      for(int y = -2; y < 2; y++)
      {
        for(int x = -2; x < 2; x++)
        {
          scales[index][0] = (1.0f / (float)brightWidth) * ((float)x + 0.5f);
          scales[index][1] = (1.0f / (float)brightHeight) * ((float)y + 0.5f);
          scales[index][2] = 0.0f;
          scales[index][3] = 0.0f;
          index++;
        }
      }
      _engine->SetPixelShaderConstantF(cb, 0, scales[0], 16);

			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  4x4 downscale: ");
	      _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }

    Vec4 hkernel[KERNEL_SIZE];
    Vec4 vkernel[KERNEL_SIZE];

    GaussKernel(hkernel, vkernel, KERNEL_SIZE, partWidth, partHeight);

    // Third step - horizontal blur
    {
      ComRef<IDirect3DSurface9> zeroDepth;
      _engine->SetRenderTargetAndDepthStencil(_rtsHorizontal, _rtHorizontal, zeroDepth);

      // We need to set the viewport as it was set to previous values inside the SetTemporaryRenderTarget.
      // We cannot use the SetDepthRange method here, because we need to use custom Width and Height
      {
        D3DVIEWPORT9 viewData;
        memset(&viewData,0,sizeof(viewData));
        viewData.X = 0;
        viewData.Y = 0;
        viewData.Width  = partWidth;
        viewData.Height = partHeight;
        viewData.MinZ = 0.0f;
        viewData.MaxZ = 1.0f;
        CALL_D3D_MEMBER(_engine,SetViewport,viewData);
      }

      // Use point sampling for the textures
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / partWidth;
      const float srcOffsetV = 0.5f / partHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);
      // No posOffset needed
      const float posOffset[4] = {0, 0, 0, 0};
      _engine->SetVertexShaderConstantF(cb, 8, posOffset, 1);

      // Set the pixel shader for blur pass
      _engine->SetPixelShader(cb, _psBlur);

      // Set current render target as temporary texture (we know we don't render into it)
      SetTextureTemp tTex0(0, _rtBlur);

      _engine->SetPixelShaderConstantF(cb, 0, hkernel[0], KERNEL_SIZE);

			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Horizontal blur: ");
	      _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }

    // Fourth step - vertical blur
    {
      ComRef<IDirect3DSurface9> zeroDepth;
      _engine->SetRenderTargetAndDepthStencil(_rtsBlur, _rtBlur, zeroDepth);

      // We need to set the viewport as it was set to previous values inside the SetTemporaryRenderTarget.
      // We cannot use the SetDepthRange method here, because we need to use custom Width and Height
      {
        D3DVIEWPORT9 viewData;
        memset(&viewData,0,sizeof(viewData));
        viewData.X = 0;
        viewData.Y = 0;
        viewData.Width  = partWidth;
        viewData.Height = partHeight;
        viewData.MinZ = 0.0f;
        viewData.MaxZ = 1.0f;
        CALL_D3D_MEMBER(_engine,SetViewport,viewData);
      }

      // Use point sampling for the textures
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

      // Set current render target as temporary texture (we know we don't render into it)
      SetTextureTemp tTex0(0, _rtHorizontal);

      _engine->SetPixelShaderConstantF(cb, 0, vkernel[0], KERNEL_SIZE);

			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Vertical blur: ");
	      _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }

    _engine->RestoreOldRenderTarget();

    // Fifth step - combine blurred image and the original one
    if(isLast)
    {
      SetTextureTemp tTex0(0, rt);
      SetTextureTemp tTex1(1, _rtBlur);

      // Use point sampling for the textures
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

      // Prepare the UV coordinates offsets
      const float srcOffsetU1 = 0.5f / partWidth;
      const float srcOffsetV1 = 0.5f / partHeight;
      const float srcOffsetU2 = 0.5f / fullWidth;
      const float srcOffsetV2 = 0.5f / fullHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - blurred image
        U[vertex][0] = baseU[vertex] + srcOffsetU1;
        V[vertex][0] = baseV[vertex] + srcOffsetV1;
        // Texture 1 - source image
        U[vertex][1] = baseU[vertex] + srcOffsetU2;
        V[vertex][1] = baseV[vertex] + srcOffsetV2;
        // Rest
        U[vertex][2] = U[vertex][3] = 0;
        V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);

      Vec4 scales[2];
      scales[0][0] = scales[0][1] = scales[0][2] = scales[0][3] = _blurScale;
      scales[1][0] = scales[1][1] = scales[1][2] = scales[1][3] = _imageScale;
      _engine->SetPixelShaderConstantF(cb, 0, scales[0], 2);
      
      // Set the pixel shader for combine pass
      _engine->SetPixelShader(cb, _psCombine);

      // Drawing
			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Combine: ");
	      _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }
  }

	GPU_END_PROFILE
}


void ConvertMatrixTransposed(PASSMATRIXTYPE &mat, Matrix4Val src);
void ConvertMatrixTransposed3(MatrixFloat34 &mat, Matrix4Val src);
void ConvertMatrix3(MatrixFloat34 &mat, Matrix4Val src);
void ConvertProjectionMatrix(D3DMATRIX &mat, Matrix4Val src, float zBias);


//-----------------------------------------------------------------------------
void EngineDD9::PPSSSM::Prepare()
{
  // This method overrides the PPSimple::Prepare method, that's why we call the PostProcess::Prepare here instead of usual base::Prepare
  PostProcess::Prepare();

  const int cb = -1;
  
  // Set the stream source
  _engine->SetStreamSource(cb, _vb, sizeof(Vertex), 4);

  // Set the vertex declaration
  _engine->SetVertexDeclaration(cb, _vd);

  // Set the vertex shader
  _engine->SetVertexShader(cb, _vs);

  // Set the pixel shader
  if (_engine->_sbTechnique == SBT_nVidia)
  {
    _engine->SetPixelShader(cb, _psNVidia);
  }
  else
  {
    _engine->SetPixelShader(cb, _ps);
  }
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPSSSM::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs,"VSPostProcess")) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessSSSM_Default", _ps, true)) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessSSSM_nVidia", _psNVidia, true)) return false;

	#ifdef TEST_RAIN_MAP
		if (!CompilePixelShader(hlsl,"PSPostProcessRainMap", _psRainMap, true)) return false;
	#endif
  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSSSM::Do(float minShadowsZ, float maxShadowsZ, bool firstLayer, bool lastLayer)
{
  // Draw
  PROFILE_SCOPE_GRF_EX(ppSSSM,pp,SCOPE_COLOR_YELLOW);

  const int cb = -1;

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Pass the FOV into the shader
  float fovW = 1.0f;
  float fovH = 1.0f;
  if (_engine->CBState(cb)._proj.IsFinite())
  {
    fovW = 1.0f / _engine->CBState(cb)._proj(0, 0);
    fovH = 1.0f / _engine->CBState(cb)._proj(1, 1);
  }
  SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(fovW, fovH, 0, 0));

  // Influence density of trees by multiplying the final alpha
  Color pscSBTSize_invSBTSize_X_AlphaMult(_engine->_sbSize, 1.0f / _engine->_sbSize, 0, 1.5f); // Empirical coefficient controlling density of trees
  SetPixelShaderConstantTemp spc1(1, (float*)&pscSBTSize_invSBTSize_X_AlphaMult);

  // Setup the matrix that converts the transformedPosition into shadow map
  _engine->SetPixelShaderConstantF(cb, 20, (float*)&_engine->_shadowMapMatrixDX, 3);

  // Setup the SB depth scale
  Color pscMaxDepth(_engine->_maxDepth, _engine->_maxDepth, _engine->_maxDepth, _engine->_maxDepth);
  _engine->SetPixelShaderConstantF(cb, 23, (float*)&pscMaxDepth, 1);

  // Setup the border constants
  D3DXVECTOR4 layerBorder;
  layerBorder = D3DXVECTOR4(maxShadowsZ, Glob.config.GetShadowZ()/*lastLayer ? 1.0f : 0.0f*/, maxShadowsZ, minShadowsZ);
  _engine->SetPixelShaderConstantF(cb, 40, layerBorder, 1);

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
  SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);

  // We're going to render to SSSM
  ComRef<IDirect3DSurface9> zeroDepth;
  _engine->SetTemporaryRenderTarget(_engine->_sssm, _engine->_sssmSurface, zeroDepth);

  // Clear the SSSM, as it's possible nothing will be drawn on certain places
  if (firstLayer)
  {
    CALL_D3D_MEMBER(_engine,Clear,0,NULL,D3DCLEAR_TARGET,PackedColor(255, 0, 0, 0),0,0);
  }

  // Set depth map as a input
  SetTextureTemp tTex0(0, _engine->_renderTargetDepthInfo);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss0MIPFILTER(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, FALSE);
  
  // Set the SB as shadow map
  _engine->UseShadowMapSB(cb);

  /*
  // because of VSM we want bilinear sampling - UseShadowMapSB set point
  _engine->D3DSetSamplerState(TEXID_SHADOWMAP, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  _engine->D3DSetSamplerState(TEXID_SHADOWMAP, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  */
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "SSSM: ");
	  _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  _engine->RestoreOldRenderTarget();

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  
  // if it was the last layer, and we are using multisampling, it is a time to Resolve / StretchRect
  // we are currently not using multisampling on SSSM on the PC, and Xbox handles it elsewhere
	//     if (_engine->GetFSAA()>0)
	//     {
	//       // TODO: make compatible with background rendering
	//       //DoAssert(!_engine->_backgroundD3D);
	//       ComRef<IDirect3DSurface9> sssmTexLevel0;
	//       _engine->_sssm->GetSurfaceLevel(0,sssmTexLevel0.Init());
	//       CALL_D3D_MEMBER(_engine,StretchRect,_engine->_sssmSurface,NULL,sssmTexLevel0,NULL,D3DTEXF_POINT);
	//     }

  Finish();
}


#ifdef TEST_RAIN_MAP

//-----------------------------------------------------------------------------
void EngineDD9::PPSSSM::DoRainMapPP()
{
  // Draw
  PROFILE_SCOPE_GRF_EX(ppRainMapPP,pp,SCOPE_COLOR_YELLOW);

  const int cb = -1;

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();
	_engine->SetPixelShader(cb, _psRainMap);

	static const D3DXVECTOR4 normOffsets[5] = 
	{
		D3DXVECTOR4(-1.0f, -1.0f,			  0.0f, -1.0f),
		D3DXVECTOR4( 1.0f, -1.0f,			 -1.0f,  0.0f),
		D3DXVECTOR4( 1.0f,  0.0f,			 -1.0f,  1.0f),
		D3DXVECTOR4( 0.0f,  1.0f,				1.0f,  1.0f),
		D3DXVECTOR4( 0.0f,  0.0f,				0.0f,  0.0f),
	};

	D3DXVECTOR4 offsets[5];

	//this is hack, VSPostprocess has exact UV in stream biased by these values
	float offsetU = 0.5f / _engine->Width();
	float offsetV = 0.5f / _engine->Height();

	float invSize = 1.0f/RAIN_MAP_SB_SIZE;
	float halfInvSize = 0.5f*invSize;

	for (int i = 0; i < 5; i++)
	{
		offsets[i].x = halfInvSize + normOffsets[i].x*invSize - offsetU;
		offsets[i].y = halfInvSize + normOffsets[i].y*invSize - offsetV;
		offsets[i].z = halfInvSize + normOffsets[i].z*invSize - offsetU;
		offsets[i].w = halfInvSize + normOffsets[i].w*invSize - offsetV;
	}

	PushPixelShaderConstantAll<5> psC(0);
	_engine->SetPixelShaderConstantF(cb, 0, (const float*) &offsets, 5);

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
  SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);

  ComRef<IDirect3DSurface9> zeroDepth;
  _engine->SetTemporaryRenderTarget(_engine->_rtRainMap, _engine->_rtRainMapSurface, zeroDepth);

	D3DVIEWPORT9 viewData;
	viewData.X = 0;
	viewData.Y = 0;
	viewData.Width  = RAIN_MAP_SB_SIZE;
	viewData.Height = RAIN_MAP_SB_SIZE;
	viewData.MinZ = 0.0f;
	viewData.MaxZ = 1.0f;
	CALL_D3D_MEMBER(_engine,SetViewport,viewData);

  // Set depth map as a input
  SetTextureTemp tTex0(0, _engine->_rtRainMapDepth);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss0MIPFILTER(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, FALSE);
  
	//_engine->UseShadowMapSB(cb);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "SSSM(Rain map): ");
	  _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

	viewData.Width  = _engine->_w;
	viewData.Height = _engine->_h;
	CALL_D3D_MEMBER(_engine,SetViewport,viewData);

  // Restore previous render target settings
  _engine->RestoreOldRenderTarget();

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  Finish();
}

#endif


//-----------------------------------------------------------------------------
bool EngineDD9::PPSSSMStencil::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs,"VSPostProcess")) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessSSSMStencil", _ps, true)) return false;
  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSSSMStencil::Do(bool isLast)
{
  PROFILE_SCOPE_GRF_EX(ppSSSMStencil,pp,SCOPE_COLOR_YELLOW);

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  const int cb = -1;
  
  {
    // make sure SSSM is not mapped as a texture
    _engine->SetTexture(cb, TEXID_SHADOWMAP,NULL);
    // SetTextureTemp did not work well - caused strange error "Texture not created with this device. SetTexture failed."
    //SetTextureTemp noSSSM(TEXID_SHADOWMAP,NULL);


    bool restore = true;
    if(_engine->_renderTargetDepthBufferSurfaceSaved.IsNull())
    {
      // We're going to render to SSSM
      //_engine->SetTemporaryRenderTarget(_engine->_sssm, _engine->_sssmSurface, _engine->_renderTargetDepthInfoZ);
      if (_engine->_depthBufferRT)
      {
        if (_engine->GetFSAA()>0)
        {
          // on PC SSSM is never multisampled
          _engine->SetTemporaryRenderTarget(_engine->_sssm, _engine->_sssmSurface, _engine->_renderTargetDepthInfoZ);
        }
        else
        {
          _engine->SetTemporaryRenderTarget(_engine->_sssm, _engine->_sssmSurface, _engine->_depthBufferRT);
        }
      }
      else
      {
        _engine->SetTemporaryRenderTarget(_engine->_sssm, _engine->_sssmSurface, _engine->_currRenderTargetDepthBufferSurface);
      }
    }
    else
    {
      _engine->SetRenderTargetAndDepthStencil(_engine->_sssmSurface, _engine->_sssm, _engine->_currRenderTargetDepthBufferSurface);
      // Set the depth range to make sure the depth range has not been changed
      _engine->SetDepthRange(_engine->_minZRange,_engine->_maxZRange);
      restore = false;
    }

    if (isLast)
    {
      CALL_D3D_MEMBER(_engine,Clear,0,NULL,D3DCLEAR_TARGET,PackedColor(255, 0, 0, 0),0,0);
    }

    // Set usual PP render states
    SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);

    // Set render states to control the stencil visualization
		if (!FULL_STENCIL)
			_engine->D3DSetRenderState(cb, D3DRS_STENCILENABLE, TRUE);

    SetRenderStateTemp rsSTENCILFUNC(D3DRS_STENCILFUNC, D3DCMP_LESS);
    SetRenderStateTemp rsSTENCILMASK(D3DRS_STENCILMASK, STENCIL_COUNTER);
    SetRenderStateTemp rsSTENCILREF(D3DRS_STENCILREF, 0);
    SetRenderStateTemp rsZTEST(D3DRS_ZENABLE,D3DZB_FALSE);
    SetRenderStateTemp rsALPHABLENDENABLE(D3DRS_ALPHABLENDENABLE, FALSE);
    SetRenderStateTemp rsSRCBLEND(D3DRS_SRCBLEND, D3DBLEND_ONE);
    SetRenderStateTemp rsDESTBLEND(D3DRS_DESTBLEND, D3DBLEND_ONE);

    // Drawing
		GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "SSSM(stencil): ");
			_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
		GPU_END_PROFILE

		if (!FULL_STENCIL)
			_engine->D3DSetRenderState(cb, D3DRS_STENCILENABLE, FALSE);

    if (restore)
    {
      // Restore previous render target settings
      _engine->RestoreOldRenderTarget();
    }

  }

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();
  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
#ifdef SSSMBLUR
bool EngineDD9::PPSSSMB::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!PostProcess::Init(hlsl,vs)) return false;

  // Create custom edge vertex resources
  if (!InitCustomEdge(vs)) return false;

  // Create pixel shaders
  {
    if (!CompilePixelShader(hlsl,"PSPostProcessSSSMBH", _psHorizontal, true)) return false;
    if (!CompilePixelShader(hlsl,"PSPostProcessSSSMBV", _psBlur, true)) return false;
    if (!CompilePixelShader(hlsl,"PSPostProcessSSSMBFinalBlur", _psFinalBlur, true)) return false;
  }

  // Success
  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSSSMB::Do(bool isLast)
{
  // Draw
  PROFILE_SCOPE_GRF_EX(ppSSSMB,pp,SCOPE_COLOR_YELLOW);

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Perform blur in 2 steps
  {
    // Protect VS constants from changing
    PushVertexShaderConstant pushC0(0);
    PushVertexShaderConstant pushC1(1);
    PushVertexShaderConstant pushC2(2);
    PushVertexShaderConstant pushC3(3);
    PushVertexShaderConstant pushC4(4);
    PushVertexShaderConstant pushC5(5);
    PushVertexShaderConstant pushC6(6);
    PushVertexShaderConstant pushC7(7);
    PushVertexShaderConstant pushC8(8);

    // Set the vertex shader + stream source
    _engine->SetStreamSource(_vbCustomEdge, sizeof(VertexCustomEdge), 4);
    _engine->SetVertexDeclaration(_vdCustomEdge);
    _engine->SetVertexShader(_vsCustomEdge);

    // Prepare the UV coordinates offset coefficients
    static const float baseU[4] = {0, 0, 1, 1};
    static const float baseV[4] = {1, 0, 1, 0};
    int fullWidth = _engine->Width();
    int fullHeight = _engine->Height();
    int partWidth = fullWidth / GBSizeDivisor;
    int partHeight = fullHeight / GBSizeDivisor;

    // No posOffset needed
    const float posOffset[4] = {0, 0, 0, 0};
    _engine->SetVertexShaderConstantF(8, posOffset, 1);

    // Set temporarily render target and depth stencil to horizontally shrunk texture
    ComRef<IDirect3DSurface9> zeroDepth;
    _engine->SetTemporaryRenderTarget(_rtHorizontal, _rtsHorizontal, zeroDepth);

    // We need to set the viewport as it was set to previous values inside the SetTemporaryRenderTarget.
    // We cannot use the SetDepthRange method here, because we need to use custom Width and Height
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData,0,sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = partWidth;
      viewData.Height = fullHeight;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      CALL_D3D_MEMBER(_engine,SetViewport,viewData);
    }

    // Set temporary render states
    SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

    // Use SRGB conversion in texture read
    SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, FALSE);
    SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, FALSE);

    // Write in the same space as you read
    SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);

    // Use linear sampling and clamping for the texture on stage 0
    SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // First step - render horizontally shrunk texture
    {
      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / partWidth;
      const float srcOffsetV = 0.5f / fullHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(0, U[0], 4);
      _engine->SetVertexShaderConstantF(4, V[0], 4);

      // Set the pixel shader for final blur
      _engine->SetPixelShader(_psHorizontal);

      // Set sssm as temporary texture
      SetTextureTemp tTex0(0, _engine->_sssm);

      // Prepare PS constants
      float invW = 1.0f / _engine->Width();
      SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invW, invW, invW, invW));

      // Drawing
      _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    }

    // Rewrite render target and depth stencil surfaces by the blurred ones
    _engine->SetRenderTargetAndDepthStencil(_rtsBlur, _rtBlur, zeroDepth);

    // Second step - render finally blurred texture from the horizontally shrunked one
    {
      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / partWidth;
      const float srcOffsetV = 0.5f / partHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(0, U[0], 4);
      _engine->SetVertexShaderConstantF(4, V[0], 4);

      // Set the pixel shader for final blur
      _engine->SetPixelShader(_psBlur);

      // Set current render target as temporary texture (we know we don't render into it)
      SetTextureTemp tTex0(0, _rtHorizontal);

      // Prepare PS constants
      float invH = 1.0f / _engine->Height();
      SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invH, invH, invH, invH));

      // Drawing
      _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    }

    // Third step - create the SSSMB map
    {
      // Rewrite render target and depth stencil surfaces by the SSSMB
      _engine->SetRenderTargetAndDepthStencil(_engine->_sssmBSurface, _engine->_sssmB, zeroDepth);

      // Prepare the UV coordinates offsets
      const float srcOffsetU = 0.5f / fullWidth;
      const float srcOffsetV = 0.5f / fullHeight;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Rest
        U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
        V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
      }
      _engine->SetVertexShaderConstantF(0, U[0], 4);
      _engine->SetVertexShaderConstantF(4, V[0], 4);

      // Set the pixel shader for final blur
      _engine->SetPixelShader(_psFinalBlur);

      // Set the textures
      SetTextureTemp tTex0(0, _engine->_sssm);
      SetTextureTemp tTex1(1, _rtBlur);

      // Drawing
      _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    }

    // Restore the original render target and depth stencil buffer
    _engine->RestoreOldRenderTarget();
  }

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  _engine->SwitchTL(TLDisabled);
}
#endif


//-----------------------------------------------------------------------------
/*!
 FXAA post process effect
*/
//-----------------------------------------------------------------------------
const char* STR_FXAA_SHADER_SOURCE = "FXAA_SHADER_SOURCE";

const char* STR_FXAA_SHADER_DEBUG = "FXAA_SHADER_DEBUG";

const char* STR_FXAA_QUALITY__PRESET = "FXAA_QUALITY__PRESET";

//const char* FXAA_QUALITY__PRESET[] =	{
//  "10", "11", "12", "13", "14", "15",                           // FXAA QUALITY - MEDIUM DITHER PRESETS
//  "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",   // FXAA QUALITY - LOW DITHER PRESETS
//  "39"                                                          // FXAA QUALITY - EXTREME QUALITY
//};

const char* FXAA_QUALITY__PRESET[] =	{
  "12",   // FXAA QUALITY - MEDIUM DITHER PRESETS
  "25",   // FXAA QUALITY - LOW DITHER PRESETS
  "39"    // FXAA QUALITY - EXTREME QUALITY
};

//-----------------------------------------------------------------------------
bool EngineDD9::PPFXAA::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl, vs)) return false;

  D3DXMacroList defines;
  _engine->CreateBasicDefines(defines, false);
  // select sources
  D3DXMACRO &sourceMacro = defines.Append();
  sourceMacro.Name = STR_FXAA_SHADER_SOURCE;
  sourceMacro.Definition = "2";     // FXAA 3.11
  // define shader complexity
  D3DXMACRO &macro = defines.Append();
#if _ENABLE_CHEATS
  D3DXMACRO &macroDebug = defines.Append();
  macroDebug.Name = STR_FXAA_SHADER_DEBUG;
  macroDebug.Definition = "1";
#endif
  // terminal macro
  const D3DXMACRO empty = { NULL, NULL };
  defines.Append(empty);

  _psFXAAs.Realloc(lenof(FXAA_QUALITY__PRESET));
  _psFXAAs.Resize(lenof(FXAA_QUALITY__PRESET));

  for (int i = 0; i < lenof(FXAA_QUALITY__PRESET); i++)
  {
    macro.Name = STR_FXAA_QUALITY__PRESET;
    macro.Definition = FXAA_QUALITY__PRESET[i];

    if (FAILED(CompilePixelShader(hlsl, "PsPpFxaa3_11", _psFXAAs[i], false, &defines)))
      return false;
  }

  if (FAILED(CompilePixelShader(hlsl, "PsSharpenFilter", _psSharpen)))
	  return false;

  if (FAILED(CompileVertexShader(hlsl, "VsPpFxaa3_11", _vsFxaa)))
    return false;

  MyD3DDevice &mDevice = _engine->GetDirect3DDevice();

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };

  if (FAILED(mDevice->CreateVertexDeclaration(vertexDecl, _vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }

  // shared rt with smaa - used for rt resampling
  if (FAILED(mDevice->CreateTexture(_engine->_w, _engine->_h, 1, D3DUSAGE_RENDERTARGET, _engine->_rtFormat, D3DPOOL_DEFAULT, _backRT.Init(), NULL)))
    return false;

  if (FAILED(_backRT->GetSurfaceLevel(0, _backSurface.Init())))
    return false;

  // shared rt with smaa
   if (FAILED(mDevice->CreateTexture(_engine->_w, _engine->_h, 1, D3DUSAGE_RENDERTARGET, _engine->_rtFormat, D3DPOOL_DEFAULT, _backRT1.Init(), NULL)))
     return false;

   if (FAILED(_backRT1->GetSurfaceLevel(0, _backSurface1.Init())))
     return false;

  ComRef<IDirect3DVertexBuffer9> vb;

  if (FAILED(mDevice->CreateVertexBuffer(
    sizeof(VertexP3T2) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }

  VertexP3T2 *v;
  vb->Lock(0, 0, (void**)&v, 0);
  v[0].pos = Vector3P(-1.0f, -1.0f, 0.0f); v[0].tc.u = 0.0f; v[0].tc.v = 1.0f;
  v[1].pos = Vector3P(-1.0f,  1.0f, 0.0f); v[1].tc.u = 0.0f; v[1].tc.v = 0.0f;
  v[2].pos = Vector3P( 1.0f, -1.0f, 0.0f); v[2].tc.u = 1.0f; v[2].tc.v = 1.0f;
  v[3].pos = Vector3P( 1.0f,  1.0f, 0.0f); v[3].tc.u = 1.0f; v[3].tc.v = 0.0f;
  vb->Unlock();

  _vb = VertexStaticData9::BufferFuture(vb);

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPFXAA::UnInit()
{
  _vd.Free();
  _vsFxaa.Free();
  _psFxaa.Free();
  _psSharpen.Free();
  
  _backSurface.Free();
  _backRT.Free();

  _backSurface1.Free();
  _backRT1.Free();

  for (int i = 0; i < _psFXAAs.Size(); i++)
    _psFXAAs[i].Free();

  _psFXAAs.Clear();

  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFXAA::Prepare()
{  
  base::Prepare();

  const int cb = -1;
  _engine->SetVertexDeclaration(cb, _vd);
  _engine->SetStreamSource(cb, _vb, sizeof(VertexP3T2), 4);
  _engine->SetVertexShader(cb, _vsFxaa);
  _engine->SetPixelShader(cb, _psFxaa, false);
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFXAA::Do(bool isLast)
{
  // Draw
  PROFILE_SCOPE_GRF_EX(pFxaa, *, SCOPE_COLOR_GREEN);
  
  if (!IsEnabled())
    return;
  
  const int cb = -1;
  
  const int width = _engine->_w;
  const int height = _engine->_h;

  D3DVIEWPORT9 viewData;
  memset(&viewData,0,sizeof(viewData));
  viewData.X = 0;
  viewData.Y = 0;
  viewData.Width  = width;
  viewData.Height = height;
  viewData.MinZ = 0.0f;
  viewData.MaxZ = 1.0f;

  IDirect3DTexture9 *rt = _engine->GetCurrRenderTargetAsTexture();

  bool rescale = _engine->_wRT != width || _engine->_hRT != height;

  if (_engine->_currRenderTargetSurface == _engine->_backBuffer || rescale)
  {
    // set temporal render target
    _engine->SetTemporaryRenderTarget(_backRT, _backSurface, NULL);
    CALL_D3D_MEMBER(_engine,SetViewport,viewData);

    if (_engine->_wRT < width || _engine->_hRT < height) 
    {
      _engine->_pp._bicubic->Do(rt, _engine->_wRT, _engine->_hRT);
    }
    else
    {
      _engine->_pp._final->Do(rt, _engine->_wRT, _engine->_hRT, rescale, true);
    }

    _engine->RestoreOldRenderTarget();

    rt = _backRT;
  }

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  Prepare();
  
  const float invW = 1.0/(float)width;
  const float invH = 1.0/(float)height;

  // Prepare the UV coordinates offsets
  const float srcOffsetU = 0.5f / width;
  const float srcOffsetV = 0.5f / height;

  SetVertexShaderConstantTemp vs0(0, D3DXVECTOR4(srcOffsetU, srcOffsetV, 0, 0));
  SetPixelShaderConstantTemp ps1(1, D3DXVECTOR4(invW, invH, width, height));

  // Set temporary render states
  SetRenderStateTemp rs0(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
  SetRenderStateTemp rs6(D3DRS_CULLMODE, D3DCULL_NONE);
  SetRenderStateTemp rs8(D3DRS_ZWRITEENABLE, FALSE);
  SetRenderStateTemp rs9(D3DRS_ZENABLE, FALSE);
  SetRenderStateTemp rs10(D3DRS_SRGBWRITEENABLE, FALSE);

  static float QualitySubpix = 0.75;
  static float QualityEdgeThreshold = 0.166;
  static float QualityEdgeThresholdMin = 0.0625;
  static float FXAADebug = 0.0f;
  static float SharpOffset = 0.6;	
  static float SharpenEdge = 0.15;

  D3DXVECTOR4 vec4(QualitySubpix, QualityEdgeThreshold, QualityEdgeThresholdMin, FXAADebug);
  SetPixelShaderConstantTemp ps2(2, vec4);
    
  _engine->SetTemporaryRenderTarget(_backRT1, _backSurface1, NULL);
  CALL_D3D_MEMBER(_engine,SetViewport,viewData);

  {
    SetTextureTemp tTex0(0, rt);
    SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);

    SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "FXAA: ");
	    _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
		GPU_END_PROFILE
  }

  _engine->RestoreOldRenderTarget();
  CALL_D3D_MEMBER(_engine,SetViewport,viewData);

  // Sharp filter
  {
	  PROFILE_SCOPE_EX(pShar, *);

	  SetTextureTemp tTex1(0, _backRT1);
    
	  SetPixelShaderConstantTemp ps1(1, D3DXVECTOR4(invW, invH, invW * SharpOffset, invH * SharpOffset));
	  SetPixelShaderConstantTemp ps2(2, D3DXVECTOR4(SharpenEdge, 0, 0, 0));

	  _engine->SetPixelShader(cb, _psSharpen);

	  // if effect is last then switch to back buffer
	  _engine->SwitchToBackBufferRenderTarget();

	  GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "FXAA sharping: ");
			_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
		GPU_END_PROFILE
  }

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFXAA::SetQuality(int quality)
{
  if (!_psFXAAs.Size() || _quality == quality) 
    return;

  _quality = quality;

  if (_quality == 0)
    _psFxaa.Free();
  else 
  {
    saturate(_quality, 0, _psFXAAs.Size() - 1);
    _psFxaa = _psFXAAs[_quality];
  }
}


//-----------------------------------------------------------------------------
int EngineDD9::PPFXAA::GetQuality() const
{
  return _quality;
}


#include "ShaderSources_3_0/PPAA/AreaTex.h"
#include "ShaderSources_3_0/PPAA/SearchTex.h"

//-----------------------------------------------------------------------------
bool EngineDD9::PPSMAA::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, ComRef<IDirect3DTexture9> &sharedRT0, ComRef<IDirect3DTexture9> &sharedRT1)
{
  HRESULT hr;
  MyD3DDevice &mDevice = _engine->GetDirect3DDevice();
    
  const int width = _engine->_w;
  const int height = _engine->_h;

  // back buffer as texture
  _backRT = sharedRT0;

  hr = _backRT->GetSurfaceLevel(0, _backSurface.Init());
  if ( FAILED(hr) )
    return false;

  // temporal RT
  _edgeRT = sharedRT1;

  hr = _edgeRT->GetSurfaceLevel(0, _edgeSurface.Init());
  if ( FAILED(hr) )
    return false;

  hr = mDevice->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, _engine->_rtFormat, D3DPOOL_DEFAULT, _blendRT.Init(), NULL);

  hr = _blendRT->GetSurfaceLevel(0, _blendSurface.Init());
  if ( FAILED(hr) )
    return false;

  hr = mDevice->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, _depthSurface.Init(), NULL);

  // helper textures
  hr = mDevice->CreateTexture(AREATEX_WIDTH, AREATEX_HEIGHT, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8L8, D3DPOOL_DEFAULT, _areaTex.Init(), NULL);
  if ( FAILED(hr) )
    return false;

  D3DLOCKED_RECT rect;
  hr = _areaTex->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
  if ( FAILED(hr) )
    return false;

  for (int i = 0; i < AREATEX_HEIGHT; i++)
    CopyMemory(((char *) rect.pBits) + i * rect.Pitch, areaTexBytes + i * AREATEX_PITCH, AREATEX_PITCH);
  _areaTex->UnlockRect(0);

  hr = mDevice->CreateTexture(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, _searchTex.Init(), NULL);
  if ( FAILED(hr) )
    return false;  

  hr = _searchTex->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
  if ( FAILED(hr) )
    return false;

  for (int i = 0; i < SEARCHTEX_HEIGHT; i++)
    CopyMemory(((char *) rect.pBits) + i * rect.Pitch, searchTexBytes + i * SEARCHTEX_PITCH, SEARCHTEX_PITCH);
  _searchTex->UnlockRect(0);

  // compile shaders
  if (!CompileVertexShader(hlsl, "DX9_SMAAEdgeDetectionVS", _edgeDetectionVS))
    return false;
    
  const int EdgeDetectorShaders = 2;
  _edgeDetectionPS.Realloc(EdgeDetectorShaders);
  _edgeDetectionPS.Resize(EdgeDetectorShaders);

  if (!CompilePixelShader(hlsl, "DX9_SMAALumaEdgeDetectionPS", _edgeDetectionPS[0]))
    return false;

  if (!CompilePixelShader(hlsl, "DX9_SMAAColorEdgeDetectionPS", _edgeDetectionPS[1]))
    return false;

  //if (!CompilePixelShader(hlsl, "DX9_SMAADepthEdgeDetectionPS", _edgeDetectionPS[2]))
  //  return false;

  if (!CompileVertexShader(hlsl, "DX9_SMAABlendingWeightCalculationVS", _blendingWeightVS))
    return false;

  const char* SMAA_PRESETS[] = { "SMAA_PRESET_LOW", "SMAA_PRESET_MEDIUM", "SMAA_PRESET_HIGH", "SMAA_PRESET_ULTRA" };

  _blendingWeightPS.Realloc(PresetCount);
  _blendingWeightPS.Resize(PresetCount);

  for (int i = 0; i < PresetCount; i++) 
  {
    D3DXMacroList defines;
    _engine->CreateBasicDefines(defines, false);
    
    D3DXMACRO &presetMacro = defines.Append();
    presetMacro.Name = SMAA_PRESETS[i];
    presetMacro.Definition = "1";

    // terminal macro
    const D3DXMACRO empty = { NULL, NULL };
    defines.Append(empty);

    if (!CompilePixelShader(hlsl, "DX9_SMAABlendingWeightCalculationPS", _blendingWeightPS[i], false, &defines))
      return false;    
  }

  if (!CompileVertexShader(hlsl, "DX9_SMAANeighborhoodBlendingVS", _neighborhoodBlendingVS))
    return false;

  if (!CompilePixelShader(hlsl, "DX9_SMAANeighborhoodBlendingPS", _neighborhoodBlendingPS))
    return false;
  
  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };

  if (FAILED(mDevice->CreateVertexDeclaration(vertexDecl, _vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }

  ComRef<IDirect3DVertexBuffer9> vb;

  if (FAILED(mDevice->CreateVertexBuffer(
    sizeof(VertexP3T2) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }

  // aligned fullscreen quad
  float pixelSize[] = { 1.0f / float(width), 1.0f / float(height) };
  
  VertexP3T2 *v;
  vb->Lock(0, 0, (void**)&v, 0);
  v[0].pos = Vector3P(-1.0f - pixelSize[0],  1.0f + pixelSize[1], 0.0f); v[0].tc.u = 0.0f; v[0].tc.v = 0.0f;
  v[1].pos = Vector3P( 1.0f - pixelSize[0],  1.0f + pixelSize[1], 0.0f); v[1].tc.u = 1.0f; v[1].tc.v = 0.0f;
  v[2].pos = Vector3P(-1.0f - pixelSize[0], -1.0f + pixelSize[1], 0.0f); v[2].tc.u = 0.0f; v[2].tc.v = 1.0f;
  v[3].pos = Vector3P( 1.0f - pixelSize[0], -1.0f + pixelSize[1], 0.0f); v[3].tc.u = 1.0f; v[3].tc.v = 1.0f;
  vb->Unlock();

  _vb = VertexStaticData9::BufferFuture(vb);
  
  return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPSMAA::UnInit()
{
  _vd.Free();

  _backSurface.Free();
  _backRT.Free();
  _edgeSurface.Free();
  _edgeRT.Free();
  _blendSurface.Free();
  _blendRT.Free();

  _areaTex.Free();
  _searchTex.Free();

  _edgeDetectionVS.Free();

  for (int i = 0; i < _edgeDetectionPS.Size(); i++) 
    _edgeDetectionPS[i].Free(); 
  _edgeDetectionPS.Clear();

  _blendingWeightVS.Free();

  for (int i = 0; i < _blendingWeightPS.Size(); i++)
    _blendingWeightPS[i].Free();
  _blendingWeightPS.Clear();

  _neighborhoodBlendingVS.Free();
  _neighborhoodBlendingPS.Free();

  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSMAA::SetQuality(int quality)
{
  saturate(quality, 0, PresetCount - 1);
  _currentPreset = (Presets) quality;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSMAA::Do(bool isLast)
{
  // Draw
  PROFILE_SCOPE_GRF_EX(smaa, *, SCOPE_COLOR_GREEN);

  if (!IsEnabled())
    return;

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "SMAA");

  Prepare();
  
  const int cb = -1;
  
  const int width = _engine->_w;
  const int height = _engine->_h;

  D3DVIEWPORT9 viewData;
  memset(&viewData,0,sizeof(viewData));
  viewData.X = 0;
  viewData.Y = 0;
  viewData.Width  = width;
  viewData.Height = height;
  viewData.MinZ = 0.0f;
  viewData.MaxZ = 1.0f;

  IDirect3DTexture9 *rt = _engine->GetCurrRenderTargetAsTexture();

  bool rescale = _engine->_wRT != width || _engine->_hRT != height;

  if (_engine->_currRenderTargetSurface == _engine->_backBuffer || rescale)
  {
    // set temporal render target
    _engine->SetTemporaryRenderTarget(_backRT, _backSurface, NULL);
    CALL_D3D_MEMBER(_engine,SetViewport,viewData);

    if (_engine->_wRT < width || _engine->_hRT < height) 
    {
      _engine->_pp._bicubic->Do(rt, _engine->_wRT, _engine->_hRT);
    }
    else
    {
      _engine->_pp._final->Do(rt, _engine->_wRT, _engine->_hRT, rescale, true);
    }

    _engine->RestoreOldRenderTarget();

    rt = _backRT;
  }

  // EDGE DETECTION PASS
  _engine->SetVertexDeclaration(cb, _vd);
  _engine->SetStreamSource(cb, _vb, sizeof(VertexP3T2), 4);
  
  // Set temporary render states
  SetRenderStateTemp rs0(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
  SetRenderStateTemp rs1(D3DRS_ALPHABLENDENABLE, FALSE);
  SetRenderStateTemp rs2(D3DRS_ALPHATESTENABLE, FALSE);
  SetRenderStateTemp rs5(D3DRS_FOGENABLE, FALSE);
  SetRenderStateTemp rs6(D3DRS_CULLMODE, D3DCULL_NONE);
  SetRenderStateTemp rs8(D3DRS_ZWRITEENABLE, TRUE);
  SetRenderStateTemp rs9(D3DRS_ZENABLE, FALSE);
  SetRenderStateTemp rs10(D3DRS_SRGBWRITEENABLE, FALSE);

  SetRenderStateTemp rs11(D3DRS_STENCILENABLE, TRUE);
  SetRenderStateTemp rs12(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
  SetRenderStateTemp rs13(D3DRS_STENCILREF, 1);
  SetRenderStateTemp rs15(D3DRS_STENCILMASK, STENCIL_FULL);  
  SetRenderStateTemp rs16(D3DRS_STENCILWRITEMASK, STENCIL_FULL);
  SetRenderStateTemp rs17(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
  SetRenderStateTemp rs18(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
  SetRenderStateTemp rs19(D3DRS_TWOSIDEDSTENCILMODE, FALSE);

  _engine->SetVertexShader(cb, _edgeDetectionVS);
  _engine->SetPixelShader(cb, _edgeDetectionPS[1]);

  // SMAA_PIXEL_SIZE
  static const int idx = 1;
  float sizeData[] = { 1.0f / (float) width, 1.0f / (float) height, 0, 0 };
  _engine->SetVertexShaderConstantF(cb, idx, sizeData, 1);
  _engine->SetPixelShaderConstantF(cb, idx, sizeData, 1);
   
  _engine->SetTemporaryRenderTarget(_edgeRT, _edgeSurface, _depthSurface);
  CALL_D3D_MEMBER(_engine,SetViewport,viewData);

  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, FALSE);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MIPFILTER(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  SetTextureTemp tTex0(0, rt);

  _engine->Clear(false, true, (PackedColor) 0, true);
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Pixel Size: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE
  
  // BLENDING WEIGHT PASS
  SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, FALSE);
  SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MIPFILTER(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  SetSamplerStateTemp ss2SRGBTEXTURE(2, D3DSAMP_SRGBTEXTURE, FALSE);
  SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss2MIPFILTER(2, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss2ADDRESSU(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss2ADDRESSV(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  
  SetRenderStateTemp rs21(D3DRS_STENCILENABLE, TRUE);
  SetRenderStateTemp rs22(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
  SetRenderStateTemp rs23(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
  SetRenderStateTemp rs24(D3DRS_STENCILREF, 1);

  _engine->SetVertexShader(cb, _blendingWeightVS);
  _engine->SetPixelShader(cb, _blendingWeightPS[_currentPreset]);

  _engine->SetRenderTargetAndDepthStencil(_blendSurface, _blendRT, _depthSurface);

  SetTextureTemp blendTex0(0, _edgeRT);
  SetTextureTemp blendTex1(1, _areaTex);
  SetTextureTemp blendTex2(2, _searchTex);

  _engine->Clear(false, true, (PackedColor) 0, false);
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Blending weight pass: ");	
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  _engine->RestoreOldRenderTarget();

  // effect IS LAST -> switch to back buffer
  _engine->SwitchToBackBufferRenderTarget();
  CALL_D3D_MEMBER(_engine,SetViewport,viewData);

  // NEIGHBORHOODBLENDING PASS
  SetTextureTemp neighborhoodTex0(0, rt);
  SetTextureTemp neighborhoodTex1(1, _blendRT);

  SetRenderStateTemp rs31(D3DRS_STENCILENABLE, FALSE);

  _engine->SetVertexShader(cb, _neighborhoodBlendingVS);
  _engine->SetPixelShader(cb, _neighborhoodBlendingPS);

  _engine->Clear(false, true);
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Neighborhood pass: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

	GPU_END_PROFILE

  // Restore usual rendering settings
  Finish();
}


//-----------------------------------------------------------------------------
/*!
SSAO postprocess effect
*/
//-----------------------------------------------------------------------------
bool EngineDD9::PPSSAO::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit)
{
  _bDebug = false;

  if (!base::Init(hlsl,vs)) return false;

  if (firstInit)
  {
    SetDefaultValues();
    _enabled = false;
    _pInterp.Initialize();
  }

  Ref<PPSSAOResources> resrc = new PPSSAOResources;

  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }

  Vertex *v;
  vb->Lock(0, 0, (void**)&v, 0);
		v[0].pos = Vector3P(-1.0, -1.0, 2.0f); v[0].t0.u = 0; v[0].t0.v = 1;
		v[1].pos = Vector3P(-1.0,  1.0, 0.0f); v[1].t0.u = 0; v[1].t0.v = 0;
		v[2].pos = Vector3P( 1.0, -1.0, 3.0f); v[2].t0.u = 1; v[2].t0.v = 1;
		v[3].pos = Vector3P( 1.0,  1.0, 1.0f); v[3].t0.u = 1; v[3].t0.v = 0;
  vb->Unlock();

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;

  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }

  //vertex shader
  if (!CompileVertexShader(hlsl, "VSPostProcessSSAO", resrc->_vsSSAO))
    return false;

  if (!CompileVertexShader(hlsl, "VSPostProcessSSAODown", resrc->_vsSSAODown))
    return false;

	if (!CompileVertexShader(hlsl, "VSPostProcessSSAOBlur", resrc->_vsSSAOBlur))
		return false;

  //pixel shaders
	for (int passes = EngineDD9::PPSSAOResources::MIN_SSAO_PASSES; passes <= EngineDD9::PPSSAOResources::MAX_SSAO_PASSES; passes++ )
  {
		D3DXMacroList ssaoMacros;
		_engine->CreateBasicDefines(ssaoMacros);
		
		enf::SString<> name;
		name.format("%d", passes);

		D3DXMACRO &macro = ssaoMacros.Insert(ssaoMacros.Size() - 1);
		macro.Name = "SSAO_PASSES";
		macro.Definition = name.cstr();
	
		if (!CompilePixelShader(hlsl,"PSPostProcessSSAO", resrc->_psSSAO[passes - EngineDD9::PPSSAOResources::MIN_SSAO_PASSES], false, &ssaoMacros))
			return false;
	}

  if (!CompilePixelShader(hlsl,"PSPostProcessSSAOBlurVert", resrc->_psBlurVert, false))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessSSAOBlurHorz", resrc->_psBlurHorz, false))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessSSAOBlurHorzDepth", resrc->_psBlurHorzDepth, false))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessSSAOBlurVertDepth", resrc->_psBlurVertDepth, false))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessSSAOFinal", resrc->_psSSAOFinal, false))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessSSAODebug", resrc->_psSSAODebug, false))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessSSAODown", resrc->_psSSAODown, false))
    return false;

  _bHalfResOld = _pInterp._lastPars.m_bHalfRes;
  _bBlurHalfResOld = _pInterp._lastPars.m_bBlurHalfRes;

  if (_engine->_d3DDevice.IsNull()) return false;
  ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

  // render targets
  if (!SetRT(device.GetRef(), resrc, _bHalfResOld, _bBlurHalfResOld))
  {
    return false;
  }
 
	GNoiseRotationTexture.Create(device);

  // assign to shared resources
  resrc->_vd = vd;
  resrc->_vb = VertexStaticData9::BufferFuture(vb);

  resrc->_texWidth  = GNoiseRotationTexture.GetWidth();
  resrc->_texHeight = GNoiseRotationTexture.GetHeight();

  _resrc = resrc;

  return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPSSAO::SetRT(IDirect3DDevice9 *device, PPSSAOResources *res, bool halfres, bool blurhalfres)
{
  int wantedheight = _engine->Height() >> (halfres ? 1 : 0);
  int wantedwidth  = _engine->Width() >> (halfres ? 1 : 0);

  res->_SSAOWidth = wantedwidth;
  res->_SSAOHeight= wantedheight;

  _bHalfResOld = halfres;
  _bBlurHalfResOld = blurhalfres;

  D3DFORMAT rtFmt = D3DFMT_A8R8G8B8;

  HRESULT err;  
  err = device->CreateTexture(wantedwidth, wantedheight, 1, D3DUSAGE_RENDERTARGET, rtFmt, D3DPOOL_DEFAULT, res->_rtSSAO.Init(), 0);
  if (FAILED(err))
  {
    _engine->DDError9("Error: Render target texture creation failed.", err); 
    return false;
  }
  err = res->_rtSSAO->GetSurfaceLevel(0, res->_rtsSSAO.Init());
  if (FAILED(err))
  {
    _engine->DDError9("Error: GetSurfaceLevel failed.", err); 
    return false;
  }

  wantedheight = _engine->Height() >> ((/*halfres || */blurhalfres) ? 1 : 0);
  wantedwidth  = _engine->Width() >> ((/*halfres || */blurhalfres) ? 1 : 0);

  res->_blurWidth = wantedwidth;
  res->_blurHeight= wantedheight;
  
  err = device->CreateTexture(wantedwidth, wantedheight, 1, D3DUSAGE_RENDERTARGET, rtFmt, D3DPOOL_DEFAULT, res->_rtBlur.Init(), 0);
  if (FAILED(err))
  {
    _engine->DDError9("Error: Render target texture creation failed.", err); 
    return false;
  }
  err = res->_rtBlur->GetSurfaceLevel(0, res->_rtsBlur.Init());
  if (FAILED(err))
  {
    _engine->DDError9("Error: GetSurfaceLevel failed.", err); 
    return false;
  }
  
  if ((!halfres && blurhalfres) || (halfres && !blurhalfres))
  {
    err = device->CreateTexture(wantedwidth, wantedheight, 1, D3DUSAGE_RENDERTARGET, rtFmt, D3DPOOL_DEFAULT, res->_rtTemp.Init(), 0);
    if (FAILED(err))
    {
      _engine->DDError9("Error: Render target texture creation failed.", err); 
      return false;
    }
    err = res->_rtTemp->GetSurfaceLevel(0, res->_rtsTemp.Init());
    if (FAILED(err))
    {
      _engine->DDError9("Error: GetSurfaceLevel failed.", err); 
      return false;
    }
  }

  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSSAO::SetDefaultValues()
{
  _pInterp._pars.SetHighestSetting();
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPSSAO::Init(PPResources *resources, bool firstInit)
{
  DoAssert(false);

  if (resources)
  {
    _resrc = resources;

    if (firstInit)
    {
      _bDebug = false;

      SetDefaultValues();
      _enabled = false;
      _pInterp.Initialize();
    }

    return true;
  }

  return false;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPSSAO::UnInit()
{
  if (_resrc)
  {
		GNoiseRotationTexture.Free();
    _resrc->UnInit();

    return true;
  }

  return false;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSSAO::Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
	bool wasDebug = _bDebug;

	UnInit();
	Init(hlsl, vs, false);

	_bDebug = wasDebug;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPSSAO::SetParams(const float *pars, int nPars, bool imm)
{
  if(nPars > 0)
  {
    if (pars[0] < 0)
    {
      _bDebug = !_bDebug;
      return 0;
    }

    SET_SC(Intensity, 0)
  }

	SET_SC(NearPixelRadius, 1)
	SET_SC(FarPixelRadius, 2)
	SET_SC(CosAngle1, 3)
	SET_SC(CosAngle0, 4)
	SET_SC(MaxDistance, 5)
	SET_SC(MaxBlurDistance, 6)
	SET_SC(BlurPasses, 7)
	SET_SC(DepthBlurPasses, 8)
	SET_SC(SSAOPasses, 9)

	SET_BOOL(HalfRes, 10)
	SET_BOOL(BlurHalfRes, 11)

  return 0;
}


//-----------------------------------------------------------------------------
int EngineDD9::PPSSAO::SetParams(const AutoArray<float> &pars)
{
	bool imm = false;
  int nPars = pars.Size();

  if(nPars > 0)
  {
    if (pars[0] < 0)
    {
      _bDebug = !_bDebug;
      return 0;
    }

		SET_SC(Intensity, 0)
	}

	SET_SC(NearPixelRadius, 1)
	SET_SC(FarPixelRadius, 2)
	SET_SC(CosAngle1, 3)
	SET_SC(CosAngle0, 4)
	SET_SC(MaxDistance, 5)
	SET_SC(MaxBlurDistance, 6)
	SET_SC(BlurPasses, 7)
	SET_SC(DepthBlurPasses, 8)
	SET_SC(SSAOPasses, 9)

	SET_BOOL(HalfRes, 10)
	SET_BOOL(BlurHalfRes, 11)

  return 0;
}


//-----------------------------------------------------------------------------
LSError EngineDD9::PPSSAO::Serialize( ParamArchive &ar )
{
  CHECK(ar.Serialize("enabled",_enabled,0,false));
  CHECK(ar.Serialize("interp", _pInterp, 0));
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPSSAO::SetQuality(int quality)
{
  switch (quality)
  {
    case 5 : _pInterp._lastPars.SetHighestSetting(); break;
    case 4 : _pInterp._lastPars.SetHighSetting(); break;
    case 3 : _pInterp._lastPars.SetMediumSetting(); break;
		case 2 : _pInterp._lastPars.SetLowSetting(); break;
  }
}

//-----------------------------------------------------------------------------
/*
Compute rays to screen edges at far plane
\param camera				camera from scene
\param vec					OUT, rays
\param farPlaneMul	
*/
//-----------------------------------------------------------------------------
typedef D3DXVECTOR4 D3DScreenVecArray[4];

void ComputeScreenRayConsts(const Camera& camera, D3DScreenVecArray& vec, float farPlaneMul = 1.0f)
{
	//camera world space corners on far plane
	float cLeft = camera.Left();
	float cTop  = camera.Top();
	float cFar  = camera.Far();

	float farPlaneCorner = cFar*sqrt(1 + cLeft*cLeft + cTop*cTop)*farPlaneMul;
	Vector3 dirLT2 = (camera.Direction() + camera.DirectionUp() * cTop - camera.DirectionAside() * cLeft).Normalized()*farPlaneCorner;
	Vector3 dirRT2 = (camera.Direction() + camera.DirectionUp() * cTop + camera.DirectionAside() * cLeft).Normalized()*farPlaneCorner;
	Vector3 dirLB2 = (camera.Direction() - camera.DirectionUp() * cTop - camera.DirectionAside() * cLeft).Normalized()*farPlaneCorner;
	Vector3 dirRB2 = (camera.Direction() - camera.DirectionUp() * cTop + camera.DirectionAside() * cLeft).Normalized()*farPlaneCorner;

	//for vertex shader, the center world pos coordinate computing
	vec[0] = D3DXVECTOR4(dirLT2.x, dirLT2.y, dirLT2.z, 0.0f);
	vec[1] = D3DXVECTOR4(dirRT2.x, dirRT2.y, dirRT2.z, 0.0f);
	vec[2] = D3DXVECTOR4(dirLB2.x, dirLB2.y, dirLB2.z, 0.0f);
	vec[3] = D3DXVECTOR4(dirRB2.x, dirRB2.y, dirRB2.z, 0.0f);
}


//-----------------------------------------------------------------------------
#if _ENABLE_CHEATS
  #define SSAO_DEBUG
#endif

#ifdef SSAO_DEBUG
  static bool ssaoon = true;
  static bool ssaoblurdepth = true;
  static int ssaomode = 0;
  static bool ssaotest = false;
  static bool ssaosave = false;
  static int ssaoparam = 0;
#endif


//-----------------------------------------------------------------------------
void EngineDD9::PPSSAO::Do(float fogEnd, float fogStart, float fogCoef)
{
  #ifdef SSAO_DEBUG
    int plus = 0, minus = 0;

    // lctrl + lalt + 
    if((::GetAsyncKeyState(VK_LCONTROL) & 0x8000) && (::GetAsyncKeyState(VK_LMENU) & 0x8000))
    {
       /* O */ if (::GetAsyncKeyState(0x4F) & 0x8000)    {  ssaoon = !ssaoon; ::Sleep(100); LogF("SSAO %s", ssaoon ? "on" : "off"); }
       /* I */ if (::GetAsyncKeyState(0x49) & 0x8000)    {  ssaomode = ssaomode == 2 ? 0 : ssaomode + 1; ::Sleep(100); LogF("SSAO mode : %s", ssaomode == 0 ? "full (Highest setting)" : ssaomode == 1 ? "full ssao, half blur (High setting)" : "half all (medium setting)" );       switch (ssaomode) 
                   {
                      case 0 : _pInterp._lastPars.SetHighestSetting(); break;
                      case 1 : _pInterp._lastPars.SetHighSetting(); break;
                      case 2 : _pInterp._lastPars.SetMediumSetting(); break;
                   }} 
       /* T */ if (::GetAsyncKeyState('T') & 0x8000)    {  _pInterp._lastPars.m_fIntensity += 0.05f; ::Sleep(100); }
       /* U */ if (::GetAsyncKeyState('U') & 0x8000)    {  _pInterp._lastPars.m_fIntensity -= 0.05f; ::Sleep(100); }

       /* D */ if (::GetAsyncKeyState(0x44) & 0x8000)    {  _bDebug = !_bDebug; ::Sleep(100); LogF("%s", _bDebug ?  "SSAO debug mode" : "SSAO normal mode"); }
       /* U */ if (::GetAsyncKeyState(0x55) & 0x8000)    {  ssaoblurdepth= !ssaoblurdepth; ::Sleep(100); LogF("%s", ssaoblurdepth ?  "SSAO bluring with edge preserving" : "SSAO blur normal mode"); }
       /* S */ if (::GetAsyncKeyState(0x53) & 0x8000)    {  ssaosave = true; ::Sleep(100); LogF("SSAO data saved into c:/"); }
       /* P */ if (::GetAsyncKeyState(0x50) & 0x8000)    {  ssaoparam = ssaoparam == 8 ? ssaoparam = 0 : ssaoparam+1; ::Sleep(100); int p=ssaoparam; LogF("SSAO param : %s = %3.3g", 
                    p==0?"m_fIntensity":
                    p==1?"m_fNearPixelRadius":
                    p==2?"m_fFarPixelRadius":
                    p==3?"m_fCosAngle1":
                    p==4?"m_fCosAngle0s":
                    p==5?"m_fMaxDistance":
                    p==6?"m_fMaxBlurDistance":
                    "blur passes",
                    p==0?_pInterp._lastPars.m_fIntensity:
                    p==1?_pInterp._lastPars.m_fNearPixelRadius:
                    p==2?_pInterp._lastPars.m_fFarPixelRadius:
                    p==3?_pInterp._lastPars.m_fCosAngle1:
                    p==4?_pInterp._lastPars.m_fCosAngle0:
                    p==5?_pInterp._lastPars.m_fMaxDistance:
                    p==6?_pInterp._lastPars.m_fMaxBlurDistance:
                    _pInterp._lastPars.m_fBlurPasses); }

       /* *=+ /=- */
       plus  = GetAsyncKeyState(0xBE/*0x6A*/) & 0x8000;
       minus = GetAsyncKeyState(0xBC/*0x6F*/) & 0x8000; 
    }

    if (!ssaoon)
    { 
      return;
    }

    if (plus|minus)
    {
      switch (ssaoparam)
      {
        case 0 : _pInterp._lastPars.m_fIntensity				= plus ? _pInterp._lastPars.m_fIntensity + 0.1 : floatMax(0.0, _pInterp._lastPars.m_fIntensity - 0.1); LogF("SSAO intensity %3.3g", _pInterp._lastPars.m_fIntensity); break;
        case 1 : _pInterp._lastPars.m_fNearPixelRadius	= plus ? floatMin(1.0f, _pInterp._lastPars.m_fNearPixelRadius + 0.001f) : floatMax(0.0, _pInterp._lastPars.m_fNearPixelRadius - 0.001); LogF("SSAO m_fNearPixelRadius %3.3g", _pInterp._lastPars.m_fNearPixelRadius); break;
        case 2 : _pInterp._lastPars.m_fFarPixelRadius		= plus ? floatMin(1.0f, _pInterp._lastPars.m_fFarPixelRadius + 0.001) : floatMax(0.0, _pInterp._lastPars.m_fFarPixelRadius - 0.001); LogF("SSAO m_fFarPixelRadius %3.3g", _pInterp._lastPars.m_fFarPixelRadius); break;
        case 3 : _pInterp._lastPars.m_fCosAngle1				= plus ? floatMin(200.0f, _pInterp._lastPars.m_fCosAngle1 + 1.0) : floatMax(2.0, _pInterp._lastPars.m_fCosAngle1 - 1.0f); LogF("SSAO m_fCosAngle1 %3.3g", _pInterp._lastPars.m_fCosAngle1); break;
        case 4 : _pInterp._lastPars.m_fCosAngle0				= plus ? floatMin(200.0f, _pInterp._lastPars.m_fCosAngle0 + 1.0) : floatMax(1.0, _pInterp._lastPars.m_fCosAngle0 - 1.0f); LogF("SSAO m_fCosAngle0 %3.3g", _pInterp._lastPars.m_fCosAngle0); break;
        case 5 : _pInterp._lastPars.m_fMaxDistance      = plus ? floatMin(2.0f, _pInterp._lastPars.m_fMaxDistance + 0.005) : floatMax(0.0f, _pInterp._lastPars.m_fMaxDistance - 0.005f); LogF("SSAO m_fMaxDistance %3.3g", _pInterp._lastPars.m_fMaxDistance); break;
        case 6 : _pInterp._lastPars.m_fMaxBlurDistance  = plus ? floatMin(200.0f, _pInterp._lastPars.m_fMaxBlurDistance + 0.1) : floatMax(0.1f, _pInterp._lastPars.m_fMaxBlurDistance - 0.1f); LogF("SSAO m_fMaxBlurDistance %3.3g", _pInterp._lastPars.m_fMaxBlurDistance); break;
        case 7 : _pInterp._lastPars.m_fBlurPasses				= plus ? floatMin(4, _pInterp._lastPars.m_fBlurPasses + 1) : floatMax(0, _pInterp._lastPars.m_fBlurPasses - 1); LogF("SSAO blur passes %d", _pInterp._lastPars.m_fBlurPasses); Sleep(200); break;
      }
    }
  #endif

  //DoAssert(!CheckMainThread()); // can be main thread when not HL recording
  if (_engine->_d3DDevice.IsNull()) 
		return;
  ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

  DoAssert(device.NotNull());
  if (device.IsNull()) 
		return;

	_SSAOparams = _pInterp.Simulate();
  if (_SSAOparams.m_fIntensity < 0.001f || !_enabled) 
		return;

  const PPSSAOResources *res = Resources();
  if (!res || res->_vb.IsNull()) 
		return;

  PROFILE_SCOPE_GRF_EX(ppSSAO,pp,SCOPE_COLOR_RED);

  if (_bHalfResOld != _SSAOparams.m_bHalfRes || _bBlurHalfResOld != _SSAOparams.m_bBlurHalfRes)
  {
    SetRT(device.GetRef(), (PPSSAOResources *) res, _SSAOparams.m_bHalfRes, _SSAOparams.m_bBlurHalfRes);
  }

  bool halfres  = _SSAOparams.m_bHalfRes;
  bool blurhalfres = _SSAOparams.m_bBlurHalfRes;
  bool smallres = blurhalfres && !halfres;

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) device->BeginScene();
  DoAssert(inScene);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "SSAO: ");

  // Prepare drawing
  device->SetStreamSource(0, 
#if _ARMA3_VERTEX_BUFFER_EX
    res->_vb.GetResult().GetBuffer(),
#else
    res->_vb.GetResult(),
#endif
    0, sizeof(VertexP3T2));
  device->SetVertexDeclaration(res->_vd);
  device->SetVertexShader(res->_vsSSAO);

  device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  //render states
  device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  device->SetRenderState(D3DRS_ZENABLE, FALSE);
  device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
  device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

  //sampler states, depth texture
  IDirect3DTexture9* depth = _engine->GetDepthAsTexture();

#ifdef SSAO_DEBUG
  if (ssaosave)
  {
    D3DXSaveTextureToFile("c:/depth.dds", D3DXIFF_DDS, depth, NULL);
  }
#endif

  device->SetTexture(0, depth);

  device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
  device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
  device->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0xFFFFFFFF);
  device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);

  //computed ssao
  device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, 0);

  //noise
	device->SetTexture(2, GNoiseRotationTexture.Get());

  device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  device->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, 0);

  float depthwidth	= _engine->Width();
  float depthheight	= _engine->Height();

	float invDepthWidth		= 1.0f/depthwidth;
	float invDepthHeight	= 1.0f/depthheight;
	
  //---------------------------------------------------------------------------------------
  //render SSAO info RTTSSAO buffer
  //---------------------------------------------------------------------------------------
  float rendwidth	  = res->_SSAOWidth;
  float rendheight	= res->_SSAOHeight;

  float noisewidth	= res->_texWidth;
  float noiseheight	= res->_texHeight;

  D3DXVECTOR4 vec191192[2];
  //half pixel offsets for current and for noise texture, VSH
  vec191192[0] = D3DXVECTOR4(
    0.5f/depthwidth,
    0.5f/depthheight,
    0.5f/noisewidth,
    0.5f/noiseheight);

  //to convert from <0..1> screen space into noise texture space
  vec191192[1] = D3DXVECTOR4(
    rendwidth/noisewidth,
    rendheight/noiseheight,
    0.0f,
    0.0f);

	//PS vectors
  D3DXVECTOR4 vec[5];

	float farPlane				= GScene->GetCamera()->Far();
	float linearFarPlane	= Glob.config.GetMaxLinearDepth();

  vec[0] = D3DXVECTOR4(
    invDepthWidth,
    invDepthHeight,
    0.0f,
    0.0f);

	// xyScale = ssaoPixelOffset*FarMulNear/(DeltaRadius*centerDepth + Far) -> XY/(Z*depth + W)
  float radNear = _SSAOparams.m_fNearPixelRadius;
  float radFar  = _SSAOparams.m_fFarPixelRadius;
  vec[1] = D3DXVECTOR4(
		invDepthWidth*radNear*radFar,
		invDepthHeight*radNear*radFar,
		radNear - radFar,
    radFar);

	//ssao limit angle and distances, the distance attenuation in shader is computed using the function 1/(1 + c*x). If using
	//sqr(dist) in PS then final intensity multiplicator must be about 4 times less due to different function flow. The square 
	//of distance gives much more weight to nearest pixels...
	float angleK, angleQ;
	enf::GetLineEq(_SSAOparams.m_fCosAngle1, _SSAOparams.m_fCosAngle0, angleK, angleQ);
 
  vec[2] = D3DXVECTOR4(
    angleK,
    angleQ,
		(1.0f/0.01f - 1.0f)/_SSAOparams.m_fMaxDistance,				//1/(1 + param*dist) = 0.01 -> param = (1.0f/0.01f - 1.0)/dist
		1.0f);	//not free const!

  //image operations, intensity = FullIntensity*(NearIntensity + centerDepth*(FarIntensity - NearIntensity))
	static const float NearIntensity =  1.0;
	static const float FarIntensity  =  10.0;
	
	float intensity = _SSAOparams.m_fIntensity*exp(fogCoef*10);  //fog in 10 m distance
	float depthFarPlane =  Glob.config.GetMaxLinearDepth();
	float invDepthFarPlane = Glob.config.GetMaxInvLinearDepth();

  vec[3] = D3DXVECTOR4(
    intensity*NearIntensity,
    intensity*(FarIntensity - NearIntensity),
    (1.0f/0.01f - 1.0f)/(_SSAOparams.m_fMaxBlurDistance*invDepthFarPlane),	
    0.0f);

	//fog
	float endf   = fogEnd*invDepthFarPlane;
	float startf = fogStart*invDepthFarPlane;

	vec[4] = D3DXVECTOR4(
		endf,
		1.0f / (endf - startf),
		fogCoef*depthFarPlane,
		0.0f);


	//world screen coordinates
	D3DScreenVecArray screenVecs;
	ComputeScreenRayConsts(*GScene->GetCamera(), screenVecs, linearFarPlane/farPlane);

  //set constants
  device->SetPixelShaderConstantF(0, vec[0], 5);
  device->SetPixelShaderConstantF(5, screenVecs[0], 4);

	device->SetVertexShaderConstantF(VSC_SSAOCorners, screenVecs[0], 4);
	device->SetVertexShaderConstantF(VSC_SSAOPar1, vec191192[0], 2);

  ComRef<IDirect3DSurface9> origRT;
  device->GetRenderTarget(0, origRT.Init());

  ComRef<IDirect3DSurface9> origDepth;
  device->GetDepthStencilSurface(origDepth.Init());


#ifdef SSAO_DEBUG
  if (ssaosave)
    D3DXSaveSurfaceToFile("C:\\origRT.png", D3DXIFF_PNG, origRT, 0, 0);
#endif

	device->SetRenderTarget(0, res->_rtsSSAO);
  device->SetDepthStencilSurface(NULL);

  device->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);

  if (halfres)
  {
    D3DVIEWPORT9 viewData;
    memset(&viewData,0,sizeof(viewData));
    viewData.X = 0;
    viewData.Y = 0;
    viewData.Width  = res->_SSAOWidth;
    viewData.Height = res->_SSAOHeight;
    viewData.MinZ = 0.0f;
    viewData.MaxZ = 1.0f;
    device->SetViewport(&viewData);
  }

	int passes = intMinMax(_SSAOparams.m_fSSAOPasses - EngineDD9::PPSSAOResources::MIN_SSAO_PASSES, EngineDD9::PPSSAOResources::MIN_SSAO_PASSES, EngineDD9::PPSSAOResources::MAX_SSAO_PASSES);

  device->SetVertexShader(res->_vsSSAO);
  device->SetPixelShader(res->_psSSAO[passes]);

  //draw 
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  SSAO pass: ");
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  #ifdef SSAO_DEBUG
  if (ssaosave)
  {
    D3DXSaveTextureToFile("c:/ssao.tga", D3DXIFF_TGA, res->_rtSSAO, NULL);
    D3DXSaveTextureToFile("c:/depth.dds", D3DXIFF_DDS, depth, NULL);
  }
  #endif
  //---------------------------------------------------------------------------------------
  //to half resolution
  //---------------------------------------------------------------------------------------
  if (smallres)
  {
    device->SetRenderTarget(0, res->_rtsTemp);

    D3DVIEWPORT9 viewData;
    memset(&viewData,0,sizeof(viewData));
    viewData.X = 0;
    viewData.Y = 0;
    viewData.Width  = res->_blurWidth;
    viewData.Height = res->_blurHeight;
    viewData.MinZ = 0.0f;
    viewData.MaxZ = 1.0f;
    device->SetViewport(&viewData);

     D3DXVECTOR4 vec191b = D3DXVECTOR4 (
      0.5/rendwidth, 
      0.5/rendheight, 
      1.0/rendwidth, 
      1.0/rendheight);

    device->SetTexture(0, res->_rtSSAO);
    device->SetVertexShaderConstantF(VSC_SSAOPar1, &vec191b[0], 1);

    device->SetVertexShader(res->_vsSSAODown);
    device->SetPixelShader(res->_psSSAODown);

    //draw 
    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
   

    #ifdef SSAO_DEBUG
      if (ssaosave)
      {
        D3DXSaveTextureToFile("c:/small.tga", D3DXIFF_TGA, res->_rtTemp, NULL);
      }
    #endif
  }

  //---------------------------------------------------------------------------------------
  //blur image
  //---------------------------------------------------------------------------------------
	int blurDepthPasses = _SSAOparams.m_fDepthBlurPasses;
	int blurPasses = _SSAOparams.m_fBlurPasses + blurDepthPasses;
	if (blurPasses > 0)
		device->SetVertexShader(res->_vsSSAOBlur);

	float pixelWidth  = smallres ? 1.0/res->_blurWidth  : 1.0/rendwidth;
	float pixelHeight = smallres ? 1.0/res->_blurHeight : 1.0/rendheight;

	float depthPixelWidth  = smallres ? 2.0/depthwidth  : 1.0/depthwidth;
	float depthPixelHeight = smallres ? 2.0/depthheight : 1.0/depthheight;

	bool useTemp = blurPasses && !blurhalfres && halfres;

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Bluring: ");

  for (int i = 0; i < blurPasses; i++)
  {
    //horizontal blur   
		D3DXVECTOR4 vecBlur[2];
		vecBlur[0] = D3DXVECTOR4(
			pixelWidth, 
			0.0f, 
			0.5f*pixelWidth, 
			0.0f);	

		vecBlur[1] = D3DXVECTOR4(
			depthPixelWidth, 
			0.0f, 
			0.5*depthPixelWidth, 
			0.0f);	

		device->SetVertexShaderConstantF(VSC_SSAOPar3, vecBlur[0], 2);
    device->SetRenderTarget(0, res->_rtsBlur);

    device->SetPixelShader(i < blurDepthPasses ? res->_psBlurHorzDepth : res->_psBlurHorz);
    
    device->SetTexture(1, smallres || (useTemp && i) ? res->_rtTemp : res->_rtSSAO);
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2); 
   
		//vertical blur
		vecBlur[0] = D3DXVECTOR4(
			0.0f, 
			pixelHeight, 
			0.0f, 
			0.5*pixelHeight);	

		vecBlur[1] = D3DXVECTOR4(
			0.0f, 
			depthPixelHeight, 
			0.0f, 
			0.5*depthPixelHeight);	

		device->SetVertexShaderConstantF(VSC_SSAOPar3, vecBlur[0], 2);

    device->SetRenderTarget(0, smallres || useTemp ? res->_rtsTemp : res->_rtsSSAO);

    device->SetPixelShader(i < blurDepthPasses ?  res->_psBlurVertDepth : res->_psBlurVert);
   
    device->SetTexture(1, res->_rtBlur);
    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }

	GPU_END_PROFILE

  //---------------------------------------------------------------------------------------
  //render SSAO into final image
  //---------------------------------------------------------------------------------------
  #ifdef SSAO_DEBUG
  if (ssaosave)
  {
    D3DXSaveTextureToFile("c:/blur.tga", D3DXIFF_TGA, smallres || useTemp ? res->_rtTemp : res->_rtSSAO, NULL);  
    ssaosave = false;
  }
  #endif

	if (blurPasses)
		device->SetVertexShader(res->_vsSSAO);

  // _engine->RestoreOldRenderTarget();
  // Restore original RT & Depth
  device->SetRenderTarget(0, origRT);
  device->SetDepthStencilSurface(origDepth);

  D3DVIEWPORT9 viewData;
  memset(&viewData,0,sizeof(viewData));
  viewData.X = 0;
  viewData.Y = 0;
  viewData.Width  = _engine->Width();
  viewData.Height = _engine->Height();
  viewData.MinZ = 0.0f;//_engine->_minZRange;
  viewData.MaxZ = 1.0f;//_engine->_maxZRange;
  device->SetViewport(&viewData);

  D3DXVECTOR4 vec191c = D3DXVECTOR4(
		smallres ? 0.5/res->_blurWidth: 0.5/rendwidth, 
		smallres ? 0.5/res->_blurHeight : 0.5/rendheight, 
		0.0, 
		0.0);	
 
	device->SetVertexShaderConstantF(VSC_SSAOPar1, &vec191c[0], 1);
 
	device->SetTexture(1, smallres || useTemp ? res->_rtTemp : res->_rtSSAO);
   
  device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
  device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  device->SetRenderState(D3DRS_SRCBLEND, _bDebug ? D3DBLEND_SRCCOLOR : D3DBLEND_ZERO);
  device->SetRenderState(D3DRS_DESTBLEND, _bDebug ? D3DBLEND_ZERO : D3DBLEND_SRCCOLOR);
 
  device->SetPixelShader(_bDebug ? res->_psSSAODebug : res->_psSSAOFinal);
 
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Combining: ");
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

	GPU_END_PROFILE

	//---------------------------------------------------------------------------------------
	//finalize as it's in normal pipeline
	//---------------------------------------------------------------------------------------
  if (!inScene) 
		device->EndScene();

  device->SetPixelShader(NULL);
  device->SetVertexShader(NULL);

  // restore SamplerState 0, fixed: news:hrc1q6$5ah$1@new-server.localdomain
  device->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0x0);

  // restore RenderState
  device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  device->SetRenderState(D3DRS_ZENABLE, TRUE);
  device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
  device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
  // SetPixelShaderConstant_A_DFORCED_E_Black
  Color newE = HBlack;
  Color zero = Color(0, 0, 0, 0);

  device->SetPixelShaderConstantF(PSC_AE, (float*)&newE, 1);
  device->SetPixelShaderConstantF(PSC_GE, (float*)&newE, 1);
  device->SetPixelShaderConstantF(PSC_DForced, (float*)&zero, 1);

  // SetPixelShaderConstantDBSZero
  static const Color zeroes[3]={Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0)};
  device->SetPixelShaderConstantF(PSC_Diffuse, (float*)zeroes, 3);
}


//-----------------------------------------------------------------------------
/*!
 FilmGrain postprocess effect
*/
//-----------------------------------------------------------------------------
void EngineDD9::PPFilmGrain::Prepare()
{
  const PPFilmGrainResources *res = Resources();
  if (!res) return;

  const int cb = -1;
  base::Prepare();

  _engine->SetStreamSource(cb, res->_vb, sizeof(VertexP3T2), 4);
  _engine->SetVertexDeclaration(cb, res->_vd);
  _engine->SetVertexShader(cb, res->_vs);
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPFilmGrain::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit)
{
  if (!base::Init(hlsl,vs)) return false;
  
  Ref<PPFilmGrainResources> resrc = new PPFilmGrainResources;

  resrc->_invtexWidth    = 1.0f;
  resrc->_invtexHeight   = 1.0f;

  resrc->_texWidth       = 1;
  resrc->_texHeight      = 1;

  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }

  Vertex *v;
  float offsetU = 1.0f / _engine->Width() * 0.5f; //See the Nearest-Point Sampling in the DX9 documentation
  float offsetV = 1.0f / _engine->Height() * 0.5f;
  vb->Lock(0, 0, (void**)&v, 0);
  v[0].pos = Vector3P(-0.5, -0.5, 0.5); v[0].t0.u = 0 + offsetU; v[0].t0.v = 1 + offsetV;
  v[1].pos = Vector3P(-0.5,  0.5, 0.5); v[1].t0.u = 0 + offsetU; v[1].t0.v = 0 + offsetV;
  v[2].pos = Vector3P( 0.5, -0.5, 0.5); v[2].t0.u = 1 + offsetU; v[2].t0.v = 1 + offsetV;
  v[3].pos = Vector3P( 0.5,  0.5, 0.5); v[3].t0.u = 1 + offsetU; v[3].t0.v = 0 + offsetV;
  vb->Unlock();

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }
  if (!CompileVertexShader(hlsl, "VSPostProcess", resrc->_vs))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessFilmGrainColor", resrc->_psColor, false))
    return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessFilmGrainMono", resrc->_psMono, false))
    return false;

	GNoiseUniformTexture.Create(_engine->GetDirect3DDevice().GetRef());

  // assign to shared resources
  resrc->_vd = vd;
  resrc->_vb = VertexStaticData9::BufferFuture(vb);

  resrc->_invtexWidth  = 1.0f/GNoiseUniformTexture.GetWidth();
  resrc->_invtexHeight = 1.0f/GNoiseUniformTexture.GetHeight();

  resrc->_texWidth     = GNoiseUniformTexture.GetWidth();
  resrc->_texHeight    = GNoiseUniformTexture.GetHeight();

  _resrc = resrc;

  if (firstInit)
  {
    SetDefaultValues();
    _enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPFilmGrain::SetDefaultValues()
{
  _pInterp._pars._sharpness     = 1.5f;
  _pInterp._pars._intensity	    = 0.1f;
  _pInterp._pars._grainSize	    = 1.7f;
  _pInterp._pars._intensityX0	  = 0.2f;
  _pInterp._pars._intensityX1	  = 1.0f;

  _pInterp._pars._monochromatic = true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPFilmGrain::Init(PPResources *resources, bool firstInit)
{
  if (resources)
  {
    _resrc = resources;

    if (firstInit)
    {
      SetDefaultValues();
      _enabled = false;
      _pInterp.Initialize();
    }

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPFilmGrain::UnInit()
{
  if (_resrc)
  {
		GNoiseUniformTexture.Free();
    _resrc->UnInit();

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPFilmGrain::SetParams(const float *pars, int nPars)
{
  if(nPars > 0)
    setIntensity(pars[0]);

  if(nPars > 1)
    setSharpness(pars[1]);

  if(nPars > 2)
    setGrainSize(pars[2]);

  if(nPars > 3)
    setIntensityX0(pars[3]);

  if(nPars > 4)
    setIntensityX1(pars[4]);

  if(nPars > 5)
    SetMonochromatic(fabs(pars[4]) < 0.01f);

  return 0;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPFilmGrain::SetParams(const AutoArray<float> &pars)
{
  int nPars = pars.Size();

  if(nPars > 0)
    setIntensity(pars[0]);

  if(nPars > 1)
    setSharpness(pars[1]);

  if(nPars > 2)
    setGrainSize(pars[2]);

  if(nPars > 3)
    setIntensityX0(pars[3]);

  if(nPars > 4)
    setIntensityX1(pars[4]);

  if(nPars > 5)
    SetMonochromatic(fabs(pars[4]) < 0.01f);

  return 0;
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPFilmGrain::Serialize( ParamArchive &ar )
{
  CHECK(ar.Serialize("enabled",_enabled,0,false));
  CHECK(ar.Serialize("interp", _pInterp, 0));
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFilmGrain::Do(bool isLast)
{
  if (!_enabled)
    return;

  // Draw
  PROFILE_SCOPE_GRF_EX(ppFilmGrain,pp,SCOPE_COLOR_YELLOW);

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  const PPFilmGrainResources *res = Resources();
  if (!res) return;
  
  //interpolated parameters
  FilmGrainPar pars = _pInterp.Simulate();

  //prepare drawing
  Prepare();
  const int cb = -1;

  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  //stage 0 - base texture - do the copy (to make sure same data is both in source texture and destination render target)
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();

  //if effect is last then switch to back buffer
  if (isLast) 
		_engine->SwitchToBackBufferRenderTarget();

  //0 texture is source frame buffer
  SetTextureTemp tTex0(0, rt);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MIPFILTER(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  //1 texture is uniform noise
  SetTextureTemp tTex1(1, GNoiseUniformTexture.Get());
  SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MIPFILTER(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

  //noise texture 
  float randmovew = toIntFloor(GRandGen.RandomValue()*res->_texWidth);
  float randmoveh = toIntFloor(GRandGen.RandomValue()*res->_texHeight);

  float texw      = (float) _engine->Width()/ (float) res->_texWidth;
  float texh      = (float) _engine->Height()/ (float) res->_texHeight;

  //parameters
  static bool  stop = false;

  if (stop)
  {
    randmoveh = 0.0f;
    randmovew = 0.0f;
  }

  D3DXVECTOR4 vec1 = D3DXVECTOR4(
    pars._intensity, 
    pars._sharpness, 
    pars._grainSize - 1.0f, 
    0.0f);

  //to compute intensity of pixel according the line, function y = kx + q = saturate((X1 - X0)*x + X0),
  //where X0 is intensity of black pixel and X1 is intensity of white pixel
  D3DXVECTOR4 vec2 = D3DXVECTOR4(
    pars._intensityX1 - pars._intensityX0, 
    pars._intensityX0, 
    0.0f, 
    0.0f);

  SetPixelShaderConstantTemp PS0(0, D3DXVECTOR4(texw, texh, texw*res->_invtexWidth*(0.5f + randmovew), texh*res->_invtexHeight*(0.5f + randmoveh)));
  SetPixelShaderConstantTemp PS1(1, vec1);
  SetPixelShaderConstantTemp PS2(2, vec2);

  //pixel shader according the mode
  _engine->SetPixelShader(cb, pars._monochromatic ? res->_psMono : res->_psColor);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "FilmGrain: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  if (!inScene) _engine->D3DEndScene();
  Finish();
}


//-----------------------------------------------------------------------------
/*!
 GodRays postprocess effect
*/
//-----------------------------------------------------------------------------
bool EngineDD9::PPGodRays::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit)
{
	if (!base::Init(hlsl,vs)) return false;

	Ref<PPGodRaysResources> resrc = new PPGodRaysResources;

	// Create the VB
	ComRef<IDirect3DVertexBuffer9> vb;
	if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
	{
		LogF("Error: Vertex buffer creation failed.");
		return false;
	}

	Vertex *v;
	vb->Lock(0, 0, (void**)&v, 0);
		v[0].x = -1.0f;	v[0].y = -1.0f;
		v[1].x = -1.0f;	v[1].y =  1.0f;
		v[2].x =  1.0f;	v[2].y = -1.0f;
		v[3].x =  1.0f;	v[3].y =  1.0f;
	vb->Unlock();

	// Create the VD
	const D3DVERTEXELEMENT9 vertexDecl[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};

	ComRef<IDirect3DVertexDeclaration9> vd;
	if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
	{
		LogF("Error: Vertex declaration creation failed.");
		return false;
	}

	if (!CompileVertexShader(vs, "VSGodRays", resrc->_vs))
		return false;

	if (!CompileVertexShader(vs, "VSGodRaysShrink", resrc->_vsShrink))
		return false;

	if (!CompilePixelShader(hlsl,"PSPostProcessGodRaysMask", resrc->_psMask, false))
		return false;

	if (!CompilePixelShader(hlsl,"PSPostProcessGodRaysMaskNoHor", resrc->_psMaskNoHor, false))
		return false;

	if (!CompilePixelShader(hlsl,"PSPostProcessGodRaysDiletate", resrc->_psDiletate, false))
		return false;

	if (!CompilePixelShader(hlsl,"PSPostProcessGodRaysShrink", resrc->_psShrink, false))
		return false;

	if (!CompilePixelShader(hlsl,"PSPostProcessGodRaysBlur", resrc->_psBlur, false))
		return false;

	if (!CompilePixelShader(hlsl,"PSPostProcessGodRaysFinal", resrc->_psFinal, false))
		return false;

	//render target for mask
	int fullWidth = _engine->Width();
	int fullHeight = _engine->Height();

	HRESULT err;

	//playing with multisampling
	bool msaa = _engine->_rtMultiSampleType != D3DMULTISAMPLE_NONE;
	resrc->_rtsMSAAMask.Free();
	if (msaa)
	{
		err = _engine->CreateRenderTarget(fullWidth, fullHeight, D3DFMT_A8R8G8B8, _engine->_rtMultiSampleType, _engine->_rtMultiSampleQuality, FALSE, resrc->_rtsMSAAMask, false, NULL);
		if (err != D3D_OK) 
		{
			_engine->DDError9("Error: Render target texture creation failed.", err);
			return false;
		}
	}

	err = _engine->CreateTextureRT(fullWidth, fullHeight, 1, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, resrc->_rtMask, resrc->_rtsMask, false);
	if (err != D3D_OK) 
	{
		_engine->DDError9("Error: Render target texture creation failed.", err);
		return false;
	}
	for (int i = 0; i < 2; i++)
	{
		err = _engine->CreateTextureRT(fullWidth >> _smallMaskLOD, fullHeight >> _smallMaskLOD, 1, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, resrc->_rtSmallMask[i], resrc->_rtsSmallMask[i], true);
		if (err != D3D_OK) 
		{
			_engine->DDError9("Error: Render target texture creation failed.", err);
			return false;
		}
	}

	// assign to shared resources
	resrc->_vd = vd;
	resrc->_vb = VertexStaticData9::BufferFuture(vb);

	_resrc = resrc;

	if (firstInit)
	{
		SetDefaultValues();
		_enabled = false;
		_pInterp.Initialize();
	}

	return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
	UnInit();
	Init(hlsl, vs, false);
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::SetTexture(TextureD3D9 *tex)
{
	const PPGodRaysResources* res = static_cast<const PPGodRaysResources*>(_resrc->GetType());
	if (res)
	{
		const_cast<PPGodRaysResources*>(res)->_kernelTexture = tex;
		_engine->TextBank()->UseMipmap(tex, 0, 0);
	}
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::Cleanup()
{
	const PPGodRaysResources* res = static_cast<const PPGodRaysResources*>(_resrc->GetType());
	if (res)
	{
		const_cast<PPGodRaysResources*>(res)->_kernelTexture.Free();
	}
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::SetDefaultValues()
{
	_pInterp._pars.SetDefault();
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPGodRays::Init(PPResources *resources, bool firstInit)
{
	if (resources)
	{
		_resrc = resources;

		if (firstInit)
		{
			SetDefaultValues();
			_enabled = false;
			_pInterp.Initialize();
		}

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPGodRays::UnInit()
{
	if (_resrc)
	{
		_resrc->UnInit();

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPGodRays::SetParams(const float *pars, int nPars, bool imm)
{
	if(nPars > 0)
		setIntensity(pars[0], imm);

	if(nPars > 1)
		setNormSize(pars[1], imm);

	if(nPars > 2)
		setBlurSize(pars[2], imm);

	if(nPars > 3)
		setOverBurnIntensity(pars[3], imm);

	if(nPars > 4)
		setOverBurnStart(pars[4], imm);

	if(nPars > 5)
		setOverBurnEnd(pars[5], imm);

	if(nPars > 6)
		setBlurPasses(pars[6], imm);

	return 0;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPGodRays::SetParams(const AutoArray<float> &pars)
{
	return SetParams(pars.Data(), pars.Size());
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPGodRays::Serialize( ParamArchive &ar )
{
	CHECK(ar.Serialize("enabled",_enabled,0,false));
	CHECK(ar.Serialize("interp", _pInterp, 0));
	CHECK(ar.Serialize("prior", _priority, 0, -1));

	return LSOK;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPGodRaysResources::IsValid() const
{
	MipInfo info = ::GEngine->TextBank()->UseMipmap(_kernelTexture, 0, 0);
	return info._texture == _kernelTexture && info._level == 0;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::Prepare()
{
	const PPGodRaysResources *res = Resources();
	if (!res) 
		return;

	const int cb = -1;
	base::Prepare();

	_engine->SetStreamSource(cb, res->_vb, sizeof(Vertex), 4);
	_engine->SetVertexDeclaration(cb, res->_vd);
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::CreateDepthMask(Vector3Par sunCenter, float ratio, float normSize)
{
	PROFILE_SCOPE_GRF_EX(ppMakeDepthMask,pp,SCOPE_COLOR_YELLOW);

	const PPGodRaysResources *res = Resources();
	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

	if (res->_kernelTexture)
		device->SetTexture(0, res->_kernelTexture->GetHandle());

	//_engine->_currRenderTargetDepthBufferSurface
	bool msaa = _engine->_rtMultiSampleType != D3DMULTISAMPLE_NONE;
	ComRefFake<IDirect3DSurface9> origDepth = msaa ? _engine->_depthBufferRT : _engine->_depthBuffer;	
	device->SetRenderTarget(0, msaa ? res->_rtsMSAAMask : res->_rtsMask);
	device->SetDepthStencilSurface(origDepth);

	device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);

	D3DXVECTOR4 center = D3DXVECTOR4(
		sunCenter[0],
		sunCenter[1], 
		normSize*ratio, 
		normSize);

	device->SetVertexShaderConstantF(VSC_GodRaysCenterSize, center, 1);
	device->SetVertexShaderConstantF(VSC_GodRaysTexOffset, D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f), 1);

	device->SetPixelShader(res->_psMask);
	device->SetVertexShader(res->_vs);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Depth mask: ");
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

	//from multisample RT to normal RT
	if (msaa)
		device->StretchRect(res->_rtsMSAAMask, NULL, res->_rtsMask, NULL, D3DTEXF_POINT);
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::ShrinkDepthMask(float invWidth, float invHeight)
{
	PROFILE_SCOPE_GRF_EX(ppShrinkDepthMask,pp,SCOPE_COLOR_YELLOW);

	const PPGodRaysResources *res = Resources();
	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

	rtIndex.Advance();

	device->SetTexture(0, res->_rtMask);
	device->SetRenderTarget(0, res->_rtsSmallMask[rtIndex.FreeRT()]);

	device->SetVertexShaderConstantF(VSC_GodRaysTexOffset, D3DXVECTOR4(invWidth, invHeight, invWidth, invHeight), 1);
	device->SetVertexShader(res->_vsShrink);
	device->SetPixelShader(res->_psShrink);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "	Shrink depth mask: ");
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::DiletateDepthMask(float invWidth, float invHeight)
{
	PROFILE_SCOPE_GRF_EX(ppDiletateDepthMask,pp,SCOPE_COLOR_YELLOW);

	const PPGodRaysResources *res = Resources();
	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

	rtIndex.Advance();

	device->SetTexture(0, NULL);
	device->SetRenderTarget(0, res->_rtsSmallMask[rtIndex.FreeRT()]);
	device->SetTexture(0, res->_rtSmallMask[rtIndex.FullRT()]);

	static const D3DXVECTOR4 normOffsets[5] = 
	{
		D3DXVECTOR4(-1.0f, -1.0f,			  0.0f, -1.0f),
		D3DXVECTOR4( 1.0f, -1.0f,			 -1.0f,  0.0f),
		D3DXVECTOR4( 0.0f,  0.0f,			  1.0f,  0.0f),
		D3DXVECTOR4(-1.0f,  1.0f,				0.0f,  1.0f),
		D3DXVECTOR4( 1.0f,  1.0f,			  0.0f,  0.0f),
	};
	
	D3DXVECTOR4 offsets[5];

	for (int i = 0; i < 5; i++)
	{
		offsets[i].x = normOffsets[i].x*invWidth;
		offsets[i].y = normOffsets[i].y*invHeight;
		offsets[i].z = normOffsets[i].z*invWidth;
		offsets[i].w = normOffsets[i].w*invHeight;
	}

	device->SetPixelShaderConstantF(0, &offsets->x, 9);
	device->SetVertexShaderConstantF(VSC_GodRaysTexOffset, D3DXVECTOR4(0.5f*invWidth, 0.5f*invHeight, invWidth, invHeight), 1);
	device->SetVertexShader(res->_vs);
	device->SetPixelShader(res->_psDiletate);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "	Diletate: ");
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::BlurDepthMask(Vector3Par projScreen, float width, float height, float mult, const GodRaysPar& pars)
{
	PROFILE_SCOPE_GRF_EX(ppBlurDepthMask,pp,SCOPE_COLOR_YELLOW);

	const PPGodRaysResources *res = Resources();
	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

	static const float blurCoefs[] = {0.125f, 0.1f, 0.08f};
	
	device->SetVertexShader(res->_vs);
	device->SetPixelShader(res->_psBlur);

	float invWidth  = 1.0f/width;
	float invHeight = 1.0f/height;

	float invBlurSize = 1.0f/(1 << _smallMaskLOD);

	device->SetPixelShaderConstantF(0, D3DXVECTOR4(projScreen[0]*invBlurSize*mult, projScreen[1]*invBlurSize*mult, 0, 0), 1);
	device->SetPixelShaderConstantF(2, D3DXVECTOR4(invWidth, invHeight, 0.5*invWidth, 0.5*invHeight), 1);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "	Radial blur: ");
	for (int i = 0; i < pars._blurPasses; i++)
	{
		rtIndex.Advance();
	
		device->SetTexture(0, NULL);
		device->SetRenderTarget(0, res->_rtsSmallMask[rtIndex.FreeRT()]);
		device->SetTexture(0, res->_rtSmallMask[rtIndex.FullRT()]);

		float x = pars._blurSize*blurCoefs[i];
		device->SetPixelShaderConstantF(1, D3DXVECTOR4(x*invWidth, x*invHeight, 0, 0), 1);
		
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	}
	GPU_END_PROFILE
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGodRays::Do(bool isLast)
{
	if (!_enabled)
		return;

	if (_engine->_d3DDevice.IsNull()) 
		return;

	const PPGodRaysResources *res = Resources();
	if (!res || !res->IsValid()) 
		return;

	const Camera& camera = *GScene->GetCamera();
	const LightSun& sun  = *GScene->MainLight();

	Vector3 eyeLightPos = camera.Position() - sun.SunDirection();
	Vector3 projScreen  = GScene->ProjectLightPos(eyeLightPos);

	if (projScreen[2] < 0.0)
		return;

	int scWidth  = _engine->_w;
	int scHeight = _engine->_h;

	if (scWidth < 1 || scHeight < 1)
		return;

	float invSCWidth = 1.0f/scWidth;
	float invSCHeight = 1.0f/scHeight;

	float mult = _engine->_wRT*invSCWidth;

	Vector3 projScreenNorm = Vector3(projScreen[0]*invSCWidth*2.0f - 1.0f, -(projScreen[1]*invSCHeight*2.0f - 1.0f), 0.0f);

	PROFILE_SCOPE_GRF_EX(ppGodRays,pp,SCOPE_COLOR_YELLOW);
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "GodRays: ");

	// If scene is not being rendered initialize scene
	bool inScene = _engine->_d3dFrameOpen;
	if (!inScene) 
		_engine->D3DBeginScene();

	//interpolated parameters
	GodRaysPar pars = _pInterp.Simulate();

	// Prepare drawing
	Prepare();

	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();
	ComRef<IDirect3DSurface9> origRT;
	device->GetRenderTarget(0, origRT.Init());
	
	ComRefFake<IDirect3DSurface9> origDepth = _engine->_rtMultiSampleType == D3DMULTISAMPLE_NONE ? _engine->_depthBuffer : _engine->_depthBufferRT;
	IDirect3DTexture9 *screenRT = _engine->CreateRenderTargetCopyAsTexture();

	if (isLast) 
		_engine->SwitchToBackBufferRenderTarget();

	//common RS
	//using device->Set...brings problems in next PP effects (in those which use engine Set methods )
	SetRenderStateTemp rs1(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
	SetRenderStateTemp rs2(D3DRS_CULLMODE, D3DCULL_NONE);
	SetRenderStateTemp rs3(D3DRS_ALPHATESTENABLE, false);
	SetRenderStateTemp rs4(D3DRS_ALPHABLENDENABLE, false);
	SetRenderStateTemp rs5(D3DRS_FILLMODE, D3DFILL_SOLID);
	SetRenderStateTemp rs7(D3DRS_ZWRITEENABLE, FALSE);

	//SetRenderStateTemp rs8(D3DRS_STENCILENABLE, FALSE);
	//is in undefined state, write directly
	_engine->D3DSetRenderState(-1, D3DRS_STENCILENABLE, FALSE);

	SetSamplerStateTemp ss01(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss02(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss03(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss04(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss05(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	SetSamplerStateTemp ss11(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss12(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss13(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	SetSamplerStateTemp ss14(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	SetSamplerStateTemp ss15(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	SetRenderStateTemp rs9(D3DRS_ZENABLE, true);
	SetRenderStateTemp rs10(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

	PushVertexShaderConstant vs0(VSC_GodRaysCenterSize);
	PushVertexShaderConstant vs1(VSC_GodRaysTexOffset);

	PushPixelShaderConstant ps0(0);
	PushPixelShaderConstant ps1(1);
	PushPixelShaderConstant ps2(2);
	PushPixelShaderConstant ps3(3);
	PushPixelShaderConstant ps4(5);
	PushPixelShaderConstant ps5(6);

	PushTextureState ts1(0);
	PushTextureState ts2(1);

	CreateDepthMask(projScreenNorm, scHeight*invSCWidth, pars._normSize);

	device->SetDepthStencilSurface(NULL);

	SetRenderStateTemp rs11(D3DRS_ZENABLE, false);
	device->SetVertexShaderConstantF(VSC_GodRaysCenterSize, D3DXVECTOR4(0.0f, 0.0f, 1.0f, -1.0f), 1);

	int smallRTWidth  = _engine->Width() >> _smallMaskLOD;
	int smallRTHeight = _engine->Height() >> _smallMaskLOD;

	D3DVIEWPORT9 viewData;
	viewData.X = 0;
	viewData.Y = 0;
	viewData.MinZ = 0.0f;
	viewData.MaxZ = 1.0f;

	viewData.Width  = smallRTWidth;
	viewData.Height = smallRTHeight;
	device->SetViewport(&viewData);

	ShrinkDepthMask(invSCWidth, invSCHeight);
	DiletateDepthMask(1.0f/smallRTWidth, 1.0f/smallRTHeight);
	BlurDepthMask(projScreen, smallRTWidth, smallRTHeight, mult, pars);

	viewData.Width  = scWidth;
	viewData.Height = scHeight;
	device->SetViewport(&viewData);

	device->SetRenderTarget(0, origRT);
	device->SetDepthStencilSurface(origDepth);

	rtIndex.Advance();
	device->SetTexture(0, res->_rtSmallMask[rtIndex.FullRT()]);
	device->SetTexture(1, screenRT);

	device->SetVertexShaderConstantF(VSC_GodRaysTexOffset, D3DXVECTOR4(
		0.5f*invSCWidth, 
		0.5f*invSCHeight, 
		0.5f/smallRTWidth, 
		0.5f/smallRTHeight), 1);

	float K, Q;
	enf::GetLineEq(pars._overBurnStart, pars._overBurnEnd, K, Q);

	//sun position, parameters, color...
	device->SetPixelShaderConstantF(0, D3DXVECTOR4(projScreen[0], projScreen[1], invSCHeight, 0), 1);
	device->SetPixelShaderConstantF(1, D3DXVECTOR4(pars._intensity, pars._overBurnIntensity,	K, Q), 1);

	//some dependency on sky configuration
	float visible = 1;//GScene->GetLandscape()->SkyThrough();
	Color lightColor = GScene->MainLight()->GetDiffuse()*(_engine->GetLastKnownAvgIntensity()*pars._intensity*visible);
	device->SetPixelShaderConstantF(2, D3DXVECTOR4(lightColor.R(), lightColor.G(), lightColor.B(), 0.0f), 1);

	device->SetVertexShader(res->_vs);
	device->SetPixelShader(res->_psFinal);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "	Combine: ");
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

	//restore
	_engine->D3DSetRenderState(-1, D3DRS_STENCILENABLE, FULL_STENCIL);

	if (!inScene) 
		_engine->D3DEndScene();
	
	GPU_END_PROFILE

	Finish();
}


//-----------------------------------------------------------------------------
/*!
drops simulator
*/
//-----------------------------------------------------------------------------
class RainDropsSimulator
{
private:
	// grid of rain drops
	static const int RainLayerGrid		= 32;														///< grid size
	static const int RainLayers				= 4;														///< layers of drops -> texture collected together to final one
	static const int MaxDropsPerLayer	= RainLayerGrid*RainLayerGrid;	///< max drops pear layer depends on grid size
	static const float AverageDropsLifeTime;													///< average value of drop life time
	static const float DropsLifTimeBias;															///< bias of each drop +- to AverageDropsLifeTime
	static const float MaxDropsPerSecond;															///< maximum number of drops generated per second

	//-----------------------------------------------------------------------------
	/*!
	grid position
	*/
	//-----------------------------------------------------------------------------
	struct GridVal
	{
		static const enf::uchar InvalidValue = 255;
		enf::uchar X, Y;	

		GridVal()
		{}

		GridVal(enf::uchar x, enf::uchar y): X(x), Y(y)
		{}

		bool IsInvalid() const
		{
			return X == InvalidValue;
		}
	};


	//-----------------------------------------------------------------------------
	/*!
	unique random grid value for simulator
	*/
	//-----------------------------------------------------------------------------
	template <int MaxGridVal> class UniqueRandomGridValue
	{
	private:
		static const int StackSize = MaxGridVal*MaxGridVal;
		enf::SArray<GridVal, StackSize> m_Stack;
	public:
		/*!
		initialize
		*/
		void Init()
		{
			m_Stack.Clear();

			//generate all possible grid values
			for (int x = 0; x < MaxGridVal; x++)
				for (int y = 0; y < MaxGridVal; y++)
					m_Stack.Insert(GridVal(x, y));

			//randomize them
			int maxGrid = MaxGridVal*MaxGridVal;
			for (int i = 0; i < maxGrid; i++)
			{
				int i1 = GRandGen.RandomInt(0, maxGrid);
				int i2 = GRandGen.RandomInt(0, maxGrid);

				enf::enf_swap(m_Stack[i1], m_Stack[i2]);
			}
		}

		/*!
		GridVal generator
		*/
		UniqueRandomGridValue()
		{
			Init();
		}

		/*!
		pop value from stack
		*/
		GridVal Pop()
		{
			if (m_Stack.IsEmpty())
				return GridVal(GridVal::InvalidValue, GridVal::InvalidValue);

			return m_Stack.Pop();
		}

		/*!
		push value back to stack but to random position
		*/
		void Push(const GridVal& val)
		{
			if (m_Stack.IsFull())
				return;

			if (m_Stack.IsEmpty())
			{
				m_Stack.Insert(val);
			}
			else
			{
				int i = intMin(m_Stack.GetCardinality() - 1, GRandGen.RandomInt(0, m_Stack.GetCardinality()));
				m_Stack.Insert(m_Stack[i]);
				m_Stack[i] = val;
			}
		}
	};

	//-----------------------------------------------------------------------------
	/*!
	one layer of drops
	*/
	//-----------------------------------------------------------------------------
	class DropLayer
	{
	private:
		UniqueRandomGridValue<RainLayerGrid> m_RandGridPos;

	public:
		struct RainDrop
		{
			float		Time;
			GridVal	GridPos;

			RainDrop() 
			{}
		};

		enf::Array<RainDrop> m_Drops;
		enf::Vector2	m_vOffset;

		/*!
		add new drop
		*/
		void Add(float time)
		{
			GridVal grid = m_RandGridPos.Pop();
			if (grid.IsInvalid())
				return;

			RainDrop& drop = m_Drops.Append();
			drop.Time = time;
			drop.GridPos = grid;
		}

		/*!
		remove drop
		*/
		void Remove(int i)
		{
			//remove from array of drops
			GridVal oldVal = m_Drops[i].GridPos;
			m_Drops[i] = m_Drops.GetLast();
			m_Drops.Pop();
			
			//add to free grid pos 
			m_RandGridPos.Push(oldVal);
		}
	};

	enf::SArray<DropLayer, RainLayers> m_Layers;
	float	m_fNewDrops;

	/*!
	generate new drops count for each layer
	*/
	void GenerateNewDropsLayerCount(int* newLayerDrops, float deltaT, float density);

public:
	/*!
	init again
	*/
	void Init();

	/*!
	simulate drops in time
	*/
	void Simulate(float deltaT, float density);

	/*!
	random offset of texture of each layer
	*/
	enf::Vector2 GetOffset(int layer) const
	{
		return m_Layers[layer].m_vOffset;
	}

	/*!
	return array of drops per each layer
	*/
	const enf::Array<DropLayer::RainDrop>& GetDrops(int layer) const
	{
		return m_Layers[layer].m_Drops;
	}

	void Test()
	{
		enf::SString<256> str = "layers: ";
		for (int i = 0; i < m_Layers.GetCardinality(); i++)
		{
			enf::SString<256> tmp;
			str += tmp.format("%d, ", GetDrops(i).GetCardinality());
		}
		LogF(str.cstr());
	}
};


const float RainDropsSimulator::AverageDropsLifeTime	= 0.35f;
const float RainDropsSimulator::DropsLifTimeBias			= 0.1f;
const float RainDropsSimulator::MaxDropsPerSecond			= 50;


//-----------------------------------------------------------------------------
void RainDropsSimulator::Init()
{
	m_fNewDrops = 0;

	m_Layers.Clear();
	for (int i = 0; i < RainLayers; i++)
	{
		DropLayer &layer = m_Layers.Append();

		layer.m_Drops.Reserve(64);
		layer.m_vOffset.x = GRandGen.RandomValue();
		layer.m_vOffset.y = GRandGen.RandomValue();
	}
}


//-----------------------------------------------------------------------------
void RainDropsSimulator::GenerateNewDropsLayerCount(int* newLayerDrops, float deltaT, float density)
{
	//calculate new drops count
	memset(newLayerDrops, 0, sizeof(newLayerDrops));

	if (density <= 0.0f)
		return;

	m_fNewDrops += deltaT*density*MaxDropsPerSecond;

	int newDrops = toIntFloor(m_fNewDrops);
	m_fNewDrops = m_fNewDrops - newDrops;

	//generate new drops into each layer almost equally
	if (newDrops)
	{
		int lastLayer = RainLayers > 1 ? GRandGen.RandomInt(0, RainLayers): 0;
		int layer			= lastLayer;

		int restDrops = newDrops;
		do 
		{
			layer = (layer < RainLayers - 1) ? layer + 1 : 0;

			int fromLayers = RainLayers;
			if (layer != lastLayer)
			{
				int drops = restDrops/fromLayers;

				restDrops -= drops;
				newLayerDrops[layer] = drops;
			}
		}	while (layer != lastLayer);

		newLayerDrops[lastLayer] = restDrops;

		//debug test
		int count = 0;
		for (int i = 0; i < RainLayers; i++)
		{
			count += newLayerDrops[i];
		}
		DoAssert(count == newDrops);
	}
}


//-----------------------------------------------------------------------------
void RainDropsSimulator::Simulate(float deltaT, float density)
{
	int newLayerDrops[RainLayers];
	GenerateNewDropsLayerCount(newLayerDrops, deltaT, density);

	for (int layerIndex = 0; layerIndex < m_Layers.GetCardinality(); layerIndex++)
	{
		DropLayer& layer = m_Layers[layerIndex];
		enf::Array<DropLayer::RainDrop>& drops	= layer.m_Drops;

		//clear what's done
		for (int i = 0; i < drops.GetCardinality();)
		{
			drops[i].Time -= deltaT;

			if (drops[i].Time < 0)
				layer.Remove(i);
			else
				i++;
		}

		//add what's new
		for (int i = 0; i < newLayerDrops[layerIndex]; i++)
		{
			float randTime = AverageDropsLifeTime + GRandGen.RandomValue11()*DropsLifTimeBias;
			layer.Add(randTime);
		}
	}
}


//-----------------------------------------------------------------------------
/*!
 Rain postprocess effect
*/
//-----------------------------------------------------------------------------
bool EngineDD9::PPRain::InitRain(const LazyLoadFile &hlsl, const LazyLoadFile &vs, Ref<PPRainResources> &resrc)
{
	// Create the VB
	ComRef<IDirect3DVertexBuffer9> vb;
	if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 4 * MaxNumOfDrops, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
	{
		LogF("Error: Vertex buffer creation failed.");
		return false;
	}

	Vertex *vStart;
	vb->Lock(0, 0, (void**)&vStart, 0);

	Vertex* v = vStart;
	for (int i = 0; i < MaxNumOfDrops; i++)
	{
		short rn1 = toInt(32767.0f*GRandGen.RandomValue11());
		short rn2 = toInt(32767.0f*GRandGen.RandomValue11());
		short rn3 = toInt(32767.0f*GRandGen.RandomValue11());

		short rNum = toInt(256*GRandGen.RandomValue());
		short tex0 = -16384;
		short tex1 =  16384;

		v->p[0] = rn1;	v->p[1] = rn2; v->p[2] = rn3; v->rand = rNum; v->t[0] = tex0; v->t[1] = tex0; v++;
		v->p[0] = rn1;	v->p[1] = rn2; v->p[2] = rn3; v->rand = rNum; v->t[0] = tex1; v->t[1] = tex0; v++;
		v->p[0] = rn1;	v->p[1] = rn2; v->p[2] = rn3; v->rand = rNum; v->t[0] = tex1; v->t[1] = tex1; v++;
		v->p[0] = rn1;	v->p[1] = rn2; v->p[2] = rn3; v->rand = rNum; v->t[0] = tex0; v->t[1] = tex1; v++;
	}
	vb->Unlock();
	Assert(v - vStart == 4*MaxNumOfDrops);

	// Create the IB
	ComRef<IDirect3DIndexBuffer9> ib;
	if (FAILED(_engine->GetDirect3DDevice()->CreateIndexBuffer(sizeof(short) * 6 * MaxNumOfDrops, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, ib.Init(), NULL)))
	{
		LogF("Error: Index buffer creation failed.");
		return false;
	}

	unsigned short *iStart;
	ib->Lock(0, 0, (void**)&iStart, 0);

	unsigned short* ind = iStart;
	for (int i = 0; i < MaxNumOfDrops; i++, ind += 6)
	{
		int si = i*4;

		ind[0] = si;
		ind[1] = si + 1;
		ind[2] = si + 3;

		ind[3] = si + 1;
		ind[4] = si + 3;
		ind[5] = si + 2;
	}
	ib->Unlock();
	Assert(ind - iStart == 6*MaxNumOfDrops);

	// Create the VD
	const D3DVERTEXELEMENT9 vertexDecl[] =
	{
		{0,  0, D3DDECLTYPE_SHORT4N,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0,  8, D3DDECLTYPE_SHORT2N,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},

		D3DDECL_END()
	};

	ComRef<IDirect3DVertexDeclaration9> vd;
	if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
	{
		LogF("Error: Vertex declaration creation failed.");
		return false;
	}

	//shaders
	if (!CompileVertexShader(vs, "VSPostProcessRain", resrc->_vs))
		return false;

	if (!CompilePixelShader(hlsl,"PSPostProcessRain", resrc->_ps, false))
		return false;

	// assign to shared resources
	resrc->_vd = vd;
	resrc->_vb = VertexStaticData9::BufferFuture(vb);
	resrc->_ib = IndexStaticData9::BufferFuture(ib);

	return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPRain::InitLayer(const LazyLoadFile &hlsl, const LazyLoadFile &vs, Ref<PPRainResources> &resrc)
{
	// Create the VB
	ComRef<IDirect3DVertexBuffer9> vb;
	if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexLayer) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
	{
		LogF("Error: Vertex buffer creation failed.");
		return false;
	}

	VertexLayer *v;

	vb->Lock(0, 0, (void**)&v, 0);
		v[0].pos = Vector3P(-1.0f, -1.0f, 2.0f); v[0].u = 0; v[0].v = 1;
		v[1].pos = Vector3P(-1.0f,  1.0f, 0.0f); v[1].u = 0; v[1].v = 0;
		v[2].pos = Vector3P( 1.0f, -1.0f, 3.0f); v[2].u = 1; v[2].v = 1;
		v[3].pos = Vector3P( 1.0f,  1.0f, 1.0f); v[3].u = 1; v[3].v = 0;
	vb->Unlock();

	// Create the VD
	const D3DVERTEXELEMENT9 vertexDecl[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};

	ComRef<IDirect3DVertexDeclaration9> vd;
	if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
	{
		LogF("Error: Vertex declaration creation failed.");
		return false;
	}

	// note: vertex shader is stored in PS.hlsl for simple effects
	if (!CompileVertexShader(vs, "VSPostProcessRainLayer", resrc->_vsLayer))
	{
		return false;
	}

	if (!CompilePixelShader(hlsl,"PSPostProcessRainLayer", resrc->_psLayer, false))
		return false;

	// assign to shared resources
	resrc->_vdLayer = vd;
	resrc->_vbLayer = VertexStaticData9::BufferFuture(vb);

	return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPRain::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit)
{
	_collisionValue.Init(1.0f);
	_resNotReady  = true;

	_actualCamCollision = 0.0f;
	_globPower = 0.0f;
	_globIntensity = 1.0f;

	if (!base::Init(hlsl,vs)) return false;

	Ref<PPRainResources> resrc = new PPRainResources;
	if (!InitRain(hlsl, vs, resrc))
		return false;

	if (!InitLayer(hlsl, vs, resrc))
		return false;

	_resrc = resrc;

	GNoiseRotationTexture.Create(_engine->GetDirect3DDevice().GetRef());

	if (firstInit)
	{
		SetDefaultValues();
		_enabled = false;
		_pInterp.Initialize();
	}

	return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRain::Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
	UnInit();
	Init(hlsl, vs, false);
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRain::SetTexture(TextureD3D9 *tex)
{
	const PPRainResources* res = static_cast<const PPRainResources*>(_resrc->GetType());
	if (res)
	{
		const_cast<PPRainResources*>(res)->_rainTexture = tex;
		_engine->TextBank()->UseMipmap(tex, 0, 0);
	}
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRain::Cleanup()
{
	const PPRainResources* res = static_cast<const PPRainResources*>(_resrc->GetType());
	if (res)
	{
		const_cast<PPRainResources*>(res)->_rainTexture.Free();
	}
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRain::SetDefaultValues()
{
	_pInterp._pars.SetDefault();
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPRain::Init(PPResources *resources, bool firstInit)
{
	_resNotReady = true;

	if (resources)
	{
		_resrc = resources;

		if (firstInit)
		{
			SetDefaultValues();
			_enabled = false;
			_pInterp.Initialize();
		}

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPRain::UnInit()
{
	if (_resrc)
	{
		GNoiseRotationTexture.Free();
		_resrc->UnInit();

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPRain::SetParams(const float *pars, int nPars, bool imm)
{
	SET_SC(Intensity, 0);
	SET_SC(Luminance, 1);
	SET_SC(Power, 2);
	SET_SC(Radius, 3);
	SET_SC(Speed, 4);
	SET_SC(Width, 5);
	SET_SC(Height, 6);
	SET_SC(RandMove, 7);
	SET_SC(RandSpeed, 8);
	SET_SC(RandGrid, 9);
	SET_SC(DistWidthMul, 10);
	SET_SC(SunLumIntensity, 11);

	SET_VEC(Dir, 12);
	SET_SC(TexDrops, 15);
	

	return 0;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPRain::SetParams(const AutoArray<float> &pars)
{
	return SetParams(pars.Data(), pars.Size());
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPRain::Serialize( ParamArchive &ar )
{
	CHECK(ar.Serialize("enabled",_enabled,0,false));
	CHECK(ar.Serialize("interp", _pInterp, 0));
	CHECK(ar.Serialize("prior", _priority, 0, -1));

	return LSOK;
}


//-----------------------------------------------------------------------------
float EngineDD9::PPRain::ComputePosIntensity(Vector3Par pos) 
{
	const Camera* camera = GScene->GetCamera();

	//trace above us, and find the "rain occluder"
	Object *camOn = GWorld->GetCameraEffect() ? NULL : GWorld->CameraOn();
	Object* obj = GWorld->GetCamInsideVehicle();
	VisualStateAge cameraAge = camera->GetAge();

	CollisionBuffer col;
	GScene->GetLandscape()->ObjectCollisionLine(cameraAge, col, Landscape::FilterIgnoreTwo(camOn,obj), pos, pos + Vector3(0,10,0),0/*0.15f*/, ObjIntersectView);
	float occlusionRadius = 0;

	if(col.Size() != 0)
	{
		for(int n = 0; n < col.Size(); n++)
		{
			const Object* cObject = col[n].object;
			// check the size of the component above us
			const ConvexComponents* comps = cObject->GetShape()->GetConvexComponents(col[n].geomLevel);
			if(comps)
			{
				const ConvexComponent* comp = (*comps)[col[n].component];
				if(comp)
				{
					// we are interested in horizontal radius only, the distinction is important for tall objects like trees
					float xzRad = comp->Min().DistanceXZ(comp->Max())*0.5f;
					occlusionRadius = floatMax(occlusionRadius, floatMin(comp->GetRadius(),xzRad));

					return 1.0f;
				}
			}
		}
	}

	return 0.0f;
}


//-----------------------------------------------------------------------------
float EngineDD9::PPRain::ComputeOcclusionIntensity() 
{
	const Camera* camera = GScene->GetCamera();
	float occl = 1.0f - _collisionValue.UpdateAbsTime(ComputePosIntensity(camera->Position()), Glob.GetTime().toFloat());
	return occl;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPRain::Prepare(float deltaT)
{
	_resNotReady = true;
	_actualCamCollision = 1.0f;

	const PPRainResources *res = Resources();
	if (!res) 
		return;

	//interpolated parameters
	_currentPars = _pInterp.Simulate();

	_currNumOfDrops = _globPower*_currentPars._power*MaxNumOfDrops;
	if (_currNumOfDrops < 1)
		return;

	//don't need to compute now, rendering rain map in shadow pass
	//_actualCamCollision = ComputeOcclusionIntensity();

	MipInfo info = _engine->TextBank()->UseMipmap(res->_rainTexture, 0, 0);
	if (info._texture != res->_rainTexture || info._level != 0)
		return;

	_resNotReady = false;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRain::RenderRain(const RainPar& pars, float camOcclusionIntensity)
{
	PROFILE_SCOPE_GRF_EX(ppRenderRain,pp,SCOPE_COLOR_YELLOW);

	const PPRainResources *res = Resources();
	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

	const Camera& camera = *GScene->GetCamera();
	const LightSun& sun  = *GScene->MainLight();

	//matrices
	Matrix4 viewZero;
	viewZero.SetOrientation(camera.InverseScaled().Orientation());
	viewZero.SetPosition(VZero);

	MatrixFloat34 viewWorldMatrix;
	ConvertMatrixTransposed3(viewWorldMatrix, viewZero);
	device->SetVertexShaderConstantF(0, (float*) &viewWorldMatrix, 3);

	//is transposed to enforce projection matrix !
	D3DXMATRIX projMatrix;
	ConvertProjectionMatrix(projMatrix, camera.ProjectionNormal(), 0.0f);

	device->SetVertexShaderConstantF(3, (float*)&projMatrix, 4);
	//device->SetVertexShaderConstantF(VSC_ShadowmapMatrix, (float*)&GEngineDD->_rainMapMatrixDX, 3);
	
	//recompute to have everything in mad instructions
	const Vector3& rainMapPos = GEngineDD->_rainMapParams._shadowPos;
	float invDiameter = GEngineDD->_rainMapParams._projTrans.x;
	float maxDepth		= GEngineDD->_rainMapParams._projTrans.y;

	float invSize = 0.5f/RAIN_MAP_SB_SIZE;

	//scale of rain map
	device->SetVertexShaderConstantF(CV_RAIN_RAINMAPSCALE, D3DXVECTOR4(invDiameter, -invDiameter, maxDepth, 1.0f), 1);

	//bias of rain map
	device->SetVertexShaderConstantF(CV_RAIN_RAINMAPPOS, D3DXVECTOR4(
		0.5f - invDiameter*rainMapPos.x + invSize, 
		0.5f + invDiameter*rainMapPos.z + invSize, 
		rainMapPos.y, 
		0.0f), 1);

	//vertex shader params
	float invradius = 1.0/pars._radius;
	float time = Glob.GetTime().toFloat();

	Vector3 relEye  = camera.Position()*invradius;
	device->SetVertexShaderConstantF(CV_RAIN_REYE, D3DXVECTOR4(
		relEye.X(),
		relEye.Y(),
		relEye.Z(),
		invradius), 1);

	const Vector3& camDir = camera.Direction();
	device->SetVertexShaderConstantF(CV_RAIN_EYEDIR, D3DXVECTOR4(
		camDir.X(),
		camDir.Y(),
		camDir.Z(),
		0.0f), 1);

	device->SetVertexShaderConstantF(CV_RAIN_VERT, D3DXVECTOR4(
		pars._width,
		pars._height,
		time,
		pars._radius), 1);

	float sign = pars._sunLumIntensity > 0.0f ? -1.0f : 1.0f; 
	Vector3 transLightDir = viewZero.Orientation().TransformVector(sun.Direction())*sign;
	device->SetVertexShaderConstantF(CV_RAIN_LIGHTDIR, D3DXVECTOR4(
		transLightDir.X(),
		transLightDir.Y(),
		transLightDir.Z(),
		0.0f), 1);

	float reltime = time*(pars._speed/pars._radius);
	device->SetVertexShaderConstantF(CV_RAIN_LUM_TEX, D3DXVECTOR4(
		abs(pars._sunLumIntensity),
		pars._luminance,//mat->GetScalar(MatRain::MV_LUM_SUN) + delta2,
		reltime, 
		0.0f), 1);

	//wind and direction into account
	Vector3 dir2d = GLandscape->GetWind();//*/GetWindSlow();
	//dir2d.y = 0.0f;
	static const float WindInfluence = 0.1f;
	dir2d = Vector3(dir2d*WindInfluence + pars._dir);//.Normalized();

	device->SetVertexShaderConstantF(CV_RAIN_DIR, D3DXVECTOR4(
		dir2d.X(),
		dir2d.Y(),
		dir2d.Z(),
		0.0f), 1); //mat->GetScalar(MatRain::MV_DISSOLVE_OFFS)));

	Vector3 transDir = viewZero.Orientation().TransformVector(dir2d); //don't need anymore, computing in vs
	device->SetVertexShaderConstantF(CV_RAIN_RDIR, D3DXVECTOR4(
		transDir.X(),
		transDir.Y(),
		transDir.Z(),
		pars._distWidthMul), 1);

	device->SetVertexShaderConstantF(CV_RAIN_PARAMS, D3DXVECTOR4(
		pars._randMove,
		pars._randSpeed,
		pars._randGrid,
		0), 1);

	device->SetVertexShaderConstantF(CV_RAIN_PARAMS2, D3DXVECTOR4(
	0.5/res->_rainTexture->Mipmap(res->_rainTexture->BestMipmap())->_w,
	1.0/pars._texDrops,
	pars._texDrops,
	0), 1);

	device->SetTexture (0, res->_rainTexture->GetHandle());

	device->SetTexture(D3DVERTEXTEXTURESAMPLER0, GNoiseRotationTexture.Get());
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	device->SetTexture(D3DVERTEXTEXTURESAMPLER1, GEngineDD->_rtRainMap);

	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	device->SetSamplerState(D3DVERTEXTEXTURESAMPLER1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);


	//not used now
	ColorVal lightColor = sun.GetAmbient() + sun.GetDiffuse();

	float intensity = pars._luminance*GWorld->GetEyeAccom()*0.5f;
	device->SetPixelShaderConstantF(0, D3DXVECTOR4(intensity*lightColor.R(), intensity*lightColor.G(), intensity*lightColor.B(), _globIntensity*camOcclusionIntensity*pars._intensity), 1);


	float invWidth  = 1.0/_engine->Width();
	float invHeight = 1.0/_engine->Height();
	device->SetPixelShaderConstantF(1, D3DXVECTOR4(invWidth, invHeight, 0.5f*invWidth, 0.5f*invHeight), 1);

	float refrIntensity = _globIntensity*camOcclusionIntensity*pars._intensity;
	device->SetPixelShaderConstantF(2, D3DXVECTOR4(invWidth*refrIntensity, invHeight*refrIntensity, 0.0f, 0.0f), 1);


	device->SetVertexShader(res->_vs);
	device->SetPixelShader(res->_ps);

	int primitives = _currNumOfDrops*2;

	GPU_START_PROFILE(device, "  Geometry drops:");
		device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, primitives*2, 0, primitives);
	GPU_END_PROFILE

	device->SetTexture(D3DVERTEXTEXTURESAMPLER0, NULL);
	device->SetTexture(D3DVERTEXTEXTURESAMPLER1, NULL);
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRain::RenderRainDrops(const RainPar& pars)
{
	PROFILE_SCOPE_GRF_EX(ppRenderRainDrops,pp,SCOPE_COLOR_YELLOW);
/*
	const PPRainResources *res = Resources();
	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();
*/
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRain::RenderRainLayer(const RainPar& pars)
{
	PROFILE_SCOPE_GRF_EX(ppRenderRainLayer,pp,SCOPE_COLOR_YELLOW);

	const PPRainResources *res = Resources();
	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

	SetRenderStateTemp rs2(D3DRS_ZENABLE, false);

	_engine->SetStreamSource(-1, res->_vbLayer, sizeof(VertexLayer), 4);
	_engine->SetVertexDeclaration(-1, res->_vdLayer);

	device->SetVertexShader(res->_vsLayer);
	device->SetPixelShader(res->_psLayer);

	float invWidth  = 1.0/_engine->Width();
	float invHeight = 1.0/_engine->Height();

	device->SetVertexShaderConstantF(VSC_RainLayer_Offset ,D3DXVECTOR4(0.5f*invWidth, 0.5f*invHeight, 0.0f, 0.0f), 1);

	const Camera& camera = *GScene->GetCamera();
	D3DScreenVecArray screenVecs;

	float farPlane = GScene->GetCamera()->Far();
	float linearFarPlane = Glob.config.GetMaxLinearDepth();

	ComputeScreenRayConsts(camera, screenVecs, linearFarPlane/farPlane);

	device->SetPixelShaderConstantF(3, D3DXVECTOR4(camera.Position().x, camera.Position().y, camera.Position().z,  Glob.config.GetMaxLinearDepth()/camera.Far()), 1);

	//the world pos coordinates computing
	device->SetVertexShaderConstantF(VSC_RainLayer_Corners,	screenVecs[0], 4);
	device->SetPixelShaderConstantF(4, screenVecs[0], 4);

	SetTextureTemp tTex2(0, _engine->GetDepthAsTexture());
	SetSamplerStateTemp ss2MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss2MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss2ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss2ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Rain layer: ");
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE
}


//-----------------------------------------------------------------------------
void EngineDD9::PPRain::Do(bool isLast)
{
	if (!_enabled || _engine->_d3DDevice.IsNull()) 
		return;

	PROFILE_SCOPE_GRF_EX(ppRain,pp,SCOPE_COLOR_YELLOW);

	// If scene is not being rendered initialize scene
	bool inScene = _engine->_d3dFrameOpen;
	if (!inScene) 
		_engine->D3DBeginScene();

	if (_resNotReady || _currNumOfDrops < 1)
	{
		if (!inScene) 
			_engine->D3DEndScene();

		return;
	}

	base::Prepare();
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Rain: ");

	const RainPar& pars = _currentPars;

	ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();
	const PPRainResources *res = Resources();

	_engine->SetStreamSource(-1, res->_vb, sizeof(Vertex), _currNumOfDrops*4);
	_engine->SetVertexDeclaration(-1, res->_vd);

	if (isLast) 
		_engine->SwitchToBackBufferRenderTarget();

	ComRefFake<IDirect3DSurface9> origDepth = _engine->_rtMultiSampleType != D3DMULTISAMPLE_NONE ? _engine->_depthBufferRT : _engine->_depthBuffer;	
	device->SetDepthStencilSurface(origDepth);

	//common RS
	SetRenderStateTemp rs1(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
	SetRenderStateTemp rs2(D3DRS_ZENABLE, true);
	SetRenderStateTemp rs22(D3DRS_ZWRITEENABLE, false);
	SetRenderStateTemp rs3(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	SetRenderStateTemp rs4(D3DRS_FILLMODE, D3DFILL_SOLID);
	SetRenderStateTemp rs5(D3DRS_CULLMODE, D3DCULL_NONE);
	SetRenderStateTemp rs6(D3DRS_ALPHABLENDENABLE, true);
	SetRenderStateTemp rs7(D3DRS_ALPHATESTENABLE, true);
	SetRenderStateTemp rs8(D3DRS_ALPHAREF, 1);
	
	_engine->D3DSetRenderState(-1, D3DRS_STENCILENABLE, FALSE);
	_engine->D3DSetRenderState(-1, D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	_engine->D3DSetRenderState(-1, D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	SetSamplerStateTemp ss01(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	SetSamplerStateTemp ss02(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	SetSamplerStateTemp ss03(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss04(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss05(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	SetSamplerStateTemp ss11(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss12(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss13(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	SetSamplerStateTemp ss14(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	SetSamplerStateTemp ss15(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
	SetTextureTemp tTex0(2, rt);	

	SetSamplerStateTemp ss21(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss22(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	SetSamplerStateTemp ss23(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss24(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	SetSamplerStateTemp ss25(2, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	PushVertexShaderConstantAll<7> vs1(0);
	PushPixelShaderConstantAll<10> ps0(0);

	PushTextureState ts1(0);
	PushTextureState ts2(1);

	device->SetIndices(res->_ib.GetResult());

	RenderRain(pars, _actualCamCollision);
	if (0)
		RenderRainDrops(pars);

	device->SetIndices(NULL);

	static bool dooo = false;
	if (dooo)
		RenderRainLayer(pars);

	GPU_END_PROFILE

	//restore
	_engine->D3DSetRenderState(-1, D3DRS_STENCILENABLE, 	FULL_STENCIL);

	if (!inScene) 
		_engine->D3DEndScene();

	Finish();
}


//-----------------------------------------------------------------------------
/*!
	DOF PP
*/
//-----------------------------------------------------------------------------
bool EngineDD9::PPDOF::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const Ref<PPGaussianBlur> &ppGaussianBlur)
{
  if (!base::Init(hlsl,vs,"VSPostProcess")) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessDOF", _ps, true)) return false;
  _ppGaussianBlur = ppGaussianBlur;
  _useFarDistance = false;
  return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPDistanceDOF::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, const Ref<PPGaussianBlur> &ppGaussianBlur)
{
	if (!grandBase::Init(hlsl,vs,"VSPostProcess")) return false;
	if (!CompilePixelShader(hlsl,"PSPostProcessDistanceDOF", _ps, true)) return false;
	_ppGaussianBlur = ppGaussianBlur;
	_useFarDistance = true;
	return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPDOF::Reload(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
	UnInit();
	Init(hlsl, vs, _ppGaussianBlur);
}

//-----------------------------------------------------------------------------
void EngineDD9::PPDOF::Do(bool isLast)
{
  PROFILE_SCOPE_GRF_EX(ppDOF,pp,SCOPE_COLOR_YELLOW);

  const int cb = -1;
  
  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Stage 0 - base texture - do the copy (to make sure same data is both in source texture and destination render target)
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
  SetTextureTemp tTex0(0, rt);

  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // Stage 1 - blurred texture
  SetTextureTemp tTex1(1, _ppGaussianBlur->GetBlurredTexture());
  SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // Stage 2 - depth map
  SetTextureTemp tTex2(2, _engine->GetDepthAsTexture());
  SetSamplerStateTemp ss2SRGBTEXTURE(2, D3DSAMP_SRGBTEXTURE, FALSE);
  SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss2ADDRESSU(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss2ADDRESSV(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // Prepare PS constants
  // with zoomed camera / binoculars there is more blur
  Matrix4Val proj = _engine->CBState(cb)._proj;
  
  AspectSettings as;
  ::GEngine->GetAspectSettings(as);

  static float adjustZoom = 0.5f;
  float zoom = proj(0,0)/as.leftFOV*adjustZoom;

  //1.3 TOH - frenkee
  float blurFactor = _useFarDistance ? 1.3f : zoom;

  static float minBlurFactor = 1;
  static float maxBlurFactor = 10;
  saturate(blurFactor,minBlurFactor,maxBlurFactor);

  static float minZoom = 1;
  static float maxZoom = 2.5; // we want to avoid too large offsets
  saturate(zoom,minZoom,maxZoom);

  const float blurRadiusInPixels = 3.0f*zoom*_engine->_blurCoefThis;
  
  float invW = 1.0f / _engine->Width();
  float invH = 1.0f / _engine->Height();

	static float NormalDepthDiv = 50.0f;
	blurFactor /= NormalDepthDiv*Glob.config.GetMaxInvLinearDepth();

	float maxFocalPlane = Glob.header.MaxDOFFocusDistance*Glob.config.GetMaxInvLinearDepth();
	float invFocalPlane = _engine->_focusDistThis*Glob.config.GetMaxInvLinearDepth();

	static float blurmulfar	= 1.25f;
	static float blurmulzoom	= 2.0f;
	blurFactor = blurFactor*(_useFarDistance ? blurmulfar : blurmulzoom);
	
// SetPixelShaderConstantTemp spc1(1, D3DXVECTOR4(invFocalPlane, invFocalPlane, invFocalPlane, invFocalPlane));
  static float xblurfactor = 1.0f;
  static float xblurpower  = 2.0f;

  SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invW * blurRadiusInPixels, invH * blurRadiusInPixels, blurFactor, maxFocalPlane));
  SetPixelShaderConstantTemp spc1(1, D3DXVECTOR4(invFocalPlane, xblurfactor, xblurpower, 0.0f));

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "DOF: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}


//-----------------------------------------------------------------------------
/*!

*/
//-----------------------------------------------------------------------------
struct EdgeParams
{
  float u[4][4];
  float v[4][4];
  float posOffset[4];

	//-----------------------------------------------------------------------------
  void SetupBilinear(int dstW, int dstH, int srcW, int srcH, float minSrcTexCoord = 0.0f, float maxSrcTexCoord = 1.0f)
  {
    // for pos. offset explanation cf. DX docs:
    // Directly Mapping Texels to Pixels 
    // Texture Coordinates
    // TODO: verify if this works well for 2x and 4x decimation case as well
    posOffset[0] = 0; //-0.5f/dstW;
    posOffset[1] = 0; //-0.5f/dstH;
    posOffset[2] = posOffset[3] = 0;

#if _ARMA3_HDR_IMPROVEMENTS
    float srcSizeCoef = maxSrcTexCoord - minSrcTexCoord;
    srcW *= srcSizeCoef;
    saturateMax(srcW, 1);
    srcH *= srcSizeCoef;
    saturateMax(srcH, 1);
#endif

    // we may perform any decimation, like 2x or 4x decimation
    int decimationW = srcW/dstW;
    int decimationH = srcH/dstH;

    const float sampOffsetU = 1.0/srcW;
    const float sampOffsetV = 1.0/srcH;
    static const float baseU[4] = {0, 0, 1, 1};
    static const float baseV[4] = {1, 0, 1, 0};
    static const float nU[4] = {0, 0,  1, 1};
    static const float nV[4] = {0, 1, 0,  1};
    for (int vertex = 0; vertex < 4; vertex++)
    {
      for (int t = 0; t < 4; t++)
      {
        u[vertex][t] = baseU[vertex] + sampOffsetU * nU[t]*decimationW;
        v[vertex][t] = baseV[vertex] + sampOffsetV * nV[t]*decimationH;
      }
    }
  }

	//-----------------------------------------------------------------------------
  void SetupPoint(int dstW, int dstH, int srcW, int srcH)
  {
    posOffset[0] = 0; //-0.5f/dstW;
    posOffset[1] = 0; //-0.5f/dstH;
    posOffset[2] = posOffset[3] = 0;
    // we may perform any decimation, like 2x or 4x decimation
    int decimationW = srcW/dstW;
    int decimationH = srcH/dstH;
    const float sampOffsetU = 1.0/srcW;
    const float sampOffsetV = 1.0/srcH;
    static const float baseU[4] = {0, 0, 1, 1};
    static const float baseV[4] = {1, 0, 1, 0};
    static const float nU[4] = {0, 0,  1, 1};
    static const float nV[4] = {0, 1, 0,  1};
    for (int vertex = 0; vertex < 4; vertex++)
    {
      for (int t = 0; t < 4; t++)
      {
        // Cf. DX docs Nearest-Point Sampling for UV offset explanation
        u[vertex][t] = baseU[vertex] + sampOffsetU * (nU[t]*decimationW + 0.5f);
        v[vertex][t] = baseV[vertex] + sampOffsetV * (nV[t]*decimationH + 0.5f);
      }
    }
  }
};


//-----------------------------------------------------------------------------
bool EngineDD9::PPDecimate::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs)) return false;

  // Create the low resolution textures cascade

  // we need textures for 4x average decimation
  // and then some for 2x avg/min/max decimation
  // we want to start calculating min/max once the picture is coarse enough
  const int startMinMaxAtSize = 80;

  int fullWidth = _engine->Width();
  int fullHeight = _engine->Height();

  // decimation 2x uses point sampling and should therefore be very accurate
  // we will round to nearest bigger pow2
  int w = roundTo2PowerNCeil(fullWidth);
  int h = roundTo2PowerNCeil(fullHeight);

  int maxSize = max(w,h);
  int minSize = min(w,h);

  D3DFORMAT rtFormat = _engine->_downsampleFormat;
  int level = 0;
  // we need at least one 4x decimation step
  do
  {
    DecimationPass &dec = _dec[level];
    w = w>>2;
    h = h>>2;
    maxSize = maxSize>>2;
    minSize = minSize>>2;
    // create the supplementary RT
    int effW = max(w,1);
    int effH = max(h,1);
    LogF("Decimation 4x: %d,%d",effW,effH);
    HRESULT err = _engine->CreateTextureRT(effW, effH, 1, rtFormat, D3DPOOL_DEFAULT, dec._rt, dec._rts, true);
    if (FAILED(err))
    {
      _engine->DDError9("Post process Low resolution texture creation failed",err);
      return false;
    }
    dec._w = effW;
    dec._h = effH;
    // proceed to next level
    level++;
  } while (minSize>startMinMaxAtSize);
  _decimations = level;

  // Create measurement textures
  const DecimationPass &smallestDec = _dec[_decimations - 1];
  HRESULT err = _engine->CreateTextureRT(smallestDec._w, smallestDec._h, 1, rtFormat, D3DPOOL_DEFAULT, _rtMeasure, _rtsMeasure, true);
  if (FAILED(err))
  {
    _engine->DDError9("Post process Low resolution texture creation failed",err);
    return false;
  }

  // Create depth buffer (as it must be set in order the query to work with some ATI graphics cards)
  err = _engine->CreateDepthStencilSurface(smallestDec._w, smallestDec._h, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, _rtsMeasureDepth, _rtsMeasure, NULL);
  if (FAILED(err))
  {
    _engine->DDError9("Measure depth buffer creation failed",err);
    return false;
  }

  //////////////////////////////////////////////////////////////////////////

#if _HELISIM_RENDER_TO_TEXTURE

  w = h = R2TMaxResolution >> 2;

  DecimationPass &dec0 = _decR2T[0];
  err = _engine->CreateTextureRT(w, h, 1, rtFormat, D3DPOOL_DEFAULT, dec0._rt, dec0._rts, false);
  if (FAILED(err))
  {
    _engine->DDError9("Post process Low resolution r2t texture creation failed", err);
    return false;
  }
  _decR2T[0]._w = w;
  _decR2T[0]._h = h;

  w = h = R2TMinResolution;
  DecimationPass &dec1 = _decR2T[1];
  err = _engine->CreateTextureRT(w, h, 1, rtFormat, D3DPOOL_DEFAULT, dec1._rt, dec1._rts, false);
  if (FAILED(err))
  {
    _engine->DDError9("Post process Low resolution r2t texture creation failed", err);
    return false;
  }
  _decR2T[1]._w = w;
  _decR2T[1]._h = h;

  err = _engine->CreateTextureRT(R2TMinResolution, R2TMinResolution, 1, rtFormat, D3DPOOL_DEFAULT, _rtMeasureR2T, _rtsMeasureR2T, false);
  if (FAILED(err))
  {
    _engine->DDError9("Post process Low resolution r2t texture creation failed", err);
    return false;
  }

  // Create depth buffer (as it must be set in order the query to work with some ATI graphics cards)
  err = _engine->CreateDepthStencilSurface(R2TMinResolution, R2TMinResolution, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, _rtsMeasureDepthR2T, _rtsMeasureR2T, NULL);
  if (FAILED(err))
  {
    _engine->DDError9("Measure r2t depth buffer creation failed",err);
    return false;
  }

#endif

  //////////////////////////////////////////////////////////////////////////

  {
    ComRef<IDirect3DVertexBuffer9> vb;
    // Create the VB + declaration
    if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexCustomEdge) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
    {
      LogF("Error: Vertex buffer creation failed.");
      return false;
    }
    void *data;
    vb->Lock(0, 0, &data, 0);
    VertexCustomEdge *v = (VertexCustomEdge *)data;
    v[0].pos = Vector3P(-0.5, -0.5, 0.5); v[0].index = 0;
    v[1].pos = Vector3P(-0.5,  0.5, 0.5); v[1].index = 1;
    v[2].pos = Vector3P( 0.5, -0.5, 0.5); v[2].index = 2;
    v[3].pos = Vector3P( 0.5,  0.5, 0.5); v[3].index = 3;
    vb->Unlock();

    _vbCustomEdge = VertexStaticData9::BufferFuture(vb);

    // Create the VD of the blur vertex
    const D3DVERTEXELEMENT9 vertexDeclCustomEdge[] =
    {
      {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
      {0, 12, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
      D3DDECL_END()
    };
    if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDeclCustomEdge, _vdCustomEdge.Init())))
    {
      LogF("Error: Vertex declaration creation failed.");
      return false;
    }
  }

  // Create the shaders
  if (!CompileVertexShader(vs,"VSPostProcessCustomEdge4T",_vsCustomEdge))
  {
    return false;
  }
  if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleThermalLuminance",_psLuminance))
  {
    return false;
  }
  if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleAvgLuminance",_psAvgLuminance))
  {
    return false;
  }
  if (!CompilePixelShader(hlsl,"PSPostProcessQuerySimple",_psQuery))
  {
    return false;
  }

  // Initialize the histogram values
  for (int i = 0; i < NBrightQueries; i++)
  {
    _histogram[i] = -1.0f;
  }

  return true;
}

#define _SAVE_RTs_TO_FILES_ 0

#if _HELISIM_RENDER_TO_TEXTURE

//-----------------------------------------------------------------------------
void EngineDD9::PPDecimate::DoDirect(const RECT *rect, int idx)
{
  PROFILE_SCOPE_GRF_EX(ppDec,ppR2t,SCOPE_COLOR_YELLOW);

  ComRef<IDirect3DDevice9> device = _engine->GetDirect3DDevice().GetRefForCB();
  
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Decimate::DoDirect: ");

  int width = rect->right - rect->left;
  int height = rect->bottom - rect->top;

  DoAssert(_engine->_r2tTargets[idx]._tiPars.NotNull());
  R2TTiPars &tiPars = *(_engine->_r2tTargets[idx]._tiPars.GetRef());
  
  // Set the vertex shader + stream source
  device->SetStreamSource(0, 
#if _ARMA3_VERTEX_BUFFER_EX
    _vbCustomEdge.GetResult().GetBuffer(), 
#else
    _vbCustomEdge.GetResult(), 
#endif
    0, sizeof(VertexCustomEdge));
  device->SetVertexDeclaration(_vdCustomEdge);
  device->SetVertexShader(_vsCustomEdge);

  // Set temporary render states
  device->SetRenderState(D3DRS_ZENABLE, FALSE);
  device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  // Set the src blend and dest blend somehow (they are not going to be set in the DoPrepareTriangle and the temporary setting returning would set a undefined value)
  device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
  device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
  device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, FALSE);
  device->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, FALSE);
  device->SetSamplerState(3, D3DSAMP_SRGBTEXTURE, FALSE);

  // by default assume bi-linear filtering
  device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

  device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  device->SetSamplerState(3, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  device->SetSamplerState(3, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  EdgeParams pars;
  {
#if _ARMA3_HDR_IMPROVEMENTS
    float texCoef = 1.0f - _engine->GetHDRLuminanceRectCoef();
    saturate(texCoef, 0.0f, 1.0f);
    texCoef *= 0.5f;

    float minTexCoord = 0.0f + texCoef;
    float maxTexCoord = 1.0f - texCoef;
    pars.SetupBilinear(_decR2T[0]._w, _decR2T[0]._h, width,height, minTexCoord, maxTexCoord);
#else
    pars.SetupBilinear(_decR2T[0]._w, _decR2T[0]._h, width, height);
#endif

    // Create RT copy
    ComRef<IDirect3DTexture9> rtCopy = _engine->CreateR2TRenderTargetCopyAsTexture(device, rect, D3DTEXF_LINEAR);

#if _SAVE_RTs_TO_FILES_
    D3DXSaveTextureToFile("C:\\rtCopy.dds", D3DXIFF_DDS, rtCopy, NULL);
#endif

    // Set temporal RT
    device->SetRenderTarget(0, _decR2T[0]._rts);
    device->SetDepthStencilSurface(_engine->_depthBuffer);
    device->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);

    // Set Viewport
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData, 0, sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = _decR2T[0]._w;
      viewData.Height = _decR2T[0]._h;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      device->SetViewport(&viewData);
    }

    // Set the pixel shader
    device->SetPixelShader(_psLuminance);
    device->SetVertexShaderConstantF(0, pars.u[0], 9);

    device->SetTexture(0, rtCopy);
    device->SetTexture(1, rtCopy);
    device->SetTexture(2, rtCopy);
    device->SetTexture(3, rtCopy);

    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }

  // perform 4x decimation + average calculation
  {
    // Set RT
    device->SetRenderTarget(0, _decR2T[1]._rts);
    device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

#if _SAVE_RTs_TO_FILES_
    D3DXSaveSurfaceToFile("C:\\psLuminance.dds", D3DXIFF_DDS, _decR2T[0]._rts, NULL, NULL);
#endif

    // Set Viewport
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData, 0, sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = _decR2T[1]._w;
      viewData.Height = _decR2T[1]._h;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      device->SetViewport(&viewData);
    }

    device->SetPixelShader(_psAvgLuminance);

    device->SetTexture(0, _decR2T[0]._rt);
    device->SetTexture(1, _decR2T[0]._rt);
    device->SetTexture(2, _decR2T[0]._rt);
    device->SetTexture(3, _decR2T[0]._rt);

    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }

  // Do the measurement
  {
    // Issue a query
    static float Alphas[NBrightQueries] = 
    {
      0.0f,
      1.0f / NBrightQueries,
      2.0f / NBrightQueries,
      3.0f / NBrightQueries,
      4.0f / NBrightQueries,
      5.0f / NBrightQueries,
      6.0f / NBrightQueries,
      7.0f / NBrightQueries,
      8.0f / NBrightQueries,
      9.0f / NBrightQueries,
      10.0f / NBrightQueries,
      11.0f / NBrightQueries,
      12.0f / NBrightQueries,
      13.0f / NBrightQueries,
      14.0f / NBrightQueries,
      15.0f / NBrightQueries
    };

    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

    device->SetRenderTarget(0, _rtsMeasureR2T);
    device->SetDepthStencilSurface(_rtsMeasureDepthR2T);
    device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    device->SetTexture(0, _decR2T[1]._rt);
    device->SetPixelShader(_psQuery);

    // Set Viewport
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData, 0, sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = R2TMinResolution;
      viewData.Height = R2TMinResolution;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      device->SetViewport(&viewData);
    }

#if _SAVE_RTs_TO_FILES_
    D3DXSaveSurfaceToFile("C:\\avgLum.dds", D3DXIFF_DDS, _decR2T[1]._rts, NULL, NULL);
#endif
    tiPars._queryR2T->Issue(device, NULL, Alphas, -1.0f);

    tiPars._queryR2T->GetNumberOfPixelsDrawn(tiPars._temperaturesR2T, NULL, true);

    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
  }

	GPU_END_PROFILE
}

#endif


//-----------------------------------------------------------------------------
void EngineDD9::PPDecimate::Do(bool isLast)
{
  PROFILE_SCOPE_GRF_EX(ppDec,pp,SCOPE_COLOR_YELLOW);
  
  const int cb = -1;
  
  Prepare();
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Decimate: ");

  // Set the vertex shader + stream source
  _engine->SetStreamSource(cb,_vbCustomEdge, sizeof(VertexCustomEdge), 4);
  _engine->SetVertexDeclaration(cb,_vdCustomEdge);
  _engine->SetVertexShader(cb,_vsCustomEdge);

  // Set temporary states
  SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
  // no alpha or fog done here
  SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE, FALSE);
  SetRenderStateTemp rsALPHATESTENABLE(D3DRS_ALPHATESTENABLE, FALSE);
  SetRenderStateTemp rsALPHABLENDENABLE(D3DRS_ALPHABLENDENABLE, FALSE);
  // no SRGB
  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, FALSE);
  SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, FALSE);
  SetSamplerStateTemp ss2SRGBTEXTURE(2, D3DSAMP_SRGBTEXTURE, FALSE);
  SetSamplerStateTemp ss3SRGBTEXTURE(3, D3DSAMP_SRGBTEXTURE, FALSE);
  // by default assume bi-linear filtering
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss3MINFILTER(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss3MAGFILTER(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  // tri-linear filtering never done 
  SetSamplerStateTemp ss0MIPFILTER(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss1MIPFILTER(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss2MIPFILTER(2, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
  SetSamplerStateTemp ss3MIPFILTER(3, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
  // clamping all stages
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss2ADDRESSU(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss2ADDRESSV(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss3ADDRESSU(3, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss3ADDRESSV(3, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // Save VS constants and restore them at the end
  PushVertexShaderConstant pushC0(0);
  PushVertexShaderConstant pushC1(1);
  PushVertexShaderConstant pushC2(2);
  PushVertexShaderConstant pushC3(3);
  PushVertexShaderConstant pushC4(4);
  PushVertexShaderConstant pushC5(5);
  PushVertexShaderConstant pushC6(6);
  PushVertexShaderConstant pushC7(7);
  PushVertexShaderConstant pushC8(8);

  // perform initial luminance calculation + 4x decimation
  {
    EdgeParams pars;
#if _ARMA3_HDR_IMPROVEMENTS
    float texCoef = 1.0f - _engine->GetHDRLuminanceRectCoef();
    saturate(texCoef, 0.0f, 1.0f);
    texCoef *= 0.5f;

    float minTexCoord = 0.0f + texCoef;
    float maxTexCoord = 1.0f - texCoef;
    pars.SetupBilinear(_dec[0]._w,_dec[0]._h,_engine->Width(),_engine->Height(), minTexCoord, maxTexCoord);
#else
    pars.SetupBilinear(_dec[0]._w,_dec[0]._h,_engine->Width(),_engine->Height());
#endif
    _engine->SetVertexShaderConstantF(cb,0, pars.u[0], 9);

    // Set the pixel shader
    _engine->SetPixelShader(cb,_psLuminance);

    // Remember the current render target (prior we set the temporary one)
    IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();

     // Set temporarily render target and depth stencil to horizontally shrunk texture
    _engine->SetTemporaryRenderTarget(_dec[0]._rt, _dec[0]._rts, NULL);

		{
			D3DVIEWPORT9 viewData;
			memset(&viewData, 0, sizeof(viewData));
			viewData.X = 0;
			viewData.Y = 0;
			viewData.Width  = _dec[0]._w;
			viewData.Height = _dec[0]._h;
			viewData.MinZ = 0.0f;
			viewData.MaxZ = 1.0f;
			CALL_D3D_MEMBER(_engine, SetViewport, viewData);
		}

    // set the source as 4 textures    
    SetTextureTemp tTex0(0, rt);
    SetTextureTemp tTex1(1, rt);
    SetTextureTemp tTex2(2, rt);
    SetTextureTemp tTex3(3, rt);

    _engine->DrawPrimitive(cb,D3DPT_TRIANGLESTRIP, 0, 2);
    
    _engine->ResolveRenderTarget();
  }

  // perform 4x decimation + average calculation
  {
    for (int i=1; i<_decimations; i++)
    {
      // perform initial luminance calculation + 4x decimation
      EdgeParams pars;
      pars.SetupBilinear(_dec[i]._w,_dec[i]._h,_dec[i-1]._w,_dec[i-1]._h);

      _engine->SetVertexShaderConstantF(cb,0, pars.u[0], 9);

      // Set the pixel shader
      _engine->SetPixelShader(cb,_psAvgLuminance);

      // Set the render target (we can use the SetRenderTarget method here, because we know the original is already stored and w)
      _engine->SetRenderTarget(_dec[i]._rts, _dec[i]._rt, NULL);

      {
        D3DVIEWPORT9 viewData;
        memset(&viewData, 0, sizeof(viewData));
        viewData.X = 0;
        viewData.Y = 0;
        viewData.Width  = _dec[i]._w;
        viewData.Height = _dec[i]._h;
        viewData.MinZ = 0.0f;
        viewData.MaxZ = 1.0f;
        CALL_D3D_MEMBER(_engine, SetViewport, viewData);
      }

      IDirect3DTexture9 *tex = _dec[i-1]._rt;

      // set the source as 4 textures    
      SetTextureTemp tTex0(0, tex);
      SetTextureTemp tTex1(1, tex);
      SetTextureTemp tTex2(2, tex);
      SetTextureTemp tTex3(3, tex);

      _engine->DrawPrimitive(cb,D3DPT_TRIANGLESTRIP, 0, 2);
      _engine->ResolveRenderTarget();
    }
  }

  // Do the measurement
  {
    // It looks Z-buffer must be enabled in order the occlusion query to work on some ATI HW
    // see http://discussms.hosting.lsoft.com/SCRIPTS/WA-MSD.EXE?A2=ind0502B&L=DIRECTXDEV&D=0&I=-3&X=60AD7D5C1D013612AF&P=3658
    SetRenderStateTemp rsZENABLE(D3DRS_ZENABLE, TRUE);
    SetRenderStateTemp rsZFUNC(D3DRS_ZFUNC, D3DCMP_ALWAYS);

    _engine->SetRenderTarget(_rtsMeasure, _rtMeasure, _rtsMeasureDepth);
    _engine->SetTexture(cb,0, _dec[_decimations-1]._rt);
    _engine->SetPixelShader(cb,_psQuery);

    // Issue a query
    float alphas[NBrightQueries];
    for (int i = 0; i < NBrightQueries; i++)
    {
      alphas[i] = (float)i / NBrightQueries;
    }
    _query.Issue(NULL, alphas, -1.0f);
  }

  // Build histogram
  {
    // Get pixel counts for every layer
    int numberOfPixelsDrawn[NBrightQueries];
    _query.GetNumberOfPixelsDrawn(numberOfPixelsDrawn, NULL, true);

    // Build the histogram
    int totalPixels = _dec[_decimations - 1]._w * _dec[_decimations - 1]._h;
    for (int i = 0; i < NBrightQueries - 1; i++)
    {
      _histogram[i] = (float)(numberOfPixelsDrawn[i] - numberOfPixelsDrawn[i + 1]) / totalPixels;
      _temperatures[i] = (numberOfPixelsDrawn[i]);
    }
    _histogram[NBrightQueries - 1] = (float)numberOfPixelsDrawn[NBrightQueries - 1] / totalPixels;
    _temperatures[NBrightQueries - 1] = numberOfPixelsDrawn[NBrightQueries - 1];

    if (CHECK_DIAG(DETIHistogram))
    {
      DIAG_MESSAGE(200, Format(" 0: %f, %d", _histogram[0], _temperatures[0]));
      DIAG_MESSAGE(200, Format(" 1: %f, %d", _histogram[1], _temperatures[1]));
      DIAG_MESSAGE(200, Format(" 2: %f, %d", _histogram[2], _temperatures[2]));
      DIAG_MESSAGE(200, Format(" 3: %f, %d", _histogram[3], _temperatures[3]));
      DIAG_MESSAGE(200, Format(" 4: %f, %d", _histogram[4], _temperatures[4]));
      DIAG_MESSAGE(200, Format(" 5: %f, %d", _histogram[5], _temperatures[5]));
      DIAG_MESSAGE(200, Format(" 6: %f, %d", _histogram[6], _temperatures[6]));
      DIAG_MESSAGE(200, Format(" 7: %f, %d", _histogram[7], _temperatures[7]));
      DIAG_MESSAGE(200, Format(" 8: %f, %d", _histogram[8], _temperatures[8]));
      DIAG_MESSAGE(200, Format(" 9: %f, %d", _histogram[9], _temperatures[9]));
      DIAG_MESSAGE(200, Format("10: %f, %d", _histogram[10], _temperatures[10]));
      DIAG_MESSAGE(200, Format("11: %f, %d", _histogram[11], _temperatures[11]));
      DIAG_MESSAGE(200, Format("12: %f, %d", _histogram[12], _temperatures[12]));
      DIAG_MESSAGE(200, Format("13: %f, %d", _histogram[13], _temperatures[13]));
      DIAG_MESSAGE(200, Format("14: %f, %d", _histogram[14], _temperatures[14]));
      DIAG_MESSAGE(200, Format("15: %f, %d", _histogram[15], _temperatures[15]));
    }
  }

	GPU_END_PROFILE

  // Restore the original render target
  _engine->RestoreOldRenderTarget();
}


//-----------------------------------------------------------------------------
int EngineDD9::PPFinalThermal::SetParams(const float *pars, int nPars)
{
  Assert(nPars==1);
  _deltaT = pars[0];

  return 0;
}


//-----------------------------------------------------------------------------
bool EngineDD9::PPFinalThermal::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs,"VSPostProcess")) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessThermalPresent", _ps)) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessThermal", _psThermal)) return false;
  if (!_ppDecimate.Init(hlsl, vs)) return false;
  if (!_ppGaussianBlur.Init(hlsl, vs)) return false;

  UINT width = _engine->Width();
  UINT height = _engine->Height();

  HRESULT err = _engine->CreateTextureRT(width, height, 1, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtTexture, _rtSurface, true);
  if (err != S_OK)
  {
    _engine->DDError9("Error: Render target texture creation failed.", err); 
    return false;
  }

  //////////////////////////////////////////////////////////////////////////

#if _HELISIM_RENDER_TO_TEXTURE

  err = _engine->CreateTextureRT(R2TMaxResolution, R2TMaxResolution, 1, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtTextureR2T, _rtSurfaceR2T, false);
  if (err != S_OK)
  {
    _engine->DDError9("Error: Render target texture creation failed.", err); 
    return false;
  }

  err = _engine->CreateTextureRT(R2TMaxResolution / 4, R2TMaxResolution / 4, 1, _engine->_rtFormat, D3DPOOL_DEFAULT,
    _blurRT, _blurRTS, false);

  if (err != S_OK)
  {
    _engine->DDError9("Error: Blur texture creation failed.", err);
    return false;
  }

#endif

  //////////////////////////////////////////////////////////////////////////

  _deltaT = 0;

  return true;
}

// TODO: avoid scene here
#include "../scene.hpp"
#include "../txtPreload.hpp"

extern bool ForceTIAlive;
extern bool ForceTIMovement;
extern bool ForceTIMetabolism;

#if _HELISIM_RENDER_TO_TEXTURE


//-----------------------------------------------------------------------------
void EngineDD9::PPFinalThermal::DoDirect(const RECT *rect, int idx)
{
  PROFILE_SCOPE_GRF_EX(ppTR2,ppR2t,SCOPE_COLOR_YELLOW);

  ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

  ComRef<IDirect3DSurface9> _renderTargetSurfaceSaved;
  ComRef<IDirect3DSurface9> _renderTargetDepthBufferSurfaceSaved;

  // Save RTs
  device->GetRenderTarget(0, _renderTargetSurfaceSaved.Init());
  device->GetDepthStencilSurface(_renderTargetDepthBufferSurfaceSaved.Init());

  R2TTiPars &tiPars = *(_engine->_r2tTargets[idx]._tiPars.GetRef());

  // Perform temperature calculations
  _ppDecimate.DoDirect(rect, idx);

  float dTemp = (float) (TempMax - TempMin) / NBrightQueries;
  const int MinPixelsDrawn = 25;
  float maxTemperature = 0.0f;

  for (int i = 0; i < NBrightQueries; i++)
  {
    if (tiPars._temperaturesR2T[i] > MinPixelsDrawn) maxTemperature = (i + 1) * dTemp;
  }

  static float latency = 0.85f;
  maxTemperature += (dTemp * 0.25f);
  tiPars._maxTemperatureR2T = (tiPars._maxTemperatureR2T + (maxTemperature - tiPars._maxTemperatureR2T) * tiPars._deltaT* latency);

  const float TemperatureMin = 0.5f;
  const float TemperatureMax = 30.0f;
  const float TemeperatureInvDiff = 1.0f / (TemperatureMax - TemperatureMin);

  float tempDiff = (tiPars._maxTemperatureR2T - _engine->_airTemperature);
  saturate(tempDiff, TemperatureMin, TemperatureMax);
  float dT = (tempDiff - TemperatureMin) * TemeperatureInvDiff;
  tempDiff *= (1.0f - sqrtf(dT)) + 1.0f;

  float tempRangeMin = _engine->_airTemperature - tempDiff;
  float tempRangeMax = _engine->_airTemperature + tempDiff;
  saturateMax(tempRangeMin, 0.0f);

  float tempRangeMin01 = (tempRangeMin - TempMin) * InvTempDiff;
  float tempRangeMax01 = (tempRangeMax - TempMin) * InvTempDiff;
  float invTempRangeDiff = 1.0f / (tempRangeMax01 - tempRangeMin01);

  {
    device->SetRenderState(D3DRS_ZENABLE, FALSE);

    // Set stream
    device->SetStreamSource(0, 
#if _ARMA3_VERTEX_BUFFER_EX
      _vb.GetResult().GetBuffer(), 
#else
      _vb.GetResult(), 
#endif
      0, sizeof(Vertex));
    // Set the vertex declaration
    device->SetVertexDeclaration(_vd);
    // Set the vertex shader
    device->SetVertexShader(_vs);
    // Set pixel shader
    device->SetPixelShader(_psThermal);

    device->SetRenderTarget(0, _rtSurfaceR2T);
    device->SetDepthStencilSurface(_engine->_depthBuffer);  // use scene depth
    device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

    // Set viewport
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData, 0, sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = R2TMaxResolution;
      viewData.Height = R2TMaxResolution;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      device->SetViewport(&viewData);
    }

    ComRef<IDirect3DTexture9> rt = _engine->CreateR2TRenderTargetCopyAsTexture(device, rect, D3DTEXF_POINT);
    device->SetTexture(0, rt);
    device->SetTexture(1, _engine->_tiConversion);

    device->SetRenderState(D3DRS_ZENABLE, FALSE);

    device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    float psConst[4] = { tempRangeMin01, tempRangeMax01, invTempRangeDiff, tiPars._tiMode };
    device->SetPixelShaderConstantF(0, &psConst[0], 1);

    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }

  // Switch to back buffer
  device->SetRenderTarget(0, _engine->_r2tRT._renderTargetSurfaceS);
  device->SetDepthStencilSurface(_engine->GetFSAA() > 0 ? _engine->_depthBufferRT : _engine->_depthBuffer);
  device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

  // Set viewport
  {
    D3DVIEWPORT9 viewData;
    memset(&viewData,0,sizeof(viewData));
    viewData.X = 0;
    viewData.Y = 0;
    viewData.Width  = rect->right - rect->left;
    viewData.Height = rect->bottom - rect->top;
    viewData.MinZ = 0.0f;
    viewData.MaxZ = 1.0f;
    device->SetViewport(&viewData);
  }

    // Fake blur
  device->StretchRect(_rtSurfaceR2T, NULL, _blurRTS, NULL, D3DTEXF_LINEAR);

#if _SAVE_RTs_TO_FILES_
  D3DXSaveSurfaceToFile("C:\\FakeBlur.dds", D3DXIFF_DDS, _blurRTS, NULL, NULL);
#endif

  device->SetTexture(0, _rtTextureR2T);
  device->SetTexture(1, _blurRT);

  device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

  device->SetPixelShader(_ps);

  static float TiBlurCoeff = 0.5f;
  float psConst[4] = { 0, 0, 0, TiBlurCoeff };
  device->SetPixelShaderConstantF(0, &psConst[0], 1);

  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

#if _SAVE_RTs_TO_FILES_
  D3DXSaveSurfaceToFile("C:\\final.dds", D3DXIFF_DDS, _engine->_r2tRT._renderTargetSurfaceS, NULL, NULL);
#endif

  // Restore RTs
  device->SetRenderTarget(0, _renderTargetSurfaceSaved);
  device->SetDepthStencilSurface(_renderTargetDepthBufferSurfaceSaved);

  _renderTargetSurfaceSaved.Free();
  _renderTargetDepthBufferSurfaceSaved.Free();

  // Restore render state
  device->SetRenderState(D3DRS_ZENABLE, TRUE);
  device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
}

#endif


//-----------------------------------------------------------------------------
void EngineDD9::PPFinalThermal::Do(bool isLast)
{
  PROFILE_SCOPE_GRF_EX(ppThr,pp,SCOPE_COLOR_YELLOW);
 
  const int cb = -1;
  
  // If the _currRenderTargetSurface is equal to _backBuffer, then it's strange and we probably have got a problem here
  DoAssert(_engine->_currRenderTargetSurface != _engine->_backBuffer);

#if (_ENABLE_CHEATS | _BULDOZER)

# if(_BULDOZER)
#  define TI_KEY GInput.GetKeyToDo
# else 
#  define TI_KEY GInput.GetCheat2ToDo
# endif
  /*if (TI_KEY(DIK_F9))
  {
    _engine->SetTIBrightnessRelative(_engine->GetTIBrightness() + 1.0f);
    GlobalShowMessage(500, Format("TI Brightness: %f", _engine->GetTIBrightness()));
  }
  if (TI_KEY(DIK_F10))
  {
    _engine->SetTIBrightnessRelative(_engine->GetTIBrightness() - 1.0f);
    GlobalShowMessage(500, Format("TI Brightness: %f", _engine->GetTIBrightness()));
  }
  if (TI_KEY(DIK_F11))
  {
    _engine->_tiContrast += 1.0f;
    GlobalShowMessage(500, Format("TI Contrast: %f", _engine->_tiContrast));
  }
  if (TI_KEY(DIK_F12))
  {
    _engine->_tiContrast -= 1.0f;
    GlobalShowMessage(500, Format("TI Contrast: %f", _engine->_tiContrast));
  }*/
  if (TI_KEY(DIK_F8))
  {
    _engine->_tiMode++;
    GlobalShowMessage(500, Format("TI Mode: %d", _engine->_tiMode));
  }
  if (TI_KEY(DIK_F7))
  {
    _engine->_tiMode--;
    GlobalShowMessage(500, Format("TI Mode: %d", _engine->_tiMode));
  }
  if (TI_KEY(DIK_F6))
  {
    _engine->_tiBlurCoef += 0.1f;
    if (_engine->_tiBlurCoef > 1.001f) _engine->_tiBlurCoef = 0.0f;
    GlobalShowMessage(500, Format("TI Blur level: %f", _engine->_tiBlurCoef));
  }
  if (TI_KEY(DIK_F1))
  {
    ForceTIAlive = !ForceTIAlive;
    GlobalShowMessage(500, Format("ForceTIAlive: %s", ForceTIAlive ? "on" : "off"));
  }
  if (TI_KEY(DIK_F2))
  {
    ForceTIMovement = !ForceTIMovement;
    GlobalShowMessage(500, Format("ForceTIMovement: %s", ForceTIMovement ? "on" : "off"));
  }
  if (TI_KEY(DIK_F3))
  {
    ForceTIMetabolism = !ForceTIMetabolism;
    GlobalShowMessage(500, Format("ForceTIMetabolism: %s", ForceTIMetabolism ? "on" : "off"));
  }
  if (TI_KEY(DIK_F5))
  {
    _engine->_tiAutoBC = !_engine->_tiAutoBC;
    GlobalShowMessage(500, Format("TI AutoContrast: %s", _engine->_tiAutoBC ? "on" : "off"));
  }
#undef TI_KEY 
#endif

  // Perform the automatic setting of brightness and contrast
  if (_engine->_tiAutoBC)
  {
    // Get the temperature histogram (decode the temperature)
    _ppDecimate.Do(isLast);

    // Calculate the expected value
    //float e = 0.0f;
    float maxTemperature = 0.0f;

    float dTemp = (float) (TempMax - TempMin) / NBrightQueries;

    // 1% per pixels from image(4096)
    static int MinPixelsDrawn = 100;

    for (int i = 0; i < NBrightQueries; i++)
    {
      //e += i * _ppDecimate._histogram[i];
      if (_ppDecimate._temperatures[i] > MinPixelsDrawn) maxTemperature = (i + 1) * dTemp;
    }

#if 0
    // Calculate the variance
    float var = 0.0f;
    for (int i = 0; i < NBrightQueries; i++)
    {
      var += (i-e) * (i-e) * _ppDecimate._histogram[i];
    }
   
    // Convert e and var to temperatures
    float tempE = TempMin + (TempMax - TempMin) * (e + 0.5f) / NBrightQueries;
    float tempVAR = TempMin + (TempMax - TempMin) * var / NBrightQueries;

    // Calculate the desired brightness and contrast
    static float ConstrastMin = 8.0f;
    static float ContrastMult = 5.0f;

    // Calculate the desired brightness and contrast
    float brightnessWanted = tempE;
    float contrastWanted = max(tempVAR * ContrastMult, ConstrastMin);
#endif

    float maxTempWanted = maxTemperature + (dTemp * 0.25f);

    // Consider some latency and change the real values accordingly
    static float latency = 0.85f;

#if 0
    _engine->SetTIBrightness(_engine->_tiBrightness + (brightnessWanted - _engine->_tiBrightness) * _deltaT * latency);
    _engine->SetTIContrast(_engine->_tiContrast + (contrastWanted - _engine->_tiContrast) * _deltaT * latency);
#endif
    _engine->_maxTemperature = (_engine->_maxTemperature + (maxTempWanted - _engine->_maxTemperature) * _deltaT * latency);
  }
  else
  {
    _engine->SetTIBrightness(_engine->_tiBrightness);
    _engine->SetTIContrast(_engine->_tiContrast);
    _engine->_maxTemperature = 30.0f;
  }

  // Do the thermal visualization (decode the temperature)
  {
    // Start the scene
    bool inScene = _engine->_d3dFrameOpen;
    if (!inScene) _engine->D3DBeginScene();

    // Prepare drawing
    Prepare();

    // Rewrite the pixel shader
    _engine->SetPixelShader(cb,_psThermal);

    // Current render target as texture
    IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
    DoAssert(rt);

    // Rewrite render target and depth stencil surfaces
    _engine->SetTemporaryRenderTarget(_rtTexture, _rtSurface, NULL);

    D3DSURFACE_DESC desc;
    _rtTexture->GetLevelDesc(0, &desc);
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData,0,sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = desc.Width;
      viewData.Height = desc.Height;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      CALL_D3D_MEMBER(_engine,SetViewport,viewData);
    }

    SetTextureTemp tTex0(0, rt);
    SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, FALSE);
    SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // Get the TIConversion texture to the input
    SetTextureTemp tTex1(1, _engine->_tiConversion);
    SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, FALSE);
    SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // Set temporary render states
    SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    SetRenderStateTemp rsZENABLE(D3DRS_ZENABLE, FALSE);
    SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE, FALSE);
    SetRenderStateTemp rsSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);

    // Control the temperature range to be displayed by constants
    // const float tempRangeMin = 0.0f;
    // const float tempRangeMax = 60.0f;
    // #define TempRangeMin01 0.18181818182f //(tempRangeMin - TempMin) * InvTempDiff
    // #define TempRangeMax01 0.63636363636f //(tempRangeMax - TempMin) * InvTempDiff
    // #define InvTempRangeDiff 2.2f // 1.0f / (tempRangeMax01 - tempRangeMin01)

    static float TemperatureMin = 0.5f;
    static float TemperatureMax = 30.0f;
    static float TemeperatureInvDiff = 1.0f / (TemperatureMax - TemperatureMin);

    float tempDiff = (_engine->_maxTemperature - _engine->_airTemperature);
    saturate(tempDiff, TemperatureMin, TemperatureMax);
    float dT = (tempDiff - TemperatureMin) * TemeperatureInvDiff;
    tempDiff *= (1.0f - sqrtf(dT)) + 1.0f;

    float tempRangeMin = _engine->_airTemperature - tempDiff;
    float tempRangeMax = _engine->_airTemperature + tempDiff;
    saturateMax(tempRangeMin, 0.0f);

    float tempRangeMin01 = (tempRangeMin - TempMin) * InvTempDiff;
    float tempRangeMax01 = (tempRangeMax - TempMin) * InvTempDiff;
    float invTempRangeDiff = 1.0f / (tempRangeMax01 - tempRangeMin01);

    if (CHECK_DIAG(DETIHistogram))
    {
      DIAG_MESSAGE(150, "dT: %.3f, temp: %.3f", dT, tempDiff);
      DIAG_MESSAGE(150, "min: %.3f, max: %.3f", tempRangeMin, tempRangeMax);
      DIAG_MESSAGE(150, "min: %.3f, max: %.3f, diff: %.3f", tempRangeMin01, tempRangeMax01, invTempRangeDiff);
    }

    SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(tempRangeMin01, tempRangeMax01, invTempRangeDiff, _engine->_tiMode));

    // Drawing
    _engine->DrawPrimitive(cb,D3DPT_TRIANGLESTRIP, 0, 2);

    _engine->RestoreOldRenderTarget();
  }

  {
    // Blur the render texture
    _ppGaussianBlur.Do(false, _rtTexture);


    PROFILE_SCOPE_GRF_EX(ppFin,pp,SCOPE_COLOR_YELLOW);

    bool inScene = _engine->_d3dFrameOpen;
    if (!inScene) _engine->D3DBeginScene();

    // Prepare drawing
    Prepare();

    // Set temporary render states
    SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    SetRenderStateTemp rsZENABLE(D3DRS_ZENABLE, FALSE);
    SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE, FALSE);

    // Use the back buffer as render target
    _engine->SwitchToBackBufferRenderTarget();

    D3DSURFACE_DESC desc;
    _engine->_backBuffer->GetDesc(&desc);
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData,0,sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = desc.Width;
      viewData.Height = desc.Height;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      CALL_D3D_MEMBER(_engine,SetViewport,viewData);
    }

    // get blurred target
    IDirect3DTexture9 *rtBlur = _ppGaussianBlur.GetBlurredTexture();
    DoAssert(rtBlur);

    // Use the textures
    SetTextureTemp tRenderTarget0(0, _rtTexture);
    SetTextureTemp tRenderTarget1(1, rtBlur);

    // Disable SRGB
    SetSamplerStateTemp tempNoSRGB0(0, D3DSAMP_SRGBTEXTURE, FALSE);
    SetSamplerStateTemp tempNoSRGB1(1, D3DSAMP_SRGBTEXTURE, FALSE);
    SetSamplerStateTemp tempMINFILTER0(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp tempMAGFILTER0(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp tempMINFILTER1(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp tempMAGFILTER1(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    // Set the blur factor
    SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(0, 0, 0, _engine->_tiBlurCoef));

    // Drawing
    _engine->DrawPrimitive(cb,D3DPT_TRIANGLESTRIP, 0, 2);

    if (!inScene) _engine->D3DEndScene();
  }

  Finish();
}


EngineDD9 *EngineDD9::QueryManager::_engine;

//-----------------------------------------------------------------------------
EngineDD9::QueryManager::QueryManager()
{
  // Create the query objects
  for (int i = 0; i < NItems; i++)
  {
    for (int b=0; b<NBrightQueries; b++)
    {
      if (FAILED(_engine->GetDirect3DDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, _query[i][b].Init())))
      {
        LogF("Warning: Current HW doesn't support queries");
        goto Break;
      }
    }
  }
  Break:

  // Initialize the startup values
  ResetQueries();
}


//-----------------------------------------------------------------------------
void EngineDD9::QueryManager::ResetQueries()
{
  // Initialize the startup values
  _newQuery = 0;
  _oldestQuery = 0;
  for (int b=0; b<NBrightQueries; b++)
  {
    _mostRecentResult[b] = -1;
  }
  _queriesIssued = 0;
  _currentResultIndex = 0;
}

//-----------------------------------------------------------------------------
bool EngineDD9::QueryManager::Issue(const float *value, const float *alpha, float masterValue)
{
  Assert(!_engine->IsCBRecording());
  const int cb = -1;
  
  if (_query[_newQuery][0].IsNull()) return false;
  if (_queriesIssued>=NItems) return false;
  
  PushPixelShaderConstant psc7(7);
  for (int b=0; b<NBrightQueries; b++)
  {
    const float queryDot[4]={b,0,0,alpha[b]};
    _engine->SetPixelShaderConstantF(cb,7, queryDot,1);
    _query[_newQuery][b]->Issue(D3DISSUE_BEGIN);
    _value[_newQuery][b] = (value) ? value[b] : -1.0f;
    _engine->DrawPrimitive(cb,D3DPT_TRIANGLESTRIP, 0, 2);
    _query[_newQuery][b]->Issue(D3DISSUE_END);
  }
  _masterValue[_newQuery] = masterValue;
  _queriesIssued++;
  DoAssert(_queriesIssued<=NItems);
  _newQuery = Next(_newQuery);
  return true;
}


//-----------------------------------------------------------------------------
bool EngineDD9::QueryManager::Issue(IDirect3DDevice9 *device, const float *value, const float *alpha, float masterValue)
{
  //Assert(!_engine->IsCBRecording());

  if (_query[_newQuery][0].IsNull()) return false;
  if (_queriesIssued>=NItems) return false;

  for (int b=0; b<NBrightQueries; b++)
  {
    const float queryDot[4]={b,0,0,alpha[b]};
    device->SetPixelShaderConstantF(7, queryDot, 1);
    _query[_newQuery][b]->Issue(D3DISSUE_BEGIN);
    _value[_newQuery][b] = (value) ? value[b] : -1.0f;
    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    _query[_newQuery][b]->Issue(D3DISSUE_END);
  }
  _masterValue[_newQuery] = masterValue;
  _queriesIssued++;
  DoAssert(_queriesIssued<=NItems);
  _newQuery = Next(_newQuery);
  return true;
}


//-----------------------------------------------------------------------------
float EngineDD9::QueryManager::GetNumberOfPixelsDrawn(int *npd, float *value, bool usePixelCount)
{
  #if _DEBUG
  // we may want to introduce artificial latency when debugging
  static int latency = 1;
  #else
  const int latency = 0;
  #endif
  while (_queriesIssued>latency)
  {
    // Get the newest available query result
    // check from beginning to the last
    // once the last one is done, all are done
    Assert(_currentResultIndex<NBrightQueries);
    DWORD nPixels;
    if (_query[_oldestQuery][_currentResultIndex]->GetData(&nPixels, sizeof(DWORD), D3DGETDATA_FLUSH) != S_OK)
    {
      // if query is not ready, break and report what we already have
      break;
    }
    _results[_currentResultIndex] = (usePixelCount) ? nPixels : nPixels>0; // we know we are sampling 1x1 - either 1 or 0 can pass
    _currentResultIndex++;
    if (_currentResultIndex>=NBrightQueries)
    {
      for (int b=0; b<NBrightQueries; b++)
      {
        _mostRecentResult[b] = _results[b];
        _mostRecentResultValue[b] = _value[_oldestQuery][b];
      }
      _mostRecentMasterValue = _masterValue[_oldestQuery];
      // we picked up the result - report it
      // we assume we will be able to pick all other results as well now
      _queriesIssued--;
      _oldestQuery = Next(_oldestQuery);
      _currentResultIndex = 0;
    }
  }
  for (int b=0; b<NBrightQueries; b++)
  {
    if (value) value[b] = _mostRecentResultValue[b];
    npd[b] = _mostRecentResult[b];
  }
  return _mostRecentMasterValue;
}



EngineDD9 *EngineDD9::QueryManagerSinglePass::_engine;

//-----------------------------------------------------------------------------
EngineDD9::QueryManagerSinglePass::QueryManagerSinglePass()
{
  // Create the query objects
  for (int i = 0; i < NItems; i++)
  {
    if (FAILED(_engine->GetDirect3DDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, _query[i].Init())))
    {
      LogF("Warning: Current HW doesn't support queries");
      goto Break;
    }
  }
  Break:

  // Initialize the startup values
  ResetQueries();
}

//-----------------------------------------------------------------------------
void EngineDD9::QueryManagerSinglePass::ResetQueries()
{
  // Initialize the startup values
  _newQuery = 0;
  _oldestQuery = 0;
  for (int b=0; b<NBrightQueries; b++)
  {
    _mostRecentResult[b] = -1;
  }
  _queriesIssued = 0;
}

//-----------------------------------------------------------------------------
bool EngineDD9::QueryManagerSinglePass::Issue(IDirect3DDevice9 *device, const float *value, float masterValue, IDirect3DTexture9 *alphaTex)
{
  if (_query[_newQuery].IsNull()) return false;
  if (_queriesIssued >= NItems) return false;

  const int alphaStage = 1;
  device->SetTexture(alphaStage, alphaTex);
  device->SetSamplerState(alphaStage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  device->SetSamplerState(alphaStage,D3DSAMP_MINFILTER,D3DTEXF_POINT);
  device->SetSamplerState(alphaStage,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
  device->SetSamplerState(alphaStage,D3DSAMP_SRGBTEXTURE,FALSE);
  device->SetSamplerState(alphaStage,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
  device->SetSamplerState(alphaStage,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);

  _query[_newQuery]->Issue(D3DISSUE_BEGIN);

  for (int b = 0; b < NBrightQueries; b++)
  {
    _value[_newQuery][b] = (value) ? value[b] : -1.0f;
  }
  device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  _query[_newQuery]->Issue(D3DISSUE_END);

  _masterValue[_newQuery] = masterValue;
  _queriesIssued++;
  DoAssert(_queriesIssued<=NItems);
  _newQuery = Next(_newQuery);

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::QueryManagerSinglePass::Issue(const float *value, float masterValue, IDirect3DTexture9 *alphaTex)
{
  Assert(!_engine->IsCBRecording());
  const int cb = -1;
  
  if (_query[_newQuery].IsNull()) return false;
  if (_queriesIssued>=NItems) return false;
  
  const int alphaStage = 1;
  PushPixelShaderConstant psc7(7);
  SetTextureTemp sampling(alphaStage,alphaTex);
  SetSamplerStateTemp samplingMagX(alphaStage,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
  SetSamplerStateTemp samplingMinX(alphaStage,D3DSAMP_MINFILTER,D3DTEXF_POINT);
  SetSamplerStateTemp samplingMipX(alphaStage,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
  SetSamplerStateTemp samplingSrgb(alphaStage,D3DSAMP_SRGBTEXTURE,FALSE);
  SetSamplerStateTemp samplingClampU(alphaStage,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
  SetSamplerStateTemp samplingClampV(alphaStage,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
  
  HRESULT hr = _query[_newQuery]->Issue(D3DISSUE_BEGIN);
  if (FAILED(hr))
  {
    _engine->DDError9("Query issue failed", hr);
    return false;
  }
  for (int b=0; b<NBrightQueries; b++)
  {
    _value[_newQuery][b] = (value) ? value[b] : -1.0f;
  }
  _engine->DrawPrimitive(cb,D3DPT_TRIANGLESTRIP, 0, 2);
  hr = _query[_newQuery]->Issue(D3DISSUE_END);
  if (FAILED(hr))
  {
    _engine->DDError9("Query issue failed", hr);
    return false;
  }
  _masterValue[_newQuery] = masterValue;
  _queriesIssued++;
  DoAssert(_queriesIssued<=NItems);
  _newQuery = Next(_newQuery);
  return true;
}

//-----------------------------------------------------------------------------
static inline int AddSaturated(int a, int b)
{
  long long sum = (long long)a+b;
  if (sum>INT_MAX) sum = INT_MAX;
  return (int)sum;
}


//-----------------------------------------------------------------------------
float EngineDD9::QueryManagerSinglePass::GetNumberOfPixelsDrawn(int *npd, float *value)
{
  #if _DEBUG
  // we may want to introduce artificial latency when debugging
  static int latency = 0;
  #else
  const int latency = 0;
  #endif
  // even with 0 frames ahead we expect to get query latency 1, hence latency here needs to be one frame higher
  int maxLatency = AddSaturated(_engine->_framesAhead,1); 
  
  PROFILE_SCOPE_EX(ppQWt,pp);
  while (_queriesIssued>latency)
  {
    DWORD nPixels;
    HRESULT ret;
    do 
    {
      // check oldest query first - it has to be completed first
      ret = _query[_oldestQuery]->GetData(&nPixels, sizeof(DWORD), D3DGETDATA_FLUSH);
      if (ret!=S_FALSE) break;
    } while (_queriesIssued>maxLatency); // prevent query latency being too high - this results in mouse latency
    // if query has succeeded, process it
    if (ret==S_OK)
    {
      // value nPixels means nPixels has succeeded, rest has failed

#if 1
      if (nPixels > NBrightQueries)
      {
        LogF("DirectX Query failed - returned %d pixels.", nPixels);
        // This sometimes happens on ATI cards, not sure why - GetData returns invalid number of pixels (random value it seems...).
        // This caused excessive blinking of picture.

        // So it is better to ignore this query... (because eye accom computation was wrong because of it)
      }
      else
#endif
      {
      for (int i=0; i<NBrightQueries; i++)
      {
        _mostRecentResultValue[i] = _value[_oldestQuery][i];
        _mostRecentResult[i] = i<(int)nPixels;
      }
      _mostRecentMasterValue = _masterValue[_oldestQuery];
      }
      // we picked up the result - report it
      // we assume we will be able to pick all other results as well now
      _queriesIssued--;
      _oldestQuery = Next(_oldestQuery);
    }
    else
    {
      break;
    }
  }
  for (int b=0; b<NBrightQueries; b++)
  {
    if (value) value[b] = _mostRecentResultValue[b];
    if (npd) npd[b] = _mostRecentResult[b];
  }
  saturateMax(_engine->_framesAheadDetected,_queriesIssued-1);
  return _mostRecentMasterValue;
}

/**
we are not interested about very small changes
on the other hand, GPU exposure control does big changes not very often
the change we do not adjust does gradually build up into a bigger change
*/

static const float HistInit[NBrightQueries]=
{
  0.1f,0.36f,0.50f,0.70f,0.80f,0.89f,0.93f,0.95f,
  1.05f,1.07f,1.11f,1.20f,1.30f,1.45f,1.64f,1.99f,
};

/*
// table could be approximated by following function.
// However, experiments led to numerical instability, and as it was for Arma2 patch,
// change in this area was considered too risky

static float HistInit(int x)
{
  // 0 1 2 3 4 5 6 7 8 9 A B C D E F
  //                 *
  //-7-6-5-4-3-2-1 0 1 2 3 4 5 6 7 8
  Assert(x>=0 && x<NBrightQueries);
  const int half = NBrightQueries/2;
  if (x>=half)
  {
    return 1+abs((x-(half-1))*(1.0f/half));
  }
  else
  {
    return 1-abs((x-(half-1))*(1.0f/half));
  }
}
*/

//-----------------------------------------------------------------------------
bool EngineDD9::PPGlow::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, PPGaussianBlur *ppGaussianBlur)
{
  if (!base::Init(hlsl,vs)) return false;
  
  _ppGaussianBlur = ppGaussianBlur;

  // Create the low resolution textures cascade
  
  // we need textures for 4x average decimation
  // and then some for 2x avg/min/max decimation
  // we want to start calculating min/max once the picture is coarse enough
  const int startMinMaxAtSize = 80;
  
  int fullWidth = _engine->Width();
  int fullHeight = _engine->Height();
  
  // decimation 2x uses point sampling and should therefore be very accurate
  // we will round to nearest bigger pow2
  int w = roundTo2PowerNCeil(fullWidth);
  int h = roundTo2PowerNCeil(fullHeight);

  int maxSize = max(w,h);
  int minSize = min(w,h);
  
  D3DFORMAT rtFormat = _engine->_downsampleFormat;
  int level = 0;
  // we need at least one 4x decimation step
  do
  {
    DecimationPass &dec = _dec[level];
    w = w>>2;
    h = h>>2;
    maxSize = maxSize>>2;
    minSize = minSize>>2;
    // create the supplementary RT
    int effW = max(w,1);
    int effH = max(h,1);
    LogF("Decimation 4x: %d,%d",effW,effH);
    HRESULT err = _engine->CreateTextureRT(effW, effH, 1, rtFormat, D3DPOOL_DEFAULT, dec._rt, dec._rts, true);
    if (FAILED(err))
    {
      _engine->DDError9("Post process Low resolution texture creation failed",err);
      return false;
    }
    dec._w = effW;
    dec._h = effH;
    // proceed to next level
    level++;
  } while (minSize>startMinMaxAtSize);
  _decimation4x = level;
  // we need at least one 2x decimation step
  do
  {
    DecimationPass &dec = _dec[level];
    w = w>>1;
    h = h>>1;
    maxSize = maxSize>>1;
    minSize = minSize>>1;
    // create the supplementary RT
    // never create less then 1x1
    int effW = max(w,1);
    int effH = max(h,1);
    LogF("Decimation 2x: %d,%d",effW,effH);
    HRESULT err = _engine->CreateTextureRT(effW, effH, 1, rtFormat, D3DPOOL_DEFAULT, dec._rt, dec._rts, true);
    if (FAILED(err))
    {
      _engine->DDError9("Post process Low resolution texture creation failed",err);
      return false;
    }
    dec._w = effW;
    dec._h = effH;
    // proceed to next level
    level++;
  } while (maxSize>1);
  _decimation2x = level;

  HRESULT err = _engine->CreateTextureRT(NBrightQueries, 1, 1, rtFormat, D3DPOOL_DEFAULT, _rtMeasure, _rtsMeasure, true);
  if (FAILED(err))
  {
    _engine->DDError9("Post process Low resolution texture creation failed",err);
    return false;
  }
  
  // stencil/depth buffer no longer needed here - we can disable it
  if (_engine->_depthBufferRT)
  {
    HRESULT err = _engine->CreateDepthStencilSurface(NBrightQueries, 1, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, _rtsMeasureDepth, _rtsMeasure, NULL);
    if (FAILED(err))
    {
      _engine->DDError9("Measure depth buffer creation failed",err);
      return false;
    }
  }
  
  _rtApertureCurrent = 0;
  D3DFORMAT apFormat = _engine->_apertureFormat;
  for (int i=0; i<2; i++)  
  {
    // create the texture
    HRESULT err = _engine->CreateTextureRT(1, 1, 1, apFormat, D3DPOOL_DEFAULT, _rtAperture[i], _rtsAperture[i], true);
    if (FAILED(err))
    {
      _engine->DDError9("Post process aperture texture creation failed",err);
      return false;
    }
    
    // clear the target to avoid uninitialized value
    {
      // save RT
      ComRef<IDirect3DSurface9> rt;
      _engine->_d3DDevice->GetRenderTarget(0,rt.Init());
      // set and clear
      _engine->_d3DDevice->SetRenderTarget(0,_rtsAperture[i]);
      _engine->_d3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,0,0);
      // restore RT
      _engine->_d3DDevice->SetRenderTarget(0,rt);
    }
  }

  {// init the histogram texture
    int histogramSize = 16;
    D3DFORMAT histFormat = _engine->_apertureFormat;
    err = _engine->_d3DDevice->CreateTexture(histogramSize,1,1,0,histFormat,D3DPOOL_DEFAULT, _histogramTholds.Init(),NULL);
    if (FAILED(err))
    {
      _engine->DDError9("Post process histogram sampling texture creation failed",err);
      return false;
    }
  
	  ComRef<IDirect3DTexture9> tmpTexture;

	  // temporary texture created in system pool used data filling
	  if ( FAILED(_engine->_d3DDevice->CreateTexture( histogramSize, 1, /*levels*/ 1, /*usage*/ 0, histFormat, D3DPOOL_SYSTEMMEM, tmpTexture.Init(), NULL )) ) 
			return false;

	  // lock and fill texture by coefficients - convert to target format (float16/float32) as needed
	  D3DLOCKED_RECT lockRect;
	  if ( FAILED(tmpTexture->LockRect(0, &lockRect, 0, 0)) ) { return false; }
	  for (int i=0; i<histogramSize; i++)
	  {
	    float value = HistInit[i];
	    if (_engine->_apertureFormat==D3DFMT_R16F)
	    {
	      D3DXFloat32To16Array(((D3DXFLOAT16 *)lockRect.pBits)+i,&value, 1);
	    }
	    else if (_engine->_apertureFormat==D3DFMT_R32F)
	    {
	      ((float *)lockRect.pBits)[i] = value;
	    }
	    else
	    {
	      Fail("Aperture format %s not supported by queries");
	    }
	  }
	  tmpTexture->UnlockRect(0);

	  // update texture
	  if ( FAILED(_engine->_d3DDevice->UpdateTexture(tmpTexture, _histogramTholds)) ) 
			return false;
  }  

  #if _DEBUG
  {
    HRESULT err = _engine->GetDirect3DDevice()->CreateOffscreenPlainSurface(1, 1, apFormat, D3DPOOL_SYSTEMMEM, _rtsDebugAp.Init(), NULL);
    if (FAILED(err))
    {
      // failed as a plain surface, try to create as a lockable render target
      err = _engine->GetDirect3DDevice()->CreateRenderTarget(1, 1, apFormat, D3DMULTISAMPLE_NONE, 0, TRUE, _rtsDebugAp.Init(), NULL);
      if (FAILED(err))
      {
        _engine->DDError9("Post process Low resolution texture creation failed",err);
        return false;
      }
    }
    err = _engine->GetDirect3DDevice()->CreateOffscreenPlainSurface(1, 1, rtFormat, D3DPOOL_SYSTEMMEM, _rtsDebugDec.Init(), NULL);
    if (FAILED(err))
    {
      err = _engine->GetDirect3DDevice()->CreateRenderTarget(1, 1, rtFormat, D3DMULTISAMPLE_NONE, 0, TRUE, _rtsDebugDec.Init(), NULL);
      if (FAILED(err))
      {
        _engine->DDError9("Post process Low resolution texture creation failed",err);
        return false;
      }
    }
  }
  #endif
  
  
  // stencil/depth buffer no longer needed here - we can disable it
  /*
  if (_engine->_depthBufferRT)
  {
    D3DSURFACE_DESC desc;
    _rtRough[0]->GetLevelDesc(0,&desc);
    int roughWidth = desc.Width;
    int roughHeight = desc.Height;
    if (
      FAILED(
        _engine->CreateDepthStencilSurface(
          roughWidth,roughHeight,D3DFMT_D24S8,
          D3DMULTISAMPLE_NONE,0,TRUE,_rtsRoughDepth,XXX
        )
      )
    )
    {
      LogF("PPGlow: Cannot create depth buffer");
      return false;
    }
  }
  */

#if _HELISIM_RENDER_TO_TEXTURE

  {
    // 1st 4x decimation step (128x128), each R2T is scaled to 128x128
    DecimationPass &dec0 = _decR2T[0];
    dec0._w = dec0._h = R2TMaxResolution / 4;

    HRESULT err = _engine->CreateTextureRT(dec0._w, dec0._h, 1, rtFormat, D3DPOOL_DEFAULT, dec0._rt, dec0._rts, true);
    if (FAILED(err))
    {
      _engine->DDError9("Post process Low resolution texture creation failed", err);
      return false;
    }

    // 2nd 4x decimation step (32x32)
    DecimationPass &dec1 = _decR2T[1];
    dec1._w = dec1._h = dec0._w / 4;
    err = _engine->CreateTextureRT(dec1._w, dec1._h, 1, rtFormat, D3DPOOL_DEFAULT, dec1._rt, dec1._rts, true);
    if (FAILED(err))
    {
      _engine->DDError9("Post process Low resolution texture creation failed", err);
      return false;
    }

    // decimation 2x
    int level = 2;
    int w = _decR2T[1]._w;
    int h = _decR2T[1]._h;
    do 
    {
      DecimationPass &dec = _decR2T[level];
      w = w >> 1;
      h = h >> 1;
      // create the supplementary RT
      // never create less then 1x1
      int effW = max(w,1);
      int effH = max(h,1);
      HRESULT err = _engine->CreateTextureRT(effW, effH, 1, rtFormat, D3DPOOL_DEFAULT, dec._rt, dec._rts, true);
      if (FAILED(err))
      {
        _engine->DDError9("Post process Low resolution texture creation failed",err);
        return false;
      }
      level++;
      dec._w = effW;
      dec._h = effH;
    } while (w > 1);
  }

#endif

  {
    // Create the VB + declaration
    ComRef<IDirect3DVertexBuffer9> vb;
    if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexCustomEdge) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
    {
      LogF("Error: Vertex buffer creation failed.");
      return false;
    }
    void *data;
    vb->Lock(0, 0, &data, 0);
    VertexCustomEdge *v = (VertexCustomEdge *)data;
    v[0].pos = Vector3P(-0.5, -0.5, 0.5); v[0].index = 0;
    v[1].pos = Vector3P(-0.5,  0.5, 0.5); v[1].index = 1;
    v[2].pos = Vector3P( 0.5, -0.5, 0.5); v[2].index = 2;
    v[3].pos = Vector3P( 0.5,  0.5, 0.5); v[3].index = 3;
    vb->Unlock();

    _vbCustomEdge = VertexStaticData9::BufferFuture(vb);

    // Create the VD of the blur vertex
    const D3DVERTEXELEMENT9 vertexDeclCustomEdge[] =
    {
      {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
      {0, 12, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
      D3DDECL_END()
    };
    if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDeclCustomEdge, _vdCustomEdge.Init())))
    {
      LogF("Error: Vertex declaration creation failed.");
      return false;
    }
  }

  // Create the shaders
  if (!CompileVertexShader(vs,"VSPostProcessCustomEdge4T",_vsCustomEdge))
  {
    return false;
  }
  
  if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleLuminance",_psLuminance))
  {
    return false;
  }
  if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleAvgLuminance",_psAvgLuminance))
  {
    return false;
  }
  if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleMaxAvgMinLuminance",_psMaxAvgMinLuminance))
  {
    return false;
  }
  ComRef<IDirect3DPixelShader9> ppal;
  if (!CompilePixelShader(hlsl,"PSPostProcessAssumedLuminance",ppal,true))
  {
    return false;
  }
  ComRef<IDirect3DPixelShader9> ppalDirect;
  if (!CompilePixelShader(hlsl,"PSPostProcessAssumedLuminanceDirect",ppalDirect,false))
  {
    return false;
  }

  if (_engine->_apertureFormat==D3DFMT_A8R8G8B8)
  {
    /**
    if there is no float render target support,
    we cannot calculate change on the GPU
    */
    _psAssumedLuminance = ppalDirect;
  }
  else
  {
    _psAssumedLuminance = ppal;
  }

  if (!CompilePixelShader(hlsl,"PSPostProcessQuery",_psQuery))
  {
    return false;
  }


  if (!CompilePixelShader(hlsl,"PSPostProcessGlow",_psGlow))
  {
    return false;
  };

  if (!CompilePixelShader(hlsl,"PSPostProcessGlowNight",_psGlowNight))
  {
    return false;
  };
  if (!CompilePixelShader(hlsl,"PSPostProcessNVG",_psGlowNVG))
  {
    return false;
  };

  return true;
}


//-----------------------------------------------------------------------------
int EngineDD9::PPGlow::SetParams(const float *pars, int nPars)
{
  Assert(nPars==3);
  _deltaT = pars[0];
  _eyeAdaptMin = pars[1];
  _eyeAdaptMax = pars[2];

  return 0;
}

#if _DEBUG
//-----------------------------------------------------------------------------
Color EngineDD9::PPGlow::GetContent(IDirect3DSurface9 *surf, IDirect3DSurface9 *temp, D3DFORMAT format)
{
  _engine->GetDirect3DDevice()->GetRenderTargetData(surf,temp);

  D3DLOCKED_RECT locked;
  temp->LockRect(&locked,NULL,D3DLOCK_READONLY);
  // assume ARGB8888 format
  Color color;
  switch (format)
  {
    case D3DFMT_A8R8G8B8:
      {
        PackedColor c = PackedColor(*(DWORD *)locked.pBits);
        color = Color(c);
      }
      break;
    case D3DFMT_R32F:
      {
        const float *c = (const float *)locked.pBits;
        color = Color(c[0],0,0,1);
      }
      break;
    case D3DFMT_G32R32F:
      {
        const float *c = (const float *)locked.pBits;
        color = Color(c[0],c[1],0,1);
      }
      break;
    case D3DFMT_A32B32G32R32F:
      {
        const float *c = (const float *)locked.pBits;
        color = Color(c[0],c[1],c[2],c[3]);
      }
      break;
    case D3DFMT_A16B16G16R16F:
      {
        const Float16b<10> *c = (const Float16b<10> *)locked.pBits;
        color = Color(c[0],c[1],c[2],c[3]);
      }
      break;
    case D3DFMT_G16R16F:
      {
        const Float16b<10> *c = (const Float16b<10> *)locked.pBits;
        color = Color(c[0],c[1],0,1);
      }
      break;
    case D3DFMT_R16F:
      {
        const Float16b<10> *c = (const Float16b<10> *)locked.pBits;
        color = Color(c[0],0,0,1);
      }
      break;
    case D3DFMT_L16:
      {
        const unsigned short *c = (const unsigned short *)locked.pBits;
        float l = c[0]*(1.0f/0xffff);
        color = Color(l,l,l,1);
      }
      break;
    case D3DFMT_G16R16:
      {
        const unsigned short *c = (const unsigned short *)locked.pBits;
        float r = c[0]*(1.0f/0xffff);
        float g = c[1]*(1.0f/0xffff);
        color = Color(r,g,0,1);
      }
      break;
    default:
      color = HBlack;
      break;
  }
  temp->UnlockRect();
  return color;
}

//-----------------------------------------------------------------------------
static Color DecodeApertureChangeF(Color color)
{
  return Color(
    DecodeApertureChange(color.R()),
    DecodeApertureChange(color.G()),
    DecodeApertureChange(color.B()),
    DecodeApertureChange(color.A())
  );
}

//-----------------------------------------------------------------------------
Color EngineDD9::PPGlow::GetContentAp(IDirect3DSurface9 *surf)
{
  // aperture change is stored as logarithm
  Color raw = GetContent(surf,_rtsDebugAp,_engine->_apertureFormat);
  return DecodeApertureChangeF(raw);
}

//-----------------------------------------------------------------------------
Color EngineDD9::PPGlow::GetContentDec(IDirect3DSurface9 *surf)
{
  return GetContent(surf,_rtsDebugDec,D3DFMT_A8R8G8B8);
}

#endif

// note: MS HDR uses sensitivity as follows:
//(0.2125, 0.7154, 0.0721);
//(0.2125, 0.7154, 0.0721);
#define R_EYE_DARK 0.05f
#define G_EYE_DARK 0.55f
#define B_EYE_DARK 0.40f
static const Color DaySens(R_EYE,G_EYE,B_EYE);

/*!
\patch 5129 Date 2/20/2007 by Ondra
- Improved: Improved night vision goggles rendering.
*/
static const Color NVGSens(0.6,0.2,0.2);
static const Color NightSens(R_EYE_DARK,G_EYE_DARK,B_EYE_DARK);

#define REINHARD_PAR0 0.9
#define REINHARD_PAR1 0.000

#define FILMIC_PAR0 0.1530
#define FILMIC_PAR1 0.3570
#define FILMIC_PAR2 0.2310
#define FILMIC_PAR3 0.1573
#define FILMIC_PAR4 0.011
#define FILMIC_PAR5 3.750
#define FILMIC_PAR6 6.0
#define FILMIC_PAR7 4.0

#if _HELISIM_RENDER_TO_TEXTURE

//-----------------------------------------------------------------------------
void EngineDD9::PPGlow::DoDirect(const RECT *rect, int rtIdx)
{
  static const float zero9[9*4] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };

  // glow is typically a last effect, but in some cases some other may follow
  PROFILE_SCOPE_GRF_EX(ppGlw, ppr2t, SCOPE_COLOR_YELLOW);

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Glow::DoDirect: ");

  int width = rect->right - rect->left;
  int height = rect->bottom - rect->top;

  ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

  ComRef<IDirect3DSurface9> _renderTargetSurfaceSaved;
  ComRef<IDirect3DSurface9> _renderTargetDepthBufferSurfaceSaved;

  // Save current RTs
  device->GetRenderTarget(0, _renderTargetSurfaceSaved.Init());
  device->GetDepthStencilSurface(_renderTargetDepthBufferSurfaceSaved.Init());

  // We will need a source as a texture
  ComRef<IDirect3DTexture9> rtCopy = _engine->CreateR2TRenderTargetCopyAsTexture(device, rect, D3DTEXF_LINEAR);

  BOOL rtIsSRGB = _engine->_caps._rtIsSRGB && _engine->_caps._canReadSRGB;

  // Set the vertex shader + stream source
  device->SetStreamSource(0, 
#if _ARMA3_VERTEX_BUFFER_EX
    _vbCustomEdge.GetResult().GetBuffer(), 
#else
    _vbCustomEdge.GetResult(), 
#endif
    0, sizeof(VertexCustomEdge));
  device->SetVertexDeclaration(_vdCustomEdge);
  device->SetVertexShader(_vsCustomEdge);

  R2THDRPars &hdrPars = *(_engine->_r2tTargets[rtIdx]._hdrPars);
  {
    float hdrMultiply = _engine->GetPostHDRBrightness() / _engine->GetHDRFactor();
    float spc7[] = {hdrMultiply, hdrMultiply, hdrMultiply, hdrMultiply};

    device->SetVertexShaderConstantF(PSC_HDRMultiply, &spc7[0], 1);

    // decide if decimated surfaces are SRGB or not
    const bool decimatedSRGB = _engine->_downsampleFormat==D3DFMT_A8B8G8R8;

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_SRGBWRITEENABLE, decimatedSRGB);
    device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    // no alpha or fog done here
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    // by default assume bi-linear filtering
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    // tri-linear filtering never done 
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    device->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    device->SetSamplerState(3, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    // clamping all stages
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetSamplerState(3, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(3, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // perform initial luminance calculation + 4x decimation
    {
      PROFILE_SCOPE_GRF_EX(ppGLu, ppr2t, SCOPE_COLOR_YELLOW);

      EdgeParams pars;
      pars.SetupBilinear(_decR2T[0]._w, _decR2T[0]._h, width, height);
      device->SetVertexShaderConstantF(0, pars.u[0], 9);

      // Set the pixel shader
      device->SetPixelShader(_psLuminance);

      // Set the render target
      device->SetRenderTarget(0, _decR2T[0]._rts);
      device->SetDepthStencilSurface(_engine->_depthBuffer);
      device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

      // set the source as 4 textures    
      device->SetTexture(0, rtCopy);
      device->SetTexture(1, rtCopy);
      device->SetTexture(2, rtCopy);
      device->SetTexture(3, rtCopy);

      device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
      device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
      device->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
      device->SetSamplerState(3, D3DSAMP_SRGBTEXTURE, rtIsSRGB);

      device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }

    device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
    device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
    device->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
    device->SetSamplerState(3, D3DSAMP_SRGBTEXTURE, decimatedSRGB);

    {
      PROFILE_SCOPE_GRF_EX(ppGAv, ppr2t, SCOPE_COLOR_YELLOW);

      // perform 4x decimation + average calculation
      EdgeParams pars;
      pars.SetupBilinear(_decR2T[1]._w, _decR2T[1]._h, _decR2T[0]._w, _decR2T[0]._h);
      device->SetVertexShaderConstantF(0, pars.u[0], 9);

      // Set the pixel shader
      device->SetPixelShader(_psAvgLuminance);

      // Set the render target
      device->SetRenderTarget(0, _decR2T[1]._rts);
      device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

      IDirect3DTexture9 *tex = _decR2T[0]._rt;

      // set the source as 4 textures    
      device->SetTexture(0, tex);
      device->SetTexture(1, tex);
      device->SetTexture(2, tex);
      device->SetTexture(3, tex);

      device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }

    {
      // perform 2x decimation + min/max/avg calculation
      PROFILE_SCOPE_GRF_EX(ppGMM, ppr2t, SCOPE_COLOR_YELLOW);

      // note: we know other temp setting is done, which will return original values once done
      for (int s=0; s<4; s++) 
      {
        device->SetSamplerState(s, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        device->SetSamplerState(s, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
      }

      // set pixel shader
      device->SetPixelShader(_psMaxAvgMinLuminance);

      // 128, 32, 16, 8, 4, 2, 1
      for (int i = 2; i < 7; i++)
      {
        EdgeParams pars;
        pars.SetupBilinear(_decR2T[i]._w, _decR2T[i]._h, _decR2T[i-1]._w, _decR2T[i-1]._h);
        device->SetVertexShaderConstantF(0, pars.u[0], 9);

        device->SetRenderTarget(0, _decR2T[i]._rts);
        device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

        IDirect3DTexture9 *tex = _decR2T[i-1]._rt;

        // set the source as 4 textures   
        device->SetTexture(0, tex);
        device->SetTexture(1, tex);
        device->SetTexture(2, tex);
        device->SetTexture(3, tex);

        device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
      }

      // perform some calculations with 1x1 surfaces
      // this is to adjust aperture
      {
        device->SetPixelShader(_psAssumedLuminance);

        float eyeAccomLastToThis =  hdrPars._accomFactorPrev / hdrPars._accomFactor;
        // this controls overall scene brightness
        // we need to know where the user selected midpoint is
#if _ARMA3_HDR_IMPROVEMENTS
        float desiredLuminance = _engine->_nightVisionThis ? 0.781f : (_engine->GetPreHDRBrightness() * _engine->GetHDRFactor() * _engine->GetDesiredLuminanceCoef());
#else
        float desiredLuminance = _engine->_nightVisionThis ? 0.781f : (_engine->GetPreHDRBrightness() * _engine->GetHDRFactor());
#endif
        float invAccom = 1.0f / hdrPars._accomFactor;

        // adaptation time depends on how we are adapting
        // if we are reacting with iris, the adaptations is very fast
        // if we are reacting with retina, it is much slower
        // even when retina is adapted, if light level is raised, the iris will react
        // this allows quite quick re-adaptation for higher light levels
        // we expect something like:
        const float irisUsedToDark = 1; // time to make change 2x
        const float irisUsedToLight = 0.5; // time to make change 0.5x

        const float retinaUsedToDark = 20; // time to make change 2x
        const float retinaUsedToLight = 1.0; // time to make change 0.5x

        // retina adaptation dynamic range is probably much smaller (100x?)
        // when retina adaptation is important:
        // 1) iris fully open
        // 2) once rods get adapted, they go down slowly
        // we do not model this, we assume under certain level retina acts
        // and above it iris acts
        // assume retina starts reacting once iris adaptation is 1.0
        const float crossBeg = 1.0f;
        const float crossEnd = 4.0f;

        float retinaFactor = InterpolativC(hdrPars._accomFactorPrev, crossBeg, crossEnd, 0, 1);

        // time to make change 2x
        const float getUsedToDark = retinaFactor*retinaUsedToDark+(1-retinaFactor)*irisUsedToDark;
        // time to make change 0.5x
        const float getUsedToLight = retinaFactor*retinaUsedToLight+(1-retinaFactor)*irisUsedToLight;

        const float log2 = 0.69314718056f;

        float minRatio = exp(-hdrPars._deltaTR2T*(log2/getUsedToLight));
        float maxRatio = exp(+hdrPars._deltaTR2T*(log2/getUsedToDark));

        // make sure the number still can be represented with float16
        saturateMin(minRatio,0.999f);
        saturateMax(maxRatio,1.001f);

        // we need the value to be representable in float16 (m10e6) format
        float minAdapt = 1, maxAdapt = 1;

        if (!hdrPars._accomFactorFixed)
        {
          minAdapt = hdrPars._eyeAdaptMinR2T*invAccom;
          maxAdapt = hdrPars._eyeAdaptMaxR2T*invAccom;
        }

        const float pars[8] = {
          eyeAccomLastToThis, hdrPars._deltaTR2T, desiredLuminance, 0, minAdapt, maxAdapt, minRatio, maxRatio
        };

        device->SetPixelShaderConstantF(8, pars, 2);
        device->SetVertexShaderConstantF(0, zero9, 9);

        int oldAperture = hdrPars._rtApertureCurrentR2T;
        // switch to next aperture texture
        hdrPars._rtApertureCurrentR2T = hdrPars._rtApertureCurrentR2T == 0;
        // Set the render target
        device->SetRenderTarget(0, hdrPars._rtsApertureR2T[hdrPars._rtApertureCurrentR2T]);
        device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

        // 1x1 maxAvgMin as source
        device->SetTexture(0, _decR2T[6]._rt);
        // old aperture value as another source
        device->SetTexture(1, hdrPars._rtApertureR2T[oldAperture]);
        device->SetTexture(2, NULL);
        device->SetTexture(3, NULL);

        // render target is always linear
        device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

        // old aperture is always linear
        // source down-sampled scene is SRGB
        device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
        device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, FALSE);

        device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
      }

      // Measure the brightness of the image
      {
        PROFILE_SCOPE_GRF_EX(ppGQy, ppr2t, SCOPE_COLOR_GREEN);

        static const float baseU[4] = {0, 0, 1, 1};
        static const float baseV[4] = {1, 0, 1, 0};

        const float tgtOffsetU = 0;
        const float tgtOffsetV = 0;
        const float offsetU = 0.5f / NBrightQueries;
        const float offsetV = 0;

        float U[4][4];
        float V[4][4];
        for (int vertex = 0; vertex < 4; vertex++)
        {
          // Texture 0 - source image - does not matter, not used
          U[vertex][0] = baseU[vertex] + offsetU;
          V[vertex][0] = baseV[vertex] + offsetV;
          // Texture 1 - histohram thresholds
          U[vertex][1] = baseU[vertex] + offsetU;
          V[vertex][1] = baseV[vertex] + offsetV;
          // Texture 2
          U[vertex][2] = 0;
          V[vertex][2] = 0;
          // Texture 3
          U[vertex][3] = 0;
          V[vertex][3] = 0;
        }

        const float posOffset[4] = { tgtOffsetU, tgtOffsetV, 0, 0 };
        device->SetVertexShaderConstantF(0, U[0], 4);
        device->SetVertexShaderConstantF(4, V[0], 4);
        device->SetVertexShaderConstantF(8, posOffset, 1);

        float histPars[NBrightQueries+1];

        // we want half of the samples to be under 1 and half above 1
        // sample position relative to the expected value
        // last one is always 1 and it is always ignored
        for (int i = 0; i < NBrightQueries; i++)
        {
          histPars[i] = HistInit[i];
        }

        // always ignored
        histPars[NBrightQueries] = 1;

        device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

        device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
        device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

        device->SetRenderTarget(0, _rtsMeasure);
        device->SetDepthStencilSurface(_rtsMeasureDepth);
        device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

        device->SetTexture(0, hdrPars._rtApertureR2T[hdrPars._rtApertureCurrentR2T]);

        device->SetPixelShader(_psQuery);


        float values[NBrightQueries];
        for (int b = 0; b < NBrightQueries; b++)
        {
          values[b] = histPars[b] * hdrPars._accomFactor;
        }
        hdrPars._querySinglePassR2T->Issue(device, values, hdrPars._accomFactor, _histogramTholds);

        // Build the histogram and check the intensity of the image
        {
          // Each level includes all the following levels
          struct HistogramItem
          {
            float minAlpha;
            int pixels;
          } histogram[NBrightQueries+2];

          bool valid = true;
          int numberOfPixelsDrawn[NBrightQueries];
          float value[NBrightQueries];
          float masterValue = hdrPars._querySinglePassR2T->GetNumberOfPixelsDrawn(numberOfPixelsDrawn,value);
          (void)masterValue;

          for (int b=0; b<NBrightQueries; b++)
          {
            histogram[b+1].minAlpha = value[b];
            histogram[b + 1].pixels = numberOfPixelsDrawn[b];
            if (numberOfPixelsDrawn[b] < 0) valid = false;
          }

          if (valid)
          {
            // as we test 1x1 surface, "histogram" is actually one point only
            float totalAlpha = 0;
            // all pixels are over 0 limit
            histogram[0].minAlpha = 0;
            histogram[0].pixels = 1 * 1;
            // no pixels are over 1 limit
            histogram[NBrightQueries + 1].minAlpha = histogram[NBrightQueries].minAlpha;
            histogram[NBrightQueries + 1].pixels = 0;
            float totalError = 0;
            // verify we got expected results, i.e. line of 1s followed by line of 0s
            int edge = -1;
            for (int b = 0; b < NBrightQueries + 1; b++)
            {
              float alphaMin = histogram[b].minAlpha;
              float alphaMax = histogram[b+1].minAlpha;
              // exclude brighter levels
              int thisCount = histogram[b].pixels - histogram[b + 1].pixels;
              if (thisCount>0)
              {
                // verify the edge is 1 pixel only
                Assert(thisCount==1);
                // verify there is only one edge
                Assert(edge<0);
                edge = b;
              }
              float sumAlpha = thisCount * (alphaMin + alphaMax) * 0.5f;
              float error = thisCount * (alphaMax - alphaMin) * 0.5f;
              totalAlpha += sumAlpha;
              totalError += error;
            }

            // if the resulting value is negative, something went wrong
            DoAssert(totalAlpha>=0);

            // we cannot use masterValue to detect changes, as due to latency this could cause
            // two values alternating and never reaching a stable state
            // if the value did not change significantly from the master value, use the master value

            // if the value did not change significantly, do not change it, as changing aperture estimate often can lead to oscillation
            //const float changeThold = 0.04f;
            static float changeThold = 0.1f;
            if (hdrPars._lastKnownAvgIntensity <= 0 ||
              fabs(totalAlpha - hdrPars._lastKnownAvgIntensity) / totalAlpha>changeThold)
            {
              hdrPars._lastKnownAvgIntensity = totalAlpha;
            }
          }
        }
      }

      // Final step
      {
        static const float baseU[4] = {0, 0, 1, 1};
        static const float baseV[4] = {1, 0, 1, 0};

        // point sampling requires 0.5 offset to avoid numerical inaccuracy
        // previous version, well tuned
        const float tgtOffsetU = 0;
        const float tgtOffsetV = 0;
        const float srcOffsetU = 0.5f / width;
        const float srcOffsetV = 0.5f / height;

        const float glowOffsetU = 0.0f;
        const float glowOffsetV = 0.0f;
        float U[4][4];
        float V[4][4];
        for (int vertex = 0; vertex < 4; vertex++)
        {
          // Texture 0 - source image
          U[vertex][0] = baseU[vertex] + srcOffsetU;
          V[vertex][0] = baseV[vertex] + srcOffsetV;
          // Texture 1 - glow buffer
          U[vertex][1] = baseU[vertex] + glowOffsetU;
          V[vertex][1] = baseV[vertex] + glowOffsetV;
          // Texture 2
          U[vertex][2] = 0;
          V[vertex][2] = 0;
          // Texture 3
          U[vertex][3] = 0;
          V[vertex][3] = 0;
        }

        const float posOffset[4] = { tgtOffsetU,tgtOffsetV, 0, 0 };
        device->SetVertexShaderConstantF(0, U[0], 4);
        device->SetVertexShaderConstantF(4, V[0], 4);
        device->SetVertexShaderConstantF(8, posOffset, 1);

        device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
        device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

        device->SetRenderTarget(0, _engine->_r2tRT._renderTargetSurfaceS);
        device->SetDepthStencilSurface(_engine->GetFSAA() > 0 ? _engine->_depthBufferRT : _engine->_depthBuffer);
        device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

        device->SetTexture(0, rtCopy); // Set the original render target as texture 0
        device->SetTexture(1, NULL); // glow texture here
        device->SetTexture(2, hdrPars._rtApertureR2T[hdrPars._rtApertureCurrentR2T]); // aperture control texture
        device->SetTexture(3, _decR2T[7]._rt); // 1x1 maxAvgMin

        device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
        device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
        device->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, FALSE);
        device->SetSamplerState(3, D3DSAMP_SRGBTEXTURE, decimatedSRGB);

				{
          D3DVIEWPORT9 viewData;
          memset(&viewData,0,sizeof(viewData));
          viewData.X = 0;
          viewData.Y = 0;
          viewData.Width  = width;
          viewData.Height = height;
          viewData.MinZ = 0.0f;
          viewData.MaxZ = 1.0f;
          device->SetViewport(&viewData);
        }

        // compute r,g,b eye sensitivity
        if (hdrPars._nightEye > 1e-3f)
        {
          // Set the pixel shader
          device->SetPixelShader(_psGlowNight);

          // Value designed for night blue shift
          static float coneAdjust = 60.0f;
          float lumScale = coneAdjust / hdrPars._accomFactor;

          // Get cones
          static float coneFull = 0.5f;
          static float coneNo = 1.1f;
          float cones = InterpolativC(hdrPars._accomFactor,coneFull,coneNo,1,-0.2);

          // Get Eye sens
          Color eyeSens = EyeSens(cones);

          // Set the temporary shader constant
          D3DXVECTOR4  spc6(eyeSens.R(), eyeSens.G(), eyeSens.B(), 1);
          device->SetPixelShaderConstantF(26, spc6, 1);

          D3DXVECTOR4 spc8(lumScale, cones, 0, 0);
          device->SetPixelShaderConstantF(28, spc8, 1);

          // Drawing
          device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        }
        else
        {
          // Set the pixel shader
          ColorVal sens = hdrPars._nightVisionR2T ? NVGSens : DaySens;
          // Set the temporary shader constant
          device->SetPixelShader(hdrPars._nightVisionR2T ? _psGlowNVG : _psGlow);

          // Set the temporary shader constant
          // Drawing
          D3DXVECTOR4 spc6(sens.R(), sens.G(), sens.B(), 1);
          device->SetPixelShaderConstantF(26, spc6, 1);
          device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        }

      }
    }
  }

  // remember properties of this frame
  hdrPars._accomFactorPrev = hdrPars._accomFactor;

  // Restore targets
  device->SetRenderTarget(0, _renderTargetSurfaceSaved);
  device->SetDepthStencilSurface(_renderTargetDepthBufferSurfaceSaved);

  _renderTargetSurfaceSaved.Free();
  _renderTargetDepthBufferSurfaceSaved.Free();

  device->SetRenderState(D3DRS_ZENABLE, TRUE);
  device->SetTexture(0, NULL);
  device->SetTexture(1, NULL);
  device->SetTexture(2, NULL);
  device->SetTexture(3, NULL);

	GPU_END_PROFILE
}

#endif

static float ZeroToneMappingPars[8] = {0};
static float ReinhardToneMappingPars[8] = {REINHARD_PAR0*REINHARD_PAR0, -REINHARD_PAR1, 0, 0, 0, 0, 0, 0};
static float FilmicToneMappingPars[8] = {FILMIC_PAR0, FILMIC_PAR1, FILMIC_PAR2, FILMIC_PAR3, FILMIC_PAR4, FILMIC_PAR5, FILMIC_PAR6, FILMIC_PAR7};

static int ToneMappingMethodIdx = 1;
static float *ToneMappingPars = ReinhardToneMappingPars;

//-----------------------------------------------------------------------------
static void SetReinhardToneMappingToDefault()
{
  ReinhardToneMappingPars[0] = REINHARD_PAR0*REINHARD_PAR0;
  ReinhardToneMappingPars[1] = REINHARD_PAR1;
}

//-----------------------------------------------------------------------------
static void SetFilmicTonemMappingToDefault()
{
  FilmicToneMappingPars[0] = FILMIC_PAR0;
  FilmicToneMappingPars[1] = FILMIC_PAR1;
  FilmicToneMappingPars[2] = FILMIC_PAR2;
  FilmicToneMappingPars[3] = FILMIC_PAR3;
  FilmicToneMappingPars[4] = FILMIC_PAR4;
  FilmicToneMappingPars[5] = FILMIC_PAR5;
  FilmicToneMappingPars[6] = FILMIC_PAR6;
  FilmicToneMappingPars[7] = FILMIC_PAR7;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGlow::SetDefaultToneMapping()
{
  SetReinhardToneMappingToDefault();
  SetFilmicTonemMappingToDefault();

  ToneMappingMethodIdx = 1;
  ToneMappingPars = ReinhardToneMappingPars;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPGlow::Do(bool isLast)
{
  static const float zero9[9*4] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  
  // glow is typically a last effect, but in some cases some other may follow
  PROFILE_SCOPE_GRF_EX(ppGlw,pp,SCOPE_COLOR_YELLOW);

  const int cb = -1;
  
  // Prepare drawing
  Prepare();
	
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Glow: ");

  IDirect3DTexture9 *origRt = _engine->_currRenderTarget;
  IDirect3DSurface9 *origRts = _engine->_currRenderTargetSurface;

  // We will need a source as a texture
  IDirect3DTexture9 *rt;
  
  // if possible, use previous render target directly for texturing
  if (isLast && _engine->_currRenderTarget)
  {
    rt = _engine->GetCurrRenderTargetAsTexture();
  }
  else
  {
    rt = _engine->CreateRenderTargetCopyAsTexture();
  }
  
  // depending on render target format it may or may not be SRGB
  Assert(_engine->_linearSpace);
  BOOL rtIsSRGB = _engine->_caps._rtIsSRGB && _engine->_caps._canReadSRGB;
  // Set the vertex shader + stream source
  _engine->SetStreamSource(cb, _vbCustomEdge, sizeof(VertexCustomEdge), 4);
  _engine->SetVertexDeclaration(cb, _vdCustomEdge);
  _engine->SetVertexShader(cb, _vsCustomEdge);


  {
    float hdrMultiply = _engine->GetPostHDRBrightness()/_engine->GetHDRFactor();
#if _ENABLE_CHEATS && _ENABLE_REPORT && _ENABLE_PERFLOG
    if (CHECK_DIAG(DEHDR) && CheckMainThread())
    {
      DIAG_MESSAGE(100,Format("Post HDR multiply %.2f",hdrMultiply));
    }
#endif
    SetPixelShaderConstantTemp spc7(PSC_HDRMultiply, D3DXVECTOR4(hdrMultiply,hdrMultiply,hdrMultiply,hdrMultiply));
    
    PushVertexShaderConstant pushC0(0);
    PushVertexShaderConstant pushC1(1);
    PushVertexShaderConstant pushC2(2);
    PushVertexShaderConstant pushC3(3);
    PushVertexShaderConstant pushC4(4);
    PushVertexShaderConstant pushC5(5);
    PushVertexShaderConstant pushC6(6);
    PushVertexShaderConstant pushC7(7);
    PushVertexShaderConstant pushC8(8);
    
    // decide if decimated surfaces are SRGB or not
    const bool decimatedSRGB = _engine->_downsampleFormat==D3DFMT_A8B8G8R8;
    
    // Decimate the original into rough texture and measure aperture
    {
      // Set temporary states
      SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, decimatedSRGB);
      SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
      // no alpha or fog done here
      SetRenderStateTemp rsALPHATESTENABLE(D3DRS_ALPHATESTENABLE, FALSE);
      SetRenderStateTemp rsALPHABLENDENABLE(D3DRS_ALPHABLENDENABLE, FALSE);
      // by default assume bi-linear filtering
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss3MINFILTER(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss3MAGFILTER(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      // tri-linear filtering never done 
      SetSamplerStateTemp ss0MIPFILTER(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss1MIPFILTER(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss2MIPFILTER(2, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss3MIPFILTER(3, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
      // clamping all stages
      SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss2ADDRESSU(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss2ADDRESSV(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss3ADDRESSU(3, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss3ADDRESSV(3, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);


      
      #if 0 //_ENABLE_CHEATS
        D3DSURFACE_DESC desc;
        rt->GetLevelDesc(0,&desc);
        DoAssert(desc.Width==_engine->Width());
        DoAssert(desc.Height==_engine->Height());
      #endif

      {
        PROFILE_SCOPE_GRF_EX(ppGLu,pp,SCOPE_COLOR_YELLOW);
    
        // perform initial luminance calculation + 4x decimation
        EdgeParams pars;
#if _ARMA3_HDR_IMPROVEMENTS
        float texCoef = 1.0f - _engine->GetHDRLuminanceRectCoef();
        saturate(texCoef, 0.0f, 1.0f);
        texCoef *= 0.5f;

        float minTexCoord = 0.0f + texCoef;
        float maxTexCoord = 1.0f - texCoef;
        pars.SetupBilinear(_dec[0]._w,_dec[0]._h,_engine->Width(),_engine->Height(), minTexCoord, maxTexCoord);
#else
        pars.SetupBilinear(_dec[0]._w,_dec[0]._h,_engine->Width(),_engine->Height());
#endif
        _engine->SetVertexShaderConstantF(cb, 0, pars.u[0], 9);

        // Set the pixel shader
        _engine->SetPixelShader(cb, _psLuminance);

        // Set the render target
        _engine->SetRenderTarget(_dec[0]._rts, _dec[0]._rt, NULL);

        // set the source as 4 textures    
        SetTextureTemp tTex0(0, rt);
        SetTextureTemp tTex1(1, rt);
        SetTextureTemp tTex2(2, rt);
        SetTextureTemp tTex3(3, rt);

        SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
        SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
        SetSamplerStateTemp tempSRGB2(2, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
        SetSamplerStateTemp tempSRGB3(3, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
        
				GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Luminance + decimation: ");
					_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
				GPU_END_PROFILE

        _engine->ResolveRenderTarget();
      }
    
      // source is always linear space since here
      SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
      SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
      SetSamplerStateTemp tempSRGB2(2, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
      SetSamplerStateTemp tempSRGB3(3, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
      
			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Decimation : ");
      { // perform 4x decimation + average calculation
        PROFILE_SCOPE_GRF_EX(ppGAv,pp,SCOPE_COLOR_YELLOW);
        for (int i=1; i<_decimation4x; i++)
        {
          // perform initial luminance calculation + 4x decimation
          EdgeParams pars;
          pars.SetupBilinear(_dec[i]._w,_dec[i]._h,_dec[i-1]._w,_dec[i-1]._h);
          
          _engine->SetVertexShaderConstantF(cb, 0, pars.u[0], 9);

          // Set the pixel shader
          _engine->SetPixelShader(cb, _psAvgLuminance);

          // Set the render target
          _engine->SetRenderTarget(_dec[i]._rts, _dec[i]._rt, NULL);

          IDirect3DTexture9 *tex = _dec[i-1]._rt;
          // set the source as 4 textures    
          SetTextureTemp tTex0(0, tex);
          SetTextureTemp tTex1(1, tex);
          SetTextureTemp tTex2(2, tex);
          SetTextureTemp tTex3(3, tex);

          _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
          _engine->ResolveRenderTarget();
        }
      }
			GPU_END_PROFILE


      { // perform 2x decimation + min/max/avg calculation
        PROFILE_SCOPE_GRF_EX(ppGMM,pp,SCOPE_COLOR_YELLOW);
				GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Final decimation: ");

        // note: we know other temp setting is done, which will return original values once done
        for (int s=0; s<4; s++) 
        {
          _engine->D3DSetSamplerState(cb, s, D3DSAMP_MINFILTER, D3DTEXF_POINT);
          _engine->D3DSetSamplerState(cb, s, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        }
        
        // Set the pixel shader
        _engine->SetPixelShader(cb, _psMaxAvgMinLuminance);
        for (int i=_decimation4x; i<_decimation2x; i++)
        {
          EdgeParams pars;
          pars.SetupBilinear(_dec[i]._w,_dec[i]._h,_dec[i-1]._w,_dec[i-1]._h);
          
          _engine->SetVertexShaderConstantF(cb, 0, pars.u[0], 9);

          // Set the render target
          _engine->SetRenderTarget(_dec[i]._rts, _dec[i]._rt, NULL);

          IDirect3DTexture9 *tex = _dec[i-1]._rt;
          // set the source as 4 textures    
          SetTextureTemp tTex0(0, tex);
          SetTextureTemp tTex1(1, tex);
          SetTextureTemp tTex2(2, tex);
          SetTextureTemp tTex3(3, tex);
          
          _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
          _engine->ResolveRenderTarget();
        }
				GPU_END_PROFILE

				GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  HDR calculation: ");
        // perform some calculations with 1x1 surfaces
        // this is to adjust aperture
        {
          _engine->SetPixelShader(cb, _psAssumedLuminance);
          
          Assert(_engine->_accomFactorThis>0);
          // values in last frame were converted to absolute by multiplying by:
          // _engine->_accomFactorPrev * lastFrameAp
          // values in this frame are converted to absolute by multiplying by:
          //  _engine->_accomFactorThis * thisFrameAp
          // absAp = lastFrameAp * _engine->_accomFactorPrev
          // absAp = thisFrameAp * _engine->_accomFactorThis

          // lastFrameAp * _engine->_accomFactorPrev/_engine->_accomFactorThis = thisFrameAp
          
          
          float eyeAccomLastToThis = _engine->_accomFactorPrev/_engine->_accomFactorThis;
          // this controls overall scene brightness
          // we need to know where the user selected midpoint is
#if _ARMA3_HDR_IMPROVEMENTS
          float desiredLuminance = _engine->_nightVisionThis ? 0.781f : (_engine->GetPreHDRBrightness() * _engine->GetHDRFactor() * _engine->GetDesiredLuminanceCoef());
#else
          float desiredLuminance = _engine->_nightVisionThis ? 0.781f : (_engine->GetPreHDRBrightness() * _engine->GetHDRFactor());
#endif
          float invAccom=1/_engine->_accomFactorThis;

          // adaptation time depends on how we are adapting
          // if we are reacting with iris, the adaptations is very fast
          // if we are reacting with retina, it is much slower
          // even when retina is adapted, if light level is raised, the iris will react
          // this allows quite quick re-adaptation for higher light levels
          // we expect something like:
          const float irisUsedToDark = 1; // time to make change 2x
          const float irisUsedToLight = 0.5; // time to make change 0.5x
          
          const float retinaUsedToDark = 20; // time to make change 2x
          const float retinaUsedToLight = 1.0; // time to make change 0.5x
          
          // retina adaptation dynamic range is probably much smaller (100x?)
          // when retina adaptation is important:
          // 1) iris fully open
          // 2) once rods get adapted, they go down slowly
          // we do not model this, we assume under certain level retina acts
          // and above it iris acts
          // assume retina starts reacting once iris adaptation is 1.0
          const float crossBeg = 1.0f;
          const float crossEnd = 4.0f;
          
          float retinaFactor = InterpolativC(_engine->_accomFactorPrev,crossBeg,crossEnd,0,1);
          
          // time to make change 2x
          const float getUsedToDark = retinaFactor*retinaUsedToDark+(1-retinaFactor)*irisUsedToDark;
          // time to make change 0.5x
          const float getUsedToLight = retinaFactor*retinaUsedToLight+(1-retinaFactor)*irisUsedToLight;
          
          const float log2 = 0.69314718056f;
          
          
          float minRatio = exp(-_deltaT*(log2/getUsedToLight));
          float maxRatio = exp(+_deltaT*(log2/getUsedToDark));

          // make sure the number still can be represented with float16
          saturateMin(minRatio,0.999f);
          saturateMax(maxRatio,1.001f);
          
          // we need the value to be representable in float16 (m10e6) format
          float minAdapt = 1, maxAdapt = 1;
          
          if (!_engine->_accomFactorFixedThis)
          {
            minAdapt = _eyeAdaptMin*invAccom;
            maxAdapt = _eyeAdaptMax*invAccom;
          }

          const float pars[8]={
            eyeAccomLastToThis,_deltaT,desiredLuminance,0,
            minAdapt,maxAdapt,minRatio,maxRatio
          };
          
          #if _ENABLE_CHEATS
            if (CHECK_DIAG(DEHDR))
            {
              Log("Adapt. factor: last %g, this %g",_engine->_accomFactorPrev,_engine->_accomFactorThis);
				  Log("eyeAccomLastToThis %g, _deltaT %g, desiredLuminance %g, minAdapt %g, maxAdapt %g, minRatio %g, maxRatio %g",
					  eyeAccomLastToThis, _deltaT, desiredLuminance, minAdapt,maxAdapt,minRatio,maxRatio);
            }
          #endif

          PushPixelShaderConstant psc8(8),psc(9);
          _engine->SetPixelShaderConstantF(cb, 8, pars, 2);
          
          _engine->SetVertexShaderConstantF(cb, 0, zero9, 9);

          int oldAperture = _rtApertureCurrent;
          // switch to next aperture texture
          _rtApertureCurrent = _rtApertureCurrent==0;
          // Set the render target
          _engine->SetRenderTarget(_rtsAperture[_rtApertureCurrent], _rtAperture[_rtApertureCurrent], NULL);

          // 1x1 maxAvgMin as source
          SetTextureTemp tTex0(0, _dec[_decimation2x-1]._rt);
          // old aperture value as another source
          SetTextureTemp tTex1(1, _rtAperture[oldAperture]);
          SetTextureTemp tTex2(2, NULL);
          SetTextureTemp tTex3(3, NULL);
          
          // render target is always linear
          SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);

          // old aperture is always linear
          // source down-sampled scene is SRGB
          SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE, decimatedSRGB);
          SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE, FALSE);
          
          _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
          
          _engine->ResolveRenderTarget();

          #if _DEBUG
            if (CHECK_DIAG(DEHDR))
            {
              // lock textures and read debugging data from them
              Color dec = GetContentDec(_dec[_decimation2x-1]._rts);
              
              Color oldAp = GetContentAp(_rtsAperture[oldAperture]);
              Color newAp = GetContentAp(_rtsAperture[_rtApertureCurrent]);
              Color oldApAbs = oldAp*_engine->_accomFactorPrev;
              Color newApAbs = oldAp*_engine->_accomFactorThis;
              Log(
                "Aperture: %g,%g,%g,%g -> %g,%g,%g,%g",
                oldAp.R(),oldAp.G(),oldAp.B(),oldAp.A(),
                newAp.R(),newAp.G(),newAp.B(),newAp.A()
              );
              Log(
                "Aperture (abs): %g,%g,%g,%g -> %g,%g,%g,%g",
                oldApAbs.R(),oldApAbs.G(),oldApAbs.B(),oldApAbs.A(),
                newApAbs.R(),newApAbs.G(),newApAbs.B(),newApAbs.A()
              );
              Log("Dec: %g,%g,%g,%g",dec.R(),dec.G(),dec.B(),dec.A());
            }
          #endif
        }
				GPU_END_PROFILE
      }
      
      // note: linear space temp goes off scope here (cf. tempNoSRGB0)
    }

#define _DEBUG_LUMINANCE_TEX 0
#if _DEBUG_LUMINANCE_TEX
    static ComRef<IDirect3DSurface9> debugSurface;
    if (!debugSurface)
    {
      D3DFORMAT rtFormat = _engine->_downsampleFormat;
      HRESULT hr = _engine->GetDirect3DDevice().GetRefForCB()->CreateOffscreenPlainSurface(1, 1, rtFormat, D3DPOOL_SYSTEMMEM, debugSurface.Init(), 0);
      if (hr != S_OK)
        _engine->DDError9("CreateOffscreenPlainSurface for debug surface failed: ", hr);
    }
    
    if (debugSurface)
    {
      HRESULT hr = _engine->GetDirect3DDevice().GetRefForCB()->GetRenderTargetData(_dec[_decimation2x-1]._rts, debugSurface);
      if (hr != S_OK)
        _engine->DDError9("GetRenderTargetData for debug surface failed: ", hr);

      D3DLOCKED_RECT rect;
      hr = debugSurface->LockRect(&rect, 0, 0);
      if (hr != S_OK)
      {
        _engine->DDError9("LockRect for debug surface failed: ", hr);
      }
      else
      {
        DWORD data = *(DWORD*)rect.pBits;
        DWORD andValue = (0xFFFFFFFF >> 22);
        DWORD a = data >> 30;
        DWORD r = data & andValue;
        DWORD g = (data >> 10) & andValue;
        DWORD b = (data >> 20) & andValue;
        LogF("Luminance data: %X, a=%u(%f), r=%u(%f), g=%u(%f), b=%u(%f)", data, a, a/3.0f, r, r/1023.0f, g, g/1023.0f, b, b/1023.0f);
        debugSurface->UnlockRect();
      }
    }
#endif //#if _DEBUG_LUMINANCE_TEX
    
    // once we have the aperture, we may perform the Bloom surface preparation
    if (_engine->_doBloom)
    {
      _engine->_pp._bloom->Do(false,rt);
    }
    
    
    // Measure the brightness of the image
    {
      PROFILE_SCOPE_GRF_EX(ppGQy,pp,SCOPE_COLOR_GREEN);
			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Measure brightness: ");

      PushVertexShaderConstant pushC0(0);
      PushVertexShaderConstant pushC1(1);
      PushVertexShaderConstant pushC2(2);
      PushVertexShaderConstant pushC3(3);
      PushVertexShaderConstant pushC4(4);
      PushVertexShaderConstant pushC5(5);
      PushVertexShaderConstant pushC6(6);
      PushVertexShaderConstant pushC7(7);
      PushVertexShaderConstant pushC8(8);
      
      // source is 1 x NBrightQueries image
      // set UV mapping so that we read pixels using point filtering

      static const float baseU[4] = {0, 0, 1, 1};
      static const float baseV[4] = {1, 0, 1, 0};
      
      const float tgtOffsetU = 0;
      const float tgtOffsetV = 0;
      const float offsetU = 0.5f / NBrightQueries;
      const float offsetV = 0;
      
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image - does not matter, not used
        U[vertex][0] = baseU[vertex] + offsetU;
        V[vertex][0] = baseV[vertex] + offsetV;
        // Texture 1 - histohram thresholds
        U[vertex][1] = baseU[vertex] + offsetU;
        V[vertex][1] = baseV[vertex] + offsetV;
        // Texture 2
        U[vertex][2] = 0;
        V[vertex][2] = 0;
        // Texture 3
        U[vertex][3] = 0;
        V[vertex][3] = 0;
      }
      const float posOffset[4] = {tgtOffsetU,tgtOffsetV,0,0};
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);
      _engine->SetVertexShaderConstantF(cb, 8, posOffset, 1);
      
      // _engine->_lastKnownAvgIntensity is the average from the last frame
      // _engine->_accomFactorThis is current exposure settings
      
      float histPars[NBrightQueries+1];
      // CPU reads eyeAccom as determined by the GPU
      // we are using log representation in the pixels shaders for great precision and range
      
      // we want half of the samples to be under 1 and half above 1
      // sample position relative to the expected value
      // last one is always 1 and it is always ignored
      for (int i=0; i<NBrightQueries; i++)
      {
        histPars[i] = HistInit[i];
      }
      // always ignored
      histPars[NBrightQueries]=1;
      
      // we want most probes to be around the expected values
      // if the result it too high, the system will avoid too fast adaptation anyway
      
      // Draw
      // Set temporary render states
      SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
      //SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, 0);
      SetRenderStateTemp rsALPHATESTENABLE(D3DRS_ALPHATESTENABLE, FALSE);

      SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, FALSE);
      SetSamplerStateTemp tempNoSRGB0(0, D3DSAMP_SRGBTEXTURE, FALSE);
      
      // Set the render target
      // note: we need depth buffer, otherwise query does not work at all
      _engine->SetRenderTarget(_rtsMeasure, _rtMeasure, _rtsMeasureDepth);
      _engine->SetTexture(cb, 0, _rtAperture[_rtApertureCurrent]);
      
      _engine->SetPixelShader(cb, _psQuery);
      
      // Drawing
      float values[NBrightQueries];
      for (int b = 0; b < NBrightQueries; b++)
      {
        values[b] = histPars[b]*_engine->_accomFactorThis;
      }
      _querySinglePass.Issue(values,_engine->_accomFactorThis,_histogramTholds);

      // Build the histogram and check the intensity of the image
      {

        // Each level includes all the following levels
        struct HistogramItem
        {
          float minAlpha;
          int pixels;
        } histogram[NBrightQueries+2];

        bool valid = true;
        int numberOfPixelsDrawn[NBrightQueries];
        float value[NBrightQueries];
        float masterValue = _querySinglePass.GetNumberOfPixelsDrawn(numberOfPixelsDrawn,value);
        (void)masterValue;
        for (int b=0; b<NBrightQueries; b++)
        {
          histogram[b+1].minAlpha = value[b];
          histogram[b + 1].pixels = numberOfPixelsDrawn[b];
          if (numberOfPixelsDrawn[b] < 0) valid = false;
        }

        if (valid)
        {
          // as we test 1x1 surface, "histogram" is actually one point only
          float totalAlpha = 0;
          // all pixels are over 0 limit
          histogram[0].minAlpha = 0;
          histogram[0].pixels = 1 * 1;
          // no pixels are over 1 limit
          histogram[NBrightQueries + 1].minAlpha = histogram[NBrightQueries].minAlpha;
          histogram[NBrightQueries + 1].pixels = 0;
          float totalError = 0;
          // verify we got expected results, i.e. line of 1s followed by line of 0s
          int edge = -1;
          for (int b = 0; b < NBrightQueries + 1; b++)
          {
            float alphaMin = histogram[b].minAlpha;
            float alphaMax = histogram[b+1].minAlpha;
            // exclude brighter levels
            int thisCount = histogram[b].pixels - histogram[b + 1].pixels;
            if (thisCount>0)
            {
              // verify the edge is 1 pixel only
              Assert(thisCount==1);
              // verify there is only one edge
              Assert(edge<0);
              edge = b;
            }
            float sumAlpha = thisCount * (alphaMin + alphaMax) * 0.5f;
            float error = thisCount * (alphaMax - alphaMin) * 0.5f;
            totalAlpha += sumAlpha;
            totalError += error;
          }

          //LogF("Avg %.3f", totalAlpha / ((float(MeasureWidth * MeasureHeight) * 255.0f) / 5.0f));

          #if _ENABLE_CHEATS && _ENABLE_REPORT && _ENABLE_PERFLOG
            if (CHECK_DIAG(DEHDR) && CheckMainThread())
            {
              DIAG_MESSAGE(
                500,Format(
                  "Avg expected measured %.4f+(%.4f), %.4f..%.4f",
                  totalAlpha,totalError,totalAlpha-totalError,totalAlpha+totalError
                )
              );
            }
          #endif
          #if _ENABLE_CHEATS
            if (CHECK_DIAG(DEHDR))
            {
              Log("Measured %g (was for eyeAccom %g)",totalAlpha,masterValue);
            }
          #endif
          // if the resulting value is negative, something went wrong
          DoAssert(totalAlpha>=0);
          /*
          // we cannot use masterValue to detect changes, as due to latency this could cause
          // two values alternating and never reaching a stable state
          // if the value did not change significantly from the master value, use the master value
          float newEyeAccom = masterValue;
          if (
            fabs(totalAlpha-masterValue)/totalAlpha>4e-2
          )
          {
            newEyeAccom = totalAlpha;
          }
          _engine->_lastKnownAvgIntensity = newEyeAccom;
          */
          // if the value did not change significantly, do not change it, as changing aperture estimate often can lead to oscillation
          //const float changeThold = 0.04f;
          static float changeThold = 0.1f;
          if (
            _engine->_lastKnownAvgIntensity<=0 ||
            fabs(totalAlpha-_engine->_lastKnownAvgIntensity)/totalAlpha>changeThold
          )
          {
            _engine->_lastKnownAvgIntensity = totalAlpha;
          }
        }
      }
    }
		GPU_END_PROFILE


    // Final step - blending of original and the glow
		GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Blend: ");
    {
      // TODO: use EdgeParams
      //EdgeParams pars;
      //pars.SetupPoint(_engine->Width(),_engine->Height(),_engine->Width(),_engine->Height());
      static const float baseU[4] = {0, 0, 1, 1};
      static const float baseV[4] = {1, 0, 1, 0};

      // when rescaling, we want to allow filtering
      bool rescale = false;
      //static bool forcePoint = false;
      //if (forcePoint) rescale = false;
      
      // point sampling requires 0.5 offset to avoid numerical inaccuracy
      // previous version, well tuned
      const float tgtOffsetU = 0;
      const float tgtOffsetV = 0;
      const float srcOffsetU = rescale ? 0 : 0.5f / _engine->Width();
      const float srcOffsetV = rescale ? 0 : 0.5f / _engine->Height();

      IDirect3DTexture9 *glow = _engine->_pp._bloom->GetBlurredTexture();
  
      D3DSURFACE_DESC glowDesc;
      glow->GetLevelDesc(0, &glowDesc);

      const float glowOffsetU = 0.5f / glowDesc.Width;
      const float glowOffsetV = 0.5f / glowDesc.Height;
      float U[4][4];
      float V[4][4];
      for (int vertex = 0; vertex < 4; vertex++)
      {
        // Texture 0 - source image
        U[vertex][0] = baseU[vertex] + srcOffsetU;
        V[vertex][0] = baseV[vertex] + srcOffsetV;
        // Texture 1 - glow buffer
        U[vertex][1] = baseU[vertex] + glowOffsetU;
        V[vertex][1] = baseV[vertex] + glowOffsetV;
        // Texture 2
        U[vertex][2] = 0;
        V[vertex][2] = 0;
        // Texture 3
        U[vertex][3] = 0;
        V[vertex][3] = 0;
      }
      const float posOffset[4] = {tgtOffsetU,tgtOffsetV,0,0};
      _engine->SetVertexShaderConstantF(cb, 0, U[0], 4);
      _engine->SetVertexShaderConstantF(cb, 4, V[0], 4);
      _engine->SetVertexShaderConstantF(cb, 8, posOffset, 1);


      // Set temporary states
      SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
      SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

      if (isLast)
      {
        // set backbuffer as target
        _engine->SwitchToBackBufferRenderTarget();
        // reset viewport z-range to default
        _engine->_minZRange = 0;
        _engine->_maxZRange = 1;
      }
      else
      {
        // Set the original render target as both target and texture 0
        _engine->SetRenderTarget(origRts,origRt, NULL);
        // reset viewport z-range to default
        _engine->_minZRange = 0;
        _engine->_maxZRange = 1;
      }
    

      SetTextureTemp tTex0(0, rt); // Set the original render target as texture 0
      SetTextureTemp tTex1(1, _engine->_doBloom ? glow : NULL); // glow texture here
      SetTextureTemp tTex2(2, _rtAperture[_rtApertureCurrent]); // aperture control texture
      SetTextureTemp tTex3(3, _dec[_decimation2x-1]._rt); // 1x1 maxAvgMin

      // note: stage0 is set to point sampling by default (see FilterPoint in EngineDD9::PostProcess::Prepare)
      D3DTEXTUREFILTERTYPE filter = rescale ? D3DTEXF_LINEAR : D3DTEXF_POINT;
      SetSamplerStateTemp sampFilterMag0(0, D3DSAMP_MAGFILTER, filter);
      SetSamplerStateTemp sampFilterMin0(0, D3DSAMP_MINFILTER, filter);
      SetSamplerStateTemp sampFilterMag1(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp sampFilterMin1(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

      SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
      SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE, rtIsSRGB);
      SetSamplerStateTemp tempSRGB2(2, D3DSAMP_SRGBTEXTURE, FALSE);
      SetSamplerStateTemp tempSRGB3(3, D3DSAMP_SRGBTEXTURE, decimatedSRGB);

#if _ENABLE_CHEATS
      if (GInput.GetCheat3ToDo(DIK_HOME) || GInput.GetCheat3ToDo(DIK_INSERT))
      {
        static const char *ToneMapMethods[] = { "Arma", "Reinhard", "Filmic" };

        ToneMappingMethodIdx++;
        ToneMappingMethodIdx %= 3;

        DIAG_MESSAGE(1000, Format("Tone map: %s", ToneMapMethods[ToneMappingMethodIdx]));

        if (ToneMappingMethodIdx == 0)
          ToneMappingPars = ZeroToneMappingPars;

        if (ToneMappingMethodIdx == 1)
        {
          ToneMappingPars = ReinhardToneMappingPars;
          DIAG_MESSAGE(5000, Format("%.3f",  ToneMappingPars[0]));
        }          

        if (ToneMappingMethodIdx == 2)
        {
          ToneMappingPars = FilmicToneMappingPars;
          DIAG_MESSAGE(5000, Format("%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f", 
            ToneMappingPars[0], ToneMappingPars[1], ToneMappingPars[2], ToneMappingPars[3],
            ToneMappingPars[4], ToneMappingPars[5], ToneMappingPars[6], ToneMappingPars[7]));
        }
      }
#endif

       SetPixelShaderConstantTemp spc29(29, D3DXVECTOR4(ToneMappingMethodIdx + 0.1f,0,0,0));
      _engine->SetPixelShaderConstantF(cb, 31, ToneMappingPars, 2);

      // compute r,g,b eye sensitivity

      if (_engine->_nightEyeThis>1e-3f)
      {
        // Set the pixel shader
        _engine->SetPixelShader(cb, _psGlowNight);

        // Value designed for night blue shift
        static float coneAdjust = 60.0f;
        float lumScale = coneAdjust/_engine->_accomFactorThis;

        // Get cones
        float cones = Cones();

        // Get Eye sens
        Color eyeSens = EyeSens(cones);
        
        // Set the temporary shader constant
        SetPixelShaderConstantTemp spc6(26, D3DXVECTOR4(eyeSens.R(), eyeSens.G(), eyeSens.B(), 1) );
        SetPixelShaderConstantTemp spc8(28, D3DXVECTOR4(lumScale, cones, 0,0));

        // Drawing
        _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
      }
      else
      {
        // Set the pixel shader
        ColorVal sens = _engine->_nightVisionThis ? NVGSens : DaySens;
        // Set the temporary shader constant
        _engine->SetPixelShader(cb, _engine->_nightVisionThis ? _psGlowNVG : _psGlow);

        // Set the temporary shader constant
        // Drawing
        SetPixelShaderConstantTemp spc6(26, D3DXVECTOR4(sens.R(), sens.G(), sens.B(), 1));
        _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
      }
      
      if (!isLast)
      {
        _engine->ResolveRenderTarget();
      }

    }
		GPU_END_PROFILE

    // unbind any textures to avoid any hanging states
    _engine->SetTexture(cb, 0, NULL);
    _engine->SetTexture(cb, 1, NULL);
    _engine->SetTexture(cb, 2, NULL);
    _engine->SetTexture(cb, 3, NULL);
    
    // remember properties of this frame
    _engine->_accomFactorPrev = _engine->_accomFactorThis;
  }
	GPU_END_PROFILE

  Finish();
  // Switching TL off
}

//-----------------------------------------------------------------------------
GameValue SetToneMapping(const GameState *state, GameValuePar oper1)
{
  if (oper1.GetType() == GameString)
  {
    RString method = oper1;

    if (strcmpi(cc_cast(method), "Arma") == 0)
    {
      ToneMappingMethodIdx = 0;
      ToneMappingPars = ZeroToneMappingPars;
    }
    else if (strcmpi(cc_cast(method), "Reinhard") == 0)
    {
      ToneMappingMethodIdx = 1;
      ToneMappingPars = ReinhardToneMappingPars;
    }
    else if (strcmpi(cc_cast(method), "Filmic") == 0)
    {
      ToneMappingMethodIdx = 2;
      ToneMappingPars = FilmicToneMappingPars;
    }
  }
  else
    state->TypeError(GameString, oper1.GetType());

  return NOTHING;
}

//-----------------------------------------------------------------------------
GameValue SetToneMappingParams(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  if (oper1.GetType() == GameString)
  {
    RString method = oper1;

    if (strcmpi(cc_cast(method), "Reinhard") == 0)
    {
      if (oper2.GetType() == GameArray)
      {
        const GameArrayType &arr = oper2;

        if (arr.Size() > 0)
        {
          float value = arr[0];
          ReinhardToneMappingPars[0] = value * value;
          ReinhardToneMappingPars[1] = arr[1];
        }
      }
    }
    else if (strcmpi(cc_cast(method), "Filmic") == 0)
    {
      if (oper2.GetType() == GameArray)
      {
        const GameArrayType &arr = oper2;

        if (arr.Size() == 8)
        {
          FilmicToneMappingPars[0] = arr[0];
          FilmicToneMappingPars[1] = arr[1];
          FilmicToneMappingPars[2] = arr[2];
          FilmicToneMappingPars[3] = arr[3];
          FilmicToneMappingPars[4] = arr[4];
          FilmicToneMappingPars[5] = arr[5];
          FilmicToneMappingPars[6] = arr[6];
          FilmicToneMappingPars[7] = arr[7];
        }
      }
    }
  }
  else
    state->TypeError(GameString, oper1.GetType());

  return NOTHING;
}

//-----------------------------------------------------------------------------
float EngineDD9::PPGlow::Cones() const
{
  // when adaptation is under 1, we assume iris is fully open
  // under certain level light is received only by rods
  // when accomFactor is high, we are receiving lower

  // subjectiveIntensity = realIntensity*_accomFactorThis
  // realIntensity = subjectiveIntensity/_accomFactorThis;
  static float coneFull = 0.5f;
  static float coneNo = 1.1f;
  float cones = InterpolativC(_engine->_accomFactorThis,coneFull,coneNo,1,-0.2);

  return cones;
}

//-----------------------------------------------------------------------------
Color EngineDD9::PPGlow::EyeSens(float cones) const
{
  // make sure we calculate luminance taking rods/cell sensitivity into account
  Color eyeSens = DaySens*cones+NightSens*(1-cones);

  return eyeSens;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPFlareIntensity::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs)
{
  if (!base::Init(hlsl,vs)) 
		return false;

  // Create flare intensity textures + render targets
  // Flare rendering is done in the back buffer - use same format as backbuffer
  for (int i = 0; i < RenderTargetNum; i++)
  {
    int size = 1 << (RenderTargetNum - i - 1);
    HRESULT err;
    err = _engine->CreateTextureRT(size, size, 1, _engine->_presentPars.BackBufferFormat, D3DPOOL_DEFAULT, _rtLightValues[i], _rtsLightValues[i], true);
    if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
  }

  // Success
  return true;
}


//-----------------------------------------------------------------------------
void EngineDD9::PPFlareIntensity::Prepare()
{
  // base can be PostProcess (PC) or PPSimple (X360). PPSimple will set shaders
  base::Prepare();
}

//-----------------------------------------------------------------------------
static inline void saturate( LONG &a, LONG minV, LONG maxV )
{
  if( a<minV ) a=minV;
  if( a>maxV ) a=maxV;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPFlareIntensity::Do(int x, int y, int sizeX, int sizeY)
{
  // TODOX360: flare visibility testing using visibility predication
  // Draw
  PROFILE_SCOPE_GRF_EX(ppFlr,pp,SCOPE_COLOR_YELLOW);

  const int cb = -1;
  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();
  

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Copy original render target surface into small texture
  RECT rect;
  rect.left = x - sizeX / 2;
  rect.right = x + sizeX / 2;
  rect.top = y - sizeY / 2;
  rect.bottom = y + sizeY / 2;
  // flares are rendered after Postprocess, therefore we are already running at backbuffer resolution
  int w = _engine->WidthBB();
  int h = _engine->HeightBB();
  saturate(rect.left,0,w);
  saturate(rect.right,0,w);
  saturate(rect.top,0,h);
  saturate(rect.bottom,0,h);
  if (rect.bottom<=rect.top || rect.right<=rect.left)
  {
    // singular case - no flare
    ComRef<IDirect3DSurface9> oldRT;
    _engine->GetDirect3DDevice()->GetRenderTarget(0,oldRT.Init());
    _engine->GetDirect3DDevice()->SetRenderTarget(0,_rtsLightValues[0]);
    // it should not matter much - the flare should be invisible anyway
    _engine->GetDirect3DDevice()->Clear(0,NULL,D3DCLEAR_TARGET,0,0,0);
    
    _engine->GetDirect3DDevice()->SetRenderTarget(0,oldRT);
  }
  else
  {
   _engine->GetDirect3DDevice()->StretchRect(_engine->_currRenderTargetSurface, &rect, _rtsLightValues[0], NULL, D3DTEXF_LINEAR);
  }

  // Downsize somewhat (16x for RenderTargetNum=4)
  for (int i = 1; i < RenderTargetNum; i++)
  {
    _engine->GetDirect3DDevice()->StretchRect(_rtsLightValues[i - 1], NULL, _rtsLightValues[i], NULL, D3DTEXF_LINEAR);
  }

  // Set light values texture to stage 12
  _engine->SetTexture(cb, 12, _rtLightValues[RenderTargetNum - 1]);

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
// ou, ov ...texcoords sampling offsets
static bool CreateScreenPlane( VertexP3T2 *v, int segsX, int segsY, float ou, float ov )
{
  float x0 = -1.0f; // -0.5
  float y0 = -1.0f;
  float z = 1.0f;
  float tu0 = 0.0f;
  float tv0 = 1.0f;
  float dx = 2.0 / segsX;
  float dy = 2.0 / segsY;
  float du = 1.0 / segsX;
  float dv = -1.0 / segsY;
  float ya = y0;
  float yb = y0 + dy;
  float tva = tv0 + ov;
  float tvb = tva + dv;
  float x;
  float tu;
  int i, j;
  for( j = 0; j < segsY; j++ )
  {
    x = x0;
    tu = tu0 + ou;
    for( i = 0; i <= segsX; i++ )
    {
      v->pos = Vector3P( x, ya, z ); v->tc.u = tu; v->tc.v = tva; ++v;
      v->pos = Vector3P( x, yb, z ); v->tc.u = tu; v->tc.v = tvb; ++v;
      if( i == segsX )
      {
        if( j != segsY - 1 )
        {
          v->pos = Vector3P( x, yb, z ); v->tc.u = tu; v->tc.v = tvb; ++v;
          v->pos = Vector3P( x0, yb, z ); v->tc.u = tu0; v->tc.v = tvb; ++v;
        }
      }
      else
      {
        x += dx;
        tu += du;
      }
    }
    ya = yb; yb += dy;
    tva = tvb; tvb += dv;
  }
  return true;
}

//-----------------------------------------------------------------------------
static bool CreateScreenPlane2( VertexP3T2 *v, int segsX, int segsY )
{
  float randmaxr = 2 * H_PI / RAND_MAX;
  int n = 3; // num randoms per vertex
  int ls = (segsX+2) * n; // line size
  int size = ls * (segsY+2);

  AutoArray<float, MemAllocLocal<float, 4096> > a;
  a.Resize(size);
  for( int i = 0; i < size; i++ )
    a[i] = rand() * randmaxr - H_PI;
  
  bool even = true;
  float dx = 2.0 / segsX;
  float dy = 2.0 / segsY;
  float xa, xb, ya, yb;
  float xa0 = -1.0f - dx * 0.5f;
  float xb0 = -1.0f;
  int i, j;
  ya = -1.0f;
  yb = ya + dy;
  for( j = 0; j < segsY; j++ )
  {
    if( even )
    {
      xa = xa0; xb = xb0;
      for( i = 0; i <= (segsX+1); i++ )
      {
        v->pos = Vector3P( xa, ya, a[ls*j + i*n] ); v->tc.u = a[ls*j + i*n + 1]; v->tc.v = a[ls*j + i*n + 2]; ++v;
        v->pos = Vector3P( xb, yb, a[ls*(j+1) + i*n] ); v->tc.u = a[ls*(j+1) + i*n + 1]; v->tc.v = a[ls*(j+1) + i*n + 2]; ++v;
        xa += dx; xb += dx;
      }
    }
    else
    {
      xa = xb0 - dx; xb = xa0;
      for( i = 0; i <= (segsX+1); i++ )
      {
        v->pos = Vector3P( xa, ya, a[ls*j + (i-1)*n] ); v->tc.u = a[ls*j + (i-1)*n + 1]; v->tc.v = a[ls*j + (i-1)*n + 2]; ++v;
        v->pos = Vector3P( xb, yb, a[ls*(j+1) + i*n] ); v->tc.u = a[ls*(j+1) + i*n + 1]; v->tc.v = a[ls*(j+1) + i*n + 2]; ++v;
        xa += dx; xb += dx;
      }
    }
    ya = yb; yb += dy;
    even = !even;
  }
  return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRotBlur::SetThresholdAngle( float angle )
{
  minCos = cos( angle * H_PI / 180 );
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPRotBlur::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs )
{
  //if( !base::Init( hlsl, vs, "VsPpRotBlur" ) ) return false;
  if (!base::Init(hlsl,vs)) return false;
  enabled = false;
  SetDefaultValues();

  // screen plane construction
  const int segsX = 16;
  const int segsY = 10;
  vertexCount = ( segsX + 1 + 1 ) * 2 * segsY - 2;
  stripSize = vertexCount - 2;
  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexP3T2) * vertexCount, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  VertexP3T2 *v;
  // pixel offsets
  float ou = 1.0f / _engine->Width() * 0.5f; //See the Nearest-Point Sampling in the DX9 documentation
  float ov = 1.0f / _engine->Height() * 0.5f;
  vb->Lock(0, 0, (void**)&v, 0);
  CreateScreenPlane( v, segsX, segsY, ou, ov );
  vb->Unlock();

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }
  // note: vertex shader is stored in PS.hlsl for simple effects
  if (!CompileVertexShader(hlsl,"VsPpRotBlur",_vs))
  {
    return false;
  }
  _vb = VertexStaticData9::BufferFuture(vb);
  _vd = vd;
  if( !CompilePixelShader( hlsl, "PsPpRotBlur", _ps ) ) return false;
  //_ppGaussianBlur = ppGaussianBlur;
  //cam = camera;
  
  _lastTime = Glob.time;

  _oldView = MIdentity;

  // angle upper bound
  _upperBound = 10.0f * (H_PI / 180.0f);

  return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRotBlur::Prepare()
{
  const int cb = -1;
  base::Prepare();

  _engine->SetStreamSource( cb, _vb, sizeof(VertexP3T2), vertexCount );
  _engine->SetVertexDeclaration(cb, _vd);
  _engine->SetVertexShader(cb, _vs);
  _engine->SetPixelShader(cb, _ps);
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRotBlur::Do( bool isLast )
{
  ///if( !enabled ) return;
  if (blurPower==0) return;
  // Draw
  PROFILE_SCOPE_GRF_EX( ppRotBlur, pp, SCOPE_COLOR_YELLOW );

  const int cb = -1;
  // threshold
  // current cam matrix
  Matrix4 &m1 = _engine->CBState(cb)._view;

  //  last frame view matrix, inverse current view matrix
  Matrix3 final3 = _oldView.Orientation() * m1.Orientation().InverseGeneral();

  // convert matrix to axis-align form, for this we use quaternion
  Quaternion<float> quatern;
  quatern.FromMatrixRotation(final3);
  quatern.Normalize();

  float cosAngle = quatern.W();
  // rotation angle, which is used as lower and upper bound
  float angle = acos(cosAngle) * 2.0f;

  static float BPMult = 0.33f;
  float bPower = (_engine->_nightVisionThis) ? (blurPower * BPMult) : blurPower;

  // apply rotation blur only when rotation angle is greater than lower bound
  if (angle < minCos)
  {
    // If scene is not being rendered initialize scene
    bool inScene = _engine->_d3dFrameOpen;
    if (!inScene) _engine->D3DBeginScene();

    // Prepare drawing
    Prepare();

    // Set temporary render states
    SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

    // Stage 0 - base texture - do the copy
    IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
    SetTextureTemp tTex0(0, rt);

    SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
    SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // Stage 1 - depth map
    SetTextureTemp tTex1(1, _engine->GetDepthAsTexture());
    SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, FALSE);
    SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    /*
    -----------------------------------------------------------------------------------------------------------
    Originally was motion vector determined by difference between position in current frame and last frame and 
    for this purpose were both matrices send into vertex shader ...
    
      posA = pos * matrixA ... old view matrix
      posB = pos * matrixB ... current view matrix
      dir = posB - posA
      dir = dir * inverseMatrixB
    
    Now we used improved method based on single matrix, way to determine this matrix is shown in next lines ...
    
      dir = (posB - posA) * inverseMatrixB
      dir = (pos * matrixB - pos * matrixA) * inverseMatrixB
      dir = pos - (pos * matrixA * inverseMatrixB);
    -----------------------------------------------------------------------------------------------------------
    */

    Vector3 vec(quatern.X(), quatern.Y(), quatern.Z());
    float dt = Glob.time - _lastTime;

    // fps > 30
    if (dt < 0.033333f)
    {
      if (angle >_upperBound)
      {
        final3.SetRotationAxis(vec, _upperBound);
      }
    }
    else
    {
      // rotation bound is select as min(maximum rotational angle, angular velocity with regard to 30fps)
      float currBound = min(_upperBound, (angle / dt) * 0.033333f);

      final3.SetRotationAxis(vec, currBound);
    }

    SetVertexShaderConstantTemp svc0( 0, D3DXVECTOR4( final3(0, 0), final3(0, 1), final3(0, 2), depthThreshold*Glob.config.GetMaxInvLinearDepth()) );
    SetVertexShaderConstantTemp svc1( 1, D3DXVECTOR4( final3(1, 0), final3(1, 1), final3(1, 2), bPower ) );
    SetVertexShaderConstantTemp svc2( 2, D3DXVECTOR4( final3(2, 0), final3(2, 1), final3(2, 2), 0 ) );    

    float kx = 1.0f / _engine->CBState(cb)._proj( 0, 0 );
    float ky = 1.0f / _engine->CBState(cb)._proj( 1, 1 );
    float ktc = _engine->CBState(cb)._proj( 1, 1 )*kx;
    SetVertexShaderConstantTemp svc6( 6, D3DXVECTOR4( kx, ky, ktc, 0 ) );
    //SetVertexShaderConstantTemp svc6( 6, D3DXVECTOR4( 1.0, 1.0, 1.0, 0 ) );

    // Drawing
		GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Rotation blur: ");
			_engine->DrawPrimitive( cb, D3DPT_TRIANGLESTRIP, 0, stripSize );
		GPU_END_PROFILE

    // If scene is not being rendered finish initialized scene
    if (!inScene) _engine->D3DEndScene();

    // Restore usual rendering settings
    Finish();
  }

  _lastTime = Glob.time;
  _oldView = m1;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPSimpleExt::Prepare()
{
  base::Prepare();
  const int cb = -1;
  const PPResourcesExt *res = Resources();
  if (res)
  {
    _engine->SetStreamSource(cb,res->_vb, sizeof(Vertex), 4);
    _engine->SetVertexDeclaration(cb,res->_vd);
    _engine->SetVertexShader(cb,res->_vs);
    _engine->SetPixelShader(cb,res->_ps);
  }
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPSimpleExt::Init(const LazyLoadFile &hlsl, const LazyLoadFile &vs, const char *vsName)
{
  if (!base::Init(hlsl,vs)) return false;

  Ref<PPResourcesExt> res = new PPResourcesExt;

  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  Vertex *v;
  float offsetU = 1.0f / _engine->Width() * 0.5f; //See the Nearest-Point Sampling in the DX9 documentation
  float offsetV = 1.0f / _engine->Height() * 0.5f;
  vb->Lock(0, 0, (void**)&v, 0);
  v[0].pos = Vector3P(-0.5, -0.5, 0.5); v[0].t0.u = 0 + offsetU; v[0].t0.v = 1 + offsetV;
  v[1].pos = Vector3P(-0.5,  0.5, 0.5); v[1].t0.u = 0 + offsetU; v[1].t0.v = 0 + offsetV;
  v[2].pos = Vector3P( 0.5, -0.5, 0.5); v[2].t0.u = 1 + offsetU; v[2].t0.v = 1 + offsetV;
  v[3].pos = Vector3P( 0.5,  0.5, 0.5); v[3].t0.u = 1 + offsetU; v[3].t0.v = 0 + offsetV;
  vb->Unlock();

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }

  // note: vertex shader is stored in PS.hlsl for simple effects
  if (!CompileVertexShader(hlsl,vsName,res->_vs))
  {
    return false;
  }

  res->_vb = VertexStaticData9::BufferFuture(vb);
  res->_vd = vd;

  _resrc = res;

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPSimpleExt::Init(PPResources *resources, bool initFirst)
{
  if (resources)
  {
    SetLevel(0xFF); // default level
    _resrc = resources;
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPRadialBlur::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit )
{
  if( !base::Init( hlsl, vs, "VSPostProcess" ) ) return false;
  PPResourcesExt *res = const_cast<PPResourcesExt *> (base::Resources());
  if( !CompilePixelShader( hlsl, "PsPpRadialBlur", res->_ps, true) ) return false;
  
  if (firstInit)
  {
    SetDefaultValues();
    enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPRadialBlur::Init(PPResources *resources, bool firstInit)
{
  if (!base::Init(resources, firstInit)) return false;

  if (firstInit)
  {
    SetDefaultValues();
    enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPRadialBlur::SetParams(const AutoArray<float> &pars)
{
  if (pars.Size() == 4)
  {
    SetBlurPower( pars[0], pars[1] );
    SetBlurOffset( pars[2], pars[3] );

    return 0;
  }

  return 4;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPRadialBlur::SetParams(const float* pars, int nPars)
{
  if (nPars == 4)
  {
    SetBlurPower( pars[0], pars[1] );
    SetBlurOffset( pars[2], pars[3] );

    return 0;
  }
  return 4;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPRadialBlur::UnInit()
{
  if (_resrc) 
  { 
    _resrc->UnInit(); 
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPRadialBlur::Do( bool isLast )
{
  if( !enabled )
    return;

  // Draw
  PROFILE_SCOPE_GRF_EX( ppRadialBlur, pp, SCOPE_COLOR_YELLOW );

  // linearly interpolated parameters
  RadBlurPar pars = _pInterp.Simulate();
  const PPResourcesExt *res = Resources();
  const int cb = -1;

  if (!res) return;

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Stage 0 - base texture - do the copy (to make sure same data is both in source texture and destination render target)
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
  SetTextureTemp tTex0(0, rt);
  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // if effect is last then switch to back buffer
  if (isLast) _engine->SwitchToBackBufferRenderTarget();

  // Prepare PS constants
  //SetPixelShaderConstantTemp spc0( 0, D3DXVECTOR4( blurPowerX, blurPowerY, blurOffsetX, blurOffsetY ) );
  SetPixelShaderConstantTemp spc0( 0, D3DXVECTOR4( pars._powerX, pars._powerY, pars._offsetX, pars._offsetY ) );
  float kx = 1.0 / _engine->CBState(cb)._proj( 0, 0 );
  float ky = 1.0 / _engine->CBState(cb)._proj( 1, 1 );
  float ktc = _engine->CBState(cb)._proj( 1, 1 )*kx;
  SetPixelShaderConstantTemp spc6( 6, D3DXVECTOR4( kx, ky, ktc, 0 ) );

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Radial blur: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPRadialBlur::Serialize( ParamArchive &ar )
{
  CHECK(ar.Serialize("enabled",enabled,0,false));
  CHECK(ar.Serialize("interp", _pInterp, 0));
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPColors::Init(PPResources *resources, bool firstInit)
{
  if (resources)
  {
    _resrc = resources;

    if (firstInit)
    {
      enabled = false;
      SetDefaultValues();
      _pInterp.Initialize();
    }

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPColors::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit )
{
  if (!base::Init(hlsl,vs)) return false;
  // sepia test
  //SetColorizeColor( Color( 1.25f, 0.85f, 0.60f, 0.0f ) );

  Ref<PPColorResources> resrc = new PPColorResources;

  // screen plane construction
  const int segsX = 1;
  const int segsY = 1;
  resrc->_vertexCount = ( segsX + 1 + 1 ) * 2 * segsY - 2;
  resrc->_stripSize = resrc->_vertexCount - 2;

  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexP3T2) * resrc->_vertexCount, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  VertexP3T2 *v;
  // pixel offsets
  float ou = 1.0f / _engine->Width() * 0.5f; //See the Nearest-Point Sampling in the DX9 documentation
  float ov = 1.0f / _engine->Height() * 0.5f;
  vb->Lock(0, 0, (void**)&v, 0);
  CreateScreenPlane( v, segsX, segsY, ou, ov );
  vb->Unlock();

  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }
  // note: vertex shader is stored in PS.hlsl for simple effects
  if (!CompileVertexShader(hlsl,"VsPpColors",resrc->_vs))
  {
    return false;
  }
  resrc-> _vb = VertexStaticData9::BufferFuture(vb);
  resrc->_vd = vd;
  if( !CompilePixelShader( hlsl, "PsPpColors", resrc->_ps ) ) return false;

  _resrc = resrc;

  if (firstInit)
  {
    SetDefaultValues();
    enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPColors::DoDirect(const RECT *rect, int idx)
{
#if _HELISIM_RENDER_TO_TEXTURE
  DoAssert(_engine->_r2tTargets[idx]._clrPars);

  if (!_engine->_r2tTargets[idx]._clrPars)
    return;

  const PPColorResources* res = Resources();
  DoAssert(res);

  if (!res)
    return;

  ComRef<IDirect3DDevice9> &device = _engine->_d3DDevice->GetRefForCB();

  R2TColorsPars &clrPars =  *_engine->_r2tTargets[idx]._clrPars.GetRef();
  {
    ComRef<IDirect3DSurface9> _renderTargetSurfaceSaved;
    ComRef<IDirect3DSurface9> _renderTargetDepthBufferSurfaceSaved;

    // Save current RTs
    device->GetRenderTarget(0, _renderTargetSurfaceSaved.Init());
    device->GetDepthStencilSurface(_renderTargetDepthBufferSurfaceSaved.Init());

    // We will need a source as a texture
    ComRef<IDirect3DTexture9> rtCopy = _engine->CreateR2TRenderTargetCopyAsTexture(device, rect, D3DTEXF_LINEAR);
    device->SetTexture(0, rtCopy);

    device->SetRenderTarget(0, _engine->_r2tRT._renderTargetSurfaceS);

    // Set viewport
    {
      D3DVIEWPORT9 viewData;
      memset(&viewData, 0, sizeof(viewData));
      viewData.X = 0;
      viewData.Y = 0;
      viewData.Width  = rect->right - rect->left;
      viewData.Height = rect->bottom - rect->top;
      viewData.MinZ = 0.0f;
      viewData.MaxZ = 1.0f;
      device->SetViewport(&viewData);
    }

    device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

    device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);

    device->SetStreamSource(0,
#if _ARMA3_VERTEX_BUFFER_EX
      res->_vb.GetResult().GetBuffer(), 
#else
      res->_vb.GetResult(), 
#endif
      0, sizeof(VertexP3T2));
    device->SetVertexDeclaration(res->_vd);
    device->SetVertexShader(res->_vs);
    device->SetPixelShader(res->_ps);

#if _ENABLE_CHEATS
    static bool SaveToTexture = false;

    if (GInput.GetCheat3ToDo(DIK_F))
      SaveToTexture = true;


    if (SaveToTexture)
      D3DXSaveTextureToFile("rtCopy.png", D3DXIFF_PNG, rtCopy, NULL);
#endif

    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    device->SetPixelShaderConstantF(0, D3DXVECTOR4(clrPars._brightness, clrPars._contrast, clrPars._offset, 0), 1);
    device->SetPixelShaderConstantF(2, D3DXVECTOR4(clrPars._lerpColor.R(), clrPars._lerpColor.G(), clrPars._lerpColor.B(), clrPars._lerpColor.A()), 1 );
    device->SetPixelShaderConstantF(3, D3DXVECTOR4(clrPars._colorizeColor.R(), clrPars._colorizeColor.G(), clrPars._colorizeColor.B(), clrPars._colorizeColor.A()), 1);
    device->SetPixelShaderConstantF(4, D3DXVECTOR4(clrPars._rgbWeights.R(), clrPars._rgbWeights.G(), clrPars._rgbWeights.B(), 0), 1);
    device->SetPixelShaderConstantF(6, D3DXVECTOR4(clrPars._coeffs.X(), clrPars._coeffs.Y(), clrPars._coeffs.Z(), 0), 1);

    device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    
		GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Colors::DoDirect:");
			device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, res->_stripSize);
		GPU_END_PROFILE

    // clear texture stage
    device->SetTexture(0, NULL);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);

    // Restore targets
    device->SetRenderTarget(0, _renderTargetSurfaceSaved);
    device->SetDepthStencilSurface(_renderTargetDepthBufferSurfaceSaved);

    _renderTargetSurfaceSaved.Free();
    _renderTargetDepthBufferSurfaceSaved.Free();

#if _ENABLE_CHEATS
      SaveToTexture = false;
#endif
  }
#endif
}

//-----------------------------------------------------------------------------
void EngineDD9::PPColors::Do( bool isLast )
{
  if( !enabled )
    return;

  // Draw
  PROFILE_SCOPE_GRF_EX( ppColors, pp, SCOPE_COLOR_YELLOW );

  ColorPar pars = _pInterp.Simulate();
  const PPColorResources* res = Resources();
  const int cb = -1;

  if (!res) return;

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Stage 0 - base texture - do the copy (to make sure same data is both in source texture and destination render target)
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
  SetTextureTemp tTex0(0, rt);

  // if effect is last then switch to back buffer
  if (isLast) _engine->SwitchToBackBufferRenderTarget();

  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // Prepare PS constants
  SetPixelShaderConstantTemp ssPpColorsPars( 0, D3DXVECTOR4( pars._brightness, pars._contrast, pars._offset, 0 ) );
  SetPixelShaderConstantTemp ssLerpColor( 2, D3DXVECTOR4( pars._lerpColor.R(), pars._lerpColor.G(), pars._lerpColor.B(), pars._lerpColor.A() ) );
  SetPixelShaderConstantTemp ssColorizeColor( 3, D3DXVECTOR4( pars._colorizeColor.R(), pars._colorizeColor.G(), pars._colorizeColor.B(), pars._colorizeColor.A() ) );
  SetPixelShaderConstantTemp ssRgbWeights( 4, D3DXVECTOR4( pars._rgbWeights.R(), pars._rgbWeights.G(), pars._rgbWeights.B(), 0 ) );

  float kx = 1.0 / _engine->CBState(cb)._proj( 0, 0 );
  float ky = 1.0 / _engine->CBState(cb)._proj( 1, 1 );
  float ktc = _engine->CBState(cb)._proj( 1, 1 )*kx;
  SetPixelShaderConstantTemp spc6( 6, D3DXVECTOR4( kx, ky, ktc, 0 ) );

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Colors:");
		_engine->DrawPrimitive( cb, D3DPT_TRIANGLESTRIP, 0, res->_stripSize );
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
void EngineDD9::PPColors::Prepare()
{
  const int cb = -1;
  const PPColorResources *res = Resources();

  if (!res) return;

  base::Prepare();
  // Set the stream source
  _engine->SetStreamSource(cb, res->_vb, sizeof(VertexP3T2), res->_vertexCount);
  // Set the vertex declaration
  _engine->SetVertexDeclaration(cb, res->_vd);
  // Set the vertex shader
  _engine->SetVertexShader(cb, res->_vs);
  // Set the pixel shader
  _engine->SetPixelShader(cb, res->_ps);
}

//-----------------------------------------------------------------------------
int EngineDD9::PPColors::SetParams(const AutoArray<float> &pars)
{
  if (pars.Size() == 15)
  {
    SetBrightness(pars[0]);
    SetContrast(pars[1]);
    SetOffset(pars[2]);
    SetBlendColor(Color( pars[3], pars[4], pars[5], pars[6] ));
    SetColorizeColor(Color( pars[7], pars[8], pars[9], pars[10] ));
    SetRGBWeights(Color( pars[11], pars[12], pars[13], pars[14] ));

    return 0;
  }

  return 15;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPColors::SetParams(const float* pars, int nPars)
{
  if (nPars == 15)
  {
    SetBrightness(pars[0]);
    SetContrast(pars[1]);
    SetOffset(pars[2]);
    SetBlendColor(Color( pars[3], pars[4], pars[5], pars[6] ));
    SetColorizeColor(Color( pars[7], pars[8], pars[9], pars[10] ));
    SetRGBWeights(Color( pars[11], pars[12], pars[13], pars[14] ));

    return 0;
  }

  return 15;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPColors::UnInit()
{
  if (_resrc) 
  {
    _resrc->UnInit();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPColors::Serialize( ParamArchive &ar )
{
  CHECK(ar.Serialize("enabled",enabled,0,false));
  CHECK(ar.Serialize("pInterp",_pInterp,0));
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPChromAber::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit )
{
  if( !base::Init( hlsl, vs, "VSPostProcess" ) ) return false;
  PPResourcesExt *res = const_cast<PPResourcesExt *> (base::Resources());
  if( !CompilePixelShader( hlsl, "PsPpChromAber", res->_ps, true) ) return false;

  if (firstInit)
  {
    SetDefaultValues();
    enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPChromAber::Init(PPResources *resources, bool firstInit)
{
  if (!base::Init(resources, firstInit)) return false;
  
  if (firstInit)
  {
    SetDefaultValues();
    enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPChromAber::SetParams(const AutoArray<float> &pars)
{
  if (pars.Size() == 3)
  {
    SetAberration( pars[0], pars[1] );
    SetAspectCorrection( pars[2] > 0.0f );

    return 0;
  }

  return 3;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPChromAber::SetParams(const float* pars, int nPars)
{
  if (nPars == 3)
  {
    SetAberration( pars[0], pars[1] );
    SetAspectCorrection( pars[2] > 0.0f );

    return 0;
  }

  return 3;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPChromAber::UnInit()
{
  if (_resrc)
  {
    _resrc->UnInit();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPChromAber::Do( bool isLast )
{
  if( !enabled )
    return;

  // Draw
  PROFILE_SCOPE_GRF_EX( ppChromAber, pp, SCOPE_COLOR_YELLOW );
  
  // linearly interpolated parameters 
  ChromAberPar pars = _pInterp.Simulate();
  const int cb = -1;


  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // if effect is last then switch to back buffer
  if (isLast) _engine->SwitchToBackBufferRenderTarget();
  
  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Stage 0 - base texture - do the copy
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
  SetTextureTemp tTex0(0, rt);

  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  float ac = 1;
  if( pars._aspectCorrection )
    ac = _engine->CBState(cb)._proj( 0, 0 ) / _engine->CBState(cb)._proj( 1, 1 );
  //SetPixelShaderConstantTemp spc6( 6, D3DXVECTOR4( ac, 0, 0, 0 ) );
  // Prepare PS constants
  SetPixelShaderConstantTemp spc0( 0, D3DXVECTOR4( pars._powerX * ac, pars._powerY, 0, 0 ) );

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "ChromAbberation: ");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPChromAber::Serialize( ParamArchive &ar )
{
  CHECK(ar.Serialize("enabled",enabled,0,false));
  CHECK(ar.Serialize("interp", _pInterp, 0));
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPWetDistort::Init(PPResources *resources, PPGaussianBlur *ppGaussianBlur, bool firstInit)
{
  if (resources)
  {
    _resrc = resources;
    _ppGaussianBlur = ppGaussianBlur;

    if (firstInit)
    {
      SetDefaultValues();
      enabled = false;
      _pInterp.Initialize();
    }

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPColorsInversion::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, bool firstInit )
{
  if( !base::Init( hlsl, vs, "VSPostProcess" ) ) return false;
  PPResourcesExt *res = const_cast<PPResourcesExt *> (base::Resources());
  if( !CompilePixelShader( hlsl, "PsPpClrInversion", res->_ps, true) ) return false;

  if (firstInit)
  {
    SetDefaultValues();
    _enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPColorsInversion::Init(PPResources *resources, bool firstInit)
{
  if (!base::Init(resources, firstInit)) return false;

  if (firstInit)
  {
    SetDefaultValues();
    _enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPColorsInversion::UnInit()
{
  if (_resrc)
  {
    _resrc->UnInit();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPColorsInversion::Do(bool isLast)
{
  if (!_enabled) return;

  // Draw
  PROFILE_SCOPE_GRF_EX(ppColorInvers, pp, SCOPE_COLOR_YELLOW);

  // linearly interpolated parameters 
  ColorInversionPar pars = _pInterp.Simulate();
  const int cb = -1;

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Stage 0 - base texture - do the copy
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
  SetTextureTemp tTex0(0, rt);

  // if effect is last then switch to back buffer
  if (isLast) _engine->SwitchToBackBufferRenderTarget();

  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // Prepare PS constants
  SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(pars._invColor.R(), pars._invColor.G(), pars._invColor.B(), 0));

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Color inversion:");
		_engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();

  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
int EngineDD9::PPColorsInversion::SetParams(const AutoArray<float> &pars)
{
  if (pars.Size() == 3)
  {
    SetInversion(Color(pars[0], pars[1], pars[2], 0));
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPColorsInversion::SetParams(const float* pars, int nPars)
{
  if (nPars == 3)
  {
    SetInversion(Color(pars[0], pars[1], pars[2], 0));
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPColorsInversion::Serialize( ParamArchive &ar )
{
  CHECK(ar.Serialize("enabled", _enabled,0,false));
  CHECK(ar.Serialize("interp", _pInterp, 0));
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPWetDistort::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, PPGaussianBlur *ppGaussianBlur, bool firstInit )
{
  if( !base::Init(hlsl,vs) )
    return false;
  _ppGaussianBlur = ppGaussianBlur;	

  Ref<PPWetDistortResources> res = new PPWetDistortResources;

  // screen plane construction
  const int segsX = 80;
  const int segsY = 50;
  res->_vertexCount = ( segsX + 1 + 1 ) * 2 * segsY;
  res->_stripSize = res->_vertexCount - 2;
  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexP3T2) * res->_vertexCount, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  VertexP3T2 *v;

  vb->Lock(0, 0, (void**)&v, 0);
  CreateScreenPlane2( v, segsX, segsY );
  vb->Unlock();


  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }
  // note: vertex shader is stored in PS.hlsl for simple effects
  if (!CompileVertexShader( hlsl,"VsPpWetDistort",res->_vs ) )
  {
    return false;
  }
  res->_vb = VertexStaticData9::BufferFuture(vb);
  res->_vd = vd;
  if( !CompilePixelShader( hlsl, "PsPpWetDistort", res->_ps ) ) return false;

  _resrc = res;

  if (firstInit)
  {
    SetDefaultValues();
    enabled = false;
    _pInterp.Initialize();
  }

  return true;

}

//-----------------------------------------------------------------------------
int EngineDD9::PPWetDistort::SetParams(const AutoArray<float> &pars)
{
  if (pars.Size() == 15)
  {
    SetBlurriness( pars[0] );
    SetEffectPower( pars[1], pars[2] );
    SetWaveSpeeds( pars[3], pars[4], pars[5], pars[6] );
    SetWaveAmplitudes( pars[7], pars[8], pars[9], pars[10] );
    SetPhaseCoefs( pars[11], pars[12], pars[13], pars[14] );

    return 0;
  }

  return 15;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPWetDistort::SetParams(const float* pars, int nPars)
{
  if (nPars == 15)
  {
    SetBlurriness( pars[0] );
    SetEffectPower( pars[1], pars[2] );
    SetWaveSpeeds( pars[3], pars[4], pars[5], pars[6] );
    SetWaveAmplitudes( pars[7], pars[8], pars[9], pars[10] );
    SetPhaseCoefs( pars[11], pars[12], pars[13], pars[14] );

    return 0;
  }

  return 15;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPWetDistort::UnInit()
{
  _ppGaussianBlur.Free();
  
  if (_resrc) 
  {
    _resrc->UnInit();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPWetDistort::Prepare()
{
  const int cb = -1;
  const PPWetDistortResources *res = Resources();
  if (res)
  {
    base::Prepare();
    // Set the stream source
    _engine->SetStreamSource( cb, res->_vb, sizeof(VertexP3T2), res->_vertexCount );
    // Set the vertex declaration
    _engine->SetVertexDeclaration(cb, res->_vd);
    // Set the vertex shader
    _engine->SetVertexShader(cb, res->_vs);
    // Set the pixel shader
    _engine->SetPixelShader(cb, res->_ps);
  }
}

//-----------------------------------------------------------------------------
void EngineDD9::PPWetDistort::Do( bool isLast )
{
  if( !enabled )
    return;

  // Draw
  PROFILE_SCOPE_GRF_EX( ppWetDistort, pp, SCOPE_COLOR_YELLOW );
  
  WetDistortPar pars = _pInterp.Simulate();
  const int cb = -1;
  const PPWetDistortResources *res = Resources();

  if (!res) return;
 
  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Stage 0 - base texture - do the copy
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();
  SetTextureTemp tTex0(0, rt);
  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // Stage 1 - blurred texture
  SetTextureTemp tTex1(1, _ppGaussianBlur->GetBlurredTexture());
  SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  float t = Glob.time.toFloat();
  SetVertexShaderConstantTemp svc0( 0, D3DXVECTOR4( t, pars._blurriness, pars._powerTop, pars._powerBottom) ); // time, blur, powerTop, powerBottom
  SetVertexShaderConstantTemp svc1( 1, D3DXVECTOR4( pars._waveSpeeds[0], pars._waveSpeeds[1], pars._waveSpeeds[2], pars._waveSpeeds[3] ) ); // speeds
  SetVertexShaderConstantTemp svc2( 2, D3DXVECTOR4( pars._waveAmps[0], pars._waveAmps[1], pars._waveAmps[2], pars._waveAmps[3] ) ); // amplitudes
  SetVertexShaderConstantTemp svc3( 3, D3DXVECTOR4( pars._phaseCoefs[0], pars._phaseCoefs[1], pars._phaseCoefs[2], pars._phaseCoefs[3] ) ); // randX, randY, posX, posY

  // chromatic aberration params
  SetPixelShaderConstantTemp spc4( 4, D3DXVECTOR4( 0.8, 1.0, 1.2, 0 ) );

  // if effect is last then switch to back buffer
  if (isLast) _engine->SwitchToBackBufferRenderTarget();
  
  float kx = 1.0 / _engine->CBState(cb)._proj( 0, 0 );
  float ky = 1.0 / _engine->CBState(cb)._proj( 1, 1 );
  float ktc = _engine->CBState(cb)._proj( 1, 1 ) *kx;
  SetVertexShaderConstantTemp svc6( 6, D3DXVECTOR4( kx, ky, ktc, 0 ) );
  
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "WetDistort: ");
		_engine->DrawPrimitive( cb, D3DPT_TRIANGLESTRIP, 0, res->_stripSize );
	GPU_END_PROFILE

  // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();
  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPWetDistort::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("enabled",enabled,0,false));
  CHECK(ar.Serialize("pInterp",_pInterp,0));  
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPDynamicBlur::Init( const LazyLoadFile &hlsl, const LazyLoadFile &vs, PPGaussianBlur *ppGaussianBlur, bool firstInit )
{
  if( !base::Init( hlsl,vs ) )
    return false;
  _ppGaussianBlur = ppGaussianBlur;

  Ref<PPDynBlurResources> res = new PPDynBlurResources;

  // screen plane construction
  const int segsX = 1;
  const int segsY = 1;
  res->_vertexCount = ( segsX + 1 + 1 ) * 2 * segsY - 2;
  res->_stripSize = res->_vertexCount - 2;
  // Create the VB
  ComRef<IDirect3DVertexBuffer9> vb;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexP3T2) * res->_vertexCount, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
  {
    LogF("Error: Vertex buffer creation failed.");
    return false;
  }
  VertexP3T2 *v;
  vb->Lock(0, 0, (void**)&v, 0);
  CreateScreenPlane( v, segsX, segsY, 0, 0 ); // ou, ov
  vb->Unlock();
  res->_vb = VertexStaticData9::BufferFuture(vb);
  // Create the VD
  const D3DVERTEXELEMENT9 vertexDecl[] =
  {
    {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
  };
  ComRef<IDirect3DVertexDeclaration9> vd;
  if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
  {
    LogF("Error: Vertex declaration creation failed.");
    return false;
  }
  res->_vd = vd;
  // create shaders (note: vertex shader is stored in PS.hlsl for simple effects)
  if( !CompileVertexShader( hlsl,"VsPpDynamicBlurFinal", res->_vsFinal ) )
    return false;
  if( !CompilePixelShader( hlsl, "PsPpDynamicBlurFinal", res->_psFinal ) )
    return false;
  if( !CompileVertexShader( hlsl,"VsPpDynamicBlur", res->_vsBlur ) )
    return false;
  if( !CompilePixelShader( hlsl, "PsPpDynamicBlur", res->_psBlur ) )
    return false;

  _resrc = res;

  if (firstInit)
  {
    SetDefaultValues();
    _enabled = false;
    _pInterp.Initialize();
  }

  return true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPDynamicBlur::Init(PPResources *resources, PPGaussianBlur *ppGaussianBlur, bool firstInit)
{
  if (resources)
  {
    _ppGaussianBlur = ppGaussianBlur;
    _resrc = resources;

    if (firstInit)
    {
      SetDefaultValues();
      _enabled = false;
      _pInterp.Initialize();
    }

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPDynamicBlur::SetParams(const AutoArray<float> &pars) 
{
  if (pars.Size() == 1)
  {
    SetBlurrines(pars[0]);

    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
int EngineDD9::PPDynamicBlur::SetParams(const float* pars, int nPars)
{
  if (nPars == 1)
  {
    SetBlurrines(pars[0]);
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
bool EngineDD9::PPDynamicBlur::UnInit()
{
  _ppGaussianBlur.Free();
  
  if (_resrc)
  {
    _resrc->UnInit();
  }

  return true;
}

//-----------------------------------------------------------------------------
void EngineDD9::PPDynamicBlur::Do( bool isLast )
{
  if (!_enabled) return;

  DynamicBlurPar pars = _pInterp.Simulate();

  // Draw
  PROFILE_SCOPE_GRF_EX( ppDynamicBlur, pp, SCOPE_COLOR_YELLOW );

  const int cb = -1;
  const PPDynBlurResources *res = Resources();

  if (!res) return;

	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "Dynamic blur: ");

  // TESTING
  /*float t = Glob.time.toFloat();
  blurriness = ( sinf( t * 2 ) + 1 ) * 20 - 5 ;
  if( blurriness < 0 )
  blurriness = 0;
  if( blurriness > 30 )
  blurriness = 30;*/

  // If scene is not being rendered initialize scene
  bool inScene = _engine->_d3dFrameOpen;
  if (!inScene) _engine->D3DBeginScene();

  // Prepare drawing
  Prepare();

  // Protect VS constants from changing
  PushVertexShaderConstant pushC0(0);
  PushVertexShaderConstant pushC1(1);
  PushVertexShaderConstant pushC2(2);
  PushVertexShaderConstant pushC3(3);
  PushVertexShaderConstant pushC4(4);
  PushVertexShaderConstant pushC5(5);
  PushVertexShaderConstant pushC6(6);
  PushVertexShaderConstant pushC7(7);
  PushVertexShaderConstant pushC8(8);

  int fullWidth = _engine->Width();
  int fullHeight = _engine->Height();
  int partWidth = fullWidth / GBSizeDivisor;
  int partHeight = fullHeight / GBSizeDivisor;

  //int modPartH = fullHeight % GBSizeDivisor;
  //partHeight += modPartH / (GBSizeDivisor / 2);

  float invDiv = 1.0f / GBSizeDivisor;
  // blur direction
  float dirX = 2;
  float dirY = 1;
  // normalize to length  (0.5 / 100 / halfSteps)
  float d = 0.00091f / sqrtf( dirX * dirX + dirY * dirY );
  dirX *= d; dirY *= d;
  //
  float blurLim = 100 * 10.0f / fullWidth;
  // blur coef for blend phase
  float blur0 = pars._blurriness / blurLim;
  if( blur0 > 1 )
    blur0 = 1;
  // blur coef for blur phase
  float blur1 = pars._blurriness - blurLim;
  if( blur1 < 0 )
    blur1 = 0;
  dirX *= blur1; dirY *= blur1;

  float aspect = (float)fullWidth / (float)fullHeight;
  ComRef<IDirect3DSurface9> zeroDepth;

  // pixel offsets
  float ks = 0.5f;
  float ou = ks / fullWidth;
  float ov = ks / fullHeight;

  ////////////////////////////////////////////////
  // GAUSS BLUR PASSES
  if( blur1 > 0 )
  {
    const int kernelSize = 6;
    float weights[kernelSize][4];
    float suma = 0;
    for( int i = 0; i < kernelSize; i++ )
    {
      weights[i][0] = ComputeGaussianValue( (float)i, 0, 2.8f );
      if( i > 0 )
        suma += weights[i][0] * 2;
      else
        suma += weights[i][0];
    }
    // normalize kernel weights
    float divider = 1.0f/suma; //1.0f/660;
    for( int i = 0; i < kernelSize; i++ )
      weights[i][0] *= divider;
    
    // Set the stream source
    _engine->SetStreamSource( cb, res->_vb, sizeof(VertexP3T2), res->_vertexCount );
    // Set the vertex declaration
    _engine->SetVertexDeclaration(cb, res->_vd);
    // Set the vertex shader
    _engine->SetVertexShader(cb, res->_vsBlur);
    // Set the pixel shader
    _engine->SetPixelShader(cb, res->_psBlur);

    ////////////////////////////////////////////////
    // BLUR PASS 1
    {
      //_engine->SetTemporaryRenderTarget(_rtHorizontal, _rtsHorizontal, zeroDepth);
      _engine->SetTemporaryRenderTarget( _ppGaussianBlur->GetFirstRtTexture(), _ppGaussianBlur->GetFirstRt(), zeroDepth );
      // set viewport
      {
        D3DVIEWPORT9 viewData;
        memset(&viewData,0,sizeof(viewData));
        viewData.X = 0;
        viewData.Y = 0;
        viewData.Width  = partWidth;
        viewData.Height = partHeight;
        viewData.MinZ = 0.0f;
        viewData.MaxZ = 1.0f;
        CALL_D3D_MEMBER(_engine,SetViewport,viewData);
      }

      // Set temporary render states
      SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
      // Use SRGB conversion in texture read
      SetSamplerStateTemp ssSRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
      // Write in the same space as you read // _VBS3_TI
      SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE, _engine->_caps._rtIsSRGB);

      // source texture is second RT of PPGaussianBlur
      SetTextureTemp tTex0( 0, _ppGaussianBlur->GetBlurredTexture() );
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

      // set shader params
      SetVertexShaderConstantTemp svc0( 0, D3DXVECTOR4( blur0, 1, 1, 0 ) ); // blurriness, 
      SetVertexShaderConstantTemp svc1( 1, D3DXVECTOR4( dirX, dirY * aspect, ou*GBSizeDivisor, ov*GBSizeDivisor) );
      _engine->SetPixelShaderConstantF( cb, 2, *weights, kernelSize );

			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Pass 1: ");
	      _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }
    //////////////////////////////////////////////
     //PASS 2
    {
      // render to second RT of PPGaussianBlur
      _engine->SetRenderTargetAndDepthStencil( _ppGaussianBlur->GetSecondRt(), _ppGaussianBlur->GetBlurredTexture(), zeroDepth );
      // source texture is first RT of PPGaussianBlur
      SetTextureTemp tTex0( 0, _ppGaussianBlur->GetFirstRtTexture() );
      SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      // set shader params
      SetVertexShaderConstantTemp svc0( 0, D3DXVECTOR4( blur0, invDiv, invDiv - 0.001f, 0 ) );
      SetVertexShaderConstantTemp svc1( 1, D3DXVECTOR4( -dirY, -dirX * aspect * invDiv, ou * GBSizeDivisor, ov) ); //  * invDiv
      _engine->SetPixelShaderConstantF( cb, 2, *weights, kernelSize );

			GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Pass2: ");
	      _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, 0, 2);
			GPU_END_PROFILE
    }		
     // Restore the original render target and depth stencil buffer
    _engine->RestoreOldRenderTarget();
  }
  // FINAL PASS
  // pixel offsets
  ks = 0.34f;
  ou = ks / fullWidth;
  ov = ks / fullHeight;

  // Set the stream source
  _engine->SetStreamSource(cb, res->_vb, sizeof(VertexP3T2), res->_vertexCount);
  // Set the vertex declaration
  _engine->SetVertexDeclaration(cb, res->_vd);
  // Set the vertex shader
  _engine->SetVertexShader(cb, res->_vsFinal);
  // Set the pixel shader
  _engine->SetPixelShader(cb, res->_psFinal);

  // Set temporary render states
  SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // Stage 0 - base texture - do the copy
  IDirect3DTexture9 *rt = _engine->CreateRenderTargetCopyAsTexture();

  SetTextureTemp tTex0( 0, rt );
  SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  // if effect is last then switch to back buffer
  if (isLast) _engine->SwitchToBackBufferRenderTarget();
  
  // Stage 1 - blurred texture
  SetTextureTemp tTex1(1, _ppGaussianBlur->GetBlurredTexture());
  SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE, _engine->_caps._rtIsSRGB);
  SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  /*SetVertexShaderConstantTemp svc0( 0, D3DXVECTOR4( blur0, 1, ou, ov ) ); // blurriness, 
  SetVertexShaderConstantTemp svc1( 1, D3DXVECTOR4( dirX * 0.5, dirY * 0.5 * aspect, ou, ov) ); // blurriness,*/
  SetVertexShaderConstantTemp svc0( 0, D3DXVECTOR4( blur0, aspect, ou, ov ) ); // blurriness, 
  SetVertexShaderConstantTemp svc1( 1, D3DXVECTOR4( dirX * 0.5, dirY * 0.5, ou*GBSizeDivisor, ov*GBSizeDivisor ) ); // blurriness,*/

  
	GPU_START_PROFILE(_engine->GetDirect3DDevice()->GetRefForCB(), "  Combine: ");
		_engine->DrawPrimitive( cb, D3DPT_TRIANGLESTRIP, 0, res->_stripSize );
	GPU_END_PROFILE

	 // If scene is not being rendered finish initialized scene
  if (!inScene) _engine->D3DEndScene();
		
	GPU_END_PROFILE

  // Restore usual rendering settings
  Finish();
}

//-----------------------------------------------------------------------------
LSError EngineDD9::PPDynamicBlur::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("enable", _enabled, 0, false));
  CHECK(ar.Serialize("pInterp",_pInterp,0));
  CHECK(ar.Serialize("prior", _priority, 0, -1));

  return LSOK;
}

//-----------------------------------------------------------------------------
void EngineDD9::ISpecialEffect::DestroyHndls()
{
  // suppressed assertion: ~Engine than _pp._effects are destroyed before ~ISpecialEffect - int hat case we try to 
  // destroy post effects which are destroyed yet
  if (_engine && _engine->_pp._effects.Size() > 0)
  {
    if (_handlesValid)
    {
      for (int i = 0; i< _hndls.Size(); i++)
      {
        if (_hndls[i] != InvalidPPHndl._hndl) _engine->DestroyPostprocess(_hndls[i]);
      }
    }
    Reset();
  }
  else
  {
    Reset();
  }
}

//-----------------------------------------------------------------------------
void EngineDD9::ISpecialEffect::Reset()
{
  _hndls.Clear();
  _handlesValid = false;
  _active = false;
  _timeStamp = Time(0);
  _lastUpdate = Time(0);
}

//-----------------------------------------------------------------------------
LSError EngineDD9::ISpecialEffect::Serialize(ParamArchive &ar)
{ 
  CHECK(ar.Serialize("active", _active, 0));
  CHECK(ar.Serialize("valid", _handlesValid, 0));
  CHECK(ar.Serialize("hndls", _hndls, 0));
  CHECK(::Serialize(ar, "timeStamp", _timeStamp, 0, Time(0)));
  CHECK(::Serialize(ar, "lastTime", _lastUpdate, 0, Time(0)));

  return LSOK; 
}

//-----------------------------------------------------------------------------
bool EngineDD9::HitSpecEffect::Init()
{
  Assert(!_handlesValid);

  _hndls.Realloc(1);
  _hndls.Resize(1);
  
  _hndls[0] = _engine->CreatePostprocess(PEDynamicBlur, DBPriority);

  _handlesValid = (_hndls[0] != InvalidPPHndl._hndl);

  _timeStamp = Glob.time;
  _active = false;

  return _handlesValid;
}

//-----------------------------------------------------------------------------
void EngineDD9::HitSpecEffect::Update()
{
  if (_active && (_timeStamp + 2 < Glob.time) && _hndls.Size() == 1)
  {
    _engine->EnablePostprocess(_hndls[0], false);    
    _active = false;
  }
}

//-----------------------------------------------------------------------------
void EngineDD9::HitSpecEffect::SetParams(const float *pars, int nPars)
{
  if (!_handlesValid || _hndls.Size() != 1) return;
  
  const static float fadeOut = 1.25f;

  AutoArray<float> locPars;
  locPars.Realloc(1);    
  locPars.Resize(1);
  locPars[0] = pars[0];

  _engine->EnablePostprocess(_hndls[0], true);
  _engine->SetPostprocessParams(_hndls[0], locPars);
  _engine->CommitPostprocess(_hndls[0], 0);

  locPars[0] = 0;

  _engine->SetPostprocessParams(_hndls[0], locPars);
  _engine->CommitPostprocess(_hndls[0], fadeOut);

  _timeStamp = Glob.time;
  _active = true;
}

//-----------------------------------------------------------------------------
bool EngineDD9::MovementSpecEffect::Init()
{
  Assert(!_handlesValid);

  _hndls.Realloc(1);
  _hndls.Resize(1);
  _hndls[0] = _engine->CreatePostprocess(PERadialBlur, RBPriority);

  _pars.Realloc(4);
  _pars.Resize(4);

  _pars[0] = 0; _pars[1] = 0; _pars[2] = 0.25; _pars[3] = 0.25;

  _handlesValid = (_hndls[0] != InvalidPPHndl._hndl);

  _timeStamp = Time(0) - 0.2;
  _active = false;

  return _handlesValid;
}

//-----------------------------------------------------------------------------
bool EngineDD9::TiredSpecEffect::Init()
{
  Assert(!_handlesValid);

  _hndls.Realloc(2);
  _hndls.Resize(2);

  // create post effects
  _hndls[0] = _engine->CreatePostprocess(PEColorCorrections, CCPriority); 
  _hndls[1] = _engine->CreatePostprocess(PEWetDistortion, WPriority);

  _handlesValid = (_hndls[0] != InvalidPPHndl._hndl && _hndls[1] != InvalidPPHndl._hndl);
  _timeStamp = Time(0);
  _lastUpdate = Time(0);
  _active = false;

  _pars.Realloc(15);
  _pars.Resize(15);
  // color correction pars
  _pars[0] = 1; _pars[1] = 1; _pars[2] = 0;
  _pars[3] = 0; _pars[4] = 0; _pars[5] = 0; _pars[6] = 0;
  _pars[7] = 0.9; _pars[8] = 0.8; _pars[9] = 0.7; _pars[10] = 0.75;
  _pars[11] = 0.199f; _pars[12] = 0.587f; _pars[13] = 0.144f; _pars[14] = 0;

  _parsCa.Realloc(15);
  _parsCa.Resize(15);
  // wet distortion
  _parsCa[0] = 0.1; _parsCa[1] = 0.15;_parsCa[2] = 0;
  _parsCa[3] = 2.1; _parsCa[4] = 3.7; _parsCa[5] = 2.5; _parsCa[6] = 1.85; 
  _parsCa[7] = 0.0051; _parsCa[8] = 0.0051; _parsCa[9] = 0.0051; _parsCa[10] = 0.0051; 
  _parsCa[11] = 0.5; _parsCa[12] = 0.3; _parsCa[13] = 10; _parsCa[14] = 4;

  return _handlesValid;
}

//-----------------------------------------------------------------------------
void EngineDD9::TiredSpecEffect::SetParams(const float *pars, int nPars)
{
  if (!_handlesValid || _hndls.Size() != 2) return;

  static const float tiredLB = 0.25f;
  static const float tiredUB = 1.0f;

  // low bound for applying effect
  if (pars[0] > tiredLB)
  {
    // update pars each 100ms
    if (_timeStamp < Glob.time)
    {
      static const float delay = 0.10;

      _pars[1] = InterpolativC(pars[0], tiredLB, tiredUB, 1, 1.5);   // 1 -> 1.5
      _pars[7] = InterpolativC(pars[0], tiredLB, tiredUB, 1, 0.9);   // 1 -> 0.9
      _pars[8] = InterpolativC(pars[0], tiredLB, tiredUB, 1, 0.8);   // 1 -> 0.8
      _pars[9] = InterpolativC(pars[0], tiredLB, tiredUB, 1, 0.7);   // 1 -> 0.7
      _pars[10] = InterpolativC(pars[0], tiredLB, tiredUB, 1, 0.75); // 1 -> 0.75

      _parsCa[0] = InterpolativC(pars[0], tiredLB, tiredUB, 0, 0.1);
      _parsCa[1] = InterpolativC(pars[0], tiredLB, tiredUB, 0, 0.15);

      _engine->SetPostprocessParams(_hndls[0], _pars);
      _engine->SetPostprocessParams(_hndls[1], _parsCa);

      _engine->CommitPostprocess(_hndls[0], 0);
      _engine->CommitPostprocess(_hndls[1], 0);

      _engine->EnablePostprocess(_hndls[0], true);
      _engine->EnablePostprocess(_hndls[1], true);

      _timeStamp = Glob.time + delay;
      _lastUpdate = Glob.time + 2*delay;

      _active = true;
    }
  }
  else
  {
    if (_active)
    {
      _engine->EnablePostprocess(_hndls[0], false);
      _engine->EnablePostprocess(_hndls[1], false);
      _active = false;
    }
  }
}

//-----------------------------------------------------------------------------
void EngineDD9::TiredSpecEffect::Update() 
{
  // when active and not updated then stop PPE
  if (_active && Glob.time > _lastUpdate && _hndls.Size() == 2)
  {
    _engine->EnablePostprocess(_hndls[0], false);
    _engine->EnablePostprocess(_hndls[1], false);
    _active = false;
  }
}

//-----------------------------------------------------------------------------
void EngineDD9::MovementSpecEffect::SetParams(const float *pars, int nPars)
{
  if (!_handlesValid || _hndls.Size() != nPars) return;

  // update 4 times per frame
  if (_lastUpdate < Glob.time)
  {
    static const float speedLowLimit = 3.5f;

    DoAssert(CheckMainThread());

    if (pars[0] > speedLowLimit)
    {
      _pars[0] = InterpolativC(pars[0], speedLowLimit, 10.0f, 0.0f, 0.01f);
      _pars[1] = _pars[0];

      _engine->SetPostprocessParams(_hndls[0], _pars);
      _engine->CommitPostprocess(_hndls[0], 0);
      _engine->EnablePostprocess(_hndls[0], true);

      static const float NextUpdate = 0.25f;
      // next update
      _lastUpdate = Glob.time + NextUpdate;
      _timeStamp = Glob.time;

      _active = true;
    }
  }
}

//-----------------------------------------------------------------------------
void EngineDD9::MovementSpecEffect::Update() 
{
  DoAssert(CheckMainThread());

  if (_active)
  {
    if ((_timeStamp + 1.0f) < Glob.time)
      _engine->EnablePostprocess(_hndls[0], false);
  }
}


//-----------------------------------------------------------------------------
// post process enum
#define POSTFX_TYPE_ENUM(type,prefix,XX) \
  XX(type, prefix, RadialBlur) \
  XX(type, prefix, FilmGrain) \
	XX(type, prefix, GodRays) \
	XX(type, prefix, EnfShColl) \
	XX(type, prefix, Rain) \
  XX(type, prefix, SSAO) \
	XX(type, prefix, DOF) \
  XX(type, prefix, ChromAberration) \
  XX(type, prefix, WetDistortion) \
  XX(type, prefix, ColorCorrections) \
  XX(type, prefix, DynamicBlur) \
  XX(type, prefix, ColorInversion)

#ifndef DECL_ENUM_POSTFX_TYPE
#define DECL_ENUM_POSTFX_TYPE
DECL_ENUM(PostFxType)
#endif
DECLARE_DEFINE_ENUM(PostFxType,PFX,POSTFX_TYPE_ENUM)

#define CHECK_INTERNAL(command) \
  {LSError err = command; if (err != LSOK) return err;}
  
//-----------------------------------------------------------------------------
template <>
LSError Serialize(ParamArchive &ar, const RStringB &name, FourFloats &value, int minVersion)
{
  if (ar.GetArVersion() < minVersion) return LSOK;
  if (ar.IsSaving())
  {
    AUTO_STATIC_ARRAY(float, array, 4);
    array.Resize(4);
    array[0] = value[0];
    array[1] = value[1];
    array[2] = value[2];
    array[3] = value[3];
    CHECK_INTERNAL(ar.SerializeArray(name, array, minVersion))
  }
  else
  {
    if (ar.GetPass() != ParamArchive::PassFirst) return LSOK;
    AUTO_STATIC_ARRAY(float, array, 4);
    CHECK_INTERNAL(ar.SerializeArray(name, array, minVersion))
    if (array.Size() < 4)
      return LSNoEntry; // default value
    Assert(array.Size() == 4);
    value[0] = array[0];
    value[1] = array[1];
    value[2] = array[2];
    value[3] = array[3];
  }
  return LSOK;
}

//-----------------------------------------------------------------------------
/*!
  Enable/disable post process effect. Type of effect is given by the 1.st parameter.
*/
//-----------------------------------------------------------------------------
void EnableSinglePPEffect(PostFxType effectType, bool enable)
{
  switch (effectType)
  {
  case PFXRadialBlur:
    GEngine->EnableRadialBlur(enable);
    break;

  case PFXFilmGrain:
    GEngine->EnableFilmGrain(enable);
    break;

	case PFXGodRays:
		GEngine->EnableGodRays(enable);
		break;

	case PFXRain:
		GEngine->EnableRain(enable);
		break;

  case PFXSSAO:
    GEngine->EnableSSAO(enable);
    break;

  case PFXChromAberration:
    GEngine->EnableChromAberration(enable);
    break;

  case PFXWetDistortion:
    GEngine->EnableWetDistortion(enable);
    break;

  case PFXColorCorrections:
    GEngine->EnableColorCorrections(enable);
    break;

  case PFXDynamicBlur:
    GEngine->EnableDynamicBlur(enable);
    break;

  case PFXColorInversion:
    GEngine->EnableClrInvers(enable);
    break;
  }
}

//-----------------------------------------------------------------------------
/*!
Create post effect and return handle to effect, if creation failed return -1
*/
//-----------------------------------------------------------------------------
int CreatePostprocess(PostFxType type, int priority)
{
  int hndl = InvalidPPHndl._hndl;

  switch (type)
  {
  case PFXRadialBlur:
    hndl = GEngine->CreatePostprocess(PERadialBlur, priority);    
    break;

  case PFXFilmGrain:
    hndl = GEngine->CreatePostprocess(PEFilmGrain, priority);
    break;

	case PFXGodRays:
		hndl = GEngine->CreatePostprocess(PEGodRays, priority);
		break;

	case PFXRain:
		hndl = GEngine->CreatePostprocess(PERain, priority);
		break;

  case PFXSSAO:
    hndl = GEngine->CreatePostprocess(PESSAO, priority);
    break;

  case PFXChromAberration:
    hndl = GEngine->CreatePostprocess(PEChromAberration, priority);
    break;

  case PFXWetDistortion:
    hndl = GEngine->CreatePostprocess(PEWetDistortion, priority);
    break;

  case PFXColorCorrections:
    hndl = GEngine->CreatePostprocess(PEColorCorrections, priority);
    break;

  case PFXDynamicBlur:
    hndl = GEngine->CreatePostprocess(PEDynamicBlur, priority);
    break;

  case PFXColorInversion:
    hndl = GEngine->CreatePostprocess(PEColorInv, priority);
    break;
  }

  return hndl;
}


//-----------------------------------------------------------------------------
int ConvertGameScalarToInt(GameValuePar par)
{
  if (par.GetType() != GameScalar)
  {
    RptF("Invalid post effect parameter type.");
    return InvalidPPHndl._hndl;
  }

  float fHndl = par;
  return toInt(fHndl);
}

//-----------------------------------------------------------------------------
int CreatePPEffectHndl(const GameState *state, GameValuePar oper1)
{
  int hndl = InvalidPPHndl._hndl;

  if (oper1.GetType() == GameArray)
  {
    // array of effect
    const GameArrayType &array = oper1;

    if (array.Size() != 2) 
    {
      state->SetError(EvalDim, array.Size(), 2);
      return hndl; 
    }

    // effect type
    RString effectName = array[0];
    PostFxType effectType = GetEnumValue<PostFxType>(cc_cast(effectName));

    if (effectType == INT_MIN) return hndl;

    int priority = ConvertGameScalarToInt(array[1]);

    return CreatePostprocess(effectType, priority);
  }

  return hndl;
}

//-----------------------------------------------------------------------------
/*! 
Create array of post effects 
*/
//-----------------------------------------------------------------------------
GameValue PPEffectCreateArr(const GameState *state, GameValuePar oper1)
{
  // [hndl1, ..., hndlN] ppEffectCreate [["effect type1", priority1], .. ["effect typeN", priorityN]]
  const GameArrayType &srcArr = oper1;

  GameArrayType destArr;
  destArr.Realloc(srcArr.Size());
  destArr.Resize(srcArr.Size());

  int i = 0;
  for (; i < srcArr.Size(); i++)
  {
    if (srcArr[i].GetType() == GameArray)
    {
      int hndl = CreatePPEffectHndl(state, srcArr[i]);

      // check handle
      if (hndl == InvalidPPHndl._hndl)
      {
        RptF("Post process effect creation failed");
        break;
      }
      destArr[i] = (float)hndl;
    }
  }

  // if not all effects successfully created
  if (i < srcArr.Size())
  {
    // destroy successfully created post processes
    for (int j = 0; j < i; j++)
    {
      int hndl = ConvertGameScalarToInt(destArr[j]);
      GEngine->DestroyPostprocess(hndl);
    }

    destArr.Clear();

    return NOTHING;
  }

  return destArr;
}

//-----------------------------------------------------------------------------
/*!
 Create post effect 
*/
//-----------------------------------------------------------------------------
GameValue PPEffectCreate(const GameState *state, GameValuePar oper1)
{
  const GameArrayType &srcArr = oper1;

  // ["effect type", priority]
  if (srcArr.Size() == 2 && srcArr[0].GetType() == GameString)
  {
    int hndl = CreatePPEffectHndl(state, oper1);

    if (hndl == InvalidPPHndl._hndl) 
      RptF("Post process effect creation failed");

    return (float)hndl;
  }
  else
  {
    // [["effect type1", priority1], ..., ["effect typeN", priorityN]]
    return PPEffectCreateArr(state, oper1);
  }
}

//-----------------------------------------------------------------------------
/*!
 Destroy effect given by hndl
*/
//-----------------------------------------------------------------------------
GameValue PPEffectDestroy(const GameState *state, GameValuePar oper1)
{
  int hndl = ConvertGameScalarToInt(oper1);

  if (hndl == InvalidPPHndl._hndl) 
  {
    RptF("Invalid post effect handle.");
    return NOTHING;
  }

  GEngine->DestroyPostprocess(hndl);

  return NOTHING;
}

//-----------------------------------------------------------------------------
/*!
Destroy array of given hndls
*/
//-----------------------------------------------------------------------------
GameValue PPEffectDestroyArr(const GameState *state, GameValuePar oper1)
{
  GameArrayType srcArr = oper1;

  for (int i = 0; i < srcArr.Size(); i++)
  {
    PPEffectDestroy(state, srcArr[i]);
  }

  return NOTHING;
}

//-----------------------------------------------------------------------------
/*!
Enable / disable post process effect 
*/
//-----------------------------------------------------------------------------
GameValue PPEffectEnable1(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{  
  int hndl = ConvertGameScalarToInt(oper1);

  if (hndl == InvalidPPHndl._hndl) 
  {
    RptF("Invalid post effect handle.");
    return NOTHING;
  }

  GEngine->EnablePostprocess(hndl, oper2);

  return NOTHING;
}

int GetPPParsLenght(const GameArrayType &srcArr)
{
  int index = 0;
  for (int i = 0; i < srcArr.Size(); i++)
  {
    if (srcArr[i].GetType() == GameBool || srcArr[i].GetType() == GameScalar) 
    { 
      index++; 
    }
    else if (srcArr[i].GetType() == GameArray)
    {
      const GameArrayType &subArray = srcArr[i];
      for (int j = 0; j < subArray.Size(); j++)
      {        
        // array can contain only bool & float
        if (subArray[j].GetType() == GameBool || subArray[j].GetType() == GameScalar) 
        { 
          index++; 
          continue;
        }

        return 0;
      }
    }
    else
      return 0;
  }

  return index;
}

//-----------------------------------------------------------------------------
/*!
Adjust post effect params 
*/
//-----------------------------------------------------------------------------
GameValue PPEffectAdjust1(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  int hndl = ConvertGameScalarToInt(oper1);

  if (hndl == InvalidPPHndl._hndl) 
  {
    RptF("Invalid post effect handle.");
    return NOTHING;
  }

  const GameArrayType &pars = oper2;
  
  const int maxSize = 32;
  int length = GetPPParsLenght(pars);
  
  if (length == 0 || length > maxSize)
  {
    RptF("Too many post effect params set");
    state->SetError(EvalDim, pars.Size(), maxSize);
    return NOTHING;
  }

  AutoArray<float, MemAllocLocal<float, maxSize> > locPars;
  locPars.Resize(32);
  
  int index = 0;
  for (int i = 0; i < pars.Size(); i++)
  {
    if (pars[i].GetType() == GameScalar)
    {
      locPars[index++] = pars[i];
    }
    else if (pars[i].GetType() == GameBool)
    {
      bool bValue = pars[i];
      locPars[index++] = bValue ? 1.0f : -1.0f;
    }
    else if (pars[i].GetType() == GameArray)
    {
      const GameArrayType &subArray = pars[i];

      for (int j = 0; j < subArray.Size(); j++)
      {
        if (subArray[j].GetType() == GameScalar)
          locPars[index++] = subArray[j];
        else
        {
          state->TypeError(GameScalar, subArray[i].GetType());
          return NOTHING;
        }
      }
    }
    else
    {
      state->TypeError(GameScalar, pars[i].GetType());
      return NOTHING;
    }
  }

  // if return != 0 then number of params is incorrect
  int parsCount = GEngine->SetPostprocessParams(hndl, locPars.Data(), index);
  if (parsCount != 0)
  {
    RptF("Incorrect number of post effect params");
    state->SetError(EvalDim, pars.Size(), parsCount);
  }

  return NOTHING;
}

//-----------------------------------------------------------------------------
/*!
Set time when old pars are replaced by last set, if 0 set immediately 
*/
//-----------------------------------------------------------------------------
GameValue PPEffectCommit1(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  int hndl = ConvertGameScalarToInt(oper1);

  if (hndl == InvalidPPHndl._hndl) 
  {
    RptF("Invalid post effect handle.");
    return NOTHING;
  }

  float time = oper2;
  float comTime = max(0.0f, time);

  GEngine->CommitPostprocess(hndl, comTime);
 
  return NOTHING;
}

//-----------------------------------------------------------------------------
/*
Overloaded version of PPEffectCommit1 
*/
//-----------------------------------------------------------------------------
GameValue PPEffectCommitArr(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  // [hndl1, ..., hndlN] ppEffectCommit time
  GameArrayType srcArr = oper1;

  for (int i = 0; i < srcArr.Size(); i++)
  {
    PPEffectCommit1(state, srcArr[i], oper2);
  }

  return NOTHING;
}

//-----------------------------------------------------------------------------
/*!
check if the specified post process effect is committed
*/
//-----------------------------------------------------------------------------
GameValue PPEffectCommitted1(const GameState *state, GameValuePar oper1)
{
  int hndl = ConvertGameScalarToInt(oper1);

  // hndl ppEffectCommit time
  if (hndl == InvalidPPHndl._hndl) 
  {
    RptF("Invalid post effect handle.");
    return true;
  }

  return GEngine->CommittedPostprocess(hndl);
};

//-----------------------------------------------------------------------------
/*!
  Function enable/disable specified post process effect. Function may be called for different oper1 parameters,
  valid parameters are GameString and GameArray.
*/
//-----------------------------------------------------------------------------
GameValue PPEffectEnable(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  // operator type ... valid types are GameString and GameArray
  const GameType &operType = oper1.GetType();

  // enable/disable
  bool enable = oper2;

  // single post process
  if (operType == GameString)
  {
    // effect type
    RString effectName = oper1;
    PostFxType effectType = GetEnumValue<PostFxType>(cc_cast(effectName));

    // is enum valid
    if (effectType == INT_MIN) return NOTHING;

    EnableSinglePPEffect(effectType, enable);

    return NOTHING;
  }

  // enable multiple post processes
  if (operType == GameArray)
  {
    // array of effect
    const GameArrayType &array = oper1;

    for (int i = 0; i < array.Size(); i++)
    {
      // effect specified by string
      if (array[i].GetType() == GameString)
      {
        // effect type
        RString effectName = array[i];
        PostFxType effectType = GetEnumValue<PostFxType>(cc_cast(effectName));

        // unrecognized effect is skip
        if (effectType == INT_MIN) continue;

        EnableSinglePPEffect(effectType, enable);
      }

      // effect specified by handle
      if (array[i].GetType() == GameScalar)
      {
        PPEffectEnable1(state, array[i], oper2);
      }
    }
  }

  return NOTHING;
}

//-----------------------------------------------------------------------------
/*!
  Function set parameters for specified post process effect. Effect is specified by first argument. 
  Second argument represent effect parameters. Parameters number depends on effect type.
*/
//-----------------------------------------------------------------------------
GameValue PPEffectAdjust( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  // effect type
  RString effectName = oper1;
  PostFxType effectType = GetEnumValue<PostFxType>(cc_cast(effectName));
  // is enum valid
  if (effectType == INT_MIN) return NOTHING;

  // array of parameters
  const GameArrayType &array = oper2;

  // set params to specified post process, each post process has different parameters count
  switch (effectType)
  {
  case PFXRadialBlur:
    // params count test
    if (array.Size() != 4)
    {
      state->SetError(EvalDim, array.Size(), 4);
      return NOTHING;
    }

    // param types test
    for (int i = 0; i < 4; i++)
    {
      if (array[i].GetType() != GameScalar)
      {
        state->TypeError(GameScalar, array[i].GetType());
        return NOTHING; 
      }
    }

    // PowerX   - float array[0]
    // PowerY   - float array[1]
    // OffsetX  - float array[2]
    // OffsetY  - float array[3]
    GEngine->SetRadialBlurrParams(array[0], array[1], array[2], array[3]);
    break;

  case PFXWetDistortion:
    // params count test
    if (array.Size() != 15)
    {
      state->SetError(EvalDim, array.Size(), 4);
      return NOTHING;
    }

    // params type test
    for (int i = 0; i < 15; i++)
    {
      if (array[i].GetType() != GameScalar)
      {
        state->TypeError(GameScalar, array[i].GetType());
        return NOTHING; 
      }
    }

    // SetBlurriness  - float value
    // SetEffectPower - float top, float bottom
    // SetWaveSpeeds  - float horizontal1, float horizontal2, float vertical1, float vertical2
    // SetWaveAmplitudes  - float horizontal1, float horizontal2, float vertical1, float vertical2
    // SetPhaseCoefs  - float randX, float randY, float posX, float posY
    GEngine->SetWetDistortionParams(array[0], array[1], array[2], array[3], array[4], array[5], array[6],
      array[7], array[8], array[9], array[10], array[11], array[12], array[13], array[14]);

    break;

  case PFXFilmGrain:
    {
      const int FILMGRAIN_PARAMS = 6;

      // takes max five params
      if (array.Size() > FILMGRAIN_PARAMS || array.Size() < 1)
      {
        state->SetError(EvalDim, array.Size(), FILMGRAIN_PARAMS);
        return NOTHING;
      } 

      float params[FILMGRAIN_PARAMS];

      // params type test
      for (int i = 0; i < array.Size() && i < FILMGRAIN_PARAMS - 1; i++)
      {
        if ((array[i].GetType() != GameScalar))
        {
          state->TypeError(GameScalar, array[i].GetType());
          return NOTHING; 
        }

        params[i] = array[i];
      }

      if (array.Size() == FILMGRAIN_PARAMS)
      {
        if (array[FILMGRAIN_PARAMS - 1].GetType() != GameBool)
        {
          state->TypeError(GameBool, array[FILMGRAIN_PARAMS - 1].GetType());
          return NOTHING; 
        }

        params[FILMGRAIN_PARAMS - 1] = array[FILMGRAIN_PARAMS - 1] ? 0.0f : 1.0f;
      }

      // intensity        - float array[0]
      // sharpness        - float array[1]
      // grainsize        - float array[2]
      // intensity at X0  - float array[3]
      // intensity at X1  - float array[4]
      // monochrom        - bool array[5]
      GEngine->SetFilmGrainParams(params, array.Size());
    }
    break;

	case PFXGodRays:
		{
			const int GODRAYS_PARAMS = 7;

			if (array.Size() > GODRAYS_PARAMS || array.Size() < 1)
			{
				state->SetError(EvalDim, array.Size(), GODRAYS_PARAMS);
				return NOTHING;
			} 

			float params[GODRAYS_PARAMS];

			// params type test
			for (int i = 0; i < array.Size() && i < GODRAYS_PARAMS - 1; i++)
			{
				if ((array[i].GetType() != GameScalar))
				{
					state->TypeError(GameScalar, array[i].GetType());
					return NOTHING; 
				}

				params[i] = array[i];
			}

			GEngine->SetGodRaysParams(params, array.Size());
		}
		break;

	case PFXRain:
		{
			const int RAIN_PARAMS = 15;

			if (array.Size() > RAIN_PARAMS || array.Size() < 1)
			{
				state->SetError(EvalDim, array.Size(), RAIN_PARAMS);
				return NOTHING;
			} 

			float params[RAIN_PARAMS];

			// params type test
			for (int i = 0; i < array.Size() && i < RAIN_PARAMS - 1; i++)
			{
				if ((array[i].GetType() != GameScalar))
				{
					state->TypeError(GameScalar, array[i].GetType());
					return NOTHING; 
				}

				params[i] = array[i];
			}

			GEngine->SetRainParams(params, array.Size());
		}
		break;



  case PFXSSAO:
    {
      const int SSAO_PARAMS = 12;

      if (array.Size() > SSAO_PARAMS || array.Size() < 1)
      {
        state->SetError(EvalDim, array.Size(), SSAO_PARAMS);
        return NOTHING;
      } 

      float params[SSAO_PARAMS];

      // params type test
      for (int i = 0; i < array.Size() && i < SSAO_PARAMS- 2; i++)
      {
        if ((array[i].GetType() != GameScalar))
        {
          state->TypeError(GameScalar, array[i].GetType());
          return NOTHING; 
        }

        params[i] = array[i];
      }

      //bool
      if (array.Size() >= 10)
      {
        for (int i = 10; i < array.Size(); i++)
        {
          if (array[i].GetType() != GameBool)
          {
           state->TypeError(GameBool, array[i].GetType());
           return NOTHING; 
          }

          params[i] = array[i] ? 0.0f : 1.0f;
        }
      }

      GEngine->SetSSAOParams(params, array.Size());
    }

    break;

  case PFXChromAberration:
    // takes three params
    if (array.Size() != 3)
    {
      state->SetError(EvalDim, array.Size(), 3);
      return NOTHING;
    } 

    // params type test
    if ((array[0].GetType() != GameScalar) || (array[1].GetType() != GameScalar))
    {
      state->TypeError(GameScalar, array[0].GetType());
      return NOTHING; 
    }

    if (array[2].GetType() != GameBool)
    {
      state->TypeError(GameScalar, array[2].GetType());
      return NOTHING; 
    }

    // aberationPowerX - float array[0]
    // aberationPowerY - float array[1]
    // aspectCorrection - bool array[2]
    GEngine->SetChromaticAberrationParams(array[0], array[1], array[2]);
    break;

  case PFXColorInversion:
    {
      const int reqSize = 3;

      if (array.Size() != reqSize)
      {
        state->SetError(EvalDim, array.Size(), reqSize);
        return NOTHING;
      }

      for (int i = 0; i < reqSize; i++)
      {
        // params type test
        if ((array[i].GetType() != GameScalar))
        {
          state->TypeError(GameScalar, array[i].GetType());
          return NOTHING; 
        }
      }

      Color invColor(array[0], array[1], array[2], 0);
      GEngine->SetClrInversParams(invColor);
    }
    break;

  case PFXDynamicBlur:
    // pars count test
    if (array.Size() != 1)
    {
      state->SetError(EvalDim, array.Size(), 1);
      return NOTHING;
    } 

    // type test 
    if (array[0].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, array[0].GetType());
      return NOTHING; 
    }

    // blurrines - float array[0]
    GEngine->SetDynamicBlurParameters(array[0]);
    break;

  case PFXColorCorrections:
    // three floats and three colors
    if (array.Size() != 6)
    {
      state->SetError(EvalDim, array.Size(), 6);
      return NOTHING;
    }

    // first three param types test
    for (int i = 0; i < 3; i++)
    {
      if (array[i].GetType() != GameScalar)
      {
        state->TypeError(GameScalar, array[i].GetType());
        return NOTHING; 
      }
    }

    // test color parameters, color is set as array of four floats
    for (int i = 3; i < 6; i++)
    {
      if (array[i].GetType() != GameArray)
      {
        state->TypeError(GameArray, array[i].GetType());
        return NOTHING;         
      }

      // array of color components
      const GameArrayType &clrArray = array[i];
      
      // color consist from 4 floats
      if (clrArray.Size() != 4)
      {
        state->SetError(EvalDim, clrArray.Size(), 4);
        return NOTHING;
      }

      for (int j = 0; j < 4; j++)
      {
        if (clrArray[j].GetType() != GameScalar)
        {
          state->TypeError(GameScalar, clrArray[j].GetType());
          return NOTHING; 
        }
      }
    }

    const GameArrayType &tmp = array[3];
    Color blendColor(tmp[0], tmp[1], tmp[2], tmp[3]);

    const GameArrayType &tmp1 = array[4];
    Color colorizedColor(tmp1[0], tmp1[1], tmp1[2], tmp1[3]);

    const GameArrayType &tmp2 = array[5];
    Color weightsColor(tmp2[0], tmp2[1], tmp2[2], tmp2[3]);

    // SetBrightness  - float value
    // SetContrast    - float value
    // SetOffset      - float value
    // SetBlendColor  - Color color ... array of 3 x float
    // SetColorizeColor - Color color ... array of 3 x float
    // SetRGBWeights  - Color weights ... array of 3 x float
    GEngine->SetColorCorrectionsParameters(array[0], array[1], array[2], blendColor, colorizedColor, weightsColor);
    break;
  }

  return NOTHING;
}

//-----------------------------------------------------------------------------
/*
  Commit for specified post process effect.
*/
//-----------------------------------------------------------------------------
GameValue PPEffectCommit( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  // effect type
  RString effectName = oper1;
  PostFxType effectType = GetEnumValue<PostFxType>(cc_cast(effectName));

  // is enum valid
  if (effectType == INT_MIN) return NOTHING;

  // positive values
  float time = oper2;
  float comTime = max(0.0f, time);

  switch(effectType)
  {
  case PFXRadialBlur:
      GEngine->CommitRadBlur(comTime);
    break;

  case PFXFilmGrain:
    GEngine->CommitFilmGrain(comTime);
    break;

	case PFXGodRays:
		GEngine->CommitGodRays(comTime);
		break;

	case PFXRain:
		GEngine->CommitRain(comTime);
		break;

  case PFXSSAO:
    GEngine->CommitSSAO(comTime);
    break;

  case PFXChromAberration:
    GEngine->CommitChromAberration(comTime);
    break;

  case PFXWetDistortion:
    GEngine->CommitWetDistortion(comTime);
    break;

  case PFXColorCorrections:
    GEngine->CommitColorCorrections(comTime);
    break;

  case PFXDynamicBlur:
    GEngine->CommitDynamicBlur(comTime);
    break;

  case PFXColorInversion:
    GEngine->CommitClrInvers(comTime);
    break;
  }

  return NOTHING;
}

//-----------------------------------------------------------------------------
GameValue PPRainForce (const GameState *state, GameValuePar oper1)
{
	float density = oper1;
	GLandscape->SetRain(density, 0.0f, true);

	return NOTHING;
}

//-----------------------------------------------------------------------------
GameValue PPEffectReload(const GameState *state, GameValuePar oper1)
{
	// effect type
	RString effectName = oper1;
	PostFxType effectType = GetEnumValue<PostFxType>(cc_cast(effectName));

	switch (effectType)
	{
		case PFXDOF:
			GEngine->ReloadDOF();
			return true;

		case PFXSSAO:
			GEngine->ReloadSSAO();
			return true;

		case PFXGodRays:
			GEngine->ReloadGodRays();
			return true;

		case PFXRain:
			GEngine->ReloadRain();
			return true;

		case PFXEnfShColl:
			GEngine->ReloadEnfShapeCollectionRes();
			return true;

		case PFXDynamicBlur:
			GEngine->ReloadRotBlur();
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
/*!
check if the specified post process effect is committed
*/
//-----------------------------------------------------------------------------
GameValue PPEffectCommitted(const GameState *state, GameValuePar oper1)
{
  // effect type
  RString effectName = oper1;
  PostFxType effectType = GetEnumValue<PostFxType>(cc_cast(effectName));

  switch (effectType)
  {
  case PFXRadialBlur:
    return GEngine->CommittedRadBlur();
		break;
  case PFXFilmGrain:
    return GEngine->CommittedFilmGrain();
    break;
	case PFXGodRays:
		return GEngine->CommittedGodRays();
		break;
	case PFXRain:
		return GEngine->CommittedRain();
		break;
	case PFXSSAO:
    return GEngine->CommittedSSAO();
    break;
  case PFXChromAberration:
    return GEngine->CommittedChromAberration();
    break;
  case PFXWetDistortion:
    return GEngine->CommittedWetDistortion();
    break;
  case PFXColorCorrections:
    return GEngine->CommittedColorCorrections();
    break;
  case PFXDynamicBlur:
    return GEngine->CommittedDynamicBlur();
    break;
  case PFXColorInversion:
    return GEngine->CommittedClrInvers();
    break;
  }

  return true;
}

#else
#include <El/Evaluator/express.hpp>
#include <El/Modules/modules.hpp>

#define EMPTY_PP_IMPLEMENTATION {return NOTHING;}

GameValue PPEffectReload(const GameState *state, GameValuePar oper1) EMPTY_PP_IMPLEMENTATION
GameValue PPRainForce (const GameState *state, GameValuePar oper1)  EMPTY_PP_IMPLEMENTATION
GameValue PPEffectCommitted(const GameState *state, GameValuePar oper1) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectCommitted1(const GameState *state, GameValuePar oper1) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectCreate(const GameState *state, GameValuePar oper1) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectDestroy(const GameState *state, GameValuePar oper1) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectDestroyArr(const GameState *state, GameValuePar oper1) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectEnable(const GameState *state, GameValuePar oper1, GameValuePar oper2) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectEnable1(const GameState *state, GameValuePar oper1, GameValuePar oper2) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectAdjust( const GameState *state, GameValuePar oper1, GameValuePar oper2 ) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectAdjust1(const GameState *state, GameValuePar oper1, GameValuePar oper2) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectCommit( const GameState *state, GameValuePar oper1, GameValuePar oper2 ) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectCommit1(const GameState *state, GameValuePar oper1, GameValuePar oper2) EMPTY_PP_IMPLEMENTATION
GameValue PPEffectCommitArr(const GameState *state, GameValuePar oper1, GameValuePar oper2) EMPTY_PP_IMPLEMENTATION
GameValue SetToneMapping(const GameState *state, GameValuePar oper1) EMPTY_PP_IMPLEMENTATION
GameValue SetToneMappingParams(const GameState *state, GameValuePar oper1, GameValuePar oper2) EMPTY_PP_IMPLEMENTATION

#endif

const GameType GameScalarOrArray(GameScalar | GameArray);
// helper only for template instantiation
extern GameTypeType GTypeGameScalarOrArray;

template<>
class ConvertTypes<&GTypeGameScalarOrArray> : public ConvertTypes<&GTypeGameAny> {};

#define FUNCTIONS_VISUAL(XX, Category) \
	XX(GameNothing, "ppRainForce",		PPRainForce, GameScalar, "intensity", "Force rain with user defined intensity (0 to force disable)", "", "", "5501", "", Category) \
  XX(GameNothing, "ppEffectReload",		PPEffectReload, GameString, "effect", "Reload given effect resources", "", "", "5501", "", Category) \
  XX(GameBool, "ppEffectCommitted", PPEffectCommitted, GameString, "effect", "Check whether given post process effect is commited", "", "", "5501", "", Category) \
  XX(GameBool, "ppEffectCommitted", PPEffectCommitted1, GameScalar, "effect", "Check whether given post process effect is commited", "", "", "5501", "", Category) \
  XX(GameScalarOrArray, "ppEffectCreate", PPEffectCreate, GameArray, "effect", "Create post process effect specified by name and priority", "", "", "5501", "", Category) \
  XX(GameNothing, "ppEffectDestroy", PPEffectDestroy, GameScalar, "effect", "Destroy post process effect given by handle", "", "", "5501", "", Category) \
  XX(GameNothing, "ppEffectDestroy", PPEffectDestroyArr, GameArray, "effect", "Create post process effect specified by name and priority", "", "", "5501", "", Category) \
  XX(GameNothing, "setToneMapping", SetToneMapping, GameString, "string", "Set tone mapping method", "setToneMapping \"filmic\"", "", "", "", Category)

FUNCTIONS_VISUAL(JNI_FUNCTION, "Visual")

static const GameFunction PPUnary[]=
{
  FUNCTIONS_VISUAL(REGISTER_FUNCTION, "Visual")
};

#define OPERATORS_VISUAL(XX, Category) \
  XX(GameNothing, "ppEffectEnable", function, PPEffectEnable, GameString, GameBool, "string", "bool", "Enable/disable specified post process effect, is overloaded form multiple effects.", "\"radialBlurr\" ppEffectEnable true, [\"chromAberration\", \"radialBlurr\"] ppEffectEnable true", "", "2.92", "", Category) \
  XX(GameNothing, "ppEffectEnable", function, PPEffectEnable, GameArray, GameBool, "array", "bool", "Enable/disable specified post process effect, is overloaded form multiple effects.", "\"radialBlurr\" ppEffectEnable true, [\"chromAberration\", \"radialBlurr\"] ppEffectEnable true", "", "2.92", "", Category) \
  XX(GameNothing, "ppEffectEnable", function, PPEffectEnable1, GameScalar, GameBool, "number", "bool", "Enable / disable post process effect", "hndl ppEffectEnable true", "", "2.92", "", Category) \
  XX(GameNothing, "ppEffectAdjust", function, PPEffectAdjust, GameString, GameArray, "string", "array", "Adjust parameters for specified post process effect", "\"radialBlurr\" ppEffectEnable []", "", "2.92", "", Category) \
  XX(GameNothing, "ppEffectAdjust", function, PPEffectAdjust1, GameScalar, GameArray, "number", "array", "Set post process effect parameters", "hndl ppEffectAdjust [0.0075, 0.0075, 0.1, 0.1]", "", "2.92", "", Category) \
  XX(GameNothing, "ppEffectCommit", function, PPEffectCommit, GameString, GameScalar, "string", "number", "Set time when old pars are replaced by last set, if 0 set immediately", "...", "", "2.92", "", Category) \
  XX(GameNothing, "ppEffectCommit", function, PPEffectCommit1, GameScalar, GameScalar, "number", "number", "Set time when old pars are replaced by last set, if 0 set immediately", "hndl ppEffectCommit 0", "", "2.92", "", Category) \
  XX(GameNothing, "ppEffectCommit", function, PPEffectCommitArr, GameArray, GameScalar, "array", "scalar", "Set time when old pars are replaced by last set, if 0 set immediately", "hndl ppEffectCommit 0", "", "2.92", "", Category) \
  XX(GameNothing, "setToneMappingParams", function, SetToneMappingParams, GameString, GameArray, "string", "arary", "Set tone mapping parameters", "\"filmic\" setToneMappingParams [0.2, 0.30, 0.20, 0.30, 0.01, 3.750, 6, 4]", "", "", "", Category) 

OPERATORS_VISUAL(JNI_OPERATOR, "Visual")

static const GameOperator PPBinary[]=
{
  OPERATORS_VISUAL(REGISTER_OPERATOR, "Visual")
};

#if DOCUMENT_COMREF
static ComRefFunc PPComRefFunc[] =
{
  FUNCTIONS_VISUAL(COMREF_FUNCTION, "Visual")
  OPERATORS_VISUAL(COMREF_OPERATOR, "Visual")
};
#endif

INIT_MODULE(PostProcess, 2)
{
  GGameState.NewFunctions(PPUnary, lenof(PPUnary));
  GGameState.NewOperators(PPBinary, lenof(PPBinary));
#if DOCUMENT_COMREF
  for (int i=0; i<lenof(PPComRefFunc); i++)
  {
    GGameState.AddComRefFunction(PPComRefFunc[i]);
  }
#endif
};

