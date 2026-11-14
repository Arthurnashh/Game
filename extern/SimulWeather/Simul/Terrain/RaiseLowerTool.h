#ifndef SIMUL_TERRAIN_RAISELOWERTOOL_H
#define SIMUL_TERRAIN_RAISELOWERTOOL_H
#include "Simul/Graph/StandardNodes/Editor3DToolInterface.h"
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/Meta/Client.h"
#include "Export.h"
#include <string>
#include "Simul/Math/Vector3.h"
#include "Simul/Graph/StandardNodes/VisualDebugInterface.h"
#include "Simul/Terrain/ToolBase.h"
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
		SIMUL_TERRAIN_EXPORT_CLASS HeightMapInterface;
		SIMUL_TERRAIN_EXPORT_CLASS RaiseLowerTool:public ToolBase
		{
		protected:
			void ChangedToolSettings();
			void Raise(float h);
			bool tool_active;
		public:
			RaiseLowerTool();
			virtual ~RaiseLowerTool();
			META_BeginProperties
				META_ValueRangePropertyWithSetCall(float	,OuterSize		,1.f,10000.f	,ChangedToolSettings	,"Maximum width in metres.")
				META_ValueRangePropertyWithSetCall(float	,InnerSize		,1.f,10000.f	,ChangedToolSettings	,"Peak width in metres.")
				META_ValueRangePropertyWithSetCall(float	,Strength		,.01f,1000.f,ChangedToolSettings	,"Strength in metres per iteration.")
			META_EndProperties
			Macro_EnableRecursiveStreaming(ToolBase)
			Macro_Node(RaiseLowerTool)
			//VisualDebugInterface for tools:
			unsigned GetNumLines() const;
			void GetLine(unsigned,float*,float*,int*) const;
			bool Is2D() const {return false;}
			// Editor3DToolInterface
			virtual void UpdatePositionAndDirection(const float p[],const float d[]);
			virtual bool IsActive() const;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif // ifndef WORLD_H

