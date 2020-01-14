//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/group/database/statement.h"

#include "queue/common/log.h"

namespace casual
{
   namespace queue
   {
      namespace group
      {
         namespace database
         {
            Statement statement( sql::database::Connection& connection)
            {
               Trace trace{ "queue::group::Database::Database precompile statements"};

               Statement result;

               /*
                  id            BLOB PRIMARY KEY,
                  queue         INTEGER,
                  origin        NUMBER, -- the first queue a message is enqueued to
                  gtrid         BLOB,
                  properties    TEXT,
                  state         INTEGER, -- (1: enqueued, 2: committed, 3: dequeued)
                  reply         TEXT,
                  redelivered   INTEGER,
                  type          TEXT,
                  available     INTEGER,
                  timestamp     INTEGER,
                  payload       BLOB,
               */
               result.enqueue = connection.precompile( "INSERT INTO message VALUES (?,?,?,?,?,?,?,?,?,?,?,?);");

               result.dequeue.first = connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, available, timestamp, payload
                     FROM 
                        message 
                     WHERE queue = :queue AND state = 2 AND  available < :available ORDER BY timestamp ASC LIMIT 1; )");

               result.dequeue.first_id = connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, available, timestamp, payload
                     FROM 
                        message 
                     WHERE id = :id AND queue = :queue AND state = 2 AND available < :available; )");

               result.dequeue.first_match = connection.precompile( R"( 
                     SELECT 
                        ROWID, id, properties, reply, redelivered, type, available, timestamp, payload
                     FROM 
                        message 
                     WHERE queue = :queue AND state = 2 AND properties = :properties AND available < :available ORDER BY timestamp ASC LIMIT 1; )");


               result.state.xid =  connection.precompile( "UPDATE message SET gtrid = :gtrid, state = 3 WHERE ROWID = :id");
               result.state.nullxid = connection.precompile( "DELETE FROM message WHERE ROWID = :id");


               result.commit1 = connection.precompile( "UPDATE message SET state = 2 WHERE gtrid = :gtrid AND state = 1;");
               result.commit2 = connection.precompile( "DELETE FROM message WHERE gtrid = :gtrid AND state = 3;");
               result.commit3 = connection.precompile( "SELECT DISTINCT( queue) FROM message WHERE gtrid = :gtrid AND state = 2;");


               /*
               result.rollback = connection.precompile( R"(
                 -- delete all enqueued  
                 DELETE FROM message WHERE gtrid = :gtrid AND state = 1;
                 -- update all dequeued back to enqueued
                 UPDATE message SET state = 2, redelivered = redelivered + 1  WHERE gtrid = :gtrid AND state = 3;
                 -- move to error queue
                 UPDATE message SET redelivered = 0, queue = ( SELECT error FROM queue WHERE rowid = message.queue)
                     WHERE message.redelivered > ( SELECT retries FROM queue WHERE rowid = message.queue);
                     )");

               */

               result.rollback1 = connection.precompile( "DELETE FROM message WHERE gtrid = :gtrid AND state = 1;");

               // this only mutates if availiable has passed, otherwise state would not be 'dequeued' (3)
               result.rollback2 = connection.precompile( 
                  "UPDATE message SET state = 2, redelivered = redelivered + 1,"
                  " available = ( SELECT CASE WHEN retry_delay = 0 THEN 0"  // no retry delay - we set 0
                     // julianday('now') - 2440587.5) *86400.0 <- some magic that sqlite recommend for fraction of seconds
                     // we multiply by 1'000'000 to get microseconds, and we add retry_delay which is in us already.
                     " ELSE ( ( julianday('now') - 2440587.5) *86400.0 * 1000 * 1000) + retry_delay END" 
                     " FROM queue WHERE id = message.queue)"
                  " WHERE gtrid = :gtrid AND state = 3");

               //! increment redelivered and "move" messages to error-queue iff we've passed retry_count and the queue has an error-queue.
               //! error-queues themselfs does not have an error-queue, hence the message will stay in the error-queue until it's consumed.
               result.rollback3 = connection.precompile(
                     "UPDATE message SET redelivered = 0, queue = ( SELECT q.error FROM queue q WHERE q.id = message.queue)"
                     " WHERE message.redelivered > ( SELECT q.retry_count FROM queue q WHERE q.id = message.queue) "
                     " AND ( SELECT q.error FROM queue q WHERE q.id = message.queue) != 0;"
                  );


               result.available.queues = connection.precompile(
                  "SELECT m.queue, q.count, MIN( m.available)"
                  " FROM message m INNER JOIN queue q ON m.queue = q.id AND m.state = 2"
                  " GROUP BY m.queue;"
               );

               result.available.message = connection.precompile(
                  "SELECT MIN( m.available)"
                  " FROM message m WHERE m.queue = :queue AND m.state = 2;"
               );

               result.id = connection.precompile( "SELECT id FROM queue where name = :name;");


               /*
                  id           INTEGER  PRIMARY KEY,
                  name         TEXT     UNIQUE,
                  retry_count  INTEGER  NOT NULL,
                  retry_delay  INTEGER  NOT NULL,
                  error        INTEGER  NOT NULL,
                  count        INTEGER  NOT NULL, -- number of (committed) messages
                  size         INTEGER  NOT NULL, -- total size of all (committed) messages
                  uncommitted_count INTEGER  NOT NULL, -- uncommitted messages
                  timestamp    INTEGER NOT NULL -- last update to the queue 
               */
               result.information.queue = connection.precompile( R"(
                  SELECT
                     q.id, q.name, q.retry_count, q.retry_delay, q.error, q.count, q.size, q.uncommitted_count, q.timestamp
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
                  available      INTEGER,
                  timestamp     INTEGER,
                  payload       BLOB,
                */
               result.information.message = connection.precompile( R"(
                  SELECT
                     m.id, m.queue, m.origin, m.gtrid, m.properties, m.state, m.reply, m.redelivered, m.type, m.available, m.timestamp, length( m.payload)
                  FROM
                     message m
                  WHERE
                     m.queue = ?
                )");


               result.peek.match = connection.precompile( R"( 
                  SELECT 
                     m.id, m.queue, m.origin, m.gtrid, m.properties, m.state, m.reply, m.redelivered, m.type, m.available, m.timestamp, length( m.payload)
                  FROM 
                     message  m
                  WHERE m.queue = :queue AND m.properties = :properties;)");


               result.peek.one_message = connection.precompile( R"( 
                  SELECT 
                    ROWID, id, properties, reply, redelivered, type, available, timestamp, payload
                  FROM 
                     message 
                  WHERE id = :id; )");

               result.restore = connection.precompile(R"( 
                  UPDATE message 
                  SET queue = :queue 
                  WHERE state = 2 AND queue != origin AND origin = :queue; )");

               result.clear = connection.precompile(R"( 
                  DELETE FROM message
                  WHERE state = 2 AND queue = :queue; )");

               result.message.remove = connection.precompile(R"( 
                  DELETE FROM message
                  WHERE state = 2 AND queue = :queue AND id = :id; )");

               
               return result;
            }

         } // database
      } //group
   } // queue
} // casual