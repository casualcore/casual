//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/state.h"
#include "service/common.h"


#include "common/algorithm.h"
#include "common/algorithm/random.h"
#include "common/environment/normalize.h"
#include "common/process.h"
#include "common/service/type.h"
#include "common/instance.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <functional>
#include <regex>

namespace casual
{
   using namespace common;

   namespace service::manager
   {

      namespace state
      {
         namespace instance
         {
            namespace sequential
            {
               std::string_view description( State value) noexcept
               {
                  switch( value)
                  {
                     case State::busy: return "busy";
                     case State::idle: return "idle";
                  }
                  return "<unknown>";
               }
            } // sequential

            namespace caller
            {
               std::string_view description( Semantic value) noexcept
               {
                  switch( value)
                  {
                     case Semantic::reply: return "reply";
                     case Semantic::no_reply: return "no_reply";
                  }
                  return "<unknown>";
               }
               
            } // caller

            void Sequential::reserve( Caller caller)
            {
               assert( ! m_caller);

               m_caller = std::move( caller);
            }

            service::id::type Sequential::unreserve()
            {
               return std::exchange( m_caller, {}).service;
            }

            void Sequential::discard()
            {
               m_caller = {};
            }

            sequential::State Sequential::state() const
            {
               return m_caller ? sequential::State::busy : sequential::State::idle;
            }

            bool Sequential::service( service::id::type service) const
            {
               return predicate::boolean( algorithm::find( m_services, service));
            }

            void Sequential::add( service::id::type service)
            {
               m_services.push_back( service);
            }

            void Sequential::remove( service::id::type service)
            {
               algorithm::container::erase( m_services, service);
            }

            bool operator < ( const Concurrent& lhs, const Concurrent& rhs)
            {
               return lhs.order < rhs.order;
            }

         } // instance

         namespace service
         {
            bool Instances::add( state::instance::sequential::id::type instance)
            {
               if( algorithm::find( m_sequential, instance))
               {
                  common::log::debug( "instance already known to service - ", instance);
                  return false;
               }

               m_sequential.push_back( instance);
               return m_sequential.size() == 1;
            }

            void Instances::add( state::instance::concurrent::id::type instance, platform::size::type order, instance::concurrent::Property property)
            {
               if( algorithm::find( m_concurrent, instance))
               {
                  common::log::debug( "instance already known to service - ", instance);
                  return;
               }

               m_concurrent.push_back( instances::Concurrent{ 
                  .id = instance,
                  .order = order,
                  .hops = property.hops});

               algorithm::sort( m_concurrent);

               prioritize();
            }

            bool Instances::remove( instance::concurrent::id::type id)
            {
               if( auto found = algorithm::find( m_concurrent, id))
                  algorithm::container::erase( m_concurrent, std::begin( found));

               prioritize();
               return m_sequential.empty() && m_concurrent.empty();
            }

            bool Instances::remove( instance::sequential::id::type id)
            {
               algorithm::container::erase( m_sequential, id);
               return m_sequential.empty() && m_concurrent.empty();
            }

            instance::concurrent::id::type Instances::next_concurrent( std::span< instance::concurrent::id::type> preferred) noexcept
            {
               if( std::empty( m_prioritized_concurrent))
                  return {};

               if( auto found = algorithm::find_first_of( m_prioritized_concurrent, preferred))
                  return found->id;

               return algorithm::random::next( m_prioritized_concurrent)->id;
            }

