#ifndef BT_PS3PPU_SPAM_DYNAMICS_H
#define BT_PS3PPU_SPAM_DYNAMICS_H

#define BT_PS3PPU_SPAM_DYNAMICS
#ifdef BT_PS3PPU_SPAM_DYNAMICS
#include "stdio.h" 
static volatile bool gbtDebugTrackDynamics = false;	//next idx 199
static volatile unsigned int btDbgIdDynamics[10] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

static void btVector3::btHitPrintDynamics(unsigned int id) 
{
	if(gbtDebugTrackDynamics)
	{
		assert(id >= 0 && id < 320);
		unsigned int idx = id >> 5;	
		unsigned int shift = 1 << (id - (idx << 5));	
		//printf("id %u, idx %u, shift %x\n", id, idx, shift);
		if(btDbgIdDynamics[idx] & shift)	
		{
			btDbgIdDynamics[idx] &= ~shift;
			printf("BT_PS3PPU_VEC to check %i (%s)\n", id, __PRETTY_FUNCTION__);
		}
	}
}
#else
static SIMD_FORCE_INLINE void btVector3::btHitPrint(unsigned int id) {}
#endif

#endif	//BT_PS3PPU_SPAM_COLISION_H