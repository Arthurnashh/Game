#include "../wpch.hpp"

#include "contextMenu.hpp"
#include "resincl.hpp"
#include "../drawText.hpp"
#include "../mbcs.hpp"
#include "../dikCodes.h"

const float border = TEXT_BORDER;

CContextMenu::CContextMenu(ControlsContainer *parent, int idc, ParamEntryPar cls)
: Control(parent, CT_CONTEXT_MENU, idc, cls)
{
  _bgColor = GetPackedColor(cls >> "colorBackground");
  _borderColor = GetPackedColor(cls >> "colorBorder");
  _separatorColor = GetPackedColor(cls >> "colorSeparator");
  _selBgColor = GetPackedColor(cls >> "colorSelectBackground");
  _checkedColor = GetPackedColor(cls >> "colorChecked");
  _enabledColor = GetPackedColor(cls >> "colorEnabled");
  _disabledColor = GetPackedColor(cls >> "colorDisabled");
  _font = GEngine->LoadFont(GetFontID(cls >> "font"));
  _rowHeight = cls >> "sizeEx";
}

void CContextMenu::OnMouseMove(float x, float y, bool active)
{
  int index = FindItem(x, y);
  if (index < 0)
  {
    _menu._selected = -1;
    return;
  }
  MenuItem *item = _menu._items[index];
  if (item->_cmd != CMD_SEPARATOR && item->_enable) _menu._selected = index;
  else _menu._selected = -1;
}

void CContextMenu::OnLButtonUp(float x, float y)
{
  if (!_parent) return;

  AddRef();
  int index = FindItem(x, y);
  if (index >= 0)
  {
    MenuItem *item = _menu._items[index];
    if (item->_cmd != CMD_SEPARATOR && item->_enable)
    {
      OnEvent(CEMenuSelected, item->_cmd);
      _parent->OnMenuSelected(item);
    }
  }
  _parent->RemoveContextMenu();
  Release();
}

bool CContextMenu::OnKeyDown(int dikCode)
{
  switch (dikCode)
  {
  case DIK_RETURN:
  case DIK_NUMPADENTER:
    AddRef();
    if (_menu._selected >= 0)
    {
      MenuItem *item = _menu._items[_menu._selected];
      if (item->_cmd != CMD_SEPARATOR && item->_enable)
      {
        OnEvent(CEMenuSelected, item->_cmd);
        _parent->OnMenuSelected(item);
      }
    }
    _parent->RemoveContextMenu();
    Release();
    return true;
  case DIK_ESCAPE:
    _parent->RemoveContextMenu();
    return true;
  case DIK_UP:
  case DIK_LEFT:
    for (int i=_menu._selected-1; i>=0; i--)
    {
      MenuItem *item = _menu._items[i];
      if (item->_cmd != CMD_SEPARATOR && item->_enable)
      {
        _menu._selected = i;
        return true;
      }
    }
    return true;
  case DIK_DOWN:
  case DIK_RIGHT:
    for (int i=_menu._selected+1; i<_menu._items.Size(); i++)
    {
      MenuItem *item = _menu._items[i];
      if (item->_cmd != CMD_SEPARATOR && item->_enable)
      {
        _menu._selected = i;
        return true;
      }
    }
    return true;
  }
  return false;
}

bool CContextMenu::OnKeyUp(int dikCode)
{
  switch (dikCode)
  {
  case DIK_RETURN:
  case DIK_NUMPADENTER:
  case DIK_ESCAPE:
  case DIK_UP:
  case DIK_LEFT:
  case DIK_DOWN:
  case DIK_RIGHT:
    return true;
  }
  return false;
}

void CContextMenu::SelectPlacement(float x, float y)
{
  // Update width and height
  float height = 0;
  float width = 0;
  for (int i=0; i<_menu._items.Size(); i++)
  {
    MenuItem* item = _menu._items[i];
    if (!item->_visible) continue;

    if (item->_cmd == CMD_SEPARATOR)
    {
      height += 2 * border;
    }
    else if (item->_text)
    {
      SetTextFont(item->_text, _font);
      SetTextAttribute(item->_text, "shadow", "false");
      saturateMax(width, GetTextWidthFloat(item->_text, _rowHeight));
      height += _rowHeight;
    }
  }

  _h = height + 2.0f * border;

  // minimum width specified by w value in config
  width = width + 4.0f * border;
  saturateMax(width,_w);
  _w = width;

  // Select placement
  _x = x; _y = y;
  saturateMin(_x, 1 - _w);
  if (_y + _h > 1 && _y > 0.5) _y -= _h;
}

