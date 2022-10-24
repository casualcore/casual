//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/tx.h"
#include "common/code/category.h"
#include "common/code/log.h"
#include "common/code/serialize.h"

#include "common/log/category.h"

#include <string>

namespace casual
{
   namespace common::code
   {

      namespace local
      {
         namespace
         {
            struct Category : std::error_category
            {
               const char* name() const noexcept override
               {
                  return "tx";
               }

               std::string message( int code) const override
               {
                  return std::string{ code::description( static_cast< code::tx>( code))};
               }


               // defines the log condition equivalence, so we can compare for logging
               bool equivalent( int code, const std::error_condition& condition) const noexcept override
               {
                  if( ! is::category< code::log>( condition))
                     return false;

                  switch( static_cast< code::tx>( code))
                  {
                     case code::tx::ok:
                     case code::tx::protocol:
                        return condition == code::log::user;

                     // rest is error
                     default:
                        return condition == code::log::error;
                  }
                  
               }
            };

            const auto& category = code::serialize::registration< Category>( 0xf6bbc7b0567d4c9fab0f91a76f50d041_uuid);

         } // <unnamed>
      } // local

      std::string_view description( code::tx code) noexcept
      {
         switch( code)
         {
            case tx::not_supported: return "TX_NOT_SUPPORTED"; // functionality not supported
            case tx::ok: return "TX_OK";
            case tx::outside: return "TX_OUTSIDE"; // transaction is outside
            case tx::rollback: return "TX_ROLLBACK"; // transaction was rolled back
            case tx::mixed: return "TX_MIXED"; // transaction was partially committed and partially rolled back
            case tx::hazard: return "TX_HAZARD"; // transaction may have been partially committed and partially rolled back
            case tx::protocol: return "TX_PROTOCOL_ERROR"; // routine invoked in an improper context
            case tx::error: return "TX_ERROR"; // transient error
            case tx::fail: return "TX_FAIL"; // fatal error
            case tx::argument: return "TX_EINVAL"; // invalid arguments was given
            case tx::committed: return "TX_COMMITTED"; // the transaction was heuristically committed
            case tx::no_begin: return "TX_NO_BEGIN"; // transaction committed but failed to start new one

            case tx::no_begin_rollback: return "TX_ROLLBACK_NO_BEGIN"; // transaction rolled back but failed to start new one
            case tx::no_begin_mixed: return "TX_MIXED_NO_BEGIN"; // mixed commit but failed to start new one
            case tx::no_begin_hazard: return "TX_HAZARD_NO_BEGIN"; // hazard commit but failed to start new one
            case tx::no_begin_committed: return "TX_COMMITTED_NO_BEGIN"; // transaction heuristically committed but failed to start new one
         }
         return "unknown";
      }

      tx operator + ( tx lhs, tx rhs) noexcept
      {
         if( lhs == rhs)
            return lhs;

         auto no_begin_combination = []( auto value)
         {
            switch( value)
            {
               case tx::rollback: return tx::no_begin_rollback;
               case tx::mixed: return tx::no_begin_mixed;
               case tx::hazard: return tx::no_begin_hazard;
               case tx::committed: return tx::no_begin_committed;

               default: return tx::no_begin;
            }
         };

         if( lhs == tx::no_begin)
            return no_begin_combination( rhs);

         if( rhs == tx::no_begin)
            return no_begin_combination( lhs);

         static_assert( tx::no_begin_rollback < tx::no_begin);
         static_assert( tx::no_begin_mixed < tx::no_begin);
         static_assert( tx::no_begin_hazard < tx::no_begin);
         static_assert( tx::no_begin_committed < tx::no_begin);

         if( lhs < tx::no_begin)
            return lhs;
         if( rhs < tx::no_begin)
            return rhs;



         // prioritize between lhs and rhs.

         // range with order of most prioritized first.
         constexpr auto prioritized_range = array::make( 
            tx::not_supported,
            tx::argument,
            tx::hazard,
            tx::mixed,
            tx::fail,
            tx::error,
            tx::committed,
            tx::rollback,
            tx::protocol,
            tx::outside,
            tx::ok
            );

         // if the found 'range' is smaller, it is farther back in the prioritization, hence less valuable.
         if( auto found_lhs = algorithm::find( prioritized_range, lhs))
            if( auto found_rhs = algorithm::find( prioritized_range, rhs))
               return found_rhs.size() <= found_lhs.size() ? lhs : rhs;

         return lhs;
      }

      tx& operator += ( tx& lhs, tx rhs) noexcept
      {
         return lhs = lhs + rhs;
      }

      std::error_code make_error_code( tx code)
      {
         return { static_cast< int>( code), local::category};
      }

   } // common::code
} // casual
