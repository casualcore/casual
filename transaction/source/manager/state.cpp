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
                     state::resource::Proxy operator () ( const common::message::resource::Manager& value) const
                     {
                        common::Trace trace{ "transform::Resource"};

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

         void configure( State& state, const common::message::transaction::Configuration& configuration)
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
   } // transaction

} // casual


