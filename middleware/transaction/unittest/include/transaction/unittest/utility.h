//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "transaction/manager/admin/model.h"

#include "common/code/tx.h"

namespace casual
{
   namespace transaction::unittest
   {
      manager::admin::model::State state();

      //! performs a commit (request/reply) to TM for the given `trid`
      common::code::tx commit( const common::transaction::ID& trid);

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);

         namespace predicate
         {
            inline auto transactions( platform::size::type count)
            {
               return [ count]( auto& state){ return common::range::size( state.transactions) == count;};
            }
         } // predicate
      }
      
   } // transaction::unittest
   
} // casual