//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <utility>

#include "transaction/manager/state.h"
#include "transaction/common.h"

#include "casual/assert.h"

#include "common/algorithm.h"
#include "common/environment.h"
#include "common/environment/expand.h"
#include "common/event/send.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


namespace casual
{
   using namespace common;

   namespace transaction::manager
   {
      namespace state
      {
         Metrics& Metrics::operator += ( const Metrics& rhs)
         {
            resource += rhs.resource;
            roundtrip += rhs.roundtrip;
            return *this;
         }

         namespace resource
         {

            void Proxy::Instance::general_reserve( platform::time::point::type now)
            {
               if( m_state != State::idle)
                  common::code::raise::error( common::code::casual::invalid_semantics, "trying to reserve rm instance: ", process.pid, " in state: ", m_state);

               m_state = State::busy;
               m_reserved = platform::time::clock::type::now();
            }

            void Proxy::Instance::reserve()
            {
               general_reserve( platform::time::clock::type::now());
            }

            void Proxy::Instance::reserve( platform::time::point::type requested)
            {
               auto now = platform::time::clock::type::now();
               general_reserve( now);
               m_pending += now - requested;
            };

            void Proxy::Instance::unreserve( const common::message::Statistics& statistics)
            {
               // could be in some more "severe" state, we only go back to idle if it was in busy.
               if( m_state == State::busy)
                  m_state = State::idle;

               m_metrics.resource += statistics.end - statistics.start;
               m_metrics.roundtrip +=  platform::time::clock::type::now() - std::exchange( m_reserved, {});
            }


            void Proxy::Instance::state( State state)
            {
               if( m_state != State::shutdown)
                  m_state = state;
            }


            Proxy::Instance::State Proxy::Instance::state() const
            {
               return m_state;
            }

            bool Proxy::booted() const
            {
               return common::algorithm::all_of( instances, []( const Instance& instance){
                  switch( instance.state())
                  {
                     case Instance::State::idle:
                     case Instance::State::error:
                     case Instance::State::busy:
                        return true;
                     default:
                        return false;
                  }
               });
            }

            bool Proxy::remove_instance( common::strong::process::id pid)
            {
               if( auto found = algorithm::find( instances, pid))
               {
                  metrics += found->metrics();
                  pending += found->pending();
                  instances.erase( std::begin( found));
                  return true;
               }
               return false;
            }

            std::string_view description( Proxy::Instance::State value) noexcept
            {
               switch( value)
               {
                  case Proxy::Instance::State::busy: return "busy";
                  case Proxy::Instance::State::absent: return "absent";
                  case Proxy::Instance::State::idle: return "idle";
                  case Proxy::Instance::State::shutdown: return "shutdown";
                  case Proxy::Instance::State::started: return "started";
                  case Proxy::Instance::State::error: return "startup-error";
               }
               return "<unknown>";
            }


            namespace external
            {
               Proxy::Proxy( common::process::Handle process, common::strong::resource::id id)
                  : process{std::move( process)}, id{std::move( id)}
               {
               }

               namespace proxy
               {
                  common::strong::resource::id id( State& state, const common::process::Handle& process)
                  {
                     if( auto found = common::algorithm::find( state.externals, process.ipc))
                        return found->id;

                     static common::strong::resource::id base_id;

                     --base_id.underlying();
                     return state.externals.emplace_back( process, base_id).id;
                  }
               } // proxy
            } // external
         } // resource

         namespace transaction
         {
            std::string_view description( Stage value) noexcept
            {
               switch( value)
               {
                  case Stage::involved: return "involved";
                  case Stage::prepare: return "prepare";
                  case Stage::post_prepare: return "post_prepare";
                  case Stage::commit: return "commit";
                  case Stage::rollback: return "rollback";
               }
               return "<unknown>";
            }

            void Branch::failed( common::strong::resource::id resource)
            {
               if( auto found = common::algorithm::find( resources, resource))
                  found->code = common::code::xa::resource_fail;
            }

         } // transaction

         platform::size::type Transaction::resource_count() const noexcept
         {
            return algorithm::accumulate( branches, platform::size::type{}, []( auto size, auto& branch)
            { 
               return size + branch.resources.size();
            });
         }

         void Transaction::purge()
         {
            auto has_resources = []( auto& branch){ return ! branch.resources.empty();};

            auto erase = std::get< 1>( algorithm::partition( branches, has_resources));
            branches.erase( std::begin( erase), std::end( erase));
         }

         void Transaction::failed( common::strong::resource::id resource)
         {
            for( auto& branch : branches)
               branch.failed( resource);
         }

      } // state

      bool State::booted() const
      {
         return common::algorithm::all_of( resources, []( const auto& p){ return p.booted();});
      }

      platform::size::type State::instances() const
      {
         return algorithm::accumulate( resources, platform::size::type{}, []( auto result, auto& resource)
         {
            return result + resource.instances.size();
         });
      }

