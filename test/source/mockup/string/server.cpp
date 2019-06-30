//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/buffer/type.h"
#include "common/algorithm.h"
#include "common/file.h"
#include "common/argument.h"
#include "common/exception/handle.h"
#include "common/server/start.h"
#include "common/server/service.h"


#include "common/serialize/macro.h"
#include "common/serialize/create.h"

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
                           std::string type = common::buffer::type::x_octet();
                           std::string match;
                           std::string result;

                           CASUAL_CONST_CORRECT_SERIALIZE(
                           {
                              CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( service));
                              CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( type));
                              CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( match));
                              CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( result));
                           })
                        };

                        std::vector< Entry> entries;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( entries));
                        })
                     };

                  } // configuration


                  struct Service 
                  {
                     struct Entry 
                     {
                        std::regex match;
                        common::service::invoke::Result result;

                        friend bool operator == ( const Entry& lhs, const common::buffer::Payload& rhs) 
                        {
                           return lhs.result.payload.type == rhs.type &&
                              std::regex_match( std::begin( rhs.memory), std::end( rhs.memory), lhs.match);
                        }
                     };

                     common::service::invoke::Result operator() ( common::service::invoke::Parameter&& parmeter)
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
                        auto archive = common::serialize::create::reader::consumed::from( stream.extension(), stream);
                        archive >> CASUAL_NAMED_VALUE( mockup);

                        return mockup;
                     }

                     std::vector< common::server::argument::Service> services( configuration::Model&& model)
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
                              common::algorithm::copy( entry.result, std::back_inserter( result.result.payload.memory));

                              return result;
                           };

                           callable.entries = common::algorithm::transform( group, transform_entry);

                           return common::server::argument::Service{ name, std::move( callable), common::service::transaction::Type::none, "mockup"};
                        };

                        // transform all groups to services
                        return common::algorithm::transform( groups, transform_service);
                     }

                  } // transform

                  void main( int argc, char** argv)
                  {
                     std::string file;

                     common::argument::Parse{ "description", 
                        common::argument::Option{ std::tie( file), { "-c", "--configuration"}, "configuration file"}
                     }( argc, argv);

                     // transform services and start server
                     common::server::start( transform::services( transform::configuration( file)));

                  }    
               } // <unnamed>
            } // local

            
         } // string
      } // mockup
   } // test
} // casual

int main( int argc, char** argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::test::mockup::string::local::main( argc, argv);
   });
}



