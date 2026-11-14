// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "gearBox.hpp"
#include "global.hpp"
#include <El/ParamFile/paramFile.hpp>

GearBox::GearBox()
:_gear(0),_gearWanted(0),_gearChangeTime(0)
{
}

void GearBox::SetGears( const AutoArray<float> &gears )
{
  _gears=gears;
}

void GearBox::ChangeGearUp( int gear, float time )
{
  saturate(gear,0,_gears.Size()-1);
  Time chTime=Glob.time+time;
  if( _gearWanted>=gear )
  {
    // change already planned
    if( _gearChangeTime>chTime ) _gearChangeTime=chTime;
    return;
  }
  _gearWanted=gear;
  _gearChangeTime=chTime;
}
void GearBox::ChangeGearDown( int gear, float time )
{
  saturate(gear,0,_gears.Size()-1);
  Time chTime=Glob.time+time;
  if( _gearWanted<=gear )
  {
    // change already planned
    if( _gearChangeTime>chTime ) _gearChangeTime=chTime;
    return;
  }
  _gearWanted=gear;
  _gearChangeTime=chTime;
}


bool GearBox::Change( float speedSize, float duty )
{
  // with heavy duty we will allow higher rpm
  // this way we will get lower gear
  int selGear=_gear;
  // make sure forward / reverse is selected correctly
  if (speedSize>=0 && selGear>=2 || speedSize>0.1f)
  {
    // we need at least gear 2
    if (selGear<2)
    {
      selGear = 2;
      ChangeGearUp(selGear,0); 
    }
    float spdGear=speedSize*_gears[selGear];
    while( selGear<_gears.Size()-1 && spdGear>=duty )
    {
      // we may change gear up
      selGear++;
      float selRpm=spdGear;
      float time=1;
      if( selRpm>1 ) time=1/(1+(selRpm-1)*4);
      ChangeGearUp(selGear,time);
      spdGear=speedSize*_gears[selGear];
    }
    while( selGear>2 && spdGear<=0.6f*duty )
    {
      selGear--;
      float selRpm=1.5*duty-spdGear;
      float time=1;
      if( selRpm>1 ) time=1/(1+(selRpm-1)*4);
      ChangeGearDown(selGear,time);
      spdGear=speedSize*_gears[selGear];
    }
  }
  else
  {
    if (speedSize<-0.1f)
    {
      selGear = 0;
      ChangeGearDown(selGear,0); 
    }
  }

  if( _gearWanted!=_gear )
  {
    if( Glob.time>=_gearChangeTime )
    {
      _gear=_gearWanted;
      return true;
    }
  }
  return false;
}

bool GearBox::Neutral()
{
  // find gear with ratio 0
  if( fabs(_gears[_gear])<1e-3f ) return false;
  for( int selGear=0; selGear<_gears.Size(); selGear++ )
  {
    if( fabs(_gears[selGear])<1e-3f )
    {
      _gear=selGear;
      return true;
    }
  }
  Fail("No neutral");
  return false;
}


/*!
@param maxSpeed max. speed (in km/h)

\patch 5128 Date 2/8/2007 by Ondra
- New: Gear shifting limits can now be defined in the config for each vehicle.
*/

void GearBox::Load( ParamEntryPar entry, float maxSpeed )
{
  int nGears = entry.GetSize();
  _gears.Realloc(nGears);
  _gears.Resize(nGears);
  saturateMax(maxSpeed, 0.01);
  float invMaxSpeed = 1.0f/maxSpeed;
  for (int i=0; i<entry.GetSize(); i++)
  {
    float ratio = entry[i];
    _gears[i] = ratio*invMaxSpeed;
  }
  // gearbox should always contain at least reverse, neutral and forward gear
  if (nGears<3)
  {
    RptF("Invalid gearbox, only %d entry, at least 3 expected",nGears);
    _gears.Realloc(3);
    _gears.Resize(3);
    _gears[0] = -1*invMaxSpeed;
    _gears[0] = 0;
    _gears[1] = 1*invMaxSpeed;
  }
  if (_gears[0]>=0 || fabs(_gears[1])>0 || _gears[2]<=0)
  {
    RptF("Invalid gearbox, first entries need to be reverse, neutral, forward");
  }
  // make sure we are initialized
  _gear = 0;
  // select the neutral gear
  Neutral();
}
