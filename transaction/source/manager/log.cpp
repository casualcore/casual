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
            namespace transform
            {
               struct Row
               {
                  Log::Row operator () ( sql::database::Row& row) const
                  {
                     Log::Row result;

                     {
                        auto gtrid = row.get< common::platform::binary_type>( 0);

                        common::range::copy( common::range::make( gtrid), std::begin( result.trid.xid.data));

                        result.trid.xid.gtrid_length = gtrid.size();
                     }

                     {
                        auto bqual = row.get< common::platform::binary_type>( 1);

                        common::range::copy(
                              common::range::make( bqual),
                              std::begin( result.trid.xid.data) + result.trid.xid.gtrid_length);

                        result.trid.xid.bqual_length = bqual.size();
                     }

                     result.trid.xid.formatID = row.get< long>( 2);

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

         m_connection.execute(
            R"( CREATE TABLE IF NOT EXISTS trans (
            gtrid         BLOB,
            bqual         BLOB,
            format        NUMBER,
            pid           NUMBER,
            state         NUMBER,
            started       NUMBER,
            updated       NUMBER,
            PRIMARY KEY (gtrid, bqual)); )");

         m_connection.execute(
            "CREATE INDEX IF NOT EXISTS i_xid_trans ON trans ( gtrid, bqual);" );


         m_statement.begin = m_connection.precompile( R"( INSERT INTO trans VALUES (?,?,?,?,?,?,?); )" );

         m_statement.update.state = m_connection.precompile( R"( UPDATE trans SET state = :state, updated = :updated WHERE gtrid = :gtrid AND bqual = :bqual; )");

         m_statement.select.all = m_connection.precompile( R"( SELECT * FROM trans; )");
         m_statement.select.transaction = m_connection.precompile( "SELECT gtrid, bqual, format, pid, state, started, updated FROM trans WHERE gtrid = :gtrid AND bqual = :bqual;");

         m_statement.remove = m_connection.precompile( "DELETE FROM trans WHERE gtrid = ? AND bqual = ?; ");

      }

      void Log::begin( const common::message::transaction::begin::Request& request)
      {
         common::log::internal::transaction << "log begin for xid: " << request.trid << "\n";

         auto started = std::chrono::time_point_cast< std::chrono::microseconds>( request.start).time_since_epoch().count();
         auto updated = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();

         long state = Log::State::cBegin;

         m_statement.begin.execute(
               common::transaction::global( request.trid),
               common::transaction::branch( request.trid),
               request.trid.xid.formatID,
               request.process.pid,
               state,
               started,
               updated);
      }

      void Log::commit( const common::message::transaction::commit::Request& request)
      {

      }

      void Log::rollback( const common::message::transaction::rollback::Request& request)
      {

      }

      void Log::prepareCommit( const common::transaction::ID& id)
      {
         auto updated = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now()).time_since_epoch().count();
         long state = State::cPreparedCommit;

         m_statement.update.state.execute(
               state,
               updated,
               common::transaction::global( id),
               common::transaction::branch( id));
      }

      std::vector< Log::Row> Log::select( const common::transaction::ID& id)
      {
         std::vector< Row> result;

         auto query = m_statement.select.transaction.query(
               common::transaction::global( id),
               common::transaction::branch( id));

         sql::database::Row row;

         while( query.fetch( row))
         {
            result.push_back( local::transform::Row()( row));
         }

         return result;
      }

      void Log::remove( const common::transaction::ID& trid)
      {
         m_statement.remove.execute(
            common::transaction::global( trid),
            common::transaction::branch( trid));
      }

      std::vector< Log::Row> Log::select()
      {
         std::vector< Row> result;

         auto query = m_statement.select.all.query();

         sql::database::Row row;

         while( query.fetch( row))
         {
            result.push_back( local::transform::Row()( row));
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
