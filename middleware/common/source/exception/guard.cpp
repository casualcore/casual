//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/guard.h"

#include "common/exception/format.h"

#include "common/event/send.h"
#include "common/terminal.h"

#include <iostream>

namespace casual
{
   namespace common::exception
   {
      namespace main
      {
         namespace log
         {
            namespace detail
            {
               int void_return( common::function< void()> callable)
               {
                  try 
                  {
                     callable();
                  }
                  catch( ...)
                  {
                     const auto error = exception::capture();
                     if( error.code() != code::casual::shutdown)
                     {
                        common::log::line( common::log::category::error, error);
                        return error.code().value();
                     }
                  }

                  return 0;
               }

               int int_return( common::function< int()> callable)
               {
                  try 
                  {
                     callable();
                  }
                  catch( ...)
                  {
                     const auto error = exception::capture();
                     if( error.code() != code::casual::shutdown)
                     {
                        common::log::line( common::log::category::error, error);
                        return error.code().value();
                     }
                  }

                  return 0;
               }
            } // detail
         } // log

         namespace fatal
         {
            int guard( common::function< void()> callable)
            {
               try 
               {
                  callable();
                  return 0;
               }
               catch( ...)
               {
                  const auto error = exception::capture();
                  if( error.code() == code::casual::shutdown)
                     return 0;

                  event::error::fatal::send( error.code(), error.what());
                  return error.code().value();
               }
            }
         } // fatal

         namespace cli
         {
            int guard( common::function< void()> callable)
            {
               try 
               {
                  callable();
               }
               catch( ...)
               {
                  const auto error = exception::capture();
                  if( error.code() != code::casual::shutdown)
                  {
                     exception::format::terminal( std::cerr, error);
                     return error.code().value();
                  }
               }

               return 0;
            }
         } // cli
      } // main
      
      void guard( common::function< void()> callable)
      {
         try 
         {
            callable();
         }
         catch( ...)
         {
            log::line( log::category::error, exception::capture());
         }
      }
      
   } // common::exception
} // casual
