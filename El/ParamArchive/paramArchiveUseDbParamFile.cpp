#include <El/elementpch.hpp>

#include "paramArchive.hpp"
#include <El/ParamFile/classDbParamFile.hpp>

static ParamClassDbFunctions GParamClassDbFunctions;
ClassDbFunctions *ParamArchive::_defaultClassDbFunctions = &GParamClassDbFunctions;
