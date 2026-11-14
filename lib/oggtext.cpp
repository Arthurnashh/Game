#ifdef _WIN32

#include "wpch.hpp"
#include "paramFileExt.hpp"
#include "soundsys.hpp"
#include "soundScene.hpp"
#include "world.hpp"
#include "oggtext.h"

#include <El/Multicore/multicore.hpp>
#include <El/Credits/credits.hpp>

#ifdef _DEBUG
  #pragma comment(lib,"libtheoraD.lib")
#else
  #pragma comment(lib,"libtheora.lib")
#endif

OggStream::OggStream( int serial /*= -1*/ ): _serial(serial), _type(TYPE_UNKNOWN), _active(true)
{

}

OggStream::~OggStream()
{
  ogg_stream_clear(&_state);
}

void TheoraDecode::initForData( OggStream* stream )
{
  stream->_theora._ctx =  th_decode_alloc(&stream->_theora._info,  stream->_theora._setup);
  switch(stream->_theora._info.pixel_fmt)
  {
      case TH_PF_420: LogF(" 4:2:0 video\n"); break;
      case TH_PF_422: LogF(" 4:2:2 video\n"); break;
      case TH_PF_444: LogF(" 4:4:4 video\n"); break;
      case TH_PF_RSVD:
      default:
        LogF(" video\n  (UNKNOWN Chroma sampling!)\n");
        break;
  }
  int ppmax = 0;
  int ret = th_decode_ctl(stream->_theora._ctx,  TH_DECCTL_GET_PPLEVEL_MAX,   &ppmax,  sizeof(ppmax));
  Assert(ret == 0);

  ppmax = 0;
  ret = th_decode_ctl(stream->_theora._ctx,  TH_DECCTL_SET_PPLEVEL,   &ppmax,    sizeof(ppmax));
}

TheoraDecode::~TheoraDecode()
{
  th_setup_free(_setup);
  th_decode_free(_ctx);
  th_info_clear(&_info);
  th_comment_clear(&_comment);
}

TheoraDecode::TheoraDecode()
{
  th_info_init(&_info);
  th_comment_init(&_comment);
  _setup  = NULL;
  _ctx  = NULL;
}

void VorbisDecode::initForData( OggStream* stream )
{
  int ret = vorbis_synthesis_init(&stream->_vorbis._dsp, &stream->_vorbis._info);
  ret = vorbis_block_init(&stream->_vorbis._dsp, &stream->_vorbis._block);
}

VorbisDecode::~VorbisDecode()
{
  //vorbis_block_clear(&_block);
  //vorbis_dsp_clear(&_dsp);
  vorbis_comment_clear(&_comment);
  vorbis_info_clear(&_info);
}

VorbisDecode::VorbisDecode()
{
  vorbis_info_init(&_info);
  vorbis_comment_init(&_comment);
}

void OggDecoder::AllocateBuffers()
{
  _audioBuffer.Realloc(AUDIO_BUFFER_SIZE);
  _audioBuffer.Resize(AUDIO_BUFFER_SIZE);
  _audiobufferIndex = _audioBuffer.Data();

  _textureWidth = _video->_theora._info.frame_width;
  _textureHeight = _video->_theora._info.frame_height;  

  for (int i=0; i<lenof(_data); i++)
  {
    _data[i].Realloc(_textureWidth * _textureHeight);
    _data[i].Resize(_textureWidth * _textureHeight);
    memset(_data[i].Data(), 0, sizeof(VideoDataType) * _data[i].Size());
  }
}

bool OggDecoder::InitCommon(RStringB name, int loops)
{
  DoAssert(CheckMainThread());

  if (isPlaying())
  {
    WaitUntilStopped();
  }

  _loops  = loops;
  _granulepos = 0;
  _lastTime  = Time(0);
  _videoDone  = true;
  _audioDone  = true;
  _name  = name;
  
  _isRunning = false;
  _totalSamples = 0;
  _stopped  = false;
  _stopWanted = false;

  if (!OpenVideoBuffer())
  {
    return false;
  }

  initCodec(true);

  AllocateBuffers();
  
  _paused = false;
  _pauseEvent.Reset();
  _loopsEvent.Reset();

  return true;
}

void OggDecoder::init(RStringB name, int &texture_width, int &texture_height, bool autoplay, int loops)
{
  if (!InitCommon(name, loops)) 
  {
    stop();
    return;
  }

  texture_width = _textureWidth;
  texture_height = _textureHeight;

  int cpuNumber = GetCPUCount() - 1;
  _threadHandle = CreateThreadOnCPU(5*1024, OggDecoder::threadProc, this, cpuNumber, THREAD_PRIORITY_NORMAL, "OggThread");
}

