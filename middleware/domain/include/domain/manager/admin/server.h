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
                  constexpr auto state = ".casual/domain/state";
                  namespace scale
                  {
                     constexpr auto instances = ".casual/domain/scale/instances";
                  } // scale

                  constexpr auto shutdown = ".casual/domain/shutdown";

                  namespace configuration
                  {
                     constexpr auto persist = ".casual/domain/configuration/persist";

                  } // configuration

                  namespace set
                  {
                     constexpr auto environment = ".casual/domain/set/environment";
                  } // set

               } // name
            } // service

            common::server::Arguments services( manager::State& state);


         } // admin
      } // manager
   } // admin

} // casual


