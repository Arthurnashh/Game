/*************************************************************************
 *
 *              INTEL CORPORATION PROPRIETARY INFORMATION
 *  This software is supplied under the terms of a license agreement or
 *  nondisclosure agreement with Intel Corporation and may not be copied
 *  or disclosed except in accordance with the terms of that agreement.
 *
 *    Copyright(c) 2002-2009 Intel Corporation. All Rights Reserved.
 *
 ************************************************************************/
/* $Revision: 1.13.4.3 $ * $Date: 2007/04/11 15:47:33 $ */
#ifndef _LIBITTNOTIFY_H_
#define _LIBITTNOTIFY_H_

#ifndef __ITT_INTERNAL_INCLUDE
#  if defined WIN32 || defined _WIN32
#    pragma message("WARNING!!! Include file libittnotify.h is deprecated and should not be included anymore")
#  else /* WIN32 */
#    warning "Include file libittnotify.h is deprecated and should not be included anymore"
#  endif /* WIN32 */
#endif /* __ITT_INTERNAL_INCLUDE */

#include <stddef.h>
#include "ittnotify.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const __itt_state_t __itt_state_err;
extern const __itt_event   __itt_event_err;
extern const int __itt_err;

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* _LIBITTNOTIFY_H_ */
