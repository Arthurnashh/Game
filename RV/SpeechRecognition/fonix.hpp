#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FONIX_HPP
#define _FONIX_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/array.hpp>

#include <Es/Common/win.h>
#include <Fonix/VoiceCmds.h>

enum FCState
{
  FCSUnavailable,       // Voice recognition is not initialized.
  FCSNotConnected,      // No communicator on this port
  FCSIdle,              // Not doing any speech recognition [waiting for push-to-talk button]
  FCSListening,         // Currently accepting wave input into the recognizer
  FCSFinishing,         // Hit end-of-speech, and finishing processing on buffered data [don't send more wave samples]
  FCSDisplaying         // Finished processing speech, got result, now displaying/animating results.
};

#define OUTPUT_WAV  0

class FonixCommunicator
{
public:
  FonixCommunicator();
  ~FonixCommunicator();

  bool Initialize(RString filename);
  void Done();
  bool Inserted();
  bool Removed();
  bool PushToTalk();
  bool Process(void* data, unsigned int length, int &id);

  FCState GetState() const;

protected:
  RString _filename;
  FCState _state;

  // Fonix voice recognition information.
  AutoArray<char> _dataVoice;        /* Main (common) recognition structure. */
  AutoArray<char> _dataVocab;        /* Recognition vocabulary */
  AutoArray<char> _dataUser;         /* Voice User structure for this headset */

#if OUTPUT_WAV
  // diagnostics - processed sound
  AutoArray<short> _processed;
#endif
};

#endif