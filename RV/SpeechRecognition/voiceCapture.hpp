#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VOICE_CAPTURE_HPP
#define _VOICE_CAPTURE_HPP

#include <Es/Common/win.h>
#include <mmreg.h>
#include <dsound.h>

class VoiceCapture
{
protected:
  ComRef<IDirectSoundCapture> _capture;
  ComRef<IDirectSoundCaptureBuffer> _buffer;

  HANDLE _notificationEvent;

  unsigned int _notifySize;
  unsigned int _bufferSize;
  unsigned int _bufferOffset;

  bool _capturing;

public:
  VoiceCapture();
  ~VoiceCapture();

  bool GetData(AutoArray<char> &data);
  void Start();
  void Stop();
};

#endif