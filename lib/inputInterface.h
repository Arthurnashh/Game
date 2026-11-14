#ifdef _MSC_VER
#pragma once
#endif

#ifndef INPUT_INTERFACE_H
#define INPUT_INTERFACE_H

#include <Es/Types/enum_decl.hpp>

#ifndef DECL_ENUM_USER_ACTION
#define DECL_ENUM_USER_ACTION
DECL_ENUM(UserAction)
#endif

class InputInterface
{
  public:
  enum CurveFactor {CurveFovMouse,Curve1Mouse,CurveFov1,CurveFov0p8};

  virtual float GetAction(UserAction action, bool checkFocus = true, float digitalValue=1.0f, unsigned char level = 1) const = 0;
  virtual bool GetActionToDo(UserAction action, bool reset = true, bool checkFocus = true, bool exclusive = false, unsigned char level = 1, bool blockButtons = false) = 0;
  virtual bool GetActionDo(UserAction action, bool checkFocus=true) = 0;
  virtual float GetActionWithCurve(CurveFactor factorKind, UserAction posAction, UserAction negAction, bool checkFocus = true, bool exclusive = false) const = 0;

  //! combine forward, turbo and fast forward
  float GetActionMoveFastForward(bool checkFocus = true) const;
  float GetActionHeliFastForward(bool checkFocus = true) const;
  float GetActionSeagullFastForward(bool checkFocus = true) const;
  float GetActionCarFastForward(bool checkFocus = true) const;

  float GetActionWithCurveExclusive(CurveFactor factorKind, UserAction posAction, UserAction negAction, bool checkFocus = true) const;

};

class DummyInput: public InputInterface
{
  virtual float GetAction(UserAction action, bool checkFocus = true, float digitalValue=1.0f, unsigned char level = 1) const override
  {
    return 0;
  }
  virtual bool GetActionToDo(UserAction action, bool reset = true, bool checkFocus = true, bool exclusive = false, unsigned char level = 1, bool blockButtons = false) override
  {
    return 0;
  }
  virtual bool GetActionDo(UserAction action, bool checkFocus = true) override
  {
    return 0;
  }
  virtual float GetActionWithCurve(CurveFactor factorKind, UserAction posAction, UserAction negAction, bool checkFocus = true, bool exclusive = false) const override
  {
    return 0;
  }
};

extern DummyInput GDummyInput;

#endif