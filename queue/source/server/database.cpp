//!
//! database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/server/database.h"

#include "common/algorithm.h"

#include "common/exception.h"


#include <chrono>

namespace casual
{
   namespace queue
   {
      namespace server
      {

         namespace local
         {
            namespace
            {

               namespace transform
               {

                  struct Message
                  {

                     queue::Message operator () ( sql::database::Row& row) const
                     {

                        // SELECT correlation, reply, redelivered, type, avalible, timestamp, payload
                        queue::Message result;

                        row.get( 0, result.correlation.get());
                        row.get( 1, result.reply);
                        row.get( 2, result.redelivered);
                        row.get( 3, result.type);

                        result.avalible = common::platform::time_type{ std::chrono::microseconds{ row.get< common::platform::time_type::rep>( 4)}};
                        result.timestamp = common::platform::time_type{ std::chrono::microseconds{ row.get< common::platform::time_type::rep>( 5)}};
                        row.get( 6, result.payload);

                        return result;
                     }

                  };



               } // transform


            } // <unnamed>
         } // local

         Database::Database( const std::string& database) : m_connection( database)
         {


            m_connection.execute(
                R"( CREATE TABLE IF NOT EXISTS queues 
              (
                  name         TEXT,
                  retries      NUMBER,
                  error        NUMBER,
                  PRIMARY KEY (name)); )");


            m_connection.execute(
                R"( CREATE TABLE IF NOT EXISTS messages 
              (correlation   BLOB,
                  queue         NUMBER,
                  origin        NUMBER, -- the first queue a message is enqueued to
                  gtrid         BLOB,
                  state         NUMBER,
                  reply         TEXT,
                  redelivered   NUMBER,
                  type          NUMBER,
                  avalible      NUMBER,
                  timestamp     NUMBER,
                  payload       BLOB,
                  PRIMARY KEY (correlation),
                  FOREIGN KEY (queue) REFERENCES queues( rowid)); )");


            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_queue_messages  ON messages ( queue);" );

            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_timestamp_messages  ON messages ( timestamp);" );

            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_gtrid_messages  ON messages ( gtrid);" );


            //
            // Global error queue
            //
            m_connection.execute( R"( INSERT INTO queues VALUES ( "casual-error-queue", 0, 0); )");
            m_errorQueue = m_connection.rowid();

            //
            // the global error queue has it self as an error queue.
            //
            m_connection.execute( " UPDATE queues SET error = ? WHERE rowid = ?;", m_errorQueue, m_errorQueue);
         }

         Queue Database::create( Queue queue)
         {

            //
            // Create corresponding error queue
            //
            m_connection.execute( "INSERT INTO queues VALUES (?,?,?);", queue.name + "_error", queue.retries, m_errorQueue);

            queue.error = m_connection.rowid();

            m_connection.execute( "INSERT INTO queues VALUES (?,?,?);", queue.name, queue.retries, queue.error);
            queue.id = m_connection.rowid();

            return queue;

         }



         void Database::enqueue(  Queue::id_type queue, const common::transaction::ID& id, const Message& message)
         {
            auto gtrid = common::transaction::global( id);

            long state = id ? queue::Message::State::added : queue::Message::State::enqueued;

            auto avalible = std::chrono::time_point_cast< std::chrono::microseconds>( message.avalible).time_since_epoch().count();
            auto timestamp = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();

            m_connection.execute( "INSERT INTO messages VALUES (?,?,?,?,?,?,?,?,?,?,?);",
                  message.correlation.get(),
                  queue,
                  queue,
                  gtrid,
                  state,
                  message.reply,
                  message.redelivered,
                  message.type,
                  avalible,
                  timestamp,
                  message.payload);
         }

         Message Database::dequeue(  Queue::id_type queue, const common::transaction::ID& id)
         {

            auto now = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();

            const std::string sql{ R"( 
SELECT correlation, reply, redelivered, type, avalible, timestamp, payload
FROM messages WHERE queue = ? AND state = 2 AND avalible < ?  ORDER BY timestamp ASC LIMIT 1; )" };


            auto query = m_connection.query( sql, queue, now);

            sql::database::Row row;

            if( ! query.fetch( row))
            {
               throw common::exception::NotReallySureWhatToNameThisException( "no messages");
            }

            auto message = local::transform::Message()( row);

            //
            // Update state
            //
            if( id)
            {
               auto gtrid = common::transaction::global( id);
               m_connection.execute( "UPDATE messages SET gtrid = ?, state = 3 WHERE correlation = ?", gtrid, message.correlation.get());
            }
            else
            {
               m_connection.execute( "DELETE FROM messages WHERE correlation = ?", message.correlation.get());
            }


            return message;
         }


         void Database::commit( const common::transaction::ID& id)
         {
            //
            // First vi update "added" to enqueued
            //
            {
               const std::string sql{ "UPDATE messages SET state = 2 WHERE gtrid = ? AND state = 1"};

               auto gtrid = common::transaction::global( id);

               m_connection.execute( sql, gtrid);

            }

            //
            // Then we delete all "removed" messages
            //
            {
               const std::string sql{ "DELETE FROM messages WHERE gtrid = ? AND state = 3"};

               auto gtrid = common::transaction::global( id);

               m_connection.execute( sql, gtrid);

            }

         }


         void Database::rollback( const common::transaction::ID& id)
         {
            //
            // First vi delete "added" messages
            //
            {
               const std::string sql{ "DELETE FROM messages WHERE gtrid = ? AND state = 1"};

               auto gtrid = common::transaction::global( id);

               m_connection.execute( sql, gtrid);

            }

            //
            // Then we make all "removed" to enqueued
            //
            {

               auto gtrid = common::transaction::global( id);

               m_connection.execute( "UPDATE messages SET state = 2, redelivered = redelivered + 1  WHERE gtrid = ? AND state = 3", gtrid);

               m_connection.execute( "UPDATE messages SET redelivered = 0, queue = ( SELECT error FROM queues WHERE rowid = messages.queue) WHERE messages.redelivered > ( SELECT retries FROM queues WHERE rowid = messages.queue)");

            }
         }


         std::vector< Queue> Database::queues()
         {
            std::vector< Queue> result;

            auto query = m_connection.query( "SELECT rowid, name, retries, error FROM queues ORDER BY name;");

            sql::database::Row row;

            while( query.fetch( row))
            {
               Queue queue;
               row.get( 0, queue.id);
               row.get( 1, queue.name);
               row.get( 2, queue.retries);
               row.get( 3, queue.error);

               result.push_back( std::move( queue));
            }
            return result;
         }

         void Database::persistenceBegin()
         {
            m_connection.begin();
         }

         void Database::persistenceCommit()
         {
            m_connection.commit();
         }

      } // server
   } // queue

} // casual