void OggDecoder::initCodec(bool isMainThread)
{
  ogg_sync_init(&_state);

  // Read headers for all streams
  readHeaders(&_state, isMainThread);

  _video = NULL;
  _audio = NULL;
  
  IteratorState it;
  int i;
  OggStream* stream;
  _streams.getFirst(it,stream,&i);
  while ( it != ITERATOR_NULL )
  {
    if (!_video && stream->_type == TYPE_THEORA) 
    {
      _video = stream;
      _video->_theora.initForData(_video);
    }
    else if (!_audio && stream->_type == TYPE_VORBIS) 
    {
      _audio = stream;
      _audio->_vorbis.initForData(_audio);
    }
    else
      stream->_active = false;
    
    _streams.getNext(it,stream,&i);
  }

  if (_video) 
  {
#if _ENABLE_REPORT
    LogF("Video stream %d x %d", _video->_theora._info.frame_width, _video->_theora._info.frame_height);
#endif
    _framerate =  float(_video->_theora._info.fps_numerator) /   float(_video->_theora._info.fps_denominator);
    _videoDone = false;
  }

  if (_audio) 
  {
#if _ENABLE_REPORT
    LogF("Audio stream channels: %d KHz: %d", _audio->_vorbis._info.channels, _audio->_vorbis._info.rate);
#endif

    if (!CreateAudio(_audio->_vorbis._info.channels, _audio->_vorbis._info.rate))
  {
      _streams.removeKey(_audio->_serial);
      delete  _audio;
      _audio  = NULL;
    }
    else
    {
      _audioDone = false;
  }
  }
 
}

DWORD WINAPI OggDecoder::threadProc( LPVOID pParam )
{
  OggDecoder* decoder = (OggDecoder*)pParam;
  decoder->threadProcNonStatic(pParam);

  return 0;
}

bool OggDecoder::OpenVideoBuffer()
{
  _fileIdent = GWorld->OpenVideoBuffer(_name, this);
  
#if _ENABLE_REPORT
  DoAssert(_fileIdent != -1);
#endif

  return _fileIdent != -1;
}

void OggDecoder::reinit()
  {
  if (!InitCommon(_name, _loops)) 
    {
    stop();
    return;
    }

  int cpuNumber = GetCPUCount() - 1;
  _threadHandle = CreateThreadOnCPU(5*1024, OggDecoder::threadProc, this, cpuNumber, THREAD_PRIORITY_NORMAL, "OggThread");
  }

void OggDecoder::Restart()
{
  DoAssert(!CheckMainThread());

  int fileIdent = _fileIdent;

  if (isReady()) 
  {
    if (_video) delete  _video, _video  = NULL;
    if (_audio) delete  _audio, _audio  = NULL;

    StopAudio();
    _wave.Free();
    ogg_sync_clear(&_state);
  }

  _fileIdent = GWorld->ResetVideoBuffer(fileIdent);
  
  if (_fileIdent == -1) 
  {
    _stopWanted = true;
    return;
  }

  _granulepos = 0;
  _lastTime  = Time(0);
  _videoDone  = true;
  _audioDone  = true;
  _frameCompleteId = 0;

  _audioBuffer.Realloc(AUDIO_BUFFER_SIZE);
  _audioBuffer.Resize(AUDIO_BUFFER_SIZE);
  _audiobufferIndex = _audioBuffer.Data();

  for (int i=0; i<lenof(_data); i++)
  {
    _data[i].Realloc(_textureWidth * _textureHeight);
    _data[i].Resize(_textureWidth * _textureHeight);
    //memset(_data[i].Data(), 0, sizeof(VideoDataType) * _data[i].Size());
  }

  _totalSamples = 0;
  _stopped = false;
  _stopWanted = false;

  initCodec(false);
}

bool OggDecoder::IsPlaybackDone()
{
  bool result = _loops == _loopsQueried;
  if (!result) _loopsQueried--;
  return result;
}

void OggDecoder::WaitUntilStopped()
{
  if (_stopped) return;

  while(_threadHandle)
  {
  stop();
    WaitForSingleObject(_threadHandle, 100);
}
}

