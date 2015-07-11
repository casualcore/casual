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
               common::transaction::ID trid( sql::database::Row& row, int index = 0)
               {
                  common::transaction::ID result;

                  {
                     auto gtrid = row.get< common::platform::binary_type>( index);

                     common::range::copy( common::range::make( gtrid), std::begin( result.xid.data));

                     result.xid.gtrid_length = gtrid.size();
                  }

                  {
                     auto bqual = row.get< common::platform::binary_type>( index + 1);

                     common::range::copy(
                           common::range::make( bqual),
                           std::begin( result.xid.data) + result.xid.gtrid_length);

                     result.xid.bqual_length = bqual.size();
                  }

                  result.xid.formatID = row.get< long>( index + 2);

                  return result;
               }

               struct Row
               {
                  Log::Row operator () ( sql::database::Row& row) const
                  {
                     Log::Row result;

                     result.trid = transform::trid( row);


                     result.pid = row.get< common::platform::pid_type>( 3);
                     result.state = static_cast< Log::State>( row.get< long>( 4));

                     result.started = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( 5)}};
                     result.updated = common::platform::time_point{ std::chrono::microseconds{ row.get< common::platform::time_point::rep>( 6)}};

                     return result;
                  }
               };
            } // transform
         }
      }

      Log::Log( const std::string& database)
            : m_connection( database)
      {
         //m_connection.execute( "PRAGMA journal_mode = WAL;");

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

         m_connection.execute(
            "CREATE INDEX IF NOT EXISTS i_deadline_trans ON trans ( deadline);" );


         m_statement.begin = m_connection.precompile( R"( INSERT INTO trans VALUES (?,?,?,?,?,?,?); )" );

         m_statement.update.state = m_connection.precompile( R"( UPDATE trans SET state = :state WHERE gtrid = :gtrid AND bqual = :bqual; )");

         m_statement.select.all = m_connection.precompile( R"( SELECT * FROM trans; )");
         m_statement.select.transaction = m_connection.precompile( "SELECT gtrid, bqual, format, pid, state, started, deadline FROM trans WHERE gtrid = :gtrid AND bqual = :bqual;");

         m_statement.remove = m_connection.precompile( "DELETE FROM trans WHERE gtrid = ? AND bqual = ?; ");


         m_statement.deadline.earliest = m_connection.precompile( R"( SELECT MIN( deadline) FROM trans WHERE state = 10; )");

         m_statement.deadline.transactions = m_connection.precompile( R"( SELECT gtrid, bqual, format, pid FROM trans WHERE deadline < :deadline AND state = 10)");

      }

      void Log::begin( const common::message::transaction::begin::Request& request)
      {
         common::log::internal::transaction << "log begin for xid: " << request.trid << "\n";

         auto started = std::chrono::time_point_cast< std::chrono::microseconds>( request.start).time_since_epoch().count();

         long state = Log::State::cBegin;

         if( request.timeout == std::chrono::microseconds{ 0})
         {
            //
            // No deadline
            //

            m_statement.begin.execute(
                  common::transaction::global( request.trid),
                  common::transaction::branch( request.trid),
                  request.trid.xid.formatID,
                  request.process.pid,
                  state,
                  started,
                  nullptr);
         }
         else
         {
            //
            // Set deadline, and we add 1s just to let boarder cases have higher chance of better semantics.
            // If this deadline kicks in, it'll be errors from RM:s, and not from xatmi-API.
            //
            auto deadline = std::chrono::time_point_cast< std::chrono::microseconds>( request.start + request.timeout + std::chrono::seconds{ 1}).time_since_epoch().count();

            m_statement.begin.execute(
                  common::transaction::global( request.trid),
                  common::transaction::branch( request.trid),
                  request.trid.xid.formatID,
                  request.process.pid,
                  state,
                  started,
                  deadline);
         }
      }


      void Log::prepare( const common::transaction::ID& id)
      {
         state( id, State::cPrepared);
      }

      void Log::remove( const common::transaction::ID& trid)
      {
         m_statement.remove.execute(
            common::transaction::global( trid),
            common::transaction::branch( trid));
      }

      void Log::timeout( const common::transaction::ID& xid)
      {
         state( xid, State::cTimeout);
      }

      std::chrono::microseconds Log::timeout()
      {
         auto query = m_statement.deadline.earliest.query();

         sql::database::Row row;

         if( query.fetch( row) && ! row.null( 0))
         {
            auto timeout = row.get< common::platform::time_point::rep>( 0);

            return std::chrono::duration_cast< std::chrono::microseconds>(
                  common::platform::time_point{ std::chrono::microseconds{ timeout}} - common::platform::clock_type::now());
         }
         return std::chrono::microseconds::min();
      }

      std::vector< common::transaction::ID> Log::passed( const common::platform::time_point& now)
      {
         std::vector< common::transaction::ID> result;

         auto query = m_statement.deadline.transactions.query(
               std::chrono::time_point_cast< std::chrono::microseconds>( now).time_since_epoch().count());

         sql::database::Row row;

         while( query.fetch( row))
         {
            result.push_back( local::transform::trid( row));
         }

         return result;
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

      void Log::state( const common::transaction::ID& id, long state)
      {
         m_statement.update.state.execute(
               state,
               common::transaction::global( id),
               common::transaction::branch( id));
      }

   } // transaction
} // casual
