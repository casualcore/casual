/*
 * monitordb.h
 *
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */

#ifndef CASUAL_MONITOR_DATABASE_H_
#define CASUAL_MONITOR_DATABASE_H_

#include "sql/database.h"
#include "common/message/monitor.h"
#include <string>
#include <vector>

#include "traffic/monitor/serviceentryvo.h"

namespace casual
{
namespace traffic
{
namespace monitor
{
   struct Transaction; // Forward

   class Database
   {
   public:

      typedef Transaction transaction_type;
      Database();

      Database( const std::string& database);

      ~Database();

      void store( const common::message::traffic::monitor::Notify& message);
      std::vector< ServiceEntryVO> select( );

      sql::database::Connection& getConnection();

   private:
      //
      // Creates table if necessary
      //
      void createTable();

      sql::database::Connection m_connection;
   };

   struct Transaction
   {
      Transaction( Database& database);
      ~Transaction();
   private:
      Database& m_database;
   };

}
}
}

void operator>>( const casual::common::message::traffic::monitor::Notify& message, casual::traffic::monitor::Database& db);


#endif /* CASUAL_MONITOR_DATABASE_H_ */
