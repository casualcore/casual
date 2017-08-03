//!
//! casual
//!


#include "common/exception/handle.h"

#include "common/exception/xatmi.h"
#include "common/exception/signal.h"
#include "common/exception/casual.h"
#include "common/exception/tx.h"

#include "common/arguments.h"

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
                  if( out)
                  {
                     *out << exception << std::endl;
                  }
                  else 
                  {
                     error::code::stream( exception.type()) << exception << std::endl;
                  }
                  return exception.code().value();
               }


               int handle( std::ostream* out = nullptr) noexcept
               {
                  try
                  {
                     throw;
                  }
                  //
                  // casual stuff
                  //
                  catch( const exception::casual::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  //
                  // signal stuff
                  //
                  catch( const exception::signal::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  //
                  // xatmi stuff
                  //
                  catch( const exception::xatmi::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  //
                  // tx stuff
                  //
                  catch( const exception::tx::exception& exception)
                  {
                     return local::log( exception, out);
                  }

                  catch( const std::system_error& exception)
                  {
                     if( out) 
                        *out  << exception << std::endl;
                     else 
                        common::log::category::error << exception << std::endl;
                  }

                  catch( const argument::exception::Help& exception)
                  {
                     if( ! out)
                        common::log::category::error << exception << std::endl;
                  }
                  catch( const argument::exception::bash::Completion& exception)
                  {
                     if( ! out)
                        common::log::category::error << exception << std::endl;
                  }
                  
                  catch( const std::exception& exception)
                  {

                     if( out) 
                        *out  << exception << std::endl;
                     else 
                        common::log::category::error << exception << std::endl;
                  }
                  catch( ...)
                  {
                     if( out) 
                        *out  << " - unexpected exception" << std::endl;
                     else 
                        common::log::category::error << " - unexpected exception" << std::endl;
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



