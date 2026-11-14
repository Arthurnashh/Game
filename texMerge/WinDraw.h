#pragma once
#include "..\..\img\Picture\picture.hpp"
#include <Es/Types/removeLinks.hpp>//FLY#include <class\removeLinks.hpp>
#include <Es/Containers/cachelist.hpp>//FLY#include <class\cachelist.hpp>

class PictureData: public Picture,public RemoveLinks,public CLTLink<1>
{
};


class WinDraw
{
protected:
	WORD *_data;
	int _x,_y;
	int _w,_h;    
	
public:
	WinDraw( const RECT &client );
	~WinDraw();
	
	// simple primitives
	void PlotNoClip( int x, int y, WORD pixel )
	{
		_data[(_h+_y-1-y)*_w+x-_x]=pixel;
	}
	void Plot( int x, int y, WORD pixel )
	{
		x-=_x;
		y-=_y;
		if( x>=0 && y>=0 && x<_w && y<_h )
		{
			_data[(_h-1-y)*_w+x]=pixel;
		}
  }

  WORD Pixel(int x, int y)
  {
    return _data[(_h+_y-1-y)*_w+x-_x];    
  }

	void DrawCross( int x, int y, int size, WORD pixel );
	void DrawBar( int x, int y, int size, WORD pixel );
	void DrawFrame( int x, int y, int size, WORD pixel );
	void DrawRect( int x, int y, int w, int h, WORD pixel );
	void DrawLine( int x0, int y0, int x1, int y1, WORD pixel );
  void Draw50Transp( int x0, int y0, int x1, int y1, WORD pixel );

	void DrawTexture( PictureData *tex, int x, int y, int w, int h, int angle );
	// draw into the window
	void Draw( HDC hdc, const RECT &client );

};


