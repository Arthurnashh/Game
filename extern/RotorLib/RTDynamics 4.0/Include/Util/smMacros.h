/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


/**
*	Macro Guidelines :
*		Always use (p) instead of p. See examples below
*		Always scope with { and }
*/

#ifndef SM_SAFE_DELETE
#define SM_SAFE_DELETE(p)								{ if ( (p) != NULL ){ delete (p); (p) = NULL; } }
#endif
#ifndef SM_SAFE_DELETE2
#define SM_SAFE_DELETE2(p)								{ if ( (p) != NULL ){ delete (p); } }
#endif

//
//	Note:
//		smDelete can only be used on l-values.
//		Therefore this is not possible : smDelete ( manager.get(i) )
//		Use smDelete2 for this purposes
//
#ifndef smDelete
#define smDelete(p)										SM_SAFE_DELETE( (p) )
#endif
#ifndef smDelete2
#define smDelete2(p)									SM_SAFE_DELETE2( (p) )
#endif

//
// Delete the content of the vector containing pointers.
//
#define SM_PURGE_PVECTOR(theData, theType) { \
    std::vector<theType*>::iterator i = theData.begin() ; \
    for ( ; i!= theData.end() ; i++ ) \
    { \
        SM_SAFE_DELETE (*i) ; \
    } \
}

