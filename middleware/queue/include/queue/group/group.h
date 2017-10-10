//!
//! casual
//!
#ifndef CASUAL_QUEUE_GROUP_GROUP_H_
#define CASUAL_QUEUE_GROUP_GROUP_H_


#include "queue/group/database.h"

#include "common/platform.h"
#include "common/message/pending.h"

#include <string>

namespace casual
{
   namespace queue
   {
      namespace group
      {
         using queue_id_type = common::platform::size::type;
         using size_type = common::platform::size::type;

         struct Settings
         {
            std::string queuebase;
            std::string name;
         };


         struct State
         {
            State( std::string filename, std::string name)
               : queuebase( std::move( filename), std::move( name)) {}


            std::unordered_map< std::string, queue_id_type> queue_id;

            Database queuebase;


            template< typename M>
            void persist( M&& message, std::vector< common::platform::ipc::id> destinations)
            {
               persistent.emplace_back( std::forward< M>( message), std::move( destinations));
            }

            std::vector< common::message::pending::Message> persistent;

            //!
            //! A log to know if we already have notified TM about
            //! a given transaction.
            //!
            std::vector< common::transaction::ID> involved;


            //!
            //! Kepp track of pending requests
            //!
            struct Pending
            {
               using request_type = common::message::queue::dequeue::Request;


               std::vector< request_type> requests;
               std::map< common::transaction::ID, std::map< queue_id_type, size_type>> transactions;

               void dequeue( const request_type& request);

               void enqueue( const common::transaction::ID& trid, queue_id_type queue);


               common::message::queue::dequeue::forget::Reply forget( const common::message::queue::dequeue::forget::Request& request);

               struct result_t
               {
                  std::vector< request_type> requests;
                  std::map< queue_id_type, size_type> enqueued;
               };

               result_t commit( const common::transaction::ID& trid);

               void rollback( const common::transaction::ID& trid);


               void erase( common::platform::pid::type pid);

            } pending;

         };

         namespace message
         {
            void pump( group::State& state);
         } // message

         struct Server
         {
            Server( Settings settings);
            ~Server();


            int start() noexcept;

         private:
            State m_state;
         };
      } // group

   } // queue

} // casual

#endif // QUEUE_H_
