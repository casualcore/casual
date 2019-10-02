//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "buffer/field.h"
#include "buffer/internal/field/string.h"

#include "common/exception/handle.h"
#include "common/serialize/macro.h"
#include "common/serialize/create.h"
#include "common/file.h"
#include "common/argument.h"


#include <string>
#include <vector>
#include <iostream>

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace field
      {
         namespace serialize
         {
            namespace local
            {
               namespace
               {
                  struct Settings
                  {
                     std::vector< std::string> files;
                     std::string output;
                  };

                  namespace model
                  {
                     struct Field
                     {
                        std::string name;
                        long size = 0;
                        std::string alignment = "left";
                        std::string padding = " ";
                        std::string default_;
                        std::string note;

                        CASUAL_CONST_CORRECT_SERIALIZE({
                           CASUAL_SERIALIZE( name);
                           CASUAL_SERIALIZE( size);
                           CASUAL_SERIALIZE( alignment);
                           CASUAL_SERIALIZE( padding);
                           CASUAL_SERIALIZE_NAME( default_, "default");
                           CASUAL_SERIALIZE( note);
                        })
                     };
                     
                     struct Mapping
                     {
                        std::string key;
                        std::vector< Field> fields;
                        std::string note;

                        CASUAL_CONST_CORRECT_SERIALIZE({
                           CASUAL_SERIALIZE( key);
                           CASUAL_SERIALIZE( fields);
                           CASUAL_SERIALIZE( note);
                        })
                     };

                     std::vector< Mapping> get( const std::string& file)
                     {
                        std::vector< Mapping> mapping;
                        common::file::Input stream( file);
                        auto archive = common::serialize::create::reader::consumed::from( stream.extension(), stream);
                        archive >> CASUAL_NAMED_VALUE( mapping);
                        archive.validate();

                        return mapping;
                     }

                     std::vector< Mapping> get( const std::vector< std::string>& files)
                     {
                        std::vector< Mapping> types;
                        common::algorithm::for_each( files, [&types]( auto& file)
                        { 
                           common::algorithm::append( get( file), types);
                        });
                        return types;
                     }
                  } // model

                  namespace generate
                  {
                     namespace name
                     {
                        constexpr auto copyright = R"(
// 
// Copyright (c) 2019, The casual project
//
// This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//
)";
                        constexpr auto indentation = "   ";
                     } // name

                     void source( std::ostream& out, const model::Mapping& model, const std::string& indentation)
                     {

                        out << indentation << "namespace " << model.key << '\n'
                           << indentation << '{' << '\n';


                        out << indentation << "   " << "auto action = internal::field::string::Convert{\n"
                           << indentation << "      " << "[]( auto& input, auto& output)  // to-/from- string\n"
                           << indentation << "      {\n";
                        
                        auto generate_field = [&]( const model::Field& field)
                        {
                           auto get_id = []( auto& name)
                           {
                              long id{};
                              buffer::internal::field::detail::check( casual_field_id_of_name( name.c_str(), &id));
                              return id;
                           };

                           out << indentation << "         " 
                              << "internal::field::string::format< " << get_id( field.name) << ", "
                              << "internal::field::string::Alignment::" << field.alignment << ">::string( input, output, " 
                              << field.size << ", '" 
                              << field.padding.at( 0) << "'); // " << field.name << "\n";
                        };

                        std::for_each( std::begin( model.fields), std::end( model.fields), generate_field);


                        out << indentation << "      }\n"
                           << indentation << "   };\n"; 

                        out << indentation << "   " << "CASUAL_MAYBE_UNUSED auto registred = internal::field::string::convert::registration( \"" <<  model.key << "\", std::move( action));\n";
                        out << indentation << '}' << '\n';
                     }

                     void source( std::ostream& out, const std::vector< model::Mapping>& model)
                     {
                        out << name::copyright 
                           << "\n\n"
                           << "#include <casual/buffer/internal/field/string.h>\n\n"
                           << "namespace casual\n"
                           << "{\n"
                           << "   namespace buffer\n"
                           << "   {\n"
                           << "      namespace local\n"
                           << "      {\n"
                           << "         namespace\n"
                           << "         {\n";

                        algorithm::for_each( model, [&]( auto& mapping)
                        {
                           generate::source( out, mapping, "            ");
                        });

                        out << "         } // <unnamed>\n"
                           << "      } // local\n"
                           << "   } // buffer\n"
                           << "} // casual\n\n";
                     }

                  } // generate

                  void main( int argc, char** argv)
                  {
                     local::Settings settings;

                     argument::Parse{
                        "generates implementation for buffer serialization/tranformation to and from string representation",
                        argument::Option{ argument::option::one::many( settings.files), { "-f", "--files"}, "file(s) with field to/from string mapping"}( argument::cardinality::one{}),
                        argument::Option{ std::tie( settings.output), { "-o", "--output"}, "output cpp file, if omitted stdout will be used"},
                     }( argc, argv);

                     auto mappings = model::get( settings.files);

                     if( ! settings.output.empty())
                     {
                        std::ofstream out{ settings.output};
                        generate::source( out, mappings);
                     }
                     else
                     {
                        generate::source( std::cout, mappings);
                     }
                  }
               } // <unnamed>
            } // local
            
         } // serialize
      } // field
   } // buffer

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::guard( std::cerr, [&]()
   {
      casual::buffer::field::serialize::local::main( argc, argv);
   });
} // main