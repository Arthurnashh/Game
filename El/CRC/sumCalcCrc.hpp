#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SUM_CALC_CRC_HPP
#define _SUM_CALC_CRC_HPP

#include <El/Interfaces/iSumCalc.hpp>
#include "crc.hpp"

class CRCSumCalculator : public SumCalculator
{
protected:
	CRCCalculator _calculator;

public:
	//! initialize
	virtual void Reset() {_calculator.Reset();}
	//! add memory block
	virtual void Add(const void *data, int len) {_calculator.Add(data, len);}
	//! get result of all Add operations since Reset()
	virtual unsigned long GetResult() const {return _calculator.GetResult();}
};

//! class of callback functions
class CRCSumCalculatorFunctions : public SumCalculatorFunctions
{
public:
	//! callback function to load create sum calculator
	virtual SumCalculator *CreateCalculator() {return new CRCSumCalculator;}
};

#endif
