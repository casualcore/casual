
#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_


#include "gateway/manager/listener.h"

#include "common/process.h"
#include "common/domain.h"

#include "common/communication/tcp.h"

#include "common/message/coordinate.h"


namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace state
         {
            using size_type = common::platform::size::type;
            
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
                  connecting,
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

                  //!
                  //! configured queues
                  //!
                  std::vector< std::string> queues;

                  size_type order = 0;
                  bool restart = false;

                  void reset();

                  friend std::ostream& operator << ( std::ostream& out, const Connection& value);
               };


            } // outbound

            struct Connections
            {
               std::vector< outbound::Connection> outbound;
               std::vector< inbound::Connection> inbound;
            };

            namespace coordinate
            {
               namespace outbound
               {

                  struct Policy
                  {
                     using message_type = common::message::gateway::domain::discover::accumulated::Reply;

                     void accumulate( message_type& message, common::message::gateway::domain::discover::Reply& reply);

                     void send( common::communication::ipc::Handle queue, message_type& message);
                  };

                  using Discover = common::message::Coordinate< Policy>;

               } // outbound


            } // coordinate

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

            struct Discover
            {
               state::coordinate::outbound::Discover outbound;
               void remove( common::platform::pid::type pid);

            } discover;

            Runlevel runlevel = Runlevel::startup;

            friend std::ostream& operator << ( std::ostream& out, const Runlevel& value);
            friend std::ostream& operator << ( std::ostream& out, const State& value);
         };


      } // manager

   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
