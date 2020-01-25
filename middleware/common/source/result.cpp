
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
               void error( common::code::system code, signal::Set mask)
               {
                  exception::system::throw_from_code( code);
               }
            } // <unnamed>
         } // local

         int result( int result)
         {
            if( result == -1)
               local::error( common::code::last::system::error(), signal::mask::current());

            return result;
         }

         int result( int result, signal::Set mask)
         {
            if( result == -1)
               local::error( common::code::last::system::error(), mask);

            return result;
         }

         namespace log
         {
            void result( int result) noexcept
            {
               if( result == -1)
                  common::log::line( common::log::category::error, common::code::last::system::error());
            }

         } // log

         optional_error error( int result) noexcept
         {
            if( result == -1)
               return optional_error{ common::code::last::system::error()};

            return {};
         }
      } // posix
   } // common
} // casual