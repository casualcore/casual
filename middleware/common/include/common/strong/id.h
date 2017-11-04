//!
//! casual
//!

#ifndef CASUAL_COMMON_STRONG_ID_H_
#define CASUAL_COMMON_STRONG_ID_H_

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
      } // strong 
   } // common
} // casual


#endif
