//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/buffer/type.h"
#include "common/flag.h"
#include "common/service/header.h"
#include "common/strong/id.h"

#include "common/flag/xatmi.h"

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace invoke
         {
            struct Service
            {
               std::string name;

               CASUAL_LOG_SERIALIZE({
                  CASUAL_SERIALIZE( name);
               })
            };

            struct Parameter
            {
               //! these flags represent the possible flags that a service can be
               //! invoked with (tpservice template).
               enum class Flag : long
               {
                  conversation = std::to_underlying( flag::xatmi::Flag::conversation),
                  in_transaction = std::to_underlying( flag::xatmi::Flag::in_transaction),
                  no_reply = std::to_underlying( flag::xatmi::Flag::no_reply),
                  send_only = std::to_underlying( flag::xatmi::Flag::send_only),
                  receive_only = std::to_underlying( flag::xatmi::Flag::receive_only),
               };
               using Flags = common::Flags< Flag>;

               Parameter() = default;
               Parameter( buffer::Payload&& payload) : payload( std::move( payload)) {}

               Parameter( Parameter&&) noexcept = default;
               Parameter& operator = (Parameter&&) noexcept = default;

               Flags flags;
               Service service;
               std::string parent;
               buffer::Payload payload;
               strong::conversation::descriptor::id descriptor;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( flags);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( parent);
                  CASUAL_SERIALIZE( payload);
                  CASUAL_SERIALIZE( descriptor);
               )
            };

            struct Result
            {
               enum class Transaction : int
               {
                  commit = std::to_underlying( flag::xatmi::Return::success),
                  rollback = std::to_underlying( flag::xatmi::Return::fail)
               };

               Result() = default;
               Result( buffer::Payload&& payload) : payload( std::move( payload)) {}

               buffer::Payload payload;
               long code = 0;
               Transaction transaction = Transaction::commit;

               friend std::ostream& operator << ( std::ostream& out, Transaction value);
               friend std::ostream& operator << ( std::ostream& out, const Result& value);
            };

            struct Forward
            {
               Parameter parameter;
            };

         } // invoke
      } // service
   } // common
} // casual


