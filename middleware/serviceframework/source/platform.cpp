//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/platform.h"

#include "serviceframework/archive/archive.h"


#include "common/process.h"
#include "common/domain.h"

namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {

         void serialize( Reader& archive, platform::Uuid& value, const char* name)
         {
            platform::binary::type uuid;
            archive >> name::value::pair::make( name, uuid);

            common::algorithm::copy_max( uuid, value.get());
         }

         void serialize( Writer& archive, const platform::Uuid& value, const char* name)
         {
            platform::binary::type uuid( sizeof( value.get()));
            common::algorithm::copy( value.get(), std::begin( uuid));

            archive << name::value::pair::make( name, uuid);
         }

         void serialize( Reader& archive, common::process::Handle& value, const char* name)
         {
            if( archive.serialtype_start( name))
            {
               archive >> name::value::pair::make( "pid", value.pid);
               archive >> name::value::pair::make( "ipc", value.ipc);   
            }
            archive.serialtype_end( name);
         }
         void serialize( Writer& archive, const common::process::Handle& value, const char* name)
         {
            archive.serialtype_start( name);

            archive << name::value::pair::make( "pid", value.pid);
            archive << name::value::pair::make( "ipc", value.ipc);   

            archive.serialtype_end( name);
         }



         void serialize( Reader& archive, common::domain::Identity& value, const char* name)
         {
            if( archive.serialtype_start( name))
            {
               archive >> name::value::pair::make( "name", value.name);
               archive >> name::value::pair::make( "id", value.id);
            }
            archive.serialtype_end( name);

         }

         void serialize( Writer& archive, const common::domain::Identity& value, const char* name)
         {
            archive.serialtype_start( name);

            archive << name::value::pair::make( "name", value.name);
            archive << name::value::pair::make( "id", value.id);

            archive.serialtype_end( name);
         }


         void serialize( Reader& archive, platform::time::point::type& value, const char* name)
         {
            platform::time::point::type::rep representation;
            archive >> name::value::pair::make( name, representation);
            value = platform::time::point::type( platform::time::point::type::duration( representation));
         }

         void serialize( Writer& archive, const platform::time::point::type& value, const char* name)
         {
            archive << name::value::pair::make( name, value.time_since_epoch().count());
         }

         void serialize( Reader& archive, std::chrono::nanoseconds& value, const char* name)
         {
            decltype( value.count()) count;
            archive >> name::value::pair::make( name, count);
            value = std::chrono::nanoseconds( count);
         }

         void serialize( Writer& archive, const std::chrono::nanoseconds& value, const char* name)
         {
            archive << name::value::pair::make( name, value.count());
         }

      } // archive

   } // serviceframework

} // casual
