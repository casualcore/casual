
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
                  offline,
                  error
               };


               bool running() const;

               common::process::Handle process;
               common::domain::Identity remote;
               std::vector< std::string> address;
               Type type = Type::unknown;
               Runlevel runlevel = Runlevel::absent;



               friend bool operator == ( const base_connection& lhs, common::platform::pid::type rhs);
               inline friend bool operator == ( common::platform::pid::type lhs, const base_connection& rhs)
               {
                  return rhs == lhs;
               }

               friend std::ostream& operator << ( std::ostream& out, const Type& value);
               friend std::ostream& operator << ( std::ostream& out, const Runlevel& value);
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

                  //!
                  //! configured services
                  //!
                  std::vector< std::string> services;
                  std::size_t order = 0;
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

            //!
            //! @return true if we have any running connections or listeners
            //!
            bool running() const;


            void event( const message::manager::listener::Event& event);

            state::Connections connections;
            std::vector< Listener> listeners;
            Runlevel runlevel = Runlevel::startup;

            friend std::ostream& operator << ( std::ostream& out, const Runlevel& value);
            friend std::ostream& operator << ( std::ostream& out, const State& value);
         };


      } // manager

   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
