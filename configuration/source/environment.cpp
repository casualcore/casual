/*
 * environment.cpp
 *
 *  Created on: 23 apr 2015
 *      Author: 40043280
 */


#include "config/environment.h"

#include "common/environment.h"
#include "common/algorithm.h"

#include "sf/archive/maker.h"

namespace casual
{
   namespace config
   {

      namespace environment
      {

         config::Environment get( const std::string& file)
         {
            config::Environment environment;

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

               std::vector< Environment::Variable> fetch( config::Environment environment, std::vector< std::string>& paths)
               {
                  std::vector< Environment::Variable> result; // = std::move( environment.variables);

                  for( auto& file : environment.files)
                  {
                     auto path = common::environment::string( file);

                     if( ! common::range::find( paths, path))
                     {
                        auto nested = get( path);

                        paths.push_back( std::move( path));

                        auto variables = fetch( std::move( nested), paths);

                        std::move( std::begin( variables), std::end( variables), std::back_inserter( result));
                     }
                  }

                  std::move( std::begin( environment.variables), std::end( environment.variables), std::back_inserter( result));

                  return result;
               }
            } // <unnamed>
         } // local

         std::vector< Environment::Variable> fetch( config::Environment environment)
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



