//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"
#include "common/log/trace.h"


namespace casual
{
   namespace tools
   {
      extern common::log::Stream log;

      namespace trace
      {
         extern common::log::Stream log;

         struct Exit
         {
            template< typename T>
            Exit( T&& information, bool print) : m_information( std::forward< T>( information)), m_print( print) {}
            ~Exit();

         private:
            std::string m_information;
            bool m_print;
         };

      } // trace

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };

      namespace license
      {
         constexpr auto c = R"(   
/*
* Copyright (c) 2018, The casual project
* 
* This software is licensed under the MIT license, https://opensource.org/licenses/MIT
*/
)";

      } // license


   } // tools


} // casual


