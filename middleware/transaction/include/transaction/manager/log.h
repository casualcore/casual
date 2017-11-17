//!
//! casual
//!

#ifndef CASUAL_TRANSACTION_MANAGER_LOG_H_
#define CASUAL_TRANSACTION_MANAGER_LOG_H_


#include "sql/database.h"

#include "sf/platform.h"

#include "common/message/transaction.h"

//
// std
//
#include <string>

namespace casual
{
   namespace transaction
   {
      struct Transaction;


      class Log
      {
      public:

         // TOOD: should be at a "higher" level
         enum class State : int
         {
            prepared = 10,
            prepare_rollback,
            timeout
         };


         Log( std::string database);


         void prepare( const transaction::Transaction& transaction);

         void remove( const common::transaction::ID& xid);



         void writeBegin();
         void writeCommit();
         void writeRollback();

         struct Stats
         {
            struct update_t
            {
               sf::platform::size::type prepare = 0;
               sf::platform::size::type remove = 0;
            } update;

            sf::platform::size::type writes = 0;
         };

         const Stats& stats() const;


         //!
         //! Only for unittest purpose
         //! @{

         struct Row
         {
            common::transaction::ID trid;
            common::strong::process::id pid;
            common::platform::time::point::type started;
            common::platform::time::point::type updated;
            State state = State::prepared;
         };

         std::vector< Row> logged();
         //! @}


      private:


         sql::database::Connection m_connection;

         struct statement_t
         {
            sql::database::Statement insert;
            sql::database::Statement remove;

         } m_statement;

         Stats m_stats;
      };


      namespace persistent
      {
         struct Writer
         {
            enum class State
            {
               begun,
               committed,
            };

            inline Writer( Log& log) : m_log( log), m_state{ State::committed} {}

            inline void begin()
            {
               if( m_state == State::committed)
               {
                  m_log.writeBegin();
                  m_state = State::begun;
               }
            }

            inline void commit()
            {
               if( m_state == State::begun)
               {
                  m_log.writeCommit();
                  m_state = State::committed;
               }
            }

            inline ~Writer()
            {
               commit();
            }

         private:
            Log& m_log;
            State m_state;
         };

      } // persistent

   } // transaction

} // casual

#endif // LOG_H_
