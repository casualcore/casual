//!
//! casual 
//!

#include "domain/manager/configuration.h"

#include "configuration/domain.h"
#include "domain/manager/manager.h"
#include "domain/transform.h"
#include "domain/manager/persistent.h"


namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace configuration
         {
            namespace local
            {
               namespace
               {
                  State state( const Settings& settings)
                  {
                     if( settings.configurationfiles.empty())
                     {
                        auto state = persistent::state::load();

                        //
                        // We don't need to prepare anything, we got the total state (hopefully)
                        //
                        state.mandatory_prepare = false;

                        //
                        // Make sure we set the domain-name.
                        //
                        common::domain::identity( common::domain::Identity{ state.configuration.name});


                        //
                        // We need to adjust 'next-id' so runtime configuration works.
                        //
                        {
                           auto max = range::max( state.executables);
                           if( max)
                           {
                              state::Executable::id_type::policy_type::value = max.front().id.value();
                           }
                        }

                        {
                           auto max = range::max( state.groups);
                           if( max)
                           {
                              state::Group::id_type::policy_type::value = max.front().id.value();
                           }
                        }

                        return state;
                     }
                     else
                     {
                        auto state = transform::state( casual::configuration::domain::get( settings.configurationfiles));

                        state.mandatory_prepare = ! settings.bare;

                        return state;
                     }
                  }

               } // <unnamed>
            } // local


            State state( const Settings& settings)
            {
               auto state = local::state( settings);

               if( settings.event_queue)
               {
                  common::message::event::subscription::Begin request;
                  request.process.queue = common::strong::ipc::id{ settings.event_queue};
                  state.event.subscription( request);
               }

               state.auto_persist = ! settings.no_auto_persist;

               return state;
            }

         } // configuration
      } // manager
   } // domain



} // casual
