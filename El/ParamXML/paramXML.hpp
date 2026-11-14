#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_PARAM_XML_HPP
#define _EL_PARAM_XML_HPP

#include <stdio.h>

bool SaveXML(const ParamFile &cfg, const char *name);
bool SaveXML(const ParamFile &cfg, FILE *f);
bool ParseXML(ParamFile &f, const char *name);

#endif

