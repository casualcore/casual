//!
//! archive_maker.cpp
//!
//! Created on: Nov 24, 2012
//!     Author: Lazan
//!

#include "sf/archive_maker.h"
#include "sf/archive_yaml_policy.h"

#include "utility/file.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace reader
         {


            template< typename A, typename S>
            class basic_holder : public holder_base
            {
            public:

               typedef A archive_type;
               typedef S source_type;


               template< typename... Arguments>
               basic_holder( Arguments&&... arguments)
                  : m_source( std::forward< Arguments>( arguments)...),
                    m_archive( m_source.archiveBuffer())
               {
               }


               Reader& reader()
               {
                  return m_archive;
               }

            private:
               source_type m_source;
               archive_type m_archive;

            };


            Holder makeFromFile( const std::string& filename)
            {
               auto extension = utility::file::extension( filename);

               if( extension == "yaml")
               {
                  typedef basic_holder< YamlRelaxed, policy::reader::Buffer > YamlRelaxedHolder;

                  return Holder( std::unique_ptr< holder_base>( new YamlRelaxedHolder( filename)));
               }

               throw exception::Validation( "could not deduce protocol for file " + filename);
            }

         } // reader
      } // archive
   } // sf
} // casual


