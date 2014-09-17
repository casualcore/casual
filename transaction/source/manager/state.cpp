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
               namespace filter
               {
                  struct Resource
                  {
                     bool operator () ( const config::domain::Group& value) const
                     {
                        return ! value.resource.key.empty();
                     }
                  };
               } // filter

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

         void configure( State& state, const common::message::transaction::manager::Configuration& configuration)
         {

            common::environment::domain::name( configuration.domain);

            {
               trace::internal::Scope trace( "transaction manager xa-switch configuration");

               auto resources = config::xa::switches::get();

               for( auto& resource : resources)
               {
                  if( ! state.xaConfig.emplace( resource.key, std::move( resource)).second)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "multiple keys in resource config");
                  }
               }
            }

            //
            // configure resources
            //
            {
               trace::internal::Scope trace( "transaction manager resource configuration");

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
               return instance.state == resource::Proxy::Instance::State::idle
                     || instance.state == resource::Proxy::Instance::State::busy;
            }

         } // filter


         namespace remove
         {
            void instance( common::platform::pid_type pid, State& state)
            {

               for( auto& resource : state.resources)
               {
                  auto found = common::range::find_if(
                     common::range::make( resource.instances),
                     filter::Instance{ pid});

                  if( ! found.empty())
                  {
                     resource.instances.erase( found.first);
                     log::internal::debug << "remove instance: " << pid << std::endl;
                     return;
                  }
               }

               log::warning << "failed to find and remove instance - pid: " << pid << std::endl;
            }
         } // remove
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

      std::size_t State::instances() const
      {
         std::size_t result = 0;

         for( auto& resource : resources)
         {
            result += resource.instances.size();
         }
         return result;
      }


   } // transaction

} // casual


