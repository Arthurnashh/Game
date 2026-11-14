#pragma once
#include <el\evaluator\express.hpp>
#include <windows.h>

struct DialogDescriptor;

#define WM_DIALOGUNLOCKNOTIFY (WM_APP+9049)

class GdDialogs
{
  static void SetParentWindowImpl(GameState *gState, const char *windowName);
public:
  static void RegisterToGameState(GameState *gState);

  static bool GetCurrentDialog(const GameState *gs, void *hWnd, DialogDescriptor **ddesc);

  template<class T>
  static void SetParentWindow(GameState *gState, T window)
  {
    char windowName[50];
    _snprintf(windowName,50,"%x",window);
    SetParentWindowImpl(gState, windowName);
  }
};

/* format zapisu dialogu


[ <xsize>, <ysize>, "title",
  ["label",<xs>,<ys>,"text",<align>],  //popisek  
  ["break"],                   //break-line
  ["begin-block"],             //zacatek bloku
  ["end-block"],               //konec bloku
  ["move-to",<x>,<y>]          //presun CP na pozici
  ["move-rel",<x>,<y>]          //presun CP na pozici
  ["textline",<xs>,<ys>,"variable",<align>], //textovy vstup
  ["textarea",<xs>,<ys>,"variable",<align>], //textovy vstup do viceradkoveho okna
  ["button",<xs>,<ys>,"text"], //simple button, currently not useful
  ["ok-button",<xs>,<ys>],     //ok button
  ["cancel-button",<xs>,<ys>], //cancel button
  ["submit",<xs>,<ys>,<id>,"text"],//jako ok button, ale lze zvolit jine ID jenz vraci
  ["browse-button",<xs>,<ys>,"filter"], //Browse button. Variable je nepovinna. Neni-li uvedena, bere se predchozi pole. Je-li uvedena, musi k ni existovat text-input
  ["check-box",<xs>,<ys>,"text","variable"], 
  ["radio-button",<xs>,<ys>,"text","variable",value],  //radiobutton priradi hodnotu value polozce, pokud je vybrana
  ["select",<xs>,<ys>,"variable",["values"]],  
  ["combo-edit",<xs>,<ys>,"variable",["values"]], 
  ["listbox",<xs>,<ys>,"variable",["values"]], 
  ["listview",<xs>,<ys>,"variable",[header][values]], 
  ["hscroll",<xs>,<ys>,"variable",<min>,<max>,<page>], 
  ["vscroll",<xs>,<ys>,"variable",<min>,<max>,<page>], 
  ["htrackbar",<xs>,<ys>,"variable",<min>,<max>,<ticks>,<miniticks>], 
  ["vtrackbar",<xs>,<ys>,"variable",<min>,<max>,<ticks>,<miniticks>], 
  ["padding",<xs>,<ys>], //nastavuje padding ovladacich prvku. Default je 3,3
  ["begin-subframe",<xs>,<ys>,"text"],
  ["end-subframe"],
]

align=0 = default (left + vcenter)
align=1 = left;
align=2 = center;
align=3 = right;
align=a+4 = align with vcenter
*/