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
         using queue_id_type = std::size_t;

         struct Settings
         {
            std::string queuebase;
            std::string name;
         };

         struct State
         {
            State( std::string filename, std::string name) : queuebase( std::move( filename), std::move( name)) {}

            Database queuebase;


            template< typename M>
            void persist( M&& message, std::vector< common::platform::queue_id_type> destinations)
            {
               persistent.emplace_back( std::forward< M>( message), std::move( destinations));
            }

            std::vector< common::message::pending::Message> persistent;


            //!
            //! Kepp track of pending requests
            //!
            struct Pending
            {
               using request_type = common::message::queue::dequeue::Request;


               std::vector< request_type> requests;
               std::map< common::transaction::ID, std::map< queue_id_type, std::size_t>> transactions;

               void dequeue( const request_type& request);

               void enqueue( const common::transaction::ID& trid, queue_id_type queue);


               common::message::queue::dequeue::forget::Reply forget( const common::message::queue::dequeue::forget::Request& request);

               struct result_t
               {
                  std::vector< request_type> requests;
                  std::map< queue_id_type, std::size_t> enqueued;
               };

               result_t commit( const common::transaction::ID& trid);

               void rollback( const common::transaction::ID& trid);

            } pending;

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
