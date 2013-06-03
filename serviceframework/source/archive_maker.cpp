//!
//! archive_maker.cpp
//!
//! Created on: Nov 24, 2012
//!     Author: Lazan
//!

#include "sf/archive_maker.h"
#include "sf/archive_yaml.h"

#include "common/file.h"

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
                  : m_source( std::forward< Arguments>( arguments)...),
                    m_archive( m_source.archiveBuffer())
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
               auto extension = common::file::extension( filename);

               if( extension == "yaml")
               {
                  typedef basic_holder< yaml::relaxed::Reader, yaml::reader::Buffer > YamlRelaxedHolder;


                  return Holder( Holder::base_value_type( new YamlRelaxedHolder( filename)));
               }

               throw exception::Validation( "could not deduce protocol for file " + filename);
            }

         } // reader
      } // archive
   } // sf
} // casual


