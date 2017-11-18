//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_TRANSACTION_INCLUDE_TRANSACTION_COMMON_H_
#define CASUAL_MIDDLEWARE_TRANSACTION_INCLUDE_TRANSACTION_COMMON_H_


#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{
   namespace transaction
   {
      extern common::log::Stream& log; // = common::log::category::transaction;

      namespace trace
      {
         extern common::log::Stream log;
      } // trace

      namespace verbose
      {
         extern common::log::Stream log;
      } // verbose


      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };

   } // transaction

} // casual

#endif // CASUAL_MIDDLEWARE_TRANSACTION_INCLUDE_TRANSACTION_COMMON_H_
