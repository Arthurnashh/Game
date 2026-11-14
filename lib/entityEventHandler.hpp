#pragma once

/*
This header needs to be used in the source which use OnEventRetAny / OnEventAny.
It is separated to save compilation time (to avoid processing templates when not used)
*/
#ifndef entityEventHandler_h__
#define entityEventHandler_h__

#include "tupleToGameArray.hpp"

#define EEVENT_ARGS(...) std::make_tuple(__VA_ARGS__)

template <class Ret, class InputTuple>
Ret EntityAI::OnEventAnyRet(EntityEvent event, const InputTuple &input)
{
  if (_eventHandlers[event].IsEmpty()) return Ret();
  GameValue value = GWorld->GetGameState()->CreateGameValue(GameArray);
  GameArrayType &arguments = value;
  arguments.Add(GameValueExt(this,GameValExtObject));
  TupleToGameArray<const InputTuple,std::tuple_size<InputTuple>::value>::Do(input,arguments);
  return FromGameValue<Ret>::Do(OnEventRet(event, value));
}

template <class InputTuple>
void EntityAI::OnEventAny(EntityEvent event, const InputTuple &input)
{
  if (_eventHandlers[event].IsEmpty()) return;
  GameValue value = GWorld->GetGameState()->CreateGameValue(GameArray);
  GameArrayType &arguments = value;
  arguments.Add(GameValueExt(this,GameValExtObject));
  TupleToGameArray<const InputTuple,std::tuple_size<InputTuple>::value>::Do(input,arguments);
  return OnEvent(event, value);
}


#endif // entityEventHandler_h__
