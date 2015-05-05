/*
 * casual_monitor.h
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#ifndef CASUAL_TRAFFIC_LOG_NOTIFY_H_
#define CASUAL_TRAFFIC_LOG_NOTIFY_H_

#include <vector>
#include <string>

#include "traffic/log/file.h"
#include "common/ipc.h"
#include "common/message/monitor.h"




namespace casual
{
namespace traffic
{
namespace log
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
         Notify( File& file) : m_file( file)
         {
         };

         void operator () ( const message_type& message);
      private:
         File& m_file;
      };
   }
}
}
}


#endif /* CASUAL_TRAFFIC_LOG_NOTIFY_H_ */
