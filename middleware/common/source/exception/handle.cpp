//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/exception/handle.h"

#include "common/exception/xatmi.h"
#include "common/exception/signal.h"
#include "common/exception/casual.h"
#include "common/exception/tx.h"

#include "common/argument/exception.h"

#include "common/log/category.h"


namespace casual
{
   namespace common
   {

      namespace exception
      {
         namespace local
         {
            namespace
            {
               template< typename E>
               int log( const E& exception, std::ostream* out)
               {
                  if( ! out)
                     out = &code::stream( exception.type());

                  log::line( *out, exception);

                  return exception.code().value();
               }


               int handle( std::ostream* out = nullptr) noexcept
               {
                  try
                  {
                     throw;
                  }
                  // casual stuff
                  catch( const exception::casual::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  // signal stuff Terminate
                  catch( const exception::signal::Terminate& exception)
                  {
                     log::line( log::category::information, exception.what());
                  }
                  catch( const exception::signal::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  // xatmi stuff
                  catch( const exception::xatmi::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  // tx stuff
                  catch( const exception::tx::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  catch( const argument::exception::user::Help& exception)
                  {
                     if( ! out)
                        log::line( log::category::error, exception.what());
                  }
                  catch( const argument::exception::user::bash::Completion& exception)
                  {
                     if( ! out)
                        log::line( log::category::error, exception.what());
                  }

                  catch( const exception::base& exception)
                  {
                     if( ! out)
                        out = &log::category::error;

                     log::line( *out, exception);
                  }
                  
                  catch( const std::exception& exception)
                  {
                     if( ! out)
                        out = &log::category::error;

                     log::line( *out, exception.what());
                  }
                  catch( ...)
                  {
                     if( ! out)
                        out = &log::category::error;

                     log::line( *out, "unexpected exception");
                  }

                  return -1;
               }
            } // <unnamed>
         } // local

         int handle() noexcept
         {
            return local::handle();
         }

         int handle( std::ostream& out) noexcept
         {
             return local::handle( &out);
         }

      } // exception
   } // common
} // casual



