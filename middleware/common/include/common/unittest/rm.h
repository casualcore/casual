//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/transaction/id.h"
#include "common/code/xa.h"

#include "common/serialize/macro.h"


namespace casual
{
   namespace common::unittest::rm
   {
      namespace state
      {
         enum class Invoke : short 
         {
            xa_open_entry,
            xa_close_entry,
            xa_start_entry,
            xa_end_entry,
            xa_rollback_entry,
            xa_prepare_entry,
            xa_commit_entry,
            xa_recover_entry,
            xa_forget_entry,
            xa_complete_entry
         };
         std::string_view description( Invoke value) noexcept;

         struct Transactions
         {
            transaction::ID current;
            std::vector< transaction::ID> all;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( current);
               CASUAL_SERIALIZE( all);
            )
         };

      } // state

      struct State
      {
         strong::resource::id id;
         state::Transactions transactions;
         std::vector< code::xa> errors;
         std::vector< state::Invoke> invocations;

         std::optional< platform::time::unit> sleep_prepare;
         std::optional< platform::time::unit> sleep_commit;
         std::optional< platform::time::unit> sleep_rollback;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( id);
            CASUAL_SERIALIZE( transactions);
            CASUAL_SERIALIZE( errors);
            CASUAL_SERIALIZE( invocations);
            CASUAL_SERIALIZE( sleep_prepare);
            CASUAL_SERIALIZE( sleep_commit);
            CASUAL_SERIALIZE( sleep_rollback);
         )
      };

      //! emulate a resource dynamic registration
      void registration( strong::resource::id id); 

      namespace state
      {
         const State& get( strong::resource::id id);
         void clear();
      } // state

   } // common::unittest::rm
} // casual

extern "C"
{
   extern struct xa_switch_t casual_mockup_xa_switch_static;

   extern struct xa_switch_t casual_mockup_xa_switch_dynamic;
}

