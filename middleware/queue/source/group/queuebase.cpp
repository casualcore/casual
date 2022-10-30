//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/queuebase.h"
#include "queue/group/queuebase/schema.h"
#include "queue/group/queuebase/statement.h"
#include "queue/common/log.h"

#include "common/algorithm.h"
#include "common/algorithm/container.h"
#include "common/execute.h"
#include "common/event/send.h"
#include "common/environment.h"
#include "common/view/binary.h"


#include "common/code/raise.h"
#include "common/code/casual.h"


#include <chrono>

namespace casual
{
   using namespace common;

   namespace queue::group
   {
      namespace local
      {
         namespace
         {

            namespace transform
            {

               void row( sql::database::Row& row, queue::ipc::message::group::message::Meta& message)
               {
                  sql::database::row::get( row,
                     message.id.get(),
                     message.queue.underlying(),
                     message.origin.underlying(),
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
                     ipc::message::group::dequeue::Message message;

                     explicit operator bool () const { return rowid != 0;}
                  };

                  // SELECT ROWID, id, properties, reply, redelivered, type, available, timestamp, payload
                  void result( sql::database::Row& row, Result& result)
                  {
                     sql::database::row::get( row,
                        result.rowid,
                        result.message.id.get(),
                        result.message.attributes.properties,
                        result.message.attributes.reply,
                        result.message.redelivered,
                        result.message.payload.type,
                        result.message.attributes.available,
                        result.message.timestamp,
                        result.message.payload.data
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
                  queuebase::Queue operator () ( sql::database::Row& row) const
                  {
                     queuebase::Queue result;

                     sql::database::row::get( row, 
                        result.id.underlying(),
                        result.name,
                        result.retry.count,
                        result.retry.delay,
                        result.error.underlying()
                     );

                     return result;
                  }
               };

            } // transform

/*
            namespace explain
            {
               auto log( const std::string& name)
               {
                  if( environment::variable::exists( "CASUAL_QUEUE_PERFORMANCE_DIRECTORY"))
                     return std::ofstream{ string::compose( environment::variable::get( "CASUAL_QUEUE_PERFORMANCE_DIRECTORY"), "/queue.", name , ".explain.", process::id())};

                  return std::ofstream{};
               }

            } // explain
            */


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

                  common::event::error::fatal::raise( code::casual::invalid_version,
                     "please remove or convert '", connection.file(), "' using casual-queue-upgrade");
               }

               sql::database::version::set( connection, required);
            }
         } // <unnamed>
      } // local

      namespace queuebase
      {
         namespace queue
         {
            std::ostream& operator << ( std::ostream& out, Type value)
            {
               switch( value)
               {
                  case Type::error_queue: return out << "error_queue";
                  case Type::queue: return out << "queue";
               }
               return out << "<unknown>";
            }
         } // queue

         namespace message
         {
            std::ostream& operator << ( std::ostream& out, State value)
            {
               switch( value)
               {
                  case State::added: return out << "added";
                  case State::enqueued: return out << "enqueued";
                  case State::removed: return out << "removed";
                  case State::dequeued: return out << "dequeued";
               }
               return out << "<unknown>";
            }
         } // message
      } // queuebase


      Queuebase::Queuebase( std::filesystem::path database) 
         : m_connection{ std::move( database)}
      {
         Trace trace{ "queue::group::Queuebase::Queuebase"};

         local::check_version( m_connection);

         // Make sure we got FK
         m_connection.statement( "PRAGMA foreign_keys = ON;");

         // Make sure we set WAL-mode.
         m_connection.statement( "PRAGMA journal_mode=WAL;");

         log::line( verbose::log, "pre_statements_path: ", m_connection.pre_statements_path());
         m_connection.pre_statements( log::category::information);


         {
            Trace trace{ "queue::group::Queuebase::Queuebase create table queue"};
         
            m_connection.statement( queuebase::schema::table::queue);
            m_connection.statement( queuebase::schema::table::index::queue);
         }

         {
            Trace trace{ "queue::group::Queuebase::Queuebase create table message"};

            m_connection.statement( queuebase::schema::table::message);
            m_connection.statement( queuebase::schema::table::index::message);
         }

         // Triggers
         {
            Trace trace{ "queue::group::Queuebase::Queuebase create triggers"};
         
            m_connection.statement( queuebase::schema::triggers); 
         }

         // Precompile all other statements
         m_statement = queuebase::statement( m_connection);

         // start transaction
         m_connection.exclusive_begin();
      }

