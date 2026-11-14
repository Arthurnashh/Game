#include <El/elementpch.hpp>

#include "paramFile.hpp"
#include <El/CRC/sumCalcCrc.hpp>

static CRCSumCalculatorFunctions GCRCSumCalculatorFunctions;
SumCalculatorFunctions *ParamFile::_defaultSumCalculatorFunctions = &GCRCSumCalculatorFunctions;
