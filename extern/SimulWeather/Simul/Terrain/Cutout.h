#ifndef SIMUL_TERRAIN_CUTOUT_H
#define SIMUL_TERRAIN_CUTOUT_H
#include <vector>
#include "Simul/Math/Float2.h"
#include "Simul/Terrain/Export.h"
#ifdef _MSC_VER
	
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace terrain
	{
		//! A generic structure defining an area with an outline that we might want to "cut-out" of
		//! some terrain. For example, a road, or a paved area in the landscape.
		struct SIMUL_TERRAIN_EXPORT Cutout
		{
			struct Vert
			{
				simul::math::float2 pos;
				float height;
				mutable bool used;
			};
			struct Corner
			{
				const Vert * V1;
				const Vert * V2;
				Corner(const Vert * v1=0,const Vert * v2=0):V1(v1),V2(v2)	{}
				bool operator==(const Corner &C)
				{
					return ((V1==C.V1&&V2==C.V2)||(V1==C.V2&&V2==C.V1));
				}
				const Corner&operator=(const Corner &C)
				{
					V1=C.V1;
					V2=C.V2;
					return *this;
				}
			};
			Cutout();
			virtual ~Cutout();
			void Clear();
			size_t NumVertices() const;
			size_t loopindex(int idx) const
			{
				while(idx<0)
					idx+=(int)vertices.size();
				return (size_t)(idx%(vertices.size()));
			}
			const Vert &vertex(int idx) const
			{
				return vertices[loopindex(idx)];
			}
			void GetCorners(simul::math::float2 &min_corner,simul::math::float2 &max_corner) const;
			bool IsInside(const simul::math::float2 &point) const;
			//! does the line from x1 to x2 cross the cutout? If so, find the point of first intersection.
			int GetCrossover(const simul::math::float2 &x1,const simul::math::float2 &x2,simul::math::float2 &intersection,int &edge) const;
			void addvertex(const simul::math::float2 &v,float h);
			void RemoveVertex(size_t v);
			void unuse() const;
			size_t FindEar() const;
			float GetHeightAt(int edge,simul::math::float2 &intersection) const;
		protected:
			std::vector<Vert> vertices;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif