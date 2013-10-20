//!
//! manager_state.cpp
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#include "transaction/manager/state.h"

#include "common/exception.h"
#include "common/trace.h"

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
                     std::shared_ptr< state::resource::Proxy> operator () ( const common::message::resource::Manager& value) const
                     {
                        common::Trace trace{ "transform::Resource"};

                        auto result = std::make_shared< state::resource::Proxy>();

                        result->id = value.id;
                        result->key = value.key;
                        result->openinfo = value.openinfo;
                        result->closeinfo = value.closeinfo;
                        result->concurency = value.instances;

                        common::logger::debug << "resource.openinfo: " << result->openinfo;
                        common::logger::debug << "resource.concurency: " << result->concurency;

                        return result;
                     }
                  };

               } // transform

            }
         } // local

         void configure( State& state, const common::message::transaction::Configuration& configuration)
         {
            {
               trace::Exit log( "transaction manager xa-switch configuration");

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
               trace::Exit log( "transaction manager resource configuration");

               std::transform(
                     std::begin( configuration.resources),
                     std::end( configuration.resources),
                     std::back_inserter( state.resources),
                     local::transform::Resource{});
            }

         }

         namespace filter
         {

            Instance::Instance( common::platform::pid_type pid)
                  : pid( pid)
            {
            }

            bool Instance::operator () ( const resource::Proxy::Instance& instance) const
            {
               return instance.id.pid == pid;
            }


            bool Started::operator () ( const std::shared_ptr< resource::Proxy::Instance>& instance) const
            {
               return instance->state > resource::Proxy::Instance::State::absent;
            }

            bool Started::operator () ( const std::shared_ptr< state::resource::Proxy>& proxy) const
            {
               return std::all_of( std::begin( proxy->instances), std::end( proxy->instances), Started{});
            }


            bool Running::operator () ( const std::shared_ptr< resource::Proxy::Instance>& instance) const
            {
               return instance->state == resource::Proxy::Instance::State::idle
                     || instance->state == resource::Proxy::Instance::State::busy;
            }

            bool Running::operator () ( const std::shared_ptr< state::resource::Proxy>& proxy) const
            {
               return std::any_of( std::begin( proxy->instances), std::end( proxy->instances), Running{});
            }


         } // filter


         namespace remove
         {
            void instance( common::platform::pid_type pid, State& state)
            {
               auto instance = state.instances.find( pid);

               if( instance != std::end( state.instances))
               {
                  auto& proxy = instance->second->proxy;
                  auto findIter = std::find(
                        std::begin( proxy->instances),
                        std::end( proxy->instances),
                        instance->second);

                  if( findIter != std::end( proxy->instances))
                  {
                     proxy->instances.erase( findIter);
                  }
                  else
                  {
                     throw exception::NotReallySureWhatToNameThisException( "inconsistency in state - action: abort");
                  }
               }
               else
               {
                  logger::error << "failed to find instance - pid: " << pid;
               }
            }
         } // remove
      } // state
   } // transaction

} // casual


