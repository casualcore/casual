//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/guard.h"

#include <iostream>

namespace casual
{
   namespace common
   {
      namespace exception
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
                        common::log::line( std::cerr, error);
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
         
      } // exception
   } // common
} // casual
