//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/service/describe/cli.h"
#include "tools/service/describe/invoke.h"

#include "common/argument.h"
#include "common/terminal.h"
#include "common/exception/handle.h"

#include "serviceframework/log.h"
#include "common/serialize/create.h"

#include "service/manager/admin/api.h"


#include <iostream>

namespace casual
{
   using namespace common;

   namespace tools
   {
      namespace service
      {
         namespace describe
         {
            namespace local
            {
               namespace
               {

                  struct Settings
                  {
                     std::string protocol;
                     std::vector< std::string> services;

                  };

                  namespace format
                  {
                     std::ostream& indentation( std::ostream& out, std::size_t indent)
                     {
                        while( indent-- > 0)
                        {
                           out << "  ";
                        }
                        return out;

                     }

                     namespace cli
                     {

                        const char* type_name( serviceframework::service::model::type::Category category)
                        {
                           switch( category)
                           {
                              case serviceframework::service::model::type::Category::integer: return "integer";
                              case serviceframework::service::model::type::Category::boolean: return "boolean";
                              case serviceframework::service::model::type::Category::character: return "character";
                              case serviceframework::service::model::type::Category::binary: return "binary";
                              case serviceframework::service::model::type::Category::string: return "string";
                              default: return "unknown";
                           }
                        }
                        void types( std::ostream& out, const std::vector< serviceframework::service::Model::Type>& types, std::size_t indent);

                        void type( std::ostream& out, const serviceframework::service::Model::Type& type, std::size_t indent)
                        {
                           switch( type.category)
                           {
                              case serviceframework::service::model::type::Category::container:
                              {
                                 indentation( out, indent) << common::terminal::color::cyan << "container";
                                 out << " " << type.role << '\n';


                                 types( out, type.attribues, indent + 1);
                                 break;
                              }
                              case serviceframework::service::model::type::Category::composite:
                              {
                                 indentation( out, indent) << common::terminal::color::cyan << "composite";
                                 out << " " << type.role << '\n';

                                 types( out, type.attribues, indent + 1);

                                 break;
                              }
                              default:
                              {
                                 indentation( out, indent) << common::terminal::color::cyan << type_name( type.category);
                                 out << " " << type.role << '\n';
                                 break;
                              }
                           }

                        }

                        void types( std::ostream& out, const std::vector< serviceframework::service::Model::Type>& types, std::size_t indent)
                        {
                           for( auto& type : types)
                           {
                              cli::type( out, type, indent);
                           }

                        }


                        void print( std::ostream& out, const serviceframework::service::Model& model)
                        {
                           out << common::terminal::color::white << "service: " << model.service << '\n';

                           out << common::terminal::color::white << "input\n";
                           types( out, model.arguments.input, 1);

                           out << common::terminal::color::white << "output\n";
                           types( out, model.arguments.output, 1);


                        }


                     } // cli
                  } // format

                  void print( const std::string& service, const common::optional< std::string>& format)
                  {
                     Trace trace{ "tools::service::local::print"};

                     auto models = service::describe::invoke( { service});


                     for( auto& model : models)
                     {
                        if( format)
                        {
                           auto archive = common::serialize::create::writer::from( format.value(), std::cout);
                           archive << CASUAL_NAMED_VALUE( model);
                        }
                        else
                        {
                           format::cli::print( std::cout, model);
                        }
                     }
                  }

                  std::vector< std::string> describe_format()
                  {
                     return { "json", "yaml", "xml", "ini"};
                  }


                  auto describe_completion = []( auto values, bool help) -> std::vector< std::string> 
                  {
                     if( help)
                     {
                        return { common::string::compose( "<service> ", describe_format())};
                     }

                     if( values.size() % 2 == 0)
                     {
                        try 
                        {
                           auto result = casual::service::manager::admin::api::state();

                           return common::algorithm::transform( result.services, []( auto& s){
                              return std::move( s.name);
                           });
                        }
                        catch( ...)
                        {
                           return { "<value>"};
                        }
                     }
                     return describe_format();
                  };
               } // <unnamed>
            } // local


            struct cli::Implementation
            {
               common::argument::Option options()
               {
                  return common::argument::Option{ &local::print, local::describe_completion, { "describe"}, R"(service describer

   * service   name of the service to to describe
   * [format]  optional format of the output, if absent a _CLI format_ is used.

   attention: the service need to be a casual aware service)"};
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Option cli::options() &
            {
               return m_implementation->options();
            }

         } // describe
      } // service
   } // tools
} // casual

