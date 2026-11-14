#include "btVector3.h"

#ifdef BT_PS3PPU_SPAM
//next idx 204
//free

static volatile unsigned int gbtDebugTrack;
static volatile bool btDbgId[320];

void btVector3::btHitPrint(unsigned int id) 
{
	if(gbtDebugTrack == 1)
	{
		gbtDebugTrack = 2;
		for(int i = 0; i < 320; ++i)
			btDbgId[i] = false;
	}
	
	if(gbtDebugTrack == 2)
	{
		assert(id >= 0 && id < 320);
		if(!btDbgId[id])	
		{
			btDbgId[id] = true;
			printf("BT_PS3PPU_VEC to check %i\n", id);//, __PRETTY_FUNCTION__);
		}
	}
}
#else
void btVector3::btHitPrint(unsigned int id) {}
#endif