      Queuebase::~Queuebase()
      {
         Trace trace{ "queue::group::Queuebase::~Queuebase"};

         if( ! m_connection)
            return;

         common::exception::guard( [ this]()
         {
            m_connection.commit();
            m_connection.post_statements( log::category::information);
         });
      }

      const std::filesystem::path& Queuebase::file() const
      {
         return m_connection.file();
      }

      std::optional< queuebase::Queue> Queuebase::queue( common::strong::queue::id id)
      {
         Trace trace{ "queue::Queuebase::queue"};
         log::line( verbose::log, "id: ", id);

         auto query = m_connection.query(
            "SELECT q.id, q.name, q.retry_count, q.retry_delay, q.error FROM queue q WHERE q.id = :id", 
            id.underlying());

         return sql::database::query::first( std::move( query), local::transform::Queue{});
      }

      queuebase::Queue Queuebase::create( queuebase::Queue queue)
      {
         Trace trace{ "queue::Queuebase::create"};

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

      void Queuebase::update( const queuebase::Queue& queue)
      {
         Trace trace{ "queue::Queuebase::updateQueue"};

         auto existing = Queuebase::queue( queue.id);

         if( existing)
         {
            constexpr auto statement = "UPDATE queue SET name = :name, retry_count = :retry_count, retry_delay = :retry_delay WHERE id = :id;";
            m_connection.execute( statement, queue.name, queue.retry.count, queue.retry.delay, queue.id.value());
            m_connection.execute( statement, queue.name + ".error", 0, 0, existing.value().error.value());
         }
      }
      void Queuebase::remove( common::strong::queue::id id)
      {
         Trace trace{ "queue::Queuebase::removeQueue"};

         auto existing = Queuebase::queue( id);

         if( existing)
         {
            m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.value().error.value());
            m_connection.execute( "DELETE FROM queue WHERE id = :id;", existing.value().id.value());
         }
      }


      std::vector< queuebase::Queue> Queuebase::update( std::vector< queuebase::Queue> update, const std::vector< common::strong::queue::id>& remove)
      {
         Trace trace{ "queue::Queuebase::update"};

         common::log::line( verbose::log, "update: ", update, " - remove: ", remove);

         // remove
         common::algorithm::for_each( remove, [&]( auto id){ this->remove( id);});

         auto split = algorithm::partition( update, []( auto& queue){ return queue.id.valid();});

         // update 
         common::algorithm::for_each( std::get< 0>( split), [&]( auto& queue){ this->update( queue);});
      
         // create
         auto result = algorithm::transform( std::get< 1>( split), [&]( auto& queue){ return this->create( queue);});

         return result;
      }

      common::strong::queue::id Queuebase::id( std::string_view name) const
      {
         Trace trace{ "queue::Queuebase::id"};

         auto result = sql::database::query::first( m_statement.id.query( name), []( auto& row)
         {
            common::strong::queue::id id;
            sql::database::row::get( row, id.underlying());
            return id;
         });

         if( ! result)
            code::raise::error( code::casual::invalid_argument, "requested queue is not hosted by this queue-group - name: ", name);

         return std::move( result.value());
      }

      queue::ipc::message::group::enqueue::Reply Queuebase::enqueue( const queue::ipc::message::group::enqueue::Request& message)
      {
         Trace trace{ "queue::Queuebase::enqueue"};

         auto reply = common::message::reverse::type( message);

         // We create a unique id if none is provided.
         reply.id = message.message.id ? message.message.id : common::uuid::make();

         auto gtrid = common::transaction::id::range::global( message.trid);

         auto state = message.trid ? queuebase::message::State::added : queuebase::message::State::enqueued;

         m_statement.enqueue.execute(
               reply.id.get(),
               message.queue.value(),
               message.queue.value(),
               gtrid,
               message.message.attributes.properties,
               state,
               message.message.attributes.reply,
               0,
               message.message.payload.type,
               message.message.attributes.available,
               platform::time::clock::type::now(),
               message.message.payload.data);

         common::log::line( verbose::log, "reply: ", reply);
         return reply;
      }

