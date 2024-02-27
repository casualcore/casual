//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/state.h"
#include "service/common.h"

#include "common/server/service.h"
#include "common/server/lifetime.h"
#include "common/algorithm.h"
#include "common/algorithm/random.h"
#include "common/environment/normalize.h"
#include "common/process.h"
#include "common/service/type.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <functional>
#include <regex>

namespace casual
{
   using namespace common;

   namespace service::manager
   {

      namespace local
      {
         namespace
         {
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
               state::Service* service,
               const common::process::Handle& caller,
               const strong::correlation::id& correlation)
            {
               assert( m_service == nullptr);

               m_service = service;
               m_caller.process = caller;
               m_caller.correlation = correlation;
            }

            void Sequential::unreserve( const common::message::event::service::Metric& metric)
            {
               m_caller = {};

               if( m_service)
               {
                  m_service->metric.update( metric);
                  m_service = nullptr;
               }
               else
                  log::line( log::category::error, code::casual::invalid_semantics, " unexpected unreserve for service: ", metric.service, ", instance: ", process.pid);
            }

            void Sequential::discard()
            {
               m_service = nullptr;
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

            std::vector< std::string> Sequential::detach()
            {
               return algorithm::accumulate( std::exchange( m_services, {}), std::vector< std::string>{}, [&]( auto result, auto service)
               {
                  service->remove( process.ipc);
                  if( service->instances.empty())
                     result.push_back( service->information.name);

                  return result;
               });                  
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

            void Sequential::remove( const state::Service* service, state::Service* replacement)
            {
               algorithm::container::trim( m_services, algorithm::remove( m_services, service));
               if( m_service && m_service == service)
                  m_service = replacement;
            }

            std::string_view description( Sequential::State value)
            {
               switch( value)
               {
                  case Sequential::State::busy: return "busy";
                  case Sequential::State::idle: return "idle";
               }
               return "<unknown>";
            }

            bool operator < ( const Concurrent& lhs, const Concurrent& rhs)
            {
               return lhs.order < rhs.order;
            }

         } // instance

         namespace service
         {

            void Instances::add( state::instance::Concurrent& instance, state::service::instance::Concurrent::Property property)
            {
               if( algorithm::find( m_concurrent, instance.process.ipc))
               {
                  log::line( casual::service::log, "instance already known to service");
                  log::line( verbose::log, "instance: ", instance, ", service: ", *this);
                  return;
               }

               m_concurrent.emplace_back( instance, std::move( property));
               algorithm::sort( m_concurrent);

               prioritize();
            }

            void Instances::remove( const common::strong::ipc::id& ipc, const std::string& service)
            {
               if( auto found = algorithm::find( m_sequential, ipc))
               {
                  found->get().remove( service);
                  m_sequential.erase( std::begin( found));
               }

               algorithm::container::erase( m_concurrent, ipc);
               prioritize();
            }

            void Instances::remove( const common::strong::ipc::id& ipc)
            {
               algorithm::container::erase( m_concurrent, ipc);
               prioritize();
            }
            
            state::instance::Caller Instances::consume( const strong::correlation::id& correlation)
            {
               for( auto& instance : m_sequential)
                  if( auto caller = instance.get().consume( correlation))
                     return caller;

               return {};
            }

            state::service::instance::Concurrent* Instances::next_concurrent( ipc_range_type preferred) noexcept
            {
               if( std::empty( m_prioritized_concurrent))
                  return nullptr;

               if( auto found = algorithm::find_first_of( m_prioritized_concurrent, preferred))
                  return std::data( found);

               return std::data( std::ranges::rotate( m_prioritized_concurrent, std::next( std::begin( m_prioritized_concurrent))));
            }

            void Instances::remove( const state::Service* service, state::Service* replacement)
            {
               for( auto& instance : m_sequential)
                  instance.get().remove( service, replacement);
            }

            void Instances::prioritize() noexcept
            {
               if( m_concurrent.empty())
               {
                  m_prioritized_concurrent = {};
                  return;
               }

               // find the range/end of the most prioritized group/partition.
               m_prioritized_concurrent = std::get< 0>( algorithm::sorted::upper_bound( m_concurrent, range::front( m_concurrent)));
               algorithm::random::shuffle( m_prioritized_concurrent);

            }

            void Metric::update( const common::message::event::service::Metric& metric)
            {
               invoked += metric.duration();
               
               if( metric.pending > decltype( metric.pending)::zero())
                  pending += metric.pending;
               
               last = metric.end;
            }

            Metric& operator += ( Metric& lhs, const Metric& rhs)
            {
               lhs.last = std::max( lhs.last, rhs.last);
               lhs.invoked += rhs.invoked;
               lhs.pending += rhs.pending; 
               lhs.remote += rhs.remote;

               return lhs;
            }

            namespace pending
            {
             
               std::optional< platform::time::point::type> Deadline::add( deadline::Entry entry)
               {
                  auto inserted = m_entries.insert(
                     std::upper_bound( std::begin( m_entries), std::end( m_entries), entry),
                     std::move( entry));

                  if( inserted != std::begin( m_entries))
                     return {};

                  return { inserted->when};
               }

               std::optional< platform::time::point::type> Deadline::remove( const strong::correlation::id& correlation)
               {
                  if( auto found = algorithm::find( m_entries, correlation))
                  {
                     if( auto next = m_entries.erase( std::begin( found)); next == std::begin( m_entries))
                        return { next->when};
                  }
                  return {};
               }

               std::optional< platform::time::point::type> Deadline::remove( const std::vector< strong::correlation::id>& correlations)
               {

                  auto [ keep, remove] = algorithm::stable::partition( m_entries, [&correlations]( auto& entry)
                  {
                     return ! predicate::boolean( algorithm::find( correlations, entry.correlation));
                  });

                  // has the next deadline been postponed?
                  if( remove && keep && range::front( remove) < range::front( keep))
                  {
                     algorithm::container::trim( m_entries, keep);
                     return { m_entries.front().when};
                  }
                  algorithm::container::trim( m_entries, keep);
                  return {};
               }

               Deadline::Expired Deadline::expired( platform::time::point::type now)
               {
                  auto pivot = std::find_if( std::begin( m_entries), std::end( m_entries), [now]( auto& entry)
                  {
                     return entry.when >= now;
                  });

                  Deadline::Expired result;
                  result.entries = algorithm::container::extract( m_entries, range::make( std::begin( m_entries), pivot));

                  if( ! m_entries.empty())
                     result.deadline = m_entries.front().when;

                  return result;
               }
            } // pending
         } // service

         void Service::remove( const common::strong::ipc::id& ipc)
         {
            instances.remove( ipc, information.name);
         }

         void Service::add( state::instance::Sequential& instance)
         {
            if( algorithm::find( instances.m_sequential, instance.process.pid))
            {
               log::line( casual::service::log, "instance already known to service");
               log::line( verbose::log, "instance: ", instance, ", service: ", *this);
               return;
            }

            instances.m_sequential.emplace_back( instance);
            instance.add( *this);
         }

         void Service::add( state::instance::Concurrent& instance, state::service::instance::Concurrent::Property property)
         {
            instances.add( instance, std::move( property));
         }
 
         common::process::Handle Service::reserve_sequential(
            const common::process::Handle& caller, 
            const strong::correlation::id& correlation)
         {
            if( auto found = algorithm::find_if( instances.m_sequential, []( auto& i){ return i.idle();}))
               return found->reserve( this, caller, correlation);

            return {};
         }

         common::process::Handle Service::reserve_concurrent( ipc_range_type preferred)
         {
            if( auto concurrent = instances.next_concurrent( preferred))
            {
               ++metric.remote;
               return concurrent->process();
            }            
            return {};
         }

         bool Service::is_discoverable() const noexcept
         {
            using Enum = decltype( information.visibility);
            if( visibility)
               return visibility == Enum::discoverable;
            return information.visibility == Enum::discoverable;
         }

         bool Service::timeoutable() const noexcept
         {
            return has_sequential() && timeout.duration > platform::time::unit::zero();
         }

        service::instance::Concurrent::Property Service::property() const noexcept
         {
            if( ! has_sequential() && ! instances.concurrent().empty())
               return range::front( instances.concurrent()).property;
            
            return {};
         }


         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
               case Runlevel::error: return "error";
            }
            return "<unknown>";
         }

      } // state