            void Instances::update_order( state::instance::concurrent::id::type instance, platform::size::type order)
            {
               if( auto found = algorithm::find( m_concurrent, instance))
               {
                  found->order = order;
                  algorithm::stable_sort( m_concurrent);
                  prioritize();
               }
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
             
               std::optional< common::chronology::time_point> Deadline::add( deadline::Entry entry)
               {
                  auto inserted = m_entries.insert(
                     std::upper_bound( std::begin( m_entries), std::end( m_entries), entry),
                     std::move( entry));

                  if( inserted != std::begin( m_entries))
                     return {};

                  return { inserted->when};
               }

               std::optional< deadline::Directive> Deadline::remove( const strong::correlation::id& correlation)
               {
                  Trace trace{ "service::manager::state::pending::Deadline::remove"};

                  if( auto found = algorithm::find( m_entries, correlation))
                  {
                     log::debug( "found: ", *found);

                     auto next = m_entries.erase( std::begin( found));

                     // if no entries left, we unset the deadline
                     if( std::empty( m_entries))
                        return { deadline::Unset{}};

                     // if we erased the first entry, we need to return the new deadline
                     if( next == std::begin( m_entries))
                        return { next->when};
                  }
                  return {};
               }

               std::optional< deadline::Directive> Deadline::remove( instance::sequential::id::type instance)
               {
                  auto has_instance = [ instance]( auto& entry){ return entry.target == instance;};

                  auto [ keep, remove] = algorithm::stable::partition( m_entries, predicate::negate( has_instance));

                  // if we removed nothing, we don't need to do anything more
                  if( std::empty( remove))
                     return {};

                  auto removed_deadline = remove->when;

                  algorithm::container::trim( m_entries, keep);

                  if( std::empty( m_entries))
                     return { deadline::Unset{}};

                  // should we postpone the deadline?
                  if( removed_deadline < m_entries.front().when)
                     return { m_entries.front().when};

                  return {};
               }

               deadline::Entry* Deadline::find_entry( const common::strong::correlation::id& correlation)
               {
                  return algorithm::find( m_entries, correlation).data();
               }

               Deadline::Expired Deadline::expired( common::chronology::time_point now)
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
 


         bool Service::is_discoverable() const noexcept
         {
            return ! visibility || *visibility == common::service::visibility::Type::discoverable;
         }

         bool Service::timeoutable() const noexcept   
         {
            return has_sequential() && timeout.duration > common::chronology::duration::zero();
         }

         
         instance::concurrent::Property Service::property() const noexcept
         {
            if( ! has_sequential() && has_concurrent())
               return { .hops = range::front( instances.concurrent()).hops};
            
            return {};
         }

         state::service::id::type Services::insert( state::Service service)
         {
            // check if we've got the service before
            if( auto id = lookup( service.information.name))
               return id;

            auto name = service.information.name;

            auto id = m_services.insert( std::move( service));
            m_lookup[ name] = id;
            return id;
         }

         state::service::id::type Services::insert( state::Service service, const std::vector< std::string>& routes)
         {
            auto id = [ &]()
            {
               // check if we've got the service by the origin name
               if( auto id = lookup( service.information.name))
                  return id;

               // check if we've got any of the routes before
               for( auto& name : routes)
                  if( auto id = lookup( name))
                     return id;

               // we haven't got it, add it...
               return m_services.insert( std::move( service));
            }();

            for( auto& name : routes)
               m_lookup[ name] = id;

            return id;
         }

         void Services::erase( state::service::id::type id)
         {
            m_services.erase( id);
            
            algorithm::container::erase_if( m_lookup, [ this, id]( auto& pair)
            {
               if( pair.second != id)
                  return false;

               m_metrics.erase( pair.first);
               return true;
            });
         }

         void Services::restore_origin_name( state::service::id::type id)
         {
            algorithm::container::erase_if( m_lookup, [ this, id]( auto& pair)
            {
               if( pair.second != id)
                  return false;

               m_metrics.erase( pair.first);
               return true;
            });

            m_lookup[ m_services[ id].information.name] = id;
         }

         void Services::replace_routes( state::service::id::type id, const std::vector< std::string>& routes)
         {
            // we do a simple remove - add.

            algorithm::container::erase_if( m_lookup, [ this, id]( auto& pair)
            {
               if( pair.second != id)
                  return false;

               m_metrics.erase( pair.first);
               return true;
            });

            for( auto& name : routes)
               m_lookup[ name] = id;
         }
                  
         state::service::id::type Services::lookup( const std::string& name) const
         {
            if( auto found = algorithm::find( m_lookup, name))
               return found->second;

            return {};
         }

         state::service::id::type Services::lookup_origin( const std::string& name) const
         {
            auto is_origin_name = [ this, &name]( auto id)
            {
               return m_services[ id].information.name == name;
            };

            auto ids = m_services.indexes();

            if( auto found = algorithm::find_if( ids, is_origin_name))
               return *found;

            return {};
         }

         std::vector< std::string> Services::names( state::service::id::type id) const
         {
            auto equal_id = [ id]( auto& pair){ return pair.second == id;};
            auto transform_name = []( auto& pair){ return pair.first;};

            return algorithm::transform_if( m_lookup, transform_name, equal_id);
         }

         service::Metric& Services::metric( const std::string& service)
         {
            return m_metrics[ service];
         }

         void Instances::remove_service( instance::sequential::id::type instance_id, service::id::type service_id)
         {
            if( sequential.contains( instance_id))
               sequential[ instance_id].remove( service_id);
         }

         instance::sequential::id::type Instances::find_sequential( common::strong::process::id pid) const
         {
            for( auto id : sequential.indexes())
               if( sequential[ id].process.pid == pid)
                  return id;

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
         directive.read_add( communication::ipc::inbound::device().descriptor());
      }

      bool State::done() const noexcept
      {
         return runlevel > decltype( runlevel())::running;
      }
   
      namespace local
      {
         namespace
         {

            template< typename Service>
            auto transform( const Service& service, configuration::model::service::Timeout timeout)
            {
               return state::Service{
                  .information = { .name = service.name},
                  .timeout = timeout,
                  .transaction = service.transaction,
                  .visibility = service.visibility,
                  .category = service.category
               };
            }

            auto find_or_add_instance( State& state, auto& instances, auto& message)
            {
               if( auto id = instances.lookup( message.process.ipc))
                  return id;

               return instances.emplace( message.process.ipc, message.process);
            }

            void find_or_add_service( State& state, auto& service, casual::configuration::model::service::Timeout timeout, auto instance_id, auto relate_service_and_instance)
            {
               Trace trace{ "service::manager::local::find_or_add_service"};
               
               static auto update_properties = []( auto& service, const auto& advertised)
               {
                  if( ! service.visibility)
                     service.visibility = advertised.visibility;

                  if( ! std::empty( advertised.category))
                     service.category = advertised.category;

                  service.transaction = advertised.transaction;
               };

               auto service_id = [ &]()
               {
                  if( auto found = algorithm::find( state.routes, service.name))
                        return state.services.insert( local::transform( service, timeout), found->second);
                  else
                     return state.services.insert( local::transform( service, timeout));
               }();

               update_properties( state.services[ service_id], service);

               relate_service_and_instance( service_id, instance_id);
            };


            void remove_services( State& state, auto instance_id, const std::vector< std::string>& services)
            {
               Trace trace{ "service::manager::local::remove_services"};

               // unadvertise only comes with origin service names.
               for( auto service_id : state.services.indexes())
               {
                  if( algorithm::find( services, state.services[ service_id].information.name))
                  {
                     state.services[ service_id].instances.remove( instance_id);
                     state.instances.remove_service( instance_id, service_id);
                  }
               }
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

                  log::debug( "advertise.services.add: ", advertise.services.add);
               }
            }


            std::vector< state::instance::Reservation> remove( State& state, auto process)
            {

               auto find_indexes = []( auto& instances, auto process)
               {
                  return algorithm::transform_if( instances.indexes(), std::identity{}, [ &instances, process]( auto id)
                  {
                     return instances[ id].process == process;
                  });
               };

               auto clean_instances_from_services = []( auto& instance_ids, auto& services)
               {
                  for( auto service_id : services.indexes())
                     for( auto instance_id : instance_ids)
                        services[ service_id].instances.remove( instance_id);
               };

               std::vector< state::instance::Reservation> result;

               state.events.remove( process);
               
               {
                  auto concurrent = find_indexes( state.instances.concurrent, process);
                  clean_instances_from_services( concurrent, state.services);
                  
                  for( auto id : concurrent)
                      state.instances.concurrent.erase( id);
               }
               {
                  auto sequential = find_indexes( state.instances.sequential, process);
                  clean_instances_from_services( sequential, state.services);

                  for( auto id : sequential)
                  {
                     if( state.instances.sequential[ id].caller())
                     {
                        auto& instance = state.instances.sequential[ id];
                        result.push_back( { .caller = instance.caller(), .callee = instance.process});
                     }

                     state.instances.sequential.erase( id);
                     std::erase( state.disabled, id);
                  }
               }

               return result;
            }

         } // <unnamed>
      } // local

