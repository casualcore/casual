//!
//! manager_state.cpp
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#include "transaction/manager/state.h"

#include "common/exception.h"
#include "common/algorithm.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/environment.h"


#include "config/domain.h"
#include "config/xa_switch.h"


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace state
      {
         namespace local
         {
            namespace
            {
               namespace transform
               {
                  struct Resource
                  {
                     state::resource::Proxy operator () ( const message::transaction::resource::Manager& value) const
                     {
                        trace::internal::Scope trace{ "transform::Resource"};

                        state::resource::Proxy result;

                        result.id = value.id;
                        result.key = value.key;
                        result.openinfo = value.openinfo;
                        result.closeinfo = value.closeinfo;
                        result.concurency = value.instances;

                        log::internal::debug << "resource.openinfo: " << result.openinfo << std::endl;
                        log::internal::debug << "resource.concurency: " << result.concurency << std::endl;

                        return result;
                     }
                  };

               } // transform

            }
         } // local


         Statistics::Statistics() :  min{ std::chrono::microseconds::max()}, max{ 0}, total{ 0}, invoked{ 0}
         {

         }

         void Statistics::start( common::platform::time_point start)
         {
            m_start = start;
         }
         void Statistics::end( common::platform::time_point end)
         {
            time( m_start, end);
         }

         void Statistics::time( common::platform::time_point start, common::platform::time_point end)
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
               switch( state)
               {
                  case State::busy:
                  {
                     assert( m_state != State::busy);
                     if( m_state == State::idle)
                     {
                        statistics.roundtrip.start( platform::clock_type::now());
                     }
                     break;
                  }
                  case State::idle:
                  {
                     assert( m_state != State::idle);
                     if( m_state == State::busy)
                     {
                        statistics.roundtrip.end( platform::clock_type::now());
                     }
                     break;
                  }
                  default:
                  {
                     break;
                  }
               }

               if( m_state != State::shutdown)
               {
                  m_state = state;
               }
            }

            Proxy::Instance::State Proxy::Instance::state() const
            {
               return m_state;
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
                        case Proxy::Instance::State::startupError: return "startupError";
                     }
                     return "<unknown>";
                  };

               return out << state_switch();
            }
         } // resource

         void configure( State& state, const common::message::transaction::manager::Configuration& configuration)
         {

            common::environment::domain::name( configuration.domain);

            {
               trace::internal::Scope trace( "transaction manager xa-switch configuration", log::internal::transaction);

               auto resources = config::xa::switches::get();

               for( auto& resource : resources)
               {
                  if( ! state.xaConfig.emplace( resource.key, std::move( resource)).second)
                  {
                     throw exception::invalid::Configuration( "multiple keys in resource config: " + resource.key);
                  }
               }
            }

            //
            // configure resources
            //
            {
               trace::internal::Scope trace( "transaction manager resource configuration", log::internal::transaction);

               std::transform(
                     std::begin( configuration.resources),
                     std::end( configuration.resources),
                     std::back_inserter( state.resources),
                     local::transform::Resource{});
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
            case XA_HEURHAZ: return Result::cXA_HEURHAZ; break;
            case XA_HEURMIX: return Result::cXA_HEURMIX; break;
            case XA_HEURCOM: return Result::cXA_HEURCOM; break;
            case XA_HEURRB: return Result::cXA_HEURRB; break;
            case XAER_RMFAIL: return Result::cXAER_RMFAIL; break;
            case XAER_RMERR: return Result::cXAER_RMERR; break;
            case XA_RBINTEGRITY: return Result::cXA_RBINTEGRITY; break;
            case XA_RBCOMMFAIL: return Result::cXA_RBCOMMFAIL; break;
            case XA_RBROLLBACK: return Result::cXA_RBROLLBACK; break;
            case XA_RBOTHER: return Result::cXA_RBOTHER; break;
            case XA_RBDEADLOCK: return Result::cXA_RBDEADLOCK; break;
            case XAER_PROTO: return Result::cXAER_PROTO; break;
            case XA_RBPROTO: return Result::cXA_RBPROTO; break;
            case XA_RBTIMEOUT: return Result::cXA_RBTIMEOUT; break;
            case XA_RBTRANSIENT: return Result::cXA_RBTRANSIENT; break;
            case XAER_INVAL: return Result::cXAER_INVAL; break;
            case XA_NOMIGRATE: return Result::cXA_NOMIGRATE; break;
            case XAER_OUTSIDE: return Result::cXAER_OUTSIDE; break;
            case XAER_NOTA: return Result::cXAER_NOTA; break;
            case XAER_ASYNC: return Result::cXAER_ASYNC; break;
            case XA_RETRY: return Result::cXA_RETRY; break;
            case XAER_DUPID: return Result::cXAER_DUPID; break;
            case XA_OK: return Result::cXA_OK; break;
            case XA_RDONLY: return Result::cXA_RDONLY; break;
         }
         return Result::cXAER_RMFAIL;
      }

      int Transaction::Resource::convert( Result value)
      {
         switch( value)
         {
            case Result::cXA_HEURHAZ: return XA_HEURHAZ; break;
            case Result::cXA_HEURMIX: return XA_HEURMIX; break;
            case Result::cXA_HEURCOM: return XA_HEURCOM; break;
            case Result::cXA_HEURRB: return XA_HEURRB; break;
            case Result::cXAER_RMFAIL: return XAER_RMFAIL; break;
            case Result::cXAER_RMERR: return XAER_RMERR; break;
            case Result::cXA_RBINTEGRITY: return XA_RBINTEGRITY; break;
            case Result::cXA_RBCOMMFAIL: return XA_RBCOMMFAIL; break;
            case Result::cXA_RBROLLBACK: return XA_RBROLLBACK; break;
            case Result::cXA_RBOTHER: return XA_RBOTHER; break;
            case Result::cXA_RBDEADLOCK: return XA_RBDEADLOCK; break;
            case Result::cXAER_PROTO: return XAER_PROTO; break;
            case Result::cXA_RBPROTO: return XA_RBPROTO; break;
            case Result::cXA_RBTIMEOUT: return XA_RBTIMEOUT; break;
            case Result::cXA_RBTRANSIENT: return XA_RBTRANSIENT; break;
            case Result::cXAER_INVAL: return XAER_INVAL; break;
            case Result::cXA_NOMIGRATE: return XA_NOMIGRATE; break;
            case Result::cXAER_OUTSIDE: return XAER_OUTSIDE; break;
            case Result::cXAER_NOTA: return XAER_NOTA; break;
            case Result::cXAER_ASYNC: return XAER_ASYNC; break;
            case Result::cXA_RETRY: return XA_RETRY; break;
            case Result::cXAER_DUPID: return XAER_DUPID; break;
            case Result::cXA_OK: return XA_OK; break;
            case Result::cXA_RDONLY: return XA_RDONLY; break;
         }
         return XAER_RMFAIL;
      }

      void Transaction::Resource::setResult( int value)
      {
         result = convert( value);
      }


      Transaction::Resource::Stage Transaction::stage() const
      {
         Resource::Stage result = Resource::Stage::cNotInvolved;

         for( auto& resource : resources)
         {
            if( result > resource.stage)
               result = resource.stage;
         }
         return result;
      }

      Transaction::Resource::Result Transaction::results() const
      {
         auto result = Resource::Result::cXA_RDONLY;

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
         return out << "{trid: " << value.trid << ", resources: " << common::range::make( value.resources) << "}";
      }

      State::State( const std::string& database) : log( database) {}


      bool State::pending() const
      {
         return ! persistentReplies.empty();
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

      std::vector< common::platform::pid_type> State::processes() const
      {
         std::vector< common::platform::pid_type> result;

         for( auto& resource : resources)
         {
            for( auto& instance : resource.instances)
            {
               result.push_back( instance.process.pid);
            }
         }
         return result;
      }

      void State::process( common::process::lifetime::Exit death)
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
               resource.instances.erase( found.first);

               log::internal::transaction << "remove dead process: " << death << std::endl;
               return;
            }
         }

         log::warning << "failed to find and remove dead instance: " << death << std::endl;
      }

      state::resource::Proxy& State::get_resource( common::platform::resource::id_type rm)
      {
         auto found = common::range::find( resources, rm);

         if( ! found)
         {
            throw common::exception::invalid::Argument{ "failed to find resource"};
         }
         return *found;
      }

      state::resource::Proxy::Instance& State::get_instance( common::platform::resource::id_type rm, common::platform::pid_type pid)
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

      State::instance_range State::idle_instance( common::platform::resource::id_type rm)
      {
         auto& resource = get_resource( rm);

         return common::range::find_if( resource.instances, state::filter::Idle{});
      }

      bool operator < ( const State::Deadline& lhs, const State::Deadline& rhs)
      {
         return lhs.deadline < rhs.deadline;
      }


   } // transaction

} // casual


