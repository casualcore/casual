//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <string>
#include <xa.h>

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         namespace resource
         {
            struct Link
            {
               inline Link( std::string key, xa_switch_t* xa) : key{ std::move( key)}, xa{ xa}{}
               inline Link( std::string key, std::string name, xa_switch_t* xa) : key{ std::move( key)}, name{ std::move( name)}, xa{ xa}{}

               std::string key;
               std::string name;
               xa_switch_t* xa = nullptr;
            };
         } // resource
      } // transaction
   } // common
} // casual