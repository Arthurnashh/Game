#include "StdAfx.h"
#include "WinDraw.h"

WinDraw::WinDraw( const RECT &client )
{
	_x=client.left;
	_y=client.top;
	_w=client.right-client.left;
	_h=client.bottom-client.top;
	if( _w&1 ) _w++;
	int size=_h*_w;
	_data=new WORD[size];
	
	DWORD *ptr=(DWORD *)_data;
	size>>=1;
	//DWORD col=0x7fff7fff;
	DWORD col=0xaaaaaaaa;
	//ForceStore(col);
	if( size>0 ) do
	{
		*ptr++=col;
	} while( --size>0 );
}


WinDraw::~WinDraw()
{
	delete[] _data;
}

void WinDraw::DrawCross( int x, int y, int size, WORD color )
{
	for( int xg=x-size; xg<=x+size; xg++ ) Plot(xg,y,color);
	for( int yg=y-size; yg<=y+size; yg++ ) Plot(x,yg,color);
}
void WinDraw::DrawBar( int x, int y, int size, WORD color )
{
	for( int xg=x-size; xg<=x+size; xg++ ) for( int yg=y-size; yg<=y+size; yg++ )
	{
		Plot(xg,yg,color);
	}
}

void WinDraw::Draw50Transp( int x0, int y0, int w, int h, WORD pixel )
{
  int l=x0;
  int t=y0;
  int r=x0+w+1;
  int b=y0+h+1;
  if (l<_x) l=_x;
  if (t<_y) t=_y;
  if (r>=_x+_w) r=_x+_w;
  if (b>=_y+_h) b=_y+_h;
  for( int yg=t; yg<b; yg++ ) for( int xg=l+(yg & 1); xg<r; xg+=2 ) 
  {
    PlotNoClip(xg,yg,((pixel & 0x7BDE) + (Pixel(xg,yg) & 0x7BDE))>>1);
  }
}

void WinDraw::DrawRect( int x, int y, int w, int h, WORD pixel )
{
	for( int xx=0; xx<=w; xx++ )
	{
		Plot(x+xx,y,pixel);
		Plot(x+xx,y+h,pixel);
	}
	for( int yy=0; yy<h; yy++ )
	{
		Plot(x,y+yy,pixel);
		Plot(x+w,y+yy,pixel);
	}
}

void WinDraw::DrawFrame( int x, int y, int size, WORD color )
{
	for( int xg=x-size; xg<=x+size; xg++ )
	{
		Plot(xg,y-size,color);
		Plot(xg,y+size,color);
	}
	for( int yg=y-size; yg<=y+size; yg++ )
	{
		Plot(x-size,yg,color);
		Plot(x+size,yg,color);
	}
}

#define MAX_CLIP 15000

void WinDraw::DrawLine( int x0, int y0, int x1, int y1, WORD pixel )
{
	// check if the line must be drawn
	if( x0<0 && x1<0 ) return;
	if( x0>=_w && x1>=_w ) return;
	if( y0<0 && y1<0 ) return;
	if( y0>=_h && y1>=_h ) return;
	
	// too big lines are not drawn correctly
	if( x0<-MAX_CLIP || x0>+MAX_CLIP ) return;
	if( x1<-MAX_CLIP || x1>+MAX_CLIP ) return;
	if( y0<-MAX_CLIP || y0>+MAX_CLIP ) return;
	if( y1<-MAX_CLIP || y1>+MAX_CLIP ) return;
	
	/* DDA */
	//WORD RCnt=1U<<15;
	long x=x0;
	long y=y0;
	long dx=x1-x,adx=dx;
	long dy=y1-y,ady=dy;
	if( adx<0 ) adx=-adx;
	if( ady<0 ) ady=-ady;
	
	if( adx<ady )
	{ /* spise svisla cara */
		long ddx=(dx<<16)/ady;
		long ddy=dy<0 ? -1 : +1;
		long vdd=dy<0 ? -_w : +_w;
		WORD *VAd=&_data[y*_w];
		x<<=16;
		x+=0x8000;
		while( ady-->=0 )
		{
			//if( RCnt&Ras )
			int xg=x>>16,yg=y;
			if( xg>=0 && yg>=0 && xg<_w && yg<_h )
			{
				VAd[xg]=pixel;
			}
			x+=ddx,y+=ddy;
			VAd+=vdd;
			//if( (RCnt>>=1)==0 ) RCnt=1U<<15;
		}
	}
	else if( adx>0 )
	{ /* spise vodorovna cara */
		long ddy=(dy<<16)/adx;
		long ddx=dx<0 ? -1 : +1;
		long oy=y;
		WORD *VAd=&_data[y*_w];
		long vdd=dy<0 ? -_w : +_w;
		y<<=16;
		y+=0x8000;
		while( adx-->=0 )
		{
			long ay=y>>16;
			if( ay!=oy ) VAd+=vdd,oy=ay;
			//if( RCnt&Ras )
			int yg=ay,xg=x;
			if( xg>=0 && yg>=0 && xg<_w && yg<_h )
			{
				VAd[xg]=pixel;
			}
			x+=ddx,y+=ddy;
			//if( (RCnt>>=1)==0 ) RCnt=1U<<15;
		}
	}
}

inline int Extract( int val, int shift, int bits )
{
	return ((val>>shift)&((1<<bits)-1))<<(8-bits);
}

#define Compose(r,g,b) \
	((r)>>(8-sRBits)<<sRShift)| \
	((g)>>(8-sGBits)<<sGShift)| \
	((b)>>(8-sBBits)<<sBShift)


