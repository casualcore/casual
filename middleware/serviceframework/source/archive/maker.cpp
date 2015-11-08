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
         namespace holder
         {
            template< typename B, typename A, typename S>
            class basic : public base< B>
            {
            public:
               typedef B base_type;
               typedef A archive_type;
               typedef S source_type;

               ~basic() noexcept {};

               template< typename... Arguments>
               basic( Arguments&&... arguments)
                  : m_archive( m_source( std::forward< Arguments>( arguments)...))
               {
               }

               virtual archive_type& archive() override
               {
                  return m_archive;
               }

            private:

               source_type m_source;
               archive_type m_archive;

            };


         }

         namespace reader
         {

            namespace from
            {
               namespace local
               {
                  namespace
                  {
                     using factory_function = std::function< Holder::base_value_type( std::ifstream&)>;

                     template< typename A>
                     struct factory
                     {
                        Holder::base_value_type operator () ( std::ifstream& file)
                        {
                           return Holder::base_value_type{ new A{ file}};
                        }
                     };
                  } // <unnamed>
               } // local

               template< typename A, typename S>
               using basic_holder = holder::basic< Reader, A, S>;

               Holder file( const std::string& filename)
               {
                  std::ifstream file( filename);

                  if( ! file.is_open())
                  {
                     throw exception::FileNotFound( filename);
                  }

                  const auto extension = common::file::name::extension( filename);

                  static const auto dispatch = std::map< std::string, local::factory_function>{
                     { "yaml", local::factory< basic_holder< yaml::relaxed::Reader, yaml::Load >>{}},
                     { "yml", local::factory< basic_holder< yaml::relaxed::Reader, yaml::Load >>{}},
                     { "json", local::factory< basic_holder< json::relaxed::Reader, json::Load >>{}},
                     { "jsn", local::factory< basic_holder< json::relaxed::Reader, json::Load >>{}},
                     { "xml", local::factory< basic_holder< xml::relaxed::Reader, xml::Load >>{}},
                     { "ini", local::factory< basic_holder< ini::relaxed::Reader, ini::Load >>{}}
                  };

                  auto found = common::range::find( dispatch, extension);

                  if( found)
                  {
                     return Holder( found->second( file));
                  }
                  else
                  {
                     throw exception::Validation( "Could not deduce protocol for file " + filename);
                  }
               }

            } // from
         } // reader
      } // archive
   } // sf
} // casual