      queue::ipc::message::group::dequeue::Reply Queuebase::dequeue( 
         const queue::ipc::message::group::dequeue::Request& message, 
         const platform::time::point::type& now)
      {
         Trace trace{ "queue::Queuebase::dequeue"};

         auto fetch = [&]( auto& message, auto& now)
         {
            auto query = [&]()
            {
               if( message.selector.id)
                  return m_statement.dequeue.first_id.query( message.selector.id.get(), message.queue.value(), now);
               if( ! message.selector.properties.empty())
                  return m_statement.dequeue.first_match.query( message.queue.value(), message.selector.properties, now);
               
               log::line( verbose::log, "fifo dequeue query");
               return m_statement.dequeue.first.query( message.queue.value(), now);
            }();

            sql::database::Row row;

            if( query.fetch( row))
               return local::transform::dequeue::result( row);

            // default where result is "absent"
            return decltype( local::transform::dequeue::result( row)){};
         };


         if( auto result = fetch( message, now))
         {
            // Update state
            if( message.trid)
               m_statement.state.xid.execute( common::transaction::id::range::global( message.trid), result.rowid);
            else
               m_statement.state.nullxid.execute( result.rowid);

            auto reply = common::message::reverse::type( message);
            reply.message.push_back( std::move( result.message));

            common::log::line( verbose::log, "reply: ", reply);

            return reply;
         }
         else
         {
               common::log::line( log, "dequeue - qid: ", message.queue, " - no message");
            return common::message::reverse::type( message);
         }

      }

      queue::ipc::message::group::message::meta::peek::Reply Queuebase::peek( const queue::ipc::message::group::message::meta::peek::Request& request)
      {
         Trace trace{ "queue::Queuebase::peek information"};

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
            queue::ipc::message::group::message::Meta message;
            local::transform::row(row, message);
            return message;
         });

         return reply;
      }

      queue::ipc::message::group::message::peek::Reply Queuebase::peek( const queue::ipc::message::group::message::peek::Request& request)
      {
         Trace trace{ "queue::Queuebase::peek messages"};

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

      std::vector< queuebase::message::Available> Queuebase::available( std::vector< common::strong::queue::id> queues) const
      {
         Trace trace{ "queue::Queuebase::available"};
         log::line( verbose::log, "queues: ", queues);

         // TODO performance: use `queues`-range in the select to minimize resultset - might be hard on sqlite

         auto result = sql::database::query::fetch( m_statement.available.queues.query(), []( auto& row)
         {
            queuebase::message::Available message;

            // SELECT q.id, q.count
            sql::database::row::get( row, 
               message.queue.underlying(), 
               message.count
            );

            return message;
         });

         log::line( verbose::log, "result: ", result);

         // keep only the intersection between result and wanted queues
         algorithm::container::trim( result, std::get< 0>( algorithm::intersection( result, queues)));

         // get the earliest available message for each queue.
         for( auto& available : result)
            available.when = Queuebase::available( available.queue).value_or( available.when);
            
         return result;
      }

      std::optional< platform::time::point::type> Queuebase::available( common::strong::queue::id queue) const
      {
         Trace trace{ "queue::Queuebase::available earliest"};
         log::line( verbose::log, "queue: ", queue);

         auto query = m_statement.available.message.query( queue.underlying());
         sql::database::Row row;

         if( ! query.fetch( row))
            return {};

         std::optional< platform::time::point::type> available;

         // "SELECT MIN( m.available)"
         sql::database::row::get( row, 
            available
         );

         return available;
      }

      platform::size::type Queuebase::restore( common::strong::queue::id queue)
      {
         Trace trace{ "queue::Queuebase::restore"};
         log::line( verbose::log, "queue: ", queue);

         m_statement.restore.execute( queue.value());
         return m_connection.affected();
      }

      platform::size::type Queuebase::clear( common::strong::queue::id queue)
      {
         Trace trace{ "queue::Queuebase::clear"};
         log::line( verbose::log, "queue: ", queue);

         m_statement.clear.execute( queue.value());
         return m_connection.affected();
      }

      std::vector< common::Uuid> Queuebase::remove( common::strong::queue::id queue, std::vector< common::Uuid> messages)
      {
         auto missing_message = [&]( auto& id)
         {
            m_statement.message.remove.execute( queue.value(), id.get());
            return m_connection.affected() == 0;
         };

         algorithm::container::trim( messages, algorithm::remove_if( messages, missing_message));
         return messages;
      }

      std::vector< common::transaction::global::ID> Queuebase::recovery_commit( common::strong::queue::id queue, std::vector< common::transaction::global::ID> gtrids)
      {
         auto missing_message = [&]( auto& id)
         {
            m_statement.commit1.execute( id());
            const auto commit1_affected = m_connection.affected();
            m_statement.commit2.execute( id());
            const auto commit2_affected = m_connection.affected();
 
            return commit1_affected == 0 && commit2_affected == 0;
         };

         algorithm::container::trim( gtrids, algorithm::remove_if( gtrids, missing_message));

         return gtrids;
      }

      std::vector< common::transaction::global::ID> Queuebase::recovery_rollback( common::strong::queue::id queue, std::vector< common::transaction::global::ID> gtrids)
      {
         auto missing_message = [&]( auto& id)
         {
            m_statement.rollback1.execute( id());
            const auto rollback1_affected = m_connection.affected();
            m_statement.rollback2.execute( id());
            const auto rollback2_affected = m_connection.affected();
            m_statement.rollback3.execute( id());

            return rollback1_affected == 0 && rollback2_affected == 0;
         };

         algorithm::container::trim( gtrids, algorithm::remove_if( gtrids, missing_message));

         return gtrids;
      }

      void Queuebase::commit( const common::transaction::ID& id)
      {
         Trace trace{ "queue::Queuebase::commit"};

         log::line( log, "commit xid: ", id);

         auto gtrid = common::transaction::id::range::global( id);
         m_statement.commit1.execute( gtrid);
         m_statement.commit2.execute( gtrid);
      }

      void Queuebase::rollback( const common::transaction::ID& id)
      {
         Trace trace{ "queue::Queuebase::rollback"};

         log::line( log, "rollback xid: ", id);

         auto gtrid = common::transaction::id::range::global( id);
         m_statement.rollback1.execute( gtrid);
         m_statement.rollback2.execute( gtrid);
         m_statement.rollback3.execute( gtrid);
      }

      std::vector< queue::ipc::message::group::state::Queue> Queuebase::queues()
      {
         Trace trace{ "queue::Queuebase::queues"};

         return sql::database::query::fetch( m_statement.information.queue.query(), []( auto& row)
         {
            queue::ipc::message::group::state::Queue queue;

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
               queue.id.underlying(),
               queue.name,
               queue.retry.count,
               queue.retry.delay,
               queue.error.underlying(),
               queue.metric.count,
               queue.metric.size,
               queue.metric.uncommitted,
               queue.metric.dequeued,
               queue.metric.enqueued,
               queue.metric.last,
               queue.created 
            );
            return queue;
         });
      }

      std::vector< queue::ipc::message::group::message::Meta> Queuebase::meta( common::strong::queue::id id)
      {
         Trace trace{ "queue::Queuebase::meta"};

         return sql::database::query::fetch( m_statement.information.message.query( id.value()), []( auto& row)
         {
            queue::ipc::message::group::message::Meta meta;
            local::transform::row(row, meta);
            return meta;
         });
      }

      void Queuebase::metric_reset( const std::vector< common::strong::queue::id>& ids)
      {
         Trace trace{ "queue::Queuebase::metric_reset"};

         auto reset = [&]( auto id)
         {
            m_statement.metric.reset.execute( id.value());
         };

         algorithm::for_each( ids, reset);
      }


      platform::size::type Queuebase::affected() const
      {
         return m_connection.affected();
      }


      void Queuebase::persist() 
      { 
         Trace trace{ "queue::Queuebase::persist"};
         log::line( log, "commit");
         m_connection.commit();
         log::line( log, "exclusive_begin");
         m_connection.exclusive_begin();
      }
      void Queuebase::rollback() 
      { 
         m_connection.rollback();
         m_connection.exclusive_begin();
      }


      sql::database::Version Queuebase::version()
      {
         return sql::database::version::get( m_connection);
      }


