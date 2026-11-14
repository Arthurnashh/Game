#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CONTEXT_MENU_HPP
#define _CONTEXT_MENU_HPP

#include "uiControls.hpp"
#include "menu.hpp"

class CContextMenu : public Control
{
protected:
  Menu _menu;

  PackedColor _bgColor;
  PackedColor _borderColor;
  PackedColor _separatorColor;
  PackedColor _selBgColor;
  PackedColor _checkedColor;
  PackedColor _enabledColor;
  PackedColor _disabledColor;
  Ref<Font> _font;
  float _rowHeight;

public:
  CContextMenu(ControlsContainer *parent, int idc, ParamEntryPar cls);
  
  Menu &GetMenu() {return _menu;}

  virtual void OnMouseMove(float x, float y, bool active = true);
  virtual void OnLButtonUp(float x, float y);
  virtual bool OnKeyDown(int dikCode);
  virtual bool OnKeyUp(int dikCode);

  virtual void OnDraw(UIViewport *vp, float alpha);
  virtual bool OnKillFocus();
#if _VBS3 // set and getRowHeight
  void SetRowHeight(float height) {_rowHeight = height;}
  float GetRowHeight() {return _rowHeight;}
#endif
  void SelectPlacement(float x, float y);
  virtual void SetFont(Font *font) {_font = font;}
  virtual void SetFontHeight(float h) {_rowHeight = h;}
#if _VBS3 // getFontHeight
  virtual float GetFontHeight() {return _rowHeight;}
#endif
  virtual void SetBgColor(PackedColor color) {_bgColor = color;}

protected:
  int FindItem(float x, float y);
};

#endif

