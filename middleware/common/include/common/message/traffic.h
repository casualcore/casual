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
               namespace connect
               {
                  //!
                  //! Used to advertise the monitorserver
                  //!
                  using Request = server::basic_id< Type::traffic_monitor_connect_request>;
                  using Reply = server::basic_id< Type::traffic_monitor_connect_reply>;

               } // connect


               //!
               //! Used to unadvertise the monitorserver
               //!
               using Disconnect = server::basic_id< Type::traffic_monitor_disconnect> ;


            } // monitor

            //!
            //! Notify traffic-monitor with statistic-event
            //!
            struct Event : basic_message< Type::traffic_event>
            {
               std::string service;
               std::string parent;
               common::process::Handle process;

               common::transaction::ID trid;

               common::platform::time_point start;
               common::platform::time_point end;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  base_type::marshal( archive);
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
            static_assert( traits::is_movable< Event>::value, "not movable");

         } // traffic

         namespace reverse
         {
            template<>
            struct type_traits< traffic::monitor::connect::Request> : detail::type< traffic::monitor::connect::Reply> {};

         } // reverse
      } // message
   } // common
} // casual

#endif // MONITOR_H_
