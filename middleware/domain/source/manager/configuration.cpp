//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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

                        // We don't need to prepare anything, we got the total state (hopefully)
                        state.mandatory_prepare = false;

                        // Make sure we set the domain-name.
                        common::domain::identity( common::domain::Identity{ state.configuration.name});

                        // We need to adjust 'next-id' so runtime configuration works.
                        {
                           auto adjust_id = []( const auto& range){
                              auto max = algorithm::max( range);
                              if( max)
                              {
                                 decltype( max.front().id)::policy_type::sequence::value = max.front().id.value() + 1;
                              }
                           };
                           adjust_id( state.executables);
                           adjust_id( state.groups);
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

               if( settings.event())
               {
                  common::message::event::subscription::Begin request;
                  request.process.ipc = settings.event();
                  state.event.subscription( request);
               }

               state.persist = settings.persist;

               return state;
            }

         } // configuration
      } // manager
   } // domain



} // casual
