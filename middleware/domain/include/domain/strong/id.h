//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "common/value/id.h"

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
            using basic_id = common::value::basic_id< platform::size::type, common::value::id::policy::unique_initialize< platform::size::type, Tag, 1>>;
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