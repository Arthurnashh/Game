//-----------------------------------------------------------------------------
// File: enf_time.h
//
// Desc: System time
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_TIME_H
#define ENF_TIME_H

#include "enf_config.h"

namespace enf
{
/** @addtogroup System
@{*/
class SerializeOutputAdapter;
class SerializeInputAdapter;

struct PUBLIC_API SDateTime
{
	ushort	Year;
	ushort	Month;
	ushort	DayOfWeek;
	ushort	Day;
	ushort	Hour;
	ushort	Minute;
	ushort	Second;
	ushort	Milliseconds;

	void Serialize(SerializeOutputAdapter* );
	void Serialize(SerializeInputAdapter* );

	SDateTime& operator=(const SDateTime& datetime);
	bool operator==(const SDateTime& datetime) const;
	bool operator<(const SDateTime& datetime) const;
	bool operator>(const SDateTime& datetime) const;

	const SDateTime& SetCurrent();
	const SDateTime& SetCurrentUTC();

	SDateTime& Zero()
	{
		Year = 0;
		Month = 0;
		DayOfWeek = 0;
		Day = 0;
		Hour = 0;
		Minute = 0;
		Second = 0;
		Milliseconds = 0;
		return *this;
	}

	SDateTime():
		Year(0),
		Month(0),
		DayOfWeek(0),
		Day(0),
		Hour(0),
		Minute(0),
		Second(0),
		Milliseconds(0)
	{
	}
};
//@}
}

#endif //ENF_TIME_H