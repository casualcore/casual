//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "queue/group/database.h"

#include "casual/platform.h"

#include <string>

namespace casual
{
   namespace queue
   {
      namespace group
      {
         using queue_id_type = platform::size::type;
         using size_type = platform::size::type;

         struct State
         {
            inline State( std::string filename, std::string name)
               : queuebase( std::move( filename), std::move( name)) 
            {
               queuebase.begin();
            }

            inline ~State() 
            {
               queuebase.commit();
            }

            //std::unordered_map< std::string, queue_id_type> queue_id;

            Database queuebase;

            inline const std::string& name() const { return queuebase.name();}

            //! persist the queuebase ( commit and begin)
            inline void persist()
            {
               queuebase.commit();
               queuebase.begin();
            }
         
            struct Pending
            {
               template< typename M>
               void reply( M&& message, const common::process::Handle& destinations)
               {
                  replies.emplace_back( std::forward< M>( message), destinations);
               }

               void add( common::message::queue::dequeue::Request&& message);
               common::message::queue::dequeue::forget::Reply forget( const common::message::queue::dequeue::forget::Request& message);

               //! forgets all pending dequeue request.
               //! @returns forget request that should be sent to pending callers
               std::vector< common::message::pending::Message> forget();

               //! @returns dequeue-request that potentially has messages available for dequeue
               std::vector< common::message::queue::dequeue::Request> extract( std::vector< common::strong::queue::id> queues);

               void remove( common::strong::process::id pid);

               std::vector< common::message::pending::Message> replies;
               std::vector< common::message::queue::dequeue::Request> dequeues;
               
            } pending;

            

            //! A log to know if we already have notified TM about
            //! a given transaction.
            std::vector< common::transaction::ID> involved;
         };
      } // group
   } // queue
} // casual