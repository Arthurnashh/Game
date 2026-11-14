#pragma once
#include <vector>
#include "Simul/Sky/Export.h"
#ifdef _MSC_VER
#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace sky
	{
		struct Star
		{
			double declination;
			double ascension;
			double magnitude;
		};
		//! A class that stores star positions by declination, ascension and magnitude.
		SIMUL_SKY_EXPORT_CLASS Stars
		{
		public:
			Stars();
			virtual ~Stars();
			//! Initialize the stars with the default stars - the stars seen from Earth.
			void UseDefaultStars(float max_magnitude=6.f);
			void LoadAsText(const char *filename);
			void SaveAsText(const char *filename);
			void Load(const char *filename);
			void Save(const char *filename);
			int GetNumStars() const;
			const Star &GetStar(int i) const;
		protected:
			std::vector<Star> stars;
		};
	}
}