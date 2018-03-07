//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/example/build/server.h"

#include "sf/archive/maker.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         namespace build
         {
            namespace server
            {

               model_type example()
               {
                  model_type result;

                  result.server_default.service.transaction.emplace( "join");
                  result.server_default.service.category.emplace( "some.category");

                  result.services = {
                        { []( configuration::build::server::Service& v){
                           v.name = "s1";
                        }},
                        { []( configuration::build::server::Service& v){
                           v.name = "s2";
                           v.transaction.emplace( "auto");
                        }},
                        { []( configuration::build::server::Service& v){
                           v.name = "s3";
                           v.function.emplace( "f3");
                        }},
                        { []( configuration::build::server::Service& v){
                           v.name = "s4";
                           v.function.emplace( "f4");
                           v.category.emplace( "some.other.category");
                        }}
                  };

                  return result;
               }

               void write( const model_type& server, const std::string& name)
               {
                  auto archive = sf::archive::writer::from::file( name);
                  archive << CASUAL_MAKE_NVP( server);
               }

               common::file::scoped::Path temporary(const model_type& server, const std::string& extension)
               {
                  common::file::scoped::Path file{ common::file::name::unique( common::directory::temporary() + "/build_server_", extension)};

                   write( server, file);

                  return file;
               }

            } // server
         } // build
      } // example
   } // configuration
} // casual
