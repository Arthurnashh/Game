#include "../wpch.hpp"

#include "menu.hpp"
#include "../drawText.hpp"
#include "../paramFileExt.hpp"
#include "../textbank.hpp"

#define CMD_NOTHING -2
#define CMD_SEPARATOR -1

MenuItem::MenuItem
(
 RString text, int key, int cmd, RString submenu, bool structured, RString textureName
)
 :	_cmd(cmd), _submenu(submenu),
 _enable(true),_visible(true),_check(false),_flagged(false)
{
  if (key >= 0)
  {
    _shortcuts.Realloc(1);
    _shortcuts.Resize(1);
    _shortcuts[0] = key;
  }
  else _shortcuts.Clear();

#if _ENABLE_SPEECH_RECOGNITION
  _speechId = 0;
#endif
  if (text.GetLength() > 0)
  {
    _baseText = structured ? CreateTextStructured(NULL, text) : CreateTextASCII(NULL, text);
    _text = _baseText;
  }
  _priority = -1;

  RString GetPictureName( RString baseName );
  if (textureName.GetLength()>0)
  {
    _cursorTexture = GlobLoadTexture(GetPictureName(textureName));
  }
  else _cursorTexture = NULL;
}

MenuItem::MenuItem(INode *text, int cmd)
 :	_cmd(cmd), _enable(true), _visible(true), _check(false), _flagged(false)
{
  _text = _baseText = text;
  _priority = -1;
  _cursorTexture = NULL;
#if _ENABLE_SPEECH_RECOGNITION
  _speechId = 0;
#endif
}

MenuItem::MenuItem(ParamEntryVal cls, const AttributeTypes &types, const RString *varNames, int varCount)
: _enable(true), _visible(true), _check(false), _flagged(false)
{
  RString text = cls >> "title";
  if (text.GetLength() > 0)
  {
    _baseText = CreateTextStructured(NULL, text);
    _text = _baseText;
  }

  int n = (cls >> "shortcuts").GetSize();
  _shortcuts.Realloc(n);
  _shortcuts.Resize(n);
  for (int i=0; i<n; i++) _shortcuts[i] = (cls >> "shortcuts")[i];

  _cmd = CMD_NOTHING;
  ConstParamEntryPtr entry = cls.FindEntry("command");
  if (entry) _cmd = *entry;

  RString submenu;
  entry = cls.FindEntry("menu");
  if (entry) _submenu = *entry;

  entry = cls.FindEntry("Params");
  if (entry)
  {
    for (int i=0; i<entry->GetEntryCount(); i++)
    {
      ParamEntryVal param = entry->GetEntry(i);
      SetAttribute(param.GetName(), param, types);
    }
  }
  // show condition
  entry = cls.FindEntry("show");
  if (entry) _condVisible.Compile(*entry, varNames, varCount);
  // enable condition
  entry = cls.FindEntry("enable");
  if (entry) _condEnable.Compile(*entry, varNames, varCount);

#if _ENABLE_SPEECH_RECOGNITION
  entry = cls.FindEntry("speechId");
  if (entry) _speechId = *entry;
#endif
#if _VBS3 // command menu eventhandlers
  entry = cls.FindEntry("eventHandler");
  if (entry) _eventHandler = *entry;
#endif

  RString GetPictureName( RString baseName );
  entry = cls.FindEntry("cursorTexture");
  if (entry)
  {
    _cursorTexture = GlobLoadTexture(GetPictureName(*entry));
  }
  else _cursorTexture = NULL;


  entry = cls.FindEntry("priority");
  if (entry) 
    _priority = *entry;
  else _priority = -1;
}

void MenuItem::AddShortcuts(const int *shortcuts, int count)
{
  int n = _shortcuts.Size();
  _shortcuts.Realloc(n + count);
  for (int i=0; i<count; i++) _shortcuts.AddFast(shortcuts[i]);
}

Menu::Menu()
{
  _enable = true;
  _visible = true;
  _selected = 0;
  _contexSensitive = false;
  _locked = false;
}

Menu::Menu(const char *text)
{
  _text = CreateTextStructured(NULL, text);
  _enable = true;
  _visible = true;
  _selected = 0;
  _locked = false;
}

void Menu::Load(RString name, const AttributeTypes &types, const RString *varNames, int varCount)
{
  Load(Pars >> name, types, varNames, varCount);
}

void Menu::Load(ParamEntryVal cls, const AttributeTypes &types, const RString *varNames, int varCount)
{
  _name = cls.GetName();
  RString text = cls >> "title";
  if (text.GetLength() > 0)
  {
    _text = CreateTextStructured(NULL, text);
  }
  ParamEntryVal items = cls >> "items";
  if (items.IsClass())
  {
    for (int i=0; i<items.GetEntryCount(); i++)
    {
      ParamEntryVal entry = items.GetEntry(i);
      if (entry.IsClass()) _items.Add(new MenuItem(entry, types, varNames, varCount));
    }
  }
  else
  {
    Assert(items.IsArray());
    for (int i=0; i<items.GetSize(); i++)
    {
      RString item = items[i];
      _items.Add(new MenuItem(cls >> item, types, varNames, varCount));
    }
  }
  
  if (cls.FindEntry("contexsensitive")) 
    _contexSensitive = cls >> "contexsensitive";

  _items.Compact();
#if _ENABLE_SPEECH_RECOGNITION
  ConstParamEntryPtr entry = cls.FindEntry("vocabulary");
  if (entry) _vocabulary = *entry;
#endif
}

