#ifdef _MSC_VER
#pragma once
#endif

#ifndef _GEAR_BOX_HPP
#define _GEAR_BOX_HPP

#include <Es/Containers/array.hpp>
#include <El/Time/time.hpp>
#include <El/ParamFile/paramFile.hpp>

/// gearbox
/**
Simplistic simulation, used to determine gear based on current speed.
*/

class GearBox
{
  AutoArray<float> _gears;
  /// current gear
  int _gear; 
  // current gear to be set
  int _gearWanted;
  /// when should be the change to _gearWanted applied
  Time _gearChangeTime;

  protected:
  void ChangeGearUp( int gear, float time=1.0 );
  void ChangeGearDown( int gear, float time=1.0 );

  public:
  GearBox();
  void SetGears( const AutoArray<float> &gears );
  /// load definition from the config
  void Load(ParamEntryPar entry, float maxSpeed);
  /// select gear based on current speed and duty
  bool Change( float speed, float duty=1.0f );
  /// change to neutral
  bool Neutral();

  int Gear() const {return _gear;}
  float Ratio() const {return _gears[_gear];}
};


#endif
