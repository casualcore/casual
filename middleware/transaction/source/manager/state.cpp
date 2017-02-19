//!
//! casual
//!

#include "transaction/manager/state.h"

#include "configuration/domain.h"


#include "common/exception.h"
#include "common/algorithm.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/environment.h"




namespace casual
{
   using namespace common;

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

            bool Proxy::ready() const
            {
               return common::range::all_of( instances, []( const Instance& i){ return i.state() == Instance::State::idle;});
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
                     auto found = range::find( state.externals, process);

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
               Trace trace( "transaction manager xa-switch configuration", log::internal::transaction);

               auto resources = configuration::resource::property::get();

               for( auto& resource : resources)
               {
                  if( ! state.resource_properties.emplace( resource.key, std::move( resource)).second)
                  {
                     throw exception::invalid::Configuration( "multiple keys in resource config: " + resource.key);
                  }
               }
            }

            //
            // configure resources
            //
            {
               Trace trace( "transaction manager resource configuration", log::internal::transaction);

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

               range::transform(
                     configuration.domain.transaction.resources,
                     state.resources,
                     transform_resource);

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

      Transaction::Resource::Result Transaction::Resource::convert( int value)
      {
         switch( value)
         {
            case XA_HEURHAZ: return Result::xa_HEURHAZ; break;
            case XA_HEURMIX: return Result::xa_HEURMIX; break;
            case XA_HEURCOM: return Result::xa_HEURCOM; break;
            case XA_HEURRB: return Result::xa_HEURRB; break;
            case XAER_RMFAIL: return Result::xaer_RMFAIL; break;
            case XAER_RMERR: return Result::xaer_RMERR; break;
            case XA_RBINTEGRITY: return Result::xa_RBINTEGRITY; break;
            case XA_RBCOMMFAIL: return Result::xa_RBCOMMFAIL; break;
            case XA_RBROLLBACK: return Result::xa_RBROLLBACK; break;
            case XA_RBOTHER: return Result::xa_RBOTHER; break;
            case XA_RBDEADLOCK: return Result::xa_RBDEADLOCK; break;
            case XAER_PROTO: return Result::xaer_PROTO; break;
            case XA_RBPROTO: return Result::xa_RBPROTO; break;
            case XA_RBTIMEOUT: return Result::xa_RBTIMEOUT; break;
            case XA_RBTRANSIENT: return Result::xa_RBTRANSIENT; break;
            case XAER_INVAL: return Result::xaer_INVAL; break;
            case XA_NOMIGRATE: return Result::xa_NOMIGRATE; break;
            case XAER_OUTSIDE: return Result::xaer_OUTSIDE; break;
            case XAER_NOTA: return Result::xaer_NOTA; break;
            case XAER_ASYNC: return Result::xaer_ASYNC; break;
            case XA_RETRY: return Result::xa_RETRY; break;
            case XAER_DUPID: return Result::xaer_DUPID; break;
            case XA_OK: return Result::xa_OK; break;
            case XA_RDONLY: return Result::xa_RDONLY; break;
         }
         return Result::xaer_RMFAIL;
      }

      int Transaction::Resource::convert( Result value)
      {
         switch( value)
         {
            case Result::xa_HEURHAZ: return XA_HEURHAZ; break;
            case Result::xa_HEURMIX: return XA_HEURMIX; break;
            case Result::xa_HEURCOM: return XA_HEURCOM; break;
            case Result::xa_HEURRB: return XA_HEURRB; break;
            case Result::xaer_RMFAIL: return XAER_RMFAIL; break;
            case Result::xaer_RMERR: return XAER_RMERR; break;
            case Result::xa_RBINTEGRITY: return XA_RBINTEGRITY; break;
            case Result::xa_RBCOMMFAIL: return XA_RBCOMMFAIL; break;
            case Result::xa_RBROLLBACK: return XA_RBROLLBACK; break;
            case Result::xa_RBOTHER: return XA_RBOTHER; break;
            case Result::xa_RBDEADLOCK: return XA_RBDEADLOCK; break;
            case Result::xaer_PROTO: return XAER_PROTO; break;
            case Result::xa_RBPROTO: return XA_RBPROTO; break;
            case Result::xa_RBTIMEOUT: return XA_RBTIMEOUT; break;
            case Result::xa_RBTRANSIENT: return XA_RBTRANSIENT; break;
            case Result::xaer_INVAL: return XAER_INVAL; break;
            case Result::xa_NOMIGRATE: return XA_NOMIGRATE; break;
            case Result::xaer_OUTSIDE: return XAER_OUTSIDE; break;
            case Result::xaer_NOTA: return XAER_NOTA; break;
            case Result::xaer_ASYNC: return XAER_ASYNC; break;
            case Result::xa_RETRY: return XA_RETRY; break;
            case Result::xaer_DUPID: return XAER_DUPID; break;
            case Result::xa_OK: return XA_OK; break;
            case Result::xa_RDONLY: return XA_RDONLY; break;
         }
         return XAER_RMFAIL;
      }

      void Transaction::Resource::set_result( int value)
      {
         result = convert( value);
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

      Transaction::Resource::Result Transaction::results() const
      {
         auto result = Resource::Result::xa_RDONLY;

         for( auto& resource : resources)
         {
            if( resource.result < result)
            {
               result = resource.result;
            }
         }
         return result;
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

      bool State::ready() const
      {
         return range::all_of( resources, []( const state::resource::Proxy& p){ return p.ready();});
      }

      std::size_t State::instances() const
      {
         std::size_t result = 0;

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
                  log::error << "resource proxy instance died - " << *found << std::endl;
               }

               resource.statistics += found->statistics;
               resource.instances.erase( std::begin( found));

               log::internal::transaction << "remove dead process: " << death << std::endl;
               return;
            }
         }

         log::warning << "failed to find and remove dead instance: " << death << std::endl;
      }

      state::resource::Proxy& State::get_resource( state::resource::id::type rm)
      {
         auto found = common::range::find( resources, rm);

         if( ! found)
         {
            throw common::exception::invalid::Argument{ "failed to find resource"};
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
            throw common::exception::invalid::Argument{ "failed to find instance"};
         }
         return *found;
      }

      State::instance_range State::idle_instance( state::resource::id::type rm)
      {
         auto& resource = get_resource( rm);

         return common::range::find_if( resource.instances, state::filter::Idle{});
      }

      const state::resource::external::Proxy& State::get_external( state::resource::id::type rm) const
      {
         auto found = range::find_if( externals, [rm]( const state::resource::external::Proxy& p){
            return p.id == rm;
         });

         if( found)
         {
            return *found;
         }

         throw common::exception::invalid::Argument{ "failed to find external resource proxy", CASUAL_NIP( rm)};
      }

   } // transaction

} // casual


