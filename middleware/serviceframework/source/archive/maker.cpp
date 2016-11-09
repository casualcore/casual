//!
//! archive_maker.cpp
//!
//! Created on: Nov 24, 2012
//!     Author: Lazan
//!

#include "sf/archive/maker.h"

#include "sf/archive/yaml.h"
#include "sf/archive/json.h"
#include "sf/archive/xml.h"
#include "sf/archive/ini.h"

#include "common/string.h"
#include "common/file.h"

#include <fstream>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
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

                  virtual void serialize() override
                  {
                     m_load_save( m_stream);
                  }

                  virtual archive_type& archive() override
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
                     const auto found = common::range::find( dispatch, common::string::lower( std::move( name)));

                     if( found)
                     {
                        return found->second( std::forward<IO>( stream));
                     }

                     throw exception::Validation{ "Could not deduce archive for name " + name};
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
                        using yml_type = maker::factory< Holder, Interface,   yaml::Load, yaml::Reader>;
                        using jsn_type = maker::factory< Holder, Interface,   json::Load, json::Reader>;
                        using xml_type = maker::factory< Holder, Interface,   xml::Load,  xml::Reader>;
                        using ini_type = maker::factory< Holder, Interface,   ini::Load,  ini::Reader>;

                        static const auto dispatch = std::map< std::string, std::function< Holder( decltype(stream))>>
                        {
                           { "yml", yml_type{}}, { "yaml", yml_type{}},
                           { "xml", xml_type{}},
                           { "jsn", jsn_type{}}, { "json", jsn_type{}},
                           { "ini", ini_type{}},
                        };

                        return maker::from::name( dispatch, std::forward<IO>( stream), std::move( name));
                     }

                  } // <unnamed>
               } // local

               Holder file( std::string name)
               {
                  std::ifstream file( name);

                  if( ! file.is_open())
                  {
                     throw exception::FileNotOpen( name);
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
                           { "yml", yml_type{}}, { "yaml", yml_type{}},
                           { "xml", xml_type{}},
                           { "jsn", jsn_type{}}, { "json", jsn_type{}},
                           { "ini", ini_type{}},
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
                     throw exception::FileNotOpen( name);
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

            } // from


         } // writer


      } // archive

   } // sf

} // casual


