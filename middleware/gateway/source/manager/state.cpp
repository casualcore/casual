//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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

            std::ostream& operator << ( std::ostream& out, const base_connection::Runlevel& value)
            {
               switch( value)
               {
                  case base_connection::Runlevel::absent: { return out << "absent";}
                  case base_connection::Runlevel::connecting: { return out << "connecting";}
                  case base_connection::Runlevel::online: { return out << "online";}
                  case base_connection::Runlevel::offline: { return out << "offline";}
                  case base_connection::Runlevel::error: { return out << "error";}
               }
               return out << "unknown";
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


            namespace outbound
            {
               void Connection::reset()
               {
                  process = common::process::Handle{};
                  runlevel = state::outbound::Connection::Runlevel::absent;
                  remote = common::domain::Identity{};
                  address.local.clear();

                  common::log::line( log, "manager::state::outbound::reset: ", *this);
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

                     // TODO maintainence: state should not communicate with anything, just hold state.
                     communication::ipc::blocking::send( queue, message);
                  }
               } // outbound

            } // coordinate
         } // state


         bool State::running() const
         {
            return algorithm::any_of( connections.outbound, std::mem_fn( &state::outbound::Connection::running))
               || algorithm::any_of( connections.inbound, std::mem_fn( &state::inbound::Connection::running));
         }

         void State::add( listen::Entry entry)
         {
            Trace trace{ "gateway::manager::State::add"};

            m_listeners.push_back( std::move( entry));
            m_descriptors.push_back( m_listeners.back().descriptor());
            directive.read.add( m_listeners.back().descriptor());
         }

         void State::remove( common::strong::file::descriptor::id listener)
         {
            Trace trace{ "gateway::manager::State::remove"};


            auto found = common::algorithm::find_if( m_listeners, [listener]( const listen::Entry& entry) {
               return entry.descriptor() == listener;
            });

            if( ! found)
               throw common::exception::system::invalid::Argument{ common::string::compose( "failed to find descriptor in listeners - descriptor: ", listener)};


            m_listeners.erase( std::begin( found));
            algorithm::trim( m_descriptors, common::algorithm::remove( m_descriptors, listener));
            directive.read.remove( listener);
         }

         listen::Connection State::accept( common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::manager::State::accept"};

            auto found = common::algorithm::find_if( m_listeners, [descriptor]( const listen::Entry& entry) {
               return entry.descriptor() == descriptor;
            });

            if( found)
               return found->accept();

            return {};
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
            }
            return out << "unknown";
         }
      } // manager
   } // gateway
} // casual
