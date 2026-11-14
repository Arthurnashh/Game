#include <El/elementpch.hpp>
#include "fonix.hpp"

#include <El/QStream/qbStream.hpp>

#pragma comment(lib, "VoiceCmds")

FonixCommunicator::FonixCommunicator()
{
  _state = FCSUnavailable;
}

FonixCommunicator::~FonixCommunicator()
{
}

bool FonixCommunicator::Initialize(RString filename)
{
  Done(); // clean up

  _filename = filename;

  // Load the common voice recognition information.
  QIFStreamB in;
  
  in.AutoOpen(filename + RString(".xvocab"));
  int len = in.rest();
  _dataVocab.Realloc(len);
  _dataVocab.Resize(len);
  in.read(_dataVocab.Data(), len);
  in.close();
  if (in.fail())
  {
    Done();
    return false;
  }

  in.AutoOpen(filename + RString(".vnn"));
  len = in.rest();
  _dataVoice.Realloc(len);
  _dataVoice.Resize(len);
  in.read(_dataVoice.Data(), len);
  in.close();
  if (in.fail())
  {
    Done();
    return false;
  }

  HRESULT result = FnxVoiceInit(_dataVoice.Data());
  if (FAILED(result))
  {
    Done();
    return false;
  }

  // Load vocabulary
  result = FnxVocabInit(_dataVocab.Data());
  if (FAILED(result))
  {
    Done();
    return false;
  }

  // There can only be one
  _state = FCSNotConnected;
//  m_bSpeechDetected = FALSE;

  Inserted();

  return true;
}

void FonixCommunicator::Done()
{
  if (_state > FCSNotConnected) Removed();

  _filename = RString();
  _state = FCSUnavailable;

  _dataVoice.Clear();
  _dataVocab.Clear();
  _dataUser.Clear();
}

bool FonixCommunicator::Inserted()
{
  //*******************************************
  // Initialize user-specific ASR information

  if (_state == FCSUnavailable) return false;

  QIFStreamB in;

  in.AutoOpen(_filename + RString(".usr"));
  int len = in.rest();
  _dataUser.Realloc(len);
  _dataUser.Resize(len);
  in.read(_dataUser.Data(), len);
  in.close();
  if (in.fail())
  {
    Removed();
    return false;
  }

  HRESULT result = FnxVoiceUserInit(_dataVoice.Data(), _dataUser.Data());
  if (FAILED(result))
  {
    Removed();
    return false;
  }

  // Select the one and only vocabulary
  void *ptr = _dataVocab.Data();
  result = FnxSelectXVocabs(_dataUser.Data(), &ptr, 1);
  if (FAILED(result))
  {
    Removed();
    return false;
  }

  // Start out in the idle state
  _state = FCSIdle;
  // m_bSpeechDetected = FALSE;
  return true;
}

bool FonixCommunicator::Removed()
{
  if (_state == FCSUnavailable) return false;
  _dataUser.Clear();
  _state = FCSNotConnected;
  return true;
}

bool FonixCommunicator::PushToTalk()
{
  if (_state == FCSDisplaying || _state == FCSIdle)
  {
    _state = FCSListening;
    return true;
  }
  return false;
}

// debugging
#if OUTPUT_WAV

#include <mmreg.h>

const DWORD VOICE_SAMPLE_RATE = 16000;
const DWORD BYTES_PER_SAMPLE = 2;

#define RIFF_FOURCC_C(ch0, ch1, ch2, ch3)  \
  ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
  ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define RIFF_FOURCC RIFF_FOURCC_C('R','I','F','F')
#define RIFF_WAVE RIFF_FOURCC_C('W','A','V','E')
#define RIFF_fmt RIFF_FOURCC_C('f','m','t',' ')
#define RIFF_data RIFF_FOURCC_C('d','a','t','a')
#define RIFF_fact RIFF_FOURCC_C('f','a','c','t')

struct RiffChunk
{
  DWORD id;
  DWORD len;
};

