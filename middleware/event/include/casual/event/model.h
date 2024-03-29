//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include "casual/xa.h"

namespace casual
{
   namespace event
   {
      namespace model
      {
         // deprecated. 
         namespace v1 
         {
            namespace uuid
            {
               using type = platform::uuid::type;
            } // uuid

            namespace process
            {
               struct Handle 
               {
                  uuid::type ipc{};
                  platform::process::native::type pid{};
               };
            } // process

            namespace transaction
            {
               struct ID
               {
                  ::XID xid{};
                  process::Handle owner;
               };
            } // transaction

            namespace service
            {
               struct Call
               {
                  struct Metric
                  {
                     struct
                     {
                        std::string name;
                        std::string parent;
                     } service;

                     process::Handle process;
                     uuid::type execution;
                     
                     //! @attention not yet assigned 
                     struct 
                     {
                        transaction::ID id;
                     } transaction;
                     
                     platform::time::point::type start{};
                     platform::time::point::type end{};

                     platform::time::unit pending{};

                     // outcome of the service call
                     int code{};
                  };

                  std::vector< Metric> metrics;
               };           

            } // service 
         } // v1

         inline namespace v2
         {
            namespace uuid = v1::uuid;
            namespace process = v1::process;
            namespace transaction = v1::transaction;
            namespace service
            {
               struct Call
               {
                  struct Metric
                  {
                     
                     struct
                     {
                        enum class Type : long
                        {
                           sequential = 1,
                           // in practice an "outbound" service
                           concurrent = 2,
                        };

                        std::string name;
                        std::string parent;
                        Type type = Type::sequential;
                     } service;

                     process::Handle process;
                     uuid::type execution;
                     
                     //! @attention not yet assigned 
                     struct 
                     {
                        transaction::ID id;
                     } transaction;
                     
                     platform::time::point::type start{};
                     platform::time::point::type end{};

                     platform::time::unit pending{};

                     // outcome of the service call
                     int code{};
                  };

                  std::vector< Metric> metrics;
               };           

            } // service 
         } // v2
      } // model
   } // event
} // casual