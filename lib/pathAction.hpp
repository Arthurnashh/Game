#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PATH_ACTION_HPP
#define _PATH_ACTION_HPP

enum PathActionType
{
  PATLadderTop,
  PATLadderBottom,
  PATUserActionBegin,
  PATUserActionEnd,
};

class PathAction : public RefCount
{
public:
  virtual PathActionType GetType() const = 0;

  virtual int GetLadderIndex() const {return -1;}
  virtual RString GetActionName() const {return RString();}
};

#endif