      State::State()
      {
         directive.read.add( communication::ipc::inbound::device().descriptor());
      }

      bool State::done() const noexcept
      {
         return runlevel > decltype( runlevel())::running;
      }

      state::Service* State::service( const std::string& name) noexcept
      {
         if( auto found = algorithm::find( services, name))
            return &found->second;

         return nullptr;
      }

      const state::Service* State::service( const std::string& name) const noexcept
      {
         if( auto found = algorithm::find( services, name))
            return &found->second;

         return nullptr;
      }

      
      namespace local
      {
         namespace
         {
            template< typename I, typename S, typename ID>
            void remove_process( I& instances, S& services, const ID& id)
            {
               Trace trace{ "service::manager::local::remove_process"};
               log::line( verbose::log, "id: ", id);

               algorithm::container::erase_if( instances, [ &id, &services]( auto& pair)
               {
                  if( pair.second.process != id)
                     return false;
                  
                  for( auto& service : services)
                     service.second.remove( pair.second.process.ipc);

                  return true;
               });
            }


            template< typename I>
            auto& find_or_add( I& instances, const common::process::Handle& process)
            {
               Trace trace{ "service::manager::local::find_or_add"};

               if( auto found = algorithm::find( instances, process.ipc))
               {
                  log::line( log, "process found: ", found->second);
                  return found->second;
               }

               return instances.emplace( process.ipc, process).first->second;
            }

