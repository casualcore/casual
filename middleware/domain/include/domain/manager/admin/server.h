//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/server/argument.h"

namespace casual
{
   namespace domain::manager
   {
      struct State;
   }

   namespace domain::manager::admin
   {
      namespace service::name
      {
         constexpr auto state = ".casual/domain/state";
         namespace scale
         {
            constexpr auto aliases = ".casual/domain/scale/aliases";
         } // scale

         namespace restart
         {
            constexpr auto aliases = ".casual/domain/restart/aliases";
            constexpr auto groups = ".casual/domain/restart/groups";
         } // restart

         constexpr auto shutdown = ".casual/domain/shutdown";

         namespace configuration
         {
            constexpr auto get = ".casual/domain/configuration/get";
            constexpr auto put = ".casual/domain/configuration/put";
            constexpr auto post = ".casual/domain/configuration/post";
         } // configuration

         namespace environment
         {
            constexpr auto set = ".casual/domain/environment/set";
         } // environment

      } // service::name

      common::server::Arguments services( manager::State& state);

   } // domain::manager::admin
} // casual


