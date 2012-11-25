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
         namespace holder
         {
            template< typename A>
            class base
            {
            public:
               typedef A archive_type;

               base() = default;
               virtual ~base() = default;
               virtual archive_type& archive() = 0;
            };


            template< typename A>
            class holder
            {
            public:
               typedef A archive_type;
               typedef base< archive_type> base_type;
               typedef std::unique_ptr< base_type> base_value_type;


               holder( base_value_type&& base) : m_base( std::move( base)) {}
               ~holder() {}

               template< typename T>
               holder& operator & ( T&& value)
               {
                  m_base->archive() & std::forward< T>( value);
                  return *this;
               }

               template< typename T>
               holder& operator >> ( T&& value)
               {
                  m_base->archive() >> std::forward< T>( value);
                  return *this;
               }

               holder( holder&& rhs)
               {
                  m_base = std::move( rhs.m_base);
               }

            private:
               base_value_type m_base;
            };
         }

         namespace reader
         {

            typedef holder::holder< Reader> Holder;

            Holder makeFromFile( const std::string& filename);

         } // reader

         namespace writer
         {
            typedef holder::holder< Writer> Holder;



         }


      } // archive
   } // sf
} // casual



#endif /* CASUAL_SF_ARCHIVEBUFFER_H_ */