void WinDraw::DrawTexture( PictureData *tex, int dx, int dy, int dw, int dh, int angle )
{
	const int sRShift=10,sGShift=5,sBShift=0;
	const int sRBits=5,sGBits=5,sBBits=5;

	if( dw<=0 ) return;
	if( dh<=0 ) return;

	int th=tex->H()<<16;
	int tw=tex->W()<<16;
	int tlU=0,tlV=0;
	int trU=tw,trV=0;
	int blU=0,blV=th;
	int brU=tw,brV=th;
	angle&=3;
	while( --angle>=0 )
	{
		// tl<-tr<-br<-bl<-tl
		int u=tlU,v=tlV;
		tlU=trU,tlV=trV;
		trU=brU,trV=brV;
		brU=blU,brV=blV;
		blU=u,blV=v;
	}

	int stepUX=(trU-tlU)/dw;
	int stepVX=(trV-tlV)/dw;
	int stepUY=(blU-tlU)/dh;
	int stepVY=(blV-tlV)/dh;
	// perform clipping (to winDraw rectangle)

	int topY=dy;
	int bottomY=dy+dh;
	int leftX=dx;
	int rightX=dx+dw;
	if( topY<_y )
	{
		tlV+=stepVY*(_y-topY);
		tlU+=stepUY*(_y-topY);
		trV+=stepVY*(_y-topY);
		trU+=stepUY*(_y-topY);
		topY=_y;
	}
	if( bottomY>_y+_h )
	{
		bottomY=_y+_h;
	}
	if( leftX<_x )
	{
		tlU+=stepUX*(_x-leftX);
		tlV+=stepVX*(_x-leftX);
		blU+=stepUX*(_x-leftX);
		blV+=stepVX*(_x-leftX);
		leftX=_x;
	}
	if( rightX>_x+_w )
	{
		rightX=_x+_w;
	}

	tex->ConvertARGB();
	tex->RemoveBlackFromAlpha();
	for( int y=topY,u=tlU,v=tlV; y<bottomY; y++,u+=stepUY,v+=stepVY )
	{
		for( int x=leftX,uu=u,vv=v; x<rightX; x++,uu+=stepUX,vv+=stepVX )
		{
			int ui=uu>>16;
			int vi=vv>>16;

			if (ui<0 || ui>=tex->W()) continue;
			if (vi<0 || vi>=tex->H()) continue;
			DWORD pixel=tex->GetPixelARGB(ui,vi);
			// convert to 1555
			int a = (pixel>>24)&0xff;
			int r = (pixel>>16)&0xff;
			int g = (pixel>> 8)&0xff;
			int b = (pixel    )&0xff;

			//if (!showAlpha) a=0xff;
			//if (!showRGB) r=g=b=0;

			if (a<0xff)
			{
				int o = 0x80*(0xff-a);
				r=(r*a+o)>>8;
				g=(g*a+o)>>8;
				b=(b*a+o)>>8;
			}
			int c = Compose(r,g,b);

			PlotNoClip(x,y,c);
		}
	}
}

void WinDraw::Draw( HDC hdc, const RECT &client )
{

	struct Info: public BITMAPINFOHEADER
	{
		DWORD masks[3]; 
	} info;
	
	//BITMAPINFOHEADER info;
	info.biSize=sizeof(BITMAPINFOHEADER);
	info.biWidth=_w;
	info.biHeight=_h;
	info.biPlanes=1;
	info.biBitCount=16;
	//info.biCompression=BI_BITFIELDS;
	info.biCompression=BI_RGB;
	info.biSizeImage=_w*_h*sizeof(WORD);
	info.biXPelsPerMeter=5000;
	info.biYPelsPerMeter=5000;
	//info.biClrUsed=3;
	info.biClrUsed=0;
	info.biClrImportant=0;
	#if 1
		info.masks[0]=0x1f<<10;
		info.masks[1]=0x1f<<5;
		info.masks[2]=0x1f<<0;
	#else
		info.masks[0]=0x1f<<11;
		info.masks[1]=0x3f<<5;
		info.masks[2]=0x1f<<0;
	#endif

	info.biSizeImage=_w*_h*sizeof(WORD);
	
	// create bitmap object from prepared data
	HBITMAP hbmp = ::CreateDIBitmap
		(
		hdc,&info,CBM_INIT,_data,(BITMAPINFO *)&info,DIB_RGB_COLORS
		);
	if( hbmp )
	{
		// copy bitmap into the canvas
		HDC hdcBits = ::CreateCompatibleDC(hdc);
		if( hdcBits )
		{
			BOOL success;
			HGDIOBJ prevObj=::SelectObject(hdcBits,hbmp);
			if( !prevObj )
			{
				::MessageBox(NULL,"SelectObject failed","Error: class WinDraw",MB_ICONERROR|MB_OK);
			}
			success=::BitBlt
			(
				hdc,
				client.left,client.top,client.right-client.left,client.bottom-client.top,
				hdcBits,0,0,SRCCOPY
			);
			if( !success )
			{
				::MessageBox(NULL,"BitBlt failed","Error: class WinDraw",MB_ICONERROR|MB_OK);
			}
			::SelectObject(hdcBits,prevObj);
			::DeleteDC(hdcBits);
		}
		else
		{
			::MessageBox(NULL,"CreateCompatibleDC failed","Error: class WinDraw",MB_ICONERROR|MB_OK);
		}
		::DeleteObject(hbmp);
	}
	else
	{
		::MessageBox(NULL,"CreateDIBitmap failed","Error: class WinDraw",MB_ICONERROR|MB_OK);
	}
}
