#include "wpch.hpp"
#include <El/ParamFile/paramFile.hpp>
#include "curve3D.hpp"

float LinCurve1D::GetInterpolated(float x) const
{
  // get linear interpolation
  int pos1 = 0;  
  for(; pos1 < _x.Size() && _x[pos1] < x; pos1++);
  
  if (pos1 == 0)  
    return _y[0];    
  else if (pos1 == _x.Size())
    return _y[_x.Size() - 1];
  else
  {
    int pos0 = pos1 - 1;
    float diff = _x[pos1] - _x[pos0];
    float coefpos1 = (x - _x[pos0])/diff;
    return _y[pos0] * (1 - coefpos1) + _y[pos1] * coefpos1;
  }  
};


void LinCurve1D::AddValue(float x, float val)
{
  _x.Add(x);
  _y.Add(val);
}

bool ConfigCurve3D::Load(ParamEntryPar par)
{
  if (!par.IsArray())
    return false; // must be array

  int n = par.GetSize() ;
  if (n > 0)
    _loaded = true;

  for(int i = 0; i < n; i++)
  {
    if (!par[i].IsArrayValue())
      return false;

    const IParamArrayValue& cur = par[i];

    if (cur.GetItemCount() != 2 || (!cur.GetItem(0)->IsFloatValue() &&  !cur.GetItem(0)->IsIntValue()) || !cur.GetItem(1)->IsArrayValue())
      return false;

    _x.AddValue(*cur.GetItem(0), i);

    LinCurve1D& yzCurve = _yz.Append();

    const IParamArrayValue& arr = *cur.GetItem(1);
    int items = arr.GetItemCount() / 2;
    for(int j = 0; j < items; j++)
    {
      yzCurve.AddValue(*arr.GetItem(2*j), *arr.GetItem(2*j+1)); 
    }
  }

  return true;
};

float ConfigCurve3D::GetInterpolated(float x, float y) const
{
  if (!_loaded)
    return y;
  return base::GetInterpolated(x,y);
}
