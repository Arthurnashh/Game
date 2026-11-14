#include <El/elementpch.hpp>

#pragma comment(lib,"dsound")

#include "voiceCapture.hpp"

const DWORD VOICE_SAMPLE_RATE = 16000;
const DWORD BYTES_PER_SAMPLE = 2;

#define NUM_REC_NOTIFICATIONS  16

VoiceCapture::VoiceCapture()
{
  _notifySize = 0;
  _bufferSize = 0;
  _bufferOffset = 0;
  _capturing = false;

  // Create IDirectSoundCapture using the preferred capture device
  HRESULT result = DirectSoundCaptureCreate(NULL, _capture.Init(), NULL);
  if (FAILED(result))
  {
    _capture.Free();
    return;
  }

  // Waveform format:
  WAVEFORMATEX fmt;
  fmt.wFormatTag      = WAVE_FORMAT_PCM;
  fmt.nChannels       = 1;
  fmt.nSamplesPerSec  = VOICE_SAMPLE_RATE;
  fmt.wBitsPerSample  = BYTES_PER_SAMPLE * 8;
  fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
  fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
  fmt.cbSize          = 0;

  // Set the notification size [NOTE: Must be an even multiple of 320 (i.e., an even multiple of a 10ms frame)],
  // or recognition accuracy might suffer. -drw
  _notifySize = 4 * 160 * sizeof(short); // 4 frames; 10ms*16kHz=160 samples; one short per sample. 
  _notifySize -= _notifySize % fmt.nBlockAlign;   

  // Set the buffer sizes 
  _bufferSize = _notifySize * NUM_REC_NOTIFICATIONS;

  // Sound-buffer description:
  DSCBUFFERDESC desc;
  memset(&desc, 0, sizeof(desc));
  desc.dwSize = sizeof(desc);
  desc.dwFlags = 0;
  desc.dwBufferBytes = _bufferSize;
  desc.lpwfxFormat = &fmt;
  desc.dwReserved = 0;

  result = _capture->CreateCaptureBuffer(&desc, _buffer.Init(), NULL);
  if (FAILED(result))
  {
    _buffer.Free();
    _capture.Free();
    return;
  }

  ComRef<IDirectSoundNotify> notify;
  result = _buffer->QueryInterface(IID_IDirectSoundNotify, (LPVOID *)notify.Init());
  if (FAILED(result))
  {
    _buffer.Free();
    _capture.Free();
    return;
  }

  _notificationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  // Setup the notification positions
  DSBPOSITIONNOTIFY pos[NUM_REC_NOTIFICATIONS];  
  for (int i=0; i<NUM_REC_NOTIFICATIONS; i++)
  {
    pos[i].dwOffset = _notifySize * i + (_notifySize - 1);
    pos[i].hEventNotify = _notificationEvent;
  }

  // Tell DirectSound when to notify us
  result = notify->SetNotificationPositions(NUM_REC_NOTIFICATIONS, pos);
  if (FAILED(result))
  {
    _buffer.Free();
    _capture.Free();
    return;
  }
}

VoiceCapture::~VoiceCapture()
{
  CloseHandle(_notificationEvent);
}

bool VoiceCapture::GetData(AutoArray<char> &data)
{
  if (WaitForSingleObject(_notificationEvent, 0) != WAIT_OBJECT_0) return false;

  if (_buffer == NULL) return false;

  // extract data
  DWORD readPos;
  DWORD capturePos;
  HRESULT result = _buffer->GetCurrentPosition(&readPos, &capturePos);
  if (FAILED(result)) return false;

  // calculate data size
  LONG size = readPos - _bufferOffset;
  if (size < 0) size += _bufferSize;
  // block align lock size so that we are always write on a boundary
  size -= (size % _notifySize);
  if (size == 0) return false;

  // lock the capture buffer down
  VOID *data1 = NULL;
  DWORD length1;
  VOID *data2 = NULL;
  DWORD length2;
  result = _buffer->Lock(_bufferOffset, size, &data1, &length1, &data2, &length2, 0);
  if (FAILED(result)) return false;

  // copy data
  DWORD length = length1;
  if (data2) length += length2;
  data.Realloc(length);
  data.Resize(length);
  memcpy(data.Data(), data1, length1);
  if (data2) memcpy(data.Data() + length1, data2, length2);

  _bufferOffset += length;
  _bufferOffset %= _bufferSize; // Circular buffer

  // unlock the capture buffer
  _buffer->Unlock(data1, length1, data2, length2);

  return true;
}

void VoiceCapture::Start()
{
  if (_buffer && !_capturing)
  {
    HRESULT result = _buffer->Start(DSCBSTART_LOOPING);
    if (SUCCEEDED(result))
    {
      _capturing = true;
      LogF("Voice capturing started.");
    }
  }
}

void VoiceCapture::Stop()
{
  if (_buffer)
  {
    if (_capturing)
    {
      _buffer->Stop();
      _capturing = false;
      LogF("Voice capturing stopped.");
    }
  }
  else _capturing = false;
}