      state::remove::Result State::remove( common::strong::process::id pid)
      {
         Trace trace{ "service::manager::State::remove"};
         log::debug( "pid: ", pid);

         if( forward.pid == pid)
            forward.clear();

         state::remove::Result result;
      
         if( auto id = instances.find_sequential( pid))
            result.deadline = pending.deadline.remove( id);

         result.reservations = local::remove( *this, pid);

         return result;
      }


      state::remove::Result State::remove( common::strong::ipc::id ipc)
      {
         Trace trace{ "service::manager::State::remove"};
         log::debug( "ipc: ", ipc);

         state::remove::Result result;

         if( auto index = instances.sequential.lookup( ipc))
            result.deadline = pending.deadline.remove( index);

         result.reservations = local::remove( *this, ipc);

         return result;
      }

      state::instance::sequential::id::type State::reserve_sequential( state::instance::Caller caller)
      {
         Trace trace{ "service::manager::State::reserve_sequential"};

         auto is_idle = [ &]( auto instance_id)
         {
            return instances.sequential[ instance_id].idle()
               && ! std::ranges::contains( disabled, instance_id);
         };

         auto& service = services[ caller.service];

         if( auto found = algorithm::find_if( service.instances.sequential(), is_idle))
         {
            instances.sequential[ *found].reserve( std::move( caller));
            return *found;
         }

         return {};
      }
            
