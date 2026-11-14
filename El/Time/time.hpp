#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TIME_HPP
#define _TIME_HPP

#include <Es/Common/fltopts.hpp>

/// any time, used for various times
class AbstractTime
{
protected:
	int _time;	// time in miliseconds
public:
	__forceinline AbstractTime() {_time = 0;}
	__forceinline AbstractTime(int time) {_time = time;}

	__forceinline int toInt() const {return _time;}
	__forceinline float toFloat() const {return 1e-3f * _time;}

  __forceinline void setInt(int newTime) { _time = newTime;}

	/** Modulo
	@param val time in sec.
	@return modulo time in sec
	*/
	__forceinline float Mod(float val) const
	{
	  int valI = toLargeInt(val*1000);
	  return (_time%valI)*1e-3f;
	}
	/** Modulo - miliseconds
	@param val time in ms
	@return modulo time in ms
	*/
	__forceinline int ModMs(int ms) const {return _time%ms;}
	// milisecond fraction
	__forceinline int MsFrac() const
	{
		return _time%1000;
	}
protected:
	float Diff( const AbstractTime &x ) const;
};

#define UITimeVal const UITime &

/// UI time - not dependent on simulation time, measured in real world (user)
class UITime : public AbstractTime
{
public:
	__forceinline UITime() : AbstractTime() {}
	__forceinline explicit UITime(int time) : AbstractTime(time) {}
	__forceinline UITime(UITimeVal src) {_time = src._time;}

	void operator +=(float diff);
	void operator -=(float diff);
	float operator -(UITimeVal src) const;
	UITime operator -(float diff) const;
	UITime operator +(float diff) const;

	__forceinline void operator =(UITimeVal src) {_time = src._time;}
	__forceinline bool operator ==(UITimeVal arg) const {return _time == arg._time;}
	__forceinline bool operator !=(UITimeVal arg) const {return _time != arg._time;}
	__forceinline bool operator <(UITimeVal arg) const {return _time < arg._time;}
	__forceinline bool operator >(UITimeVal arg) const {return _time > arg._time;}
	__forceinline bool operator <=(UITimeVal arg) const {return _time <= arg._time;}
	__forceinline bool operator >=(UITimeVal arg) const {return _time >= arg._time;}

	__forceinline float Diff(UITimeVal x ) const {return AbstractTime::Diff(x);}
};
TypeIsSimple(UITime);

#define TimeVal const Time &

/// ingame time (simulation, milisecond resolution)
class Time : public AbstractTime
{
public:
	__forceinline Time() : AbstractTime() {}
	__forceinline explicit Time(int time) : AbstractTime(time) {}
	__forceinline Time(TimeVal src) {_time = src._time;}

	__forceinline Time Floor() const {return Time(_time-MsFrac());}
	__forceinline Time AddMs(int ms) const {return Time(_time+ms);}

	void operator +=(float diff);
	void operator -=(float diff);
	float operator -(TimeVal src) const;
	Time operator -(float diff) const;
	Time operator +(float diff) const;

	__forceinline void operator =(TimeVal src) {_time = src._time;}
	__forceinline bool operator ==(TimeVal arg) const {return _time == arg._time;}
	__forceinline bool operator !=(TimeVal arg) const {return _time != arg._time;}
	__forceinline bool operator <(TimeVal arg) const {return _time < arg._time;}
	__forceinline bool operator >(TimeVal arg) const {return _time > arg._time;}
	__forceinline bool operator <=(TimeVal arg) const {return _time <= arg._time;}
	__forceinline bool operator >=(TimeVal arg) const {return _time >= arg._time;}

	__forceinline float Diff( TimeVal x ) const {return AbstractTime::Diff(x);}
};
TypeIsSimple(Time);

#define UITIME_MAX		UITime(INT_MAX)
#define UITIME_MIN		UITime(-INT_MAX)
#define TIME_MAX			Time(INT_MAX)
#define TIME_MIN			Time(-INT_MAX)

#define TIMESEC_MAX TimeSec(SHRT_MAX)
#define TIMESEC_MIN TimeSec(-SHRT_MAX)

//! ingame time (compact, second resolution)
class TimeSec 
{
	short _time;

	public:
	TimeSec(){}
	__forceinline explicit TimeSec(short time) {_time=time;}
	explicit TimeSec(Time time)
	{
		int timeSec =time.toInt()/1000;
		saturate(timeSec,-SHRT_MAX,SHRT_MAX);
		_time = timeSec;
	}
	operator Time() const {return Time(_time*1000);}

