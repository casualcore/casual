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

               namespace set_operations
               {
                  namespace local
                  {
                     namespace
                     {
                        template< typename F>
                        void set_operation( std::vector< std::string> globs, State& state, F&& func)
                        {
                           casual::configuration::user::Model model;
                           auto archive = serialize::create::reader::consumed::from( state.format, std::cin);
                           archive >> model;

                           auto a = model::transform( model);
                           auto b = admin::local::load( std::move( globs));

                           a = func( a, std::move( b));

                           auto writer = serialize::create::writer::from( state.format);
                           writer << model::transform( a);
                           writer.consume( std::cout);
                        }
                     } // <unnamed>
                  } // local

                  auto set_union( State& state)
                  {
                     auto invoke = [ &state]( std::vector< std::string> globs)
                     {
                        local::set_operation( globs, state, []( auto lhs, auto rhs)
                        {
                           return set_union( lhs, std::move( rhs));
                        });
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--union"},
                        R"(union of configuration from stdin(lhs) and supplied glob pattern(rhs), outputs to stdout
   rhs has precedence over lhs
                        
   The format is default yaml, but could be supplied via the --format option)"
                     };
                  }

                  auto set_difference( State& state)
                  {
                     auto invoke = [ &state]( std::vector< std::string> globs)
                     {
                        local::set_operation( globs, state, []( auto lhs, auto rhs)
                        {
                           return set_difference( lhs, std::move( rhs));
                        });
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--difference"},
                        R"(difference of configuration from stdin(lhs) and supplied glob pattern(rhs), outputs to stdout
   lhs has precedence over rhs
                        
   The format is default yaml, but could be supplied via the --format option)"
                     };
                  }

                  auto set_intersection( State& state)
                  {
                     auto invoke = [ &state]( std::vector< std::string> globs)
                     {
                        local::set_operation( globs, state, []( auto lhs, auto rhs)
                        {
                           return set_intersection( lhs, std::move( rhs));
                        });
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--intersection"},
                        R"(intersection of configuration from stdin(lhs) and supplied glob pattern(rhs), outputs to stdout
   lhs has precedence over rhs
                        
   The format is default yaml, but could be supplied via the --format option)"
                     };
                  }
               } // set_operation
               
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
                  local::set_operations::set_union( state),
                  local::set_operations::set_difference( state),
                  local::set_operations::set_intersection( state)
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