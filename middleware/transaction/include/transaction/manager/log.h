//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/global.h"

#include "sql/database.h"

#include "common/platform.h"

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

            Log( std::string database);
            ~Log();

            void prepare( const Transaction& transaction);
            void remove( const global::ID& global);

            //! persist the current "transaction" and start a new one
            void persist();

            struct Stats
            {
               struct
               {
                  common::platform::size::type prepare = 0;
                  common::platform::size::type remove = 0;
               } update;

               common::platform::size::type writes = 0;
            };

            const Stats& stats() const;


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

            struct
            {
               sql::database::Statement insert;
               sql::database::Statement remove;

            } m_statement;

            Stats m_stats;
         };
      } // manager
   } // transaction
} // casual


