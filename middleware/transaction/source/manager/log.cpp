//!
//! casual
//!

#include "transaction/manager/log.h"
#include "transaction/manager/state.h"
#include "transaction/common.h"

#include "common/algorithm.h"

#include <chrono>


#include <cassert>

namespace casual
{
   namespace transaction
   {

      Log::Log( std::string database)
            : m_connection( std::move( database))
      {
         //m_connection.execute( "PRAGMA journal_mode = WAL;");

         common::log::line( log, "transaction log version: ",  sql::database::version::get( m_connection));

         sql::database::version::set( m_connection, sql::database::Version{ 1, 0});

         m_connection.execute(
            R"( CREATE TABLE IF NOT EXISTS trans (
            gtrid         BLOB NOT NULL,
            bqual         BLOB NOT NULL,
            format        NUMBER NOT NULL,
            pid           NUMBER NOT NULL,
            state         NUMBER NOT NULL,
            started       NUMBER NOT NULL,
            deadline      NUMBER,
            PRIMARY KEY (gtrid, bqual)); )");

         m_connection.execute(
            "CREATE INDEX IF NOT EXISTS i_xid_trans ON trans ( gtrid, bqual);" );


         m_statement.insert = m_connection.precompile( R"( INSERT INTO trans VALUES (?,?,?,?,?,?,?); )" );
         m_statement.remove = m_connection.precompile( "DELETE FROM trans WHERE gtrid = ? AND bqual = ?; ");
   
      }


      void Log::prepare( const transaction::Transaction& transaction)
      {
         Trace trace{ "transaction::Log::prepare"};

         m_statement.insert.execute(
            common::transaction::global( transaction.trid),
            common::transaction::branch( transaction.trid),
            transaction.trid.xid.formatID,
            transaction.trid.owner().pid.value(),
            State::prepared,
            transaction.started,
            transaction.deadline
         );
         ++m_stats.update.prepare;

         common::log::line( verbose::log, "total prepares: ", m_stats.update.prepare);
      }

      void Log::remove( const common::transaction::ID& trid)
      {
         Trace trace{ "transaction::Log::remove"};

         m_statement.remove.execute(
            common::transaction::global( trid),
            common::transaction::branch( trid));

         ++m_stats.update.remove;

         common::log::line( verbose::log, "total removes: ", m_stats.update.remove);
      }


      void Log::write_begin()
      {
         Trace trace{ "transaction::Log::write_begin"};

         m_connection.begin();
      }

      void Log::write_commit()
      {
         Trace trace{ "transaction::Log::write_commit"};

         m_connection.commit();
         ++m_stats.writes;

         common::log::line( verbose::log, "total commits: ", m_stats.writes);
      }


      void Log::write_rollback()
      {
         m_connection.rollback();
      }

      const Log::Stats& Log::stats() const
      {
         return m_stats;
      }

      namespace local
      {
         namespace
         {
            namespace transform
            {
               common::transaction::ID trid( sql::database::Row& row, int index = 0)
               {
                  common::transaction::ID result;

                  {
                     auto gtrid = row.get< common::platform::binary::type>( index);

                     common::algorithm::copy( common::range::make( gtrid), std::begin( result.xid.data));

                     result.xid.gtrid_length = gtrid.size();
                  }

                  {
                     auto bqual = row.get< common::platform::binary::type>( index + 1);

                     common::algorithm::copy(
                           common::range::make( bqual),
                           std::begin( result.xid.data) + result.xid.gtrid_length);

                     result.xid.bqual_length = bqual.size();
                  }

                  result.xid.formatID = row.get< long>( index + 2);

                  return result;
               }

               Log::Row row( sql::database::Row& row)
               {
                  Log::Row result;

                  result.trid = transform::trid( row);

                  result.pid = common::strong::process::id{ row.get< common::platform::process::native::type>( 3)};
                  result.state = static_cast< Log::State>( row.get< long>( 4));

                  result.started = common::platform::time::point::type{ std::chrono::microseconds{ row.get< common::platform::time::point::type::rep>( 5)}};
                  result.updated = common::platform::time::point::type{ std::chrono::microseconds{ row.get< common::platform::time::point::type::rep>( 6)}};

                  return result;
               }
            } // transform

         } // <unnamed>
      } // local

      std::vector< Log::Row> Log::logged()
      {
         std::vector< Row> result;

         auto query = m_connection.query( R"( SELECT * FROM trans; )");

         sql::database::Row row;

         while( query.fetch( row))
         {
            result.push_back( local::transform::row( row));
         }
         return result;
      }


   } // transaction
} // casual
