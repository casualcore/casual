//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/xa.h"
#include "common/code/log.h"
#include "common/code/category.h"
#include "common/code/serialize.h"
#include "common/code/casual.h"

#include "common/log/category.h"
#include "common/log/line.h"


#include <string>

namespace casual
{
   namespace common::code
   {

      namespace local
      {
         namespace
         {
            namespace ax
            {
               struct Category : std::error_category
               {
                  const char* name() const noexcept override
                  {
                     return "ax";
                  }

                  std::string message( int code) const override
                  {
                     return std::string{ code::description( static_cast< code::ax>( code))};
                  }

                  // defines the log condition equivalence, so we can compare for logging
                  bool equivalent( int code, const std::error_condition& condition) const noexcept override
                  {
                     if( ! is::category< code::log>( condition))
                        return false;

                     switch( static_cast< code::ax>( code))
                     {
                        case code::ax::error:
                        case code::ax::argument:
                        case code::ax::protocol:
                           return condition == code::log::error;

                        // rest is internal stuff
                        default:
                           return condition == code::log::internal;
                     }
                  }
               };

               const auto& category = code::serialize::registration< Category>( 0x6f60bebf503f4c71b2aaacd3dcc0fac2_uuid);

            } // ax

            namespace xa::severity
            {
               //! Used to rank the xa codes, the lower the enum value (higher up),
               //! the more severe...
               enum struct order : int
               {
                  heuristic_hazard,
                  heuristic_mix,
                  heuristic_commit,
                  heuristic_rollback,
                  resource_fail,
                  resource_error,
                  rollback_integrity,
                  rollback_communication,
                  rollback_unspecified,
                  rollback_other,
                  rollback_deadlock,
                  protocol,
                  rollback_protocoll,
                  rollback_timeout,
                  rollback_transient,
                  argument,
                  no_migrate,
                  outside,
                  outstanding_async,
                  retry,
                  duplicate_xid,
                  invalid_xid,  //! nothing to do?
                  ok,      //! Went as expected
                  read_only,  //! Went "better" than expected
               };

               code::xa convert( severity::order code)
               {
                  switch( code)
                  {
                     case order::heuristic_hazard: return code::xa::heuristic_hazard;
                     case order::heuristic_mix: return code::xa::heuristic_mix;
                     case order::heuristic_commit: return code::xa::heuristic_commit;
                     case order::heuristic_rollback: return code::xa::heuristic_rollback;
                     case order::resource_fail: return code::xa::resource_fail;
                     case order::resource_error: return code::xa::resource_error;
                     case order::rollback_integrity: return code::xa::rollback_integrity;
                     case order::rollback_communication: return code::xa::rollback_communication;
                     case order::rollback_unspecified: return code::xa::rollback_unspecified;
                     case order::rollback_other: return code::xa::rollback_other;
                     case order::rollback_deadlock: return code::xa::rollback_deadlock;
                     case order::protocol: return code::xa::protocol;
                     case order::rollback_protocoll: return code::xa::rollback_protocoll;
                     case order::rollback_timeout: return code::xa::rollback_timeout;
                     case order::rollback_transient: return code::xa::rollback_transient;
                     case order::argument: return code::xa::argument;
                     case order::no_migrate: return code::xa::no_migrate;
                     case order::outside: return code::xa::outside;
                     case order::invalid_xid: return code::xa::invalid_xid;
                     case order::outstanding_async: return code::xa::outstanding_async;
                     case order::retry: return code::xa::retry;
                     case order::duplicate_xid: return code::xa::duplicate_xid;
                     case order::ok: return code::xa::ok;
                     case order::read_only: return code::xa::read_only;
                  }

                  common::log::error( casual::invalid_argument, "invalid xa code - value: ", std::to_underlying( code));
                  return code::xa::resource_error;
               }