      std::vector< common::strong::process::id> State::processes() const
      {
         std::vector< common::strong::process::id> result;

         for( auto& resource : resources)
            for( auto& instance : resource.instances)
               result.push_back( instance.process.pid);

         return result;
      }

      state::resource::Proxy& State::get_resource( common::strong::resource::id rm)
      {
         if( auto found = find_resource( rm))
            return *found;

         code::raise::error( code::casual::invalid_argument, "failed to find resource - rm: ", rm);
      }

      state::resource::Proxy* State::find_resource( common::strong::resource::id rm)
      {
         if( auto found = common::algorithm::find( resources, rm))
            return found.data();

         return nullptr;
      }

      state::resource::Proxy* State::find_resource( const std::string& name)
      {
         if( auto found = common::algorithm::find_if( resources, [&name]( auto& resource){ return resource.configuration.name == name;}))
            return &( *found);

         return nullptr;
      }

      state::resource::Proxy& State::get_resource( const std::string& name)
      {
         if( auto found = find_resource( name))
            return *found;

         code::raise::error( code::casual::invalid_argument, "failed to find resource - name: ", name);
      }

      const state::resource::Proxy& State::get_resource( const std::string& name) const
      {
         if( auto found = common::algorithm::find_if( resources, [&name]( auto& resource){ return resource.configuration.name == name;}))
            return *found;

         code::raise::error( code::casual::invalid_argument, "failed to find resource - name: ", name);
      }

      state::resource::Proxy::Instance& State::get_instance( common::strong::resource::id rm, common::strong::process::id pid)
      {
         auto& resource = get_resource( rm);
         if( auto found = common::algorithm::find( resource.instances, pid))
            return *found;

         code::raise::error( code::casual::invalid_argument, "failed to find instance - rm: ", rm, ", pid: ", pid);
      }

      bool State::remove_instance( common::strong::process::id pid)
      {
         return ! common::algorithm::find_if( resources, [pid]( auto& r){
            return r.remove_instance( pid);
         }).empty();
      }  

      state::resource::Proxy::Instance* State::try_reserve( common::strong::resource::id rm)
      {
         if( auto resource = find_resource( rm))
         {
            if( auto found = common::algorithm::find( resource->instances, state::resource::Proxy::Instance::State::idle))
            {
               found->reserve();
               return found.data();
            }
         }
         
         return nullptr;
      }

      const state::resource::external::Proxy* State::find_external( common::strong::resource::id rm) const noexcept
      {
         if( auto found = common::algorithm::find( externals, rm))
            return found.data();
         return nullptr;
      }

      const state::resource::external::Proxy& State::get_external( common::strong::resource::id rm) const
      {
         if( auto found = find_external( rm))
            return *found;

         code::raise::error( code::casual::invalid_argument, "failed to find external resource proxy: ", rm);
      }

      common::message::transaction::configuration::alias::Reply State::configuration(
         const common::message::transaction::configuration::alias::Request& request)
      {
         auto reply = common::message::reverse::type( request);

         auto transform_resource = []( auto& resource)
         {
            common::message::transaction::configuration::Resource result;

            result.id = resource.id;
            result.key = resource.configuration.key;
            result.name = resource.configuration.name;
            result.openinfo = resource.configuration.openinfo;
            result.closeinfo = resource.configuration.closeinfo;

            return result;
         };

         // first we get the 'named' resources, if any.
         algorithm::for_each( resources, [&]( auto& resource)
         {
            if( common::algorithm::find( request.resources, resource.configuration.name))
               reply.resources.push_back( transform_resource( resource));
         });

         // get the implicit resource configuration, based on alias
         if( auto found = algorithm::find( alias.configuration, request.alias))
         {
            const auto& rms = found->second;

            algorithm::transform( rms, reply.resources, [&]( auto rm)
            {
               return transform_resource( get_resource( rm));
            });
         }

         auto id_less = []( auto& l, auto& r){ return l.id < r.id;};
         auto id_equal = []( auto& l, auto& r){ return l.id == r.id;};

         // make sure we only reply with unique rm:S
         algorithm::container::trim( reply.resources, algorithm::unique( algorithm::sort( reply.resources, id_less), id_equal));

         return reply;
      }

      configuration::model::transaction::Model State::configuration() const
      {
         configuration::model::transaction::Model result;
         result.log = persistent.log.file();
         result.resources = algorithm::transform( resources, []( auto& resource)
         {
            return resource.configuration;
         });

         result.mappings = algorithm::transform( alias.configuration, [this]( auto& pair)
         {
            configuration::model::transaction::Mapping result;
            result.alias = pair.first;
            result.resources = algorithm::transform( pair.second, [this]( auto id)
            {
               if( auto found = common::algorithm::find( resources, id))
                  return found->configuration.name;

               return std::string{};
            });
            return result;
         });

         return result;
      }
      
   } // transaction::manager
} // casual