            template< typename A>
            void find_or_add_service( State& state, const state::Service& service, A&& add_instance)
            {
               Trace trace{ "service::manager::local::find_or_add service"};

               log::line( log, "service: ", service);

               auto add_service = [&]( auto& name)
               {
                  // is the service advertised before?
                  if( auto found = algorithm::find( state.services, name))
                  {
                     log::line( log, "found: ", *found);


                     // update properties.
                     // TODO: semantics - should we do this? We might degrade the properties

                     auto assign_if_not_equal = []( auto&& source, auto& target)
                     {
                        if( source != target) 
                           target = source;
                     };

                     assign_if_not_equal( service.information.category, found->second.information.category);
                     assign_if_not_equal( service.information.transaction, found->second.information.transaction);

                     // is there config override for visibility?
                     found->second.information.visibility = found->second.visibility ? *found->second.visibility : service.information.visibility;


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

               if( auto found = algorithm::find( state.routes, service.information.name))
               {
                  // service has routes, use them instead
                  algorithm::for_each( found->second, add_service);
               }
               else
                  add_service( service.information.name);
                  
            }


            template< typename Service>
            auto transform( const Service& service, configuration::model::service::Timeout timeout)
            {
               state::Service result;

               result.information.name = service.name;
               result.information.transaction = service.transaction;
               result.information.visibility = service.visibility;
               result.information.category = service.category;
               result.timeout = std::move( timeout);

               return result;
            }

            template< typename I>
            void remove_services( State& state, I& instance, const std::vector< std::string>& services)
            {
               Trace trace{ "service::manager::local::remove_services"};

               // unadvertise only comes with origin service names.
               for( auto& pair : state.services)
                  if( algorithm::find( services, pair.second.information.name))
                     pair.second.remove( instance.process.ipc);
            }

            template< typename M>
            void restrict_add_services( const State& state, M& advertise)
            {
               if( auto found = algorithm::find( state.restriction.servers, advertise.alias))
               {
                  // we transform all regex once.
                  auto expressions = algorithm::transform( found->services, []( auto& expression){ return std::regex{ expression};});

                  // for all added services we match against the expressions, and keep the matched.
                  algorithm::container::trim( advertise.services.add, algorithm::filter( advertise.services.add, [&expressions]( auto& service)
                  {
                     return algorithm::any_of( expressions, [&name = service.name]( auto& expression)
                     {
                        return std::regex_match( name, expression);
                     });
                  }));

                  log::line( verbose::log, "advertise.services.add: ", advertise.services.add);
               }
            }

         } // <unnamed>
      } // local

      void State::remove( common::strong::process::id pid)
      {
         Trace trace{ "service::manager::State::remove"};
         log::line( verbose::log, "pid: ", pid);

         if( forward.pid == pid)
            forward.clear();

         events.remove( pid);

         local::remove_process( instances.sequential, services, pid);
         local::remove_process( instances.concurrent, services, pid);
      }

      void State::remove( common::strong::ipc::id ipc)
      {
         Trace trace{ "service::manager::State::remove"};
         log::line( verbose::log, "ipc: ", ipc);

         events.remove( ipc);

         local::remove_process( instances.sequential, services, ipc);
         local::remove_process( instances.concurrent, services, ipc);
      }
      
      State::prepare_shutdown_result State::prepare_shutdown( std::vector< common::process::Handle> processes)
      {
         prepare_shutdown_result result;

         algorithm::container::trim( processes, algorithm::remove_if( processes, [&]( auto& process)
         {
            if( auto found = common::algorithm::find( instances.sequential, process.ipc))
            {
               algorithm::append_unique( found->second.detach(), std::get< 0>( result));
               std::get< 1>( result).push_back( algorithm::container::extract( instances.sequential, std::begin( found)).second);
               return true;
            }
            else if( common::algorithm::find( instances.concurrent, process.ipc))
               local::remove_process( instances.concurrent, services, process.ipc);

            return false;
         }));

         std::get< 2>( result) = std::move( processes);

         return result;
      }

