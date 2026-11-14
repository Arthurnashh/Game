#ifndef SIMUL_BASE_TIMING_H
#define SIMUL_BASE_TIMING_H
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:

#endif
#include "Simul/Base/Export.h"

namespace simul
{
	namespace base
	{
		struct Int64
		{
			unsigned int LowPart;
			int HighPart;
		};
		//!  \brief A microsecond timer.
		//!  Provides timing to microsecond accuracy.
		class SIMUL_BASE_EXPORT Timer
		{
		private:
		#ifdef _MSC_VER     
			Int64 iStart,iStop;
			int PerfFreqAdjust;			// in case Freq is too big
			int OverheadTicks;			// overhead  in calling timer
			Int64 iPerfFreq;		 // ticks per second
			int Oht;
			int ReduceMag;
			void EmptyFunction()
			{
				return;
			}
		#endif
		public:
			float Time;
			float TimeSum;
			Timer();
			~Timer();
			//! Start timing: record the start of an event.
			void StartTime();
			//! Stop timing: record the end of an event. The value Time can now be read.
			void FinishTime();
			//! Update the value of Time and continue timing.
			void UpdateTime()
			{
				FinishTime();
				StartTime();
			}
		};
	}
}

#endif
 
