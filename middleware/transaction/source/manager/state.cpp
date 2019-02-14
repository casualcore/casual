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

               std::ostream& operator << ( std::ostream& out, const Proxy& value)
               {
                  return out << "{ id: " << value.id
                        << ", concurency: " << value.concurency
                        << ", key: " << value.key
                        << ", openinfo: \"" << value.openinfo
                        << "\", closeinfo: \"" << value.closeinfo
                        << "\", instances: " << common::range::make( value.instances)
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Proxy::Instance& value)
               {
                  return out << "{ id: " << value.id
                        << ", process: " << value.process
                        << ", state: " << value.state()
                        << '}';
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

            void configure( State& state, const common::message::domain::configuration::Reply& configuration)
            {

               {
                  Trace trace{ "transaction manager xa-switch configuration"};

                  auto resources = configuration::resource::property::get();

                  for( auto& resource : resources)
                  {
                     auto result = state.resource_properties.emplace( resource.key, std::move( resource));
                     if( ! result.second)
                        throw common::exception::casual::invalid::Configuration( "multiple keys in resource config: " + result.first->first);
                  }
               }

               // configure resources
               {
                  Trace trace{ "transaction manager resource configuration"};

                  auto transform_resource = []( const auto& r){

                     state::resource::Proxy proxy{ state::resource::Proxy::generate_id{}};

                     proxy.name = r.name;
                     proxy.concurency = r.instances;
                     proxy.key = r.key;
                     proxy.openinfo = r.openinfo;
                     proxy.closeinfo = r.closeinfo;
                     proxy.note = r.note;

                     return proxy;
                  };

                  auto validate = [&state]( const common::message::domain::configuration::transaction::Resource& r) {
                     if( ! common::algorithm::find( state.resource_properties, r.key))
                     {
                        log::line( log::category::error, "failed to correlate resource key '", r.key, "' - action: skip resource");

                        common::event::error::send( "failed to correlate resource key '" + r.key + "'");
                        return false;
                     }
                     return true;
                  };

                  common::algorithm::transform_if(
                        configuration.domain.transaction.resources,
                        state.resources,
                        transform_resource,
                        validate);

               }
            }
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

         std::vector< common::strong::resource::id> Transaction::involved() const
         {
            return common::algorithm::transform( resources, []( auto& r){ return r.id;});
         }

         void Transaction::involved( common::strong::resource::id resource)
         {
            if( ! algorithm::find( resources, resource))
            {
               log::line( verbose::log, "new resource involved: ", resource);
               resources.emplace_back( resource);
            }
         }

         Transaction::Resource::Stage Transaction::stage() const
         {
            Resource::Stage result = Resource::Stage::not_involved;

            for( auto& resource : resources)
            {
               if( result > resource.stage)
                  result = resource.stage;
            }
            return result;
         }

         common::code::xa Transaction::results() const
         {
            auto result = Resource::Result::xa_RDONLY;

            for( auto& resource : resources)
            {
               if( resource.result < result)
               {
                  result = resource.result;
               }
            }
            return Resource::convert( result);
         }


         std::ostream& operator << ( std::ostream& out, const Transaction& value)
         {
            return out << "{ trid: " << value.trid
               << ", resources: " << common::range::make( value.resources)
               << ", correlation: " << value.correlation
               << ", remote-resource: " << value.resource
               << '}';
         }

         State::State( std::string database) : persistent_log( std::move( database)) {}


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
            {
               throw common::exception::system::invalid::Argument{ "failed to find resource"};
            }
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


