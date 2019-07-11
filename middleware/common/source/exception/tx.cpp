//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/exception/tx.h"
#include "common/log/category.h"

namespace casual
{
   namespace common
   {
      namespace exception
      {
         namespace tx
         {
            namespace local
            {
               namespace
               {
                  template< typename E> 
                  void throw_type( const std::string& information)
                  {
                     throw E{ information};
                  }
               } // <unnamed>
            } // local
                        
            void handle( code::tx code, const std::string& information)
            {
               switch( code)
               {
                  case code::tx::ok:
                     break;
                  case code::tx::fail: 
                     local::throw_type< Fail>( information);
                  case code::tx::error: 
                     local::throw_type< Error>( information);
                  case code::tx::protocol: 
                     local::throw_type< Protocol>( information);
                  case code::tx::argument: 
                     local::throw_type< Argument>( information);
                  case code::tx::outside: 
                     local::throw_type< Outside>( information);
                  case code::tx::no_begin: 
                     local::throw_type< no::Begin>( information);
                  case code::tx::not_supported: 
                     local::throw_type< no::Support>( information);
                  default: 
                     throw exception{ code, information};
               }
            } 
            void handle( code::tx code)
            {
               handle( code, "");
            }


            code::tx handle()
            {
               try 
               {
                  throw;
               }
               catch( const common::exception::tx::exception& exception)
               {
                  log::line( code::stream( exception.type()), exception);
                  return exception.type();
               }
               catch( const common::exception::base& exception)
               {
                  log::line( log::category::error, exception);
               }
               catch( const std::exception& exception)
               {
                  log::line( log::category::error, exception);
               }

               return code::tx::fail;
            }
         } // tx
      } // exception
   } // common
} // casual