// just a helper macro to make the performance logging easier.
#define CASUAL_QUEUE_EXPLAIN_STATEMENT( value) \
log_statement( value, #value)

      void Queuebase::explain( const std::filesystem::path& path)
      {
         std::ofstream stream{ path};
         
         if( ! stream)
         {
            log::line( log::category::error, code::casual::invalid_path, " quebase metric path: ", path);
            return;
         }

         auto log_statement = [&]( const sql::database::Statement& statement, const char* name)
         {
            auto origin = statement.sql();
            auto sql = string::compose( "EXPLAIN QUERY PLAN ", origin);

            common::log::line( stream, name, ":\nsql: ", origin);
            m_connection.statement( sql, stream);
            stream << '\n';
         };

         common::log::trace::Scope trace{ "queue::group::database::local::explain::statement", stream};

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.enqueue);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.dequeue.first);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.dequeue.first_id);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.dequeue.first_match);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.state.xid);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.state.nullxid);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.commit1);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.commit2);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.commit3);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.rollback1);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.rollback2);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.rollback3);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.id);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.information.queue);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.information.message);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.peek.match);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.peek.one_message);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.restore);
         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.clear);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.message.remove);

         CASUAL_QUEUE_EXPLAIN_STATEMENT( m_statement.metric.reset);
      }
   } // queue::group
} // casual
