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
            namespace detail
            {     
               auto transform( auto& service, auto global_timeout)
               {
                  return state::Service{
                     .information = { .name = service.name},
                     .timeout = set_union( global_timeout, service.timeout),
                     .visibility = service.visibility,
                  };
               }

            } // detail


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

               // take care of routes
               {
                  // we replace the routes we've got.
                  state.routes = {};
                  state.reverse_routes = {};

                  auto add_routes = [ &state]( auto& service)
                  {
                     if( service.routes.empty())
                        return;

                     for( auto& route : service.routes)
                        state.reverse_routes.emplace( route, service.name);

                     state.routes.emplace( service.name, service.routes);
                  };

                  algorithm::for_each( change.added, add_routes);
                  algorithm::for_each( change.modified, add_routes);
               }

               auto remove = [ &state]( auto& service)
               {
                  auto origin = state.services.lookup_origin( service.name);

                  if( state.services[ origin].instances.empty())
                  {
                     // we just remove the service
                     state.services.erase( origin);
                     return;
                  }

                  // otherwise we need to keep it but "unconfigure" some properties
                  state.services[ origin].timeout = {};
                  state.services[ origin].visibility = {};

                  state.services.restore_origin_name( origin);
               };

               auto add = [ &state]( auto& service)
               {
                  if( service.routes.empty())
                     state.services.insert( detail::transform( service, state.timeout));
                  else 
                     state.services.insert( detail::transform( service, state.timeout), service.routes);               
               };

               auto modify = [ &state]( auto& service)
               {
                  auto origin = state.services.lookup_origin( service.name);

                  state.services[ origin].timeout = service.timeout;
                  state.services[ origin].visibility = service.visibility;

                  if( service.routes.empty())
                     state.services.restore_origin_name( origin);
                  else
                     state.services.replace_routes( origin, service.routes);
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
         state.restriction = std::move( wanted.restriction);

         local::services( state, std::move( current.services), std::move( wanted.services));
      }

   } // service::manager::configuration 
} // casual