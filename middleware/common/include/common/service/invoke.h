//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/buffer/type.h"
#include "common/flag.h"
#include "common/service/header.h"

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
               enum class Flag : long
               {
                  // What flags need to be defined here? What are these flags representing?
                  // The type is used (at least) for "argument.flags" when invoking
                  // a service. At least "now" the value in argument.flags is the caller
                  // supplied flags (in for examle a tpconnect or tpcall). When invoking a
                  // service the caller TPNOREPLY is of interest. Also TPSENDONLY
                  // and/or TPRECVONLY for conversational services.
                  // The need for no_transaction is questionable. The caller TPNOTRAN
                  // flag controls if the caller transaction is passed on to the service
                  // or not. If the called service has a transaction active is another
                  // matter. A new transaction can be started even when the callers
                  // transaction was not passed on (e.g. auto). 
                  no_transaction = cast::underlying( flag::xatmi::Flag::no_transaction),
                  send_only = cast::underlying( flag::xatmi::Flag::send_only),
                  receive_only = cast::underlying( flag::xatmi::Flag::receive_only),
                  no_time = cast::underlying( flag::xatmi::Flag::no_time),
                  // added:
                  no_reply = cast::underlying( flag::xatmi::Flag::no_reply),
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
               platform::descriptor::type descriptor = 0;

               CASUAL_LOG_SERIALIZE({
                  CASUAL_SERIALIZE( flags);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( parent);
                  CASUAL_SERIALIZE( payload);
                  CASUAL_SERIALIZE( descriptor);
               })
            };

            struct Result
            {
               enum class Transaction : int
               {
                  commit = cast::underlying( flag::xatmi::Return::success),
                  rollback = cast::underlying( flag::xatmi::Return::fail)
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


