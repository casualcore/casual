//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "tools/service/call/cli.h"
#include "common/argument.h"
#include "common/optional.h"
#include "common/service/call/context.h"
#include "common/buffer/type.h"
#include "common/exception/handle.h"


#include "service/manager/admin/api.h"


#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <cstring>

#include "xatmi.h"

namespace casual 
{
   using namespace common;

   namespace local
   {
      namespace
      {
         namespace service
         {
            auto state()
            {
               auto result = casual::service::manager::admin::api::state();

               return common::algorithm::transform( result.services, []( auto& s){
                  return std::move( s.name);
               });
            }
         }

         void call( const std::string& service)
         {
            // Read buffer from stdin
            auto payload = buffer::payload::binary::stream( std::cin);

            auto reply = common::service::call::context().sync( service, buffer::payload::Send{ payload}, {});

            // Print the result 
            buffer::payload::binary::stream( reply.buffer, std::cout);
         }

         auto call_complete = []( auto values, bool help) -> std::vector< std::string>
         {
            if( help) 
               return { "<service>"};

            try 
            {
               return service::state();
            }
            catch( ...)
            {
               return { "<value>"};
            }
         };
      } // <unnamed>
   } // local

   namespace tools 
   {
      namespace service
      {
         namespace call
         {
            struct cli::Implementation
            {
               common::argument::Option options()
               {

                  return common::argument::Option{ &local::call, local::call_complete, { "call"}, R"(generic service call

* service   name of the service to invoke

Reads buffer from stdin and call the provided service, prints the reply buffer to stdout.
Assumes that the input buffer to be in a conformant format, ie, created by casual or some other tool.
Error will be printed to stderr
)"};
               }

            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Option cli::options() &
            {
               return m_implementation->options();
            }
         } // call
      } // manager  
   } // gateway
} // casual



