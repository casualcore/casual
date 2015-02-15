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

            template< typename A, typename S>
            using basic_holder = holder::basic< Reader, A, S>;

            Holder makeFromFile( const std::string& filename)
            {
               std::ifstream file( filename);

               if( ! file.is_open())
               {
                  throw exception::FileNotFound( filename);
               }

               const auto extension = common::file::extension( filename);


               if( extension == "yaml" || extension == "yml")
               {
                  typedef basic_holder< yaml::relaxed::Reader, yaml::Load > YamlRelaxedHolder;

                  return Holder( Holder::base_value_type( new YamlRelaxedHolder( file)));
               }

               if( extension == "json" || extension == "jsn")
               {
                  typedef basic_holder< json::relaxed::Reader, json::Load > JsonRelaxedHolder;

                  return Holder( Holder::base_value_type( new JsonRelaxedHolder( file)));
               }



               if( extension == "xml")
               {
                  typedef basic_holder< xml::relaxed::Reader, xml::Load > XmlRelaxedHolder;

                  return Holder( Holder::base_value_type( new XmlRelaxedHolder( file)));
               }


               throw exception::Validation( "could not deduce protocol for file " + filename);
            }

         } // reader
      } // archive
   } // sf
} // casual


