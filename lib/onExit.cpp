#include "wpch.hpp"

#pragma warning(disable:4074)
#pragma init_seg(compiler)

extern void MemoryInit();
extern void MemoryDone();

class OnExit
{
	public:
	OnExit() {MemoryInit();}
	~OnExit() {MemoryDone();}
};

static OnExit Exit INIT_PRIORITY_URGENT;