	__forceinline void operator =(TimeSec src) {_time = src._time;}
	__forceinline bool operator ==(TimeSec arg) const {return _time == arg._time;}
	__forceinline bool operator !=(TimeSec arg) const {return _time != arg._time;}
	__forceinline bool operator <(TimeSec arg) const {return _time < arg._time;}
	__forceinline bool operator >(TimeSec arg) const {return _time > arg._time;}
	__forceinline bool operator <=(TimeSec arg) const {return _time <= arg._time;}
	__forceinline bool operator >=(TimeSec arg) const {return _time >= arg._time;}
};

TypeIsSimple(TimeSec);

/**
@for AverageTime computation
*/
struct ValueStamp
{
	float time;
	float value;
};
TypeIsSimple(ValueStamp)


//-----------------------------------------------------------------------------
/**
@value smoother, return average of values of maxTime, must be > 0
*/
//-----------------------------------------------------------------------------
class AverageTimeValue
{
protected:
	AutoArray<ValueStamp> _values;
	float _lastValue;
	float _maxTime;

	/**
	@lerp two values
	*/
	_forceinline float Lerp(float v1, float v2, float l)
	{
		Assert(l >= 0.0f && l <= 1.0f);
		return v1 + (v2 - v1)*l;
	}

	/**
	@append value - create new time stamp
	*/
	void AppendValue(float value, float time)
	{
		ValueStamp& val = _values.Append();
		val.value = value;
		val.time = time;

		//find last valid time
		int i = 0;
		for (; i < _values.Size() && time - _values[i].time > _maxTime; i++)
			;

		if (!i)
			return;

		//update last invalid item to valid one
		float rest = time - _values[i - 1].time - _maxTime;

		_values[i - 1].value = Lerp(_values[i - 1].value, _values[i].value, rest/(_values[i].time - _values[i - 1].time));
		_values[i - 1].time += rest;

		_values.Delete(0, i - 1);
	}

	/**
	@value smoother, return average of values in maxTime
	*/
	float ComputeTimeAverage()
	{
		Assert(_values.Size() > 1 && _maxTime > 0.0f);
		_lastValue = 0.0f;

		const float invMaxTime = 1.0f/_maxTime;
		for (int i = 0; i < _values.Size() - 1; i++)
		{
			float avg = (_values[i + 1].value + _values[i].value)*0.5f;
			float w = (_values[i + 1].time - _values[i].time)*invMaxTime;
			_lastValue += avg*w;
		}
		
		return _lastValue;
	}

public:
	AverageTimeValue()
	{
		Init(1.0);
	}

	AverageTimeValue(float maxTime)
	{
		Init(maxTime);
	}

	void Init(float maxTime)
	{
		_maxTime = floatMax(maxTime,  0.0f);
		_lastValue = 0.0f;
		_values.Clear();
	}

	/*!
	update with absolute time
	\param value	new value
	\param time
	*/
	float UpdateAbsTime(float value, float absTime)
	{
		if (_values.Size())
		{	
			float lastTime = _values.Last().time;
			if (absTime == lastTime)
				return _lastValue;

			if (absTime < lastTime)
				Init(_maxTime);
		}

		AppendValue(value, absTime);

		return (_values.Size() > 1) ? ComputeTimeAverage() : value;
	}
};

//-----------------------------------------------------------------------------
/**
@value smoother, return average of values of maxTime, must be > 0
*/
//-----------------------------------------------------------------------------
class AverageTimeDeltaValue : public AverageTimeValue
{
private:
	//hide
	__forceinline float UpdateAbsTime(float value, float absTime)
	{
		return AverageTimeValue::UpdateAbsTime(value, absTime);
	}

protected:
	float m_fCurrentTime;

public:
	AverageTimeDeltaValue(): AverageTimeValue(), m_fCurrentTime(0.0f)
	{}

	AverageTimeDeltaValue(float maxTime): AverageTimeValue(maxTime), m_fCurrentTime(0.0f)
	{}

	void Init(float maxTime)
	{
		AverageTimeValue::Init(maxTime);
		m_fCurrentTime = 0.0f;
	}

	/*!
	update with delta time
	\param value	new value
	\param time
	*/
	float UpdateDeltaTime(float value, float timeDelta)
	{
		m_fCurrentTime += timeDelta;

		return UpdateAbsTime(value, m_fCurrentTime);
	}
};



#endif
