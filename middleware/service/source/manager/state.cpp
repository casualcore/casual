//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/state.h"
#include "service/transform.h"
#include "service/common.h"

#include "common/server/service.h"
#include "common/server/lifetime.h"
#include "common/algorithm.h"

#include "common/process.h"

// std
#include <functional>

namespace casual
{
   namespace service
   {
      using namespace common;

      namespace manager
      {
         namespace local
         {
            namespace
            {
               template< typename M, typename ID>
               auto get( M& map, ID&& id) ->
                std::enable_if_t< common::traits::container::is_associative< M>::value, decltype( map.at( id))>
               {
                  auto found = common::algorithm::find( map, id);

                  if( found)
                  {
                     return found->second;
                  }
                  throw state::exception::Missing{ common::string::compose( "missing id: ", id)};
               }

               template< typename C, typename ID>
               auto get( C& container, ID&& id) ->
                std::enable_if_t< common::traits::container::is_sequence< C>::value, decltype( *std::begin( container))>
               {
                  auto found = common::algorithm::find( container, id);

                  if( found)
                  {
                     return *found;
                  }
                  throw state::exception::Missing{ common::string::compose( "missing id: ", id)};
               }

            } // <unnamed>
         } // local


         namespace state
         {

            namespace instance
            {
               void Sequential::reserve(
                  const common::platform::time::point::type& when, 
                  state::Service* service,
                  const common::process::Handle& caller,
                  const common::Uuid& correlation)
               {
                  assert( m_service == nullptr);

                  m_service = service;
                  m_last = when;
                  m_caller = caller;
                  m_correlation = correlation;
               }

               state::Service* Sequential::unreserve( const common::platform::time::point::type& now)
               {
                  assert( m_service != nullptr);

                  m_service->unreserve( now, m_last);

                  return std::exchange( m_service, nullptr);
               }

               void Sequential::discard()
               {
                  m_service = nullptr;
                  m_correlation = uuid::empty();
                  m_caller = {};
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

                     namespace view
                     {
                        auto compare = []( const std::string& lhs, const std::string& rhs){ return lhs < rhs;};
                     } // view
                  } // <unnamed>
               } // local

               Sequential::State Sequential::state() const
               {
                  if( m_service == nullptr)
                     return State::idle;

                  return m_service == local::exiting_address() ? State::exiting : State::busy;
               }

               void Sequential::exiting()
               {
                  m_service = local::exiting_address();
               }


               bool Sequential::service( const std::string& name) const
               {
                  return algorithm::sorted::search( m_services, name, local::view::compare);
               }


               void Sequential::add( const state::Service& service)
               {
                  m_services.emplace_back( service.information.name);
                  algorithm::sort( m_services, local::view::compare);
               }

               void Sequential::remove( const std::string& service)
               {
                  auto found = algorithm::find_if( m_services, [&service]( auto& s){ return s.get() == service;});

                  if( found)
                     m_services.erase( std::begin( found));
               }


               bool operator < ( const Concurrent& lhs, const Concurrent& rhs)
               {
                  return lhs.order < rhs.order;
               }

               std::ostream& operator << ( std::ostream& out, const Concurrent& value)
               {
                  return out << "{ process: " << value.process
                     << ", order: " << value.order
                     << '}';
               }

               std::ostream& operator << ( std::ostream& out, Sequential::State value)
               {
                  switch( value)
                  {
                     case Sequential::State::busy: return out << "busy";
                     case Sequential::State::idle: return out << "idle";
                     case Sequential::State::exiting: return out << "exiting";
                  }
                  return out << "unknown";
               }

               std::ostream& operator << ( std::ostream& out, const Sequential& value)
               {
                  auto state = value.state();

                  out << "{ state: " << state
                     << ", process: " << value.process;

                  if( state == Sequential::State::busy)
                  {
                     out  << ", service: " << *value.m_service
                        << ", correlation: " << value.correlation()
                        << ", caller: " << value.caller();
                  }

                  return out << '}';
               }

            } // instance

            namespace service
            {

               std::ostream& operator << ( std::ostream& out, const Pending& value)
               {
                  return out << "{ request: " << value.request
                     << ", when: " << value.when.time_since_epoch().count()
                     << '}';
               }