      state::instance::concurrent::id::type State::reserve_concurrent( 
         state::service::id::type service_id,
         std::span< state::instance::concurrent::id::type> preferred)
      {
         Trace trace{ "service::manager::State::reserve_concurrent"};

         auto& service = services[ service_id];

         if( auto instance_id = service.instances.next_concurrent( preferred))
            return instance_id;

         return {};
      }

      void State::unreserve( state::instance::sequential::id::type instance_id, const common::message::event::service::Metric& metric)
      {
         Trace trace{ "service::manager::State::unreserve"};

         instances.sequential[ instance_id].unreserve();
         services.metric( metric.service).update( metric);
      }
      
      State::prepare_shutdown_result State::prepare_shutdown( std::vector< common::process::Handle> processes)
      {
         prepare_shutdown_result result;

         auto handle_process = [ &]( const auto& process)
         {
            if( auto instance_id = instances.sequential.lookup( process.ipc))
            {
               // take care of services
               for( auto service_id : instances.sequential[ instance_id].services())
                  if( services[ service_id].instances.remove( instance_id))
                     // This was the last instance for the service, we get the name(s) of the service for lookup-rejects
                     algorithm::container::append( services.names( service_id), result.services);

               result.instances.push_back( instance_id);
            }
            else
            {
               result.unknown.push_back( process);
            }
            
         };

         algorithm::for_each( processes, handle_process);

         return result;
      }

      State::update_result_t State::update( message::service::Advertise&& message)
      {
         Trace trace{ "service::manager::State::update sequential"};

         if( ! message.process)
         {
            log::error( code::casual::internal_unexpected_value, " invalid process ", message.process, " tries to advertise services - action: ignore");
            log::debug( "message: ", message);
            return {};
         }


         if( message.clear())
         {
            // we just remove the process all together.
            std::ignore = remove( message.process.pid);
            return {}; 
         }
         
         // honour possible restriction for the alias
         local::restrict_add_services( *this, message);

         auto instance_id = local::find_or_add_instance( *this, instances.sequential, message);
         instances.sequential[ instance_id].alias = std::move( message.alias);

         // instance might be in "shutdown" mode..
         if( auto found = algorithm::find( disabled, instance_id))
         {
            algorithm::container::erase( disabled, std::begin( found));
            return {};
         }

         State::update_result_t result;

         // add
         {
            auto add_service = [ &]( auto& service)
            {
               auto relate_service_and_instance = [ this, &result]( auto service_id, auto instance_id)
               {
                  instances.sequential[ instance_id].add( service_id);

                  // if the service just been callable and it's discoverable, we need to inform the discovery
                  if( services[ service_id].instances.add( instance_id))
                     result.discoverable = services[ service_id].is_discoverable();
               };

               local::find_or_add_service( *this, service, timeout, instance_id, relate_service_and_instance);
            };

            algorithm::for_each( message.services.add, add_service);
         }

         // remove
         local::remove_services( *this, instance_id, message.services.remove);

         // now we need to check if there are pending request that this instance has enabled
         {
            auto requested_service = [ &, instance_id]( auto& pending)
            {
               if( auto service_id = services.lookup( pending.request.requested))
                  return instances.sequential[ instance_id].service( service_id);
               return false;
            };
 
            if( auto found = algorithm::find_if( pending.lookups, requested_service))
               result.pending.push_back( algorithm::container::extract( pending.lookups, std::begin( found)));
         }

         return result;
      }


