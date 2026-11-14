#ifndef RECORDINGPLAYBACKINTERFACE_H
#define RECORDINGPLAYBACKINTERFACE_H

#include <string>

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! An interface for controlling recording and playback of node properties.
			class RecordingPlaybackInterface
			{
			public:
				struct KeyFrameLink
				{
					int previous;
					int next;
				};
				typedef enum {END,CONTROL,KEYFRAME,KEYFRAMELINK};
				typedef enum {NONE,START_RECORDING,RECORDING,START_PLAYBACK,PLAYBACK} RecordPlaybackMode;
				RecordPlaybackMode recordPlaybackMode;

				virtual bool FastForward(int step)=0;
				virtual bool Rewind(int step)=0;

				virtual bool StartRecording()=0;
				virtual void StopRecording()=0;

				virtual bool StartPlayback()=0;
				virtual void StopPlayback()=0;

				virtual void SavePlaybackAs(const std::string &filename)=0;
				virtual const std::string &GetPlaybackFilename()=0;

				virtual bool LoadPlaybackFile(const std::string &filename)=0;
				virtual bool IsRecording()=0;
				virtual bool IsPlayingBack()=0;

				virtual float GetDuration()=0;
				virtual void Goto(float)=0;
			};
		}
	}
}
#endif

