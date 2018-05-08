//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "common/server/argument.h"

namespace casual
{
   namespace domain
   {

      namespace manager
      {
         struct State;

         namespace admin
         {
            namespace service
            {
               namespace name
               {
                  constexpr auto state() { return ".casual/domain/state";}
                  namespace scale
                  {
                     constexpr auto instances() { return ".casual/domain/scale/instances";}
                  } // scale

                  constexpr auto shutdown() { return ".casual/domain/shutdown";}

                  namespace configuration
                  {
                     constexpr auto persist() { return ".casual/domain/configuration/persist";}

                  } // configuration

               } // name
            } // service

            common::server::Arguments services( manager::State& state);


         } // admin
      } // manager
   } // admin

} // casual


