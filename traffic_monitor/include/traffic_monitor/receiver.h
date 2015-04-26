/*
 * casual_monitor.h
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#ifndef CASUAL_MONITOR_H_
#define CASUAL_MONITOR_H_

#include <vector>
#include <string>

#include "traffic_monitor/database.h"
#include "common/ipc.h"
#include "common/message/monitor.h"




namespace casual
{
   namespace traffic_monitor
   {
      class Receiver
      {
      public:

         Receiver( const std::vector< std::string>& arguments);
         ~Receiver();


         //!
         //! Start the server
         //!
         void start();

      private:
         common::ipc::receive::Queue& m_receiveQueue;
         Database& m_database;
      };

      namespace handle
      {
         //!
         //! Used with the dispatch handler to handle relevant
         //! messages from queue
         //!
         struct Notify
         {
         public:
            typedef common::message::traffic_monitor::Notify message_type;
            Notify( Database& db ) : database( db)
            {
            };

            void operator () ( const message_type& message);

         private:
            Database& database;

         };
      }
   }
}



#endif /* CASUAL_MONITOR_H_ */
