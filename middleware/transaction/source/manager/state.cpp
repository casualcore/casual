//!
//! casual
//!

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

   namespace transaction
   {
      namespace state
      {



         Statistics::Statistics() :  min{ std::chrono::microseconds::max()}, max{ 0}, total{ 0}, invoked{ 0}
         {

         }

         void Statistics::start( const common::platform::time::point::type& start)
         {
            m_start = start;
         }
         void Statistics::end( const common::platform::time::point::type& end)
         {
            time( m_start, end);
         }

         void Statistics::time( const common::platform::time::point::type& start, const common::platform::time::point::type& end)
         {
            auto time = std::chrono::duration_cast< std::chrono::microseconds>( end - start);
            total += time;

            if( time < min) min = time;
            if( time > max) max = time;

            ++invoked;
         }

         Statistics& operator += ( Statistics& lhs, const Statistics& rhs)
         {
            if( rhs.min < lhs.min) lhs.min = rhs.min;
            if( rhs.max > lhs.max) lhs.max = rhs.max;
            lhs.total += rhs.total;
            lhs.invoked += rhs.invoked;

            return lhs;
         }

         Stats& operator += ( Stats& lhs, const Stats& rhs)
         {
            lhs.resource += rhs.resource;
            lhs.roundtrip += rhs.roundtrip;

            return lhs;
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
               return common::range::all_of( instances, []( const Instance& i){
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

            bool Proxy::remove_instance( common::platform::pid::type pid)
            {
               auto found = common::range::find_if( instances, [pid]( auto& i){
                  return i.process.pid == pid;
               });

               if( found)
               {
                  statistics += found->statistics;
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
                        case Proxy::Instance::State::error: return "startupError";
                     }
                     return "<unknown>";
                  };

               return out << state_switch();
            }


            namespace external
            {
               Proxy::Proxy( const common::process::Handle& process, id::type id)
                  : process{ process}, id{ id}
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
                     auto found = common::range::find( state.externals, process);

                     if( found)
                     {
                        return found->id;
                     }

                     static id::type base_id = 0;

                     state.externals.emplace_back( process, --base_id);
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
                  if( ! state.resource_properties.emplace( resource.key, std::move( resource)).second)
                  {
                     throw common::exception::casual::invalid::Configuration( "multiple keys in resource config: " + resource.key);
                  }
               }
            }

            //
            // configure resources
            //
            {
               Trace trace{ "transaction manager resource configuration"};


               auto transform_resource = []( const common::message::domain::configuration::transaction::Resource& r){

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
                  if( ! common::range::find( state.resource_properties, r.key))
                  {
                     common::log::category::error << "failed to correlate resource key '" << r.key << "' - action: skip resource\n";

                     common::event::error::send( "failed to correlate resource key '" + r.key + "'");
                     return false;
                  }
                  return true;
               };

               common::range::transform_if(
                     configuration.domain.transaction.resources,
                     state.resources,
                     transform_resource,
                     validate);

            }

         }

         namespace filter
         {

            bool Running::operator () ( const resource::Proxy::Instance& instance) const
            {
               return instance.state() == resource::Proxy::Instance::State::idle
                     || instance.state() == resource::Proxy::Instance::State::busy;
            }

         } // filter


      } // state

      Transaction::Resource::Result Transaction::Resource::convert( common::error::code::xa value)
      {
         switch( value)
         {
            using xa = common::error::code::xa;

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

      common::error::code::xa Transaction::Resource::convert( Result value)
      {
         using xa = common::error::code::xa;
         
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

      void Transaction::Resource::set_result( common::error::code::xa value)
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

      common::error::code::xa Transaction::results() const
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

      State::State( const std::string& database) : log( database) {}


      bool State::outstanding() const
      {
         return ! persistent.replies.empty();
      }


      bool State::booted() const
      {
         return common::range::all_of( resources, []( const auto& p){ return p.booted();});
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

      std::vector< common::platform::pid::type> State::processes() const
      {
         std::vector< common::platform::pid::type> result;

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
            auto found = common::range::find_if(
               resource.instances,
               state::filter::Instance{ death.pid});

            if( found)
            {
               if( found->state() != state::resource::Proxy::Instance::State::shutdown)
               {
                  common::log::category::error << "resource proxy instance died - " << *found << std::endl;
               }

               resource.statistics += found->statistics;
               resource.instances.erase( std::begin( found));

               transaction::log << "remove dead process: " << death << std::endl;
               return;
            }
         }

         common::log::category::warning << "failed to find and remove dead instance: " << death << std::endl;
      }

      state::resource::Proxy& State::get_resource( state::resource::id::type rm)
      {
         auto found = common::range::find( resources, rm);

         if( ! found)
         {
            throw common::exception::system::invalid::Argument{ "failed to find resource"};
         }
         return *found;
      }

      state::resource::Proxy::Instance& State::get_instance( state::resource::id::type rm, common::platform::pid::type pid)
      {
         auto& resource = get_resource( rm);

         auto found = common::range::find_if( resource.instances, [=]( const state::resource::Proxy::Instance& instance){
               return instance.process.pid == pid;
            });

         if( ! found)
         {
            throw common::exception::system::invalid::Argument{ "failed to find instance"};
         }
         return *found;
      }

      bool State::remove_instance( common::platform::pid::type pid)
      {
         return common::range::find_if( resources, [pid]( auto& r){
            return r.remove_instance( pid);
         });
      }

      State::instance_range State::idle_instance( state::resource::id::type rm)
      {
         auto& resource = get_resource( rm);

         return common::range::find_if( resource.instances, state::filter::Idle{});
      }

      const state::resource::external::Proxy& State::get_external( state::resource::id::type rm) const
      {
         auto found = common::range::find_if( externals, [rm]( const state::resource::external::Proxy& p){
            return p.id == rm;
         });

         if( found)
         {
            return *found;
         }

         throw common::exception::system::invalid::Argument{ common::string::compose( "failed to find external resource proxy: ", rm)};
      }

   } // transaction

} // casual


