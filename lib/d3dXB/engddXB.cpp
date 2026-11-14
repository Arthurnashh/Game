#include "../wpch.hpp"

#if !_DISABLE_GUI || defined _XBOX
#include "engddXB.hpp"
#include "txtd3dXB.hpp"

#include "../winpch.hpp"
#include "../Shape/poly.hpp"
#include "../global.hpp"
#include "../packFiles.hpp"

#include <Es/Files/filenames.hpp>
#include <Es/Strings/bstring.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/Common/perfProf.hpp>

#include "../lights.hpp"

#ifndef _XBOX
	#include <dxerr8.h>
#endif

#include <El/Debugging/debugTrap.hpp>

void ParseFlashpointCfg(ParamFile &file);

HWND AppInit
(
	HINSTANCE hInst,HINSTANCE hPrev,int sw, bool fullscreen,
	int x, int y, int width, int height
);

void DDErrorXB( const char *text, int err )
{
	#ifndef _XBOX
	LogF("%s:%s",text,DXGetErrorString8(err));
	#else
	LogF("%s:%x",text,err);
	#endif
	if (GDebugger.IsDebugger())
	{
		//BREAK();
	}
}

#define FONT_SIZE 16

bool ParseUserParams(ParamFile &cfg);
void SaveUserParams(ParamFile &cfg);

// constructor - attach  a engine to given backBuffer

void EngineDDXB::InitSurfaces()
{
  ParamFile cfg;
  ParseUserParams(cfg);
  ParamFile fcfg;
  ParseFlashpointCfg(fcfg);
  LoadConfig(cfg,fcfg);

	Init3D();
	DoSetGamma();

	// direct-color mode required
	D3DConstruct(cfg,fcfg);
}

// destroy engine
void EngineDDXB::DestroySurfaces()
{
  Done3D();
	_vBufferLast = NULL; // last SetStreamSource
	_iBufferLast = NULL; // last SetIndices

	//_zBuffer.Free();
	_backBuffer.Free();
	_d3DDevice.Free();

}

EngineDDXB::~EngineDDXB()
{
  ParamFile cfg;
  if (ParseUserParams(cfg))
  {
	  SaveConfig(cfg);
    SaveUserParams(cfg);
  }

	D3DDestruct();
	DestroySurfaces();

	_direct3D.Free();
  Assert(_iBuffersAllocated==0);
  Assert(_vBuffersAllocated==0);
  Assert(_vBuffersUsed==0);
}

void EngineDDXB::DoSetGamma()
{
	D3DGAMMARAMP ramp;
	// prepare gamma ramp
	int table[256];

	table[0]=0;
	float eGamma=1/_gamma;
	for( int i=1; i<256; i++ )
	{
		float x=i*(1/255.0);
		float fx=pow(x,eGamma);
		#ifdef _XBOX
			int ifx=toInt(fx*255.0);
			saturate(ifx,0,255);
		#else
			int ifx=toInt(fx*65535.0);
			saturate(ifx,0,65535);
		#endif
		table[i]=ifx;
	}

	for( int i=0; i<256; i++ )
	{
		ramp.red[i]=ramp.green[i]=ramp.blue[i]=table[i];
	}
	_d3DDevice->SetGammaRamp(0,&ramp);
	LogF("set gamma %.3f",_gamma);
}

void EngineDDXB::SetGamma( float gamma )
{
	saturate(gamma,1e-3,1e3);
	_gamma=gamma;
	if (_d3DDevice)
	{
		DoSetGamma();
	}

}

template <class Type>
static Type LoadCfgDef(ParamEntryPar cfg, const char *name, Type def)
{
	ConstParamEntryPtr entry = cfg.FindEntry(name);
	if (!entry) return def;
	return *entry;
}


