//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/admin/cli.h"
#include "configuration/model/transform.h"
#include "configuration/model/load.h"

#include "common/argument.h"
#include "common/file.h"
#include "common/serialize/create.h"

namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace admin
      {
         namespace local
         {
            namespace
            {
               struct State
               {
                  std::string format{ "yaml"};
               };

               auto load( std::vector< std::string> globs)
               {
                  return model::load( file::find( std::move( globs)));
               }

               auto validate()
               {
                  auto invoke = []( std::vector< std::string> globs)
                  {
                     local::load( std::move( globs));
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     { "--validate"},
                     R"(validates configuration from supplied glob patterns

On success exit with 0, on error not 0, and message printed to stderr)"
                  };
               }

               auto normalize( State& state)
               {
                  auto invoke = [&state]( std::vector< std::string> globs)
                  {
                     // load the model, transform back to user model...
                     auto domain = model::transform( local::load( std::move( globs)));
                     auto writer = serialize::create::writer::from( state.format);
                     writer << domain;
                     writer.consume( std::cout);
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     { "--normalize"},
                     R"(normalizes the supplied configuration glob pattern to stdout

The format is default yaml, but could be supplied via the --format option)"
                  };
               }

               auto format( State& state)
               {
                  auto complete = []( auto values, auto help)
                  {
                     return std::vector< std::string>{ "yaml", "json", "ini", "xml"};
                  };

                  return argument::Option{
                     std::tie( state.format),
                     std::move( complete),
                     { "--format"},
                     R"(defines what format should be used)"
                  };
               }
            } // <unnamed>
         } // local
         struct CLI::Implementation
         {
            
            auto options()
            {
               constexpr auto description = R"(configuration utility - does NOT actively configure anything
               
Used to check and normalize configuration
)";
               return argument::Group{ [](){}, { "configuration"}, description,
                  local::normalize( state),
                  local::validate(),
                  local::format( state),                  
               };
            }

            local::State state;
         };

         CLI::CLI() = default; 
         CLI::~CLI() = default; 

         common::argument::Group CLI::options() &
         {
            return m_implementation->options();
         }

      } // admin
      
   } // configuration
} // casual