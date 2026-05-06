//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/code/convert.h"

#include "common/log.h"

namespace casual
{
   namespace common::code::convert::to
   {

      code::tx tx( code::xa code)
      {
         // mapping XA return codes to TX return codes based on documentation/xa/return-code-mapping.md

         switch( code)
         {
            case xa::ok:                    return tx::ok;
            case xa::read_only:             return tx::ok;

            // XA_RB* - rollback codes
            case xa::rollback_unspecified:
            case xa::rollback_communication:
            case xa::rollback_deadlock:
            case xa::rollback_other:
            case xa::rollback_protocoll:
            case xa::rollback_timeout:
            case xa::rollback_transient:
            case xa::rollback_integrity:    return tx::rollback;

            // heuristic outcomes
            case xa::heuristic_hazard:      return tx::hazard;
            case xa::heuristic_mix:         return tx::mixed;
            case xa::heuristic_commit:      return tx::committed;
            case xa::heuristic_rollback:    return tx::rollback;

            // XAER_NOTA - invalid xid, indicates synchronization failure
            case xa::invalid_xid:           return tx::fail;

            // temporary/recoverable RM errors
            case xa::retry:                 return tx::error;
            case xa::resource_error:        return tx::error;
            case xa::duplicate_xid:         return tx::error;
            // not really a specified mapping for no_migrate in the mapping spec 
            case xa::no_migrate:            return tx::error;


            // hard failures
            case xa::resource_fail:         return tx::fail;
            case xa::argument:              return tx::fail;
            case xa::protocol:              return tx::fail;

            case xa::outside:               return tx::outside;

            // outstanding_async is handled by TM internally, not mapped here
            case xa::outstanding_async:     return tx::error;
         }
         return tx::fail;
      }

      code::casual casual( std::errc code)
      {
         switch( code)
         {
            case std::errc::invalid_argument:            return code::casual::invalid_argument;
            case std::errc::no_such_file_or_directory:   return code::casual::invalid_path;

            case std::errc::connection_refused:          return code::casual::communication_refused;
            case std::errc::protocol_error:              return code::casual::communication_protocol;
            case std::errc::address_in_use:              return code::casual::communication_address_in_use;
            case std::errc::address_not_available:       return code::casual::communication_refused;
            case std::errc::connection_reset:            return code::casual::communication_unavailable;
            case std::errc::broken_pipe:                 return code::casual::communication_unavailable;
            
            case std::errc::no_such_process: return code::casual::domain_instance_unavailable;
            
            case std::errc::interrupted:        return code::casual::interrupted;

            default: 
               log::debug( "no explict conversion for ", code, "(", std::to_underlying( code), ") - using: ", code::casual::internal_unexpected_value); 
               return code::casual::internal_unexpected_value;
         }
      }

   } // common::code::convert::to
} // casual
