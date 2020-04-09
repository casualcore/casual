//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <utility>

#include "transaction/manager/state.h"
#include "transaction/common.h"


#include "common/algorithm.h"
#include "common/environment.h"
#include "common/environment/string.h"
#include "common/event/send.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace manager
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
                  {
                     m_state = state;
                  }
               }

               Proxy::Instance::State Proxy::Instance::state() const
               {
                  return m_state;
               }

               bool Proxy::booted() const
               {
                  return common::algorithm::all_of( instances, []( const Instance& i){
                     switch( i.state())
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
                  Proxy::Proxy( common::process::Handle  process, id::type id)
                     : process{std::move( process)}, id{std::move( id)}
                  {
                  }

                  bool operator == ( const Proxy& lhs, const common::process::Handle& rhs)
                  {
                     return lhs.process == rhs;
                  }

                  namespace proxy
                  {
                     id::type id( State& state, const common::process::Handle& process)
                     {
                        auto found = common::algorithm::find( state.externals, process);

                        if( found)
                        {
                           return found->id;
                        }

                        static id::type base_id;
                        
                        --base_id.underlaying();
                        state.externals.emplace_back( process, base_id);
                        return state.externals.back().id;
                     }
                  } // proxy
               } // external
            } // resource

         } // state

         Transaction::Resource::Result Transaction::Resource::convert( common::code::xa value)
         {
            switch( value)
            {
               using xa = common::code::xa;

               case xa::heuristic_hazard: return Result::xa_HEURHAZ; break;
               case xa::heuristic_mix: return Result::xa_HEURMIX; break;
               case xa::heuristic_commit: return Result::xa_HEURCOM; break;
               case xa::heuristic_rollback: return Result::xa_HEURRB; break;
               case xa::resource_fail: return Result::xaer_RMFAIL; break;
               case xa::resource_error: return Result::xaer_RMERR; break;
               case xa::rollback_integrity: return Result::xa_RBINTEGRITY; break;
               case xa::rollback_communication: return Result::xa_RBCOMMFAIL; break;
               case xa::rollback_unspecified: return Result::xa_RBROLLBACK; break;
               case xa::rollback_other: return Result::xa_RBOTHER; break;
               case xa::rollback_deadlock: return Result::xa_RBDEADLOCK; break;
               case xa::protocol: return Result::xaer_PROTO; break;
               case xa::rollback_protocoll: return Result::xa_RBPROTO; break;
               case xa::rollback_timeout: return Result::xa_RBTIMEOUT; break;
               case xa::rollback_transient: return Result::xa_RBTRANSIENT; break;
               case xa::argument: return Result::xaer_INVAL; break;
               case xa::no_migrate: return Result::xa_NOMIGRATE; break;
               case xa::outside: return Result::xaer_OUTSIDE; break;
               case xa::invalid_xid: return Result::xaer_NOTA; break;
               case xa::outstanding_async: return Result::xaer_ASYNC; break;
               case xa::retry: return Result::xa_RETRY; break;
               case xa::duplicate_xid: return Result::xaer_DUPID; break;
               case xa::ok: return Result::xa_OK; break;
               case xa::read_only: return Result::xa_RDONLY; break;
            }
            return Result::xaer_RMFAIL;
         }

         common::code::xa Transaction::Resource::convert( Result value)
         {
            using xa = common::code::xa;
            
            switch( value)
            {
               case Result::xa_HEURHAZ: return xa::heuristic_hazard; break;
               case Result::xa_HEURMIX: return xa::heuristic_mix; break;
               case Result::xa_HEURCOM: return xa::heuristic_commit; break;
               case Result::xa_HEURRB: return xa::heuristic_rollback; break;
               case Result::xaer_RMFAIL: return xa::resource_fail; break;
               case Result::xaer_RMERR: return xa::resource_error; break;
               case Result::xa_RBINTEGRITY: return xa::rollback_integrity; break;
               case Result::xa_RBCOMMFAIL: return xa::rollback_communication; break;
               case Result::xa_RBROLLBACK: return xa::rollback_unspecified; break;
               case Result::xa_RBOTHER: return xa::rollback_other; break;
               case Result::xa_RBDEADLOCK: return xa::rollback_deadlock; break;
               case Result::xaer_PROTO: return xa::protocol; break;
               case Result::xa_RBPROTO: return xa::rollback_protocoll; break;
               case Result::xa_RBTIMEOUT: return xa::rollback_timeout; break;
               case Result::xa_RBTRANSIENT: return xa::rollback_transient; break;
               case Result::xaer_INVAL: return xa::argument; break;
               case Result::xa_NOMIGRATE: return xa::no_migrate; break;
               case Result::xaer_OUTSIDE: return xa::outside; break;
               case Result::xaer_NOTA: return xa::invalid_xid; break;
               case Result::xaer_ASYNC: return xa::outstanding_async; break;
               case Result::xa_RETRY: return xa::retry; break;
               case Result::xaer_DUPID: return xa::duplicate_xid; break;
               case Result::xa_OK: return xa::ok; break;
               case Result::xa_RDONLY: return xa::read_only; break;
            }
            return xa::resource_fail;
         }

         void Transaction::Resource::set_result( common::code::xa value)
         {
            result = convert( value);
         }

         bool Transaction::Resource::done() const
         {
            switch( result)
            {
            case Result::xa_RDONLY:
            case Result::xaer_NOTA:
               return true;
            default:
               return false;
            }
         }

         std::ostream& operator << ( std::ostream& out, Transaction::Resource::Stage value)
         {
            using Stage = Transaction::Resource::Stage;
            auto stringify = []( Stage value)
            {
               switch( value)
               {
                  case Stage::involved: return "involved";
                  case Stage::prepare_requested: return "prepare_requested";
                  case Stage::prepare_replied: return "prepare_replied";
                  case Stage::commit_requested: return "commit_requested";
                  case Stage::commit_replied: return "commit_replied";
                  case Stage::rollback_requested: return "rollback_requested";
                  case Stage::rollback_replied: return "rollback_replied";
                  case Stage::done: return "done";
                  case Stage::error: return "error";
                  case Stage::not_involved: return "not_involved";
               }
               return "unknown";
            };

            return out << stringify( value);
         }


         std::vector< common::strong::resource::id> Transaction::Branch::involved() const
         {
            return common::algorithm::transform( resources, []( auto& r){ return r.id;});
         }

         void Transaction::Branch::involve( common::strong::resource::id resource)
         {
            if( ! algorithm::find( resources, resource))
            {
               log::line( verbose::log, "new resource involved: ", resource);
               resources.emplace_back( resource);
            }
         }

         Transaction::Resource::Stage Transaction::Branch::stage() const
         {
            auto min_stage = []( Resource::Stage stage, auto& resource){ return std::min( stage, resource.stage);};

            return algorithm::accumulate( resources, Resource::Stage::not_involved, min_stage);
         }

         Transaction::Resource::Result Transaction::Branch::results() const
         {
            auto severe_result = []( Resource::Result result, auto& resource){ return std::min( result, resource.result);};

            return algorithm::accumulate( resources, Resource::Result::xa_RDONLY, severe_result);
         }


         platform::size::type Transaction::resource_count() const noexcept
         {
            auto count_resources = []( auto size, auto& branch){ return size + branch.resources.size();};

            return algorithm::accumulate( branches, 0, count_resources);
         }    

         Transaction::Resource::Stage Transaction::stage() const
         {
            auto min_branch_stage = []( Resource::Stage stage, auto& branch)
            {
               return std::min( stage, branch.stage());
            };
            return algorithm::accumulate( branches, Resource::Stage::not_involved, min_branch_stage);
         }

         common::code::xa Transaction::results() const
         {
            auto severe_branch_results = []( Resource::Result result, auto& branch)
            {
               return std::min( result, branch.results());
            };
            return Resource::convert( algorithm::accumulate( branches, Resource::Result::xa_RDONLY, severe_branch_results));
         }

         namespace local
         {
            namespace
            {
               auto initialize_log = []( auto&& settings, auto&& configuration)
               {
                  return common::environment::string( common::coalesce( 
                     std::move( settings), 
                     std::move( configuration), 
                     environment::directory::domain() + "/transaction/log.db"));

               };

               namespace configure
               {
                  auto properties = []( auto&& properties)
                  {
                     Trace trace{ "transaction::manager::state::local::configure::properties"};
                     std::map< std::string, configuration::resource::Property> result;
               
                     for( auto& property : properties)
                     {
                        auto emplaced = result.emplace( property.key, std::move( property));
                        if( ! emplaced.second)
                           common::code::raise::error( common::code::casual::invalid_configuration, "multiple keys in resource config: ", emplaced.first->first);
                     }

                     return result;
                  };

                  auto resources = []( auto&& resources, const auto& properties)
                  {
                     Trace trace{ "transaction::manager::state::local::configure::resources"};

                     auto transform_resource = []( const auto& r)
                     {
                        state::resource::Proxy proxy{ state::resource::Proxy::generate_id{}};

                        proxy.name = common::coalesce( r.name, common::string::compose( ".rm.", r.key, '.', proxy.id.value()));
                        proxy.concurency = r.instances;
                        proxy.key = r.key;
                        proxy.openinfo = r.openinfo;
                        proxy.closeinfo = r.closeinfo;
                        proxy.note = r.note;

                        return proxy;
                     };

                     auto validate = [&]( const auto& r) 
                     {
                        if( common::algorithm::find( properties, r.key))
                           return true;
                        
                        common::event::error::send( code::casual::invalid_argument, "failed to correlate resource key '", r.key, "' - action: skip resource");
                        return false;   
                     };

                     std::vector< state::resource::Proxy> result;

                     common::algorithm::transform_if(
                        resources,
                        result,
                        transform_resource,
                        validate);

                     return result;
                  };

                  auto alias( const State& state, const configuration::Model& model)
                  {
                     Trace trace{ "transaction::manager::state::local::configure::alias"};

                     std::map< std::string, std::vector< state::resource::id::type>> result;

                     auto add_resources = [&]( auto& server)
                     {
                        std::vector< state::resource::id::type> ids;

                        auto transform_rm = [&state]( auto& name)
                        {
                           return state.get_resource( name).id;
                        };
                        
                        ids = algorithm::transform( server.resources, transform_rm);

                        // find group resources

                        auto add_group_resource = [&]( auto& group)
                        {
                           if( auto found = algorithm::find( model.domain.groups, group))
                              algorithm::transform( found->resources, ids, transform_rm);
                        };

                        algorithm::for_each( server.memberships, add_group_resource);

                        if( ! ids.empty())
                           result[ server.alias] = std::move( ids);

                     };

                     algorithm::for_each( model.domain.servers, add_resources);

                     return result;
                  }

               } // configure
            } // <unnamed>
         } // local


         State::State( 
            manager::Settings settings, 
            configuration::Model configuration,
            std::vector< configuration::resource::Property> properties) 
            : persistent( local::initialize_log( settings.log, configuration.transaction.log))
         {
            Trace trace{ "transaction::manager::State::State"};

            resource.properties = local::configure::properties( std::move( properties));

            resources = local::configure::resources( configuration.transaction.resources, resource.properties);

            m_alias.configuration = local::configure::alias( *this, configuration);
         }


         bool State::outstanding() const
         {
            return ! persistent.replies.empty();
         }


         bool State::booted() const
         {
            return common::algorithm::all_of( resources, []( const auto& p){ return p.booted();});
         }

         size_type State::instances() const
         {
            size_type result = 0;

            for( auto& resource : resources)
            {
               result += resource.instances.size();
            }
            return result;
         }

         std::vector< common::strong::process::id> State::processes() const
         {
            std::vector< common::strong::process::id> result;

            for( auto& resource : resources)
            {
               for( auto& instance : resource.instances)
               {
                  result.push_back( instance.process.pid);
               }
            }
            return result;
         }

         void State::operator () ( const common::process::lifetime::Exit& death)
         {

            for( auto& resource : resources)
            {
               if( auto found = common::algorithm::find( resource.instances, death.pid))
               {
                  if( found->state() != state::resource::Proxy::Instance::State::shutdown)
                  {
                     log::line( log::category::error, "resource proxy instance died - ", *found);
                  }

                  resource.metrics += found->metrics;
                  resource.instances.erase( std::begin( found));

                  log::line( log, "remove dead process: ", death);
                  return;
               }
            }

            log::line( log::category::warning, "failed to find and remove dead instance: ", death);
         }

         state::resource::Proxy& State::get_resource( state::resource::id::type rm)
         {
            if( auto found = common::algorithm::find( resources, rm))
               return *found;

            code::raise::error( code::casual::invalid_argument, "failed to find resource - rm: ", rm);
         }

         state::resource::Proxy* State::find_resource( const std::string& name)
         {
            if( auto found = common::algorithm::find_if( resources, [&name]( auto& r){ return r.name == name;}))
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
            if( auto found = common::algorithm::find_if( resources, [&name]( auto& r){ return r.name == name;}))
               return *found;

            code::raise::error( code::casual::invalid_argument, "failed to find resource - name: ", name);
         }

         state::resource::Proxy::Instance& State::get_instance( state::resource::id::type rm, common::strong::process::id pid)
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

         State::instance_range State::idle_instance( state::resource::id::type rm)
         {
            auto& resource = get_resource( rm);

            return common::algorithm::find_if( resource.instances, state::filter::idle());
         }

         const state::resource::external::Proxy& State::get_external( state::resource::id::type rm) const
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
               result.key = resource.key;
               result.name = resource.name;
               result.openinfo = resource.openinfo;
               result.closeinfo = resource.closeinfo;

               return result;
            };

            // first we get the 'named' resources, if any.
            algorithm::for_each( resources, [&]( auto& resource)
            {
               if( common::algorithm::find( request.resources, resource.name))
                  reply.resources.push_back( transform_resource( resource));
            });

            // get the implicit resource configuration, based on alias
            if( auto found = algorithm::find( m_alias.configuration, request.alias))
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
            algorithm::trim( reply.resources, algorithm::unique( algorithm::sort( reply.resources, id_less), id_equal));

            return reply;
         }
         
      } // manager
   } // transaction
} // casual


