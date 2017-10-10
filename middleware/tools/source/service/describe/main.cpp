//!
//! casual 
//!
#include "service/describe/describe.h"

#include "common/arguments.h"
#include "common/terminal.h"

#include "sf/log.h"
#include "sf/archive/maker.h"


#include <iostream>

namespace casual
{
   using namespace common;

   namespace tools
   {
      namespace service
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

                     const char* type_name( sf::service::model::type::Category category)
                     {
                        switch( category)
                        {
                           case sf::service::model::type::Category::integer: return "integer";
                           case sf::service::model::type::Category::boolean: return "boolean";
                           case sf::service::model::type::Category::character: return "character";
                           case sf::service::model::type::Category::binary: return "binary";
                           case sf::service::model::type::Category::string: return "string";
                           default: return "unknown";
                        }
                     }
                     void types( std::ostream& out, const std::vector< sf::service::Model::Type>& types, std::size_t indent);

                     void type( std::ostream& out, const sf::service::Model::Type& type, std::size_t indent)
                     {
                        switch( type.category)
                        {
                           case sf::service::model::type::Category::container:
                           {
                              indentation( out, indent) << common::terminal::color::cyan << "container";
                              out << " " << type.role << '\n';


                              types( out, type.attribues, indent + 1);
                              break;
                           }
                           case sf::service::model::type::Category::composite:
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

                     void types( std::ostream& out, const std::vector< sf::service::Model::Type>& types, std::size_t indent)
                     {
                        for( auto& type : types)
                        {
                           cli::type( out, type, indent);
                        }

                     }


                     void print( std::ostream& out, const sf::service::Model& model)
                     {
                        out << common::terminal::color::white << "service: " << model.service << '\n';

                        out << common::terminal::color::white << "input\n";
                        types( out, model.arguments.input, 1);

                        out << common::terminal::color::white << "output\n";
                        types( out, model.arguments.output, 1);


                     }


                  } // cli
               } // format

               void print( const Settings& settings)
               {
                  Trace trace{ "tools::service::local::print"};

                  auto models = service::describe( settings.services);


                  for( auto& model : models)
                  {
                     if( ! settings.protocol.empty())
                     {
                        auto archive = sf::archive::writer::from::name( std::cout, settings.protocol);

                        archive << CASUAL_MAKE_NVP( model);
                     }
                     else
                     {
                        format::cli::print( std::cout, model);
                     }
                  }
               }
            } // <unnamed>
         } // local


         int main( int argc, char **argv)
         {
            local::Settings settings;

            try
            {
               Arguments arguments{ "Describes a casual service",
                  {
                     argument::directive( {"-f", "--format"}, "format to print (json|yaml|xml|ini), if absent CLI format is used ", settings.protocol),
                     argument::directive( {"-s", "--services"}, "services to describe ", settings.services),
                  }};

               arguments.parse( argc, argv);
            }
            catch( ...)
            {
               return common::exception::handle();
            }

            local::print( settings);


            return 0;
         }

      } // service
   } // tools
} // casual


int main( int argc, char **argv)
{
   return casual::tools::service::main( argc, argv);
}
