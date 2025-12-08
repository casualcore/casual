//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/buffer/type.h"
#include "common/algorithm.h"
#include "common/algorithm/sorted.h"
#include "common/file.h"
#include "casual/argument.h"
#include "common/exception/guard.h"

#include "common/serialize/macro.h"
#include "common/serialize/create.h"


#include "server/start.h"
#include "server/service.h"


#include <xatmi.h>

#include <string>
#include <vector>
#include <regex>

namespace casual
{
   namespace test
   {
      namespace mockup
      {
         namespace string
         {
            namespace local
            {
               namespace
               {

                  common::log::Stream log{ "test.mockup"};
                  
                  namespace configuration
                  {
                     struct Model
                     {
                        struct Entry 
                        {
                           std::string service;
                           std::string type = std::string( common::buffer::type::x_octet);
                           std::string match;
                           std::string result;

                           CASUAL_CONST_CORRECT_SERIALIZE(
                           {
                              CASUAL_SERIALIZE( service);
                              CASUAL_SERIALIZE( type);
                              CASUAL_SERIALIZE( match);
                              CASUAL_SERIALIZE( result);
                           })
                        };

                        std::vector< Entry> entries;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( entries);
                        })
                     };

                  } // configuration


                  struct Service 
                  {
                     struct Entry 
                     {
                        std::regex match;
                        server::service::invoke::Result result;

                        friend bool operator == ( const Entry& lhs, const common::buffer::Payload& rhs) 
                        {
                           auto string = common::binary::span::to_string_like( rhs.data);
                           return lhs.result.payload.type == rhs.type &&
                              std::regex_match( std::begin( string), std::end( string), lhs.match);
                        }
                     };

                     server::service::invoke::Result operator() ( server::service::invoke::Parameter&& parmeter)
                     {
                        common::log::line( local::log, "mockup service invoked: ", parmeter.service.name);

                        auto found = common::algorithm::find( entries, parmeter.payload);

                        if( found)
                           return found->result;

                        throw std::runtime_error{ "failed to find a match"};
                     }

                     std::vector< Entry> entries;
                  };

                  
                  namespace transform
                  {
                     configuration::Model configuration( const std::string& file)
                     {
                        configuration::Model mockup;

                        common::file::Input stream( file);
                        auto archive = common::serialize::create::reader::consumed::from( stream);
                        archive >> CASUAL_NAMED_VALUE( mockup);

                        return mockup;
                     }

                     std::vector< server::argument::Service> services( configuration::Model&& model)
                     {
                        // group the configuration per service

                        common::log::line( local::log, "configuration: ", model);

                        auto service_less_than = []( auto& lhs, auto& rhs){ return lhs.service < rhs.service;};

                        common::algorithm::sort( model.entries, service_less_than);

                        auto groups = common::algorithm::sorted::group( model.entries, service_less_than);
                        
                        // transform to a casual service
                        auto transform_service = []( auto& group)
                        {
                           auto name = group.at( 0).service;

                           local::Service callable;

                           auto transform_entry = []( auto& entry)
                           {
                              local::Service::Entry result;

                              result.match = entry.match;
                              result.result.payload.type = entry.type;
                              auto binary = common::binary::span::make( entry.result);
                              common::algorithm::container::append( binary, result.result.payload.data);

                              return result;
                           };

                           callable.entries = common::algorithm::transform( group, transform_entry);

                           return server::argument::Service{ 
                              .name = name,
                              .function = std::move( callable), 
                              .transaction = server::service::transaction::Type::none, 
                              .visibility = server::service::visibility::Type::discoverable,
                              .category = "mockup"
                           };
                        };

                        // transform all groups to services
                        return common::algorithm::transform( groups, transform_service);
                     }

                  } // transform

                  void main( int argc, const char** argv)
                  {
                     std::string file;

                     argument::parse( "mockup-string", {
                        argument::Option{ std::tie( file), {{ "-c", "--configuration"}}, "configuration file"}
                     }, argc, argv);

                     // transform services and start server
                     server::start( transform::services( transform::configuration( file)));

                  }    
               } // <unnamed>
            } // local

            
         } // string
      } // mockup
   } // test
} // casual

int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::test::mockup::string::local::main( argc, argv);
   });
}



