#include "common/code/convert.h"

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
            } // to
         } // convert
      } // code
   } // common
} // casual
