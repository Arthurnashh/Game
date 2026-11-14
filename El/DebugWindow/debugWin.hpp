#ifdef _MSC_VER
#pragma once
#endif

#ifndef  _DEBUG_WIN_HPP
#define  _DEBUG_WIN_HPP

#include <El/Color/colors.hpp>
#include <El/Interfaces/iAppInfo.hpp>

class OnPaintContext;

class DebugWindow: public RemoveLinks
{
public:
	DebugWindow(const char *title, bool hScroll = false, bool vScroll = false);
	virtual ~DebugWindow();

	virtual void OnPaint( const OnPaintContext &dc ) = 0;
	virtual void Update();
	virtual void Close();
	virtual bool IsOpen();
	virtual void OnSize(int x, int y, int w, int h) {}
  virtual void OnScroll(int bar, int request, int pos) {}
  virtual void OnKeyDown(unsigned short key, unsigned short repCnt, unsigned short flags) {}
};

class DebugListWindow : public DebugWindow
{
	typedef DebugWindow base;

protected:
	WINDOW_HANDLE _listBox;

public:
	DebugListWindow( const char *title );

	virtual void OnPaint( const OnPaintContext &dc ) {}
	virtual void OnSize(int x, int y, int w, int h);

	void ResetContent();
	void AddString(const char *text);
};

typedef short DebugPixel;

class DebugMemWindow: public DebugWindow
{
	typedef DebugWindow base;

private:
	Temp<DebugPixel> _data;
	int _w,_h;

public:
	DebugMemWindow( const char *title, int w, int h );
	~DebugMemWindow();

	void OnPaint( const OnPaintContext &dc );
	
	DebugPixel Get( int x, int y ) const;
	DebugPixel &Set( int x, int y );
	
	DebugPixel &operator () ( int x, int y ) {return Set(x,y);}
	DebugPixel operator () ( int x, int y ) const {return Get(x,y);}

	void Plot( int x, int y, DebugPixel color );
	void Line( int xb, int yb, int xe, int ye, DebugPixel color );

	static DebugPixel DColorGray( float val )
	{
		int gray=toIntFloor(val*31);
		saturate(gray,0,31);
		return (gray<<10)|(gray<<5)|gray;
	}
	static DebugPixel DColor( ColorVal color )
	{
		int r=toIntFloor(color.R()*31);
		int g=toIntFloor(color.G()*31);
		int b=toIntFloor(color.B()*31);
		saturate(r,0,31);
		saturate(g,0,31);
		saturate(b,0,31);
		return (r<<10)|(g<<5)|b;
	}
};

#endif
