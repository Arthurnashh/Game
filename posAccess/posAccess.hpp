#ifdef _MSC_VER
#pragma once
#endif

#ifndef _POS_ACCESS_HPP
#define _POS_ACCESS_HPP

void PosAccessInit(const char *folder = "");

/// before creating engine all configs need to be loaded
void PosAccessCreateEngine();

void PosAccessDone();

#endif
