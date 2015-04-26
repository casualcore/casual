/*
 * monitordb.h
 *
 *  Created on: 8 dec 2012
 *      Author: hbergk
 */

#ifndef MONITORDB_H_
#define MONITORDB_H_

#include "sql/database.h"
#include "common/message/monitor.h"
#include <string>
#include <vector>

#include "traffic_monitor/serviceentryvo.h"

namespace casual
{
   namespace traffic_monitor
   {
      class Database
      {
      public:

         Database();

         Database( const std::string& database);

         ~Database();

         void insert( const common::message::traffic_monitor::Notify& message);
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
         Transaction( Database& db);
         ~Transaction();

         Database& m_database;
      };

   }

}

void operator>>( const casual::common::message::traffic_monitor::Notify& message, casual::traffic_monitor::Database& db);


#endif /* MONITORDB_H_ */
