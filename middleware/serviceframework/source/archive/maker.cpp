//!
//! casual
//!

#include "sf/archive/maker.h"

#include "sf/archive/yaml.h"
#include "sf/archive/json.h"
#include "sf/archive/xml.h"
#include "sf/archive/ini.h"
#include "sf/archive/log.h"

#include "common/string.h"
#include "common/file.h"
#include "common/buffer/type.h"

#include <fstream>
#include <iostream>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace name
         {
            namespace
            {
               constexpr auto yml = "yml"; constexpr auto yaml = "yaml";
               constexpr auto xml = "xml";
               constexpr auto jsn = "jsn"; constexpr auto json = "json";
               constexpr auto ini = "ini";
      
            } // <unnamed>
         } // local


         namespace reader
         {
            namespace from
            {
               namespace local
               {
                  namespace
                  {
                     template<typename IO>
                     archive::Reader name( IO& stream, std::string name)
                     {
                        auto make_yaml = []( auto& source){ return archive::yaml::relaxed::reader( source);};
                        auto make_json = []( auto& source){ return archive::json::relaxed::reader( source);};
                        auto make_xml = []( auto& source){ return archive::xml::relaxed::reader( source);};
                        auto make_ini = []( auto& source){ return archive::ini::relaxed::reader( source);};

                        static const auto dispatch = std::map< std::string, std::function< archive::Reader( decltype(stream))>>
                        {
                           { name::yml, make_yaml}, { name::yaml, make_yaml}, { common::buffer::type::yaml(), make_yaml}, 
                           { name::jsn, make_json}, { name::json, make_json}, { common::buffer::type::json(), make_json},
                           { name::xml, make_xml}, { common::buffer::type::xml(), make_xml},
                           { name::ini, make_ini}, { common::buffer::type::ini(), make_ini},
                        };
                        
                        auto type = common::string::lower( std::move( name));
                        const auto found = common::algorithm::find( dispatch, type);

                        if( found)
                        {
                           return found->second( stream);
                        }

                        throw exception::Validation{ string::compose( "Could not deduce archive from name: ", type)};
                     }
                  } // <unnamed>
               } // local

               archive::Reader data( std::istream& stream)
               {
                  //
                  // Skip any possible whitespace
                  //
                  stream >> std::ws;

                  //
                  // TODO: Maybe do this more fancy
                  //
                  // YAML-directive starts with '%'
                  // YAML-document starts with '-'
                  // YAML-comment starts with '#'
                  // XML must start with '<'
                  // JSON-object starts with '{'
                  // JSON-array with starts '['
                  // INI usually starts with '['
                  //


                  switch( stream.peek())
                  {
                  case '%':
                  case '-':
                  case '#':
                     return local::name( stream, name::yml);
                  case '<':
                     return local::name( stream, name::xml);
                  case '{':
                     return local::name( stream, name::jsn);
                  case '[':
                     return local::name( stream, name::ini);
                  default:
                     throw exception::Validation{ "Could not deduce archive from input"};
                  }
               }

               struct File::Implementation
               {
                  Implementation( const std::string& name) : file( name) 
                  {
                     if( ! file.is_open())
                     {
                        throw exception::system::invalid::File( name);
                     }
                  } 
                  std::ifstream file;
               };

               File::File( const std::string& name) 
                  : m_implementation( name), 
                   m_reader( from::name( m_implementation->file, common::file::name::extension( name))) 
               {

               }
               File::~File() = default;
               File::File( File&&) = default;
               File& File::operator = ( File&&) = default;

               File file( const std::string& name)
               {
                  File result( name);

                  return result;
               }

               archive::Reader data()
               {
                  return data( std::cin);
               }

               archive::Reader name( std::string name)
               {
                  return local::name( std::cin, std::move( name));
               }

               archive::Reader name( std::istream& stream, std::string name)
               {
                  return local::name( stream, std::move( name));
               }

               archive::Reader buffer( const platform::binary::type& data, std::string name)
               {
                  return local::name( data, std::move( name));
               }
            } // from
         } // reader


         namespace writer
         {
            namespace from
            {
               namespace local
               {
                  namespace
                  {
                     template<typename IO>
                     archive::Writer name( IO& stream, std::string name)
                     {
                       auto make_yaml = []( auto& destination){ return archive::yaml::writer( destination);};
                       auto make_json = []( auto& destination){ return archive::json::writer( destination);};
                       auto make_xml = []( auto& destination){ return archive::xml::writer( destination);};
                       auto make_ini = []( auto& destination){ return archive::ini::writer( destination);};

                        static const auto dispatch = std::map< std::string, std::function< archive::Writer(IO&)>>
                        {
                           { name::yml, make_yaml}, { name::yaml, make_yaml}, { common::buffer::type::yaml(), make_yaml}, 
                           { name::jsn, make_json}, { name::json, make_json}, { common::buffer::type::json(), make_json},
                           { name::xml, make_xml}, { common::buffer::type::xml(), make_xml},
                           { name::ini, make_ini}, { common::buffer::type::ini(), make_ini},
                           //{ "", make_log},
                        };

                        auto type = common::string::lower( std::move( name));
                        const auto found = common::algorithm::find( dispatch, type);

                        if( found)
                        {
                           return found->second( stream);
                        }

                        throw exception::Validation{ string::compose( "Could not deduce archive from name: ", type)};
                     }
                  } // <unnamed>
               } // local

               struct File::Implementation
               {
                  Implementation( const std::string& name) : file( name) 
                  {
                     if( ! file.is_open())
                     {
                        throw exception::system::invalid::File( name);
                     }
                  } 
                  std::ofstream file;
               };

               File::File( const std::string& name) 
                  : m_implementation( name), 
                   m_writer( from::name( m_implementation->file, common::file::name::extension( name))) 
               {

               }
               File::~File() = default;
               File::File( File&&) = default;
               File& File::operator = ( File&&) = default;

               File file( const std::string& name)
               {
                  File result( name);

                  return result;
               }

               archive::Writer name( std::string name)
               {
                  if( name.empty()) return archive::log::writer( std::cout);
                  return local::name( std::cout, std::move( name));
               }

               archive::Writer name( std::ostream& stream, std::string name)
               {
                  if( name.empty()) return archive::log::writer( stream);
                  return local::name( stream, std::move( name));
               }

               archive::Writer buffer( platform::binary::type& data, std::string name)
               {
                  return local::name( data, std::move( name));
               }
            } // from
         } // writer
      } // archive
   } // sf
} // casual


