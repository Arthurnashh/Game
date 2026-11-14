#if _MSC_VER>=1100
#pragma once
#endif

#ifndef _ES_REPORTSTACK_HPP
#define _ES_REPORTSTACK_HPP

#include <Es/Containers/listBidir.hpp>
#include <Es/Strings/rString.hpp>

#if !_ENABLE_REPORT
  #define REPORTSTACKITEM(a)
  #define GETREPORTSTACK() "ReportStack not available"
  #define RPTSTACK()
  #define LOGSTACK()
  #define CHECKREPORTSTACK() false
#else
  class ReportStackItem;

  //! Class to represent a simplified stack copy with additional informations
  class ReportStack : public TListBidir<ReportStackItem, TLinkBidirD>
  {
  public:
    //! Function to return all stack informations in the single string
    RString GetStack() const;
  };

  extern ReportStack GReportStack;

  //! Class to represent item of the stack (allocated on the real stack)
  class ReportStackItem : public TLinkBidirD
  {
    friend class ReportStack;
    //! Info on the stack
    RString _str;
  public:
    //! Constructor
    ReportStackItem(const RString &info):_str(info)
    {
      GReportStack.Add(this);
    }
  };

  //! Put new item on the stack (will be removed automatically at the end of the scope)
  #define REPORTSTACKITEM(a) ReportStackItem reportStackItem(a)
  //! Return string that represents the whole stack context
  #define GETREPORTSTACK() cc_cast(GReportStack.GetStack())
  #define CHECKREPORTSTACK() (!GReportStack.Empty())
  //!{ Report the entire stack
  #define RPTSTACK() RptF("ReportStack: %s", GETREPORTSTACK())
  #define LOGSTACK() LogF("ReportStack: %s", GETREPORTSTACK())
  //!}
#endif

#endif
