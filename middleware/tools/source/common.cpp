//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "tools/common.h"

#include <iostream>

namespace casual
{
   namespace tools
   {
      common::log::Stream log{ "casual.tools"};

      namespace trace
      {
         common::log::Stream log{ "casual.tools.trace"};
         
         Exit::~Exit()
         {
            if( m_print)
            {
               if( std::uncaught_exception())
               {
                  std::cerr << m_information << " - failed" << '\n';
               }
               else
               {
                  std::cout << m_information << " - ok" << '\n';
               }
            }

         }

      } // trace


   } // tools


} // casual
