//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <utility>

#include "transaction/manager/state.h"
#include "transaction/common.h"

#include "configuration/domain.h"


#include "common/exception/system.h"
#include "common/exception/casual.h"
#include "common/algorithm.h"
#include "common/environment.h"
#include "common/event/send.h"


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace manager
      {

         Settings::Settings() :
            log{ environment::directory::domain() + "/transaction/log.db"}
         {}

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


         State::State( std::string database) : persistent( std::move( database)) {}


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
               auto found = common::algorithm::find( resource.instances, death.pid);

               if( found)
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
            auto found = common::algorithm::find( resources, rm);

            if( ! found)
               throw common::exception::system::invalid::Argument{ "failed to find resource"};

            return *found;
         }

         state::resource::Proxy& State::get_resource( const std::string& name)
         {
            auto found = common::algorithm::find_if( resources, [&name]( auto& r){ return r.name == name;});

            if( ! found)
               throw common::exception::system::invalid::Argument{ "failed to find resource"};

            return *found;
         }

         state::resource::Proxy::Instance& State::get_instance( state::resource::id::type rm, common::strong::process::id pid)
         {
            auto& resource = get_resource( rm);

            auto found = common::algorithm::find_if( resource.instances, [=]( const state::resource::Proxy::Instance& instance){
                  return instance.process.pid == pid;
               });

            if( ! found)
            {
               throw common::exception::system::invalid::Argument{ "failed to find instance"};
            }
            return *found;
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
            auto found = common::algorithm::find_if( externals, [rm]( const state::resource::external::Proxy& p){
               return p.id == rm;
            });

            if( found)
            {
               return *found;
            }

            throw common::exception::system::invalid::Argument{ common::string::compose( "failed to find external resource proxy: ", rm)};
         }
         
      } // manager
   } // transaction
} // casual


