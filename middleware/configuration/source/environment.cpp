//!
//! casual
//!

#include "configuration/environment.h"
#include "common/environment.h"
#include "common/algorithm.h"

#include "sf/archive/maker.h"

namespace casual
{
   namespace configuration
   {
      Environment::Environment() = default;
      Environment::Environment( std::function< void(Environment&)> foreign) { foreign( *this);}

      bool operator == ( const Environment& lhs, const Environment& rhs)
      {
         return lhs.files == rhs.files && rhs.variables == rhs.variables;
      }




      namespace environment
      {
         Variable::Variable() = default;
         Variable::Variable( std::function< void(Variable&)> foreign) { foreign( *this);}


         bool operator == ( const Variable& lhs, const Variable& rhs)
         {
            return lhs.key == rhs.key && lhs.value == rhs.value;
         }


         configuration::Environment get( const std::string& file)
         {
            configuration::Environment environment;

            //
            // Create the reader and deserialize configuration
            //
            auto reader = sf::archive::reader::from::file( file);

            reader >> CASUAL_MAKE_NVP( environment);

            return environment;
         }

         namespace local
         {
            namespace
            {

               std::vector< Variable> fetch( configuration::Environment environment, std::vector< std::string>& paths)
               {
                  std::vector< Variable> result;

                  for( auto& file : environment.files)
                  {
                     auto path = common::environment::string( file);

                     if( ! common::range::find( paths, path))
                     {
                        auto nested = get( path);

                        paths.push_back( std::move( path));

                        auto variables = fetch( std::move( nested), paths);

                        common::range::move( variables, result);
                     }
                  }

                  common::range::move( environment.variables, result);

                  return result;
               }
            } // <unnamed>
         } // local

         std::vector< Variable> fetch( configuration::Environment environment)
         {
            //
            // So we only fetch one file one time. If there are circular dependencies.
            //
            std::vector< std::string> paths;

            return local::fetch( std::move( environment), paths);
         }

      } // environment


   } // config
} // casual



