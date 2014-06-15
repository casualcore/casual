//!
//! database.h
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#ifndef CASUALQUEUESERVERDATABASE_H_
#define CASUALQUEUESERVERDATABASE_H_

#include "sql/database.h"

#include "queue/message.h"

#include "common/message/queue.h"

namespace casual
{
   namespace queue
   {
      namespace server
      {
         struct Queue
         {
            using id_type = std::size_t;

            Queue() = default;
            Queue( std::string name) : name( std::move( name)) {}
            Queue( std::string name, std::size_t retries) : name( std::move( name)), retries( retries) {}

            //Queue( Queue&&) = default;
            //Queue& operator = ( Queue&&) = default;

            id_type id = 0;
            std::string name;
            std::size_t retries = 0;
            id_type error = 0;
         };

         class Database
         {
         public:
            Database( const std::string& database);

            Queue create( Queue queue);

            //void enqueue( Queue::id_type queue, const common::transaction::ID& id, const Message& message);

            void enqueue( const common::message::queue::enqueue::Request& message);


            //Message dequeue( Queue::id_type queue, const common::transaction::ID& id);

            common::message::queue::dequeue::Reply dequeue( const common::message::queue::dequeue::Request& message);


            void commit( const common::transaction::ID& id);
            void rollback( const common::transaction::ID& id);


            std::vector< Queue> queues();

            void persistenceBegin();
            void persistenceCommit();

            
            //!
            //! @return "global" error queue
            //!
            Queue::id_type error() const { return m_errorQueue;}

         private:
            sql::database::Connection m_connection;
            Queue::id_type m_errorQueue;
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
