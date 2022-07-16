//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/argument.h"
#include "common/pimpl.h"

namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace admin
         {
            struct CLI 
            {
               CLI();
               ~CLI();

               common::argument::Group options() &;
               std::vector< std::tuple< std::string, std::string>> information() &;

            private:
               struct Implementation;
               common::move::Pimpl< Implementation> m_implementation;
            };
         } // admin
      } // manager  
   } // transaction
} // casual
