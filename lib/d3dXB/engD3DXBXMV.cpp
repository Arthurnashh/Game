#include "../wpch.hpp"

#if !_DISABLE_GUI || defined _XBOX

#include "engddXB.hpp"
#include "../textbank.hpp"
#include <xmv.h>
#pragma comment(lib,"xmv.lib")

#include <El/common/perfLog.hpp>
#include <El/common/perfProf.hpp>

struct XMVDecoder;

class VideoPlaybackXMV: public VideoPlayback
{
  friend class EngineDDXB;

protected:
  char *_buffer;
  DWORD _bufferSize;

  AutoArray<char> _buffer2;
  char *_pending;
  char *_decoding;
  char *_loading;

  QIFStream _file;
  DWORD _offset;
  DWORD _size;

public:
  XMVDecoder *_decoder;
  explicit VideoPlaybackXMV(const char *path, float volume);
  ~VideoPlaybackXMV();

  HRESULT GetNextPacket(VOID **data, DWORD *size);
  HRESULT ReleasePreviousPacket(LONGLONG offset, DWORD size);

private:
  void operator = (const VideoPlaybackXMV &src);
  VideoPlaybackXMV(const VideoPlaybackXMV &src);
  
  void Close();
  void PreRead();
};

static HRESULT CALLBACK GetNextPacket(DWORD ctx, VOID **data, DWORD *size)
{
  VideoPlaybackXMV *video = (VideoPlaybackXMV *)ctx;
  Assert(video);
  return video->GetNextPacket(data, size);
}

static HRESULT CALLBACK ReleasePreviousPacket(DWORD ctx, LONGLONG offset, DWORD size)
{
  VideoPlaybackXMV *video = (VideoPlaybackXMV *)ctx;
  Assert(video);
  return video->ReleasePreviousPacket(offset, size);
}

void VideoPlaybackXMV::Close()
{
  if (_decoder)
  {
    XMVDecoder_CloseDecoder(_decoder);
    _decoder = NULL;
  }

  if (_buffer)
  {
    XALLOC_ATTRIBUTES attr;
    attr.dwObjectType = 0;
    attr.dwHeapTracksAttributes = FALSE;
    attr.dwMustSucceed = FALSE;
    attr.dwFixedSize = FALSE;
    attr.dwAllocatorId = 0;
    attr.dwAlignment = XALLOC_PHYSICAL_ALIGNMENT_DEFAULT;
    attr.dwMemoryProtect = XALLOC_MEMPROTECT_READWRITE;
    attr.dwZeroInitialize = FALSE;
    attr.dwMemoryType = XALLOC_MEMTYPE_PHYSICAL;
    XMemFree(_buffer, (DWORD &)attr);
    _buffer = NULL;
  }
}

VideoPlaybackXMV::VideoPlaybackXMV(const char *path, float volume)
{
  _decoder = NULL;
  
  _pending = NULL;
  _decoding = NULL;
  _loading = NULL;

  _offset = 0;
  _size = 0;

  _buffer = NULL;
  _bufferSize = 0;

  // We need to read in the first 4K of data for the XMV player to initialize
  // itself from. This is most conveniently read as an array of DWORDS.
  DWORD first4Kbytes[4096 / sizeof(DWORD)];

  _file.open(path);
  _file.read(first4Kbytes, sizeof(first4Kbytes));
  if (_file.fail() || _file.eof()) return;

  HRESULT result = XMVDecoder_CreateDecoderForPackets
  (
    XMVFLAG_FULL_LOOP, first4Kbytes, (DWORD)this, ::GetNextPacket, ::ReleasePreviousPacket, &_decoder
  );
  if (FAILED(result))
  {
    Close();
    return;
  }

  // The size of the first packet and the minimum size of the two packet buffers are stored in the
  // second and third DWORDS of the file. From xmv.h:
  // *    DWORD NextPacketSize  // The size of the next packet
  // *    DWORD ThisPacketSize  // The size of this packet
  // *    DWORD MaxPacketSize   // The size of the largest packet in the file
  _size = first4Kbytes[1];
  _bufferSize = first4Kbytes[2];

  // Check for illegal parameters.
  if (_size > _bufferSize)
  {
    Close();
    return;
  }

  XALLOC_ATTRIBUTES attr;
  attr.dwObjectType = 0;
  attr.dwHeapTracksAttributes = FALSE;
  attr.dwMustSucceed = FALSE;
  attr.dwFixedSize = FALSE;
  attr.dwAllocatorId = 0;
  attr.dwAlignment = XALLOC_PHYSICAL_ALIGNMENT_DEFAULT;
  attr.dwMemoryProtect = XALLOC_MEMPROTECT_READWRITE;
  attr.dwZeroInitialize = FALSE;
  attr.dwMemoryType = XALLOC_MEMTYPE_PHYSICAL;

  _buffer = (char *)XMemAlloc(2 * _bufferSize, (DWORD &)attr);
  if (!_buffer)
  {
    Close();
    return;
  }

  _loading = _buffer;
  _decoding = _buffer + _bufferSize;

  PreRead();

  ComRef<IDirectSoundStream> soundStream;
  result = _decoder->EnableAudioStream(0, 0, NULL, soundStream.Init());
  if (SUCCEEDED(result))
  {
    LONG float2dbLong(float val);
    soundStream->SetVolume(float2dbLong(volume));
  }

/*
  HRESULT result = XMVDecoder_CreateDecoderForFile(XMVFLAG_FULL_LOOP, path, &_decoder);
  if (_decoder && SUCCEEDED(result))
  {
    _decoder->EnableAudioStream(0, 0, NULL, NULL);
  }
*/
}

