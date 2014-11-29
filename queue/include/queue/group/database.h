//!
//! database.h
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#ifndef CASUALQUEUESERVERDATABASE_H_
#define CASUALQUEUESERVERDATABASE_H_

#include "sql/database.h"


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
            Database( const std::string& database);

            Queue create( Queue queue);

            //!
            //! @return the created queues
            //!
            std::vector< Queue> update( std::vector< Queue> update, const std::vector< Queue::id_type>& remove);


            //bool remove( const std::string& name);


            common::message::queue::enqueue::Reply enqueue( const common::message::queue::enqueue::Request& message);

            common::message::queue::dequeue::Reply dequeue( const common::message::queue::dequeue::Request& message);


            void commit( const common::transaction::ID& id);
            void rollback( const common::transaction::ID& id);


            std::vector< common::message::queue::information::Queue> queues();

            std::vector< common::message::queue::information::Message> messages( Queue::id_type id);


            void persistenceBegin();
            void persistenceCommit();

            
            //!
            //! @return "global" error queue
            //!
            Queue::id_type error() const { return m_errorQueue;}


            //!
            //! @return the number of rows affected by the last statement.
            //!
            std::size_t affected() const;

            void begin();
            void commit();
            void rollback();



         private:

            void updateQueue( const Queue& queue);
            void removeQueue( Queue::id_type id);

            std::vector< Queue> queue( Queue::id_type id);

            sql::database::Connection m_connection;
            Queue::id_type m_errorQueue;

            struct Statement
            {
               sql::database::Statement enqueue;
               sql::database::Statement dequeue;

               struct state_t
               {
                  sql::database::Statement xid;
                  sql::database::Statement nullxid;

               } state;


               sql::database::Statement commit1;
               sql::database::Statement commit2;

               sql::database::Statement rollback1;
               sql::database::Statement rollback2;
               sql::database::Statement rollback3;

               struct info_t
               {
                  sql::database::Statement queues;
                  sql::database::Statement messages;

               } information;

            } m_statement;

         };


         namespace scoped
         {
            struct Writer
            {
               Writer( Database& db) : m_db( db)
               {
                  // TODO: error checking?
                  m_db.persistenceBegin();
               }

               ~Writer()
               {
                  m_db.persistenceCommit();
               }

            private:
               Database& m_db;
            };
         } // scoped

      } // server
   } // queue


} // casual

#endif // DATABASE_H_
