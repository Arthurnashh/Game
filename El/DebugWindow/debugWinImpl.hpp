#ifndef _DEBUG_WIN_IMPL_HPP
#define _DEBUG_WIN_IMPL_HPP

#include <Es/Common/win.h>
#include "debugWin.hpp"

class OnPaintContext
{
	public:
	HDC dc;
};

HWND GetWindowHandle( DebugWindow *window );

#endif


