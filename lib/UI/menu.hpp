#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MENU_HPP
#define _MENU_HPP

#include <El/HierWalker/hierWalker.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/SimpleExpression/simpleExpression.hpp>

/// Single row of the menu
class MenuItem : public RefCount
{
public:
  Ref<INode> _baseText;
  Ref<INode> _text;
  FindArray<int> _shortcuts;
  //cursor texture showed in quick command
  Texture *_cursorTexture;
  //when menu change, item with highest priority gets focus
  int _priority;

  RString _submenu;
  int _cmd;
  AttributeList _attributes;

#if _VBS3 // command menu eventhandlers
  RString _eventHandler;
#endif

  bool _enable;
  bool _visible;
  bool _check;
  bool _flagged;

  ExpressionCode _condEnable;
  ExpressionCode _condVisible;

#if _ENABLE_SPEECH_RECOGNITION
  int _speechId;
#endif

public:
  MenuItem(ParamEntryVal cls, const AttributeTypes &types, const RString *varNames, int varCount);
  MenuItem
  (
  RString text, int key, int cmd, RString submenu = RString(), bool structured = true, RString textureName = RString()
  );
  MenuItem(INode *text, int cmd);
  void AddShortcuts(const int *shortcuts, int count);
  void SetAttribute(RString name, RString value, const AttributeTypes &types)
  {
    _attributes.SetAttribute(name, value, types);
  }
  IAttribute *AccessAttribute(int index, const AttributeTypes &types)
  {
    return _attributes.AccessAttribute(index, types);
  }
  IAttribute *AccessAttribute(RString name, const AttributeTypes &types)
  {
    return _attributes.AccessAttribute(name, types);
  }
};

/// (Commanding) menu data structure
class Menu : public RefCount
{
public:
  /// used as a unique identifier of the menu
  RString _name;
  Ref<INode> _text;
  RefArray<MenuItem> _items;
  int _selected;
#if _ENABLE_SPEECH_RECOGNITION
  RString _vocabulary;
#endif

  bool _enable;
  bool _visible;
  bool _contexSensitive;
  bool _locked;

public:
  Menu();
  Menu(const char *text);

  RString GetName() const {return _name;}
  void SetName(RString name) {_name = name;}

  void Load(RString name, const AttributeTypes &types, const RString *varNames, int varCount);
  void Load(ParamEntryVal cls, const AttributeTypes &types, const RString *varNames, int varCount);
  void AddItem(MenuItem *item);

  bool EnableCommand(int cmd, bool enable = true);
  bool ShowCommand(int cmd, bool show = true);
  bool ShowAndEnableCommand(int cmd, bool show = true, bool enable = true);

  bool CheckCommand(int cmd, bool check = true);
  bool SetText(int cmd, RString text);
  bool ResetText(int cmd);

  bool IsContexSensitive()const{return _contexSensitive;};
  void SetContexSensitive(bool contexSensitive) {_contexSensitive = contexSensitive;};

  MenuItem *Find(int cmd);

  void ValidateIndex();
  void PrevIndex();
  void NextIndex();
};

#endif

