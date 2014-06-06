//!
//! database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/server/database.h"

#include "common/algorithm.h"


namespace casual
{
   namespace queue
   {
      namespace server
      {

         Database::Database( const std::string& database) : m_connection( database)
         {

         }

         void Database::create( const std::string& queue)
         {
            m_connection.execute(
                "CREATE TABLE IF NOT EXISTS " + queue +
          R"( (correlation   BLOB,
               gtrid         BLOB,
               bqual         BLOB,
               reply         TEXT,
               redelivered   NUMBER,
               type          NUMBER,
               avalible      NUMBER,
               timetamp      NUMBER,
               payload       BLOB,
               PRIMARY KEY (gtrid, bqual)); )");




         }

         void Database::persistencePrepare()
         {
            m_connection.begin();
         }

         void Database::enqueue( const std::string& queue, const Message& message)
         {
         }

         bool Database::dequeue( const std::string& queue, Message& message)
         {



            return true;
         }

         std::vector< std::string> Database::queues()
         {
            std::vector< std::string> result;

            static const std::string query{ R"(
                 SELECT name FROM sqlite_master
                 WHERE type='table'
                 ORDER BY name;
              )"};

            auto fetch = m_connection.query( query);

            for( auto rows = fetch.fetch(); ! rows.empty(); rows = fetch.fetch())
            {
               result.push_back( rows.front().get< std::string>( 0));
            }
            return result;
         }

         void Database::persistenceCommit()
         {
            m_connection.commit();
         }

      } // server
   } // queue

} // casual
