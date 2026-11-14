#ifndef SIMUL_META_EDITOR3DTOOLINTERFACE_H
#define SIMUL_META_EDITOR3DTOOLINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class Editor3DToolInterface
			{
			public:
				virtual void UpdatePositionAndDirection(const float[],const float[])=0;
				virtual void SetShiftAltCtrlState(bool shift,bool alt,bool ctrl)=0;
				virtual void SetMouseButtonState(int btn,bool state)=0;
				virtual bool IsActive() const=0;
			};
		}
	}
}
#endif