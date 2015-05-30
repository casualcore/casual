//!
//! monitor.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMMONMESSAGEMONITOR_H_
#define COMMMONMESSAGEMONITOR_H_

#include "common/message/type.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace traffic
         {
            namespace monitor
            {
               //!
               //! Used to advertise the monitorserver
               //!
               typedef server::basic_connect< cTrafficMonitorConnect> Connect;

               //!
               //! Used to unadvertise the monitorserver
               //!
               typedef server::basic_disconnect< cTrafficMonitorDisconnect> Disconnect;


            } // monitor

            //!
            //! Notify traffic-monitor with statistic-event
            //!
            struct Event : basic_message< cTrafficEvent>
            {
               std::string service;
               std::string parent;
               common::process::Handle process;

               common::Uuid execution;

               common::transaction::ID trid;

               common::platform::time_point start;
               common::platform::time_point end;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & service;
                  archive & parent;
                  archive & process;
                  archive & execution;
                  archive & trid;
                  archive & start;
                  archive & end;
               )

               friend std::ostream& operator << ( std::ostream& out, const Event& value);
            };

         } // traffic
      } // message
   } // common
} // casual

#endif // MONITOR_H_
