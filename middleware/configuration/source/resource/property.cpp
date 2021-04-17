//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/resource/property.h"
#include "configuration/common.h"

#include "common/serialize/create.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


#include "common/file.h"
#include "common/environment.h"
#include "common/serialize/line.h"

namespace casual
{
   using namespace common;

   namespace configuration::resource
   {
      namespace property
      {
         namespace local
         {
            namespace
            {

               auto get( const std::vector< std::string>& patterns)
               {
                  return algorithm::accumulate( file::find( patterns), std::vector< Property>{}, []( auto result, auto& file)
                  {
                     auto get_properties = []( common::file::Input file)
                     {
                        common::log::line( verbose::log, "file: ", file);

                        std::vector< Property> resources;

                        // Create the reader and deserialize configuration
                        auto reader = common::serialize::create::reader::consumed::from( file);

                        reader >> CASUAL_NAMED_VALUE( resources);
                        reader.validate();

                        // Make sure we've got valid configuration
                        validate( resources);

                        common::log::line( verbose::log, "resources: ", resources);

                        return resources;
                     };
                     algorithm::append( get_properties( file), result);
                     return result;
                  });
               }

            } // <unnamed>
         } // local

         std::vector< Property> get( const std::string& glob)
         {
            Trace trace{ "config::resource::get"};

            return local::get( { glob});
         }

         std::vector< Property> get()
         {
            // Try to find configuration file
            auto file = common::environment::variable::get( common::environment::variable::name::resource::configuration, "");

            if( ! file.empty())
               return get( file);

            std::string base = common::environment::directory::casual() / "configuration" / "resources";

            return local::get( {
               base + ".yaml",
               base + ".json",
               base + ".xml",
               base + ".ini",
            });
         }


      } // property


      void validate( const std::vector< Property>& properties)
      {

      }


   } // config::resource
} // casual

