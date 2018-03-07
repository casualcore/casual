//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUALQUEUESERVERDATABASE_H_
#define CASUALQUEUESERVERDATABASE_H_

#include "sql/database.h"

#include "common/exception/system.h"
#include "common/string.h"
#include "common/message/queue.h"
#include "common/message/pending.h"

namespace casual
{
   namespace queue
   {
      namespace group
      {
         using Queue = common::message::queue::Queue;
         using size_type = common::platform::size::type;

         namespace message
         {
            enum State
            {
               added = 1,
               enqueued = 2,
               removed = 3,
               dequeued = 4
            };
         } // message

         namespace pending
         {
            struct Dequeue
            {
               common::strong::queue::id id;
               common::platform::size::type pending;
               common::platform::size::type count;
            };
         } // pending
         class Database
         {
         public:


            Database( const std::string& database, std::string groupname);


            std::string file() const;

            Queue create( Queue queue);

            //!
            //! @return the created queues
            //!
            std::vector< Queue> update( std::vector< Queue> update, const std::vector< common::strong::queue::id>& remove);


            common::message::queue::enqueue::Reply enqueue( const common::message::queue::enqueue::Request& message);

            common::message::queue::dequeue::Reply dequeue( const common::message::queue::dequeue::Request& message);

            common::message::queue::peek::information::Reply peek( const common::message::queue::peek::information::Request& request);
            common::message::queue::peek::messages::Reply peek( const common::message::queue::peek::messages::Request& request);


            size_type restore( common::strong::queue::id id);

            void commit( const common::transaction::ID& id);
            void rollback( const common::transaction::ID& id);

            //!
            //! @return queues that was affected of the commit
            //!
            std::vector< common::strong::queue::id> committed( const common::transaction::ID& id);



            std::vector< common::message::queue::information::Queue> queues();

            std::vector< common::message::queue::information::Message> messages( common::strong::queue::id id);


            inline bool has_pending() const noexcept { return ! m_requests.empty();}

            common::message::queue::dequeue::forget::Reply pending_forget( const common::message::queue::dequeue::forget::Request& request);

            //! consume and forget all pending dequeues
            std::vector< common::message::pending::Message> pending_forget();

            std::vector< common::message::queue::dequeue::Request> pending();

            void pending_add( const common::message::queue::dequeue::Request& request);
            void pending_erase( common::strong::process::id pid);
            
            
            //!
            //! @return "global" error queue
            //!
            common::strong::queue::id error() const { return m_error_queue;}


            //!
            //! @return the number of rows affected by the last statement.
            //!
            size_type affected() const;






            void begin();
            void commit();
            void rollback();


            //!
            //! If message has a queue-id that will be returned,
            //! otherwise we lookup id from name
            //!
            //! @param message enqueue or dequeue message
            //! @return id to the queue
            //!
            template< typename M>
            common::strong::queue::id quid( M&& message) const
            {
               if( message.queue)
               {
                  return message.queue;
               }

               auto found = common::algorithm::find( m_name_mapping, message.name);

               if( found)
               {
                  return found->second;
               }

               throw common::exception::system::invalid::Argument{ 
                  common::string::compose( "requested queue is not hosted by this queue-group - message: ", message)};
            }

            
            //!
            //! @attention only exposed for unittest purposes
            //! @{
            std::vector< pending::Dequeue> get_pending();
            void pending_add( common::strong::queue::id id);
            void pending_set( common::strong::queue::id id, common::platform::size::type value);
            //! @}

            inline const std::string& name() const { return m_name;}

            sql::database::Version version();
 
         private:

           


            void update_queue( const Queue& queue);
            void remove_queue( common::strong::queue::id id);

            std::vector< Queue> queue( common::strong::queue::id id);

            void update_mapping();


            std::vector< common::message::queue::dequeue::Request> m_requests;

            std::unordered_map< std::string, common::strong::queue::id> m_name_mapping;


            sql::database::Connection m_connection;
            common::strong::queue::id m_error_queue;

            struct Statement
            {
               sql::database::Statement enqueue;


               struct
               {
                  sql::database::Statement first;
                  sql::database::Statement first_id;
                  sql::database::Statement first_match;

               } dequeue;



               struct
               {
                  sql::database::Statement xid;
                  sql::database::Statement nullxid;

               } state;

               struct
               {
                  sql::database::Statement add;
                  sql::database::Statement set;
                  sql::database::Statement check;
               } pending;


               sql::database::Statement commit1;
               sql::database::Statement commit2;
               sql::database::Statement commit3;

               sql::database::Statement rollback1;
               sql::database::Statement rollback2;
               sql::database::Statement rollback3;

               struct
               {
                  sql::database::Statement queue;
                  sql::database::Statement message;

               } information;

               struct
               {
                  sql::database::Statement match;
                  sql::database::Statement first;
                  sql::database::Statement one_message;
               } peek;

               sql::database::Statement restore;



            } m_statement;

            std::string m_name;

         };

      } // server
   } // queue


} // casual

#endif // DATABASE_H_