HRESULT VideoPlaybackXMV::GetNextPacket(VOID **data, DWORD *size)
{
  if (_file.PreRead(FileRequestPriority(1), _offset, _size))
  {
    Assert(_loading);
    _file.seekg(_offset, QIOS::beg);
    _file.read(_loading, _size);
    if (_file.fail() || _file.eof()) return E_FAIL;
    _pending = _decoding;
    _decoding = _loading;
    _loading = NULL;

    *data = _decoding;
    *size = _size;
  }
  else
  {
    *data = NULL;
    *size = 0;
  }
  return S_OK;
}

HRESULT VideoPlaybackXMV::ReleasePreviousPacket(LONGLONG offset, DWORD size)
{
  if (size > 0)
  {
    // Check for bad input file - buffer overrun
    if (size > _bufferSize) return E_FAIL;

    Assert(offset >> 32 == 0);
    _offset = (DWORD)(offset & 0xffffffff);
    _size = size;

    _loading = _pending;
    _pending = NULL;

    PreRead();
  }
  return S_OK;
}

VideoPlaybackXMV::~VideoPlaybackXMV()
{
  Close();
}

const int PreloadSize = 2 * 1024 * 1024;
const int PreloadAlign = 1 * 1024 * 1024;

void VideoPlaybackXMV::PreRead()
{
  // preload aligned to 1 MB
  QFileSize offset = _offset;
  QFileSize end = _offset+PreloadSize;
  // align preloading to make sure we load big chunks
  end &= ~(PreloadAlign-1);
  
  if (end>offset)
  {
    //LogF("PreRead %x,%x",offset,end);
    for (QFileSize total = 0; total < end-offset; total += _bufferSize)
    { // this loop is actually not needed - PreRead does it internally
      _file.PreRead(FileRequestPriority(600), offset+total, _bufferSize);
    }
  }
  
}

VideoPlayback *EngineDDXB::VideoOpen(const char *path, float volume)
{
  VideoPlaybackXMV *xmv = new VideoPlaybackXMV(path, volume);
  if (!xmv->_decoder)
  {
    RptF("Video decoder not created for %s",cc_cast(path));
    delete xmv;
    return NULL;
  }

  // switch to presentation interval 1
  if (_presentationInterval!=1)
  {
    D3DPRESENT_PARAMETERS pp = _pp;
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    _presentationInterval = 1;
    
    _d3DDevice->Reset(&pp);
  }
  return xmv;
}

