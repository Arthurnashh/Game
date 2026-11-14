/*!
\file
File integrity check
Calculate checksum and if it does not match, do something clever.
*/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CRC_HPP
#define _CRC_HPP

//! calculate CRC

/*!
CRC code from http://www.faqs.org/faqs/compression-faq/part1/section-25.html
The CRCs this code generates agree with the vendor-supplied Verilog models
of several of the popular FDDI "MAC" chips.
*/

class CRCCalculator
{
	unsigned long _table[256];
	unsigned long _crc;

	void InitTable();

	public:
	//! Constructor
	CRCCalculator();
	//! Calculate CRC of memory block
	unsigned long CRC( const void *data, int len );
	//! Calculate CRC of memory block
	/*!
	\name Incremental interface
	\patch_internal 1.11 Date 8/1/2001 by Ondra
	- New: Incremental CRC interface.
	*/
	//@{
	//! initialize
	void Reset();
	//! add memory block
	void Add(const void *data, int len);
	//! add memory block - OStream like interface (usefull for QIStream::copy)
	__forceinline void write(const void *data, int len) {Add(data,len);}
	//! add signle character
	void Add(char c);
	//! get result of all Add operations since Reset()
	unsigned long GetResult() const {return ~_crc;}
	//}@

};


#endif
