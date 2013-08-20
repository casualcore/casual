//!
//! manager_state.cpp
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#include "transaction/manager_state.h"

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

                  struct Group
                  {
                     std::shared_ptr< state::resource::Proxy> operator () ( const config::domain::Group& value) const
                     {
                        auto result = std::make_shared< state::resource::Proxy>();

                        result->key = value.resource.key;
                        result->openinfo = value.resource.openinfo;
                        result->closeinfo = value.resource.closeinfo;
                        result->concurency = std::stoul( value.resource.instances);

                        return result;
                     }
                  };

               } // transform

            }
         } // local

         void configure( State& state)
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

               auto domain = config::domain::get();

               auto resourcesEnd = std::partition(
                     std::begin( domain.groups), std::end( domain.groups), local::filter::Resource());

               std::transform(
                     std::begin( domain.groups),
                     resourcesEnd,
                     std::back_inserter( state.resources), local::transform::Group());
            }

         }


         void remove::instance( common::platform::pid_type pid, State& state)
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
      } // state
   } // transaction

} // casual


