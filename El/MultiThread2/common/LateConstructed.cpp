#include "LateConstructed.h"

namespace BredyLibs
{

template<class T, size_t sz>
HiddenClass<T,sz>::~HiddenClass()
{
  if (B::IsInited()) (*this)->~T();
}

}