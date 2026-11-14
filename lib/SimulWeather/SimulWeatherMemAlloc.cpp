#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER
#include "Simul/Base/memAllocator.h"

class SimulWeatherMemAllocatorArma: public simul::base::BaseMemoryAllocator
{
public:
  virtual void *New(size_t size)   {  return GMemFunctions->New(size); }
  virtual void *New(size_t size, const char *file, int line) {return GMemFunctions->New(size, file, line);}
  virtual void Delete(void *mem){return GMemFunctions->Delete(mem);}
  virtual void Delete(void *mem, const char *file, int line) {return GMemFunctions->Delete(mem, file, line);}     
};

namespace simul
{
  namespace base
  {
    BaseMemoryAllocator* BaseMemoryAllocator::GetMemoryAllocator()
    {
      static SimulWeatherMemAllocatorArma alloc;
      return &alloc;
    }
  }
}

#endif //#if _ARMA3_SIMUL_WEATHER