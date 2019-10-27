//! 
//! Copyright (c) 2019, The casual project
//
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

namespace casual
{
   namespace queue
   {
      namespace group
      {
         namespace database
         {
            namespace schema
            {
               namespace table
               {
                  inline namespace v2_0
                  {
                     constexpr auto queue = R"( CREATE TABLE IF NOT EXISTS queue 
(
   id           INTEGER  PRIMARY KEY,
   name         TEXT     UNIQUE,
   retry_count  INTEGER  NOT NULL,
   retry_delay  INTEGER  NOT NULL,
   error        INTEGER  NOT NULL,
   count        INTEGER  NOT NULL, -- number of (committed) messages
   size         INTEGER  NOT NULL, -- total size of all (committed) messages
   uncommitted_count INTEGER  NOT NULL, -- uncommitted messages
   timestamp    INTEGER NOT NULL -- last update to the queue 
); )";


                     constexpr auto message = R"( CREATE TABLE IF NOT EXISTS message 
(  id            BLOB PRIMARY KEY,
   queue         INTEGER  NOT NULL,
   origin        NUMBER   NOT NULL, -- the first queue a message is enqueued to
   gtrid         BLOB,
   properties    TEXT,
   state         INTEGER  NOT NULL, -- (1: enqueued, 2: committed, 3: dequeued)
   reply         TEXT,
   redelivered   INTEGER  NOT NULL,
   type          TEXT,
   available     INTEGER  NOT NULL,
   timestamp     INTEGER  NOT NULL,
   payload       BLOB,
   FOREIGN KEY (queue) REFERENCES queue( id)
); )";
                     namespace index
                     {
                        constexpr auto queue = R"( 
CREATE INDEX IF NOT EXISTS i_id_queue ON queue ( id);
)";

                        constexpr auto message = R"( 
CREATE INDEX IF NOT EXISTS i_message_id  ON message ( id);
CREATE INDEX IF NOT EXISTS i_message_queue  ON message ( queue);
CREATE INDEX IF NOT EXISTS i_dequeue_message  ON message ( queue, state, timestamp ASC);
CREATE INDEX IF NOT EXISTS i_dequeue_message_properties ON message ( queue, state, properties, timestamp ASC);
CREATE INDEX IF NOT EXISTS i_message_timestamp ON message ( timestamp ASC);
CREATE INDEX IF NOT EXISTS i_message_available ON message ( available ASC);
CREATE INDEX IF NOT EXISTS i_gtrid_message  ON message ( gtrid);
)";
                     } // index
                  } // inline v2_0
               } // table

               inline namespace v2_0
               {
                  constexpr auto triggers = R"(
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

CREATE TRIGGER IF NOT EXISTS update_message_state UPDATE OF state ON message 
BEGIN
   UPDATE queue SET
      count = count + 1,
      size = size + length( new.payload),
      uncommitted_count = uncommitted_count - 1
   WHERE id = new.queue AND old.state = 1 AND new.state = 2;
END;

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
               
)";
                  
               } // inline v2_0

            } // schema
         } // database
      } // group
   } // queue
} // casual