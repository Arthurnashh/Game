#include <Es/essencepch.hpp>
#include <Es/Framework/debugLog.hpp>
#include "reportStack.hpp"

#if !_SUPER_RELEASE

ReportStack GReportStack;

RString ReportStack::GetStack() const
{
  RString result;
  for (ReportStackItem *si = Start(); NotEnd(si); si = Advance(si))
  {
    if (result.IsEmpty())
    {
      result = si->_str;
    }
    else
    {
      result = result + RString("///") + si->_str;
    }
  }
  return result;
}

#endif
