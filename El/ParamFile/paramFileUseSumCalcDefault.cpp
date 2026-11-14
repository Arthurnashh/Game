#include <El/elementpch.hpp>

#include "paramFile.hpp"
#include <El/Interfaces/iSumCalc.hpp>

static SumCalculatorFunctions GSumCalculatorFunctions;
SumCalculatorFunctions *ParamFile::_defaultSumCalculatorFunctions = &GSumCalculatorFunctions;
