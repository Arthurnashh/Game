#ifndef SIMUL_FadeTable_H
#define SIMUL_FadeTable_H

#include <vector>
#include "Simul/Sky/Export.h"
#include "Simul/Sky/Float4.h"
#include "Simul/Base/PropertyMacros.h"

#include "Simul/Base/Referenced.h"

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace sky
	{
		//! The base keyframe structure, containing \ref time, the time that the keyframe represents.
		SIMUL_SKY_EXPORT_STRUCT BaseKeyframe
		{
			BaseKeyframe():time(0),checksum(0){}
			virtual ~BaseKeyframe(){}
			virtual unsigned GetPropertiesChecksum() const {return 0;}
			//! The time this keyframe is for.
			float time;
			unsigned checksum;
			//! Save this keyframe's values to the stream.
			virtual std::ostream &Save(std::ostream &os) const;
			//! Load this keyframe's values from the stream.
			virtual std::istream &Load(std::istream &is);
			//! Return true if the keyframe has a value with the given, case-insensitive, name; return false otherwise.
			virtual bool HasValue(const char *name) const;
			//! Return the float value with the given, case-insensitive, name.
			virtual float GetValue(const char *name) const;
			//! Set the float value with the given, case-insensitive, name.
			virtual void SetValue(const char *name,float val);
			//! Set the integer value with the given, case-insensitive, name.
			virtual void SetInt(const char *,int ){}
			//! Return the integer value with the given, case-insensitive, name.
			virtual int GetInt(const char *) const {return 0;}
			//! Return the colour value with the given, case-insensitive, name.
			virtual simul::sky::float4 GetColour(const char *) const{return simul::sky::float4();}
			//! Set the colour value with the given, case-insensitive, name.
			virtual void SetColour(const char *,simul::sky::float4 ){}
		};
		//! The base class for keyframers. A keyframer stores a list of keyframes, each representing the state at
		//! a particular time.
		SIMUL_SKY_EXPORT_CLASS BaseKeyframer
		{
		public:
			BaseKeyframer();
			META_BeginProperties
				META_ValueProperty(bool	,	DayLoop		,"Whether to loop keyframes - if true, LoopRange determines the looping point.")
				META_ValueProperty(float,	LoopRange	,"The timescale for keyframe looping. All kefyrames should have times between 0 and this range.")
			META_EndProperties
			const std::vector<BaseKeyframe*> &GetKeyframes(){return keyframes;}
			//! Get a pointer to the i'th keyframe.
			BaseKeyframe *GetKeyframe(int i);
			//! Get a pointer to the i'th keyframe.
			const BaseKeyframe *GetKeyframe(int i) const;
			//! Insert a keyframe at the given time and return a pointer to it.
			virtual BaseKeyframe *InsertKeyframe(float time)=0;
			//! Delete the specified keyframe.
			void DeleteKeyframe(int i);
			//! Make sure the keyframes are in ascending order of \a time.
			virtual void SortKeyframes();
			//! Number of keyframes.
			virtual int GetNumKeyframes() const;
			//! Time of a particular keyframe.
			virtual float GetKeyframeTime(int i) const;
			//! Returns the index of the current keyframe.
			virtual int GetCurrentKeyframe() const
			{
				return current_keyframe;
			}
			//! Get the keyframe BEFORE or AT time t:
			virtual int GetKeyframeAtTime(float t) const;
			//! Stream/save  to the std::osteam os.
			std::ostream &Save(std::ostream &os) const;
			//! Stream/load from the std::isteam is.
			std::istream &Load(std::istream &is);
			//! Test whether the keyframes at the specified time have changed and must be invalidated.
			bool ChecksumsFailed(float time);
			virtual void KeyframesChanged(){}
      virtual float GetInterp(int k,float t) const;
		protected:
			static float FractionalPart(float t,float range)
			{
				t/=range;
				int T=(int)t;
				if(t<0.f)
					T--;
				return (t-(float)T)*range;
			}
			virtual unsigned GetGlobalChecksum() const
			{
				return 0;
			}
			//! Called when the third, partially-calculated keyframe has been changed and must be recalculated.
			virtual void RestartNextKeyframe()=0;
			std::vector<BaseKeyframe*> keyframes;
			void ClearKeyframes();
			int keyframe1,keyframe2,keyframe3;
			int current_keyframe;
			bool sorted;
			bool texture_complete[3];
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif