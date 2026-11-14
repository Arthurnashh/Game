#include "precomp.h"

#include "Simul/Base/InitializeAndDetectProcessor.h"
#if defined(WIN32) && !defined(__GNUC__)
#include <mmintrin.h>
#include <xmmintrin.h>
#endif
using namespace simul::base;

void CPUId(char *text)
{
#if (defined(WIN32)) && !defined(__GNUC__)
	__asm
	{
		mov esi,text
		mov eax,0
		cpuid
		mov [esi],ebx
		mov [esi+4],edx
		mov [esi+8],ecx
		mov [esi+12],eax
	}
#else
	text[0]='X';
	text[1]='X';
	text[2]='X';
	text[3]='X';
#endif
}
namespace simul
{
	namespace base
	{
		void InitializeAndDetectProcessor()
		{
#if defined(WIN32) && !defined(__GNUC__)
			char text[50];
			CPUId(text);
			for(int i=0;i<50;i++)
				if(text[i]<=2)
					text[i]=0;
			//if(strcmp(text,"GenuineIntel")==0)
			{
				int mxcsr=_mm_getcsr();
				mxcsr&=0xFFFFFFC0;
				mxcsr|=(1<<6)|(1<<15);
				_mm_setcsr(mxcsr);
			}
#endif
		}
	}
}
