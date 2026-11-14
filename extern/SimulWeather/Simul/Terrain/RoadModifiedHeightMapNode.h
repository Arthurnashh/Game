#ifndef SIMUL_TERRAIN_ROADMODIFIEDHEIGHTMAPNODE_H
#define SIMUL_TERRAIN_ROADMODIFIEDHEIGHTMAPNODE_H
#include "Simul/Terrain/HeightMapInterface.h"
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/Meta/Client.h"
#include "Export.h"
#include <string>
#include "Simul/Math/Vector3.h"
#include "Simul/Graph/StandardNodes/VisualDebugInterface.h"
#ifdef _MSC_VER
	#pragma comment(lib,"SimulTerrain")
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
	#pragma warning(disable:4541)
#endif
namespace simul
{
	namespace terrain
	{
		SIMUL_TERRAIN_EXPORT_CLASS RoadModifiedHeightMapNode
			:public HeightMapInterface
			,public simul::graph::meta::ClientUser<HeightMapClientInterface>
			//,public simul::graph::standardnodes::VisualDebugInterface
			,public simul::graph::meta::Node
		{
			void Rebuild();
			void RecalcTiling();
			void ChangedWorldSize();
			float *data;
			void ToGrid(int &X,int &Y,float x,float y) const;
			void ToGrid(int &X,int &Y,float &px,float &py,float x,float y) const;
		public:
			RoadModifiedHeightMapNode();
			virtual ~RoadModifiedHeightMapNode();
			Macro_EnableRecursiveStreaming(RoadModifiedHeightMapNode,simul::graph::meta::Node)
			Macro_Node(RoadModifiedHeightMapNode)
			virtual bool IntersectLineSegment(const float start[],const float end[],float intersection[]);
			virtual float GetHeightAt(float x,float y) const;
			virtual float GetHeightAt(int x,int y) const;
			virtual void SetHeightAt(int x,int y,float h);
			//! Get the actual heights in the grid.
			virtual const float *GetData() const;
			//VisualDebugInterface for tools:
			unsigned GetNumLines() const;
			void GetLine(unsigned,float*,float*) const;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif // ifndef WORLD_H

