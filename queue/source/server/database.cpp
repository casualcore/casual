//!
//! database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/server/database.h"

#include "common/algorithm.h"

#include "common/exception.h"
#include "common/internal/log.h"


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

                  struct Reply
                  {

                     common::message::queue::dequeue::Reply::Message operator () ( sql::database::Row& row) const
                     {

                        // SELECT correlation, reply, redelivered, type, avalible, timestamp, payload
                        common::message::queue::dequeue::Reply::Message result;

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
            m_connection.execute( " UPDATE queues SET error = :qid WHERE rowid = :qid;", m_errorQueue);



            //
            // Precompile all other statements
            //
            {
               m_statement.enqueue = m_connection.precompile( "INSERT INTO messages VALUES (?,?,?,?,?,?,?,?,?,?,?);");

               m_statement.dequeue = m_connection.precompile( R"( 
                     SELECT 
                        correlation, reply, redelivered, type, avalible, timestamp, payload
                     FROM 
                        messages 
                     WHERE queue = :queue AND state = 2 AND avalible < :avalible  ORDER BY timestamp ASC LIMIT 1; )");


               m_statement.state.xid =  m_connection.precompile( "UPDATE messages SET gtrid = :gtrid, state = 3 WHERE correlation = :correlation");
               m_statement.state.nullxid = m_connection.precompile( "DELETE FROM messages WHERE correlation = :correlation");


               m_statement.commit1 = m_connection.precompile( "UPDATE messages SET state = 2 WHERE gtrid = :gtrid AND state = 1;");
               m_statement.commit2 = m_connection.precompile( "DELETE FROM messages WHERE gtrid = :gtrid AND state = 3;");


               /*
               m_statement.rollback = m_connection.precompile( R"(
                 -- delete all enqueued  
                 DELETE FROM messages WHERE gtrid = :gtrid AND state = 1; 
                 -- update all dequeued back to enqueued
                 UPDATE messages SET state = 2, redelivered = redelivered + 1  WHERE gtrid = :gtrid AND state = 3;
                 -- move to error queue
                 UPDATE messages SET redelivered = 0, queue = ( SELECT error FROM queues WHERE rowid = messages.queue) 
                     WHERE messages.redelivered > ( SELECT retries FROM queues WHERE rowid = messages.queue);
                     )");

               */

               m_statement.rollback1 = m_connection.precompile( "DELETE FROM messages WHERE gtrid = :gtrid AND state = 1;");
               m_statement.rollback2 = m_connection.precompile( "UPDATE messages SET state = 2, redelivered = redelivered + 1  WHERE gtrid = :gtrid AND state = 3");
               m_statement.rollback3 = m_connection.precompile(
                     "UPDATE messages SET redelivered = 0, queue = ( SELECT error FROM queues WHERE rowid = messages.queue)"
                     " WHERE messages.redelivered > ( SELECT retries FROM queues WHERE rowid = messages.queue);");

               m_statement.information.queues = m_connection.precompile( R"(
                  SELECT
                     q.rowid, q.name, q.retries, q.error, COUNT( m.correlation)
                  FROM
                     queues q LEFT JOIN messages m ON q.rowid = m.queue AND m.state = 3
                     GROUP BY q.rowid 
                      ;
                  )");

            }
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




         void Database::enqueue( const common::message::queue::enqueue::Request& message)
         {

            common::log::internal::queue << "enqueue - qid: " << message.queue << " correlation: " << message.message.correlation << " size: " << message.message.payload.size() << " xid: " << message.xid.xid << std::endl;

            auto gtrid = common::transaction::global( message.xid.xid);

            long state = message.xid.xid ? queue::Message::State::added : queue::Message::State::enqueued;

            auto avalible = std::chrono::time_point_cast< std::chrono::microseconds>( message.message.avalible).time_since_epoch().count();
            auto timestamp = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();

            m_statement.enqueue.execute(
                  message.message.correlation.get(),
                  message.queue,
                  message.queue,
                  gtrid,
                  state,
                  message.message.reply,
                  0,
                  message.message.type,
                  avalible,
                  timestamp,
                  message.message.payload);
         }




         common::message::queue::dequeue::Reply Database::dequeue( const common::message::queue::dequeue::Request& message)
         {

            common::message::queue::dequeue::Reply reply;

            auto now = std::chrono::time_point_cast< std::chrono::microseconds>(
                  common::platform::clock_type::now()).time_since_epoch().count();


            auto resultset = m_statement.dequeue.query( message.queue, now);

            sql::database::Row row;

            if( ! resultset.fetch( row))
            {
               common::log::internal::queue << "dequeue - qid: " << message.queue << " - no message" << std::endl;

               return reply;
            }

            auto result = local::transform::Reply()( row);


            //
            // Update state
            //
            if( message.xid.xid)
            {
               auto gtrid = common::transaction::global(  message.xid.xid);

               m_statement.state.xid.execute( gtrid, result.correlation.get());
            }
            else
            {
               m_statement.state.nullxid.execute( result.correlation.get());
            }

            common::log::internal::queue << "dequeue - qid: " << message.queue << " correlation: " << result.correlation << " size: " << result.payload.size() << " xid: " << message.xid.xid << std::endl;

            reply.message.push_back( std::move( result));

            return reply;
         }


         void Database::commit( const common::transaction::ID& id)
         {
            common::log::internal::queue << "commit xid: " << id << std::endl;

            auto gtrid = common::transaction::global( id);

            m_statement.commit1.execute( gtrid);
            m_statement.commit2.execute( gtrid);
         }


         void Database::rollback( const common::transaction::ID& id)
         {
            common::log::internal::queue << "rollback xid: " << id << std::endl;

            auto gtrid = common::transaction::global( id);

            m_statement.rollback1.execute( gtrid);
            m_statement.rollback2.execute( gtrid);
            m_statement.rollback3.execute();
         }


         std::vector< common::message::queue::Information::Queue> Database::queues()
         {
            std::vector< common::message::queue::Information::Queue> result;

            //auto query = m_connection.query( "SELECT rowid, name, retries, error FROM queues ORDER BY name;");
            auto query = m_statement.information.queues.query();

            sql::database::Row row;

            while( query.fetch( row))
            {
               common::message::queue::Information::Queue queue;

               row.get( 0, queue.id);
               row.get( 1, queue.name);
               row.get( 2, queue.retries);
               row.get( 3, queue.error);
               row.get( 4, queue.messages);

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

         std::size_t Database::affected() const
         {
            return m_connection.affected();
         }

      } // server
   } // queue

} // casual
