//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

//! "helpers" for the cobol binding implementation

#include "casual/xatmi/defines.h"

#ifdef __cplusplus
extern "C" {
#endif

//! A function that returns a pointer to a copy of the
//! arguments to the "in progress" service call, to allow
//! the COBOL api TPSVCSTART routine to find the input
//! to the service.  
extern int tpsvcinfo_cobol_support( const TPSVCINFO**, const char** buffer_type, const char** buffer_subtype);

//! Variant of tpreturn() that returns instead of doing longjmp().
//! Needed by COBOL api.
extern void tpreturn_cobol_support( int rval, long rcode, char* data, long len, long flags);

#ifdef __cplusplus
}
#endif
