//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/manager/state.h"

#include "gateway/message.h"
#include "gateway/manager/handle.h"
#include "gateway/common.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               auto path( std::string_view name)
               {
                  return string::compose( directory::name::base( process::path()), '/', name);
               }
            } // <unnamed>
         } // local
         namespace state
         {
            namespace executable
            {
               std::string path( const outbound::Group& value)
               {
                  if( value.configuration.connect == decltype( value.configuration.connect)::reversed)
                     return local::path( "casual-gateway-outbound-reverse-group");
                  return local::path( "casual-gateway-outbound-group");
               }

               std::string path( const inbound::Group& value)
               {
                  if( value.configuration.connect == decltype( value.configuration.connect)::reversed)
                     return local::path( "casual-gateway-inbound-reverse-group");
                  return local::path( "casual-gateway-inbound-group");
               }
            
            } // executable

            std::ostream& operator << ( std::ostream& out, Runlevel value)
            {
               switch( value)
               {
                  case Runlevel::startup: { return out << "startup";}
                  case Runlevel::running: { return out << "running";}
                  case Runlevel::shutdown: { return out << "shutdown";}
               }
               return out << "unknown";
            }

         } // state

         bool State::done() const
         {
            using Runlevel = decltype( runlevel());

            if( runlevel() <= Runlevel::running)
               return false;

            return inbound.groups.empty() && outbound.groups.empty();
         }


      } // manager
   } // gateway
} // casual
