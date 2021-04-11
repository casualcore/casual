//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "common/strong/type.h"

namespace casual
{
   namespace domain
   {
      namespace strong
      {
         using namespace common::strong;

         namespace detail
         {
            template< typename Tag>
            struct policy
            {
               static auto generate()
               {
                  static platform::size::type value{};
                  return ++value;
               }
            };

            template< typename Tag>
            using basic_id = common::strong::Type< platform::size::type, policy< Tag>>;
         } // detail

         namespace group
         {
            struct tag{};
            using id = detail::basic_id< tag>;
         } // server

         namespace server
         {
            struct tag{};
            using id = detail::basic_id< tag>;
         } // server

         namespace executable
         {
            struct tag{};
            using id = detail::basic_id< tag>;
         } // executable
         
      } // strong
   } // domain
} // casual