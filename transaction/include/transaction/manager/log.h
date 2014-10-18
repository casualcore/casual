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
            cPreparedCommit,
            cPrepareRollback


         };

         struct Row
         {
            common::transaction::ID trid;
            common::platform::pid_type pid = 0;
            common::platform::time_type started;
            common::platform::time_type updated;
            State state = cUnknown;
         };


         Log( const std::string& database);

         void begin( const common::message::transaction::begin::Request& request);

         void commit( const common::message::transaction::commit::Request& request);

         void rollback( const common::message::transaction::rollback::Request& request);

         void remove( const common::transaction::ID& xid);


         void prepareCommit( const common::transaction::ID& id);


         std::vector< Row> select( const common::transaction::ID& id);

         std::vector< Row> select();



         void writeBegin();
         void writeCommit();
         void writeRollback();

      private:

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
