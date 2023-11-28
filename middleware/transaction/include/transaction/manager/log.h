//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "sql/database.h"

#include "casual/platform.h"

#include "common/transaction/global.h"
#include "common/message/transaction.h"


#include <string>

namespace casual
{
   namespace transaction::manager
   {
      namespace state
      {
         struct Transaction;   
      } // state


      struct Log
      {
         // TOOD: should be at a "higher" level
         enum class State : int
         {
            prepared = 10,
            prepare_rollback,
            timeout
         };

         Log() = default;
         Log( std::filesystem::path file);
         ~Log();

         void prepare( const state::Transaction& transaction);
         void remove( common::transaction::id::range::type::global global);
         inline void remove( const common::transaction::global::ID& global) { remove( global.range());}

         //! persist the current "transaction" and start a new one
         void persist();

         struct Statistics
         {
            struct
            {
               platform::size::type prepare{};
               platform::size::type remove{};

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( prepare);
                  CASUAL_SERIALIZE( remove);
               )
            } update;

            platform::size::type writes{};

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( update);
               CASUAL_SERIALIZE( writes);
            )
         };

         const Statistics& statistics() const;

         inline auto& file() const noexcept { return m_connection.file();}


         //! Only for unittest purpose
         //! @{
         struct Row
         {
            common::transaction::ID trid;
            common::strong::process::id pid;
            platform::time::point::type started;
            platform::time::point::type updated;
            State state = State::prepared;
         };

         std::vector< Row> logged();
         //! @}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_connection, "connection");
            CASUAL_SERIALIZE_NAME( m_statement, "statement");
            CASUAL_SERIALIZE_NAME( m_stats, "stats");
         )

      private:

         sql::database::Connection m_connection;

         struct
         {
            sql::database::Statement insert;
            sql::database::Statement remove;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( insert);
               CASUAL_SERIALIZE( remove);  
            )

         } m_statement;

         Statistics m_stats;
      };

   } // transaction::manager
} // casual


