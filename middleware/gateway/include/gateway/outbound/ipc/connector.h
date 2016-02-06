//!
//! connector.h
//!
//! Created on: Dec 24, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_IPC_CONNECTOR_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_IPC_CONNECTOR_H_


#include <string>

namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         namespace ipc
         {
            struct Settings
            {
               std::string domain_path;

            };

            struct State
            {
               State( Settings settings);

               std::string domain_path;
            };

            struct Connector
            {
               Connector( Settings settings);

               void start();

            private:
               State m_state;

            };

         } // ipc
      } // outbound
   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_OUTBOUND_IPC_CONNECTOR_H_
