#ifndef SIMUL_STANDARDNODES_VISUALDEBUGINTERFACE_H
#define SIMUL_STANDARDNODES_VISUALDEBUGINTERFACE_H
#include "Simul/Graph/StandardNodes/VisualDebugInterface2D.h"
#include "Simul/Graph/StandardNodes/Export.h"
#include <math.h>
#include <vector>
namespace simul
{
	namespace graph
	{
		namespace standardnodes 
		{
			class VisualDebug2D : public VisualDebugInterface2D
			{
				struct Vert
				{
					float x,y;
				};
				struct Line
				{
					Vert v1,v2;
					int colour;
				};
				mutable std::vector<Line> lines;
				float scale;
				float origin_x,origin_y;
				float angle,sa,ca;
			protected:
				VisualDebug2D()
				{
					scale=1.f;
					origin_x=0.f;
					origin_y=0.f;
					angle=0;
					sa=0;
					ca=1.f;
				}
				void SetOriginAndScale(float x0,float y0,float sc)
				{
					origin_x=x0;
					origin_y=y0;
					scale=sc;
				}
				void SetRotation(float a)
				{
					angle=a;
					sa=sin(angle);
					ca=cos(angle);
				}
				
				void ClearLines()
				{
					lines.resize(0);
				}
				void DrawLine(float x1,float y1,float x2,float y2,int c)
				{
					lines.push_back(Line());
					Line &L=lines.back();
					if(!angle)
					{
						L.v1.x=origin_x+x1/scale;
						L.v1.y=origin_y+y1/scale;
						L.v2.x=origin_x+x2/scale;
						L.v2.y=origin_y+y2/scale;
					}
					else
					{
						L.v1.x=origin_x+(ca*x1-sa*y1)/scale;
						L.v1.y=origin_y+(sa*x1+ca*y1)/scale;
						L.v2.x=origin_x+(ca*x2-sa*y2)/scale;
						L.v2.y=origin_y+(sa*x2+ca*y2)/scale;
					}
					L.colour=c;
				}
				void DrawCircle(float x,float y,float r,int c)
				{
					for (int i=0;i<12;i++)
					{
						float angle1=2.f*3.14159f*(float)i/12.f;
						float angle2=2.f*3.14159f*(float)(i+1.f)/12.f;
						DrawLine(x+r*cos(angle1),y+r*sin(angle1),
							x+r*cos(angle2),y+r*sin(angle2),c);
					}
				}
				void DrawBox(float x1,float y1,float x2,float y2,int c)
				{
					DrawLine(x1,y1,x2,y1,c);
					DrawLine(x1,y2,x2,y2,c);
					DrawLine(x1,y1,x1,y2,c);
					DrawLine(x2,y1,x2,y2,c);
				}
				void DrawCentredBox(float x,float y,float w,float h,int c)
				{
					DrawBox(x-.5f*w,y-.5f*h,x+.5f*w,y+.5f*h,c);
				}
				void DrawAngledBox(float x,float y,float a,float w,float h,int c)
				{
					float old_angle=angle;
					float old_origin_x=origin_x;
					float old_origin_y=origin_y;
					SetOriginAndScale(old_origin_x+x/scale,old_origin_y+y/scale,scale);
					SetRotation(old_angle+a);
					DrawCentredBox(0,0,w,h,c);
					SetRotation(old_angle);
					SetOriginAndScale(old_origin_x,old_origin_y,scale);
				}
				void Draw(VisualDebugInterface2D *vdi)
				{
					unsigned n=vdi->GetNum2DLines();
					float x1[3],x2[3];
					int c;
					for(unsigned i=0;i<n;i++)
					{
						vdi->Get2DLine(i,x1,x2,&c);
						DrawLine(x1[0],x1[1],x2[0],x2[1],c);
					}
				}
			public:
				//! Get the number of lines to draw this frame.
				virtual unsigned GetNum2DLines() const
				{
					return (unsigned)lines.size();
				}
				//! Get the end-points of the specified debug line.
				virtual void Get2DLine(unsigned i,float* x1,float* x2,int* c) const
				{
					const Line &L=lines[i];
					x1[0]=L.v1.x;
					x1[1]=L.v1.y;
					x2[0]=L.v2.x;
					x2[1]=L.v2.y;
					*c=L.colour;
					if(i>=lines.size()-1)
						lines.resize(0);
				}
			};
		}
	}
}
#endif