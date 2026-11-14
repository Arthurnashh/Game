#include "corelAnalys.hpp"

/////////////////////////////////////////////////////////////////////////////
// PerfLogSection


DataSection::DataSection()
{
}

DataSection::~DataSection()
{
}

void DataSection::Normalize()
{
  // make sure all columns have equal length
  int maxSize=0;
  for( int c=0; c<Size(); c++ )
  {
    saturateMax(maxSize,Get(c).Size());
  }
  for( int c=0; c<Size(); c++ )
  {
    DataColumn &column=Set(c);
    int oldSize=column.Size();
    if( oldSize<maxSize )
    {
      column.Resize(maxSize);
      for( int l=oldSize; l<maxSize; l++ )
      {
        column[l]=0;
      }
    }
  }

}

double DataSection::AnalyzeColumn(int lead, int col)
{
  const DataColumn &x=Get(lead);
  const DataColumn &y=Get(col);
  
  RegressionAnalyser<double> analyser;
  for( int i=0; i<x.Size(); i++ )
  {
    analyser.Sample(x[i],y[i]);
  }

  return analyser.Result();
}

void DataSection::Analyze( DataSectionAnalysis &ret )
{
  ret._correlation.Resize(Size());
  for( int c=0; c<Size(); c++ )
  {
    double r = AnalyzeColumn(0,c);
    ret._correlation[c]=r;
  }
}

