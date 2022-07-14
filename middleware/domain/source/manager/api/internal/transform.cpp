//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/domain/manager/api/internal/transform.h"

namespace casual
{
   using namespace common;
   
   namespace domain::manager::api::internal::transform
   {
      api::Model state( admin::model::State state)
      {
         Model result;

         auto transform_group = []( auto& group)
         {
            model::Group result;
            result.id = group.id;
            result.name = std::move( group.name);
            result.note = std::move( group.note);
            result.dependencies = std::move( group.dependencies);
            return result;
         };

         algorithm::transform( state.groups, result.groups, transform_group);

         auto assign_process = []( auto& source, auto& target)
         {
            target.id = source.id;
            target.alias = std::move( source.alias);
            target.note = std::move( source.note);
            target.path = std::move( source.path);
            target.arguments = std::move( source.arguments);
            target.memberships = std::move( source.memberships);
            target.environment.variables = std::move( source.environment.variables);
         };

         auto transform_instance_state = []( auto state)
         {
            using Enum = decltype( state);
            switch( state)
            {
               case Enum::running: return model::instance::State::running;
               // treat spawned as scale_out, for now.
               case Enum::spawned : return model::instance::State::scale_out;
               case Enum::scale_out: return model::instance::State::scale_out;
               case Enum::scale_in: return model::instance::State::scale_in;
               case Enum::exit: return model::instance::State::exit;
               case Enum::error: return model::instance::State::error;
            }
            assert( ! "unknown state");
         };
         
         auto transform_server = [assign_process, transform_instance_state]( auto& server)
         {
            model::Server result;
            assign_process( server, result);
            result.id = server.id;
            result.resources = std::move( server.resources);
            result.restriction = std::move( server.restriction);

            auto transform_instance = [transform_instance_state]( auto& instance)
            {
               model::Server::instance_type result;
               result.handle.pid = instance.handle.pid.value();
               instance.handle.ipc.value().copy( result.handle.ipc);
               result.state = transform_instance_state( instance.state);
               result.spawnpoint = instance.spawnpoint;
               return result;
            };

            algorithm::transform( server.instances, result.instances, transform_instance);

            return result;
         };

         algorithm::transform( state.servers, result.servers, transform_server);

         auto transform_executable = [assign_process, transform_instance_state]( auto& executable)
         {
            model::Executable result;
            assign_process( executable, result);
            result.id = executable.id;

            auto transform_instance = [transform_instance_state]( auto& instance)
            {
               model::Executable::instance_type result;
               result.handle = instance.handle.value();
               result.state = transform_instance_state( instance.state);
               result.spawnpoint = instance.spawnpoint;
               return result;
            };
            algorithm::transform( executable.instances, result.instances, transform_instance);
            return result;
         };

         algorithm::transform( state.executables, result.executables, transform_executable);

         return result;
      }
   } // domain::manager::api::internal::transform
} // casual
