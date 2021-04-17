//! 
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/file.h"
#include "common/serialize/create.h"
#include "common/serialize/yaml.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace create
         {
            namespace file
            {

               namespace detail
               {
                  inline common::file::scoped::Path temporary(std::string_view extension)
                  {
                     const auto prefix = common::directory::temporary() / "configuration_";
                     return { common::file::name::unique( prefix.string(), extension)};
                  }
               } // detail

               template< typename M>
               common::file::scoped::Path temporary( M&& model, std::string_view root, std::string_view extension)
               {
                  common::file::scoped::Path path = detail::temporary( extension);

                  std::ofstream file{ path};
                  auto archive = common::serialize::create::writer::from( extension);
                  archive << CASUAL_NAMED_VALUE_NAME( model, root.data());;
                  archive.consume( file);

                  return path;
               }
            } // file

            //! creates a given model from yaml
            template< typename M, typename Yaml>
            auto model( Yaml&& yaml, std::string_view root)
            {
               M model;
               auto archive = common::serialize::yaml::consumed::reader( std::forward< Yaml>( yaml));
               archive >> CASUAL_NAMED_VALUE_NAME( model, root.data());
               archive.validate();
               return model;
            };

         } // create    
      } // example
   } // configuration
} // casual
