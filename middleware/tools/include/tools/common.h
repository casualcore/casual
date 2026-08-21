//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/log/line.h"
#include "common/execute.h"

#include <iostream>


namespace casual
{
   namespace tools
   {
      namespace license
      {
         constexpr auto c = R"(   
/*
* Copyright (c) 2026, The casual project
* 
* This software is licensed under the MIT license, https://opensource.org/licenses/MIT
*/
)";

      } // license

      namespace verbose
      {
         //! logs to std::clog if settings[.mandatory].verbose is true.
         template< typename S, typename... Ts>
         void log( const S& settings, Ts&&... ts)
         {
            if constexpr (requires( S settings) { settings.directive.verbose; }) 
            {
               if( settings.directive.verbose)
                  common::log::line( std::clog, std::forward< Ts>( ts)...);
            } 
            else if constexpr (requires( S settings) { settings.verbose; })
            {
               if( settings.verbose)
                  common::log::line( std::clog, std::forward< Ts>( ts)...);
            }
         }
      } // verbose

      namespace source::keep
      {
         auto guard( const auto& settings, auto& path)
         {
            return common::execute::scope( [ keep = settings.directive.source.keep, &path]()
            { 
               if( keep)
                  path.release();
            });
         }
      } // source::keep

   } // tools
} // casual
