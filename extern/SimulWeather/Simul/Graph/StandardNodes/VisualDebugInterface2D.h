#ifndef SIMUL_META_VISUALDEBUG2DINTERFACE_H
#define SIMUL_META_VISUALDEBUG2DINTERFACE_H
namespace simul
{
	namespace graph
	{
		namespace standardnodes 
		{
			//! Classes which implement this interface can return a list of lines, in 2D or 3D,
			//! which a view can then show onscreen for debugging.
			class VisualDebugInterface2D
			{
			public:
				//! Get the number of lines to draw this frame.
				virtual unsigned GetNum2DLines() const=0;
				//! Get the end-points of the specified debug line.
				virtual void Get2DLine(unsigned,float*,float*,int*) const=0;
			};

		}
	}
}
#endif
