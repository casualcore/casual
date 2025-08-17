//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "domain/manager/admin/model.h"

#include <regex>

namespace casual
{
   namespace domain::unittest
   {
      namespace home
      {
         //! creates a temporary directory that is used as the domain home.
         //! Sets the environment variable `CASUAL_DOMAIN_HOME` to the path of the directory.
         //! and unsets it when the object is destroyed.
         struct Directory
         {
            Directory();
            ~Directory();

            Directory( Directory&&) noexcept = default;
            Directory& operator = ( Directory&&)  noexcept = default;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_home);
            )

         private:
            common::unittest::directory::temporary::Scoped m_home;
         };
         
      } // home

      manager::admin::model::State state();

      common::process::Handle server( const manager::admin::model::State& state, std::string_view alias, platform::size::type index = 0);
      common::strong::process::id executable( const manager::admin::model::State& state, std::string_view alias, platform::size::type index = 0);

      namespace instances
      {
         bool count( const manager::admin::model::State& model, std::string_view alias, platform::size::type count);

         namespace has
         {
            bool state( const manager::admin::model::State& model, std::string_view alias, manager::admin::model::instance::State state);
            
         } // has
         
      } // instances         


      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);

         namespace predicate
         {
            namespace alias::has
            {
               auto instances( std::string_view expression, platform::size::type count) -> common::unique_function< bool( const manager::admin::model::State&)>;

               auto state_count( std::string_view alias, manager::admin::model::instance::State state, platform::size::type count) -> common::unique_function< bool( const manager::admin::model::State&)>;


            } // alias::has
            
         } // predicate
      } // fetch

   } // domain::unittest
} // casual
