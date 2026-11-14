#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_SUM_CALC_HPP
#define _I_SUM_CALC_HPP

class SumCalculator
{
public:
	//! virtual destructor
	virtual ~SumCalculator() {}
	//! initialize
	virtual void Reset() {}
	//! add memory block
	virtual void Add(const void *data, int len) {}
	//! add memory block - OStream like interface (usefull for QIStream::copy)
	__forceinline void write(const void *data, int len) {Add(data,len);}
	//! get result of all Add operations since Reset()
	virtual unsigned long GetResult() const {return 0;}
};

//! class of callback functions
class SumCalculatorFunctions
{
public:
	virtual ~SumCalculatorFunctions() {}

	//! callback function to load create sum calculator
	virtual SumCalculator *CreateCalculator() {return new SumCalculator;}
};

#endif
