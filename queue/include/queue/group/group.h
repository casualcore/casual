//!
//! queue.h
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#ifndef CASUALQUEUESERVER_H_
#define CASUALQUEUESERVER_H_

#include <string>

#include "queue/group/database.h"

#include "common/platform.h"
#include "common/message/pending.h"


namespace casual
{
   namespace queue
   {
      namespace group
      {
         struct Settings
         {
            std::string queuebase;
         };

         struct State
         {
            State( std::string filename) : queuebase( std::move( filename)) {}

            Database queuebase;


            template< typename M>
            void persist( M&& message, common::platform::queue_id_type destination)
            {
               persistent.push_back( common::message::pending::Message{ std::forward< M>( message), { destination}});

               //persistent.emplace_back( std::forward< M>( message), { destination});
            }

            std::vector< common::message::pending::Message> persistent;

         };


         struct Server
         {
            Server( Settings settings);


            void start();

         private:
            State m_state;
         };
      } // server

   } // queue

} // casual

#endif // QUEUE_H_
