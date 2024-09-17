//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/domain.h"

#include <vector>
#include <filesystem>

namespace casual
{
   namespace file::manager::admin::model
   {
      inline namespace v1 
      {
         struct Request
         {
            common::process::Handle process;
            platform::binary::type trid;
            std::filesystem::path path;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( trid);
               CASUAL_SERIALIZE( path);
            )
         };

         struct State
         {
            std::vector< Request> working;
            std::vector< Request> pending;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( working);
               CASUAL_SERIALIZE( pending);
            )
         };

      } // v1

   } // file::manager::admin::model
} // casual