void EngineDDXB::LoadConfig(ParamEntryPar cfg,ParamEntryPar fcfg)
{
	if (!_windowed)
	{
		_w = LoadCfgDef(fcfg,"Resolution_W",_w);
		_h = LoadCfgDef(fcfg,"Resolution_H",_h);
		_pixelSize = LoadCfgDef(fcfg,"Resolution_Bpp",_pixelSize);
		_refreshRate = LoadCfgDef(fcfg,"refresh",_refreshRate);
	}
	{
		// user config
		SetGamma(LoadCfgDef(cfg,"gamma",GetDefaultGamma()));
		_userEnabledWBuffer = LoadCfgDef(cfg, "useWBuffer", true);
/*
		if (!_windowed)
		{
			_w = LoadCfgDef(cfg,"width",_w);
			_h = LoadCfgDef(cfg,"height",_h);
			_pixelSize = LoadCfgDef(cfg,"bpp",_pixelSize);
			_refreshRate = LoadCfgDef(cfg,"refresh",_refreshRate);
		}
*/
	}

	base::LoadConfig(cfg,fcfg);
}
void EngineDDXB::SaveConfig(ParamFile &cfg)
{
	if (!IsOutOfMemory())
	{
#ifndef _XBOX
		{
			// gloabal config
			ParamFile cfg;
			ParseFlashpointCfg(cfg);

			if (!_windowed)
			{
				cfg.Add("Resolution_W",_w);
				cfg.Add("Resolution_H",_h);
				cfg.Add("Resolution_Bpp",_pixelSize);
				cfg.Add("refresh",_refreshRate);
			}

			FlashpointCfgSave(cfg);
		}
#endif
		{
			// user config
      cfg.Add("gamma",_gamma);
		}
		base::SaveConfig(cfg);
	}
}

void EngineDDXB::Clear( bool clearZ, bool clear, PackedColor color )
{
	D3DRECT rect;
	rect.x1=0,rect.y1=0;
	rect.x2=_w,rect.y2=_h;
	int flags=0;
	if (clear) flags|=D3DCLEAR_TARGET;
	if (clearZ)
	{
		flags|=D3DCLEAR_ZBUFFER;
		if (_hasStencilBuffer) flags|=D3DCLEAR_STENCIL;
	}

	_d3DDevice->Clear(0,NULL,flags,color,1,0);
}

void EngineDDXB::WorkToBack()
{
}


int EngineDDXB::FrameTime() const
{
	return GlobalTickCount()-_frameTime;
}


void EngineDDXB::ReportGRAM(const char *name)
{
	#ifndef _XBOX
	IDirect3DDevice8 *device = GetDirect3DDevice();

	if (name) LogF("VRAM report: %s",name);
	int dwFree = device->GetAvailableTextureMem();
	if (name) LogF("  VRAM free %d",dwFree);
	#endif
}


void EngineDDXB::CreateSurfaces(bool windowed )
{
	_windowed = windowed;

	InitSurfaces();

	// clear all backbuffers
#ifndef _XBOX
	Clear(true,true);
	BackToFront(1);
	Clear(true,true);
	BackToFront(1);
	Clear(true,true);
	BackToFront(1);
#endif

	ReportGRAM("After creating Frame-Buffer");
}

void EngineDDXB::Pause()
{
}

/*!
\patch_internal 1.36 Date 12/12/2001 by Ondra
- Fixed: Improved multimonitor support in windowed mode.
*/

EngineDDXB::EngineDDXB
(
	HINSTANCE hInst, HINSTANCE hPrev, int sw,
	int x, int y, int w, int h, bool windowed, int bpp
)
:_texLoc(TexLocalVidMem),
_flippable(FALSE),
_pixelSize(bpp),
_refreshRate(0),
_bias(0),
_gamma(1.0f/0.85f),
//_sMesh(NULL),
_mesh(NULL),
_textBank(NULL),
_useDXTL(false),
_useHWTL(false),
_userEnabledWBuffer(true),
_can565(false),_can88(false),_can8888(false),
_lastClamp(0),
_hasStencilBuffer(false),
_iBuffersAllocated(0),_vBuffersAllocated(0),
_vBuffersUsed(0),
_iBuffersRequested(0),_vBuffersRequested(0),
_resetNeeded(false),
_lastKnownAvgIntensityAccomFactor(1),
_lastKnownAvgIntensity(-1) // until someone provides valid value, return negative
{
  RegisterFreeOnDemandSystemMemory(this);
  RegisterFreeOnDemandSystemMemory(&_shaderCacheLRU);

	//_formatSet=SingleTex;
	_renderMode = RMTris;
	_enableReorder = false;
	_tlActive = false;
  _visTestCount = 0;

  _texture0MaxColor = HWhite;
  _activePushBuffer = NULL;
  
	CreateD3D();

	// raster parameters
	#ifndef _XBOX
	if (windowed)
	{
		// search for a location that is handled by selected apapted
		//int x=0,y=0;
		SearchMonitor(x,y,w,h);
		_hwndApp=AppInit(hInst,hPrev,sw,FALSE,x,y,w,h);

		RECT client;
		// adjust width, height to match client area
		GetClientRect(_hwndApp,&client);
		_w = client.right-client.left+1;
		_h = client.bottom-client.top+1;
		CreateSurfaces(windowed);
		//_textBank=new TextBank(_engine);
	}
	else
	#endif
	{

		_hwndApp=AppInit(hInst,hPrev,sw,FALSE,0,0,160,160);
		#ifndef _XBOX
		Log("Window created (%x)",_hwndApp);
		if( !_hwndApp ) return;
		#endif
		
		_w = w;
		_h = h;
		CreateSurfaces(windowed);
		//_textBank=new TextBank(_engine);
	}
}


