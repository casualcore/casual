//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "common/serialize/macro.h"

#include "file/code.h"

#include "common/transaction/id.h"

#include <filesystem>

namespace casual
{

   namespace file::message
   {

      namespace reserve
      {
         using base_request = common::message::basic_request< common::message::Type::file_reserve_request>;
         struct Request : base_request
         {
            using base_request::base_request;

            common::transaction::ID trid;
            std::filesystem::path path;
            bool wait{true};

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_request::serialize( archive);
               CASUAL_SERIALIZE( trid);
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( wait);
            )
         };

         using base_reply = common::message::basic_reply< common::message::Type::file_reserve_reply>;
         struct Reply : base_reply
         {
            std::filesystem::path path;
            file::code code{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_reply::serialize( archive);
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( code);
            )
         };
        
      } // reserve
      
   } // file::message

   namespace common::message::reverse
   {
      template<>
      struct type_traits< casual::file::message::reserve::Request> : detail::type< casual::file::message::reserve::Reply> {};
   } // common::message::reverse
   
} // casual