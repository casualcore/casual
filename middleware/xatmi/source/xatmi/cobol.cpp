//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/xatmi/cobol.h"
#include "casual/xatmi/internal/code.h"

#include "common/server/context.h"

int tpsvcinfo_cobol_support(const TPSVCINFO** tpsvcinfo,
                            const char** buffer_type,
                            const char** buffer_subtype)
{
   // is an internal error wrap needed? Should not fail in normal
   // cases. Possibly if COBOL api TPSVCSTART is called in a context
   // that isn't an invoked service.
   // should perhaps add a method to context that verifies that
   // there is an active service call, and generates an error
   // if not (TBD !). Then use that method instead of state()
   // (or in additon to state()).
   //return casual::xatmi::internal::error::wrap( [&](){
   //   something...
   //});
   *tpsvcinfo = &casual::common::server::context().state().information.argument;
   *buffer_type = casual::common::server::context().state().buffer_type.c_str();
   *buffer_subtype = casual::common::server::context().state().buffer_subtype.c_str();
   return 0; //Assumne OK. Only for now!!!! prototyping! 
}

void tpreturn_cobol_support( int rval, long rcode, char* data, long len, long flags)
{
   casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::context().normal_return( 
         static_cast< casual::common::flag::xatmi::Return>( rval), rcode, data, len);
   });
}
