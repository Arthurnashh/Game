#include "wpch.hpp"

/*!
\file
Implementation file RecoilFunction classe
*/

#include "recoil.hpp"
#include "paramFileExt.hpp"
#include <El/Common/randomGen.hpp>

// recoil effects helpers

int RecoilFunction::FindTime(float time) const
{
  int maxI = -1;
  for (int i=0; i<_queue.Size(); i++)
  {
    if (_queue[i]._time>time) break;
    maxI = i;
  }
  return maxI;
}

void RecoilFunction::AddItem(float time, float offset, float angle)
{
	// get last item time
	if (_queue.Size()>0)
	{
		time += _queue[_queue.Size()-1]._time;
	}
	RecoilItem &item = _queue.Append();
	// TODO: separate X/Y angle
	item._impulseAngX = angle;
	item._impulseAngY = angle*-0.33f;
  item._offsetZ = offset;
	item._time = time;
}

RecoilFunction::RecoilFunction()
:_cfg(NULL)
{
}

RecoilFunction::RecoilFunction( RStringB name )
{
	ParamEntryVal entry = Pars>>"CfgRecoils">>name;

	_cfg = &entry;
	if (entry.GetSize()<=0)
	{
		return;
	}

	// load items from cfg

	for (int i=0; i<entry.GetSize()-2; i+=3)
	{
		float itemTime = entry[i];
		float itemMoveZ = entry[i+1];
		float itemRotX = entry[i+2];

		AddItem(itemTime,itemMoveZ,itemRotX);

	}
	_queue.Compact();
}

RecoilFunction::~RecoilFunction()
{
}
const RStringB &RecoilFunction::GetName() const
{
	return _cfg->GetName();
}

void RecoilFunction::GetRecoil(int index, float time, float &impulseAngX, float &impulseAngY, float &offsetZ) const
{
  if (index<0 || index+1>=_queue.Size())
  {
    // out of range - return zero
    impulseAngX = 0;
    impulseAngY = 0;
    offsetZ = 0;
    return;
  }
  const RecoilItem &item = _queue[index];
  const RecoilItem &next = _queue[index+1];
  float factor = (time-item._time)/(next._time-item._time);
  saturate(factor,0,1);
  impulseAngX = item._impulseAngX*(1-factor)+next._impulseAngX*factor;
  impulseAngY = item._impulseAngY*(1-factor)+next._impulseAngY*factor;
  offsetZ = item._offsetZ*(1-factor)+next._offsetZ*factor;
}

bool RecoilFunction::CheckItemDone(int index, float time) const
{
  if (index<0 || index+1>=_queue.Size())
  {
    // out of range - return done
    return true;
  }
  const RecoilItem &next = _queue[index+1];
  return time>=next._time;
}

bool RecoilFunction::GetFFRamp(float time, RecoilFFRamp &ramp) const
{
  int index = FindTime(time);
  if (index>=0)
  {
    // check if the item is in <startTime,endTime)
    const RecoilItem &item = _queue[index];
    // check next item
    if (index+1<_queue.Size())
    {
      // next item - interpolate between item
      const RecoilItem &next = _queue[index+1];
      ramp._begAmplitude = item._impulseAngX+item._impulseAngY+item._offsetZ;
      ramp._endAmplitude = next._impulseAngX+next._impulseAngY+next._offsetZ;
      ramp._startTime = item._time;
      ramp._duration = next._time-item._time;
    }
    else
    {
      const float lastDuration = 0.2f;
      if (time>item._time+lastDuration)
      {
		    ramp._begAmplitude = 0;
		    ramp._endAmplitude = 0;
		    ramp._duration = 1000;
		    ramp._startTime = item._time;
      }
      else
      {
        // last item - fade to zero
        ramp._begAmplitude = item._impulseAngX+item._impulseAngY+item._offsetZ;
        ramp._endAmplitude = 0;
        ramp._startTime = item._time;
        ramp._duration = lastDuration; // assume some default time for fade out
      }
    }
    return true;
  }
	ramp._begAmplitude = 0;
	ramp._endAmplitude = 0;
	ramp._duration = 1000;
	ramp._startTime = 0;
	return false;
}

bool RecoilFunction::GetTerminated( int index, float time ) const
{
	if (_queue.Size()<=1) return true;
	const RecoilItem &item = _queue[_queue.Size()-1];
	return item._time<=time;
}
