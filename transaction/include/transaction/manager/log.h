//!
//! log.h
//!
//! Created on: Nov 3, 2013
//!     Author: Lazan
//!

#ifndef LOG_H_
#define LOG_H_

#include "sql/database.h"

#include "common/message/transaction.h"

//
// std
//
#include <string>

namespace casual
{
   namespace transaction
   {
      class Log
      {
      public:

         // TOOD: should be at a "higher" level
         enum State
         {
            cUnknown = 0,
            cBegin = 10,
            cPrepared,
            cPrepareRollback,
            cTimeout
         };

         struct Row
         {
            common::transaction::ID trid;
            common::platform::pid_type pid = 0;
            common::platform::time_point started;
            common::platform::time_point updated;
            State state = cUnknown;
         };


         Log( const std::string& database);

         void begin( const common::message::transaction::begin::Request& request);

         void prepare( const common::transaction::ID& id);

         void remove( const common::transaction::ID& xid);

         //!
         //! Set transaction in timeout-mode
         //!
         void timeout( const common::transaction::ID& xid);

         //!
         //! @returns the earliest deadline there is.
         //!
         common::platform::time_point deadline();

         std::vector< common::transaction::ID> passed( const common::platform::time_point& now);


         std::vector< Row> select( const common::transaction::ID& id);

         std::vector< Row> select();



         void writeBegin();
         void writeCommit();
         void writeRollback();

      private:

         void state( const common::transaction::ID& id, long state);

         sql::database::Connection m_connection;

         struct statement_t
         {
            sql::database::Statement begin;

            struct update_t
            {
               sql::database::Statement state;

            } update;

            struct select_t
            {
               sql::database::Statement all;
               sql::database::Statement transaction;

            } select;

            struct deadline_t
            {
               sql::database::Statement earliest;
               sql::database::Statement transactions;

            } deadline;

            sql::database::Statement remove;


         } m_statement;

      };

      namespace scoped
      {
         struct Writer
         {
            Writer( Log& log) : m_log( log)
            {
               // TODO: error checking?
               m_log.writeBegin();
            }

            ~Writer()
            {
               m_log.writeCommit();
            }

         private:
            Log& m_log;
         };
      } // scoped

   } // transaction

} // casual

#endif // LOG_H_
