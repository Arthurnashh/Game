#include <El/elementpch.hpp>

#include "paramArchive.hpp"
#include <El/Interfaces/iClassDb.hpp>

static ClassDbFunctions GClassDbFunctions;
ClassDbFunctions *ParamArchive::_defaultClassDbFunctions = &GClassDbFunctions;
