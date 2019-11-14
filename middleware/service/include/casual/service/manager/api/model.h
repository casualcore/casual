//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#include <string>
#include <vector>
#include <iosfwd>

namespace casual
{
   namespace service
   {
      namespace manager
      {
         namespace api
         {
            inline namespace v1 
            {
               namespace model
               {
                  namespace uuid
                  {
                     using type = casual::platform::uuid::type;
                  } // uuid

                  namespace process
                  {
                     namespace id
                     {
                        using type = casual::platform::process::native::type;
                     } // id
                     
                     struct Handle 
                     {
                        id::type pid;
                        uuid::type ipc;
                     };
                  } // process

                  namespace instance
                  {
                     struct Sequential
                     {
                        enum class State : short
                        {
                           idle,
                           busy,
                        };

                        model::process::Handle process;
                        State state = State::busy;
                     };

                     struct Concurrent
                     {
                        process::Handle process;
                     };

                  } // instance

                  struct Metric
                  {
                     platform::size::type count = 0;
                     std::chrono::nanoseconds total = std::chrono::nanoseconds::zero();
                     struct 
                     {
                        std::chrono::nanoseconds min = std::chrono::nanoseconds::zero();
                        std::chrono::nanoseconds max = std::chrono::nanoseconds::zero();
                     } limit;
                  };

                  namespace service
                  {
                     struct Metric
                     {
                        model::Metric invoked;
                        model::Metric pending;

                        platform::time::point::type last = platform::time::point::limit::zero();
                        platform::size::type remote = 0;
                     };

                     namespace instance
                     {
                        struct Sequential
                        {
                           process::id::type pid;
                        };

                        struct Concurrent
                        {
                           process::id::type pid;
                           platform::size::type hops;
                        };

                     } // instance
                  } // service

                  struct Service
                  {
                     enum class Transaction : platform::size::type
                     {
                        automatic = 0,
                        join = 1,
                        atomic = 2,
                        none = 3,
                        branch,
                     };
                     friend std::ostream& operator << ( std::ostream& out, Transaction value);

                     std::string name;
                     std::chrono::nanoseconds timeout;
                     std::string category;
                     Transaction transaction = Transaction::automatic;

                     service::Metric metric;

                     struct
                     {
                        std::vector< service::instance::Sequential> sequential;
                        std::vector< service::instance::Concurrent> concurrent;
                     } instances;
                  };
                  

               } // model
               
               struct Model
               {
                  struct
                  {
                     std::vector< model::instance::Sequential> sequential;
                     std::vector< model::instance::Concurrent> concurrent;
                  } instances;

                  std::vector< model::Service> services;
               };

            } // v1
         } // api
      } // manager
   } // service
} // casual