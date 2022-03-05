//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/xa.h"
#include "common/code/log.h"
#include "common/code/category.h"
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
