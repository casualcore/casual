//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/configuration.h"
#include "domain/manager/state.h"
#include "domain/manager/state/create.h"
#include "domain/manager/transform.h"
#include "domain/manager/task/create.h"

#include "configuration/message.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"



namespace casual
{
   using namespace common;
   namespace domain::manager::configuration
   {
      namespace local
      {
         namespace
         {

            //! @return a tuple with intersected and complement of the configuration (compared to state)
            auto interesection( casual::configuration::Model current, casual::configuration::Model wanted)
            {
               casual::configuration::Model intersection;
               casual::configuration::Model complement;

               auto extract = []( auto& source, auto& lookup, auto predicate, auto& interesected, auto& complemented)
               {
                  auto split = algorithm::intersection( source, lookup, predicate);
                  algorithm::move( std::get< 0>( split), interesected);
                  algorithm::move( std::get< 1>( split), complemented);
               };

               auto alias_equal = []( auto& lhs, auto& rhs){ return lhs.alias == rhs.alias;};

               // take care of servers and executables
               extract( wanted.domain.servers, current.domain.servers, alias_equal, intersection.domain.servers, complement.domain.servers);
               extract( wanted.domain.executables, current.domain.executables, alias_equal, intersection.domain.executables, complement.domain.executables);

               auto name_equal = []( auto& lhs, auto& rhs){ return lhs.name == rhs.name;};

               extract( wanted.domain.groups, current.domain.groups, name_equal, intersection.domain.groups, complement.domain.groups);

               return std::make_tuple( std::move( intersection), std::move( complement));
            }

            namespace task
            {
               auto complement( State& state, const casual::configuration::Model& model)
               {
                  auto servers = transform::alias( model.domain.servers, state.groups);
                  auto executables = transform::alias( model.domain.executables, state.groups);

                  algorithm::append( servers, state.servers);
                  algorithm::append( executables, state.executables);

                  auto task = manager::task::create::scale::aliases( "model put", state::create::boot::order( state, servers, executables));

                  // add, and possible start, the tasks
                  return state.tasks.add( std::move( task));
               }
            } // task

         } // <unnamed>
      } // local

      casual::configuration::Model get( const State& state)
      {
         Trace trace{ "domain::manager::configuration::get"};

         auto futures = algorithm::transform( state.configuration.suppliers, []( auto& process)
         {
            return communication::device::async::call( process.ipc, casual::configuration::message::Request{ common::process::handle()});
         });

         return algorithm::accumulate( futures, casual::domain::manager::transform::model( state), []( auto model, auto& future)
         {
            return model += std::move( future.get( communication::ipc::inbound::device()).model);
         });
      }

      std::vector< common::strong::correlation::id> post( State& state, casual::configuration::Model wanted)
      {
         Trace trace{ "domain::manager::configuration::post"};

         if( state.runlevel() > decltype( state.runlevel())::running)
            return {};

         std::vector< state::dependency::Group> groups;

         auto add_singleton = [&]( auto& id, std::string descripton)
         {
            if( auto handle = state.singleton( id))
               if( auto server = state.server( handle.pid))
               {
                  auto& group = groups.emplace_back();
                  group.description = std::move( descripton);
                  group.servers.push_back( server->id);
               }
         };

         // TODO the order?
         if( state.configuration.model.service != wanted.service)
            add_singleton( communication::instance::identity::service::manager.id, "casual-service-manager");
         if( state.configuration.model.queue != wanted.queue)
            add_singleton( communication::instance::identity::queue::manager.id, "casual-queue-manager");
         if( state.configuration.model.transaction != wanted.transaction)
            add_singleton( communication::instance::identity::transaction::manager.id, "casual-transaction-manager");
         if( state.configuration.model.gateway != wanted.gateway)
            add_singleton( communication::instance::identity::gateway::manager.id, "casual-gateway-manager");

         
         // we use the wanted as our new configuration when 'managers' ask for it.
         state.configuration.model = std::move( wanted);
         
         return { state.tasks.add( manager::task::create::restart::aliases( std::move( groups)))};
      }


      std::vector< common::strong::correlation::id> put( State& state, casual::configuration::Model wanted)
      {
         Trace trace{ "domain::manager::configuration::put"};
         log::line( verbose::log, "model: ", wanted);

         auto interesection = local::interesection( transform::model( state), std::move( wanted));

         log::line( verbose::log, "interesection: ", std::get< 0>( interesection));
         log::line( verbose::log, "complement: ", std::get< 1>( interesection));

         return { local::task::complement( state, std::get< 1>( interesection))};
      }

   } // domain::manager::configuration

} // casual
