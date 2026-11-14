#include "wpch.hpp"
#include <Es/Common/win.h>

#include "global.hpp"
#include <El/ParamFile/paramFile.hpp>
#include <El/ParamArchive/paramArchive.hpp>

#include <El/Interfaces/iAppInfo.hpp>

#include "stringtableExt.hpp"

#include <time.h>

#include "saveGame.hpp"

bool NoLandscape=false;
bool ObjViewer=false;

INSTANCE_HANDLE hInstApp;
WINDOW_HANDLE   hwndApp;

ParamFile Pars;
ParamFile ExtParsMission;

#if _GAMES_FOR_WINDOWS
RString GameHeader::GetPlayerName() const
{
  const XUSER_SIGNIN_INFO *info = GSaveSystem.GetUserInfo();
  if (!info) return RString();
  return info->szUserName;
}
#endif

LSError Clock::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("timeInYear",_timeInYear,1));
  CHECK(ar.Serialize("timeInYear32",_timeInYear32,1,0));
  if (ar.IsLoading() && _timeInYear32==0)
  {
    // if _timeInYear32 was missing, calculate it
    _timeInYear32 = toLargeInt(_timeInYear*timeInYear32Scale);
  }
  CHECK(ar.Serialize("changeTime",_changeTime,1));
  CHECK(ar.Serialize("timeOfDay",_timeOfDay,1));
  CHECK(ar.Serialize("year", _year,1));
	CHECK(ar.Serialize("timeMultiplier",_timeMultiplier,1));
  return LSOK;
}

void Clock::Read(SimpleStream &in)
{
  in.Read(&_timeInYear, sizeof(_timeInYear));
  in.Read(&_timeInYear32, sizeof(_timeInYear32));
  in.Read(&_changeTime, sizeof(_changeTime));
  in.Read(&_timeOfDay, sizeof(_timeOfDay));
  in.Read(&_timeMultiplier, sizeof(_timeMultiplier));
}

void Clock::Write(SimpleStream &out)
{
  out.Write(&_timeInYear, sizeof(_timeInYear));
  out.Write(&_timeInYear32, sizeof(_timeInYear32));
  out.Write(&_changeTime, sizeof(_changeTime));
  out.Write(&_timeOfDay, sizeof(_timeOfDay));
	out.Write(&_timeMultiplier, sizeof(_timeMultiplier));
}

bool Clock::AdvanceTime( float deltaT )
{
  // deltaT is in years
  _changeTime += deltaT*_timeMultiplier;
  bool ret = fabs(_changeTime) > OneSecond*0.25f;
  if (ret)
  {
    // note: float is not enough for 1 sec precision
    // in year there are 365*24*60*60 = 31536000 sec, float is able to represent 2^24 = 16777216
    
    // timeInYear calculate using fixed point maths
    // convert floating point to fixed point
    float change32 = timeInYear32Scale*_changeTime;
    int change32Int = toLargeInt(change32);
    
    _timeInYear32 += change32Int;

    // handle end of year
    if (_timeInYear32>=timeInYear32Scale)
    {
      _timeInYear32 -= timeInYear32Scale;
      _year++;
    }
    else if (_timeInYear32<0)
    {
      _timeInYear32 += timeInYear32Scale;
      _year--;
    }

    _changeTime = (change32-change32Int)/float(timeInYear32Scale);
    
    _timeInYear = _timeInYear32/float(timeInYear32Scale);
  }
  //_timeOfDay = fastFmod(_timeInYear + _changeTime, OneDay) * (1.0 / OneDay);
  const float InvOneDay=1.0f / OneDay;
  _timeOfDay = (_timeInYear32%timeInYear32OneDay) * (1.0f/timeInYear32OneDay) +_changeTime * InvOneDay;
  return ret;
}

void Clock::SetTime(float time, float date, int year)
{
  // calculate the time in 32 bits fixed point, then convert to float
  saturate(time,0,1);
  saturate(date,0,1);
  saturate(year,1800,3000);
  _year = year;
  
  float date32 = date * timeInYear32Scale;
  float time32 = time * timeInYear32Scale;
  int date32Int = toLargeInt(date32);
  int time32Int = toLargeInt(time32);
  
  _timeInYear32 = date32Int + time32Int;
  // handle end of year
  if (_timeInYear32 >= timeInYear32Scale)
  {
    _timeInYear32 -= timeInYear32Scale;
    _year++;
  }
  else if (_timeInYear32 < 0)
  {
    _timeInYear32 += timeInYear32Scale;
    _year--;
  }

  // rest does not fit to 32 bits
  _changeTime = ((date32 - date32Int) + (time32 - time32Int)) / float(timeInYear32Scale);

  // conversion to float
  _timeInYear = _timeInYear32 / float(timeInYear32Scale);
  const float InvOneDay = 1.0f / OneDay;
  _timeOfDay = (_timeInYear32 % timeInYear32OneDay) * (1.0f / timeInYear32OneDay) + _changeTime * InvOneDay;
}

