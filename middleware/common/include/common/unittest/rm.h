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
   namespace common
   {
      namespace unittest
      {
         namespace rm
         {
            struct State
            {
               struct Transactions
               {
                  transaction::ID current;
                  std::vector< transaction::ID> all;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( current);
                     CASUAL_SERIALIZE( all);
                  )

               } transactions;

               std::vector< code::xa> errors;

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
               friend std::string_view description( Invoke value);

               std::vector< Invoke> invocations;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( transactions);
                  CASUAL_SERIALIZE( errors);
                  CASUAL_SERIALIZE( invocations);
               )
            };

            //! emulate a resource dynamic registration
            void registration( strong::resource::id id); 

            const State& state( strong::resource::id id);
            void clear();
            
         } // rm

      } // unittest
   } // common
} // casual

extern "C"
{
   extern struct xa_switch_t casual_mockup_xa_switch_static;

   extern struct xa_switch_t casual_mockup_xa_switch_dynamic;
}

