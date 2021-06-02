//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/global.h"

#include "sql/database.h"

#include "casual/platform.h"

#include "common/message/transaction.h"


#include <string>

namespace casual
{
   namespace transaction
   {
      namespace manager
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

            Log() = default;
            Log( std::filesystem::path file);
            ~Log();

            void prepare( const Transaction& transaction);
            void remove( const global::ID& global);

            //! persist the current "transaction" and start a new one
            void persist();

            struct Stats
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

            const Stats& stats() const;

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

            Stats m_stats;
         };
      } // manager
   } // transaction
} // casual


