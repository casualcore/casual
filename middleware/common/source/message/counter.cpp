//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/message/counter.h"

#include "common/algorithm.h"

namespace casual
{
   namespace common::message::counter
   {
      namespace local
      {
         namespace
         {            
            std::vector< Entry> global_counter;

            Entry& get( message::Type type)
            {
               if( auto found = algorithm::find( global_counter, type))
                  return *found;

               return global_counter.emplace_back( type);
            } 
   
         } // <unnamed>
      } // local

      namespace add
      {
         void sent( message::Type type) noexcept
         {
            ++local::get( type).sent;
         }
         
         void received( message::Type type) noexcept
         {
            ++local::get( type).received;
         }
         
      } // add

      std::vector< Entry> entries() noexcept
      {
         return local::global_counter;
      }
      
   } // common::message::counter
} // casual
