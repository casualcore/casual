//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/manager/configuration.h"
#include "transaction/manager/state.h"

#include "configuration/system.h"
#include "configuration/model/change.h"

#include "common/environment/expand.h"
#include "common/environment.h"
#include "common/file.h"
#include "common/event/send.h"
#include "common/instance.h"
#include "common/algorithm/coalesce.h"
#include "common/algorithm/container.h"


namespace casual
{
   using namespace common;

   namespace transaction::manager::configuration
   {
      namespace local
      {
         namespace
         {
            void log( State& state, const casual::configuration::model::transaction::Model& model)
            {
               // Can't change log path runtime
               if( state.persistent.log)
                  return;

               auto initialize_log = []( std::string configuration) -> std::filesystem::path
               {
                  Trace trace{ "transaction::manager::configuration::local::initialize_log"};

                  if( ! configuration.empty())
                     return common::environment::expand( std::move( configuration));

                  auto file = directory::create( environment::directory::transaction()) / "log.db";

                  // TODO: remove this in 2.0 (that exist to be backward compatible)
                  {
                     // if the wanted path exists, we can't overwrite with the old
                     if( std::filesystem::exists( file))
                        return file;

                     auto old = environment::directory::domain() / "transaction" / "log.db";
                     if( std::filesystem::exists( old))
                     {
                        std::filesystem::rename( old, file);
                        event::notification::send( "transaction log file moved: ", std::filesystem::relative( old), " -> ", std::filesystem::relative( file));
                        log::line( log::category::warning, "transaction log file moved: ", old, " -> ", file);
                     }
                  }

                  return file;
               };

               state.persistent.log = Log{ initialize_log( model.log)};
            }
            
            void system( State& state, casual::configuration::model::system::Model model)
            {
               if( model)
               {
                  if( model != state.system.configuration)
                     state.system.configuration = std::move( model);
               }
               else if( auto model = casual::configuration::system::get())
               {
                  if( model != state.system.configuration)
                     state.system.configuration = std::move( model);
               }
            }

            std::optional< state::resource::proxy::Instance> spawn_instance( State& state, const state::resource::Proxy& proxy)
            {
               auto system = algorithm::find( state.system.configuration.resources, proxy.configuration.key);

               if( ! system)
                  return {};

               try
               {
                  auto pid = process::spawn(
                     system->server,
                     {
                        "--id", std::to_string( proxy.id.value()),
                     },
                     { common::instance::variable( { proxy.configuration.name, proxy.id.value()})}
                  );

                  return state::resource::proxy::Instance{ proxy.id, pid};
               }
               catch( ...)
               {
                  common::event::error::send( common::exception::capture().code(), "failed to spawn resource-proxy-instance: " + system->server);
               }
               return {};
            }

            auto exit_handle( auto shared)
            {
               return [ shared]( task::unit::id, const common::message::event::process::Exit& event)
               {
                  algorithm::container::erase( shared->pids, event.state.pid);
                  log::line( verbose::log, "pids left: ", shared->pids);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };
            }
  

