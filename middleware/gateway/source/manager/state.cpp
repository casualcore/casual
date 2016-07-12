//!
//! casual 
//!

#include "gateway/manager/state.h"

#include "gateway/message.h"
#include "gateway/common.h"



#include "common/trace.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace manager
      {
         namespace state
         {

            bool operator == ( const base_connection& lhs, common::platform::pid::type rhs)
            {
               return lhs.process.pid == rhs;
            }

            std::ostream& operator << ( std::ostream& out, const base_connection::Type& value)
            {
               switch( value)
               {
                  case base_connection::Type::ipc: { return out << "ipc";}
                  case base_connection::Type::tcp: { return out << "tcp";}
                  default: return out << "unknown";
               }
            }

            std::ostream& operator << ( std::ostream& out, const base_connection::Runlevel& value)
            {
               switch( value)
               {
                  case base_connection::Runlevel::absent: { return out << "absent";}
                  case base_connection::Runlevel::booting: { return out << "booting";}
                  case base_connection::Runlevel::online: { return out << "online";}
                  case base_connection::Runlevel::offline: { return out << "offline";}
                  case base_connection::Runlevel::error: { return out << "error";}
                  default: return out << "unknown";
               }
            }

            bool base_connection::running() const
            {
               switch( runlevel)
               {
                  case Runlevel::booting:
                  case Runlevel::online:
                  {
                     return true;
                  }
                  default: return false;
               }
            }

            namespace inbound
            {
               std::ostream& operator << ( std::ostream& out, const Connection& value)
               {
                  return out << "{ type: " << value.type << ", runlevel: " << value.runlevel << ", process: " << value.process << ", remote: " << value.remote << '}';
               }
            }

            namespace outbound
            {
               std::ostream& operator << ( std::ostream& out, const Connection& value)
               {
                  return out << "{ type: " << value.type
                        << ", runlevel: " << value.runlevel
                        << ", process: " << value.process
                        << ", remote: " << value.remote
                        << ", services: " << range::make( value.services)
                        << '}';
               }
            }


         } // state


         bool State::running() const
         {
            return range::any_of( listeners, std::mem_fn( &Listener::running))
               || range::any_of( connections.outbound, std::mem_fn( &state::outbound::Connection::running))
               || range::any_of( connections.inbound, std::mem_fn( &state::inbound::Connection::running));
         }

         void State::event( const message::manager::listener::Event& event)
         {
            Trace trace{ "manager::State::event"};

            log << "event: " << event << '\n';

            auto found = range::find( listeners, event.correlation);

            if( found)
            {
               found->event( event);
            }
            else
            {
               throw exception::invalid::Argument{ "failed to correlate listener to event", CASUAL_NIP( event)};
            }
         }

         std::ostream& operator << ( std::ostream& out, const State::Runlevel& value)
         {
            switch( value)
            {
               case State::Runlevel::startup: { return out << "startup";}
               case State::Runlevel::online: { return out << "online";}
               case State::Runlevel::shutdown: { return out << "shutdown";}
               default: return out << "unknown";
            }
         }

         std::ostream& operator << ( std::ostream& out, const State& value)
         {
            return out << "{ runlevel: " << value.runlevel
               << ", listeners: " << range::make( value.listeners)
               << ", outbound: " << range::make( value.connections.outbound)
               << ", inbound: " << range::make( value.connections.inbound)
               << '}';

         }

      } // manager
   } // gateway
} // casual
