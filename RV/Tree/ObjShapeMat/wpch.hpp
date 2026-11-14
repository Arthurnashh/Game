/*  wpch.hpp
    This is the source for the precompiled header file.
*/

#ifndef WPCH_HPP_INCLUDED
#define WPCH_HPP_INCLUDED

#include <windows.h>
// #include <ole2.h>

//#include "wclass.hpp"

// Optima++ does not know some new keywords
#define explicit
#define debugNew new
//#define AssertDebug Assert

#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>


#include <El/Math/math3d.hpp>
#ifdef WJAVA_NATIVE_DLL
#include "wjnative.hpp"
#endif

//  Include target-wide header files below.
#ifdef _DEBUG
#undef new
#undef delete
#endif

// make for variable local

//#define for if( false ) {} else for


#endif
