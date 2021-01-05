//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/code/convert.h"

#include "common/log.h"

namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace convert
         {
            namespace to
            {
               code::tx tx( code::xa code)
               {
                  using xa = code::xa;
                  using tx = code::tx;
                  switch( code)
                  {
                     case xa::rollback_unspecified:
                     case xa::rollback_communication:
                     case xa::rollback_deadlock:
                     case xa::rollback_integrity:
                     case xa::rollback_other:
                     case xa::rollback_protocoll:
                     case xa::rollback_timeout:
                     case xa::rollback_transient:
                     case xa::no_migrate:
                     case xa::heuristic_hazard: return tx::hazard;
                     case xa::heuristic_commit:
                     case xa::heuristic_rollback:
                     case xa::heuristic_mix: return tx::mixed;
                     case xa::retry:
                     case xa::read_only: return tx::ok;
                     case xa::ok: return tx::ok;
                     case xa::outstanding_async:
                     case xa::resource_error:
                     case xa::invalid_xid: return tx::no_begin;
                     case xa::argument:
                     case xa::protocol: return tx::protocol;
                     case xa::resource_fail:
                     case xa::duplicate_xid:
                     case xa::outside: return tx::outside;
                     default: return tx::fail;
                  }
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
                     
                     case std::errc::interrupted:        return code::casual::interupted;

                     default: 
                       log::line( log::debug, "no explict conversion for ", code, "(", cast::underlying( code), ") - using: ", code::casual::internal_unexpected_value); 
                       return code::casual::internal_unexpected_value;
                  }

               }
            } // to
         } // convert
      } // code
   } // common
} // casual