void Menu::AddItem(MenuItem *item)
{
  _items.Add(item);
}

MenuItem *Menu::Find(int cmd)
{
  for (int i=0; i<_items.Size(); i++)
  {
    MenuItem *item = _items[i];
    if (item->_cmd == cmd) return item;
  }
  return NULL;
}

bool Menu::EnableCommand(int cmd, bool enable)
{
  bool found = false;
  for (int i=0; i<_items.Size(); i++)
  {
    MenuItem *item = _items[i];
    if (item->_cmd == cmd)
    {
      item->_enable = enable;
      found = true; // continue (multiple items with the same id can be present)
    }
  }
  return found;
}

bool Menu::ShowCommand(int cmd, bool show)
{
  bool found = false;
  for (int i=0; i<_items.Size(); i++)
  {
    MenuItem *item = _items[i];
    if (item->_cmd == cmd)
    {
      item->_visible = show;
      found = true; // continue (multiple items with the same id can be present)
    }
  }
  return found;
}

bool Menu::ShowAndEnableCommand(int cmd, bool show, bool enable)
{
  bool found = false;
  for (int i=0; i<_items.Size(); i++)
  {
    MenuItem *item = _items[i];
    if (item->_cmd == cmd)
    {
      item->_visible = show;
      item->_enable = enable;
      found = true; // continue (multiple items with the same id can be present)
    }
  }
  return found;
}

bool Menu::SetText(int cmd, RString text)
{
  bool found = false;
  for (int i=0; i<_items.Size(); i++)
  {
    MenuItem *item = _items[i];
    if (item->_cmd == cmd)
    {
      item->_text = CreateTextStructured(NULL, text);
      found = true; // continue (multiple items with the same id can be present)
    }
  }
  return found;
}

bool Menu::ResetText(int cmd)
{
  bool found = false;
  for (int i=0; i<_items.Size(); i++)
  {
    MenuItem *item = _items[i];
    if (item->_cmd == cmd)
    {
      item->_text = item->_baseText;
      found = true; // continue (multiple items with the same id can be present)
    }
  }
  return found;
}

bool Menu::CheckCommand(int cmd, bool check)
{
  bool found = false;
  for (int i=0; i<_items.Size(); i++)
  {
    MenuItem *item = _items[i];
    if (item->_cmd == cmd)
    {
      item->_check = check;
      found = true; // continue (multiple items with the same id can be present)
    }
  }
  return found;
}

void Menu::ValidateIndex()
{
  int n = _items.Size();
  if (n == 0)
  {
    _selected = 0;
    return;
  }

  saturate(_selected, 0, n);
  for (int i=_selected; i<n; i++)
  {
    MenuItem* item = _items[i];
    if (item->_cmd == CMD_SEPARATOR) continue;
    if (!item->_visible || !item->_enable) continue;
    _selected = i;
    return;
  }
  for (int i=0; i<_selected; i++)
  {
    MenuItem* item = _items[i];
    if (item->_cmd == CMD_SEPARATOR) continue;
    if (!item->_visible || !item->_enable) continue;
    _selected = i;
    return;
  }
}

void Menu::NextIndex()
{
  int n = _items.Size();
  if (n == 0)
  {
    _selected = 0;
    return;
  }

  saturate(_selected, 0, n);
  for (int i=_selected+1; i<n; i++)
  {
    MenuItem* item = _items[i];
    if (item->_cmd == CMD_SEPARATOR) continue;
    if (!item->_visible || !item->_enable) continue;
    _selected = i;
    return;
  }
  for (int i=0; i<_selected; i++)
  {
    MenuItem* item = _items[i];
    if (item->_cmd == CMD_SEPARATOR) continue;
    if (!item->_visible || !item->_enable) continue;
    _selected = i;
    return;
  }
}

void Menu::PrevIndex()
{
  int n = _items.Size();
  if (n == 0)
  {
    _selected = 0;
    return;
  }

  saturate(_selected, 0, n);
  for (int i=_selected-1; i>=0; i--)
  {
    MenuItem* item = _items[i];
    if (item->_cmd == CMD_SEPARATOR) continue;
    if (!item->_visible || !item->_enable) continue;
    _selected = i;
    return;
  }
  for (int i=n-1; i>_selected; i--)
  {
    MenuItem* item = _items[i];
    if (item->_cmd == CMD_SEPARATOR) continue;
    if (!item->_visible || !item->_enable) continue;
    _selected = i;
    return;
  }
}