DWORD WINAPI OggDecoder::threadProcNonStatic( LPVOID pParam )
{
  _isRunning = false;

  // wait until loops count is set
  _loopsEvent.Wait();

  _isRunning = true;

  while (!_stopped && (_loops>0))
  {
    if (_stopWanted) break;

    play();
    _loops--;

    if (_stopWanted) break;

    if (_loops>0)
    {
      //audio and video streams must be reinitialized
      Restart();
    }    
  }

  _isRunning = false;

  deinit();

  _stopWanted = true;
  _stopped = true;

  return TRUE;
}

void OggDecoder::readHeaders(ogg_sync_state* state, bool isMainThread) 
{
  ogg_page page;

  bool headersDone = false;
  while (!headersDone && readPage(state, &page, isMainThread)) 
  {
    int ret = 0;
    int serial = ogg_page_serialno(&page);
    OggStream* stream = 0;
    if(ogg_page_bos(&page)) 
    {
      stream = new OggStream(serial);
      ret = ogg_stream_init(&stream->_state, serial);
      _streams.put(serial, stream);
    }
    else
      _streams.get(serial,stream);

    // Add a complete page to the bitstream
    ret = ogg_stream_pagein(&stream->_state, &page);
    
    ogg_packet packet;
    while (!headersDone &&   (ret = ogg_stream_packetpeek(&stream->_state, &packet)) != 0) 
    {

        // A packet is available. If it is not a header packet we exit.
        // If it is a header packet, process it as normal.
        headersDone = headersDone || handleTheoraHeader(stream, &packet);
        headersDone = headersDone || handleVorbisHeader(stream, &packet);
        if (!headersDone) 
        {
          // Consume the packet
          ret = ogg_stream_packetout(&stream->_state, &packet);
          if(ret<0)
          {
            ErrorMessage(EMError, "Error parsing Theora stream headers; corrupt stream?\n");
            return;
          }
        }
    }
  } 
}

bool OggDecoder::handleTheoraHeader( OggStream* stream, ogg_packet* packet )
{
  int ret = th_decode_headerin(&stream->_theora._info,  &stream->_theora._comment, &stream->_theora._setup,  packet);
  if (ret == TH_ENOTFORMAT)
    return false; 

  if (ret > 0) 
  {
    stream->_type = TYPE_THEORA;
    return false;
  }

  return true;
}

bool OggDecoder::handleVorbisHeader( OggStream* stream, ogg_packet* packet )
{
  int ret = vorbis_synthesis_headerin(&stream->_vorbis._info,   &stream->_vorbis._comment,   packet);
  if (stream->_type == TYPE_VORBIS && ret == OV_ENOTVORBIS) 
  {
    return true;
  }
  else if (ret == 0) 
  {
    stream->_type = TYPE_VORBIS;
  }
  return false;
}

 

#define CLAMP(v)    ((v) > 255 ? 255 : (v) < 0 ? 0 : (v))

// structure holds pointers to y, u, v channels
typedef struct _OggPlayYUVChannels 
{
	    unsigned char * ptry;
	    unsigned char * ptru;
	    unsigned char * ptrv;
	    int             y_width;
	    int             y_height;
  		int				      y_pitch;
	    int             uv_width;
	    int             uv_height;
	  	int				uv_pitch;
} OggPlayYUVChannels;

// structure holds pointers to y, u, v channels
typedef struct _OggPlayRGBChannels 
{
	    unsigned char * ptro;
	    int             rgb_width;
	    int             rgb_height;
		int				rgb_pitch;
} OggPlayRGBChannels;

// Vanilla implementation of YUV->RGB conversion 
void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb)
{
		unsigned char * ptry = yuv->ptry;
		unsigned char * ptru = yuv->ptru;
		unsigned char * ptrv = yuv->ptrv;
		unsigned char * ptro = rgb->ptro;
		unsigned char * ptro2;
		int i, j;
	  
    int height  = intMin(rgb->rgb_height, yuv->y_height); 
    int width   = intMin(rgb->rgb_width, yuv->y_width); 

    for (i = 0; i < height; i++) 
		{
			ptro2 = ptro;
			for (j = 0; j < width; j += 2) 
			{
				short pr, pg, pb, y;
				short r, g, b;
	  
				pr = (-56992 + ptrv[j/2] * 409) >> 8;
				pg = (34784 - ptru[j/2] * 100 - ptrv[j/2] * 208) >> 8;
				pb = (-70688 + ptru[j/2] * 516) >> 8;
	  
				y = 298*ptry[j] >> 8;
				r = y + pr;
				g = y + pg;
			  b = y + pb;
	  
				*ptro2++ = CLAMP(r);
				*ptro2++ = CLAMP(g);
				*ptro2++ = CLAMP(b);
				*ptro2++ = 255;
  
				y = 298*ptry[j + 1] >> 8;
				r = y + pr;
				g = y + pg;
				b = y + pb;
	  
				*ptro2++ = CLAMP(r);
				*ptro2++ = CLAMP(g);
				*ptro2++ = CLAMP(b);
				*ptro2++ = 255;
			}

			ptry += yuv->y_pitch;
		
			if (i & 1) 
			{
				ptru += yuv->uv_pitch;
				ptrv += yuv->uv_pitch;
			}
	
			ptro += rgb->rgb_pitch;
		}
}


