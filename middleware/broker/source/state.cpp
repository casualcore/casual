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

         void Instance::lock( const common::platform::time_point& when)
         {
            assert( m_state != State::busy);

            if( m_state == State::idle)
            {
               m_state = State::busy;
            }

            m_last = when;
         }

         void Instance::unlock( const common::platform::time_point& when)
         {
            assert( m_state != State::idle);

            if( m_state == State::busy)
            {
               m_state = State::idle;
            }

            m_last = when;
         }

         namespace service
         {
            void Instance::lock( const common::platform::time_point& when)
            {
               if( ! remote())
               {
                  instance.get().lock( when);
               }
            }

            void Instance::unlock( const common::platform::time_point& when)
            {
               instance.get().unlock( when);
            }

         } // service

         void Service::remove( common::platform::pid::type instance)
         {
            range::trim( m_instances, range::remove( m_instances, instance));
            partition_instances();
         }

         bool Service::has( common::platform::pid::type instance)
         {
            return range::find( m_instances, instance);
         }

         void Service::add( state::Instance& instance, std::size_t hops)
         {
            m_instances.emplace_back( instance, hops);

            partition_instances();
         }

         void Service::partition_instances()
         {
            range::stable_sort( m_instances);

            auto partition = range::divide_if( m_instances, []( const service::Instance& i){
               return i.hops() != 0;
            });

            instances.local = std::get< 0>( partition);
            instances.remote = std::get< 1>( partition);
         }

         service::Instance* Service::idle()
         {
            auto found = range::find_if( instances.local, std::mem_fn( &service::Instance::idle));

            if( found)
            {
               return &(*found);
            }

            if( instances.local.empty() && ! instances.remote.empty())
            {
               return &(*instances.remote);
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

      state::Instance& State::instance( common::platform::pid::type pid)
      {
         return local::get( instances, pid);
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

            template< typename S>
            auto find_or_add( S& services, common::message::service::call::Service&& service) -> decltype( services.at( service.name))
            {
               Trace trace{ "broker::local::find_or_add service"};

               auto found = range::find( services, service.name);

               if( found)
               {
                  return found->second;
               }

               return services.emplace( service.name, std::move( service)).first->second;
            }


            common::message::service::call::Service transform( common::message::service::advertise::Service&& service)
            {
               common::message::service::call::Service result;

               result.name = std::move( service.name);
               result.timeout = service.timeout;
               result.transaction = service.transaction;
               result.type = service.type;

               return result;
            }


            template< typename I, typename S>
            void add_services( State& state, I& instances, S&& services, common::process::Handle process)
            {
               Trace trace{ "broker::local::add_services"};

               auto& instance = local::find_or_add( instances, process);

               for( auto& s : services)
               {
                  auto hops = s.hops;
                  auto& service = find_or_add( state.services, transform( std::move( s)));
                  service.add( instance, hops);
               }
            }

            template< typename I, typename S>
            void remove_services( State& state, I& instances, S& services, common::process::Handle process)
            {
               Trace trace{ "broker::local::remove_services"};

               auto& instance = find_or_add( instances, process);

               for( auto& s : services)
               {
                  auto service = state.find_service( s);

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

         local::remove_process( instances, services, pid);
      }


      void State::add( common::message::service::Advertise& message)
      {
         Trace trace{ "broker::State::add"};

         local::add_services( *this, instances, std::move( message.services), message.process);
      }

      void State::remove( const common::message::service::Unadvertise& message)
      {
         Trace trace{ "broker::State::remove"};

         local::remove_services( *this, instances, message.services, message.process);
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
         local::add_services( *this, instances, std::move( services), common::process::handle());
      }



   } // broker

} // casual
