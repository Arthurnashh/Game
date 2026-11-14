#ifndef SIMUL_TERRAIN_HEIGHTMAPNODE_H
#define SIMUL_TERRAIN_HEIGHTMAPNODE_H
#include "Simul/Terrain/HeightMapInterface.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/Client.h"
#include "Export.h"
#include <string>
#include "Simul/Math/Vector3.h"
#include "Simul/Graph/StandardNodes/VisualDebugInterface.h"
#ifdef _MSC_VER
	
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
	#pragma warning(disable:4541)
#endif
namespace simul
{
	namespace terrain
	{
		SIMUL_TERRAIN_EXPORT_CLASS HeightMapNode
			:public HeightMapInterface
			,public simul::graph::meta::ClientUser<HeightMapClientInterface>
			,public simul::graph::standardnodes::VisualDebugInterface
			,public simul::graph::meta::Group
		{
		public:
			HeightMapNode();
			virtual ~HeightMapNode();
			void Rebuild() const;
			META_BeginProperties
				META_ValuePropertyWithSetCall(int	,PageSize			,NeedRebuild		,"How large is a page of tiles (in vertices)? Must be (2^n)+1.")
				META_ValuePropertyWithSetCall(int	,TileSize			,RecalcTiling		,"How large is each tile? Must be (2^n)+1 and be smaller than PageSize.")
				META_ValuePropertyWithSetCall(float	,PageWorldX			,ChangedWorldSize	,"The size of a terrain page, in world units.")
				META_ValuePropertyWithSetCall(float	,PageWorldZ			,ChangedWorldSize	,"The size of a terrain page, in world units.")
				META_ValuePropertyWithSetCall(float	,BaseAltitude		,ChangedWorldSize	,"The base altitude for the terrain.")
				META_ValuePropertyWithSetCall(float	,FlattenBelow		,ChangedWorldSize	,"Level off values below here.")
				META_ValuePropertyWithSetCall(float	,MaxHeight			,ChangedWorldSize	,"Maximum height of the terrain.")
				META_ValuePropertyWithSetCall(float	,FractalScale		,ChangedWorldSize	,"Scale length for fractal terrain.")
				META_ValuePropertyWithSetCall(int	,FractalOctaves		,ChangedWorldSize	,".")
				META_ValuePropertyWithSetCall(int	,FractalFrequency	,ChangedWorldSize	,".")
				META_ValuePropertyWithSetCall(float	,Persistence		,ChangedWorldSize	,".")
				META_ValuePropertyWithSetCall(int	,NumMipMapLevels	,RecalcTiling		,"Upper LOD limit.")
			META_EndProperties
			Macro_EnableRecursiveStreaming(simul::graph::meta::Group)
			Macro_Node(HeightMapNode)
			virtual bool IntersectLineSegment(const float start[],const float end[],float intersection[]);
			virtual float GetHeightAt(float x,float y) const;
			virtual float GetHeightAt(int x,int y) const;
			virtual const float *GetNormalAt(int x,int y) const;
			virtual const float *GetNormalAt(float x,float y) const;
			virtual void SetHeightAt(int x,int y,float h);
			virtual void GetBlockCorners(int i,int j,float*x1,float *x2) const;
			virtual void GetVertexPos(int i,int j,float*x) const;
			//! Get the actual heights in the grid.
			virtual const float *GetData() const;
			//VisualDebugInterface for tools:
			unsigned GetNumLines() const;
			void GetLine(unsigned,float*,float*,int*) const;
			bool Is2D() const {return false;}
		protected:
			void NeedRebuild();
			void RecalcTiling();
			void ChangedWorldSize();
			mutable float *data;
			void ToGrid(int &X,int &Y,float x,float y) const;
			void ToGrid(int &X,int &Y,float &px,float &py,float x,float y) const;
			void Smooth(float strength);
			mutable bool need_rebuild;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif // ifndef WORLD_H

