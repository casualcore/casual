//!
//! casual
//!

#include "broker/state.h"
#include "broker/transform.h"
#include "broker/common.h"

#include "common/server/service.h"
#include "common/server/lifetime.h"
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
            auto get( M& map, ID&& id) ->
             common::traits::enable_if_t< common::traits::container::is_associative< M>::value, decltype( map.at( id))>
            {
               auto found = common::range::find( map, id);

               if( found)
               {
                  return found->second;
               }
               throw state::exception::Missing{ "missing", CASUAL_NIP( id)};
            }

            template< typename C, typename ID>
            auto get( C& container, ID&& id) ->
             common::traits::enable_if_t< common::traits::container::is_sequence< C>::value, decltype( *std::begin( container))>
            {
               auto found = common::range::find( container, id);

               if( found)
               {
                  return *found;
               }
               throw state::exception::Missing{ "missing", CASUAL_NIP( id)};
            }

         } // <unnamed>
      } // local


      namespace state
      {

         namespace instance
         {
            void Local::reserve( const common::platform::time::point::type& when, state::Service* service)
            {
               assert( m_service == nullptr);

               m_service = service;

               m_last = when;
            }

            state::Service* Local::unreserve( const common::platform::time::point::type& now)
            {
               assert( m_service != nullptr);

               auto result = m_service;
               m_service = nullptr;

               result->unreserve( now, m_last);

               return result;
            }

            namespace local
            {
               namespace
               {

                  Service global;

                  Service* exiting_address()
                  {
                     return &global;
                  }
               } // <unnamed>
            } // local

            Local::State Local::state() const
            {
               if( m_service == nullptr)
                  return State::idle;

               return m_service == local::exiting_address() ? State::exiting : State::busy;
            }

            void Local::exiting()
            {
               m_service = local::exiting_address();
            }

            bool operator < ( const Remote& lhs, const Remote& rhs)
            {
               return lhs.order < rhs.order;
            }

         } // instance

         namespace service
         {
            void Metric::add( const common::platform::time::point::type::duration& duration)
            {
               ++m_count;
               m_total += std::chrono::duration_cast< std::chrono::microseconds>( duration);
            }

            void Metric::reset()
            {
               m_count = 0;
               m_total = std::chrono::microseconds::zero();
            }


            namespace instance
            {
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

         state::instance::Local& Service::local( common::platform::pid::type instance)
         {
            return local::get( instances.local, instance);
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


         void Service::unreserve( const common::platform::time::point::type& now, const common::platform::time::point::type& then)
         {
            m_last = now;
            metric.add( now - then);
         }

         void Service::partition_remote_instances()
         {
            range::stable_sort( instances.remote);
         }


         bool Service::Instances::active() const
         {
            return ! remote.empty() || range::any_of( local, []( const auto& i){
               return i.state() != instance::Local::State::exiting;
            });
         }

         void Service::metric_reset()
         {
            metric.reset();
            pending.reset();
         }


         common::process::Handle Service::reserve( const common::platform::time::point::type& now)
         {
            auto found = range::find_if( instances.local, std::mem_fn( &service::instance::Local::idle));

            if( found)
            {
               found->reserve( now, this);
               return found->process();
            }

            if( instances.local.empty() && ! instances.remote.empty())
            {
               ++m_remote_invocations;
               return instances.remote.front().process();
            }
            return {};
         }

         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ name: " << service.information.name
                  << ", type: " << service.information.category
                  << ", transaction: " << service.information.transaction
                  << ", timeout: " << service.information.timeout.count()
                  << "}";
          }

      } // state



      state::Service& State::service( const std::string& name)
      {
         return local::get( services, name);
      }

      std::vector< common::platform::ipc::id::type> State::subscribers() const
      {
         return range::transform( events.event< common::message::event::service::Call>().subscribers(), []( auto& v){
            return v.queue;
         });
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
            auto& find_or_add( I& instances, common::process::Handle process)
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
            auto& find_or_add_service( Services& services, Service&& service)
            {
               Trace trace{ "broker::local::find_or_add service"};

               log << "service: " << service << '\n';

               auto found = range::find( services, service.name);

               if( found)
               {
                  if( found->second.information.category != service.category)
                     found->second.information.category = service.category;

                  if( found->second.information.transaction != service.transaction)
                     found->second.information.transaction = service.transaction;

                  log << "found: " << found->second << '\n';

                  return found->second;
               }

               return services.emplace( service.name, service).first->second;
            }



            template< typename Service>
            common::message::service::call::Service transform( const Service& service, std::chrono::microseconds timeout)
            {
               common::message::service::call::Service result;

               result.name = service.name;
               result.timeout = timeout;
               result.transaction = service.transaction;
               result.category = service.category;

               return result;
            }

            template< typename Service>
            common::message::service::call::Service transform( const Service& service)
            {
               common::message::service::call::Service result;

               result.name = service.name;
               result.timeout = service.timeout;
               result.transaction = service.transaction;
               result.category = service.category;

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

         events.remove( pid);

         local::remove_process( instances.local, services, pid);
         local::remove_process( instances.remote, services, pid);
      }

      void State::prepare_shutdown( common::platform::pid::type pid)
      {
         Trace trace{ "broker::State::prepare_shutdown"};

         auto found = common::range::find( instances.local, pid);

         if( found)
         {
            found->second.exiting();
         }
         else
         {
            //
            // Assume that it's a remote instances
            //
            local::remove_process( instances.remote, services, pid);
         }
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
                  auto& service = local::find_or_add_service( services, local::transform( s, default_timeout));
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
               log::category::error << "failed to deduce gateway advertise directive - action: ignore - message: " << message << '\n';
               break;
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
                  auto& service = local::find_or_add_service( services, local::transform( s));
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
               log::category::error << "failed to deduce gateway advertise directive - action: ignore - message: " << message << '\n';
            }
         }

      }

      state::instance::Local& State::local( common::platform::pid::type pid)
      {
         return local::get( instances.local, pid);
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



      void State::connect_broker( std::vector< common::server::Service> services)
      {
         common::message::service::Advertise message;

         message.directive = common::message::service::Advertise::Directive::add;

         range::transform( services, message.services, []( common::server::Service& s){
            common::message::service::advertise::Service result;

            result.category = s.category;
            result.name = s.name;
            result.transaction = s.transaction;

            return result;
         });

         message.process = process::handle();

         update( message);
      }



   } // broker

} // casual
