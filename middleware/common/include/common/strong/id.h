//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/value/optional.h"
#include "common/platform.h"
#include "common/uuid.h"

namespace casual
{
   namespace common
   {
      namespace strong 
      {
         namespace process
         {
            struct policy
            {
               constexpr static platform::process::native::type initialize() noexcept { return -1;}
               constexpr static bool empty( platform::process::native::type value) noexcept { return value < 0;}
            };
            using id = value::basic_optional< platform::process::native::type, policy>;
         } // process

         namespace ipc
         {
            namespace tag
            {
               struct type{};
            } // tag

            struct policy
            {
               constexpr static Uuid initialize() noexcept { return {};}
               constexpr static bool empty( const Uuid& value) noexcept { return value.empty();}
            };

            using id = value::basic_optional< Uuid, policy>;            
         } // ipc

         namespace socket
         {
            namespace tag
            {
               struct type{};
            } // tag
   
            using id = value::Optional< platform::socket::native::type, platform::socket::native::invalid, tag::type>;
            
         } // socket

         namespace resource
         {
            struct stream
            {
               static std::ostream& print( std::ostream& out, bool valid, platform::resource::native::type value);
            };
            namespace tag
            {
               struct type{};
            } // tag

            using id = value::Optional< platform::resource::native::type, platform::resource::native::invalid, tag::type, stream>;

         } // resource

         namespace queue
         {
            namespace tag
            {
               struct type{};
            } // tag
   
            using id = value::Optional< platform::queue::native::type, platform::queue::native::invalid, tag::type>;
            
         } // ipc

         namespace file
         {
            namespace descriptor
            {
               namespace tag
               {
                  struct type{};
               } // tag
      
               using id = value::Optional< platform::file::descriptor::native::type,platform::file::descriptor::native::invalid, tag::type>;
            
            } // descriptor
         } // file


      } // strong 
   } // common
} // casual
