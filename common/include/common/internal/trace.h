//!
//! trace.h
//!
//! Created on: Dec 30, 2013
//!     Author: Lazan
//!

#ifndef INTERNAL_TRACE_H_
#define INTERNAL_TRACE_H_

#include "common/trace.h"
#include "common/internal/log.h"

namespace casual
{
   namespace common
   {
      namespace trace
      {
         namespace internal
         {

            class Scope : base_trace
            {
            public:
               template< typename T>
               Scope( T&& info) : base_trace( log::internal::trace, std::forward< T>( info)) {}
            };


         } // internal
      } // trace
   } // common

} // casual

#endif // INTERNAL_TRACE_H_
