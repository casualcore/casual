//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
      namespace manager
      {
         Log::Log( std::filesystem::path file)
            : m_connection( std::move( file))
         {
            
            // Make sure we set WAL-mode.
            m_connection.statement( "PRAGMA journal_mode=WAL;");

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
            m_statement.remove = m_connection.precompile( "DELETE FROM trans WHERE gtrid = ?; ");

            // start a "transaction" for the sql-lite db
            m_connection.begin();
         }

         Log::~Log()
         {
            // persist.
            m_connection.commit();
         } 


         void Log::prepare( const Transaction& transaction)
         {
            Trace trace{ "transaction::Log::prepare"};

            common::log::line( verbose::log, "transaction: ", transaction);

            auto prepare_branch = [&]( auto& branch)
            {
               m_statement.insert.execute(
                  common::transaction::id::range::global( branch.trid),
                  common::transaction::id::range::branch( branch.trid),
                  branch.trid.xid.formatID,
                  branch.trid.owner().pid.value(),
                  State::prepared,
                  transaction.started,
                  transaction.deadline
               );
               ++m_stats.update.prepare;
            };

            common::algorithm::for_each( transaction.branches, prepare_branch);

            common::log::line( verbose::log, "total prepares: ", m_stats.update.prepare);
         }

         void Log::remove( const global::ID& global)
         {
            Trace trace{ "transaction::Log::remove"};

            m_statement.remove.execute( global.global());

            m_stats.update.remove += m_connection.affected();

            common::log::line( verbose::log, "total removes: ", m_stats.update.remove);
         }

         void Log::persist()
         {
            Trace trace{ "transaction::Log::persist"};

            // commit and and start a new "transaction" for the sql-lite db
            
            m_connection.commit();
            ++m_stats.writes;
            m_connection.begin();

            common::log::line( verbose::log, "total commits: ", m_stats.writes);
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
                        auto gtrid = row.get< platform::binary::type>( index);

                        common::algorithm::copy( common::range::make( gtrid), std::begin( result.xid.data));

                        result.xid.gtrid_length = gtrid.size();
                     }

                     {
                        auto bqual = row.get< platform::binary::type>( index + 1);

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

                     result.pid = common::strong::process::id{ row.get< platform::process::native::type>( 3)};
                     result.state = static_cast< Log::State>( row.get< long>( 4));

                     result.started = platform::time::point::type{ std::chrono::microseconds{ row.get< platform::time::point::type::rep>( 5)}};
                     result.updated = platform::time::point::type{ std::chrono::microseconds{ row.get< platform::time::point::type::rep>( 6)}};

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

      } // manager
   } // transaction
} // casual
