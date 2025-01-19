//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "file/api/file.h"

#include "file/message.h"
#include "file/create.h"
#include "file/instance.h"


#include "common/code/casual.h"
#include "common/communication/instance.h"

namespace casual
{
   namespace file
   {
      namespace v1
      {
         namespace local
         {
            namespace 
            {
               common::communication::instance::outbound::detail::optional::Device device{ instance::identity};
            } //
         } // local

         namespace blocking
         {
            auto reserve( std::filesystem::path path) -> std::filesystem::path
            {
               auto reply = common::communication::ipc::call( 
                  local::device, 
                  message::create::blocking::request( std::move( path)));

               switch( reply.code)
               {
               case code::ok:
                  return std::move( reply.path);
               default:
                  common::code::raise::error( reply.code, "failed to reserve file");
               }
            }
         } // blocking

         namespace non
         {
            namespace blocking
            {
               auto reserve( std::filesystem::path path) -> std::optional< std::filesystem::path>
               {
                  auto reply = common::communication::ipc::call( 
                     local::device, 
                     message::create::non::blocking::request( std::move( path)));

                  switch( reply.code)
                  {
                  case code::ok:
                     return std::move( reply.path);
                  case code::busy:
                     return {};
                  default:
                     common::code::raise::error( reply.code, "failed to reserve file");
                  }
               }
            } // blocking
         } // non
      } // v1
   } // file
} // casual
