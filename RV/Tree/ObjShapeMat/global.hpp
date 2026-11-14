#ifndef _GLOBAL_HPP
#define _GLOBAL_HPP

#include <math.h>
#include <Es/Common/fltOpts.hpp>
#include "optima2mfc.h"

inline WString NumStr( double a )
  {
  char Buf[80];
  sprintf(Buf,"%.3f",a);
  return WString(Buf);
  }

//--------------------------------------------------

inline WString NumStr( int a )
  {
  char Buf[80];
  sprintf(Buf,"%d",a);
  return WString(Buf);
  }

//--------------------------------------------------

//bool PromptForTexture( WWindow *parent, WString &path );

bool DeCzech( char *dst, const char *src );

extern WFilePath DataRoot;

//void SortListView( WListView *list, int item=0  );

#endif


