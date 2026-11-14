#ifndef SIMUL_GRAPH_CLOUDS_CELLULARCLOUDGRID_H
#define SIMUL_GRAPH_CLOUDS_CELLULARCLOUDGRID_H
#include <vector>
#ifdef _MSC_VER

#pragma warning(push)
#pragma warning(disable:4251)		// disable dllexport warning for typedefs derived from stl classes!
#endif
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Math/Noise3D.h"
#include "Simul/Math/RandomNumberGenerator.h"
#include "Simul/Graph/StandardNodes/MemoryUsageInterface.h"
#include "Simul/Clouds/Export.h"
#include "Simul/Clouds/CloudInterface.h"


namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class ShowProgressInterface;
		}
		namespace meta
		{
			class MemoryInterface;
			class Node;
		}
	}
	namespace clouds
	{
		//! A cellular automata grid of cloud densities.
		//! Don't create a CellularCloudGrid directly, instead, create a FastCloudNode.
		SIMUL_CLOUDS_EXPORT_CLASS CellularCloudGrid
			:public simul::graph::standardnodes::MemoryUsageInterface
			,public CloudInterface
		{
		public:
			CellularCloudGrid();
			virtual ~CellularCloudGrid();
			META_BeginProperties
				META_PropertyCategory(Detail)
					META_ValueRangePropertyWithSetCall(unsigned	,GridLength		,8		,1024	,ChangedGrid,"Y (North-South) axis grid size of volumetric texture")
					META_ValueRangePropertyWithSetCall(unsigned	,GridWidth		,8		,1024	,ChangedGrid,"X (East-West) axis grid size of volumetric texture")
					META_ValueRangePropertyWithSetCall(unsigned	,GridHeight		,1		,64		,ChangedGrid,"Z (Up-Down) axis grid size of volumetric texture")
				META_EndPropertyCategory(Detail)
				META_PropertyCategory(Generation)
					META_ValueRangePropertyWithSetCall(float	,Humidity		,0		,1		,NeedRecalculate,"Humidity coefficient: more humid = more clouds")
					META_ValueRangePropertyWithSetCall(unsigned	,RandomSeed		,1		,1000	,NeedRecalculate,"Integer random number seed for cloud generation")
					META_ValuePropertyWithSetCall(bool			,Wrap							,NeedRecalculate,"Wrap the cloud data horizontally")
					META_ValueRangePropertyWithSetCall(unsigned	,NoiseResolution,4		,128	,ChangedNoise,"Resolution of the 3D Perlin noise used to generate clouds")
					META_ValueRangePropertyWithSetCall(unsigned	,NoiseOctaves	,1		,12		,ChangedNoise,"Number of octaves in the Perlin noise function")
					META_ValuePropertyWithSetCall(bool			,UseTbb							,ChangedTbb,"Use Intel(R) Threading Building Blocks(TM).")
					META_ValuePropertyWithSetCall(bool			,ThreadSafety					,ChangedThreadSafety,"Thread-safe, use this if calling the same cloud from multiple threads.")
				META_EndPropertyCategory(Generation)
			META_EndProperties
			virtual float GetCloud(int x,int y,int z) const;
			virtual float InterpCloud(float x,float y,float z) const;
			virtual float GetCloud(unsigned x,unsigned y,unsigned z) const;
			void Init(
				simul::graph::meta::MemoryInterface *mi=NULL,
				simul::math::NoiseInterface *ni=NULL,unsigned l=0,unsigned w=0,unsigned h=0);
			virtual void SaveGrid(std::ostream &os) const;
			virtual void LoadGrid(std::istream &is);
			void DeallocateMemory(simul::graph::meta::MemoryInterface *mi);
			//! Set the Simul SDK license key as a text string.
			void SetLicense(const char *txt);
		// MemoryUsageInterface
			virtual unsigned GetMemoryUsage() const;
			friend class ApplyInit;
			bool CheckLicense() const;
		protected:
			unsigned GetRebuildChecksum() const;
			simul::math::NoiseInterface *noise;
			bool changed;
			unsigned *cld;		// cloud density grid - 1 bit per cell, so x*y*z/8 bytes.
			virtual void Recalculate()=0;
			virtual void ChangedGrid();
			virtual void ChangedTbb();
			virtual void ChangedThreadSafety();
			virtual void ChangedNoise();
			void LimitNoiseByGrid();
			void Recalc()
			{
				Recalculate();
			}
			void NeedRecalculate();
			void GetIndexAndBit(unsigned x,unsigned y,unsigned z,unsigned &index,unsigned &b) const;
			void SetCloud(unsigned x,unsigned y,unsigned z);
			unsigned w32;
			simul::math::RandomNumberGenerator random;
			unsigned gridsize;	// number of 4-byte numbers in grid
			unsigned license_code[2];
			float noiseInterp;
			float active_humidity;
			bool noise_changed;
			bool enabled;
			float GetNoiseAtGrid(int i,int j,int k) const;
		};
	}
}

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#endif
