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

               template<typename T, typename F, typename S, typename A>
               class Basic : public Base<T>
               {
               public:
                  typedef T base_type;
                  typedef F file_type;
                  typedef S stream_type;
                  typedef A archive_type;

                  explicit Basic( const std::string& name) : m_file( name), m_archive( m_stream())
                  {
                     if( ! m_file.is_open())
                     {
                        throw exception::FileNotOpen( name);
                     }
                  }

                  virtual ~Basic() noexcept {}

                  virtual void serialize() override
                  {
                     m_stream( m_file);
                  }

                  virtual archive_type& archive() override
                  {
                     return m_archive;
                  }

               private:

                  file_type m_file;
                  stream_type m_stream;
                  archive_type m_archive;
               };


               namespace from
               {
                  template<typename D>
                  auto file( const D& dispatch, const std::string& name) -> decltype(common::range::find( dispatch, name)->second( name))
                  {
                     const auto extension = common::file::name::extension( name);

                     auto found = common::range::find( dispatch, extension);

                     if( found)
                     {
                        return found->second( name);
                     }

                     throw exception::Validation{ "Could not deduce archive for file " + name};
                  }
               } // from

            } // <unnamed>

         } // maker


         namespace reader
         {
            namespace local
            {
               namespace
               {
                  template<typename S, typename A>
                  using Basic = maker::Basic<Reader, std::ifstream, S, A>;

                  using function = std::function< Holder( const std::string&)>;

                  template< typename S, typename A>
                  struct factory
                  {
                     Holder operator () ( const std::string& name) const
                     {
                        return Holder::base_type{ new Basic<S,A>{ name}};
                     }
                  };

               } // <unnamed>
            } // local

            namespace from
            {

               Holder file( const std::string& name)
               {
                  static const auto dispatch = std::map< std::string, local::function>
                  {
                     { "yaml",   local::factory< yaml::Load,   yaml::relaxed::Reader>{}},
                     { "yml",    local::factory< yaml::Load,   yaml::relaxed::Reader>{}},
                     { "json",   local::factory< json::Load,   json::relaxed::Reader>{}},
                     { "jsn",    local::factory< json::Load,   json::relaxed::Reader>{}},
                     { "xml",    local::factory< xml::Load,    xml::relaxed::Reader>{}},
                     { "ini",    local::factory< ini::Load,    ini::relaxed::Reader>{}},
                  };

                  return maker::from::file( dispatch, name);
               }

            } // from

         } // reader


         namespace writer
         {
            namespace local
            {
               namespace
               {

                  template<typename S, typename A>
                  using Basic = maker::Basic<Writer, std::ofstream,S,A>;

                  using function = std::function< Holder( const std::string&)>;

                  template< typename S, typename A>
                  struct factory
                  {
                     Holder operator () ( const std::string& name) const
                     {
                        return Holder::base_type{ new Basic<S,A>{ name}};
                     }
                  };

               } // <unnamed>

            } // local


            namespace from
            {
               Holder file( const std::string& name)
               {
                  static const auto dispatch = std::map< std::string, local::function>
                  {
                     { "yaml",   local::factory< yaml::Save,   yaml::Writer>{}},
                     { "yml",    local::factory< yaml::Save,   yaml::Writer>{}},
                     { "json",   local::factory< json::Save,   json::Writer>{}},
                     { "jsn",    local::factory< json::Save,   json::Writer>{}},
                     { "xml",    local::factory< xml::Save,    xml::Writer>{}},
                     { "ini",    local::factory< ini::Save,    ini::Writer>{}},
                  };

                  return maker::from::file( dispatch, name);
               }

            } // from


         } // writer


      } // archive

   } // sf

} // casual


