//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/database.h"
#include "queue/common/log.h"

#include "common/algorithm.h"
#include "common/execute.h"
#include "common/exception/casual.h"
#include "common/event/send.h"


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

                  void row( sql::database::Row& row, common::message::queue::information::Message& message)
                  {
                     row.get( 0, message.id.get());
                     row.get( 1, message.queue.underlaying());
                     row.get( 2, message.origin.underlaying());
                     row.get( 3, message.trid);
                     row.get( 4, message.properties);
                     row.get( 5, message.state);
                     row.get( 6, message.reply);
                     row.get( 7, message.redelivered);
                     row.get( 8, message.type);
                     row.get( 9, message.available);
                     row.get( 10, message.timestamp);
                     row.get( 11, message.size);
                  }

                  void row( sql::database::Row& row, common::message::queue::dequeue::Reply::Message& message, int offset = 0)
                  {
                     row.get( offset, message.id.get());
                     row.get( offset + 1, message.properties);
                     row.get( offset + 2, message.reply);
                     row.get( offset + 3, message.redelivered);
                     row.get( offset + 4, message.type);

                     message.available = common::platform::time::point::type{ std::chrono::microseconds{ row.get< common::platform::time::point::type::rep>( offset + 5)}};
                     message.timestamp = common::platform::time::point::type{ std::chrono::microseconds{ row.get< common::platform::time::point::type::rep>( offset + 6)}};
                     row.get( offset + 7, message.payload);
                  }


                  struct Reply
                  {

                     std::tuple< std::int64_t, common::message::queue::dequeue::Reply::Message> operator () ( sql::database::Row& row) const
                     {

                        // SELECT ROWID, id, properties, reply, redelivered, type, avalible, timestamp, payload
                        std::tuple< std::int64_t, common::message::queue::dequeue::Reply::Message> result;
                        row.get( 0, std::get< 0>( result));

                        transform::row( row, std::get< 1>( result), 1);

                        return result;
                     }

                  };

                  struct Queue
                  {
                     group::Queue operator () ( sql::database::Row& row) const
                     {
                        group::Queue result;

                        row.get( 0, result.id.underlaying());
                        row.get( 1, result.name);
                        row.get( 2, result.retries);
                        row.get( 3, result.error.underlaying());
                        row.get( 4, result.type);

                        return result;
                     }

                  };

               } // transform


               template< typename C>
               void check_version( C& connection)
               {
                  Trace trace{ "queue::group::database::local::check_version"};

                  auto version = sql::database::version::get( connection);

                  common::log::line( log, "queue-base version: ", version);

                  if( ! version && connection.table( "queue"))
                  {
                     const auto message = "please convert to a newer version of the queue-base - or remove the current";
                     common::event::error::send( message, common::event::error::Severity::error);
                     throw common::exception::casual::invalid::Version{ message};
                  }

                  sql::database::version::set( connection, sql::database::Version{ 1, 0});
               }
            } // <unnamed>
         } // local



         Database::Database( const std::string& database, std::string groupname) 
            : m_connection( database), m_name( std::move( groupname))
         {

            Trace trace{ "queue::group::Database::Database"};

            local::check_version( m_connection);

            auto update_name_mapping = common::execute::scope( [&](){
               update_mapping();
            });


            //
            // Make sure we got FK
            //
            m_connection.execute( "PRAGMA foreign_keys = ON;");

            //
            // We can't set WAL mode, for some reason...
            //
            //m_connection.execute( "PRAGMA journal_mode=WAL;");

            {
               Trace trace{ "queue::group::Database::Database create table queue"};
            
               m_connection.execute(
                  R"( CREATE TABLE IF NOT EXISTS queue 
                  (
                     id           INTEGER  PRIMARY KEY,
                     name         TEXT     UNIQUE,
                     retries      INTEGER  NOT NULL,
                     error        INTEGER  NOT NULL,
                     type         INTEGER  NOT NULL,
                     count        INTEGER  NOT NULL, -- number of (committed) messages
                     size         INTEGER  NOT NULL, -- total size of all (committed) messages
                     uncommitted_count INTEGER  NOT NULL, -- uncommitted messages
                     pending      INTEGER  NOT NULL, -- number of pending dequeues wating
                     timestamp    INTEGER NOT NULL -- last update to the queue 
                     ); )"
               );

               m_connection.execute(
                     "CREATE INDEX IF NOT EXISTS i_id_queue ON queue ( id);" );
               
               // make sure pending is reset when we start
               m_connection.execute( "UPDATE queue SET pending = 0;");
            }

            {
               Trace trace{ "queue::group::Database::Database create table message"};

               m_connection.execute(
                  R"( CREATE TABLE IF NOT EXISTS message 
                  ( id            BLOB PRIMARY KEY,
                     queue         INTEGER,
                     origin        NUMBER, -- the first queue a message is enqueued to
                     gtrid         BLOB,
                     properties    TEXT,
                     state         INTEGER, -- (1: enqueued, 2: committed, 3: dequeued)
                     reply         TEXT,
                     redelivered   INTEGER,
                     type          TEXT,
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

               m_connection.execute(
                  "CREATE INDEX IF NOT EXISTS i_timestamp_message  ON message ( timestamp ASC);" );

               m_connection.execute(
                  "CREATE INDEX IF NOT EXISTS i_gtrid_message  ON message ( gtrid);" );
            }


            auto now = common::platform::time::clock::type::now();

            {
               Trace trace{ "queue::group::Database::Database create group error queue"};
               //
               // group error queue
               //
               if( m_name.empty())
               {
                  m_name = common::uuid::string( common::uuid::make());
               }
               m_connection.execute( "INSERT OR REPLACE INTO queue VALUES ( 1, ?, 0, 1, 1, 0, 0, 0, 0, ?); ", m_name + ".group.error", now);
               m_error_queue = common::strong::queue::id{ 1};
            }



            //
            // Triggers
            //
            {
               Trace trace{ "queue::group::Database::Database create triggers"};
           
               m_connection.execute( R"(
                  CREATE TRIGGER IF NOT EXISTS insert_message INSERT ON message 
                  BEGIN
                     UPDATE queue SET
                        timestamp = new.timestamp,
                        uncommitted_count = uncommitted_count + 1
                     WHERE id = new.queue AND new.state = 1;

                  UPDATE queue SET
                        timestamp = new.timestamp,
                        count = count + 1,
                        size = size + length( new.payload)
                     WHERE id = new.queue AND new.state = 2;

                  END;

                  )");

               m_connection.execute( R"(
                  CREATE TRIGGER IF NOT EXISTS update_message_state UPDATE OF state ON message 
                  BEGIN
                     UPDATE queue SET
                        count = count + 1,
                        size = size + length( new.payload),
                        uncommitted_count = uncommitted_count - 1
                     WHERE id = new.queue AND old.state = 1 AND new.state = 2;
                  END;

                  )");

               m_connection.execute( R"(
                  CREATE TRIGGER IF NOT EXISTS update_message_queue UPDATE OF queue ON message 
                  BEGIN

                     UPDATE queue SET  -- 'queue move' update the new queue
                        count = count + 1,
                        size = size + length( new.payload)
                     WHERE id = new.queue AND new.queue != old.queue;

                     UPDATE queue SET  -- 'queue move' update the old queue
                        count = count - 1,
                        size = size - length( old.payload)
                     WHERE id = old.queue AND old.queue != new.queue;

                  END;

                  )");

               m_connection.execute( R"(
                  CREATE TRIGGER IF NOT EXISTS delete_message DELETE ON message 
                  BEGIN
                     UPDATE queue SET 
                        count = count - 1,
                        size = size - length( old.payload)
                     WHERE id = old.queue AND old.state IN ( 2, 3);

                     UPDATE queue SET 
                        uncommitted_count = uncommitted_count - 1
                     WHERE id = old.queue AND old.state = 1;
                  END;

                  )");
            }

            //
            // Precompile all other statements
            //
            {
               Trace trace{ "queue::group::Database::Database precompile statements"};

               m_statement.enqueue = m_connection.precompile( "INSERT INTO message VALUES (?,?,?,?,?,?,?,?,?,?,?,?);");

               m_statement.dequeue.first = m_connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, avalible, timestamp, payload
                     FROM 
                        message 
                     WHERE queue = :queue AND state = 2 AND ( avalible is NULL OR avalible < :avalible) ORDER BY timestamp ASC LIMIT 1; )");

               m_statement.dequeue.first_id = m_connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, avalible, timestamp, payload
                     FROM 
                        message 
                     WHERE id = :id AND queue = :queue AND state = 2 AND ( avalible is NULL OR avalible < :avalible); )");

               m_statement.dequeue.first_match = m_connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, avalible, timestamp, payload
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


               /*
                  id           INTEGER  PRIMARY KEY,
                  name         TEXT     UNIQUE,
                  retries      INTEGER,
                  error        INTEGER,
                  type         INTEGER,
                  count        INTEGER, -- number of (committed) messages
                  size         INTEGER, -- total size of all (committed) messages
                  uncommitted_count INTEGER, -- uncommitted messages
                  pending  --
                  timestamp    INTEGER, -- last update to the queue
                */

               m_statement.information.queue = m_connection.precompile( R"(
                  SELECT
                     q.id, q.name, q.retries, q.error, q.type, q.count, q.size, q.uncommitted_count, q.pending, q.timestamp
                  FROM
                     queue q  
                      ;
                  )");

               /*
                  id            BLOB PRIMARY KEY,
                  queue         INTEGER,
                  origin        NUMBER, -- the first queue a message is enqueued to
                  gtrid         BLOB,
                  properties    TEXT,
                  state         INTEGER,
                  reply         TEXT,
                  redelivered   INTEGER,
                  type          TEXT,
                  avalible      INTEGER,
                  timestamp     INTEGER,
                  payload       BLOB,
                */
               m_statement.information.message = m_connection.precompile( R"(
                  SELECT
                     m.id, m.queue, m.origin, m.gtrid, m.properties, m.state, m.reply, m.redelivered, m.type, m.avalible, m.timestamp, length( m.payload)
                  FROM
                     message m
                  WHERE
                     m.queue = ?
                )");


               m_statement.peek.match = m_connection.precompile( R"( 
                  SELECT 
                     m.id, m.queue, m.origin, m.gtrid, m.properties, m.state, m.reply, m.redelivered, m.type, m.avalible, m.timestamp, length( m.payload)
                  FROM 
                     message  m
                  WHERE m.queue = :queue AND m.properties = :properties;)");


               m_statement.peek.one_message = m_connection.precompile( R"( 
                  SELECT 
                    id, properties, reply, redelivered, type, avalible, timestamp, payload
                  FROM 
                     message 
                  WHERE id = :id; )");

               m_statement.restore = m_connection.precompile(R"( 
                  UPDATE message 
                  SET queue = :queue 
                  WHERE state = 2 AND queue != origin AND origin = :queue; )");


               m_statement.pending.add = m_connection.precompile( R"(
                  UPDATE queue
                  SET pending = pending + 1 
                  WHERE id = :id; 
                 )");

               m_statement.pending.set = m_connection.precompile( R"(
                  UPDATE queue
                  SET pending = :pending
                  WHERE id = :id; 
                 )");
               m_statement.pending.check = m_connection.precompile( R"( 
                  SELECT 
                    id, pending, count
                  FROM 
                     queue
                  WHERE pending > 0 AND count > 0;
                  )");
            }
         }


         std::string Database::file() const
         {
            return m_connection.file();
         }

         Queue Database::create( Queue queue)
         {

            Trace trace{ "queue::Database::create"};



            /*
                  id           INTEGER  PRIMARY KEY,
                  name         TEXT     UNIQUE,
                  retries      INTEGER  NOT NULL,
                  error        INTEGER  NOT NULL,
                  type         INTEGER  NOT NULL,
                  count        INTEGER  NOT NULL, -- number of (committed) messages
                  size         INTEGER  NOT NULL, -- total size of all (committed) messages
                  uncommitted_count INTEGER  NOT NULL, -- uncommitted messages
                  timestamp    INTEGER NOT NULL -- last update to the queue
             */

            auto now = common::platform::time::clock::type::now();

            //
            // Create corresponding error queue
            //
            m_connection.execute( "INSERT INTO queue VALUES ( NULL,?,?,?,?, 0, 0, 0, 0, ?);", queue.name + ".error", queue.retries, m_error_queue.value(), Queue::Type::error_queue, now);
            queue.error = common::strong::queue::id{ m_connection.rowid()};

            m_connection.execute( "INSERT INTO queue VALUES ( NULL,?,?,?,?, 0, 0, 0, 0, ?);", queue.name, queue.retries, queue.error.value(), Queue::Type::queue, now);
            queue.id = common::strong::queue::id{ m_connection.rowid()};
            queue.type = Queue::Type::queue;

            common::log::line( log, "queue: ", queue);

            update_mapping();

            return queue;
         }

         void Database::update_queue( const Queue& queue)
         {
            Trace trace{ "queue::Database::updateQueue"};

            auto existing = Database::queue( queue.id);

            if( ! existing.empty())
            {
               m_connection.execute( "UPDATE queue SET name = :name, retries = :retries WHERE id = :id;", queue.name, queue.retries, queue.id.value());
               m_connection.execute( "UPDATE queue SET name = :name, retries = :retries WHERE id = :id;", queue.name + ".error", queue.retries, existing.front().error.value());
            }
         }
         void Database::remove_queue( common::strong::queue::id id)
         {
            Trace trace{ "queue::Database::removeQueue"};

            auto existing = Database::queue( id);

            if( ! existing.empty())
            {
               m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.front().error.value());
               m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.front().id.value());
            }
         }

         std::vector< Queue> Database::queue( common::strong::queue::id id)
         {
            std::vector< Queue> result;

            auto query = m_connection.query( "SELECT q.id, q.name, q.retries, q.error FROM queue q WHERE q.id = :id AND q.type = :type", id.value(), Queue::Type::queue);

            auto row = query.fetch();

            if( ! row.empty())
            {
               result.push_back( local::transform::Queue()( row.front()));
            }

            return result;
         }

         void Database::update_mapping()
         {
            Trace trace{ "queue::Database::update_mapping"};

            m_name_mapping.clear();

            for( auto&& queue : queues())
            {
               m_name_mapping[ queue.name] = queue.id;
            }
         }


         std::vector< Queue> Database::update( std::vector< Queue> update, const std::vector< common::strong::queue::id>& remove)
         {
            Trace trace{ "queue::Database::update"};

            common::log::line( verbose::log, "update: ", update, " - remove: ", remove);

            std::vector< Queue> result;

            auto create = common::algorithm::partition( update, []( const Queue& q){ return q.id.empty();});

            common::algorithm::transform( std::get< 0>( create), result, [&]( auto& q){ return this->create( q);});

            common::algorithm::for_each( std::get< 1>( create), [&]( auto& q){ this->update_queue( q);});

            common::algorithm::for_each( remove, [&]( auto id){ this->remove_queue( id);});


            update_mapping();

            return result;
         }


         common::message::queue::enqueue::Reply Database::enqueue( const common::message::queue::enqueue::Request& message)
         {
            Trace trace{ "queue::Database::enqueue"};

            common::log::line( verbose::log, "message: ", message);

            auto reply = common::message::reverse::type( message);


            //
            // We create a unique id if none is provided.
            //
            reply.id = message.message.id ? message.message.id : common::uuid::make();

            auto gtrid = common::transaction::global( message.trid);

            long state = message.trid ? message::State::added : message::State::enqueued;


            m_statement.enqueue.execute(
                  reply.id.get(),
                  message.queue.value(),
                  message.queue.value(),
                  gtrid,
                  message.message.properties,
                  state,
                  message.message.reply,
                  0,
                  message.message.type,
                  message.message.available,
                  common::platform::time::clock::type::now(),
                  message.message.payload);
            

            

            common::log::line( verbose::log, "reply: ", reply);
            return reply;
         }


         common::message::queue::dequeue::Reply Database::dequeue( const common::message::queue::dequeue::Request& message)
         {
            Trace trace{ "queue::Database::dequeue"};

            common::log::line( verbose::log, "message: ", message);

            common::message::queue::dequeue::Reply reply;

            auto now = std::chrono::time_point_cast< std::chrono::microseconds>(
                  common::platform::time::clock::type::now()).time_since_epoch().count();

            auto query = [&](){
               if( message.selector.id)
               {
                  return m_statement.dequeue.first_id.query( message.selector.id.get(), message.queue.value(), now);
               }
               if( ! message.selector.properties.empty())
               {
                  return m_statement.dequeue.first_match.query( message.queue.value(), message.selector.properties, now);
               }
               return m_statement.dequeue.first.query( message.queue.value(), now);
            };

            auto resultset = query();

            sql::database::Row row;

            if( ! resultset.fetch( row))
            {
               common::log::line( log, "dequeue - qid: ", message.queue, " - no message");

               if( message.block)
                  pending_add( message);

               return reply;
            }

            auto result = local::transform::Reply{}( row);

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

            reply.message.push_back( std::move( std::get< 1>( result)));

            common::log::line( verbose::log, "reply: ", reply);

            return reply;
         }

         common::message::queue::peek::information::Reply Database::peek( const common::message::queue::peek::information::Request& request)
         {
            Trace trace{ "queue::Database::peek information"};

            common::log::line( verbose::log, "message: ", request);

            auto reply = common::message::reverse::type( request);


            auto get_query = [&](){
               if( ! request.selector.properties.empty())
               {
                  return m_statement.peek.match.query( request.queue.value(), request.selector.properties);
               }
               return m_statement.information.message.query( request.queue.value());
            };

            auto query = get_query();

            sql::database::Row row;

            while( query.fetch( row))
            {
               /*
                m.id, m.queue, m.origin, m.gtrid, m.state, m.reply, m.redelivered, m.type, m.avalible, m.timestamp, length( m.payload)
                */

               common::message::queue::information::Message message;

               local::transform::row(row, message);

               reply.messages.push_back( std::move( message));
            }

            return reply;
         }

         common::message::queue::peek::messages::Reply Database::peek( const common::message::queue::peek::messages::Request& request)
         {
            Trace trace{ "queue::Database::peek messages"};

            common::log::line( verbose::log, "message: ", request);

            auto reply = common::message::reverse::type( request);

            sql::database::Row row;

            for( auto& id : request.ids)
            {
               auto query = m_statement.peek.one_message.query( id.get());

               if( query.fetch( row))
               {
                  common::message::queue::dequeue::Reply::Message message;

                  local::transform::row(row, message);

                  reply.messages.push_back( std::move( message));
               }
               else
               {
                  log << "failed to find message with id: " << id << std::endl;
               }
            }

            return reply;
         }

         size_type Database::restore( common::strong::queue::id queue)
         {
            Trace trace{ "queue::Database::restore"};

            log << "queue: " << queue << std::endl;

            m_statement.restore.execute( queue.value());
            return m_connection.affected();
         }


         void Database::commit( const common::transaction::ID& id)
         {
            Trace trace{ "queue::Database::commit"};

            log << "commit xid: " << id << std::endl;

            auto gtrid = common::transaction::global( id);

            m_statement.commit1.execute( gtrid);
            m_statement.commit2.execute( gtrid);
         }


         void Database::rollback( const common::transaction::ID& id)
         {
            Trace trace{ "queue::Database::rollback"};

            log << "rollback xid: " << id << std::endl;

            auto gtrid = common::transaction::global( id);

            m_statement.rollback1.execute( gtrid);
            m_statement.rollback2.execute( gtrid);
            m_statement.rollback3.execute();
         }

         std::vector< common::strong::queue::id> Database::committed( const common::transaction::ID& id)
         {
            std::vector< common::strong::queue::id> result;

            auto gtrid = common::transaction::global( id);
            auto resultset =  m_statement.commit3.query( gtrid);

            sql::database::Row row;

            while( resultset.fetch( row))
            {
               common::strong::queue::id::value_type queue;
               row.get( 0, queue);
               result.emplace_back( queue);
            }

            return result;
         }

         void Database::pending_add( const common::message::queue::dequeue::Request& request)
         {
            Trace trace{ "queue::Database::pending_add"};

            common::log::line( log, "request: ", request);

            m_requests.push_back( request);
            pending_add( request.queue);
         }

         common::message::queue::dequeue::forget::Reply Database::pending_forget( const common::message::queue::dequeue::forget::Request& request)
         {
            Trace trace{ "queue::Database::pending_forget"};

            common::message::queue::dequeue::forget::Reply reply;
            reply.correlation = request.correlation;

            auto found = common::algorithm::find_if( m_requests, [&]( const auto& r){
               return r.queue == request.queue && r.process == request.process;
            });

            reply.found = static_cast< bool>( found);

            if( found)
            {
               m_requests.erase( std::begin( found));
            }

            return reply;
         }

         std::vector< common::message::pending::Message> Database::pending_forget()
         {
            Trace trace{ "queue::Database::pending_forget all"};

            return common::algorithm::transform( std::exchange( m_requests, {}), []( auto& pending){
                  common::message::queue::dequeue::forget::Request forget;
                  
                  forget.process = common::process::handle();
                  forget.queue = pending.queue;

                  return common::message::pending::Message{ std::move( forget), pending.process.queue};
            });
            
         }

         std::vector< common::message::queue::dequeue::Request> Database::pending()
         {
            Trace trace{ "queue::Database::pending"};

            const auto queues = get_pending();

            if( queues.empty())
               return {};

            std::vector< common::message::queue::dequeue::Request> result;

            auto requests = common::range::make( m_requests);

            for( auto& queue : queues)
            {
               auto splice = common::algorithm::stable_partition( requests, [&]( auto& r){ return r.queue != queue.id;});

               auto prospects = std::get< 1>( splice);

               common::log::line( verbose::log, "queue.count: ", queue.count, " - prospect.size: ", prospects.size());

               if( prospects.size() <= queue.count)
               {
                  common::algorithm::move( prospects, result);
                  requests = std::get< 0>( splice);
                  pending_set( queue.id, 0);
               }
               else 
               {
                  // we have more pending than new messages, 

                  auto replies = common::range::make( std::begin( prospects), std::begin( prospects) + queue.count);
                  common::algorithm::move( replies, result);

                  // clean up, and move the consumed to the back, for erasure.
                  requests = common::range::make( 
                     std::begin( requests),
                     std::rotate( std::begin( replies), std::end( replies), std::end( prospects)));

                  pending_set( queue.id, prospects.size() - replies.size());
               }
            }
            m_requests.erase( std::end( requests), std::end( m_requests));


            return result;
         }

         void Database::pending_erase( common::strong::process::id pid)
         {
            Trace trace{ "queue::Database::pending_erase"};

            auto found = std::get< 1>( common::algorithm::stable_partition( m_requests, [=]( const auto& r){
               return r.process.pid != pid;
            }));

            m_requests.erase( std::begin( found), std::end( found));
         }


         void Database::pending_add( common::strong::queue::id id)
         {
            Trace trace{ "queue::Database::pending_add"};

            common::log::line( verbose::log, "id: ", id);

            m_statement.pending.add.execute( id.value());
         }

         void Database::pending_set( common::strong::queue::id id, common::platform::size::type value)
         {
            Trace trace{ "queue::Database::pending_set"};

            common::log::line( verbose::log, "id: ", id, " - value: ", value);

            m_statement.pending.set.execute( value, id.value());
         }

            
         std::vector< pending::Dequeue> Database::get_pending()
         {
            Trace trace{ "queue::Database::get_pending"};

            auto query = m_statement.pending.check.query();

            std::vector< pending::Dequeue> result;

            sql::database::Row row;

            while( query.fetch( row))
            {
               pending::Dequeue pending;

               row.get( 0, pending.id.underlaying());
               row.get( 1, pending.pending);
               row.get( 2, pending.count);

               result.push_back( pending);
            }
            return result;
         }

         std::vector< common::message::queue::information::Queue> Database::queues()
         {
            Trace trace{ "queue::Database::queues"};

            std::vector< common::message::queue::information::Queue> result;

            auto query = m_statement.information.queue.query();

            sql::database::Row row;

            while( query.fetch( row))
            {

               /*
                  id           INTEGER  PRIMARY KEY,
                  name         TEXT     UNIQUE,
                  retries      INTEGER  NOT NULL,
                  error        INTEGER  NOT NULL,
                  type         INTEGER  NOT NULL,
                  count        INTEGER  NOT NULL, -- number of (committed) messages
                  size         INTEGER  NOT NULL, -- total size of all (committed) messages
                  uncommitted_count INTEGER  NOT NULL, -- uncommitted messages
                  pending     ---
                  timestamp    INTEGER NOT NULL, -- last update to the queue
                */
               common::message::queue::information::Queue queue;

               row.get( 0, queue.id.underlaying());
               row.get( 1, queue.name);
               row.get( 2, queue.retries);
               row.get( 3, queue.error.underlaying());
               row.get( 4, queue.type);
               row.get( 5, queue.count);
               row.get( 6, queue.size);
               row.get( 7, queue.uncommitted);
               row.get( 8, queue.pending);
               row.get( 9, queue.timestamp);

               result.push_back( std::move( queue));
            }
            return result;
         }

         std::vector< common::message::queue::information::Message> Database::messages( common::strong::queue::id id)
         {
            std::vector< common::message::queue::information::Message> result;

            auto query = m_statement.information.message.query( id.value());

            sql::database::Row row;

            while( query.fetch( row))
            {
               /*
                m.id, m.queue, m.origin, m.gtrid, m.state, m.reply, m.redelivered, m.type, m.avalible, m.timestamp, length( m.payload)
                */
               common::message::queue::information::Message message;

               local::transform::row(row, message);

               result.push_back( std::move( message));
            }

            return result;
         }


         size_type Database::affected() const
         {
            return m_connection.affected();
         }


         void Database::begin() { m_connection.exclusive_begin();}
         void Database::commit() { m_connection.commit();}
         void Database::rollback() { m_connection.rollback();}


         sql::database::Version Database::version()
         {
            return sql::database::version::get( m_connection);
         }

      } // server
   } // queue

} // casual
