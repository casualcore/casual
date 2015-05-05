/*
 * casual_monitor.h
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#ifndef CASUAL_MONITOR_HANDLER_H_
#define CASUAL_MONITOR_HANDLER_H_

#include <vector>
#include <string>

#include "traffic/monitor/database.h"
#include "common/ipc.h"
#include "common/message/monitor.h"




namespace casual
{
namespace traffic
{
namespace monitor
{
    namespace handle
   {
      //!
      //! Used with the dispatch handler to handle relevant
      //! messages from queue
      //!
      struct Notify
      {
      public:
         typedef common::message::traffic::monitor::Notify message_type;
         Notify( Database& database) : m_database( database)
         {
         };

         void operator () ( const message_type& message);
      private:
         Database& m_database;
      };
   }
}
}
}


#endif /* CASUAL_MONITOR_HANDLER_H_ */
