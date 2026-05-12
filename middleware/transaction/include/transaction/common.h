//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log.h"


namespace casual
{
   namespace transaction
   {

      using Trace = common::Trace;

      namespace log
      {
         template< typename... Ts>
         void line( Ts&&... ts)
         {
            common::log::line( common::log::category::transaction, std::forward< Ts>( ts)...);
         }

         //! only error log if code is not success
         template< typename C, typename... Ts>
         auto code( C code, Ts&&... ts)
         {
            if( ! common::code::success( code))
               common::log::error( code, std::forward< Ts>( ts)...);

            return code;
         }

         template< typename T, typename... Ts>
         void event( T&& t, Ts&&... ts)
         {
            auto& stream = common::log::category::event::transaction;

            if( stream)
            {
               common::log::stream::thread::Lock lock;

               common::stream::write( stream, t);

               ( common::stream::write( stream, '|', std::forward< Ts>( ts)), ...);

               common::stream::write( stream, '\n');
            }
         }
         
      } // log


   } // transaction

} // casual


