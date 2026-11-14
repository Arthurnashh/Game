#ifndef MOUSEINTERFACE_H
#define MOUSEINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class MouseInterface
			{
			public:
				virtual void HandleMouseMoved(int x,int y)=0;
				virtual void HandleMouseWheel(int z)=0;
				virtual void HandleMousePressed(int button_number,int x,int y)=0;	//< Left = 0, Right=1, Middle=2...
				virtual void HandleMouseReleased(int button_number)=0;	//< Left = 0, Right=1, Middle=2...
			};
		}
	}
}
#endif