               namespace instance
               {
                  bool operator < ( const Concurrent& lhs, const Concurrent& rhs)
                  {
                     return std::make_tuple( lhs.get(), lhs.hops()) < std::make_tuple( rhs.get(), rhs.hops());
                  }

               } // instance

            } // service

            void Service::remove( common::strong::process::id instance)
            {
               {
                  auto found = algorithm::find( instances.sequential, instance);

                  if( found)
                  {
                     found->get().remove( information.name);
                     instances.sequential.erase( std::begin( found));
                  }
               }

               {
                  auto found = algorithm::find( instances.concurrent, instance);

                  if( found)
                  {
                     instances.concurrent.erase( std::begin( found));
                     partition_remote_instances();
                  }
               }
            }

            state::instance::Sequential& Service::local( common::strong::process::id instance)
            {
               return local::get( instances.sequential, instance);
            }



            void Service::add( state::instance::Sequential& instance)
            {
               instances.sequential.emplace_back( instance);
               instance.add( *this);
            }

            void Service::add( state::instance::Concurrent& instance, size_type hops)
            {
               instances.concurrent.emplace_back( instance, hops);
               partition_remote_instances();
            }


            void Service::unreserve( const common::platform::time::point::type& now, const common::platform::time::point::type& then)
            {
               m_last = now;
               metric += now - then;
            }

            void Service::partition_remote_instances()
            {
               algorithm::stable_sort( instances.concurrent);
            }


            bool Service::Instances::active() const
            {
               return ! concurrent.empty() || algorithm::any_of( sequential, []( const auto& i){
                  return i.state() != instance::Sequential::State::exiting;
               });
            }

            void Service::metric_reset()
            {
               metric = {};
               pending = {};
               m_remote_invocations = 0;
               m_last = common::platform::time::point::type::min();
            }

