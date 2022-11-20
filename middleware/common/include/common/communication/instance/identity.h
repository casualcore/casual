//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/uuid.h"
#include "common/serialize/macro.h"

#include <string_view>

namespace casual
{
   namespace common::communication::instance
   {
      //! holds an instance id and corresponding environment variables
      struct Identity 
      {
         Uuid id;
         std::string_view environment;

         inline friend bool operator == ( const Identity& lhs, const Uuid& rhs) noexcept { return lhs.id == rhs;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( id);
            CASUAL_SERIALIZE( environment);
         )
      };

      namespace identity
      {
         namespace service
         {
            inline const Identity manager{ 0xf58e0b181b1b48eb8bba01b3136ed82a_uuid, "CASUAL_SERVICE_MANAGER_PROCESS"};
         } // service

         namespace gateway
         {
             inline const Identity manager{ 0xb9624e2f85404480913b06e8d503fce5_uuid, "CASUAL_GATEWAY_MANAGER_PROCESS"};
         } // domain

         namespace queue
         {
            inline const Identity manager{ 0xc0c5a19dfc27465299494ad7a5c229cd_uuid, "CASUAL_QUEUE_MANAGER_PROCESS"};
         } // queue

         namespace transaction
         {
            inline const Identity manager{ 0x5ec18cd92b2e4c60a927e9b1b68537e7_uuid, "CASUAL_TRANSACTION_MANAGER_PROCESS"};
         } // transaction

      } // identity

   } //common::communication::instance
} // casual
