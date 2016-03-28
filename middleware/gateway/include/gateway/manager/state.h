
#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_


#include "gateway/manager/listener.h"

#include "common/process.h"
#include "common/domain.h"

#include "common/communication/tcp.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace state
         {
            struct base_connection
            {
               enum class Type
               {
                  unknown,
                  ipc,
                  tcp
               };

               enum class Runlevel
               {
                  absent,
                  booting,
                  online,
                  shutdown,
                  error
               };

               common::process::Handle process;
               common::domain::Identity remote;
               Type type = Type::unknown;
               Runlevel runlevel = Runlevel::absent;


               friend bool operator == ( const base_connection& lhs, common::platform::pid_type rhs);
               inline friend bool operator == ( common::platform::pid_type lhs, const base_connection& rhs)
               {
                  return rhs == lhs;
               }
            };

            namespace inbound
            {
               struct Connection : base_connection
               {

                  friend std::ostream& operator << ( std::ostream& out, const Connection& value);
               };


            } // inbound

            namespace outbound
            {
               struct Connection : base_connection
               {


                  std::string address;

                  bool restart = false;

                  friend std::ostream& operator << ( std::ostream& out, const Connection& value);
               };


            } // outbound

            struct Connections
            {
               std::vector< outbound::Connection> outbound;
               std::vector< inbound::Connection> inbound;
            };



         } // state

         struct State
         {
            enum class Runlevel
            {
               startup,
               online,
               shutdown
            };

            struct
            {
               std::vector< common::communication::tcp::Address> listeners;

            } configuration;

            void event( const message::manager::listener::Event& event);


            state::Connections connections;
            std::vector< Listener> listeners;
            Runlevel runlevel = Runlevel::startup;
         };


      } // manager

   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
