//!
//! log.cpp
//!
//! Created on: Nov 3, 2013
//!     Author: Lazan
//!

#include "transaction/manager/log.h"

#include "common/algorithm.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"

#include <chrono>


#include <cassert>

namespace casual
{
   namespace transaction
   {

      namespace local
      {
         namespace
         {
            sql::database::Blob global( const common::transaction::ID& id)
            {
               return { id.xid().gtrid_length, id.xid().data};
            }

            sql::database::Blob branch( const common::transaction::ID& id)
            {
               return { id.xid().bqual_length, id.xid().data + id.xid().gtrid_length};
            }

            void createTables( sql::database::Connection& db)
            {
               db.execute(
                     R"( CREATE TABLE IF NOT EXISTS trans (
                     gtrid         BLOB,
                     bqual         BLOB,
                     format        NUMBER,
                     pid           NUMBER,
                     state         NUMBER,
                     started       NUMBER,
                     updated       NUMBER,
                     PRIMARY KEY (gtrid, bqual)); )");
            }

            void update( sql::database::Connection& connection, const common::transaction::ID& id, long state)
            {
               auto updated = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();

               auto gtrid = local::global( id);
               auto bqual = local::branch( id);

               const std::string sql{ R"( UPDATE trans SET state = ?, updated = ? WHERE gtrid = ? AND bqual = ?; )" };

               connection.execute( sql, state, updated, gtrid, bqual);
            }

            void remove( sql::database::Connection& connection, const common::transaction::ID& xid)
            {
               auto gtrid = local::global( xid);
               auto bqual = local::branch( xid);

               const std::string sql{ R"( DELETE FROM trans WHERE gtrid = ? AND bqual = ?; )" };

               connection.execute( sql, gtrid, bqual);
            }

            template< typename M>
            void insertBegin( sql::database::Connection& connection, M&& message)
            {
               auto started = std::chrono::time_point_cast< std::chrono::microseconds>( message.start).time_since_epoch().count();

               auto updated = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();

               const std::string sql{ R"( INSERT INTO trans VALUES (?,?,?,?,?,?,?); )" };

               auto gtrid = local::global( message.xid);
               auto bqual = local::branch( message.xid);

               long state = Log::State::cBegin;
               connection.execute( sql, gtrid, bqual, message.xid.xid().formatID, message.id.pid, state, started, updated);

            }

            sql::database::Query select( sql::database::Connection& connection, const common::transaction::ID& id)
            {
               auto gtrid = local::global( id);
               auto bqual = local::branch( id);


               const std::string sql{ R"( SELECT gtrid, bqual, format, pid, state, started, updated FROM trans WHERE gtrid = ? AND bqual = ?; )" };

               return connection.query( sql, gtrid, bqual);
            }

            sql::database::Query select( sql::database::Connection& connection)
            {
               std::vector< sql::database::Row> result;

               const std::string sql{ R"( SELECT * FROM trans; )" };

               return connection.query( sql);


            }

            namespace transform
            {
               struct Row
               {
                  Log::Row operator () ( sql::database::Row& row) const
                  {
                     Log::Row result;

                     {
                        auto gtrid = row.get< common::platform::binary_type>( 0);

                        common::range::copy( common::range::make( gtrid), std::begin( result.xid.xid().data));

                        result.xid.xid().gtrid_length = gtrid.size();
                     }

                     {
                        auto bqual = row.get< common::platform::binary_type>( 1);

                        common::range::copy(
                              common::range::make( bqual),
                              std::begin( result.xid.xid().data) + result.xid.xid().gtrid_length);

                        result.xid.xid().bqual_length = bqual.size();
                     }

                     result.xid.xid().formatID = row.get< long>( 2);

                     result.pid = row.get< common::platform::pid_type>( 3);
                     result.state = static_cast< Log::State>( row.get< long>( 4));

                     result.started = common::platform::time_type{ std::chrono::microseconds{ row.get< common::platform::time_type::rep>( 5)}};
                     result.updated = common::platform::time_type{ std::chrono::microseconds{ row.get< common::platform::time_type::rep>( 6)}};

                     return result;
                  }
               };
            } // transform
         }
      }

      Log::Log( const std::string& database)
            : m_connection( database)
      {
         local::createTables( m_connection);

      }

      void Log::begin( const common::message::transaction::begin::Request& request)
      {
         common::log::internal::transaction << "log begin for xid: " << request.xid << "\n";

         local::insertBegin( m_connection, request);

      }

      void Log::commit( const common::message::transaction::commit::Request& request)
      {

      }

      void Log::rollback( const common::message::transaction::rollback::Request& request)
      {

      }

      void Log::prepareCommit( const common::transaction::ID& id)
      {
         local::update( m_connection, id, State::cPreparedCommit);
      }

      std::vector< Log::Row> Log::select( const common::transaction::ID& id)
      {
         std::vector< Row> result;

         auto query = local::select( m_connection, id);
         auto rows = query.fetch();

         std::transform( std::begin( rows), std::end( rows), std::back_inserter( result), local::transform::Row());


         return result;
      }

      void Log::remove( const common::transaction::ID& xid)
      {
         local::remove( m_connection, xid);
      }

      std::vector< Log::Row> Log::select()
      {
         std::vector< Row> result;

         auto query = local::select( m_connection);

         for( auto rows = query.fetch(); ! rows.empty(); rows = query.fetch())
         {
            std::transform( std::begin( rows), std::end( rows), std::back_inserter( result), local::transform::Row());
         }

         return result;
      }

      void Log::writeBegin()
      {
         m_connection.begin();
      }

      void Log::writeCommit()
      {
         common::trace::internal::Scope trace{ "transaction log write persistence"};
         m_connection.commit();
      }



      void Log::writeRollback()
      {
         m_connection.rollback();
      }

   } // transaction
} // casual
