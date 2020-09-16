//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "sql/database.h"

#include "queue/group/database/statement.h"

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
         using size_type = platform::size::type;

         namespace message
         {
            enum class State
            {
               added = 1,
               enqueued = 2,
               removed = 3,
               dequeued = 4
            };
         } // message

         namespace message
         {
            struct Available
            {
               common::strong::queue::id queue;
               platform::size::type count;
               platform::time::point::type when;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( when);
               )

               inline friend bool operator == ( const Available& lhs, common::strong::queue::id rhs) { return lhs.queue == rhs;}
            };
         } // message
         class Database
         {
         public:

            Database( const std::string& database, std::string groupname);
            ~Database();

            std::string file() const;

            //! creates or updates a queue
            //! @returns the created or updated queue
            Queue create( Queue queue);
            void update( const Queue& queue);
            void remove( common::strong::queue::id id);

            //! @returns the created queues
            std::vector< Queue> update( std::vector< Queue> update, const std::vector< common::strong::queue::id>& remove);

            //! @returns the queue if it exists
            //! @{
            //common::optional< Queue> queue( const std::string& name);
            common::optional< Queue> queue( common::strong::queue::id id);
            //! @}

            common::message::queue::enqueue::Reply enqueue( const common::message::queue::enqueue::Request& message);
            
            common::message::queue::dequeue::Reply dequeue( 
               const common::message::queue::dequeue::Request& message, 
               const platform::time::point::type& now);

            common::message::queue::peek::information::Reply peek( const common::message::queue::peek::information::Request& request);
            common::message::queue::peek::messages::Reply peek( const common::message::queue::peek::messages::Request& request);

            //! @returns all queues that is found in `queues` and has messages potentially avaliable for dequeue
            std::vector< message::Available> available( std::vector< common::strong::queue::id> queues) const;

            //! @returns the earliest available message in the queue, if any.
            common::optional< platform::time::point::type> available( common::strong::queue::id queue) const;

            //! @return number of restored messages for the queue
            size_type restore( common::strong::queue::id id);

            //! @returns number of 'deleted' messages for the queue
            //! @note: only `enqueued` messages are deleted (no pending for commmit)
            size_type clear( common::strong::queue::id queue);

            //! @returns id:s of the messages that was removed
            std::vector< common::Uuid> remove( common::strong::queue::id queue, std::vector< common::Uuid> messages);

            void commit( const common::transaction::ID& id);
            void rollback( const common::transaction::ID& id);

            //! information
            std::vector< common::message::queue::information::Queue> queues();
            //! information
            std::vector< common::message::queue::information::Message> messages( common::strong::queue::id id);

            void metric_reset( const std::vector< common::strong::queue::id>& ids);
            

            //! @return the number of rows affected by the last statement.
            size_type affected() const;

            void begin();
            void commit();
            void rollback();

            //! @returns the id of of the queue corresponding to the name
            common::strong::queue::id id( const std::string& name) const;


            //! If message has a queue-id that will be returned,
            //! otherwise we lookup id from name
            //!
            //! @param message enqueue or dequeue message
            //! @return id to the queue
            template< typename M>
            common::strong::queue::id id( const M& message) const
            {
               if( message.queue)
                  return message.queue;
               else 
                  return id( message.name);
            }


            inline const std::string& name() const { return m_name;}

            sql::database::Version version();
 
         private:

            //! potentially log explain performance metrics
            void explain();

            sql::database::Connection m_connection;
            database::Statement m_statement;
            std::string m_name;
            std::ofstream m_explain_log;
         };

      } // group
   } // queue
} // casual


