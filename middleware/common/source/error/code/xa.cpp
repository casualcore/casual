//!
//! casual
//!

#include "common/error/code/xa.h"
#include "common/log/category.h"
//#include "common/exception/tx.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace error
      {

         namespace code 
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
                           using ax = code::ax;
                           switch( static_cast< code::ax>( code))
                           {
                              case ax::join: return "TM_JOIN";
                              case ax::resume: return "TM_RESUME";
                              case ax::ok: return "TM_OK";
                              case ax::error: return "TMER_TMERR";
                              case ax::argument: return "TMER_INVAL";
                              case ax::protocol: return "TMER_PROTO";
                              default: return "unknown: " + std::to_string( code);
                           }
                        }
                     };

                     const Category category{};

                  } // ax
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
                           using xa = code::xa;
                           switch( static_cast< code::xa>( code))
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

                              default: return "unknown: " + std::to_string( code);
                           }
                        }
                     };

                     const Category category{};

                  } // xa

               } // <unnamed>
            } // local

            std::error_code make_error_code( ax code)
            {
               return { static_cast< int>( code), local::ax::category};
            }

            common::log::Stream& stream( code::ax code)
            {
               switch( code)
               {
                  // transaction
                  case ax::join:
                  case ax::ok:
                  case ax::resume: return common::log::category::transaction;

                  // rest is errors
                  default: return common::log::category::error;
               }
            }

            std::error_code make_error_code( xa code)
            {
               return { static_cast< int>( code), local::xa::category};
            }

            common::log::Stream& stream( code::xa code)
            {
               switch( code)
               {
                  // transaction
                  case xa::read_only:
                  case xa::ok:
                  case xa::outside:
                  case xa::argument:
                  case xa::no_migrate: return common::log::category::transaction;

                  // rest is errors
                  default: return common::log::category::error;
               }
            }
            
            
         } // code 
      } // error


   } // common
} // casual