void OggDecoder::handleTheoraData(OggStream* stream, ogg_packet* packet, void *videoData) 
{
  int ret = th_decode_packetin(stream->_theora._ctx,   packet, &_granulepos);
  if (ret == TH_DUPFRAME)
  {
    return;
  }

  PROFILE_SCOPE_EX(tdHnd, ogg);

  th_ycbcr_buffer buffer;
  ret = th_decode_ycbcr_out(stream->_theora._ctx, buffer);

  OggPlayYUVChannels oggYuv;
  oggYuv.ptry = buffer[0].data;
  oggYuv.ptru = buffer[2].data;
  oggYuv.ptrv = buffer[1].data;

  oggYuv.y_width = buffer[0].width;
  oggYuv.y_height = buffer[0].height;
  oggYuv.y_pitch = buffer[0].stride;

  oggYuv.uv_width = buffer[1].width;
  oggYuv.uv_height = buffer[1].height;
  oggYuv.uv_pitch = buffer[1].stride;

  
  OggPlayRGBChannels oggRgb;
  oggRgb.ptro = static_cast<unsigned char*>(videoData);
  oggRgb.rgb_height = _textureHeight;
  oggRgb.rgb_width = _textureWidth;
  oggRgb.rgb_pitch = _textureWidth*4;

  {
    PROFILE_SCOPE_EX(tdYuv, ogg);
  oggplay_yuv2rgb(&oggYuv, &oggRgb);
}

}

bool OggDecoder::readPage( ogg_sync_state* state, ogg_page* page, bool isMainThread)
{
  int ret = 0;
  if (GWorld->VideoBufferEof(_fileIdent)) return ogg_sync_pageout(state, page) == 1;

  // takes the data stored in the buffer of the ogg_sync_state struct and inserts them into an ogg_page
  while((ret = ogg_sync_pageout(state, page)) != 1) 
  {    
    char* buffer = ogg_sync_buffer(state, 4096);
    Assert(buffer);

    // Read from the file into the buffer
    if (isMainThread)
    {
      DoAssert(CheckMainThread());
      GWorld->ReadVideoBuffer(_fileIdent);
    }
    
    int bytes = GWorld->CopyVideoBuffer( buffer, _fileIdent );
    
    if (_stopWanted) return false;

    if (bytes == 0) 
    {
      // End of file. 
      if (GWorld->VideoBufferEof(_fileIdent)) 
        return ogg_sync_pageout(state, page) == 1;
      else
      {
        if (!isMainThread)
        {
          SwitchToThread();
#if _ENABLE_REPORT
          LogF("SwitchToThread");
#endif
    }
      }
    }

    // Update the synchronisation layer with the number
    // of bytes written to the buffer
    ret = ogg_sync_wrote(state, bytes);
    Assert(ret == 0);
  }

  return true;
}

bool OggDecoder::readPacket( ogg_sync_state* state, OggStream* stream, ogg_packet* packet )
{
  int ret = 0;

  PROFILE_SCOPE_EX(rpOgg, ogg);

  // assembles a data packet for output to the codec decoding engine
  while ((ret = ogg_stream_packetout(&stream->_state, packet)) != 1) 
  {
    if (_stopWanted) return false;

    ogg_page page;
    if (!readPage(state, &page))
    {
      return false;
    }

    int serial = ogg_page_serialno(&page);
    
    //OggStream* pageStream = _streams[serial];
    OggStream* pageStream;
    _streams.get(serial,pageStream);

    if (pageStream && pageStream->_active) 
    {
      // adds a complete page to the bitstream
      ret = ogg_stream_pagein(&pageStream->_state, &page);
      Assert(ret == 0);
    }
  }

  return true;
}

