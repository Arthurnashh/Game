#ifndef SIMULCLOUDS_Texture2D_H
#define SIMULCLOUDS_Texture2D_H


#include "Simul/Sky/Export.h"
namespace simul
{
	namespace sky
	{
		SIMUL_SKY_EXPORT_CLASS Texture2D
		{
			float *tex;
			int X;
			int Y;
			bool Wrap;
			bool Border;
			float Edge;
			int texsize;
			float GetBorder(int i,int j);
		public:
			Texture2D();
			~Texture2D();
			float Get(float u,float v);
			float *Setup(int x,int y,bool wrap,float edge=-1.f);
		};
	}
}
#endif
