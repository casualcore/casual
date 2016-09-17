//!
//! casual
//!

#include "broker/state.h"
#include "broker/transform.h"
#include "broker/common.h"

#include "common/server/service.h"
#include "common/server/lifetime.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/exception.h"
#include "common/algorithm.h"

#include "common/process.h"



#include <functional>

namespace casual
{

   namespace broker
   {
      using namespace common;


      namespace local
      {
         namespace
         {
            template< typename M, typename ID>
            auto get( M& map, ID&& id) -> decltype( map.at( id))
            {
               auto found = common::range::find( map, id);

               if( ! found)
               {
                  throw state::exception::Missing{ "missing", CASUAL_NIP( id)};
               }
               return found->second;
            }

         } // <unnamed>
      } // local


      namespace state
      {

         namespace instance
         {
            void Local::lock( const common::platform::time_point& when)
            {
               assert( m_state != State::busy);

               if( m_state == State::idle)
               {
                  m_state = State::busy;
               }

               ++invoked;
               m_last = when;
            }

            void Local::unlock( const common::platform::time_point& when)
            {
               assert( m_state != State::idle);

               if( m_state == State::busy)
               {
                  m_state = State::idle;
               }

               m_last = when;
            }

            void Remote::requested( const common::platform::time_point& when)
            {
               ++invoked;
               m_last = when;
            }

            bool operator < ( const Remote& lhs, const Remote& rhs)
            {
               return lhs.order < rhs.order;
            }

         } // instance

         namespace service
         {

            namespace instance
            {
               void Local::lock( const common::platform::time_point& when)
               {
                  get().lock( when);
                  ++invoked;
               }

               void Remote::lock( const common::platform::time_point& when)
               {
                  get().requested( when);
                  ++invoked;
               }


               bool operator < ( const Remote& lhs, const Remote& rhs)
               {
                  return std::make_tuple( lhs.get(), lhs.hops()) < std::make_tuple( rhs.get(), rhs.hops());
               }

            } // instance

         } // service

         void Service::remove( common::platform::pid::type instance)
         {
            {
               auto found = range::find( instances.local, instance);

               if( found)
               {
                  instances.local.erase( std::begin( found));
               }

            }

            {
               auto found = range::find( instances.remote, instance);

               if( found)
               {
                  instances.remote.erase( std::begin( found));
                  partition_remote_instances();
               }
            }
         }



         void Service::add( state::instance::Local& instance)
         {
            instances.local.emplace_back( instance);
         }

         void Service::add( state::instance::Remote& instance, std::size_t hops)
         {
            instances.remote.emplace_back( instance, hops);
            partition_remote_instances();
         }


         void Service::partition_remote_instances()
         {
            range::stable_sort( instances.remote);
         }

         service::instance::base_instance* Service::idle()
         {
            auto found = range::find_if( instances.local, std::mem_fn( &service::instance::Local::idle));

            if( found)
            {
               return &(*found);
            }

            if( instances.local.empty() && ! instances.remote.empty())
            {
               return &instances.remote.front();
            }

            return nullptr;
         }

         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ name: " << service.information.name
                  << ", type: " << service.information.type
                  << ", transaction: " << service.information.transaction
                  << ", timeout: " << service.information.timeout.count()
                  << "}";
          }

      } // state


      void State::traffic_t::monitors_t::add( common::process::Handle process)
      {
         processes.push_back( process);
      }

      void State::traffic_t::monitors_t::remove( common::platform::pid::type pid)
      {
         range::trim( processes, range::remove_if( processes, [pid]( const process::Handle& p){ return p.pid == pid;}));
      }

      std::vector< common::platform::ipc::id::type> State::traffic_t::monitors_t::get() const
      {
         return range::transform( processes, []( const process::Handle& p){ return p.queue;});
      }


      state::Service& State::service( const std::string& name)
      {
         return local::get( services, name);
      }

      state::instance::Local& State::local_instance( common::platform::pid::type pid)
      {
         return local::get( instances.local, pid);
      }


