/*
 * (C) 1998-2001 3Dconnexion. All rights reserved. 
 * Permission to use, copy, modify, and distribute this software for all
 * purposes and without fees is hereby grated provided that this copyright
 * notice appears in all copies.  Permission to modify this software is granted
 * and 3Dconnexion will support such modifications only is said modifications are
 * approved by 3Dconnexion.
 *
 */

#ifndef SPACEMOUSE_HPP
#define SPACEMOUSE_HPP

/* SpaceWare Specific Includes */
#include "wpch.hpp"

#include "connexion3d/spwmacro.h"  /* Common macros used by SpaceWare functions. */
#include "connexion3d/si.h"        /* Required for any SpaceWare support within an app.*/
#include "connexion3d/siapp.h"     /* Required for siapp.lib symbols */

class SpaceMouse
{
protected:
  #define NAXIS  6
  #define NBUTTONS 8

  SiHdl       devHdl;       /* Handle to Spaceball Device */

public:
  SpaceMouse(HWND hwnd);
  ~SpaceMouse();

  int  Init(HWND hwnd);
  bool CheckMessage(const MSG& msg, float Axis[NAXIS], bool Buttons[NBUTTONS]);
  bool IsActive() {return devHdl != NULL;};

/*
  void  SbMotionEvent(SiSpwEvent *pEvent);
  void  SbZeroEvent();
  void  SbButtonPressEvent(int buttonnumber);
  void  SbButtonReleaseEvent(int buttonnumber);
*/ 
};

//! global SpaceMouse instance
extern SRef<SpaceMouse> SpaceMouseDev;

#endif