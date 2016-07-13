//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_TRANSACTION_INCLUDE_TRANSACTION_COMMON_H_
#define CASUAL_MIDDLEWARE_TRANSACTION_INCLUDE_TRANSACTION_COMMON_H_


#include "common/internal/log.h"
#include "common/trace.h"

namespace casual
{
   namespace transaction
   {
      extern common::log::Stream& log; // = common::log::internal::transaction;

      struct Trace : common::Trace
      {
         template< typename T>
         Trace( T&& value) : common::Trace( std::forward< T>( value), log) {}
      };

   } // transaction

} // casual

#endif // CASUAL_MIDDLEWARE_TRANSACTION_INCLUDE_TRANSACTION_COMMON_H_
