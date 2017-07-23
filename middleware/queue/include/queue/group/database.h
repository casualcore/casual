//!
//! casual
//!

#ifndef CASUALQUEUESERVERDATABASE_H_
#define CASUALQUEUESERVERDATABASE_H_

#include "sql/database.h"

#include "common/exception/system.h"
#include "common/string.h"
#include "common/message/queue.h"

namespace casual
{
   namespace queue
   {
      namespace group
      {
         using Queue = common::message::queue::Queue;


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

         class Database
         {
         public:
            Database( const std::string& database, std::string groupname);


            std::string file() const;

            Queue create( Queue queue);

            //!
            //! @return the created queues
            //!
            std::vector< Queue> update( std::vector< Queue> update, const std::vector< Queue::id_type>& remove);


            //bool remove( const std::string& name);


            common::message::queue::enqueue::Reply enqueue( const common::message::queue::enqueue::Request& message);

            common::message::queue::dequeue::Reply dequeue( const common::message::queue::dequeue::Request& message);

            common::message::queue::peek::information::Reply peek( const common::message::queue::peek::information::Request& request);
            common::message::queue::peek::messages::Reply peek( const common::message::queue::peek::messages::Request& request);


            std::size_t restore( Queue::id_type id);

            void commit( const common::transaction::ID& id);
            void rollback( const common::transaction::ID& id);

            //!
            //! @return queues that was affected of the commit
            //!
            std::vector< Queue::id_type> committed( const common::transaction::ID& id);



            std::vector< common::message::queue::information::Queue> queues();

            std::vector< common::message::queue::information::Message> messages( Queue::id_type id);

            
            //!
            //! @return "global" error queue
            //!
            Queue::id_type error() const { return m_error_queue;}


            //!
            //! @return the number of rows affected by the last statement.
            //!
            std::size_t affected() const;



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
            Queue::id_type quid( M&& message) const
            {
               if( message.queue != 0)
               {
                  return message.queue;
               }

               auto found = common::range::find( m_name_mapping, message.name);

               if( found)
               {
                  return found->second;
               }

               throw common::exception::system::invalid::Argument{ 
                  common::string::compose( "requested queue is not hosted by this queue-group - message: ", message)};
            }


         private:


            void updateQueue( const Queue& queue);
            void removeQueue( Queue::id_type id);

            std::vector< Queue> queue( Queue::id_type id);

            void update_mapping();



            std::unordered_map< std::string, Queue::id_type> m_name_mapping;


            sql::database::Connection m_connection;
            Queue::id_type m_error_queue;

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

         };

      } // server
   } // queue


} // casual

#endif // DATABASE_H_
