//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/manager/configuration.h"
#include "service/common.h"

#include "configuration/model/change.h"

#include "casual/assert.h"

#include "common/algorithm/coalesce.h"



namespace casual
{
   using namespace common;

   namespace service::manager::configuration
   {
      namespace local
      {
         namespace
         {
            void restrictions( State& state, 
               casual::configuration::model::service::Restriction current, 
               casual::configuration::model::service::Restriction wanted)
            {
               auto change = casual::configuration::model::change::calculate( current.servers, wanted.servers, []( auto& l, auto& r){ return l.alias == r.alias;});
               log::line( verbose::log, "change: ", change);

               state.restriction = std::move( wanted);
            }


            //! tries to conform to updated service configuration.
            //!
            //! @attention This is **way** to complicated, and I don't think it's actually works in all cases.
            //!  We need to make the state and/or the update semantics a lot simpler. 
            //!  I'm convinced that this is possible, but how is not given, and need some thoughts...
            void services( State& state, 
               std::vector< casual::configuration::model::service::Service> current, 
               std::vector< casual::configuration::model::service::Service> wanted)
            {
               auto change = casual::configuration::model::change::calculate( current, wanted, []( auto& l, auto& r){ return l.name == r.name;});
               log::line( verbose::log, "change: ", change);

               auto find_services_with_origin_name = [&state]( auto& name)
               {
                  std::vector< decltype( std::begin( state.services))> result;

                  for( auto current = std::begin( state.services); current != std::end( state.services); ++current)
                     if( current->second.information.name == name)
                        result.push_back( current);

                  return result;
               };

               auto remove = [&]( auto& configuration)
               {
                  auto services = find_services_with_origin_name( configuration.name);

                  auto [ origin, routes] = algorithm::partition( services, []( auto iterator)
                  {
                     return iterator->first == iterator->second.information.name;
                  });

                  
                  // make sure we've got an origin service with no configuration.
                  auto origin_service = [&]( auto& origin)
                  {
                     if( origin)
                     {
                        auto result = &(*origin)->second;
                        result->timeout = {};
                        return result;
                     }

                     state::Service result;
                     result.information.name = configuration.name;

                     auto iterator = std::get< 0>( state.services.emplace( configuration.name, std::move( result)));
                     return &iterator->second;
                  }( origin);


                  algorithm::for_each( routes, [&state, origin_service]( auto iterator)
                  {
                     if( auto found = algorithm::find( state.reverse_routes, iterator->first))
                        state.reverse_routes.erase( std::begin( found));

                     auto service = &iterator->second;

                     service->instances.remove( service, origin_service);

                     origin_service->metric += service->metric;
                      
                     state.services.erase( iterator);
                  });

                  if( auto found = algorithm::find( state.routes, configuration.name))
                     state.routes.erase( std::begin( found));
               };
   
               auto add = [&state]( auto& service)
               {
                  state::Service result;
                  result.information.name = service.name;
                  result.timeout = std::move( service.timeout);
                  result.discoverable = service.discoverable;

                  if( service.routes.empty())
                     state.services.try_emplace( std::move( service.name), std::move( result));
                  else 
                  {
                     for( auto& route : service.routes)
                     {
                        state.services.try_emplace( route, result);
                        state.reverse_routes.emplace( route, service.name);
                     }

                     state.routes.emplace( service.name, std::move( service.routes));
                  }
               };

               auto modify = [&]( auto& configuration)
               {
                  auto services = find_services_with_origin_name( configuration.name);

                  for( auto iterator : services)
                  {
                     iterator->second.timeout = configuration.timeout;
                     iterator->second.discoverable = configuration.discoverable;
                  }

                  auto [ origin, routes] = algorithm::partition( services, []( auto iterator)
                  {
                     return iterator->first == iterator->second.information.name;
                  });

                  auto existing = algorithm::coalesce( origin, routes);
                  casual::assertion( existing, "should be existing services...");

                  auto existing_service = &range::front( existing)->second;
                  
                  // add
                  {
                     auto add = std::get< 1>( algorithm::intersection( configuration.routes, routes, []( auto& name, auto& iterator)
                     {
                        return name == iterator->first;
                     }));


                     for( auto& route : add)
                     {
                        // add the route and use an existing service to keep possible instances.
                        auto& service = state.services.try_emplace( route, *existing_service).first->second;
                        service.metric.reset();
                        state.reverse_routes.emplace( route, configuration.name);
                     }

                     if( auto found = algorithm::find( state.routes, configuration.name))
                        algorithm::append( add, found->second);
                     else 
                        state.routes.emplace( configuration.name, range::to_vector( add));
                  }

                  // remove
                  {
                     auto remove = std::get< 1>( algorithm::intersection( routes, configuration.routes, []( auto& iterator, auto& name)
                     {
                        return name == iterator->first;
                     }));

                     algorithm::for_each( remove, [&state, existing_service]( auto iterator)
                     {
                        if( auto found = algorithm::find( state.reverse_routes, iterator->first))
                           state.reverse_routes.erase( std::begin( found));

                        auto service = &iterator->second;

                        service->instances.remove( service, existing_service);

                        existing_service->metric += service->metric;

                        if( auto found = algorithm::find( state.routes, service->information.name))
                           algorithm::container::trim( found->second, algorithm::remove( found->second, iterator->first));
                        
                        state.services.erase( iterator);

                     });
                  }
                  
               };

               algorithm::for_each( change.removed, remove);
               algorithm::for_each( change.added, add);
               algorithm::for_each( change.modified, modify);

            }
         } // <unnamed>
      } // local

      void conform( State& state, casual::configuration::model::service::Model current, casual::configuration::model::service::Model wanted)
      {
         Trace trace{ "service::manager::configuration::conform"};

         log::line( verbose::log, "current: ", current);
         log::line( verbose::log, "wanted: ", wanted);

         state.timeout = std::move( wanted.global.timeout);

         local::restrictions( state, std::move( current.restriction), std::move( wanted.restriction));
         local::services( state, std::move( current.services), std::move( wanted.services));

      }
   } // service::manager::configuration 
} // casual