            common::process::Handle Service::reserve( 
               const common::platform::time::point::type& now, 
               const common::process::Handle& caller, 
               const common::Uuid& correlation)
            {
               auto found = algorithm::find_if( instances.sequential, std::mem_fn( &service::instance::Sequential::idle));

               if( found)
               {
                  found->reserve( now, this, caller, correlation);
                  return found->process();
               }

               if( instances.sequential.empty() && ! instances.concurrent.empty())
               {
                  ++m_remote_invocations;
                  return instances.concurrent.front().process();
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

         std::vector< common::strong::ipc::id> State::subscribers() const
         {
            return algorithm::transform( events.event< common::message::event::service::Call>().subscribers(), []( auto& v){
               return v.ipc;
            });
         }



         namespace local
         {
            namespace
            {
               template< typename I, typename S>
               void remove_process( I& instances, S& services, common::strong::process::id pid)
               {
                  Trace trace{ "service::manager::local::remove_process"};

                  auto found = common::algorithm::find( instances, pid);

                  if( found)
                  {
                     log::line( log, "remove process pid: ", pid);

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
                  Trace trace{ "service::manager::local::find_or_add"};

                  auto found = algorithm::find( instances, process.pid);

                  if( found)
                  {
                     log::line( log, "process found: ", found->second);
                     return found->second;
                  }

                  return instances.emplace( process.pid, process).first->second;
               }

               template< typename Services, typename Service>
               auto& find_or_add_service( Services& services, Service&& service)
               {
                  Trace trace{ "service::manager::local::find_or_add service"};

                  log::line( log, "service: ", service);

                  auto found = algorithm::find( services, service.name);

                  if( found)
                  {
                     if( found->second.information.category != service.category)
                        found->second.information.category = service.category;

                     if( found->second.information.transaction != service.transaction)
                        found->second.information.transaction = service.transaction;

                     log::line( log, "found: ", found->second);

                     return found->second;
                  }

                  return services.emplace( service.name, service).first->second;
               }



               template< typename Service>
               common::message::service::call::Service transform( const Service& service, common::platform::time::unit timeout)
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
                  Trace trace{ "service::manager::local::remove_services"};

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

         void State::remove_process( common::strong::process::id pid)
         {
            Trace trace{ "service::manager::State::remove_process"};

            if( forward.pid == pid)
            {
               forward = common::process::Handle{};
            }

            events.remove( pid);

            local::remove_process( instances.sequential, services, pid);
            local::remove_process( instances.concurrent, services, pid);
         }

         void State::prepare_shutdown( common::strong::process::id pid)
         {
            Trace trace{ "service::manager::State::prepare_shutdown"};

            auto found = common::algorithm::find( instances.sequential, pid);

            if( found)
            {
               found->second.exiting();
            }
            else
            {
               //
               // Assume that it's a remote instances
               //
               local::remove_process( instances.concurrent, services, pid);
            }
         }

         void State::update( common::message::service::Advertise& message)
         {
            Trace trace{ "service::manager::State::update local"};

            if( ! message.process)
            {
               log::line( common::log::category::error, "invalid process ", message.process, " tries to advertise services - action: ignore");
               log::line( verbose::log, "message: ", message);
               return;
            }

            switch( message.directive)
            {
               case common::message::service::Advertise::Directive::add:
               {
                  auto& instance = local::find_or_add( instances.sequential, message.process);

                  for( auto& s : message.services)
                  {
                     auto& service = local::find_or_add_service( services, local::transform( s, default_timeout));
                     service.add( instance);
                  }

                  break;
               }
               case common::message::service::Advertise::Directive::remove:
               {
                  local::remove_services( *this, instances.sequential, message.services, message.process);
                  break;
               }
               case common::message::service::Advertise::Directive::replace:
               {

                  break;
               }
               default:
               {
                  log::line( log::category::error, "failed to deduce gateway advertise directive - action: ignore");
                  log::line( verbose::log, "message: ", message);
                  break;
               }
            }
         }


         void State::update( common::message::service::concurrent::Advertise& message)
         {
            Trace trace{ "service::manager::State::update remote"};

            if( ! message.process)
            {
               log::line( common::log::category::error, "invalid process ", message.process, " tries to advertise services - action: ignore");
               log::line( verbose::log, "message: ", message);
               return;
            }

            using Directive = common::message::service::concurrent::Advertise::Directive;

            switch( message.directive)
            {
               case Directive::add:
               {

                  auto& instance = local::find_or_add( instances.concurrent, message.process);
                  instance.order = message.order;

                  for( auto& s : message.services)
                  {
                     auto hops = s.hops;
                     auto& service = local::find_or_add_service( services, local::transform( s));
                     service.add( instance, hops);
                  }

                  break;
               }
               case Directive::remove:
               {
                  local::remove_services( *this, instances.concurrent, message.services, message.process);
                  break;
               }
               default:
               {
                  log::line( log::category::error, "failed to deduce gateway advertise directive - action: ignore");
                  log::line( log::category::verbose::error, "message: ", message);
               }
            }
         }

         std::vector< std::string> State::metric_reset( std::vector< std::string> lookup)
         {
            Trace trace{ "service::manager::State::metric_reset"};

            std::vector< std::string> result;

            if( lookup.empty())
            {
               for( auto& service : services)
               {
                  service.second.metric_reset();
                  result.push_back( service.first);
               }
            }
            else
            {
               for( auto& name : lookup)
               {
                  auto service = find_service( name);
                  if( service)
                  {
                     service->metric_reset();
                     result.push_back( std::move( name));
                  }
               }
            }

            return result;
         }

         state::instance::Sequential& State::local( common::strong::process::id pid)
         {
            return local::get( instances.sequential, pid);
         }

         state::Service* State::find_service( const std::string& name)
         {
            auto found = algorithm::find( services, name);

            if( found)
            {
               return &found->second;
            }
            return nullptr;
         }



         void State::connect_manager( std::vector< common::server::Service> services)
         {
            common::message::service::Advertise message;

            message.directive = common::message::service::Advertise::Directive::add;

            algorithm::transform( services, message.services, []( common::server::Service& s){
               common::message::service::advertise::Service result;

               result.category = s.category;
               result.name = s.name;
               result.transaction = s.transaction;

               return result;
            });

            message.process = process::handle();

            update( message);
         }
      } // manager
   } // service
} // casual
