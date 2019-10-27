//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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

         namespace message
         {
            struct Available
            {
               common::strong::queue::id queue;
               common::platform::size::type count;
               common::platform::time::point::type when;

               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
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
               const common::platform::time::point::type& now);

            common::message::queue::peek::information::Reply peek( const common::message::queue::peek::information::Request& request);
            common::message::queue::peek::messages::Reply peek( const common::message::queue::peek::messages::Request& request);

            //! @returns all queues that is found in `queues` and has messages potentially avaliable for dequeue
            std::vector< message::Available> available( std::vector< common::strong::queue::id> queues) const;

            //! @returns the earliest available message in the queue, if any.
            common::optional< common::platform::time::point::type> available( common::strong::queue::id queue) const;


            size_type restore( common::strong::queue::id id);

            void commit( const common::transaction::ID& id);
            void rollback( const common::transaction::ID& id);

            //! information
            std::vector< common::message::queue::information::Queue> queues();
            //! information
            std::vector< common::message::queue::information::Message> messages( common::strong::queue::id id);
            

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

            sql::database::Connection m_connection;

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
                  sql::database::Statement queues;
                  sql::database::Statement message;
               } available;

               

               sql::database::Statement id;

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


