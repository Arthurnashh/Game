#ifndef CurveH
#define CurveH
#include "Simul/Base/Units.h"
#include "Simul/Base/Export.h"
#include <string>
#include <vector>
#ifdef _MSC_VER
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(push)
	#pragma warning(disable:4251)
	#pragma warning(disable:4275)
// Using Visual Studio? Then this pragma works:
	
#endif
namespace simul
{
	namespace base
	{
		/// \internal A datapoint for a Curve.
		struct SIMUL_BASE_EXPORT CurveDatum
		{
			float X;	//< The horizontal co-ordinate
			float Y;	//< The vertical co-ordinate
			void Clear()
			{
				X=0;
				Y=0;
			}
			void operator=(const CurveDatum &TD)
			{
				X=TD.X;
				Y=TD.Y;
			}
			void Set(float x,float y)
			{
				X=x;
				Y=y;
			}
		};
		//! \internal  A list of datapoints in the x-y plane with sequential x values.
		SIMUL_BASE_EXPORT_CLASS Curve : public std::vector<CurveDatum>
		{
			std::string XUnits;
			std::string YUnits;
		public:
			Curve();
			virtual ~Curve();
			CurveDatum* operator[](int i);
			float operator()(float x) const;	//< Get the linearly interpolated y value at x.
			CurveDatum * Add();
			void Cut(CurveDatum *D);
			void Clear();
			void Save(std::string &out,const std::string &Name) const;
			void Save(std::string &out,const std::string &Name,const std::string &Units) const;
			void SetProperty(const std::string &name,const std::string &property);
			void Load(const std::string &in);
			void SetCurvePoint(float r,float t,float snap_tolerance_revs);
			void operator=(const Curve &C);
			float ValueAt(float r) const;
			CurveDatum *GetPoint(float r);
			iterator GetIteratorOfDatum(CurveDatum *D);
			const CurveDatum *GetPoint(float r) const;
			CurveDatum *GetPointAtIndex(int n);
			void DeletePoint(float r);
			PhysicalUnits::Type XUnitType;
			PhysicalUnits::Type YUnitType;
			float FindFirstOf(float y);				//< Find the x where y first occurs - if at all.
			float GetMaxX() const;					//< Get the largest x value.
			float GetMaxY() const;					//< Get the largest y value.
		};
	}
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
