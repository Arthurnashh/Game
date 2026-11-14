/*
Copyright (C)2006 bo Ondrej Novak
http://bredy.jinak.cz
novak.on@centrum.cz


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _INTERLOCKED__H_
#define _INTERLOCKED__H_




#if defined(_WIN32)
#include "src-windows/interlocked.h"
#elif defined(_LINUX)
#include "src-linux/interlocked.h"
#else

#error Platform not specified

namespace MultiThread
{

///Interlocked increment
/**
 * @param mem pointer to variable, that will be incremented 
 * @return value of that variable after increment 
 * @note operation increment and retrieve the result is atomic. No other thread
 *  can interfere with it.
 */
long  MTIncrement(long volatile *mem);
///Interlocked increment
/**
* @param mem pointer to variable, that will be decremented
* @return value of that variable after decrement 
* @note operation increment and retrieve the result is atomic. No other thread
*  can interfere with it.
*/
long  MTDecrement(long volatile *mem);
///Interlocked exchange and add
/**
* Interlocked exchange and add provides interlocked ADD operation
* @param mem pointer to variable, that will be subject of operation
* @param value value that will be added.
* @return value of variable before operation
* @note operation retrieve, add and store  is atomic. No other thread
*  can interfere with it.
*/
long  MTExchangeAdd(long volatile *mem, long value);
///Interlocked exchange 
/**
* @param mem pointer to variable, that will be subject of operation
* @param value that will be stored into variable
* @return value of the variable before the operation
* @note operation retrieve exchange and store is atomic. No other thread
*  can interfere with it.
*/
long  MTExchange(long volatile *mem, long value);
///Interlocked exchange 
/**
* @param mem pointer to variable, that will be subject of operation
* @param value that will be stored into variable
* @param comp value that must contain variable to carry out the operation
* @return value of the variable before the operation (even if operation failed
* @note Function atomically test variable for given value (comp) and when
* variable contains this value, new value (value) is stored. Function 
* returns previous value. In case test fails, function does nothing, only returns
* current variable's value.
*/
long  MTCompareExchange(long volatile *mem, long value, long comp);
}
#endif





#endif

