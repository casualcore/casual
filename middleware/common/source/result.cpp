
#include "common/result.h"

#include "common/code/system.h"
#include "common/signal.h"
#include "common/log/category.h"
#include "common/exception/system.h"

namespace casual
{
   namespace common
   {
      namespace posix
      {
         namespace local
         {
            namespace
            {
               void error( common::code::system last_error)
               {
                  using system = common::code::system;
                  switch( last_error)
                  {
                     case system::interrupted:
                     {
                        common::signal::handle();

                        //
                        // we got a signal we don't have a handle for
                        // We fall through
                        //
                        common::log::line( common::log::category::warning, "no signal handler for signal - ", last_error);

                     } // @fallthrough
                     default:
                     {
                        exception::system::throw_from_code( last_error);
                     }
                  }
               }
            } // <unnamed>
         } // local

         int result( int result)
         {
            if( result == -1)
            {
               local::error( common::code::last::system::error());
            }
            return result;
         }

         namespace log
         {
            void result( int result) noexcept
            {
               if( result == -1)
               {
                  common::log::line( common::log::category::error, common::code::last::system::error());
               }
            }

         } // log

      } // posix
   } // common
} // casual