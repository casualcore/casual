//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/value/optional.h"
#include "common/platform.h"

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
               constexpr static platform::process::native::type initialize() { return -1;}
               constexpr static bool empty( platform::process::native::type value) { return value < 0;}
            };
            using id = value::basic_optional< platform::process::native::type, policy>;
         } // process

         namespace ipc
         {
            namespace tag
            {
               struct type{};
            } // tag
   
            using id = value::Optional< platform::ipc::native::type, platform::ipc::native::invalid, tag::type>;
            
         } // ipc

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

      } // strong 
   } // common
} // casual
