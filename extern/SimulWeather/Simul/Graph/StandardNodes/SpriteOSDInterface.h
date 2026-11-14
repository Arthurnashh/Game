#ifndef SIMUL_META_SPRITEOSDINTERFACE_H
#define SIMUL_META_SPRITEOSDINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
// Graph
// 
			class SpriteOSDInterface
			{
			public:
				virtual int GetNumSprites()=0;
				virtual const char *GetSpriteName(int index)=0;
				virtual float GetSpriteCentreX(int index)=0;
				virtual float GetSpriteCentreY(int index)=0;
				virtual float GetSpriteWidth(int index)=0;
				virtual float GetSpriteHeight(int index)=0;
				virtual float GetSpriteRotation(int index)=0;
			};
// 
		}
	}
}
#endif