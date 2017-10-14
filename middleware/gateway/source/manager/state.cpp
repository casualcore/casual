//!
//! casual 
//!

#include "gateway/manager/state.h"

#include "gateway/message.h"
#include "gateway/manager/handle.h"
#include "gateway/common.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace manager
      {
         namespace state
         {

            bool operator == ( const base_connection& lhs, common::strong::process::id rhs)
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
                  case base_connection::Runlevel::connecting: { return out << "connecting";}
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
                  case Runlevel::connecting:
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
               void Connection::reset()
               {
                  process = common::process::Handle{};
                  runlevel = state::outbound::Connection::Runlevel::absent;
                  remote = common::domain::Identity{};

                  log << "manager::state::outbound::reset: " << *this << '\n';
               }

               std::ostream& operator << ( std::ostream& out, const Connection& value)
               {
                  return out << "{ type: " << value.type
                        << ", runlevel: " << value.runlevel
                        << ", process: " << value.process
                        << ", remote: " << value.remote
                        << ", restart: " << value.restart
                        << ", order: " << value.order
                        << ", services: " << range::make( value.services)
                        << '}';
               }
            }

            namespace coordinate
            {
               namespace outbound
               {
                  void Policy::accumulate( message_type& message, common::message::gateway::domain::discover::Reply& reply)
                  {
                     Trace trace{ "manager::state::coordinate::outbound::Policy accumulate"};

                     message.replies.push_back( std::move( reply));
                  }

                  void Policy::send( common::strong::ipc::id queue, message_type& message)
                  {
                     Trace trace{ "manager::state::coordinate::outbound::Policy send"};

                     manager::ipc::device().blocking_send( queue, message);

                  }
               } // outbound

            } // coordinate
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
               throw exception::system::invalid::Argument{ string::compose( "failed to correlate listener to event: ", event)};
            }
         }

         void State::Discover::remove( common::strong::process::id pid)
         {
            outbound.remove( pid);
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
