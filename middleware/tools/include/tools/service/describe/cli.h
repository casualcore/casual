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
   namespace tools 
   {
      namespace service
      {
         namespace describe
         {

            struct cli 
            {
               cli();
               ~cli();

               common::argument::Option options() &;

            private:
               struct Implementation;
               common::move::Pimpl< Implementation> m_implementation;
            };

         } // describe  
      } // service
   } // tools
} // casual
