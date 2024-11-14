//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"
#include "common/log.h"


namespace casual
{
   namespace tools
   {
      namespace trace
      {
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

      using Trace = common::Trace;

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


