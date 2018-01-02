//!
//! casual
//!

#include "common/code/tx.h"
#include "common/log/category.h"
#include "common/exception/tx.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace code
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
                     switch( static_cast< code::tx>( code))
                     {
                        case tx::not_supported: return "TX_NOT_SUPPORTED: functionality not supported";
                        case tx::ok: return "TX_OK";
                        case tx::outside: return "TX_OUTSIDE: transaction is outside";
                        case tx::rollback: return "TX_ROLLBACK: transaction was rolled back";
                        case tx::mixed: return "TX_MIXED: transaction was partially committed and partially rolled back";
                        case tx::hazard: return "TX_HAZARD: transaction may have been partially committed and partially rolled back";
                        case tx::protocol: return "TX_PROTOCOL_ERROR: routine invoked in an improper context";
                        case tx::error: return "TX_ERROR: transient error";
                        case tx::fail: return "TX_FAIL: fatal error";
                        case tx::argument: return "TX_EINVAL: invalid arguments was given";
                        case tx::committed: return "TX_COMMITTED: the transaction was heuristically committed";
                        case tx::no_begin: return "TX_NO_BEGIN: transaction committed but failed to start new one";

                        case tx::no_begin_rollback: return "TX_ROLLBACK_NO_BEGIN: transaction rolled back but failed to start new one";
                        case tx::no_begin_mixed: return "TX_MIXED_NO_BEGIN: mixed commit but failed to start new one";
                        case tx::no_begin_hazard: return "TX_HAZARD_NO_BEGIN: hazard commit but failed to start new one";
                        case tx::no_begin_committed: return "TX_COMMITTED_NO_BEGIN: transaction heuristically committed but failed to start new one";

                        default: return "unknown";
                     }
                  }
               };

               const Category category{};

            } // <unnamed>
         } // local

         std::error_code make_error_code( tx code)
         {
            return { static_cast< int>( code), local::category};
         }

         common::log::Stream& stream( code::tx code)
         {
            switch( code)
            {
               // transaction
               case tx::not_supported:
               case tx::ok:
               case tx::outside:
               case tx::argument:
               case tx::rollback:
               case tx::no_begin: return common::log::category::transaction;

               // rest is errors
               default: return common::log::category::error;
            }
         }

      } // code
   } // common
} // casual
