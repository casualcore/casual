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

namespace casual
{
   namespace queue
   {
      namespace server
      {

         class Database
         {
         public:
            Database( const std::string& database);

            void create( const std::string& queue);

            void enqueue( const std::string& queue, const Message& message);

            bool dequeue( const std::string& queue, Message& message);


            std::vector< std::string> queues();

            void persistencePrepare();
            void persistenceCommit();


         private:
            sql::database::Connection m_connection;
         };

      } // server
   } // queue


} // casual

#endif // DATABASE_H_
