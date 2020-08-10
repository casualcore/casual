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
                  void Policy::operator() ( message_type& message, common::message::gateway::domain::discover::Reply& reply)
                  {
                     Trace trace{ "manager::state::coordinate::outbound::Policy"};

                     message.replies.push_back( std::move( reply));
                  }


                  namespace rediscover
                  {
                     namespace local
                     {
                        namespace
                        {
                           auto request = []( auto& outbound, auto& correlation)
                           {
                              message::outbound::rediscover::Request message{ common::process::handle()};
                              message.correlation = correlation;
                              message.services = outbound.services;
                              message.queues = outbound.queues;

                              return Tasks::Result::Request{ outbound.process, std::move( message)};
                           };
                        } // <unnamed>
                     } // local

                     Tasks::Result Tasks::add( State& state, std::string description)
                     {
                        Trace trace{ "manager::state::coordinate::outbound::rediscover::Tasks::add"};

                        auto correlation = uuid::make();

                        auto running_outbounds = algorithm::sort( algorithm::filter( state.connections.outbound, []( auto& outbound){ return outbound.running();}));

                        if( ! running_outbounds)
                           return { correlation, {}, std::move( description)};

                        Task task{
                           correlation,
                           algorithm::transform( running_outbounds, []( auto& outbound){ return outbound.process.pid;}),
                           description
                        };

                        m_tasks.push_back( std::move( task));

                        // we start with the last one (highest order).
                        auto& outbound = range::back( running_outbounds);

                        return { correlation, local::request( outbound, correlation), std::move( description)};
                     }

                     Tasks::Result Tasks::reply( State& state, const message::outbound::rediscover::Reply& message)
                     {
                        Trace trace{ "manager::state::coordinate::outbound::rediscover::Tasks::reply"};

                        auto task = algorithm::find( m_tasks, message.correlation);

                        if( ! task)
                        {
                           log::line( log::category::error, "failed to correlate rediscover reply - action: discard");
                           return {};
                        }

                        if( algorithm::trim( task->outbounds, algorithm::remove( task->outbounds, message.process.pid)).empty())
                        {
                           // we're done, no more outbounds to rediscover, we remove the task
                           Tasks::Result result{ task->correlation, {}, std::move( task->description)};
                           m_tasks.erase( std::begin( task));
                           return result;
                        }

                        auto outbound = algorithm::find( state.connections.outbound, task->outbounds.back());

                        if( ! outbound)
                        {
                           log::line( log::category::error, "failed to find outbound - action: discard");
                           return { task->correlation, {}, task->description};
                        }

                        return { task->correlation, local::request( *outbound, task->correlation), task->description};
                     }

                     std::vector< Tasks::Task> Tasks::remove( common::strong::process::id pid)
                     {
                        auto remove_pid = [&]( auto& task)
                        {
                           algorithm::trim( task.outbounds, algorithm::remove( task.outbounds, pid));
                        };

                        algorithm::for_each( m_tasks, remove_pid);

                        auto split = algorithm::partition( m_tasks, []( auto& task){ return ! task.outbounds.empty();});

                        auto result = range::to_vector( std::get< 1>( split));

                        algorithm::trim( m_tasks, std::get< 0>( split));

                        return result;
                     }

                  } // rediscover

               } // outbound

            } // coordinate
         } // state

         namespace local
         {
            namespace
            {
               auto has_descriptor( strong::file::descriptor::id descriptor)
               {
                  return [descriptor]( auto& entry)
                  { 
                     return entry.descriptor() == descriptor;
                  };
               }
            } // <unnamed>
         } // local


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

            if( auto found = common::algorithm::find_if( m_listeners, local::has_descriptor( listener)))
            {
               m_listeners.erase( std::begin( found));
               algorithm::trim( m_descriptors, common::algorithm::remove( m_descriptors, listener));
               directive.read.remove( listener);
            }

            code::raise::log( code::casual::invalid_argument, "failed to find descriptor in listeners - descriptor: ", listener);
         }

         listen::Connection State::accept( common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::manager::State::accept"};

            if( auto found = common::algorithm::find_if( m_listeners, local::has_descriptor( descriptor)))
               return found->accept();

            return {};
         }

         const state::outbound::Connection* State::outbound( common::strong::process::id pid)
         {
            if( auto found = algorithm::find( connections.outbound, pid))
               return &range::front( found);

            return nullptr;
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
