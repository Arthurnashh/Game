#ifdef _MSC_VER
#pragma once
#endif

#ifndef _HI_RES_TIME_HPP
#define _HI_RES_TIME_HPP

typedef __int64 SectionTimeHandle;

SectionTimeHandle StartSectionTime();
__int64 GetSectionResolution();
float GetSectionTime(SectionTimeHandle section);
bool CompareSectionTimeGE(SectionTimeHandle section, float time);

#endif