            task::Unit scale_in_resource( State& state, common::strong::resource::id id)
            {
               Trace trace{ "transaction::manager::configuration::local::scale_in_resource"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto action = [ &state, id, shared]( task::unit::id)
               {
                  Trace trace{ "transaction::manager::configuration::local::scale_in_resource action"};
                  log::line( verbose::log, "id: ", id);

                  auto proxy = state.find_resource( id);

                  if( ! proxy)
                     return task::unit::action::Outcome::abort;

                  auto shutdownable = proxy->shutdownable();

                  log::line( verbose::log, "shutdownable: ", shutdownable);

                  if( std::empty( shutdownable))
                     return task::unit::action::Outcome::abort;

                  for( auto& instance : shutdownable)
                  {
                     instance.state( state::resource::proxy::instance::State::shutdown);
                     shared->pids.push_back( instance.process.pid);

                     if( instance.process.ipc)
                        state.multiplex.send( instance.process.ipc, message::shutdown::Request{ common::process::handle()});
                     else
                        signal::send( instance.process.pid, common::code::signal::terminate);
                  }

                  return task::unit::action::Outcome::success;
               };

               return task::create::unit(
                  task::create::action( "scale_in_resource", std::move( action)),
                  exit_handle( shared)
               );

            }


            task::Unit scale_out_resource( State& state, common::strong::resource::id id)
            {
               Trace trace{ "transaction::manager::configuration::local::scale_out_resource"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto action = [ &state, id, shared]( task::unit::id)
               {
                  Trace trace{ "transaction::manager::configuration::local::scale_out_resource action"};

                  auto proxy = state.find_resource( id);

                  if( ! proxy)
                     return task::unit::action::Outcome::abort;

                  auto spawned = algorithm::generate_n( std::max( 0l, proxy->scale_difference()),[ &state, proxy]()
                  {
                     return spawn_instance( state, *proxy);
                  });

                  if( std::empty( spawned))
                     return task::unit::action::Outcome::abort;

                  shared->pids = algorithm::transform( spawned, []( auto& instance){ return instance.process.pid;});

                  algorithm::move( spawned, std::back_inserter( proxy->instances));

                  return task::unit::action::Outcome::success;
               };

               auto instance_ready = [ shared]( task::unit::id, const common::message::transaction::resource::Ready& message)
               {
                  Trace trace{ "transaction::manager::configuration::local::scale_out_resource instance_ready"};

                  algorithm::container::erase( shared->pids, message.process.pid);
                  log::line( verbose::log, "pids left: ", shared->pids);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };

               return task::create::unit(
                  task::create::action( "scale_out_resource", std::move( action)),
                  std::move( instance_ready),
                  exit_handle( shared)
               );

            }

            task::Unit restart_resource( State& state, common::strong::resource::id id, std::vector< strong::process::id> restarts)
            {
               Trace trace{ "transaction::manager::configuration::local::restart_resource"};
               log::line( verbose::log, "id: ", id);

               struct Shared
               {
                  bool done() const 
                  {
                     return restarts.empty() && spawned.empty();
                  }

                  std::vector< strong::process::id> restarts;
                  std::vector< strong::process::id> spawned;
               };

               auto shared = std::make_shared< Shared>();
               shared->restarts = std::move( restarts);

               auto action = [ &state, id, shared]( task::unit::id)
               {
                  Trace trace{ "transaction::manager::configuration::local::restart_resource action"};

                  auto proxy = state.find_resource( id);

                  if( ! proxy)
                     return task::unit::action::Outcome::abort;

                  auto instance = algorithm::find_first_of( proxy->instances, shared->restarts);

                  if( ! instance)
                     return task::unit::action::Outcome::abort;

                  instance->state( state::resource::proxy::instance::State::shutdown);

                  if( instance->process.ipc)
                     state.multiplex.send( instance->process.ipc, message::shutdown::Request{ common::process::handle()});
                  else
                     signal::send( instance->process.pid, common::code::signal::terminate);

                  return task::unit::action::Outcome::success;
               };

               auto instance_exit = [ &state, id, shared]( task::unit::id, const common::message::event::process::Exit& event)
               {
                  if( auto found = algorithm::find( shared->restarts, event.state.pid))
                     algorithm::container::erase( shared->restarts, std::begin( found));
                  else
                     return shared->done() ? task::unit::Dispatch::done : task::unit::Dispatch::pending;

                  log::line( verbose::log, "instances left: ", shared->restarts);

                  if( shared->done())
                     return task::unit::Dispatch::done;

                  auto proxy = state.find_resource( id);

                  if( ! proxy)
                     return task::unit::Dispatch::done; // we can't continue

                  auto instance = algorithm::find( proxy->instances, event.state.pid);

                  if( ! instance)
                     return task::unit::Dispatch::done; // we can't continue

                  if( auto spawned = local::spawn_instance( state, *proxy))
                  {
                     shared->spawned.push_back( spawned->process.pid);
                     *instance = std::move( *spawned);
                     return task::unit::Dispatch::pending;
                  }
                  else
                     return task::unit::Dispatch::done; // we can't continue             
               };

               auto instance_ready = [ &state, id, shared]( task::unit::id, const common::message::transaction::resource::Ready& message)
               {
                  Trace trace{ "transaction::manager::configuration::local::restart_resource instance_ready"};

                  if( auto found = algorithm::find( shared->spawned, message.process.pid))
                     algorithm::container::erase( shared->spawned, std::begin( found));
                  else
                     return shared->done() ? task::unit::Dispatch::done : task::unit::Dispatch::pending;

                  if( shared->restarts.empty())
                     return task::unit::Dispatch::done;

                  auto proxy = state.find_resource( id);

                  if( ! proxy)
                     return task::unit::Dispatch::done; 

                  auto instance = algorithm::find_first_of( proxy->instances, shared->restarts);

                  if( ! instance)
                     return task::unit::Dispatch::done; // we're actually done.

                  // shutdown
                  {
                     instance->state( state::resource::proxy::instance::State::shutdown);

                     if( instance->process.ipc)
                        state.multiplex.send( instance->process.ipc, message::shutdown::Request{ common::process::handle()});
                     else
                        signal::send( instance->process.pid, common::code::signal::terminate);
                  }
                  
                  return task::unit::Dispatch::pending;
               };

               return task::create::unit(
                  task::create::action( "restart_resource", std::move( action)),
                  std::move( instance_exit),
                  std::move( instance_ready)
               );
            }

            [[maybe_unused]] task::Unit restart_resource( State& state, state::resource::Proxy& proxy)
            {
               Trace trace{ "transaction::manager::configuration::local::restart_resource"};
               log::line( verbose::log, "proxy: ", proxy);

               auto restart = algorithm::transform( proxy.instances, []( auto& instance){ return instance.process.pid;});

               return restart_resource( state, proxy.id, restart);
            }

            task::Group scale_resources( State& state, std::vector< common::strong::resource::id> ids)
            {
               Trace trace{ "transaction::manager::configuration::local::scale_resources"};
               log::line( verbose::log, "ids: ", ids);

               std::vector< task::Unit> tasks;

               for( auto id : ids)
               {
                  if( auto found = state.find_resource( id))
                  {
                     if( found->scale_difference() > 0)
                        tasks.push_back( scale_out_resource( state, id));
                     if( found->scale_difference() < 0)
                        tasks.push_back( scale_in_resource( state, id));
                  }
               }

               return task::Group{ std::move( tasks)};
            };


            void added_resources( State& state, auto configuration)
            {
               Trace trace{ "transaction::manager::configuration::local::added_resources"};

               auto valid_configuration = common::algorithm::remove_if( configuration, [ &state]( auto& resource)
               {
                  if( ! common::algorithm::contains( state.system.configuration.resources, resource.key))
                  {
                     common::event::error::send( code::casual::invalid_configuration, "failed to correlate system resource key: '", resource.key, "'");
                     return true;
                  }
                  return false;
               });
               
               auto resources = common::algorithm::transform( valid_configuration, []( const auto& configuration)
               {
                  state::resource::Proxy result{ configuration};

                  // make sure we've got a name
                  result.configuration.name = common::algorithm::coalesce( 
                     std::move( result.configuration.name), 
                     common::string::compose( ".rm.", result.configuration.key, '.', result.id));

                  return result;
               });

               auto ids = common::algorithm::transform( resources, []( auto& resource){ return resource.id;});
               common::algorithm::move( resources, std::back_inserter( state.resources));

               state.task.coordinator.then( scale_resources( state, std::move( ids)));
            }

            void modified_resources( State& state, auto configurations)
            {
               Trace trace{ "transaction::manager::configuration::local::modified_resources"};

               struct Context
               {
                  strong::resource::id id;
                  std::vector< strong::process::id> restart;
               };

               auto contexts = algorithm::accumulate( configurations, std::vector< Context>{}, [ &state]( auto result, auto& configuration)
               {
                  if( auto proxy = state.find_resource( configuration.name))
                  {
                     Context context;
                     context.id = proxy->id;

                     auto difference = configuration.instances - proxy->configuration.instances;
                     proxy->configuration = configuration;

                     if( difference < 0)
                     {
                        // we only restart those who will not be scaled in
                        auto restarts = range::make( std::begin( proxy->instances), std::next( std::end( proxy->instances), difference));
                        context.restart = algorithm::transform( restarts, []( auto& instance){ return instance.process.pid;});
                     }
                     else
                     {
                        // no scaling, or scale out, all current should be restarted
                        context.restart = algorithm::transform( proxy->instances, []( auto& instance){ return instance.process.pid;});
                     }

                     result.push_back( std::move( context));
                  }
                     
                  return result;
               });

               if( std::empty( contexts))
                  return;

               // start with potentially scaling
               {
                  auto tasks = algorithm::accumulate( contexts, std::vector< task::Unit>{}, [ &state]( auto result, auto& context)
                  {
                     auto& proxy = state.get_resource( context.id);

                     if( proxy.scale_difference() > 0)
                        result.push_back( scale_out_resource( state, context.id));
                     else if( proxy.scale_difference() < 0)
                        result.push_back( scale_in_resource( state, context.id));
                 
                     return result;
                  });

                  log::line( verbose::log, "scale tasks: ", tasks);

                  if( ! std::empty( tasks))
                     state.task.coordinator.then( task::Group{ std::move( tasks)});
               }

               // restart the remaining instances
               {
                  auto tasks = algorithm::accumulate( contexts, std::vector< task::Unit>{}, [ &state]( auto result, auto& context)
                  {
                     if( ! std::empty( context.restart))
                        result.push_back( restart_resource( state, context.id, context.restart));
                 
                     return result;
                  });

                  log::line( verbose::log, "restart tasks: ", tasks);

                  if( ! std::empty( tasks))
                     state.task.coordinator.then( task::Group{ std::move( tasks)});

               }
            }

            void removed_resources( State& state, auto configurations)
            {
               Trace trace{ "transaction::manager::configuration::local::removed_resources"};

               // configure to 0 instances
               auto ids = algorithm::accumulate( configurations, std::vector< common::strong::resource::id>{}, [ &state]( auto result, auto& configuration)
               {
                  if( auto proxy = state.find_resource( configuration.name))
                  {
                     proxy->configuration.instances = 0;
                     result.push_back( proxy->id);
                  }
                  return result;
               });

               if( std::empty( ids))
                  return;

               auto remove_action = [ &state, ids]( task::unit::id)
               {
                  Trace trace{ "transaction::manager::configuration::local::removed_resources remove_action"};

                  algorithm::for_each( ids, [ &state]( auto id)
                  {
                     if( auto found = algorithm::find( state.resources, id))
                     {
                        log::line( verbose::log, "found: ", *found);

                        if( std::empty( found->instances))
                           algorithm::container::erase( state.resources, std::begin( found));
                        else
                           log::error( code::casual::invalid_semantics, "resource ", id, " should not have any instances: ", found->instances);
                     }
                  });
                  return task::unit::action::Outcome::abort;
               };

               state.task.coordinator
                  .then( scale_resources( state, ids))
                  .then( task::create::action( "remove-action", std::move( remove_action)));
            }


            void resources( State& state, 
               std::vector< casual::configuration::model::transaction::Resource> current, 
               std::vector< casual::configuration::model::transaction::Resource> wanted)
            {
               Trace trace{ "transaction::manager::configuration::local::resources"};

               auto equal_name = []( auto& lhs, auto& rhs){ return lhs.name == rhs.name;};

               auto change = casual::configuration::model::change::calculate( current, wanted, equal_name);
               common::log::line( verbose::log, "change: ", change);

               if( change.added)
                  added_resources( state, change.added);
               if( change.modified)
                  modified_resources( state, change.modified);
               if( change.removed)
                  removed_resources( state, change.removed);
            }

            void alias_configuration( State& state, std::vector< casual::configuration::model::transaction::Mapping> wanted)
            {
               Trace trace{ "transaction::manager::configuration::local::alias_configuration"};

               auto action = [ &state, wanted = std::move( wanted)]( task::unit::id)
               {
                  // validate, just to give user a heads up for configuration mismatches 
                  { 
                     auto unmapped = algorithm::accumulate( wanted, std::vector< std::string_view>{}, [ &state]( auto result, auto& configuration)
                     {
                        return algorithm::accumulate( configuration.resources, std::move( result), [ &state]( auto result, auto& name)
                        {
                           if( ! state.find_resource( name))
                              algorithm::append_unique_value( name, result);
                           
                           return result;
                        });
                     });

                     if( ! std::empty( unmapped))
                        common::event::error::send( code::casual::invalid_configuration, "resource association to unknown resource names: ", unmapped);
                  } 

                  // replace the alias configuration regardless
                  state.alias.configuration = algorithm::accumulate( wanted, decltype( state.alias.configuration){}, [ &state]( auto result, auto& configuration)
                  {
                     auto ids = algorithm::accumulate( configuration.resources, std::vector< strong::resource::id>{}, [ &state]( auto result, auto& name)
                     {
                        if( auto proxy = state.find_resource( name))
                           result.push_back( proxy->id);
                        return result;
                     });

                     result.emplace( configuration.alias, std::move( ids));
                     return result;
                  });

                  return task::unit::action::Outcome::abort;
               };

               state.task.coordinator
                  .then( task::create::action( "alias_configuration", std::move( action)));
               
            }
         } // <unnamed>
      } // local
      

      void conform( State& state, casual::configuration::Model wanted)
      {
         Trace trace{ "transaction::manager::configuration::conform"};

         local::system( state, std::move( wanted.system));
         local::log( state, wanted.transaction);

         auto current = state.configuration();

         if( current == wanted.transaction)
         {
            common::log::line( verbose::log, "nothing to update");
            return;
         }

         local::resources( state, std::move( current.resources), std::move( wanted.transaction.resources));

         if( current.mappings != wanted.transaction.mappings)
            local::alias_configuration( state, std::move( wanted.transaction.mappings));

      }
      
   } // transaction::manager::configuration
   
} // casual