bool OggDecoder::play()
{
  PROFILE_SCOPE_EX(plOgg, ogg);

  int ret = 0;
  ogg_packet packet;
  int totalBufferedSamples = 0;
  int frames = 0;

  while ((!_audioDone || !_videoDone) && !_stopped)
  {
    if (_stopWanted) return false;

    _pauseEvent.Wait();
    PlayAudio();

    if (_stopWanted) return false;

    // play rest of decoded audio samples
    if (_audioDone && _totalSamples>0)
    {
      // all audio data pass
      if (FillBuffer(_audioBuffer.Data(), sizeof(AudioBufferDataType)*_totalSamples* _audio->_vorbis._info.channels))
      {        
        _audiobufferIndex = _audioBuffer.Data();
        totalBufferedSamples  += _totalSamples;
        _totalSamples = 0;
    }
    }

    if (_stopWanted) return false;

    if (_audio && _audio->_active && !_audioDone)
    {
      bool canProceed = _totalSamples* _audio->_vorbis._info.channels < AUDIO_BUFFER_SIZE - 2000; 
      if (!canProceed)
      {
        if (FillBuffer(_audioBuffer.Data(), sizeof(AudioBufferDataType)*_totalSamples* _audio->_vorbis._info.channels))
        {        
          _audiobufferIndex = _audioBuffer.Data();
          totalBufferedSamples  += _totalSamples;
          _totalSamples = 0;
          canProceed  = true;
        };
        
      }
      bool wasReaded = true;
      if ( canProceed && (wasReaded = readPacket(&_state, _audio, &packet))  ) 
      {
        if (vorbis_synthesis(&_audio->_vorbis._block, &packet) == 0) 
        {
          ret = vorbis_synthesis_blockin(&_audio->_vorbis._dsp, &_audio->_vorbis._block);
        }

        PROFILE_SCOPE_EX(vorbs, ogg);

        float** pcm = 0;
        int samples = 0;
        while ((samples = vorbis_synthesis_pcmout(&_audio->_vorbis._dsp, &pcm)) > 0) 
        {
            if (samples > 0) 
            {

              for (int i=0;i < samples; ++i) {
                for(int j=0; j < _audio->_vorbis._info.channels; ++j) 
                {
                  int v = static_cast<int>(floorf(0.5 + pcm[j][i]*32767.0));
                  if (v > 32767) v = 32767;
                  if (v <-32768) v = -32768;
                  *_audiobufferIndex++ = v;
                }
              }
              
              _totalSamples +=samples;

              if (FillBuffer(_audioBuffer.Data(), sizeof(AudioBufferDataType)*_totalSamples* _audio->_vorbis._info.channels))
              {        
                _audiobufferIndex = _audioBuffer.Data();
                totalBufferedSamples  += _totalSamples;
                _totalSamples = 0;
              }
              
            }

            ret = vorbis_synthesis_read(&_audio->_vorbis._dsp, samples);
        }
      }
      if (!wasReaded)
      {
          _audioDone = true;
      }
    }

    if (_stopWanted) return false;
    
    if (_video && !_videoDone)
    {
      float video_time = th_granule_time(_video->_theora._ctx, _granulepos);
      float audio_time = 0.0f;
      bool audioDone = _audioDone && IsBufferStopped();
     
      // video sync is performed on played audio samples
      if (_audio && !audioDone)
      {
        __int64 position = GetVoicePosition();
        audio_time =  float(position) / float(_audio->_vorbis._info.rate);
        
        if (audio_time < video_time)
        {
          float diffTime = video_time - audio_time;
          int ms = (int)(diffTime * 1000);
          float buffer_time =  float(totalBufferedSamples - GetVoicePosition()) / float(_audio->_vorbis._info.rate);
          int buffer_ms = (int)(buffer_time * 1000);
          ms  = intMin(buffer_ms, ms);
          if (!_stopWanted) 
          {
            Sleep(ms / 2);
          }
        };    
      }
      
      bool condition = (audio_time > video_time);
      
      if (_stopWanted) return false;

      //if audio stream doesn't exist or is shorter as video stream
      if (!condition && (!_audio || (_audio && (audioDone && _totalSamples == 0) && IsBufferStopped())))
      {
        float dT = Glob.time - _lastTime;
        condition = dT >= (1.0f / _framerate);
        if (!condition)
        {
          int diffTime = toInt(1000.0f * ((1.0f / _framerate) - dT));
          if (!_stopWanted) 
          {
            if (!_stopWanted) 
            {
              Sleep(diffTime / 2);
            }
          }
        }
      }     

      if (condition)
      {
        // process video
        ogg_packet packet;
        if (readPacket(&_state, _video, &packet)) 
        {
          frames++;
          _lastTime  = Glob.time;

          int newId = 1-_frameCompleteId;
          ScopeLockSection lockData(_dataCS[newId]);
          handleTheoraData(_video, &packet, _data[newId].Data());

          // new data ready, switch to them
          _frameCompleteId = newId;
        }
        else
        {
          _videoDone = true;
        }
      }
    }
  }

#if _ENABLE_REPORT
  LogF("Video playback done, frames played: %d", frames);
#endif

  StopAudio();

  return true;
}

