//!
//! casual
//!

#ifndef CASUAL_SF_PLATFORM_H_
#define CASUAL_SF_PLATFORM_H_


#include "common/uuid.h"
#include "common/platform.h"
#include "common/transaction/id.h"
#include "common/algorithm.h"
#include "common/optional.h"
#include "common/string.h"



#include "sf/namevaluepair.h"


#include <chrono>

namespace casual
{
   namespace common
   {
      namespace process
      {
         struct Handle;
      } // process

      namespace domain
      {
         struct Identity;
      } // domain

   } // common
   namespace sf
   {
      using common::optional;

      namespace string
      {
         using namespace common::string;   
      } // string
      
      namespace platform
      {
         namespace size 
         {
             using namespace common::platform::size;
         } // size 
         namespace buffer
         {
            using namespace common::platform::buffer;
         } // buffer

         namespace process
         {
            using id = common::platform::process::id;
         } // process



         namespace binary
         {
            using namespace common::platform::binary;

            namespace range
            {
               namespace immutable
               {
                  using type = common::Range< binary::type::const_iterator>;
               } // immutable
            } // range

         } // binary

         using Uuid = common::Uuid;

         namespace uuid
         {
            using namespace common::uuid;
         } // uuid

         namespace time
         {
            using namespace common::platform::time;
         } // time


         namespace ipc
         {
            using id = common::platform::ipc::id;
         } // ipc

      } // platform

      namespace archive
      {
         class Reader;
         class Writer;

         void serialize( Reader& archive, platform::Uuid& value, const char* name);
         void serialize( Writer& archive, const platform::Uuid& value, const char* name);

         void serialize( Reader& archive, platform::process::id& value, const char* name);
         void serialize( Writer& archive, const platform::process::id& value, const char* name);

         void serialize( Reader& archive, platform::ipc::id& value, const char* name);
         void serialize( Writer& archive, const platform::ipc::id& value, const char* name);

         void serialize( Reader& archive, common::process::Handle& value, const char* name);
         void serialize( Writer& archive, const common::process::Handle& value, const char* name);

         void serialize( Reader& archive, common::domain::Identity& value, const char* name);
         void serialize( Writer& archive, const common::domain::Identity& value, const char* name);


         void serialize( Reader& archive, platform::time::point::type& value, const char* name);
         void serialize( Writer& archive, const platform::time::point::type& value, const char* name);

         void serialize( Reader& archive, std::chrono::nanoseconds& value, const char* name);
         void serialize( Writer& archive, const std::chrono::nanoseconds& value, const char* name);

         template< typename R, typename P>
         void serialize( Reader& archive, std::chrono::duration< R, P>& value, const char* name)
         {
            std::chrono::nanoseconds ns;
            serialize( archive, ns, name);
            value = std::chrono::duration_cast< std::chrono::duration< R, P>>( ns);
         }

         template< typename R, typename P>
         void serialize( Writer& archive, const std::chrono::duration< R, P>& value, const char* name)
         {
            serialize( archive, std::chrono::duration_cast<std::chrono::nanoseconds>( value), name);
         }


      } // archive
   } // sf

} // casual

#endif // PLATFORM_H_
