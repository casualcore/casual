//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/fanout/state.h"

#include "queue/common/log.h"

namespace casual
{
   namespace queue::fanout
   {
      namespace state
      {
         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
               case Runlevel::error: return "error";
            }
            return "<unknown>";
         }

         namespace instance
         {
            std::string_view description( Life value)
            {
               switch( value)
               {
                  case Life::restart: return "restart";
                  case Life::stop: return "stop";
               }
               return "<unknown>";
            }

         } // instance
         
      } // state

      casual::configuration::model::queue::fanout::Group State::configuration_model() const
      {
         Trace trace{ "queue::fanout::State::configuration_model"};

         casual::configuration::model::queue::fanout::Group result;
         result.alias = configuration.alias;
         result.note = configuration.note;
         result.memberships = configuration.memberships;


         std::ranges::transform( profiles.values(), std::back_inserter( result.queues), []( const auto& profile)
         {
            return profile.configuration;
         });

         return result;
      }

      bool State::done() const 
      { 
         if( runlevel == state::Runlevel::running)
            return false;

         return instances.empty();
      }
      
   } // queue::fanout
} // casual