               severity::order convert( code::xa code)
               {
                  switch( code)
                  {
                     case code::xa::heuristic_hazard: return order::heuristic_hazard;
                     case code::xa::heuristic_mix: return order::heuristic_mix;
                     case code::xa::heuristic_commit: return order::heuristic_commit;
                     case code::xa::heuristic_rollback: return order::heuristic_rollback;
                     case code::xa::resource_fail: return order::resource_fail;
                     case code::xa::resource_error: return order::resource_error;
                     case code::xa::rollback_integrity: return order::rollback_integrity;
                     case code::xa::rollback_communication: return order::rollback_communication;
                     case code::xa::rollback_unspecified: return order::rollback_unspecified;
                     case code::xa::rollback_other: return order::rollback_other;
                     case code::xa::rollback_deadlock: return order::rollback_deadlock;
                     case code::xa::protocol: return order::protocol;
                     case code::xa::rollback_protocoll: return order::rollback_protocoll;
                     case code::xa::rollback_timeout: return order::rollback_timeout;
                     case code::xa::rollback_transient: return order::rollback_transient;
                     case code::xa::argument: return order::argument;
                     case code::xa::no_migrate: return order::no_migrate;
                     case code::xa::outside: return order::outside;
                     case code::xa::invalid_xid: return order::invalid_xid;
                     case code::xa::outstanding_async: return order::outstanding_async;
                     case code::xa::retry: return order::retry;
                     case code::xa::duplicate_xid: return order::duplicate_xid;
                     case code::xa::ok: return order::ok;
                     case code::xa::read_only: return order::read_only;
                  }

                  common::log::error( casual::invalid_argument, "invalid xa code - value: ", std::to_underlying( code));
                  return order::resource_error;
               }
               

               
            } // xa::severity


         } // <unnamed>
      } // local

      std::string_view description( code::ax code) noexcept
      {
         switch( code)
         {
            case ax::join: return "TM_JOIN";
            case ax::resume: return "TM_RESUME";
            case ax::ok: return "TM_OK";
            case ax::error: return "TMER_TMERR";
            case ax::argument: return "TMER_INVAL";
            case ax::protocol: return "TMER_PROTO";
         }
         return "unknown";
      }

      std::error_code make_error_code( code::ax code)
      {
         return { static_cast< int>( code), local::ax::category};
      }

      code::xa severest( code::xa a, code::xa b) noexcept
      {
         return local::xa::severity::convert( std::min( local::xa::severity::convert( a), local::xa::severity::convert( b)));
      }

      namespace local
      {
         namespace
         {
            namespace xa
            {
               struct Category : std::error_category
               {
                  const char* name() const noexcept override
                  {
                     return "xa";
                  }

                  std::string message( int code) const override
                  {
                     return std::string{ code::description( static_cast< code::xa>( code))};
                  }

                  // defines the log condition equivalence, so we can compare for logging
                  bool equivalent( int code, const std::error_condition& condition) const noexcept override
                  {
                     if( ! is::category< code::log>( condition))
                        return false;

                     switch( static_cast< code::xa>( code))
                     {
                        case code::xa::read_only:
                        case code::xa::ok:
                        case code::xa::outside:
                        case code::xa::argument:
                        case code::xa::no_migrate: 
                           return condition == code::log::internal;

                        // rest is error
                        default:
                           return condition == code::log::error;
                     }
                  }
               };

               const auto& category = code::serialize::registration< Category>( 0xb478a1e5e4ea4bb88e941417e514145b_uuid);

            } // xa  
         } // <unnamed>
      } // local

      std::string_view description( code::xa code) noexcept
      {
         switch( code)
         {
            case xa::rollback_unspecified: return "XA_RBROLLBACK";
            case xa::rollback_communication: return "XA_RBCOMMFAIL";
            case xa::rollback_deadlock: return "XA_RBDEADLOCK";
            case xa::rollback_other: return "XA_RBOTHER";
            case xa::rollback_protocoll: return "XA_RBPROTO";
            case xa::rollback_timeout: return "XA_RBTIMEOUT";
            case xa::rollback_transient: return "XA_RBTRANSIENT";
            case xa::rollback_integrity: return "XA_RBINTEGRITY";


            case xa::no_migrate: return "XA_NOMIGRATE";
            case xa::heuristic_hazard: return "XA_HEURHAZ";
            case xa::heuristic_commit: return "XA_HEURCOM";
            case xa::heuristic_rollback: return "XA_HEURRB";
            case xa::heuristic_mix: return "XA_HEURMIX";
            case xa::retry: return "XA_RETRY";
            case xa::read_only: return "XA_RDONLY";
            case xa::ok: return "XA_OK";

            case xa::outstanding_async: return "XAER_ASYNC";
            case xa::invalid_xid: return "XAER_NOTA";
            case xa::argument: return "XAER_INVAL";
            case xa::protocol: return "XAER_PROTO";
            case xa::resource_fail: return "XAER_RMFAIL";
            case xa::resource_error: return "XAER_RMERR";
            case xa::duplicate_xid: return "XAER_DUPID";
            case xa::outside: return "XAER_OUTSIDE";
         }
         return "unknown";
      }

      std::error_code make_error_code( xa code)
      {
         return { static_cast< int>( code), local::xa::category};
      }

   } // common::code
} // casual
