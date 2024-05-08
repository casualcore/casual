//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/argument.h"
#include "common/pimpl.h"

namespace casual
{
   namespace configuration
   {
      namespace admin
      {
         //! exposed for domain cli, to keep compatibility... 
         //! @remove in 2.0
         namespace deprecated
         {
            common::argument::Option get();
            common::argument::Option post();
            common::argument::Option put();
            common::argument::Option edit();

         } // deprecated


         struct CLI 
         {
            CLI();
            ~CLI();

            common::argument::Group options() &;
         private:
            struct Implementation;
            common::move::Pimpl< Implementation> m_implementation;
         };
      } // admin
      
   } // configuration
} // casual