      std::vector< state::service::pending::Lookup> State::update( message::service::concurrent::Advertise&& message)
      {
         Trace trace{ "service::manager::State::update concurrent"};
         
         if( ! message.process)
         {
            log::error( code::casual::internal_unexpected_value, " invalid process ", message.process, " tries to advertise services - action: ignore");
            log::debug( "message: ", message);
            return {};
         }

         if( message.directive == decltype( message.directive)::reset)
         {
            // remove the instance and it's associations
            auto removed = remove( message.process.ipc);

            // When we remove a concurrent instance, we do not expect any reservations
            // or any timeouts to be affected. Since callers don't ever wait for concurrent
            // instances, and timeouts are not in play for concurrent instances.
            if( ! std::empty( removed.reservations))
               log::error( code::casual::internal_unexpected_value, "concurrent instance ", message.process.ipc, " had reservations - ", removed.reservations);

            if( removed.deadline)
               log::error( code::casual::internal_unexpected_value, "removed concurrent instance ", message.process.ipc, " resulted in a deadline update");

            // we're removing stuff, no new services can be available.
            return {};
         }

         if( message.directive == decltype( message.directive)::instance)
         {
            auto id = instances.concurrent.lookup( message.process.ipc);

            if( ! id)
               return {};

            instances.concurrent[ id].order = message.order;
            instances.concurrent[ id].alias = message.alias;
            instances.concurrent[ id].description = message.description;

            // We need to go through all services and update order/prio

            for( auto service_id : services.indexes())
               services[ service_id].instances.update_order( id, message.order);

            // nothing added, no new services available.
            return {};
         }


         // honour possible restriction for the alias
         local::restrict_add_services( *this, message);

         auto instance_id = local::find_or_add_instance( *this, instances.concurrent, message);
         instances.concurrent[ instance_id].alias = std::move( message.alias);
         instances.concurrent[ instance_id].order = message.order;
         instances.concurrent[ instance_id].description = std::move( message.description);

         // add
         {
            auto add_service = [&]( auto& service)
            {
               configuration::model::service::Timeout timeout;
               if( service.timeout.duration > common::chronology::duration::zero())
                  timeout.duration = service.timeout.duration;

               auto relate_service_and_instance = [ &]( auto service_id, auto instance_id)
               {
                  services[ service_id].instances.add( instance_id, message.order, service.property);
               };

               local::find_or_add_service( *this, service, timeout, instance_id, relate_service_and_instance);
            };

            algorithm::for_each( message.services.add, add_service);
         }

         // remove
         local::remove_services( *this, instance_id, message.services.remove);

         // find all potentially pending that might be enabled by the new concurrent service(s)
         {
            auto requested_service = [ &]( auto& pending)
            {
               if( auto service_id = services.lookup( pending.request.requested))
                  return services[ service_id].is_concurrent_only();
               return false;
            };

            auto extract = algorithm::stable::filter( pending.lookups, requested_service);

            return algorithm::container::extract( pending.lookups, extract);

         }
      }

      std::vector< state::instance::concurrent::id::type> State::disassociate( common::transaction::global::id::range gtrid)
      {
         Trace trace{ "service::manager::State::disassociate"};
         log::debug( "gtrid: ", gtrid);

         if( auto found = algorithm::find( transaction.associations, gtrid))
            return algorithm::container::extract( transaction.associations, std::begin( found)).second;

         return {};
      }

      std::vector< std::string> State::metric_reset( std::vector< std::string> lookup)
      {
         Trace trace{ "service::manager::State::metric_reset"};

         if( lookup.empty())
         {
            return algorithm::transform( services.m_metrics, []( auto& pair)
            {
               pair.second.reset();
               return pair.first;
            });
         }

         return algorithm::accumulate( lookup, std::vector< std::string>{}, [ &]( auto result, auto& name)
         {
            if( auto found = algorithm::find( services.m_metrics, name))
            {
               found->second.reset();
               result.push_back( found->first);
            }
            return result;
         });
      }


      void State::connect_manager( const std::vector< casual::manager::Service>& services)
      {
         Trace trace{ "service::manager::State::connect_manager"};

         // We advertise to our self
         auto advertise = casual::manager::service::advertise::transform( services);

         if( advertise.sequential)
            (void)State::update( std::move( *advertise.sequential));
         if( advertise.concurrent)
            (void)State::update( std::move( *advertise.concurrent));

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