/*!
\patch 1.44 Date 2/6/2002 by Jirka
- Fixed: Program now properly works with year in date
*/
static const int daysInMonth[]={31,28,31,30,31,30,31,31,30,31,30,31};
int GetDaysInMonth(int year, int month)
{
  if (month == 1 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) return 29;
  else return daysInMonth[month];
}

void Clock::FormatDate(const char *format, char *buffer)
{
  tm tmDate = {0, 0, 0, 0, 0, 0};
  GetDate(tmDate);
  FormatDate(format, buffer, tmDate);
}

void Clock::GetDate(tm &tmDate)
{
  int dayOfYear = _timeInYear32/timeInYear32OneDay;
  float timeOfDay = _timeOfDay;
  if (timeOfDay > 1.0)
  {
    dayOfYear++;
    timeOfDay--;
  }
  if (timeOfDay < 0.0)
  {
    dayOfYear--;
    timeOfDay++;
  }
  Assert(dayOfYear >= 0 && dayOfYear < 365);
  tmDate.tm_year = _year - 1900;
  tmDate.tm_yday = dayOfYear;
  int m = 0;
  while (tmDate.tm_yday >= GetDaysInMonth(_year, m))
  {
    tmDate.tm_yday -= GetDaysInMonth(_year, m);
    m++;
  }
  tmDate.tm_mday = tmDate.tm_yday + 1;
  tmDate.tm_mon = m;
  
  tmDate.tm_yday = 0;
  tmDate.tm_hour = 12; // avoid day shift
  tmDate.tm_min = 0;
  tmDate.tm_sec = 0;
  mktime(&tmDate);
  //Assert(tmDate.tm_yday == dayOfYear);

  timeOfDay *= 24.0f;
  tmDate.tm_hour = toIntFloor(timeOfDay);
  timeOfDay -= tmDate.tm_hour;

  timeOfDay *= 60.0f;
  tmDate.tm_min = toIntFloor(timeOfDay);
  timeOfDay -= tmDate.tm_min;

  timeOfDay *= 60.0f;
  tmDate.tm_sec = toIntFloor(timeOfDay);
  timeOfDay -= tmDate.tm_sec;
}

void Clock::FormatDate(const char *format, char *buffer, const tm &tmDate)
{
  const char *ptrsrc = format;
  char *ptrdst = buffer;
  while (*ptrsrc)
  {
    if (*ptrsrc == '%')
    {
      ptrsrc++;
      switch (*ptrsrc)
      {
      case 0:
        break;
      case 'a':
        strcpy(ptrdst, LocalizeString(IDS_SUNDAY + tmDate.tm_wday));
        while (*ptrdst) ptrdst++;
        ptrsrc++;
        break;
      case 'b':
        strcpy(ptrdst, LocalizeString(IDS_JANUARY + tmDate.tm_mon));
        while (*ptrdst) ptrdst++;
        ptrsrc++;
        break;
      case 'd':
        #ifdef _WIN32
          _itoa(tmDate.tm_mday, ptrdst, 10);
          while (*ptrdst) ptrdst++;
        #else
          ptrdst += sprintf(ptrdst,"%d",tmDate.tm_mday);
        #endif
        ptrsrc++;
        break;
      case 'y':
        #ifdef _WIN32
          _itoa(tmDate.tm_year, ptrdst, 10);
          while (*ptrdst) ptrdst++;
        #else
          ptrdst += sprintf(ptrdst,"%d",tmDate.tm_year);
        #endif
        ptrsrc++;
        break;
      case 'H':
        ptrdst += sprintf(ptrdst,"% 2d",tmDate.tm_hour);
        ptrsrc++;
        break;
      case 'M':
        ptrdst += sprintf(ptrdst,"%02d",tmDate.tm_min);
        ptrsrc++;
        break;
      case 'S':
        ptrdst += sprintf(ptrdst,"%02d",tmDate.tm_sec);
        ptrsrc++;
        break;
      default:
        ptrsrc++;
        break;
      }
    }
    else
      *(ptrdst++) = *(ptrsrc++);
  }
  *(ptrdst++) = 0;
}

float Clock::ScanDateTime(const char *date, const char *time, int &year)
{
  int hour=0,min=0;
  int day=0,month=0;
  sscanf(time,"%d:%d",&hour,&min);
  sscanf(date,"%d/%d/%d",&day,&month,&year);
  saturate(year,1000,9999);
  saturate(day,1,31);
  saturate(month,1,12);
  saturate(hour,0,24);
  saturate(min,0,60);
  day--;
  while( --month>0 )
  {
    day += GetDaysInMonth(year, month - 1);
  }
  return hour*OneHour+min*OneMinute+day*OneDay+0.5*OneSecond;
}

float Clock::ScanDateTime(int year, int month, int day, int hour, int min)
{
  saturate(year,1000,9999);
  saturate(day,1,31);
  saturate(month,1,12);
  saturate(hour,0,24);
  saturate(min,0,60);
  day--;
  while( --month>0 )
  {
    day += GetDaysInMonth(year, month-1);
  }
  return hour*OneHour+min*OneMinute+day*OneDay+0.5*OneSecond;
}


