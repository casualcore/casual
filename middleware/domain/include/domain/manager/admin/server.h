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
         constexpr std::string_view state = ".casual/domain/state";
         namespace scale
         {
            constexpr std::string_view aliases = ".casual/domain/scale/aliases";
         } // scale

         namespace restart
         {
            constexpr std::string_view aliases = ".casual/domain/restart/aliases";
            constexpr std::string_view groups = ".casual/domain/restart/groups";
         } // restart

         constexpr std::string_view shutdown = ".casual/domain/shutdown";

         namespace configuration
         {
            constexpr std::string_view get = ".casual/domain/configuration/get";
            constexpr std::string_view put = ".casual/domain/configuration/put";
            constexpr std::string_view post = ".casual/domain/configuration/post";
         } // configuration

         namespace environment
         {
            constexpr std::string_view set = ".casual/domain/environment/set";
            constexpr std::string_view unset = ".casual/domain/environment/unset";
         } // environment

      } // service::name

      common::server::Arguments services( manager::State& state);

   } // domain::manager::admin
} // casual


