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
   namespace domain::manager::admin
   {
      struct cli 
      {
         cli();
         ~cli();

         common::argument::Group options() &;

         std::vector< std::tuple< std::string, std::string>> information() &;

      private:
         struct Implementation;
         common::move::basic_pimpl< Implementation> m_implementation;
      };

   } // domain::manager::admin
} // casual