      namespace local
      {
         namespace
         {
            template< typename I, typename S>
            void remove_process( I& instances, S& services, common::platform::pid::type pid)
            {
               Trace trace{ "broker::local::remove_process"};

               auto found = common::range::find( instances, pid);

               if( found)
               {
                  log << "remove process pid: " << pid << std::endl;

                  for( auto& s : services)
                  {
                     s.second.remove( pid);
                  }

                  instances.erase( std::begin( found));
               }
            }




            template< typename I>
            auto find_or_add( I& instances, common::process::Handle process) -> decltype( instances.at( process.pid))
            {
               Trace trace{ "broker::local::find_or_add"};

               auto found = range::find( instances, process.pid);

               if( found)
               {
                  log << "process found\n";
                  return found->second;
               }

               return instances.emplace( process.pid, process).first->second;
            }

            template< typename Services, typename Service>
            auto find_or_add_service( Services& services, Service&& service) -> decltype( services.at( service.name))
            {
               Trace trace{ "broker::local::find_or_add service"};

               auto found = range::find( services, service.name);

               if( found)
               {
                  return found->second;
               }

               return services.emplace( service.name, std::move( service)).first->second;
            }


            template< typename Service>
            common::message::service::call::Service transform( Service&& service)
            {
               common::message::service::call::Service result;

               result.name = std::move( service.name);
               result.timeout = service.timeout;
               result.transaction = service.transaction;
               result.type = service.type;

               return result;
            }


            template< typename I, typename S>
            void remove_services( State& state, I& instances, S& services, common::process::Handle process)
            {
               Trace trace{ "broker::local::remove_services"};

               auto& instance = find_or_add( instances, process);

               for( auto& s : services)
               {
                  auto service = state.find_service( s.name);

                  if( service)
                  {
                     service->remove( instance.process.pid);
                  }
               }
            }

         } // <unnamed>
      } // local

      void State::remove_process( common::platform::pid::type pid)
      {
         Trace trace{ "broker::State::remove_process"};

         if( forward.pid == pid)
         {
            forward = common::process::Handle{};
         }

         local::remove_process( instances.local, services, pid);
         local::remove_process( instances.remote, services, pid);
      }


      void State::update( common::message::service::Advertise& message)
      {
         Trace trace{ "broker::State::update local"};

         switch( message.directive)
         {
            case common::message::service::Advertise::Directive::add:
            {

               //
               // Local instance
               //

               auto& instance = local::find_or_add( instances.local, message.process);

               for( auto& s : message.services)
               {
                  auto& service = local::find_or_add_service( services, local::transform( std::move( s)));
                  service.add( instance);
               }

               break;
            }
            case common::message::service::Advertise::Directive::remove:
            {
               local::remove_services( *this, instances.local, message.services, message.process);
               break;
            }
            case common::message::service::Advertise::Directive::replace:
            {

               break;
            }
            default:
            {
               log::error << "failed to deduce gateway advertise directive - action: ignore - message: " << message << '\n';
            }
         }
      }


      void State::update( common::message::gateway::domain::Advertise& message)
      {
         Trace trace{ "broker::State::update remote"};

         switch( message.directive)
         {
            case common::message::gateway::domain::Advertise::Directive::add:
            {

               auto& instance = local::find_or_add( instances.remote, message.process);
               instance.order = message.order;

               for( auto& s : message.services)
               {
                  auto hops = s.hops;
                  auto& service = local::find_or_add_service( services, local::transform( std::move( s)));
                  service.add( instance, hops);
               }

               break;
            }
            case common::message::gateway::domain::Advertise::Directive::remove:
            {
               local::remove_services( *this, instances.remote, message.services, message.process);
               break;
            }
            case common::message::gateway::domain::Advertise::Directive::replace:
            {

               break;
            }
            default:
            {
               log::error << "failed to deduce gateway advertise directive - action: ignore - message: " << message << '\n';
            }
         }

      }


      state::Service* State::find_service( const std::string& name)
      {
         auto found = range::find( services, name);

         if( found)
         {
            return &found->second;
         }
         return nullptr;
      }



      void State::connect_broker( std::vector< common::message::service::advertise::Service> services)
      {
         common::message::service::Advertise message;

         message.directive = common::message::service::Advertise::Directive::add;
         message.services = std::move( services);
         message.process = process::handle();

         update( message);
      }



   } // broker

} // casual