void EngineDDXB::Init()
{
  Engine::Init();

  // Load or generate and save shaders
  if (QFBankQueryFunctions::AutoBank("SCPool\\"))
  {
    _shaderCacheLight.Realloc(_shaderCacheLightId.Size());
    _shaderCache.Realloc(_shaderCacheId.Size());
  }
  else
  {
#if _SUPER_RELEASE
    Fail("Error: File SCPool.pbo not found");
#else
    // Fill out shader caches and save them into file
    LogF("SCPool not found - optimizing vertex shaders");
    {
      InitVertexShaderCache();
      // Save caches
      {
        // Create temporary folder
        const char tempFolder[] = "D:\\_SCPool";
        ::CreateDirectory(tempFolder, NULL);

        // Add Light cache items to the temporary folder
        for (int i = 0; i < _shaderCacheLightId.Size(); i++)
        {
          for (int j = 0; j < SC_LIGHTCOMBINATIONS; j++)
          {
            // Create the filename
            char fileName[256];
            sprintf(fileName, "%s\\lci%d_%d.xvu", tempFolder, i, j);

            // Create file
            QOFStream f(fileName);
            f.write(_shaderCacheLight[i][j]._shader.Data(), _shaderCacheLight[i][j]._shader.Size() * sizeof(DWORD));
          }
        }

        // Add Basic cache items to the temporary folder
        for (int i = 0; i < _shaderCacheId.Size(); i++)
        {
          // Create the filename
          char fileName[256];
          sprintf(fileName, "%s\\bci%d.xvu", tempFolder, i);

          // Create file
          QOFStream f(fileName);
          f.write(_shaderCache[i]._shader.Data(), _shaderCache[i]._shader.Size() * sizeof(DWORD));
        }

        // Create the PBO from the temporary folder
        FileBankManager mgr;
        mgr.Create("D:\\dta\\SCPool.pbo", tempFolder);
      }
    }
#endif
  }

}

bool EngineDDXB::SwitchRes(int w, int h, int bpp)
{
	if (w==_w && h==_h && _pixelSize==bpp) return true;
	// react to window resizing by changing surface dimensions
	_w = w;
	_h = h;
	_pixelSize = bpp;
	D3DDEVICE_CREATION_PARAMETERS pars;
	_d3DDevice->GetCreationParameters(&pars);
	int adapter = pars.AdapterOrdinal;
	FindMode(adapter);
	if (_windowed)
	{
		Reset();
	}
	else
	{
		ResetHard();
	}
	return true;
}

bool EngineDDXB::SwitchWindowed(bool windowed)
{
	if (windowed == _windowed) return true;
	_windowed = windowed;
	_pp.Windowed = windowed;
	Reset();
	return true;
}

bool EngineDDXB::SwitchRefreshRate(int refresh)
{
	if (_pp.Windowed) return false;
	if (refresh==0) return false;
	if (_refreshRate==refresh) return true;
	_refreshRate = refresh;
	ResetHard();
	return true;
}

static int FormatToBpp(D3DFORMAT format)
{
	switch (format)
	{
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			return 32;
		#ifndef _XBOX
		case D3DFMT_R8G8B8:
			return 24;
		#endif
		#ifndef _XBOX
		case D3DFMT_X4R4G4B4:
		#endif
		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_A4R4G4B4:
			return 16;
		#ifndef _XBOX
		case D3DFMT_R3G3B2:
		case D3DFMT_A8R3G3B2:
		#endif
		case D3DFMT_A8:
			return 8;
		default:
			// unknown format
			return 0;
	}

}

void EngineDDXB::ListResolutions(FindArray<ResolutionInfo> &ret)
{
	ret.Clear();
	if (_pp.Windowed)
	{
		// no resolution change possible
		return;
	}
	// enumerate resolutions available on current device
	D3DDEVICE_CREATION_PARAMETERS pars;
	_d3DDevice->GetCreationParameters(&pars);
	int adapter = pars.AdapterOrdinal;
	for (int i=0; i<(int)_direct3D->GetAdapterModeCount(adapter); i++)
	{
		D3DDISPLAYMODE mode;
		_direct3D->EnumAdapterModes(adapter,i,&mode);
		// mode contains resolution and refresh rate
		ResolutionInfo info;
		info.w = mode.Width;
		info.h = mode.Height;
		info.bpp = FormatToBpp(mode.Format);
		if (info.bpp) ret.AddUnique(info);

	}
}

