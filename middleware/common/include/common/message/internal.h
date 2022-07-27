//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"

#include <filesystem>

namespace casual
{
   namespace common::message::internal
   {
      namespace dump
      {
         using State = message::basic_message< Type::internal_dump_state>;
      } // dump

      namespace configure
      {
         using base_type = message::basic_message< Type::internal_configure_log>;
         struct Log : base_type
         {
            std::filesystem::path path;

            struct
            {
               std::string inclusive;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( inclusive);
               )
            } expression;


            CASUAL_CONST_CORRECT_SERIALIZE(
               base_type::serialize( archive);
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( expression);
            );
         };
      } // configure
      

   } // common::message::internal
} // casual