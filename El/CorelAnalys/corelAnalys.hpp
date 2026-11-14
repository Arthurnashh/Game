#ifndef _EL_COREL_ANALYS_HPP
#define _EL_COREL_ANALYS_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <Es/essencepch.hpp>
#include <Es/Common/fltopts.hpp>
#include <Es/Containers/array.hpp>

/**
\file
Regression (correlation) analysis
*/

struct DataColumn: public AutoArray<float>
{
};

TypeIsMovable(DataColumn)

struct DataSectionAnalysis
{
  AutoArray<double> _correlation;
  // TODO: a1,a0 (see: Bartsch, p. 751 - kovariance)
  //AutoArray<double> _correlation;
  //AutoArray<double> _correlation;
};

template <class Numeric=double>
class RegressionAnalyser
{
  int _count;
  Numeric _sumX;
  Numeric _sumY;
  Numeric _sumX2;
  Numeric _sumY2;
  Numeric _sumXY;
  
  public:
  RegressionAnalyser()
  {
    Reset();
  }
  void Reset()
  {
    _count = 0;
    _sumX = 0;
    _sumY = 0;
    _sumX2 = 0;
    _sumY2 = 0;
    _sumXY = 0;
  }
  void Sample(Numeric x, Numeric y)
  {
    _count++;
    _sumX += x;
    _sumY += y;
    _sumX2 += x*x;
    _sumY2 += y*y;
    _sumXY += x*y;
  }
  Numeric Result() const
  {
    Numeric nom = _sumXY - _sumX*_sumY/_count;
    Numeric denomX = _sumX2 - _sumX*_sumX/_count;
    Numeric denomY = _sumY2 - _sumY*_sumY/_count;
    Numeric r=0;
    if( denomY<1e-10 ) r=nom*1e10;
    else if( denomX<1e-10 ) r=nom*1e10;
    else r=nom/sqrt(denomX*denomY);
    if (r<-1) return -1;
    if (r>+1) return +1;
    return r;
  }
};

struct DataSection: public AutoArray<DataColumn>
{
  DataSection();
  ~DataSection();

  void Normalize();
  /// check correlation between first and other data collumns
  void Analyze( DataSectionAnalysis &ret );
  /// check correlation between any two data collumns
  double AnalyzeColumn( int lead, int col );
};

TypeIsMovable(DataSection)

#endif
