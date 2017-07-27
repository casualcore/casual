//!
//! casual
//!

#include "sf/archive/maker.h"

#include "sf/archive/yaml.h"
#include "sf/archive/json.h"
#include "sf/archive/xml.h"
#include "sf/archive/ini.h"

#include "common/string.h"
#include "common/file.h"
#include "common/buffer/type.h"

#include <fstream>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace
         {
            const auto cYML{ "yml"}; const auto cYAML{ "yaml"};
            const auto cXML{ "xml"};
            const auto cJSN{ "jsn"}; const auto cJSON{ "json"};
            const auto cINI{ "ini"};
         }

         namespace maker
         {
            namespace
            {
               template<typename I, typename IO, typename LS, typename A>
               class Basic : public I
               {
               public:
                  typedef IO stream_type;
                  typedef LS load_save_type;
                  typedef A archive_type;

                  Basic( stream_type stream) : m_archive( m_load_save()), m_stream( std::forward<IO>( stream)) {}

                  virtual ~Basic() noexcept {}

                  void serialize() override
                  {
                     m_load_save( m_stream);
                  }

                  archive_type& archive() override
                  {
                     return m_archive;
                  }

               private:
                  load_save_type m_load_save;
                  archive_type m_archive;
                  stream_type m_stream;
               };

               namespace from
               {
                  template<typename D, typename IO>
                  auto name( const D& dispatch, IO&& stream, std::string name) -> decltype(common::range::find( dispatch, name)->second( std::forward<IO>( stream)))
                  {
                     const auto found = common::range::find( dispatch, common::string::lower( name));

                     if( found)
                     {
                        return found->second( std::forward<IO>( stream));
                     }

                     throw exception::Validation{ "Could not deduce archive from name", CASUAL_NIP( name)};
                  }
               } // from

               template<typename H, typename I, typename LS, typename A>
               struct factory
               {
                  template<typename IO>
                  H operator () ( IO&& stream) const
                  {
                     return typename H::base_type{ new Basic<I, IO, LS, A>{ std::forward<IO>( stream)}};
                  }
               };

            } // <unnamed>

         } // maker


         namespace reader
         {
            namespace from
            {
               namespace local
               {
                  namespace
                  {
                     template<typename IO>
                     Holder name( IO&& stream, std::string name)
                     {
                        using yml_type = maker::factory< Holder, Interface,   yaml::Load, yaml::relaxed::Reader>;
                        using jsn_type = maker::factory< Holder, Interface,   json::Load, json::relaxed::Reader>;
                        using xml_type = maker::factory< Holder, Interface,   xml::Load,  xml::relaxed::Reader>;
                        using ini_type = maker::factory< Holder, Interface,   ini::Load,  ini::relaxed::Reader>;

                        static const auto dispatch = std::map< std::string, std::function< Holder( decltype(stream))>>
                        {
                           { cYML, yml_type{}}, { common::buffer::type::yaml(), yml_type{}}, { cYAML, yml_type{}},
                           { cXML, xml_type{}}, { common::buffer::type::xml(), xml_type{}},
                           { cJSN, jsn_type{}}, { common::buffer::type::json(), jsn_type{}}, { cJSON, jsn_type{}},
                           { cINI, ini_type{}}, { common::buffer::type::ini(), ini_type{}}
                        };

                        return maker::from::name( dispatch, std::forward<IO>( stream), std::move( name));
                     }
                  } // <unnamed>
               } // local

               Holder data( std::istream& stream)
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
                     return local::name( stream, cYML);
                  case '<':
                     return local::name( stream, cXML);
                  case '{':
                     return local::name( stream, cJSN);
                  case '[':
                     return local::name( stream, cINI);
                  default:
                     throw exception::Validation{ "Could not deduce archive from input"};
                  }
               }

               Holder data()
               {
                  return data( std::cin);
               }

               Holder file( std::string name)
               {
                  std::ifstream file( name);

                  if( ! file.is_open())
                  {
                     throw exception::invalid::File( name);
                  }

                  return local::name( std::move( file), common::file::name::extension( name));
               }

               Holder name( std::string name)
               {
                  return local::name( std::cin, std::move( name));
               }

               Holder name( std::istream& stream, std::string name)
               {
                  return local::name( stream, std::move( name));
               }

               Holder buffer( const platform::binary::type& data, std::string type)
               {
                  return local::name( data, std::move( type));
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
                     Holder name( IO&& stream, std::string name)
                     {
                        using yml_type = maker::factory< Holder, Interface,   yaml::Save, yaml::Writer>;
                        using jsn_type = maker::factory< Holder, Interface,   json::Save, json::Writer>;
                        using xml_type = maker::factory< Holder, Interface,   xml::Save,  xml::Writer>;
                        using ini_type = maker::factory< Holder, Interface,   ini::Save,  ini::Writer>;

                        static const auto dispatch = std::map< std::string, std::function< Holder( decltype(stream))>>
                        {
                           { cYML, yml_type{}}, { common::buffer::type::yaml(), yml_type{}}, { cYAML, yml_type{}},
                           { cXML, xml_type{}}, { common::buffer::type::xml(),  xml_type{}},
                           { cJSN, jsn_type{}}, { common::buffer::type::json(), jsn_type{}}, { cJSON, jsn_type{}},
                           { cINI, ini_type{}}, { common::buffer::type::ini(),  ini_type{}}
                        };

                        return maker::from::name( dispatch, std::forward<IO>( stream), std::move( name));
                     }
                  } // <unnamed>
               } // local

               Holder file( std::string name)
               {
                  std::ofstream file( name);

                  if( ! file.is_open())
                  {
                     throw exception::invalid::File( name);
                  }

                  return local::name( std::move( file), common::file::name::extension( name));
               }

               Holder name( std::string name)
               {
                  return local::name( std::cout, std::move( name));
               }

               Holder name( std::ostream& stream, std::string name)
               {
                  return local::name( stream, std::move( name));
               }

               Holder buffer( platform::binary::type& data, std::string type)
               {
                  return local::name( data, std::move( type));
               }

            } // from


         } // writer


      } // archive

   } // sf

} // casual