      std::vector< state::service::pending::Lookup> State::update( common::message::service::Advertise&& message)
      {
         Trace trace{ "service::manager::State::update sequential"};

         if( ! message.process)
         {
            log::line( common::log::category::error, code::casual::internal_unexpected_value, " invalid process ", message.process, " tries to advertise services - action: ignore");
            log::line( verbose::log, "message: ", message);
            return {};
         }

         if( message.clear())
         {
            // we just remove the process all together.
            remove( message.process.pid);
            return {}; 
         }
         
         // honour possible restriction for the alias
         local::restrict_add_services( *this, message);

         auto& instance = local::find_or_add( instances.sequential, message.process);

         // add
         {
            auto add_service = [&]( auto& service)
            {
               auto add_instance = [&instance]( auto& service)
               {
                  service.add( instance);
               };
               local::find_or_add_service( *this, local::transform( service, timeout), add_instance);
            };

            algorithm::for_each( message.services.add, add_service);
         }

         // remove
         local::remove_services( *this, instance, message.services.remove);

         // now we need to check if there are pending request that this instance has enabled
         {
            auto requested_service = [&instance]( auto& pending)
            {
               return instance.service( pending.request.requested);
            };

            if( auto found = algorithm::find_if( pending.lookups, requested_service))
               return { algorithm::container::extract( pending.lookups, std::begin( found))};
         }

         return {};
      }


      std::vector< state::service::pending::Lookup> State::update( common::message::service::concurrent::Advertise&& message)
      {
         Trace trace{ "service::manager::State::update concurrent"};

         if( ! message.process)
         {
            log::line( common::log::category::error, code::casual::internal_unexpected_value, " invalid process ", message.process, " tries to advertise services - action: ignore");
            log::line( verbose::log, "message: ", message);
            return {};
         }

         if( message.reset)
         {
            // remove the instance and it's associations
            remove( message.process.ipc);

            // we're removing stuff, no new services can be available.
            return {};
         }


         // honour possible restriction for the alias
         local::restrict_add_services( *this, message);

         auto& instance = local::find_or_add( instances.concurrent, message.process);
         instance.order = message.order;



         // add
         {
            auto add_service = [&]( auto& service)
            {
               auto add_instance = [&instance, property = service.property]( auto& service)
               {
                  service.add( instance, property);
               };

               configuration::model::service::Timeout timeout;
               if( service.timeout.duration > platform::time::unit::zero())
                  timeout.duration = service.timeout.duration;

               local::find_or_add_service( *this, local::transform( service, timeout), add_instance);
            };

            algorithm::for_each( message.services.add, add_service);
         }

         // remove
         local::remove_services( *this, instance, message.services.remove);

         // find all potentially pending that might be enabled by the new concurrent service(s)
         auto [ keep, remove] = algorithm::stable::partition( pending.lookups, [&]( auto& pending)
         {
            auto is_requested_concurrent_service = [ &]( const auto& service)
            {
               return service.second.has_concurrent() && ! service.second.has_sequential() && service.first == pending.request.requested;
            };

            if( auto found = algorithm::find_if( services, is_requested_concurrent_service))
               return found->second.instances.empty(); // false/remove if not empty, hence what we 'want' to return

            return true; // keep
         });

         return algorithm::container::extract( pending.lookups, remove);
      }

      std::vector< common::strong::ipc::id> State::disassociate( common::transaction::global::id::range gtrid)
      {
         Trace trace{ "service::manager::State::disassociate"};
         log::line( verbose::log, "gtrid: ", gtrid);

         if( auto found = algorithm::find( transaction.associations, gtrid))
            return algorithm::container::extract( transaction.associations, std::begin( found)).second;

         return {};
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
               if( auto service = State::service( name))
               {
                  service->metric.reset();
                  result.push_back( std::move( name));
               }
            }
         }

         return result;
      }

      state::instance::Sequential* State::sequential( common::strong::ipc::id ipc)
      {
         if( auto found = algorithm::find( instances.sequential, ipc))
            return &found->second;
         return nullptr;
      }

      void State::connect_manager( std::vector< common::server::Service> services)
      {
         Trace trace{ "service::manager::State::connect_manager"};

         auto transform_service = []( auto& service)
         {
            common::message::service::advertise::Service result;
            result.category = service.category;
            result.name = service.name;
            result.transaction = service.transaction;
            result.visibility = service.visibility;
            return result;
         };

         auto& instance = local::find_or_add( instances.sequential, process::handle());

         for( auto service : algorithm::transform( services, transform_service))
         {
            local::find_or_add_service( 
               *this, 
               local::transform( service, {}), [&instance]( auto& service) { service.add( instance);});
         }
      }

      void State::Metric::add( metric_type metric)
      {
         m_message.metrics.push_back( std::move( metric));
      }

      void State::Metric::add( std::vector< metric_type> metrics)
      {
         algorithm::move( std::move( metrics), m_message.metrics);
      }

   } // service::manager
} // casual
