//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/build/model/load.h"

#include "common/file.h"
#include "common/serialize/create.h"

namespace casual
{
   namespace configuration::build::model::load
   {
      namespace local
      {
         namespace
         {
            template< typename M>
            auto model( const std::filesystem::path& path)
            {
               M model;
               common::file::Input stream{ path};
               auto archive = common::serialize::create::reader::consumed::from( stream);
               archive >> model;
               archive.validate();

               return normalize( std::move( model));
            };
         } // <unnamed>
      } // local

      server::Model server( const std::filesystem::path& path)
      {
         return local::model< server::Model>( path);
      }

      executable::Model executable( const std::filesystem::path& path)
      {
         return local::model< executable::Model>( path);
      }
      
   } // configuration::build::model::load
} // casual