#ifdef _MSC_VER
#pragma once
#endif

#ifndef DIAG_GRAPH_HPP
#define DIAG_GRAPH_HPP

#include <Es/Strings/rString.hpp>
#include <El/Color/colors.hpp>
/// performance/visual feedback graphing
/**
Engine uses this interface to get the graph data
*/
class IDiagGraph: public RefCount
{
  public:
  virtual RString GetName() const = 0;
  /// count of data sets
  virtual int GetDataSetCount() const = 0;
  
  virtual int GetDataSetSize(int set) const = 0;
  /**
  @param dx [out] difference of x from previous data item
  @param dy [out] y data for given item
  */
  virtual void GetDataSetItem(int set, int i, float &dx, float &y) const = 0;

  virtual PackedColor GetDataColor(int set) const = 0;
  
  virtual float GetMaxTime() const = 0;
  
  virtual bool ValidForFPSMode( int fpsMode ) const = 0;
};

#endif

