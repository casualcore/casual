//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/database.h"
#include "queue/group/database/schema.h"
#include "queue/group/database/statement.h"
#include "queue/common/log.h"

#include "common/algorithm.h"
#include "common/execute.h"
#include "common/event/send.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


#include <chrono>

namespace casual
{
   using namespace common;

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
                     sql::database::row::get( row,
                        message.id.get(),
                        message.queue.underlaying(),
                        message.origin.underlaying(),
                        message.trid,
                        message.properties,
                        message.state,
                        message.reply,
                        message.redelivered,
                        message.type,
                        message.available,
                        message.timestamp,
                        message.size
                     );  
                  }

                  namespace dequeue
                  {
                     struct Result 
                     {
                        std::int64_t rowid{};
                        common::message::queue::dequeue::Reply::Message message;
                     };

                     // SELECT ROWID, id, properties, reply, redelivered, type, available, timestamp, payload
                     void result( sql::database::Row& row, Result& result)
                     {
                        sql::database::row::get( row,
                           result.rowid,
                           result.message.id.get(),
                           result.message.properties,
                           result.message.reply,
                           result.message.redelivered,
                           result.message.type,
                           result.message.available,
                           result.message.timestamp,
                           result.message.payload
                        );
                     }

                     auto result( sql::database::Row& row)
                     {
                        Result result;
                        dequeue::result( row, result);
                        return result;
                     }
                     
                  } // dequeue


                  struct Queue
                  {
                     group::Queue operator () ( sql::database::Row& row) const
                     {
                        group::Queue result;

                        sql::database::row::get( row, 
                           result.id.underlaying(),
                           result.name,
                           result.retry.count,
                           result.retry.delay,
                           result.error.underlaying()
                        );

                        return result;
                     }
                  };

               } // transform


               template< typename C>
               void check_version( C& connection)
               {
                  Trace trace{ "queue::group::database::local::check_version"};

                  auto required = sql::database::Version{ 3, 0};

                  auto version = sql::database::version::get( connection);

                  common::log::line( log, "queue-base version: ", version);

                  if( ( ! version && connection.table( "queue")) || ( version && version != required))
                  {
                     common::log::line( common::log::category::verbose::error, "version: ", version);

                     common::event::error::raise(  code::casual::invalid_version, common::event::error::Severity::fatal,
                        "please remove or convert '", connection.file(), "' using casual-queue-upgrade");
                  }

                  sql::database::version::set( connection, required);
               }
            } // <unnamed>
         } // local



         Database::Database( const std::string& database, std::string groupname) 
            : m_connection( database), m_name( std::move( groupname))
         {
            Trace trace{ "queue::group::Database::Database"};

            local::check_version( m_connection);

            // Make sure we got FK
            m_connection.statement( "PRAGMA foreign_keys = ON;");

            // We can't set WAL mode, for some reason...
            //m_connection.execute( "PRAGMA journal_mode=WAL;");

            {
               Trace trace{ "queue::group::Database::Database create table queue"};
            
               m_connection.statement( database::schema::table::queue);
               m_connection.statement( database::schema::table::index::queue);
            }

            {
               Trace trace{ "queue::group::Database::Database create table message"};

               m_connection.statement( database::schema::table::message);
               m_connection.statement( database::schema::table::index::message);
            }

            // Triggers
            {
               Trace trace{ "queue::group::Database::Database create triggers"};
           
               m_connection.statement( database::schema::triggers); 
            }

            // Precompile all other statements
            m_statement = database::statement( m_connection);
         }


         std::string Database::file() const
         {
            return m_connection.file();
         }

         common::optional< Queue> Database::queue( common::strong::queue::id id)
         {
            Trace trace{ "queue::Database::queue"};
            log::line( verbose::log, "id: ", id);

            auto query = m_connection.query(
               "SELECT q.id, q.name, q.retry_count, q.retry_delay, q.error FROM queue q WHERE q.id = :id", 
               id.underlaying());

            return sql::database::query::first( std::move( query), local::transform::Queue{});
         }

         Queue Database::create( Queue queue)
         {
            Trace trace{ "queue::Database::create"};

            /*
               id                INTEGER  PRIMARY KEY,
               name              TEXT     UNIQUE,
               retry_count       INTEGER  NOT NULL,
               retry_delay       INTEGER  NOT NULL,
               error             INTEGER  NOT NULL,
               count             INTEGER  NOT NULL, -- number of (committed) messages
               size              INTEGER  NOT NULL, -- total size of all (committed) messages
               uncommitted_count INTEGER  NOT NULL, -- uncommitted messages
               metric_dequeued   INTEGER  NOT NULL,
               metric_enqueued   INTEGER  NOT NULL,
               last              INTEGER NOT NULL, -- last update to the queue
               created           INTEGER NOT NULL -- when the queue was created
             */

            auto now = platform::time::clock::type::now();

            constexpr auto statement = "INSERT INTO queue VALUES ( NULL,?,?,?,?, 0, 0, 0, 0, 0, 0, ?);";

            // Create corresponding error queue
            // Note that 'error-queues' has '0' as error, hence a rollbacked message will never be moved to another queue.
            m_connection.execute( statement, queue.name + ".error", 0, 0, 0, now);
            queue.error = common::strong::queue::id{ m_connection.rowid()};

            m_connection.execute( statement, queue.name, queue.retry.count, queue.retry.delay, queue.error.value(), now);
            queue.id = common::strong::queue::id{ m_connection.rowid()};

            common::log::line( log, "queue: ", queue);

            return queue;
         }

         void Database::update( const Queue& queue)
         {
            Trace trace{ "queue::Database::updateQueue"};

            auto existing = Database::queue( queue.id);

            if( existing)
            {
               constexpr auto statement = "UPDATE queue SET name = :name, retry_count = :retry_count, retry_delay = :retry_delay WHERE id = :id;";
               m_connection.execute( statement, queue.name, queue.retry.count, queue.retry.delay, queue.id.value());
               m_connection.execute( statement, queue.name + ".error", 0, 0, existing.value().error.value());
            }
         }
         void Database::remove( common::strong::queue::id id)
         {
            Trace trace{ "queue::Database::removeQueue"};

            auto existing = Database::queue( id);

            if( existing)
            {
               m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.value().error.value());
               m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.value().id.value());
            }
         }


         std::vector< Queue> Database::update( std::vector< Queue> update, const std::vector< common::strong::queue::id>& remove)
         {
            Trace trace{ "queue::Database::update"};

            common::log::line( verbose::log, "update: ", update, " - remove: ", remove);

            // remove
            common::algorithm::for_each( remove, [&]( auto id){ this->remove( id);});

            auto split = algorithm::partition( update, []( auto& queue){ return ! queue.id.empty();});

            // update 
            common::algorithm::for_each( std::get< 0>( split), [&]( auto& queue){ this->update( queue);});
         
            // create
            auto result = algorithm::transform( std::get< 1>( split), [&]( auto& queue){ return this->create( queue);});

            return result;
         }

         common::strong::queue::id Database::id( const std::string& name) const
         {
            Trace trace{ "queue::Database::id"};

            auto result = sql::database::query::first( m_statement.id.query( name), []( auto& row)
            {
               common::strong::queue::id id;
               sql::database::row::get( row, id.underlaying());
               return id;
            });

            if( ! result)
               code::raise::error( code::casual::invalid_argument, "requested queue is not hosted by this queue-group - name: ", name);

            return std::move( result.value());
         }

         common::message::queue::enqueue::Reply Database::enqueue( const common::message::queue::enqueue::Request& message)
         {
            Trace trace{ "queue::Database::enqueue"};

            auto reply = common::message::reverse::type( message);

            // We create a unique id if none is provided.
            reply.id = message.message.id ? message.message.id : common::uuid::make();

            auto gtrid = common::transaction::id::range::global( message.trid);

            auto state = message.trid ? message::State::added : message::State::enqueued;

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
                  platform::time::clock::type::now(),
                  message.message.payload);

            common::log::line( verbose::log, "reply: ", reply);
            return reply;
         }

         common::message::queue::dequeue::Reply Database::dequeue( 
            const common::message::queue::dequeue::Request& message,
            const platform::time::point::type& now)
         {
            Trace trace{ "queue::Database::dequeue"};

            common::message::queue::dequeue::Reply reply;

            auto query = [&]()
            {
               if( message.selector.id)
                  return m_statement.dequeue.first_id.query( message.selector.id.get(), message.queue.value(), now);
               if( ! message.selector.properties.empty())
                  return m_statement.dequeue.first_match.query( message.queue.value(), message.selector.properties, now);
               return m_statement.dequeue.first.query( message.queue.value(), now);
            };

            auto first = sql::database::query::first( query(), []( auto& row)
            {
               return local::transform::dequeue::result( row);
            });

            if( ! first)
            {
               common::log::line( log, "dequeue - qid: ", message.queue, " - no message");
               return reply;
            }

            auto& result = first.value();

            // Update state
            if( message.trid)
               m_statement.state.xid.execute( common::transaction::id::range::global( message.trid), result.rowid);
            else
               m_statement.state.nullxid.execute( result.rowid);

            reply.message.push_back( std::move( result.message));

            common::log::line( verbose::log, "reply: ", reply);

            return reply;
         }

         common::message::queue::peek::information::Reply Database::peek( const common::message::queue::peek::information::Request& request)
         {
            Trace trace{ "queue::Database::peek information"};

            auto reply = common::message::reverse::type( request);

            auto get_query = [&](){
               if( ! request.selector.properties.empty())
               {
                  return m_statement.peek.match.query( request.queue.value(), request.selector.properties);
               }
               return m_statement.information.message.query( request.queue.value());
            };

            reply.messages = sql::database::query::fetch( get_query(), []( sql::database::Row& row)
            {
               common::message::queue::information::Message message;
               local::transform::row(row, message);
               return message;
            });

            return reply;
         }

         common::message::queue::peek::messages::Reply Database::peek( const common::message::queue::peek::messages::Request& request)
         {
            Trace trace{ "queue::Database::peek messages"};

            auto reply = common::message::reverse::type( request);

            sql::database::Row row;

            for( auto& id : request.ids)
            {
               auto query = m_statement.peek.one_message.query( id.get());

               if( query.fetch( row))
               {
                  auto result = local::transform::dequeue::result( row);
                  reply.messages.push_back( std::move( result.message));
               }
               else
                  log::line( log, "failed to find message with id: ", id);
            }

            return reply;
         }

         std::vector< message::Available> Database::available( std::vector< common::strong::queue::id> queues) const
         {
            Trace trace{ "queue::Database::available"};
            log::line( verbose::log, "queues: ", queues);

            // TODO performance: use `queues`-range in the select to minimize resultset - might be hard on sqlite

            auto result = sql::database::query::fetch( m_statement.available.queues.query(), []( auto& row)
            {
               message::Available message;

               // SELECT m.queue, q.count, MIN( m.available)
               sql::database::row::get( row, 
                  message.queue.underlaying(), 
                  message.count,
                  message.when
               );

               return message;
            });

            log::line( verbose::log, "result: ", result);

            // keep only the intersection between result and wanted queues
            algorithm::trim( result, std::get< 0>( algorithm::intersection( result, queues)));

            return result;
         }

         common::optional< platform::time::point::type> Database::available( common::strong::queue::id queue) const
         {
            Trace trace{ "queue::Database::available earliest"};
            log::line( verbose::log, "queue: ", queue);

            return sql::database::query::first( m_statement.available.message.query( queue.underlaying()), []( auto& row)
            {
               platform::time::point::type available;

               // SELECT m.queue, q.count, MIN( m.available)
               sql::database::row::get( row, 
                  available
               );
               return available;
            });
         }

         size_type Database::restore( common::strong::queue::id queue)
         {
            Trace trace{ "queue::Database::restore"};
            log::line( verbose::log, "queue: ", queue);

            m_statement.restore.execute( queue.value());
            return m_connection.affected();
         }

         size_type Database::clear( common::strong::queue::id queue)
         {
            Trace trace{ "queue::Database::clear"};
            log::line( verbose::log, "queue: ", queue);

            m_statement.clear.execute( queue.value());
            return m_connection.affected();
         }

         std::vector< common::Uuid> Database::remove( common::strong::queue::id queue, std::vector< common::Uuid> messages)
         {
            auto missing_message = [&]( auto& id)
            {
               m_statement.message.remove.execute( queue.value(), id.get());
               return m_connection.affected() == 0;
            };

            algorithm::trim( messages, algorithm::remove_if( messages, missing_message));
            return messages;
         }


         void Database::commit( const common::transaction::ID& id)
         {
            Trace trace{ "queue::Database::commit"};

            log::line( log, "commit xid: ", id);

            auto gtrid = common::transaction::id::range::global( id);

            m_statement.commit1.execute( gtrid);
            m_statement.commit2.execute( gtrid);
         }

         void Database::rollback( const common::transaction::ID& id)
         {
            Trace trace{ "queue::Database::rollback"};

            log::line( log, "rollback xid: ", id);

            auto gtrid = common::transaction::id::range::global( id);

            m_statement.rollback1.execute( gtrid);
            m_statement.rollback2.execute( gtrid);
            m_statement.rollback3.execute();
         }

         std::vector< common::message::queue::information::Queue> Database::queues()
         {
            Trace trace{ "queue::Database::queues"};

            return sql::database::query::fetch( m_statement.information.queue.query(), []( auto& row)
            {
               common::message::queue::information::Queue queue;

               /*
                  id                INTEGER  PRIMARY KEY,
                  name              TEXT     UNIQUE,
                  retry_count       INTEGER  NOT NULL,
                  retry_delay       INTEGER  NOT NULL,
                  error             INTEGER  NOT NULL,
                  count             INTEGER  NOT NULL, -- number of (committed) messages
                  size              INTEGER  NOT NULL, -- total size of all (committed) messages
                  uncommitted_count INTEGER  NOT NULL, -- uncommitted messages
                  metric_dequeued   INTEGER  NOT NULL,
                  metric_enqueued   INTEGER  NOT NULL,
                  last              INTEGER NOT NULL, -- last update to the queue
                  created           INTEGER NOT NULL -- when the queue was created
                */
               sql::database::row::get( row, 
                  queue.id.underlaying(),
                  queue.name,
                  queue.retry.count,
                  queue.retry.delay,
                  queue.error.underlaying(),
                  queue.count,
                  queue.size,
                  queue.uncommitted,
                  queue.metric.dequeued,
                  queue.metric.enqueued,
                  queue.last,
                  queue.created 
               );
               return queue;
            });
         }

         std::vector< common::message::queue::information::Message> Database::messages( common::strong::queue::id id)
         {
            Trace trace{ "queue::Database::messages"};

            return sql::database::query::fetch( m_statement.information.message.query( id.value()), []( auto& row)
            {
               common::message::queue::information::Message message;
               local::transform::row(row, message);
               return message;
            });
         }

         void Database::metric_reset( const std::vector< common::strong::queue::id>& ids)
         {
            Trace trace{ "queue::Database::metric_reset"};

            auto reset = [&]( auto id)
            {
               m_statement.metric.reset.execute( id.value());
            };

            algorithm::for_each( ids, reset);
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
