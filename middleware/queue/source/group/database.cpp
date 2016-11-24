//!
//! casual
//!

#include "queue/group/database.h"
#include "queue/common/log.h"

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

                  void row( sql::database::Row& row, common::message::queue::information::Message& message)
                  {
                     row.get( 0, message.id.get());
                     row.get( 1, message.queue);
                     row.get( 2, message.origin);
                     row.get( 3, message.trid);
                     row.get( 4, message.properties);
                     row.get( 5, message.state);
                     row.get( 6, message.reply);
                     row.get( 7, message.redelivered);
                     row.get( 8, message.type);
                     row.get( 9, message.avalible);
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

                     message.avalible = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( offset + 5)}};
                     message.timestamp = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( offset + 6)}};
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

            Trace trace{ "Database::Database"};

            auto update_name_mapping = common::scope::execute( [&](){
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
                  timestamp    INTEGER NOT NULL -- last update to the queue 
                   ); )"
              );

            m_connection.execute(
                  "CREATE INDEX IF NOT EXISTS i_id_queue ON queue ( id);" );


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


            auto now = common::platform::clock_type::now();
            //
            // group error queue
            //
            if( groupname.empty())
            {
               groupname = common::uuid::string( common::uuid::make());
            }
            m_connection.execute( "INSERT OR REPLACE INTO queue VALUES ( 1, ?, 0, 1, 1, 0, 0, 0, ?); ", groupname + ".group.error", now);
            m_error_queue = 1;



            //
            // Triggers
            //

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

            //
            // Precompile all other statements
            //
            {
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
                  timestamp    INTEGER, -- last update to the queue
                */

               m_statement.information.queue = m_connection.precompile( R"(
                  SELECT
                     q.id, q.name, q.retries, q.error, q.type, q.count, q.size, q.uncommitted_count, q.timestamp
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

            auto now = common::platform::clock_type::now();

            //
            // Create corresponding error queue
            //
            m_connection.execute( "INSERT INTO queue VALUES ( NULL,?,?,?,?, 0, 0, 0, ?);", queue.name + ".error", queue.retries, m_error_queue, Queue::Type::error_queue, now);
            queue.error = m_connection.rowid();

            m_connection.execute( "INSERT INTO queue VALUES ( NULL,?,?,?,?, 0, 0, 0, ?);", queue.name, queue.retries, queue.error, Queue::Type::queue, now);
            queue.id = m_connection.rowid();
            queue.type = Queue::Type::queue;

            log << "queue: " << queue << std::endl;

            update_mapping();

            return queue;
         }

         void Database::updateQueue( const Queue& queue)
         {
            Trace trace{ "queue::Database::updateQueue"};

            auto existing = Database::queue( queue.id);

            if( ! existing.empty())
            {
               m_connection.execute( "UPDATE queue SET name = :name, retries = :retries WHERE id = :id;", queue.name, queue.retries, queue.id);
               m_connection.execute( "UPDATE queue SET name = :name, retries = :retries WHERE id = :id;", queue.name + ".error", queue.retries, existing.front().error);
            }
         }
         void Database::removeQueue( Queue::id_type id)
         {
            Trace trace{ "queue::Database::removeQueue"};

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

            auto query = m_connection.query( "SELECT q.id, q.name, q.retries, q.error FROM queue q WHERE q.id = :id AND q.type = :type", id, Queue::Type::queue);

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


         std::vector< Queue> Database::update( std::vector< Queue> update, const std::vector< Queue::id_type>& remove)
         {
            Trace trace{ "queue::Database::update"};

            std::vector< Queue> result;

            auto create = common::range::partition( update, []( const Queue& q){ return q.id == 0;});

            common::range::transform( std::get< 0>( create), result, std::bind( &Database::create, this, std::placeholders::_1));

            common::range::for_each( std::get< 1>( create), std::bind( &Database::updateQueue, this, std::placeholders::_1));

            common::range::for_each( remove, std::bind( &Database::removeQueue, this, std::placeholders::_1));


            update_mapping();

            return result;
         }




         common::message::queue::enqueue::Reply Database::enqueue( const common::message::queue::enqueue::Request& message)
         {
            Trace trace{ "queue::Database::enqueue"};

            log << "request: " << message << std::endl;

            auto reply = common::message::reverse::type( message);


            //
            // We create a unique id if none is provided.
            //
            reply.id = message.message.id ? message.message.id : common::uuid::make();

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
                  message.message.type,
                  message.message.avalible,
                  common::platform::clock_type::now(),
                  message.message.payload);

            return reply;
         }




         common::message::queue::dequeue::Reply Database::dequeue( const common::message::queue::dequeue::Request& message)
         {
            Trace trace{ "queue::Database::dequeue"};

            log << "request: " << message << std::endl;

            common::message::queue::dequeue::Reply reply;

            auto now = std::chrono::time_point_cast< std::chrono::microseconds>(
                  common::platform::clock_type::now()).time_since_epoch().count();

            auto query = [&](){
               if( message.selector.id)
               {
                  return m_statement.dequeue.first_id.query( message.selector.id.get(), message.queue, now);
               }
               if( ! message.selector.properties.empty())
               {
                  return m_statement.dequeue.first_match.query( message.queue, message.selector.properties, now);
               }
               return m_statement.dequeue.first.query( message.queue, now);
            };

            auto resultset = query();

            sql::database::Row row;

            if( ! resultset.fetch( row))
            {
               log << "dequeue - qid: " << message.queue << " - no message" << std::endl;

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

            log << "dequeue - qid: " << message.queue << " id: " << std::get< 1>( result).id << " size: " << std::get< 1>( result).payload.size() << " trid: " << message.trid << std::endl;

            reply.message.push_back( std::move( std::get< 1>( result)));

            return reply;
         }

         common::message::queue::peek::information::Reply Database::peek( const common::message::queue::peek::information::Request& request)
         {
            Trace trace{ "queue::Database::peek information"};

            log << "request: " << request << std::endl;

            auto reply = common::message::reverse::type( request);


            auto get_query = [&](){
               if( ! request.selector.properties.empty())
               {
                  return m_statement.peek.match.query( request.queue, request.selector.properties);
               }
               return m_statement.information.message.query( request.queue);

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

            log << "request: " << request << std::endl;

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
                  timestamp    INTEGER NOT NULL, -- last update to the queue
                */
               common::message::queue::information::Queue queue;

               row.get( 0, queue.id);
               row.get( 1, queue.name);
               row.get( 2, queue.retries);
               row.get( 3, queue.error);
               row.get( 4, queue.type);
               row.get( 5, queue.count);
               row.get( 6, queue.size);
               row.get( 7, queue.uncommitted);
               row.get( 8, queue.timestamp);

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
                m.id, m.queue, m.origin, m.gtrid, m.state, m.reply, m.redelivered, m.type, m.avalible, m.timestamp, length( m.payload)
                */
               common::message::queue::information::Message message;

               local::transform::row(row, message);

               result.push_back( std::move( message));
            }

            return result;
         }


         std::size_t Database::affected() const
         {
            return m_connection.affected();
         }


         void Database::begin() { m_connection.exclusive_begin();}
         void Database::commit() { m_connection.commit();}
         void Database::rollback() { m_connection.rollback();}


      } // server
   } // queue

} // casual
