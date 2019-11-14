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
                std::enable_if_t< common::traits::is::container::associative::like< M>::value, decltype( map.at( id))>
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
                std::enable_if_t< common::traits::is::container::sequence::like< C>::value, decltype( *std::begin( container))>
               {
                  auto found = common::algorithm::find( container, id);

                  if( found)
                  {
                     return *found;
                  }
                  throw state::exception::Missing{ common::string::compose( "missing id: ", id)};
               }

               namespace equal
               {
                  auto service( const std::string& name)
                  {
                     return [&name]( auto service)
                     {
                        return service->information.name == name;
                     };
                  }
                  
               } // equal

            } // <unnamed>
         } // local


         namespace state
         {

            namespace instance
            {
               void Sequential::reserve(
                  const platform::time::point::type& when, 
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

               state::Service* Sequential::unreserve( const platform::time::point::type& now)
               {
                  assert( state() == State::busy);

                  m_service->metric.update( now, m_last);

                  return std::exchange( m_service, nullptr);
               }

               void Sequential::discard()
               {
                  m_service = nullptr;
                  m_correlation = uuid::empty();
                  m_caller = {};
               }

               Sequential::State Sequential::state() const
               {
                  return m_service == nullptr ? State::idle : State::busy;
               }

               bool Sequential::service( const std::string& name) const
               {
                  return ! algorithm::find_if( m_services, local::equal::service( name)).empty();
               }

               void Sequential::deactivate()
               {
                  algorithm::for_each( m_services, [&]( auto service)
                  {
                     service->remove( process.pid);
                  });
                  m_services.clear();
               }

               void Sequential::add( state::Service& service)
               {
                  m_services.push_back( &service);
               }

               void Sequential::remove( const std::string& name)
               {
                  if( auto found = algorithm::find_if( m_services, local::equal::service( name)))
                     m_services.erase( std::begin( found));
               }

               bool operator < ( const Concurrent& lhs, const Concurrent& rhs)
               {
                  return lhs.order < rhs.order;
               }

               std::ostream& operator << ( std::ostream& out, Sequential::State value)
               {
                  switch( value)
                  {
                     case Sequential::State::busy: return out << "busy";
                     case Sequential::State::idle: return out << "idle";
                  }
                  return out << "<unknown>";
               }

            } // instance

            namespace service
            {
               namespace instance
               {
                  bool operator < ( const Concurrent& lhs, const Concurrent& rhs)
                  {
                     return std::make_tuple( lhs.get(), lhs.hops()) < std::make_tuple( rhs.get(), rhs.hops());
                  }

               } // instance


               void Advertised::remove( common::strong::process::id instance)
               {
                  if( auto found = algorithm::find( instances.sequential, instance))
                  {
                     found->get().remove( information.name);
                     instances.sequential.erase( std::begin( found));
                  }

                  if( auto found = algorithm::find( instances.concurrent, instance))
                  {
                     instances.concurrent.erase( std::begin( found));
                     instances.partition();
                  }
               }

               state::instance::Sequential& Advertised::sequential( common::strong::process::id instance)
               {
                  return local::get( instances.sequential, instance);
               }

            } // service


            void Service::Metric::update( const common::message::event::service::Metric& metric)
            {
               invoked += metric.duration();
               last = metric.end;
            }
            
            void Service::Metric::update( const platform::time::point::type& now, const platform::time::point::type& then)
            {
               last = now;
               invoked += now - then;
            }

            void Service::add( state::instance::Sequential& instance)
            {
               instances.sequential.emplace_back( instance);
               instance.add( *this);
            }

            void Service::add( state::instance::Concurrent& instance, size_type hops)
            {
               instances.concurrent.emplace_back( instance, hops);
               instances.partition();
            }

            common::process::Handle Service::reserve( 
               const platform::time::point::type& now, 
               const common::process::Handle& caller, 
               const common::Uuid& correlation)
            {
               if( auto found = algorithm::find_if( instances.sequential, []( auto& i){ return i.idle();}))
               {
                  found->reserve( now, this, caller, correlation);
                  return found->process();
               }

               if( instances.sequential.empty() && ! instances.concurrent.empty())
               {
                  ++metric.remote;
                  return instances.concurrent.front().process();
               }
               return {};
            }
         } // state

         State::State( common::message::domain::configuration::service::Manager configuration)
            : default_timeout{ configuration.default_timeout}
         {
            Trace trace{ "service::manager::State::State"};

            auto add_service = [&]( auto& service)
            {
               common::message::service::call::Service message;
               message.timeout = service.timeout;
               message.name = service.name;

               if( service.routes.empty())
                  services.emplace( service.name, std::move( message));
               else 
               {
                  for( auto& route : service.routes)
                     services.emplace( route, message);

                  routes.emplace( service.name, std::move( service.routes));
               }
            };

            algorithm::for_each( configuration.services, add_service);

         }

         state::Service& State::service( const std::string& name)
         {
            return local::get( services, name);
         }

         namespace local
         {
            namespace
            {
               template< typename I, typename S>
               void remove_process( I& instances, S& services, common::strong::process::id pid)
               {
                  Trace trace{ "service::manager::local::remove_process"};

                  log::line( verbose::log, "pid: ", pid);

                  if( auto found = common::algorithm::find( instances, pid))
                  {
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

               template< typename A>
               void find_or_add_service( State& state, const common::message::service::call::Service& service, A&& add_instance)
               {
                  Trace trace{ "service::manager::local::find_or_add service"};

                  log::line( log, "service: ", service);

                  auto add_service = [&]( auto& name)
                  {
                     // is the service advertised before?
                     if( auto found = algorithm::find( state.services, name))
                     {
                        // update properties.
                        // TODO: semantics - should we do this? We might degrade the properties

                        auto asssign_if_not_equal = []( auto& target, auto&& source)
                        {
                           if( target != source) target = source;
                        };

                        asssign_if_not_equal( found->second.information.category, service.category);
                        asssign_if_not_equal( found->second.information.transaction, service.transaction);

                        add_instance( found->second);
                        log::line( verbose::log, "existing service: ", found->second);
                     }
                     else 
                     {
                        // a new service (to us), add to advertised AND services
                        auto& advertised = state.services.emplace( name, service).first->second;
                        add_instance( advertised);
                        log::line( verbose::log, "new service: ", advertised);
                     }
                  };

                  if( auto found = algorithm::find( state.routes, service.name))
                  {
                     // service has routes, use them instead
                     algorithm::for_each( found->second, add_service);
                  }
                  else
                     add_service( service.name);
                     
               }


               template< typename Service>
               common::message::service::call::Service transform( const Service& service, platform::time::unit timeout)
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

         void State::remove( common::strong::process::id pid)
         {
            Trace trace{ "service::manager::State::remove"};
            log::line( verbose::log, "pid: ", pid);

            if( forward.pid == pid)
               forward = common::process::Handle{};

            events.remove( pid);

            local::remove_process( instances.sequential, services, pid);
            local::remove_process( instances.concurrent, services, pid);
         }

         void State::deactivate( common::strong::process::id pid)
         {
            Trace trace{ "service::manager::State::deactivate"};
            log::line( verbose::log, "pid: ", pid);

            if( auto found = common::algorithm::find( instances.sequential, pid))
               found->second.deactivate();
            else
            {
               // Assume that it's a remote instances
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

                  auto add_service = [&]( auto& service)
                  {
                     auto add_instance = [&instance]( auto& service)
                     {
                        service.add( instance);
                     };
                     local::find_or_add_service( *this, local::transform( service, default_timeout), add_instance);
                  };

                  algorithm::for_each( message.services, add_service);

                  break;
               }
               case common::message::service::Advertise::Directive::remove:
               {
                  local::remove_services( *this, instances.sequential, message.services, message.process);
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

                  auto add_service = [&]( auto& service)
                  {
                     auto add_instance = [&instance, hops = service.hops]( auto& service)
                     {
                        service.add( instance, hops);
                     };
                     local::find_or_add_service( *this, local::transform( service), add_instance);
                  };

                  algorithm::for_each( message.services, add_service);

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
                  service.second.metric.reset();
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
                     service->metric.reset();
                     result.push_back( std::move( name));
                  }
               }
            }

            return result;
         }

         state::instance::Sequential& State::sequential( common::strong::process::id pid)
         {
            return local::get( instances.sequential, pid);
         }

         state::Service* State::find_service( const std::string& name)
         {
            auto found = algorithm::find( services, name);

            if( found)
               return &found->second;

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

         
         void State::Metric::add( metric_type metric)
         {
            m_message.metrics.push_back( std::move( metric));
         }
         void State::Metric::add( std::vector< metric_type> metrics)
         {
            algorithm::move( std::move( metrics), m_message.metrics);
         }
      } // manager
   } // service
} // casual