void EngineDDXB::ListRefreshRates(FindArray<int> &ret)
{
  ret.Clear();
	if (_pp.Windowed)
	{
		ret.Add(0); // return default
		// no refresh change possible
		return;
	}
	// list refresh rates for current w,h,bpp
	D3DDEVICE_CREATION_PARAMETERS pars;
	_d3DDevice->GetCreationParameters(&pars);
	int adapter = pars.AdapterOrdinal;
	for (int i=0; i<(int)_direct3D->GetAdapterModeCount(adapter); i++)
	{
		D3DDISPLAYMODE mode;
		_direct3D->EnumAdapterModes(adapter,i,&mode);
		// mode contains resolution and refresh rate
		if
		(
			mode.Width==Width() && mode.Height==Height() &&
			FormatToBpp(mode.Format)==PixelSize()
		)
		{
			ret.AddUnique(mode.RefreshRate);
		}

	}

}

void EngineDDXB::PrepareScreenshot()
{
#ifdef _XBOX
  PROFILE_SCOPE(sShot);

  ComRef<IDirect3DSurface8> front;
  _d3DDevice->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,front.Init());

  if (front.IsNull())
  {
    _screenshot.Free();
    return;
  }

  D3DSURFACE_DESC desc;
  front->GetDesc(&desc);

  // Copy the source into the DXT1 surface
  RECT rect;
  rect.left = desc.Width / 4;
  rect.top = desc.Height / 4;
  rect.right = desc.Width - desc.Width / 4;
  rect.bottom = desc.Height - desc.Height / 4;
  
  // Generate a 64x64 DXT1 surface
  if (!_screenshot)
  {
    //HRESULT hr = _d3DDevice->CreateImageSurface(64, 64, D3DFMT_DXT1, _screenshot.Init());
    HRESULT hr = _d3DDevice->CreateImageSurface(
      rect.right-rect.left, rect.bottom-rect.top, desc.Format, _screenshot.Init()
    );
    if (FAILED(hr)) return;
  }

  POINT dest;
  dest.x = 0;
  dest.y = 0;
  // backbuffer is tiled surface and can be busy - locking it can be very slow
  // to prevent stall, we should copy its content someplace else
  D3DCOPYRECTSTATE state;
  if (desc.Format==D3DFMT_LIN_A8R8G8B8 || desc.Format==D3DFMT_A8R8G8B8)
  {
    // when backbuffer contains alpha channel, we need to ignore it
    state.ColorFormat = D3DCOPYRECT_COLOR_FORMAT_X8R8G8B8_O8R8G8B8;
    state.Operation = D3DCOPYRECT_SRCCOPY;

    state.ColorKeyEnable = FALSE;
    state.ClippingPoint = 0;
    state.ClippingSize = ((rect.bottom-rect.top)<<16)|(rect.left-rect.right);

    _d3DDevice->SetCopyRectsState(&state,NULL);
    HRESULT hr = _d3DDevice->CopyRects(front,&rect,1,_screenshot,&dest);
    state.ColorFormat = D3DCOPYRECT_COLOR_FORMAT_DEFAULT;
    _d3DDevice->SetCopyRectsState(&state,NULL);
    if (FAILED(hr)) _screenshot.Free();
  }
  else
  {
    HRESULT hr = _d3DDevice->CopyRects(front,&rect,1,_screenshot,&dest);
    if (FAILED(hr)) _screenshot.Free();
  }
#endif
}

void EngineDDXB::CleanUpScreenshot()
{
  _screenshot.Free();
}

extern void InitFPU();
extern void DeinitFPU();

bool EngineDDXB::WriteScreenshotToXPR(RString path)
{
#ifdef _XBOX
  if (!_screenshot) return false;

  ComRef<IDirect3DSurface8> dxtShot;
  _d3DDevice->CreateImageSurface(64,64,D3DFMT_DXT1,dxtShot.Init());
  if (dxtShot.NotNull())
  {
    DeinitFPU();
    HRESULT hr = D3DXLoadSurfaceFromSurface(
      dxtShot, NULL, NULL, _screenshot, NULL, NULL, D3DX_FILTER_TRIANGLE, D3DCOLOR(0)
    );
    InitFPU();
    if (SUCCEEDED(hr))
    {
      HRESULT result = XGWriteSurfaceOrTextureToXPR(dxtShot, path, TRUE);
      return SUCCEEDED(result);
    }
  }
  
#endif
  return false;
}

void EngineDDXB::PersistDisplay()
{
  if (_d3DDevice) _d3DDevice->PersistDisplay();
}

#endif