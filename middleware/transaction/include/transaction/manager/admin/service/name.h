//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <string_view>

namespace casual
{
   namespace transaction::manager
   {
      namespace admin
      {
         namespace service::name
         {
            constexpr std::string_view state = ".casual/transaction/state";

            namespace scale::resource
            {
               constexpr std::string_view proxies = ".casual/transaction/scale/resource/proxies";
            } // update

         } // service::name

      } // admin

   } // transaction::manager
} // casual

