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
               auto found = common::algorithm::find_if( instances, [pid]( auto& i){
                  return i.process.pid == pid;
               });

               if( found)
               {
                  metrics += found->metrics;
                  instances.erase( std::begin( found));
                  return true;
               }
               return false;
            }

            std::ostream& operator << ( std::ostream& out, const Proxy::Instance::State& value)
            {
               auto state_switch = [&]()
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
                  };

               return out << state_switch();
            }


            namespace external
            {
               Proxy::Proxy( common::process::Handle  process, common::strong::resource::id id)
                  : process{std::move( process)}, id{std::move( id)}
               {
               }

               bool operator == ( const Proxy& lhs, const common::process::Handle& rhs)
               {
                  return lhs.process == rhs;
               }

               namespace proxy
               {
                  common::strong::resource::id id( State& state, const common::process::Handle& process)
                  {
                     if( auto found = common::algorithm::find( state.externals, process))
                        return found->id;

                     static common::strong::resource::id base_id;
                     
                     --base_id.underlaying();
                     return state.externals.emplace_back( process, base_id).id;

                  }
               } // proxy
            } // external
         } // resource

         namespace code::priority
         {
            common::code::xa convert( xa code)
            {
               switch( code)
               {
                  case xa::heuristic_hazard: return common::code::xa::heuristic_hazard; break;
                  case xa::heuristic_mix: return common::code::xa::heuristic_mix; break;
                  case xa::heuristic_commit: return common::code::xa::heuristic_commit; break;
                  case xa::heuristic_rollback: return common::code::xa::heuristic_rollback; break;
                  case xa::resource_fail: return common::code::xa::resource_fail; break;
                  case xa::resource_error: return common::code::xa::resource_error; break;
                  case xa::rollback_integrity: return common::code::xa::rollback_integrity; break;
                  case xa::rollback_communication: return common::code::xa::rollback_communication; break;
                  case xa::rollback_unspecified: return common::code::xa::rollback_unspecified; break;
                  case xa::rollback_other: return common::code::xa::rollback_other; break;
                  case xa::rollback_deadlock: return common::code::xa::rollback_deadlock; break;
                  case xa::protocol: return common::code::xa::protocol; break;
                  case xa::rollback_protocoll: return common::code::xa::rollback_protocoll; break;
                  case xa::rollback_timeout: return common::code::xa::rollback_timeout; break;
                  case xa::rollback_transient: return common::code::xa::rollback_transient; break;
                  case xa::argument: return common::code::xa::argument; break;
                  case xa::no_migrate: return common::code::xa::no_migrate; break;
                  case xa::outside: return common::code::xa::outside; break;
                  case xa::invalid_xid: return common::code::xa::invalid_xid; break;
                  case xa::outstanding_async: return common::code::xa::outstanding_async; break;
                  case xa::retry: return common::code::xa::retry; break;
                  case xa::duplicate_xid: return common::code::xa::duplicate_xid; break;
                  case xa::ok: return common::code::xa::ok; break;
                  case xa::read_only: return common::code::xa::read_only; break;
               }

               casual::terminate( "invalid value for code::priority::xa: ", cast::underlying( code));

            }

            xa convert( common::code::xa code)
            {
               switch( code)
               {
                  case common::code::xa::heuristic_hazard: return xa::heuristic_hazard; break;
                  case common::code::xa::heuristic_mix: return xa::heuristic_mix; break;
                  case common::code::xa::heuristic_commit: return xa::heuristic_commit; break;
                  case common::code::xa::heuristic_rollback: return xa::heuristic_rollback; break;
                  case common::code::xa::resource_fail: return xa::resource_fail; break;
                  case common::code::xa::resource_error: return xa::resource_error; break;
                  case common::code::xa::rollback_integrity: return xa::rollback_integrity; break;
                  case common::code::xa::rollback_communication: return xa::rollback_communication; break;
                  case common::code::xa::rollback_unspecified: return xa::rollback_unspecified; break;
                  case common::code::xa::rollback_other: return xa::rollback_other; break;
                  case common::code::xa::rollback_deadlock: return xa::rollback_deadlock; break;
                  case common::code::xa::protocol: return xa::protocol; break;
                  case common::code::xa::rollback_protocoll: return xa::rollback_protocoll; break;
                  case common::code::xa::rollback_timeout: return xa::rollback_timeout; break;
                  case common::code::xa::rollback_transient: return xa::rollback_transient; break;
                  case common::code::xa::argument: return xa::argument; break;
                  case common::code::xa::no_migrate: return xa::no_migrate; break;
                  case common::code::xa::outside: return xa::outside; break;
                  case common::code::xa::invalid_xid: return xa::invalid_xid; break;
                  case common::code::xa::outstanding_async: return xa::outstanding_async; break;
                  case common::code::xa::retry: return xa::retry; break;
                  case common::code::xa::duplicate_xid: return xa::duplicate_xid; break;
                  case common::code::xa::ok: return xa::ok; break;
                  case common::code::xa::read_only: return xa::read_only; break;
               }

               casual::terminate( "invalid value for common::code::xa: ", cast::underlying( code));
            }
            
         } // code::priority

         namespace transaction
         {
            std::ostream& operator << ( std::ostream& out, Stage value)
            {
               switch( value)
               {
                  case Stage::involved: return out << "involved";
                  case Stage::prepare: return out << "prepare";
                  case Stage::commit: return out << "commit";
                  case Stage::rollback: return out << "rollback";
               }
               return out << "<unknown>";
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
         if( auto found = common::algorithm::find( resources, rm))
            return *found;

         code::raise::error( code::casual::invalid_argument, "failed to find resource - rm: ", rm);
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
         auto has_pid = [pid]( auto& instance){ return instance.process.pid == pid;};

         if( auto found = common::algorithm::find_if( resource.instances, has_pid))
            return *found;

         code::raise::error( code::casual::invalid_argument, "failed to find instance - rm: ", rm, ", pid: ", pid);
      }

      bool State::remove_instance( common::strong::process::id pid)
      {
         return ! common::algorithm::find_if( resources, [pid]( auto& r){
            return r.remove_instance( pid);
         }).empty();
      }

      state::resource::Proxy::Instance* State::idle( common::strong::resource::id rm)
      {
         auto& resource = get_resource( rm);

         return common::algorithm::find_if( resource.instances, []( auto& instance){ return instance.state() == decltype( instance.state())::idle;}).data();
      }

      const state::resource::external::Proxy& State::get_external( common::strong::resource::id rm) const
      {
         if( auto found = common::algorithm::find( externals, rm))
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


