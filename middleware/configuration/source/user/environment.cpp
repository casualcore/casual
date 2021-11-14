//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/user/environment.h"

#include "common/environment/expand.h"
#include "common/environment.h"
#include "common/algorithm.h"
#include "common/file.h"
#include "common/serialize/create.h"

namespace casual
{
   namespace configuration::user::domain::environment
   {

      Environment get( const std::filesystem::path& path)
      {
         Environment environment;

         // Create the reader and deserialize configuration
         common::file::Input file{ path};
         auto reader = common::serialize::create::reader::consumed::from( file);

         reader >> CASUAL_NAMED_VALUE( environment);
         reader.validate();

         return environment;
      }

      namespace local
      {
         namespace
         {
            std::vector< Variable> fetch( Environment environment, std::vector< std::string>& paths)
            {
               std::vector< Variable> result;

               if( environment.files)
               {
                  for( auto& file : environment.files.value())
                  {
                     auto path = common::environment::expand( file);

                     if( ! common::algorithm::find( paths, path))
                     {
                        auto nested = get( path);

                        paths.push_back( std::move( path));

                        auto variables = fetch( std::move( nested), paths);

                        common::algorithm::move( variables, result);
                     }
                  }
               }

               if( environment.variables)
                  common::algorithm::move( environment.variables.value(), result);

               return result;
            }
         } // <unnamed>
      } // local

      std::vector< Variable> fetch( Environment environment)
      {
         // So we only fetch one file one time. If there are circular dependencies.
         std::vector< std::string> paths;

         return local::fetch( std::move( environment), paths);
      }

      std::vector< common::environment::Variable> transform( const std::vector< Variable>& variables)
      {
         return common::algorithm::transform( variables, []( auto& variable)
         {
            return common::environment::Variable{ variable.key + '=' + variable.value};
         });
      }

      std::vector< Variable> transform( const std::vector< common::environment::Variable>& variables)
      {
         return common::algorithm::transform( variables, []( auto& variable)
         {
            Variable result;
            result.key = variable.name();
            result.value = variable.value();
            return result;
         });
      }

   } // configuration::user::domain::environment
} // casual