void CContextMenu::OnDraw(UIViewport *vp, float alpha)
{
  // check if menu is ready to draw
  /*
  bool ready = true;
  for (int i=0; i<_menu._items.Size(); i++)
  {
    MenuItem* item = _menu._items[i];
    if (!item->_visible) continue;
    if (item->_cmd == CMD_SEPARATOR) continue;
    if (item->_text && !IsTextReadyToDraw(item->_text, rowHeight)) ready = false;
  }
  if (!ready) return;
  */

  float widthS = SCALED(_w);
  float heightS = SCALED(_h);
  float borderS = SCALED(border);

  int w = GEngine->Width2D();
  int h = GEngine->Height2D();

  MipInfo mip = GEngine->TextBank()->UseMipmap(NULL, 0, 0);
  GEngine->Draw2D(mip, _bgColor, Rect2DPixel(_x * w, _y * h, widthS * w, heightS * h));
  GEngine->DrawLine(Line2DPixel(_x * w, _y * h, (_x + widthS) * w, _y * h), _borderColor, _borderColor);
  GEngine->DrawLine(Line2DPixel((_x + widthS) * w, _y * h, (_x + widthS) * w, (_y + heightS) * h), _borderColor, _borderColor);
  GEngine->DrawLine(Line2DPixel((_x + widthS) * w, (_y + heightS) * h, _x * w, (_y + heightS) * h), _borderColor, _borderColor);
  GEngine->DrawLine(Line2DPixel(_x * w, (_y + heightS) * h, _x * w, _y * h), _borderColor, _borderColor);

  float top = _y + borderS;
  float left = _x + borderS;
  float width = widthS - 2.0f * borderS;
  for (int i=0; i<_menu._items.Size(); i++)
  {
    MenuItem* item = _menu._items[i];
    if (!item->_visible) continue;

    if (item->_cmd == CMD_SEPARATOR)
    {
      top += border;
      GEngine->DrawLine
      (
        Line2DPixel(left * w, top * h, (left + width) * w, top * h),
        _separatorColor, _separatorColor
      );
      top += borderS;
    }
    else if (item->_text)
    {
      if (i == _menu._selected)
        GEngine->Draw2D
        (
          mip, _selBgColor,
          Rect2DPixel(left * w, top * h, width * w, SCALED(_rowHeight) * h)
        );

      PackedColor col;
      if (item->_check)
        col = _checkedColor;
      else if (item->_enable)
        col = _enabledColor;
      else
        col = _disabledColor;
      ::SetTextColor(item->_text, col);
      DrawText(vp, item->_text, Rect2DFloat(left, top, width, SCALED(_rowHeight)), SCALED(_rowHeight));
      top += SCALED(_rowHeight);
    }
  }
}

int CContextMenu::FindItem(float x, float y)
{
  if (x < _x + border || x >= _x + SCALED(_w - border)) return -1;
  if (y < _y + border || y >= _y + SCALED(_h - border)) return -1;
  y -= _y + SCALED(border);
  for (int i=0; i<_menu._items.Size(); i++)
  {
    MenuItem* item = _menu._items[i];
    if (!item->_visible) continue;

    if (item->_cmd == CMD_SEPARATOR)
    {
      y -= 2 * SCALED(border);
      if (y < 0) return i;
    }
    else if (item->_text)
    {
      y -= SCALED(_rowHeight);
      if (y < 0) return i;
    }
  }
  Fail("Unaccessible");
  return -1;
}

bool CContextMenu::OnKillFocus()
{
  if (!Control::OnKillFocus()) return false;

  if (_parent) _parent->RemoveContextMenu();
  return true;
}