OggDecoder::OggDecoder(): _pauseEvent(true), _loopsEvent(true), _frameCompleteId(0)
{
  _video            = NULL;
  _audio            = NULL;
  _threadHandle     = NULL;
  _loops            = 0;
  _loopsQueried     = 0;
}

OggDecoder::~OggDecoder()
{
  DoAssert(CheckMainThread());

  WaitUntilStopped();

  _audioBuffer.Clear();
  for (int i=0; i<lenof(_data); i++)
  {
    _data[i].Clear();
  }
}

void OggDecoder::deinit()
{
  GWorld->CloseVideoBuffer(_fileIdent);
  
  if (_video)
  {
    delete  _video;
    _video  = NULL;
  };

  if (_audio)
  {
    delete  _audio;
    _audio  = NULL;
  }
  
  StopAudio();
  _wave.Free();

  _audioBuffer.Resize(0);
  _audioBuffer.Clear();
  for (int i=0; i<lenof(_data); i++)
  {
    _data[i].Clear();
  }
  ogg_sync_clear(&_state);
  _fileIdent = -1;

  ::CloseHandle(_threadHandle);
  _threadHandle = NULL;
}

bool OggDecoder::getData( void **videoData )
{
  ScopeLockSection lockData(_dataCS[_frameCompleteId]);

  memcpy(*videoData, _data[_frameCompleteId].Data(), _data[_frameCompleteId].Size() * sizeof(VideoDataType));

  return true;
}

void OggDecoder::PauseAudio(bool pause)
  {
  if (_wave)
  {
    _wave->Pause(pause); // set flag wantPlaying
  }
}

bool OggDecoder::CreateAudio(int channels, int frequency)
{
  if (!GSoundsys) return false;

  _wave = GSoundsys->CreateDecoderWave();

  if (!_wave) return false;

  return _wave->InitSourceVoice(channels, frequency);
}

bool OggDecoder::FillBuffer(const void *data, size_t nbytes)
{
  if (_wave)
  {
    return _wave->FillBuffer(data, nbytes);
  }

    return false;
  }

bool OggDecoder::IsBufferStopped() const
{
  if (_wave) 
  {
    return _wave->IsBuffersStopped();
  }

  return true;
}

void OggDecoder::PlayAudio()
{
  if (_wave)
  {
    _wave->Play();
  }
}

void OggDecoder::StopAudio()
  {
  if (_wave)
  {
    _wave->Stop();
  }
}
  
__int64 OggDecoder::GetVoicePosition()
  {
  if (_wave)
  {
    return _wave->GetVoicePosition();
  }

  return 0;
}

void OggDecoder::Start()
{
  if (_threadHandle)
  {    
    _loopsEvent.Set();
    _pauseEvent.Set(); 
  }
}

void OggDecoder::pause(bool pause)
{
  if (_threadHandle ==  NULL || !_isRunning) return;
  if (pause == _paused) return;

  _paused = pause;

  if (pause)
  {
    _pauseEvent.Reset();
    PauseAudio(true);
  }
  else
  {
    // wakeup
    _pauseEvent.Set();
  }
}

void OggDecoder::stop()
{
  if (_stopped) return;

  if (_paused) _pauseEvent.Set();
  if (!_isRunning) _loopsEvent.Set();

  _stopWanted = true;
}

void OggDecoder::setLoops(int loops)
{
  _loops  = loops;
  _loopsQueried = _loops;
  _loopsEvent.Set();
}

bool OggDecoder::isPlaying()
{
  return (!_stopped && !_paused);
}

bool OggDecoder::isReady()
{
  return  (_fileIdent != -1);
}

REGISTER_CREDITS("Theora","2002-2009 by the Xiph.Org Foundation http://www.xiph.org/",STANDARD_DISCLAIMER)
REGISTER_CREDITS("OggVorbis","1994-2002 by the Xiph.Org Foundation http://www.xiph.org/",STANDARD_DISCLAIMER)

#endif

