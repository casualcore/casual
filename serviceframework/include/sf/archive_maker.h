//!
//! casual_sf_archivebuffer.h
//!
//! Created on: Nov 19, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SF_ARCHIVEBUFFER_H_
#define CASUAL_SF_ARCHIVEBUFFER_H_


#include <memory>

#include "sf/archive_base.h"
#include "sf/archive_yaml_policy.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace reader
         {
            class holder_base
            {
            public:
               holder_base() {};
               virtual ~holder_base() {};
               virtual Reader& reader() = 0;
            };



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


            class Holder
            {
            public:
               Holder( std::unique_ptr< holder_base>&& base) : m_base( std::move( base)) {}
               ~Holder() {}

               template< typename T>
               Holder& operator & ( T&& value)
               {
                  m_base->reader() & std::forward< T>( value);
                  return *this;
               }

               template< typename T>
               Holder& operator >> ( T&& value)
               {
                  m_base->reader() >> std::forward< T>( value);
                  return *this;
               }

               Holder( Holder&& rhs)
               {
                  m_base = std::move( rhs.m_base);
               }

            private:
               std::unique_ptr< holder_base> m_base;
            };

            /*
            std::unique_ptr< holder_base> makeHolder( std::istream& stream)
            {
              typedef basic_holder< YamlRelaxed, policy::reader::Buffer<> > YamlRelaxedHolder;

              return std::unique_ptr< holder_base>( new YamlRelaxedHolder( stream));
            }
            */


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


         }
      }

   }

}



#endif /* CASUAL_SF_ARCHIVEBUFFER_H_ */
