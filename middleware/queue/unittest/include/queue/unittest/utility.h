//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "queue/manager/admin/model.h"

namespace casual
{
   namespace queue::unittest
   {
      queue::manager::admin::model::State state();

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);
      }

      std::vector< manager::admin::model::Message> messages( const std::string& queue);

      namespace scale
      {
         void aliases( const std::vector< manager::admin::model::scale::Alias>& aliases);

         namespace all::forward
         {
            void aliases( platform::size::type instances);
         } // all::forward

      } // scale
               
   } // queue::unittest
} // casual