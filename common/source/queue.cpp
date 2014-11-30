//!
//! casual_queue.cpp
//!
//! Created on: Jun 10, 2012
//!     Author: Lazan
//!

#include "common/queue.h"
#include "common/ipc.h"


#include <algorithm>

namespace casual
{
   namespace common
   {
      namespace queue
      {
         namespace policy
         {

            marshal::input::Binary Blocking::next( ipc::receive::Queue& ipc)
            {
               auto message = ipc( flags);

               assert( ! message.empty());

               return marshal::input::Binary( std::move( message.front()));
            }

            marshal::input::Binary Blocking::next( ipc::receive::Queue& ipc, const std::vector< platform::message_type_type>& types)
            {
               std::vector< marshal::input::Binary> result;

               auto message = ipc( types, flags);

               assert( ! message.empty());

               return marshal::input::Binary( std::move( message.front()));
            }

            std::vector< marshal::input::Binary> NonBlocking::next( ipc::receive::Queue& ipc)
            {
               std::vector< marshal::input::Binary> result;

               auto message = ipc( flags);

               if( ! message.empty())
               {
                  result.emplace_back( std::move( message.front()));
               }

               return result;
            }

            std::vector< marshal::input::Binary> NonBlocking::next( ipc::receive::Queue& ipc, const std::vector< platform::message_type_type>& types)
            {
               std::vector< marshal::input::Binary> result;

               auto message = ipc( types, flags);

               if( ! message.empty())
               {
                  result.emplace_back( std::move( message.front()));
               }

               return result;
            }
         }

      } // queue
   } // common
} // casual