void EngineDDXB::VideoClose(VideoPlayback *video)
{
  VideoPlaybackXMV *xmv = static_cast<VideoPlaybackXMV *>(video);
  delete xmv;
  // switch to normal presentation interval
  if (_presentationInterval!=_defPresentationInterval)
  {
    _d3DDevice->Reset(&_pp);
    _presentationInterval = _defPresentationInterval;
  }
  
}
void EngineDDXB::VideoFrame(VideoPlayback *video)
{
  VideoPlaybackXMV *xmv = static_cast<VideoPlaybackXMV *>(video);
  
  XMVVIDEO_DESC desc;
  xmv->_decoder->GetVideoDescriptor(&desc);
  // video resolution is desc.Width x desc.Height

  int movieHeight = desc.Height;
  int movieWidth = desc.Width;


  //PROFILE_SCOPE_GRF(xmv,SCOPE_COLOR_GRAY);

  // create source surface
  ComRef<IDirect3DTexture8> texture;
  HRESULT hr = _d3DDevice->CreateTexture(movieWidth, movieHeight, 1, 0, D3DFMT_YUY2, 0, texture.Init());
  if (FAILED(hr))
  {
    LogF("CreateTexture failed with 0x%x", hr);
    return;
  }
  ComRef<IDirect3DSurface8> src;
  hr = texture->GetSurfaceLevel(0, src.Init());
  if (FAILED(hr))
  {
    LogF("GetSurfaceLevel failed with 0x%x", hr);
    return;
  }

  D3DSURFACE_DESC xmvSurfaceDesc;
  src->GetDesc(&xmvSurfaceDesc);
  
  XMVRESULT xr = XMV_NOFRAME;
  hr = xmv->_decoder->GetNextFrame(src, &xr, NULL);
  if (FAILED(hr))
  {
    LogF("GetNextFrame failed with 0x%x", hr);
  }
  while (xr == XMV_NOFRAME)
  {
    Sleep(0);
    hr = xmv->_decoder->GetNextFrame(src, &xr, NULL);
    if (FAILED(hr))
    {
      LogF("GetNextFrame failed with 0x%x", hr);
      break;
    }
  }

  DoSwitchTL(false);
  
  D3DSetRenderState( D3DRS_YUVENABLE, TRUE );
  D3DSetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  D3DSetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  D3DSetRenderState( D3DRS_ZENABLE,  FALSE );

  // Draw the texture as a quad.
  _d3DDevice->SetTexture(0, texture);
  _lastTexture[0].Free();

  // Mark material as invalid to force setting states
  _lastMat = (TexMaterial *)-1;

  // Wrapping isn't allowed on linear textures.
  D3DSetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
  D3DSetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
  _lastClamp = TexClampU|TexClampV;
  
  FLOAT fLeft   = 0;
  FLOAT fRight  = GEngine->Width();
  FLOAT fTop    = 0;
  FLOAT fBottom = GEngine->Height();
  
  SetTexture0Color(HWhite,HWhite,true);
  
  SelectPixelShaderSpecular(PSSNormal);
  SelectPixelShader(PSNormal);
  DoSelectPixelShader();
  
  // On linear textures the texture coordinate range is from 0,0 to width,height, instead
  // of 0,0 to 1,1.
  // check SVertexVSNonTLDecl
  // reset stencil to avoid any postprocessing
  D3DSetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
  D3DSetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
  D3DSetRenderState(D3DRS_STENCILWRITEMASK, 0xff);

  _currentShadowStateSettings = SS_None;
  
  _d3DDevice->Begin( D3DPT_QUADLIST );
  
  const float posOffset = -0.5f;
  _d3DDevice->SetVertexData2f( 10, 0, 0 );
  // diffuse - used to control brightness
  _d3DDevice->SetVertexData4f( 3, 1.0f,1.0f,1.0f,1.0f );
  // specular + fog
  _d3DDevice->SetVertexData4f( 4, 0,0,0,1 );
  
  _d3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, movieHeight );
  _d3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posOffset+fLeft,  posOffset+fBottom, 0.0f, 1.0f );
  
  _d3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, 0 );
  _d3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posOffset+fLeft,  posOffset+fTop,    0.0f, 1.0f );
  
  _d3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, movieWidth, 0 );
  _d3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posOffset+fRight, posOffset+fTop,    0.0f, 1.0f );
  
  _d3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, movieWidth, movieHeight );
  _d3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posOffset+fRight, posOffset+fBottom, 0.0f, 1.0f );
  _d3DDevice->End();

  D3DSetRenderState( D3DRS_YUVENABLE, FALSE );

  src.Free();
  texture.Free();
}

#endif