#ifdef _MSC_VER
#pragma once
#endif

#ifndef _JVM_UI_HPP
#define _JVM_UI_HPP

#include "jvmInit.hpp"
#include "engine.hpp"

class JVMUI : public RefCount
{
private:
  jclass _jvmClass;

  jmethodID _addPanelID;
  jmethodID _removePanelID;
  jmethodID _movePanelID;
  jmethodID _noPanelID;

  jmethodID _sendMouseID;
  jmethodID _sendMouseWheelID;
  jmethodID _sendKeyID;

  Ref<Texture> _texture;
  
  Ref<Texture> _cursorTexture;
  int _cursorHotspotX;
  int _cursorHotspotY;
  PackedColor _cursorColor;

public:
  JVMUI(int width, int height);
  ~JVMUI();

  bool IsValid()
  {
    return _texture != NULL;
  }

  bool IsVisible() const;

  void AddPanel(RString name, int x, int y, int width, int height);
  void RemovePanel();
  void MovePanel(int x, int y);

  void Draw(const Rect2DPixel &rect);

  static const int KeyFirst = 400;
  static const int KeyTyped = KeyFirst;
  static const int KeyPressed = 1 + KeyFirst;
  static const int KeyReleased = 2 + KeyFirst;

  static const int MouseFirst = 500;
  static const int MouseClicked = MouseFirst;
  static const int MousePressed = 1 + MouseFirst;
  static const int MouseReleased = 2 + MouseFirst;
  static const int MouseMoved = 3 + MouseFirst;
  static const int MouseEntered = 4 + MouseFirst;
  static const int MouseExited = 5 + MouseFirst;
  static const int MouseDragged = 6 + MouseFirst;
  static const int MouseWheel = 7 + MouseFirst;

  bool Simulate(float deltaT);
  bool DoKeyDown(int dikCode);
  bool DoKeyUp(int dikCode);
  bool DoChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
  bool DoIMEChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
  bool DoIMEComposition(unsigned nChar, unsigned nFlags);
private:
  int GetModifiers() const;
  int DIKCodeToKeyCode(int dikCode);
};

#endif