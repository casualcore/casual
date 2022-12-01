//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/algorithm.h"

namespace casual
{
   namespace common::algorithm::is
   {

      template< typename R>
      bool sorted( R&& range)
      {
         return std::is_sorted( std::begin( range), std::end( range));
      }

      template< typename R, typename C>
      bool sorted( R&& range, C compare)
      {
         return std::is_sorted( std::begin( range), std::end( range), compare);
      }

      template< typename R>
      bool unique( R&& range)
      {
         auto current = range::make( std::forward< R>( range));

         while( current.size() >= 2)
         {
            auto& value = *current;
            if( algorithm::find( ++current, value))
               return false;
         }
         return true;
      }
   } // common::algorithm::is
} // casual
