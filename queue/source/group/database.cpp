//!
//! database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/group/database.h"

#include "common/algorithm.h"

#include "common/exception.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"


#include <chrono>

namespace casual
{
   namespace queue
   {
      namespace group
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

                        row.get( 0, result.id.get());
                        row.get( 1, result.correlation);
                        row.get( 2, result.reply);
                        row.get( 3, result.redelivered);
                        row.get( 4, result.type);

                        result.avalible = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( 5)}};
                        result.timestamp = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( 6)}};
                        row.get( 7, result.payload);

                        return result;
                     }

                  };

                  struct Queue
                  {
                     group::Queue operator () ( sql::database::Row& row) const
                     {
                        group::Queue result;

                        row.get( 0, result.id);
                        row.get( 1, result.name);
                        row.get( 2, result.retries);
                        row.get( 3, result.error);
                        row.get( 4, result.type);

                        return result;
                     }

                  };



               } // transform


            } // <unnamed>
         } // local




         Database::Database( const std::string& database) : m_connection( database)
         {

            common::trace::internal::Scope trace{ "Database::Database", common::log::internal::queue};

            //
            // Make sure we got FK
            //
            m_connection.execute( "PRAGMA foreign_keys = ON;");

            m_connection.execute(
                R"( CREATE TABLE IF NOT EXISTS queues 
                (
                  id           INTEGER  PRIMARY KEY,
                  name         TEXT     UNIQUE,
                  retries      NUMBER,
                  error        NUMBER,
                  type         NUMBER ); )"
              );


            m_connection.execute(
                R"( CREATE TABLE IF NOT EXISTS messages 
                ( id            BLOB PRIMARY KEY,
                  queue         INTEGER,
                  origin        NUMBER, -- the first queue a message is enqueued to
                  gtrid         BLOB,
                  correlation   TEXT,
                  state         NUMBER,
                  reply         TEXT,
                  redelivered   NUMBER,
                  type          NUMBER,
                  avalible      NUMBER,
                  timestamp     NUMBER,
                  payload       BLOB,
                  FOREIGN KEY (queue) REFERENCES queues( id)); )");

            m_connection.execute(
                  "CREATE INDEX IF NOT EXISTS i_id_messages  ON messages ( id);" );

            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_queue_messages  ON messages ( queue);" );

            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_timestamp_messages  ON messages ( timestamp);" );

            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_gtrid_messages  ON messages ( gtrid);" );


            //
            // Global error queue
            //
            m_connection.execute( R"( INSERT OR IGNORE INTO queues VALUES ( 1, "casual-error-queue", 0, 1, 1); )");
            m_errorQueue = 1;

            //
            // the global error queue has it self as an error queue.
            //
            //m_connection.execute( " UPDATE OR IGNORE queues SET error = :qid WHERE rowid = :qid;", m_errorQueue);



            //
            // Precompile all other statements
            //
            {
               m_statement.enqueue = m_connection.precompile( "INSERT INTO messages VALUES (?,?,?,?,?,?,?,?,?,?,?,?);");

               m_statement.dequeue = m_connection.precompile( R"( 
                     SELECT 
                        id, correlation, reply, redelivered, type, avalible, timestamp, payload
                     FROM 
                        messages 
                     WHERE queue = :queue AND state = 2 AND avalible < :avalible  ORDER BY timestamp ASC LIMIT 1; )");


               m_statement.state.xid =  m_connection.precompile( "UPDATE messages SET gtrid = :gtrid, state = 3 WHERE id = :id");
               m_statement.state.nullxid = m_connection.precompile( "DELETE FROM messages WHERE id = :id");


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
                     "UPDATE messages SET redelivered = 0, queue = ( SELECT error FROM queues WHERE id = messages.queue)"
                     " WHERE messages.redelivered > ( SELECT retries FROM queues WHERE id = messages.queue);");

               m_statement.information.queues = m_connection.precompile( R"(
                  SELECT
                     q.id, q.name, q.retries, q.error, q.type, COUNT( m.id)
                  FROM
                     queues q LEFT JOIN messages m ON q.id = m.queue AND m.state = 2
                  GROUP BY q.id 
                  ORDER BY q.id  
                      ;
                  )");

               m_statement.information.messages = m_connection.precompile( R"(
                  SELECT
                     m.id, m.type, m.state
                  FROM
                     messages m
                  WHERE
                     m.queue = ?
                )");

            }
         }

         Queue Database::create( Queue queue)
         {

            common::trace::internal::Scope trace{ "queue::Database::create", common::log::internal::queue};

            //
            // Create corresponding error queue
            //
            m_connection.execute( "INSERT INTO queues VALUES ( NULL,?,?,?,?);", queue.name + "_error", queue.retries, m_errorQueue, Queue::cErrorQueue);
            queue.error = m_connection.rowid();

            m_connection.execute( "INSERT INTO queues VALUES ( NULL,?,?,?,?);", queue.name, queue.retries, queue.error, Queue::cQueue);
            queue.id = m_connection.rowid();

            return queue;
         }

         void Database::updateQueue( const Queue& queue)
         {
            common::trace::internal::Scope trace{ "queue::Database::updateQueue", common::log::internal::queue};

            auto existing = Database::queue( queue.id);

            if( ! existing.empty())
            {
               m_connection.execute( "UPDATE queues SET name = :name, retries = :retries WHERE id = :id;", queue.name, queue.retries, queue.id);
               m_connection.execute( "UPDATE queues SET name = :name, retries = :retries WHERE id = :id;", queue.name + "_error", queue.retries, existing.front().error);

            }
         }
         void Database::removeQueue( Queue::id_type id)
         {
            common::trace::internal::Scope trace{ "queue::Database::removeQueue", common::log::internal::queue};

            auto existing = Database::queue( id);

            if( ! existing.empty())
            {
               m_connection.execute( "DELETE FROM queues WHERE id = :id;", existing.front().error);
               m_connection.execute( "DELETE FROM queues WHERE id = :id;", existing.front().id);
            }
         }

         std::vector< Queue> Database::queue( Queue::id_type id)
         {
            std::vector< Queue> result;

            auto query = m_connection.query( "SELECT q.id, q.name, q.retries, q.error FROM queues q WHERE q.id = :id AND q.type = :type", id, Queue::cQueue);

            auto row = query.fetch();

            if( ! row.empty())
            {
               result.push_back( local::transform::Queue()( row.front()));
            }

            return result;
         }

         std::vector< Queue> Database::update( std::vector< Queue> update, const std::vector< Queue::id_type>& remove)
         {
            std::vector< Queue> result;

            auto create = common::range::partition( update, []( const Queue& q){ return q.id == 0;});

            common::range::transform( create, result, std::bind( &Database::create, this, std::placeholders::_1));

            auto toUpdate = common::range::make( update) - create;

            common::range::for_each( toUpdate, std::bind( &Database::updateQueue, this, std::placeholders::_1));

            common::range::for_each( remove, std::bind( &Database::removeQueue, this, std::placeholders::_1));


            return result;
         }




         void Database::enqueue( const common::message::queue::enqueue::Request& message)
         {
            common::trace::internal::Scope trace{ "queue::Database::enqueue", common::log::internal::queue};

            common::log::internal::queue << "enqueue - qid: " << message.queue << " id: " << message.message.id << " size: " << message.message.payload.size() << " trid: " << message.trid << std::endl;

            auto gtrid = common::transaction::global( message.trid);

            long state = message.trid ? message::State::added : message::State::enqueued;

            auto avalible = std::chrono::time_point_cast< std::chrono::microseconds>( message.message.avalible).time_since_epoch().count();
            auto timestamp = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();

            m_statement.enqueue.execute(
                  message.message.id.get(),
                  message.queue,
                  message.queue,
                  gtrid,
                  message.message.correlation,
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
            common::trace::internal::Scope trace{ "queue::Database::dequeue", common::log::internal::queue};

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
            if( message.trid)
            {
               auto gtrid = common::transaction::global(  message.trid);

               m_statement.state.xid.execute( gtrid, result.id.get());
            }
            else
            {
               m_statement.state.nullxid.execute( result.id.get());
            }

            common::log::internal::queue << "dequeue - qid: " << message.queue << " id: " << result.id << " size: " << result.payload.size() << " trid: " << message.trid << std::endl;

            reply.message.push_back( std::move( result));

            return reply;
         }


         void Database::commit( const common::transaction::ID& id)
         {
            common::trace::internal::Scope trace{ "queue::Database::commit", common::log::internal::queue};

            common::log::internal::queue << "commit xid: " << id << std::endl;

            auto gtrid = common::transaction::global( id);

            m_statement.commit1.execute( gtrid);
            m_statement.commit2.execute( gtrid);
         }


         void Database::rollback( const common::transaction::ID& id)
         {
            common::trace::internal::Scope trace{ "queue::Database::rollback", common::log::internal::queue};

            common::log::internal::queue << "rollback xid: " << id << std::endl;

            auto gtrid = common::transaction::global( id);

            m_statement.rollback1.execute( gtrid);
            m_statement.rollback2.execute( gtrid);
            m_statement.rollback3.execute();
         }


         std::vector< common::message::queue::information::Queue> Database::queues()
         {
            common::trace::internal::Scope trace{ "queue::Database::queues", common::log::internal::queue};

            std::vector< common::message::queue::information::Queue> result;

            auto query = m_statement.information.queues.query();

            sql::database::Row row;

            while( query.fetch( row))
            {
               common::message::queue::information::Queue queue;


               row.get( 0, queue.id);
               row.get( 1, queue.name);
               row.get( 2, queue.retries);
               row.get( 3, queue.error);
               row.get( 4, queue.type);
               row.get( 5, queue.messages);

               result.push_back( std::move( queue));
            }
            return result;
         }

         std::vector< common::message::queue::information::Message> Database::messages( Queue::id_type id)
         {
            std::vector< common::message::queue::information::Message> result;

            auto query = m_statement.information.messages.query( id);

            sql::database::Row row;

            while( query.fetch( row))
            {
               common::message::queue::information::Message message;

               row.get( 0, message.id.get());
               row.get( 1, message.type);
               row.get( 2, message.state);

               result.push_back( std::move( message));
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

         void Database::begin() { m_connection.begin();}
         void Database::commit() { m_connection.commit();}
         void Database::rollback() { m_connection.rollback();}

      } // server
   } // queue

} // casual
