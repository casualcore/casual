//!
//! casual 
//!

#include "configuration/example/build/server.h"

#include "sf/archive/maker.h"

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
                  result.server_default.service.type.emplace( 10);

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
                           v.type.emplace( 5);
                        }}
                  };

                  return result;
               }

               void write( const model_type& server, const std::string& file)
               {
                  auto archive = sf::archive::writer::from::file( file);
                  archive << CASUAL_MAKE_NVP( server);
               }

               common::file::scoped::Path temporary(const model_type& server, const std::string& extension)
               {
                  common::file::scoped::Path file{ common::file::name::unique( common::directory::temporary() + "/build_server_", extension)};

                  auto archive = sf::archive::writer::from::file( file);
                  archive << CASUAL_MAKE_NVP( server);

                  return file;
               }

            } // server
         } // build
      } // example
   } // configuration
} // casual
