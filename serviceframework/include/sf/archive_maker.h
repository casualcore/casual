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


            Holder makeFromFile( const std::string& filename);

         } // reader

         namespace writer
         {

         }


      } // archive
   } // sf
} // casual



#endif /* CASUAL_SF_ARCHIVEBUFFER_H_ */
