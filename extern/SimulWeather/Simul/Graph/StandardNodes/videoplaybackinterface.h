#ifndef SIMUL_STANDARD_VIDEOPLAYBACK_INTERFACE_H
#define SIMUL_STANDARD_VIDEOPLAYBACK_INTERFACE_H
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! An interface for a class that plays video.
			class VideoPlaybackInterface
			{
			public:
				//! Set the current frame number.
				virtual void SetFrame(unsigned frame)=0;
				//! Advance one frame.
				virtual void GotoNextFrame()=0;
				//! Get the current frame number.
				virtual unsigned GetFrame() const=0;
				//! Get the current frame data.
				virtual const void *GetCurrentFrame(void *ptr)=0;
				virtual const void *GetNextFrame(void *ptr)=0;
				//! Get the \a width and \a height of the video.
				virtual void GetFrameParameters(int &width,int &height) const=0;
				//! Returns true if the video is playing and false otherwise.
				virtual bool IsPlaying()const=0;
			};
		}
	}
}
#endif