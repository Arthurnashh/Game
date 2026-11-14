#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PILOTHEAD_HPP
#define _PILOTHEAD_HPP

struct PilotHeadPars
{
	float friction;
	float movement;
	float maxAmp;
	float maxSpeed;
	float radius;

	PilotHeadPars();
	PilotHeadPars( ParamEntryPar entry );
};

class PilotHead
{
	Vector3 _pos; // current head position
	Vector3 _speed; // current head speed
	Vector3 _neck,_head; // neutral neck and head positions
	PilotHeadPars _pars;
	bool _valid;

	public:
	void Reset( const Vehicle *vehicle ){_pos=_head,_speed=vehicle->Speed();}
	void Init( Vector3Par neck, Vector3Par head, const Entity *vehicle );
	void SetPars( const PilotHeadPars &pars );
	void SetPars( const char *name );

	PilotHead();

	bool IsValid() const {return _valid;} // some vehicles have no head calculation

	Vector3Val Position() const {return _pos;}
	Vector3Val Neck() const {return _neck;}
	Vector3Val Neutral() const {return _head;}
	Vector3Val Speed() const {return _speed;}
	//void ApplyForces( float deltaT, Vector3Par accel, Vector3Par torque );
	void Move( float deltaT, const Frame &newFrame, const Frame &oldFrame );
};

#endif