void WriteWav(const char *filename, AutoArray<short> &data)
{
  int samples = data.Size();
  int headerSize = sizeof(WAVEFORMATEX);
  int chunkSize = sizeof(RiffChunk);

  int riffSize = sizeof(DWORD) +          // "WAVE"
    chunkSize + headerSize +              // fmt chunk
    chunkSize + sizeof(int) +             // fact chunk
    chunkSize + samples * sizeof(short);  // data chunk

  QOFStream out;
  out.open(filename);

  // RIFF chunk
  RiffChunk chunk;
  chunk.id = RIFF_FOURCC;
  chunk.len = riffSize;
  out.write(&chunk, chunkSize);
  DWORD id = RIFF_WAVE;
  out.write(&id, sizeof(DWORD));

  // fmt chunk
  chunk.id = RIFF_fmt;
  chunk.len = headerSize;
  out.write(&chunk, chunkSize);

  WAVEFORMATEX fmt;
  fmt.wFormatTag      = WAVE_FORMAT_PCM;
  fmt.nChannels       = 1;
  fmt.nSamplesPerSec  = VOICE_SAMPLE_RATE;
  fmt.wBitsPerSample  = BYTES_PER_SAMPLE * 8;
  fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
  fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
  fmt.cbSize          = 0;
  out.write(&fmt, headerSize);

  // fact chunk
  chunk.id = RIFF_fact;
  chunk.len = sizeof(int);
  out.write(&chunk, chunkSize);
  out.write(&samples, sizeof(int));

  // data chunk
  chunk.id = RIFF_data;
  chunk.len = samples * sizeof(short);
  out.write(&chunk, chunkSize);
  out.write(data.Data(), samples * sizeof(short));

  out.close();
}
#endif

bool FonixCommunicator::Process(void* data, unsigned int length, int &id)
{
  id = -1;
  if (_state == FCSListening || _state == FCSFinishing)
  {
    DWORD flags;

    // Submit the copied data to the speech recognizer
    if (_state == FCSFinishing)
    {
      // Provide time to finish doing recognition, but don't send more data
      HRESULT result = FnxVoiceRecognize(_dataVoice.Data(), _dataUser.Data(), NULL, 0, &flags);
      if (FAILED(result))
      {
        Removed();
        Done();
        return false;
      }
    }
    else 
    {
      // Send in some wave data and do some processing 
      HRESULT result = FnxVoiceRecognize(_dataVoice.Data(), _dataUser.Data(), (short *)data, length / sizeof(short), &flags);
      if (FAILED(result))
      {
        Removed();
        Done();
        return false;
      }

      if (flags & END_OF_SPEECH)
      {
        _state = FCSFinishing;
      }

      // Diagnostics
      if (flags & SPEECH_DETECTED)
      {
      }
      else
      {
#if OUTPUT_WAV
        const int backoff = 4000; // = 250ms at 16kHz;

        // Speech has still not been detected.  So only save the last 250ms of speech
        int n = _processed.Size();
        int nWanted = backoff;
        if (n > nWanted)
        {
          _processed.Delete(0, n - nWanted);
        }
#endif
      }
    }

#if OUTPUT_WAV
    // Add the current speech to the playback speech buffer
    int n = _processed.Size();
    int added = length / sizeof(short);
    _processed.Resize(n + added);
    memcpy(_processed.Data() + n, data, added * sizeof(short));
#endif

    if (flags & RESULTS_AVAILABLE)
    {
      int n;
      DWORD *IDs;
      FLOAT *confidences;

      // Processing has completed on the speech, so get the results
      HRESULT result = FnxVoiceGetResults(_dataVoice.Data(), _dataUser.Data(), &IDs, &confidences, &n);
      if (FAILED(result))
      {
        Removed();
        Done();
        return false;
      }

      LogF("Speech recognition results:");
      for (int i=0; i<n; i++)
      {
        LogF("Confidence of %d: %.2f", IDs[i], confidences[i]);
      }

      // Check confidences ...
      if (n > 0 && confidences[0] > 20.0f) id = IDs[0];

      // Prepare to display animation
      _state = FCSDisplaying;

      // Save _processed
#if OUTPUT_WAV
      WriteWav("processed.waw", _processed);
      _processed.Clear();
#endif
    }
  }

  return true;
}

FCState FonixCommunicator::GetState() const
{
  return _state;
}
