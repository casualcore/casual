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
         struct CLI 
         {
            CLI();
            ~CLI();

            common::argument::Group options() &;
         private:
            struct Implementation;
            common::move::basic_pimpl< Implementation> m_implementation;
         };
      } // admin
      
   } // configuration
} // casual
