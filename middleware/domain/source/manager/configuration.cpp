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
                           auto max = range::max( state.executables, []( const state::Executable& l, const state::Executable& r){ return l.id < r.id;});
                           if( max)
                           {
                              state::Executable::set_next_id( max.front().id + 1);
                           }
                        }

                        {
                           auto max = range::max( state.groups, []( const state::Group& l, const state::Group& r){ return l.id < r.id;});
                           if( max)
                           {
                              state::Group::set_next_id( max.front().id + 1);
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

               state.auto_persist = ! settings.no_auto_persist;

               return state;
            }

         } // configuration
      } // manager
   } // domain



} // casual
