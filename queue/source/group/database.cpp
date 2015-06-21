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

// temp
#include "common/chronology.h"


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

                     std::tuple< std::int64_t, common::message::queue::dequeue::Reply::Message> operator () ( sql::database::Row& row) const
                     {

                        // SELECT ROWID, id, properties, reply, redelivered, type, avalible, timestamp, payload
                        std::tuple< std::int64_t, common::message::queue::dequeue::Reply::Message> result;
                        row.get( 0, std::get< 0>( result));

                        row.get( 1, std::get< 1>( result).id.get());
                        row.get( 2, std::get< 1>( result).properties);
                        row.get( 3, std::get< 1>( result).reply);
                        row.get( 4, std::get< 1>( result).redelivered);
                        row.get( 5, std::get< 1>( result).type.name);
                        row.get( 6, std::get< 1>( result).type.subname);

                        std::get< 1>( result).avalible = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( 7)}};
                        std::get< 1>( result).timestamp = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( 8)}};
                        row.get( 9, std::get< 1>( result).payload);

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




         Database::Database( const std::string& database, std::string groupname) : m_connection( database)
         {

            common::trace::internal::Scope trace{ "Database::Database", common::log::internal::queue};

            //
            // Make sure we got FK
            //
            m_connection.execute( "PRAGMA foreign_keys = ON;");

            //
            // We can't set WAL mode, for some reason...
            //
            //m_connection.execute( "PRAGMA journal_mode=WAL;");

            m_connection.execute(
                R"( CREATE TABLE IF NOT EXISTS queue 
                (
                  id           INTEGER  PRIMARY KEY,
                  name         TEXT     UNIQUE,
                  retries      INTEGER,
                  error        INTEGER,
                  type         INTEGER ); )"
              );


            m_connection.execute(
                  "CREATE INDEX IF NOT EXISTS i_id_queue  ON queue( id);" );


            m_connection.execute(
                R"( CREATE TABLE IF NOT EXISTS message 
                ( id            BLOB PRIMARY KEY,
                  queue         INTEGER,
                  origin        NUMBER, -- the first queue a message is enqueued to
                  gtrid         BLOB,
                  properties    TEXT,
                  state         INTEGER,
                  reply         TEXT,
                  redelivered   INTEGER,
                  type          TEXT,
                  subtype       TEXT,
                  avalible      INTEGER,
                  timestamp     INTEGER,
                  payload       BLOB,
                  FOREIGN KEY (queue) REFERENCES queue( id)); )");

            m_connection.execute(
                  "CREATE INDEX IF NOT EXISTS i_id_message  ON message ( id);" );

            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_queue_message  ON message ( queue);" );

            m_connection.execute(
              "CREATE INDEX IF NOT EXISTS i_dequeue_message  ON message ( queue, state, timestamp ASC);" );

            m_connection.execute(
              "CREATE INDEX IF NOT EXISTS i_dequeue_message_properties ON message ( queue, state, properties, timestamp ASC);" );

            /*
            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_timestamp_message  ON message ( timestamp ASC);" );
            */

            m_connection.execute(
               "CREATE INDEX IF NOT EXISTS i_gtrid_message  ON message ( gtrid);" );


            //
            // group error queue
            //
            if( groupname.empty())
            {
               groupname = common::uuid::string( common::uuid::make());
            }
            m_connection.execute( "INSERT OR REPLACE INTO queue VALUES ( 1, ?, 0, 1, 1); ", groupname + ".group.error");
            m_error_queue = 1;


            //
            // Precompile all other statements
            //
            {
               m_statement.enqueue = m_connection.precompile( "INSERT INTO message VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);");

               m_statement.dequeue.first = m_connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, subtype, avalible, timestamp, payload
                     FROM 
                        message 
                     WHERE queue = :queue AND state = 2 AND ( avalible is NULL OR avalible < :avalible) ORDER BY timestamp ASC LIMIT 1; )");

               m_statement.dequeue.first_id = m_connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, subtype, avalible, timestamp, payload
                     FROM 
                        message 
                     WHERE id = :id AND queue = :queue AND state = 2 AND ( avalible is NULL OR avalible < :avalible); )");

               m_statement.dequeue.first_match = m_connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, subtype, avalible, timestamp, payload
                     FROM 
                        message 
                     WHERE queue = :queue AND state = 2 AND properties = :properties AND ( avalible is NULL OR avalible < :avalible) ORDER BY timestamp ASC LIMIT 1; )");


               m_statement.state.xid =  m_connection.precompile( "UPDATE message SET gtrid = :gtrid, state = 3 WHERE ROWID = :id");
               m_statement.state.nullxid = m_connection.precompile( "DELETE FROM message WHERE ROWID = :id");


               m_statement.commit1 = m_connection.precompile( "UPDATE message SET state = 2 WHERE gtrid = :gtrid AND state = 1;");
               m_statement.commit2 = m_connection.precompile( "DELETE FROM message WHERE gtrid = :gtrid AND state = 3;");
               m_statement.commit3 = m_connection.precompile( "SELECT DISTINCT( queue) FROM message WHERE gtrid = :gtrid AND state = 2;");


               /*
               m_statement.rollback = m_connection.precompile( R"(
                 -- delete all enqueued  
                 DELETE FROM message WHERE gtrid = :gtrid AND state = 1;
                 -- update all dequeued back to enqueued
                 UPDATE message SET state = 2, redelivered = redelivered + 1  WHERE gtrid = :gtrid AND state = 3;
                 -- move to error queue
                 UPDATE message SET redelivered = 0, queue = ( SELECT error FROM queue WHERE rowid = message.queue)
                     WHERE message.redelivered > ( SELECT retries FROM queue WHERE rowid = message.queue);
                     )");

               */

               m_statement.rollback1 = m_connection.precompile( "DELETE FROM message WHERE gtrid = :gtrid AND state = 1;");
               m_statement.rollback2 = m_connection.precompile( "UPDATE message SET state = 2, redelivered = redelivered + 1  WHERE gtrid = :gtrid AND state = 3");
               m_statement.rollback3 = m_connection.precompile(
                     "UPDATE message SET redelivered = 0, queue = ( SELECT error FROM queue WHERE id = message.queue)"
                     " WHERE message.redelivered > ( SELECT retries FROM queue WHERE id = message.queue);");

               m_statement.information.queue = m_connection.precompile( R"(
                  SELECT
                     q.id, m.state, q.name, q.retries, q.error, q.type, COUNT( m.id), 
                       MIN( length( m.payload)), MAX( length( m.payload)), AVG( length( m.payload)), 
                       SUM( length( m.payload)), MAX( m.timestamp)
                  FROM
                     queue q LEFT JOIN message m ON q.id = m.queue
                  GROUP BY q.id, m.state 
                  ORDER BY q.id  
                      ;
                  )");

               /*
                  id            BLOB PRIMARY KEY,
                  queue         INTEGER,
                  origin        NUMBER, -- the first queue a message is enqueued to
                  gtrid         BLOB,
                  properties   TEXT,
                  state         INTEGER,
                  reply         TEXT,
                  redelivered   INTEGER,
                  type          TEXT,
                  subtype       TEXT,
                  avalible      INTEGER,
                  timestamp     INTEGER,
                  payload       BLOB,
                */
               m_statement.information.message = m_connection.precompile( R"(
                  SELECT
                     m.id, m.queue, m.origin, m.gtrid, m.state, m.reply, m.redelivered, m.type, m.subtype, m.avalible, m.timestamp, length( m.payload)
                  FROM
                     message m
                  WHERE
                     m.queue = ?
                )");

            }
         }


         std::string Database::file() const
         {
            return m_connection.file();
         }

         Queue Database::create( Queue queue)
         {

            common::trace::internal::Scope trace{ "queue::Database::create", common::log::internal::queue};

            common::log::internal::queue << "queue.name: " << queue.name << std::endl;


            //
            // Create corresponding error queue
            //
            m_connection.execute( "INSERT INTO queue VALUES ( NULL,?,?,?,?);", queue.name + ".error", queue.retries, m_error_queue, Queue::cErrorQueue);
            queue.error = m_connection.rowid();

            m_connection.execute( "INSERT INTO queue VALUES ( NULL,?,?,?,?);", queue.name, queue.retries, queue.error, Queue::cQueue);
            queue.id = m_connection.rowid();

            return queue;
         }

         void Database::updateQueue( const Queue& queue)
         {
            common::trace::internal::Scope trace{ "queue::Database::updateQueue", common::log::internal::queue};

            auto existing = Database::queue( queue.id);

            if( ! existing.empty())
            {
               m_connection.execute( "UPDATE queue SET name = :name, retries = :retries WHERE id = :id;", queue.name, queue.retries, queue.id);
               m_connection.execute( "UPDATE queue SET name = :name, retries = :retries WHERE id = :id;", queue.name + ".error", queue.retries, existing.front().error);

            }
         }
         void Database::removeQueue( Queue::id_type id)
         {
            common::trace::internal::Scope trace{ "queue::Database::removeQueue", common::log::internal::queue};

            auto existing = Database::queue( id);

            if( ! existing.empty())
            {
               m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.front().error);
               m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.front().id);
            }
         }

         std::vector< Queue> Database::queue( Queue::id_type id)
         {
            std::vector< Queue> result;

            auto query = m_connection.query( "SELECT q.id, q.name, q.retries, q.error FROM queue q WHERE q.id = :id AND q.type = :type", id, Queue::cQueue);

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

            common::range::transform( std::get< 0>( create), result, std::bind( &Database::create, this, std::placeholders::_1));

            common::range::for_each( std::get< 1>( create), std::bind( &Database::updateQueue, this, std::placeholders::_1));

            common::range::for_each( remove, std::bind( &Database::removeQueue, this, std::placeholders::_1));


            return result;
         }




         common::message::queue::enqueue::Reply Database::enqueue( const common::message::queue::enqueue::Request& message)
         {
            common::trace::internal::Scope trace{ "queue::Database::enqueue", common::log::internal::queue};

            common::message::queue::enqueue::Reply reply;

            //
            // We create a unique id if none is provided.
            //
            reply.id = message.message.id ? message.message.id : common::uuid::make();


            common::log::internal::queue << "enqueue - qid: " << message.queue << " id: " << reply.id << " size: " << message.message.payload.size() << " trid: " << message.trid << std::endl;

            auto gtrid = common::transaction::global( message.trid);

            long state = message.trid ? message::State::added : message::State::enqueued;


            m_statement.enqueue.execute(
                  reply.id.get(),
                  message.queue,
                  message.queue,
                  gtrid,
                  message.message.properties,
                  state,
                  message.message.reply,
                  0,
                  message.message.type.name,
                  message.message.type.subname,
                  message.message.avalible,
                  common::platform::clock_type::now(),
                  message.message.payload);

            return reply;
         }




         common::message::queue::dequeue::Reply Database::dequeue( const common::message::queue::dequeue::Request& message)
         {
            common::trace::internal::Scope trace{ "queue::Database::dequeue", common::log::internal::queue};

            common::message::queue::dequeue::Reply reply;

            auto now = std::chrono::time_point_cast< std::chrono::microseconds>(
                  common::platform::clock_type::now()).time_since_epoch().count();

            auto query = [&](){
               if( message.selector.id)
               {
                  return m_statement.dequeue.first_id.query( message.queue, now, message.selector.id.get());
               }
               if( ! message.selector.properties.empty())
               {
                  return m_statement.dequeue.first_match.query( message.queue, now, message.selector.properties);
               }
               return m_statement.dequeue.first.query( message.queue, now);
            };

            auto resultset = query();

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

               m_statement.state.xid.execute( gtrid, std::get< 0>( result));
            }
            else
            {
               m_statement.state.nullxid.execute( std::get< 0>( result));
            }

            common::log::internal::queue << "dequeue - qid: " << message.queue << " id: " << std::get< 1>( result).id << " size: " << std::get< 1>( result).payload.size() << " trid: " << message.trid << std::endl;

            reply.message.push_back( std::move( std::get< 1>( result)));

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

         std::vector< Queue::id_type> Database::committed( const common::transaction::ID& id)
         {
            std::vector< Queue::id_type> result;

            auto gtrid = common::transaction::global( id);
            auto resultset =  m_statement.commit3.query( gtrid);

            sql::database::Row row;

            while( resultset.fetch( row))
            {
               Queue::id_type queue;
               row.get( 0, queue);
               result.push_back( queue);
            }

            return result;
         }


         std::vector< common::message::queue::information::Queue> Database::queues()
         {
            common::trace::internal::Scope trace{ "queue::Database::queues", common::log::internal::queue};

            std::vector< common::message::queue::information::Queue> result;

            auto query = m_statement.information.queue.query();

            sql::database::Row row;

            while( query.fetch( row))
            {
               common::message::queue::information::Queue queue;


               row.get( 0, queue.id);
               row.get( 1, queue.message.state);
               row.get( 2, queue.name);
               row.get( 3, queue.retries);
               row.get( 4, queue.error);
               row.get( 5, queue.type);
               row.get( 6, queue.message.counts);
               row.get( 7, queue.message.size.min);
               row.get( 8, queue.message.size.max);
               row.get( 9, queue.message.size.average);
               row.get( 10, queue.message.size.total);
               row.get( 11, queue.message.timestamp);

               result.push_back( std::move( queue));
            }
            return result;
         }

         std::vector< common::message::queue::information::Message> Database::messages( Queue::id_type id)
         {
            std::vector< common::message::queue::information::Message> result;

            auto query = m_statement.information.message.query( id);

            sql::database::Row row;

            while( query.fetch( row))
            {
               /*
                m.id, m.queue, m.origin, m.gtrid, m.state, m.reply, m.redelivered, m.type, m.subtype, m.avalible, m.timestamp, length( m.payload)
                */
               common::message::queue::information::Message message;

               row.get( 0, message.id.get());
               row.get( 1, message.queue);
               row.get( 2, message.origin);
               row.get( 3, message.trid);
               row.get( 4, message.state);
               row.get( 5, message.reply);
               row.get( 6, message.redelivered);
               row.get( 7, message.type.name);
               row.get( 8, message.type.subname);
               row.get( 9, message.avalible);
               row.get( 10, message.timestamp);
               row.get( 11, message.size);

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
