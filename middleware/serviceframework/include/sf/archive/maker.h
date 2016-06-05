//!
//! casual_sf_archivebuffer.h
//!
//! Created on: Nov 19, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SF_ARCHIVE_MAKER_H_
#define CASUAL_SF_ARCHIVE_MAKER_H_


#include <memory>

#include "sf/archive/archive.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {

         namespace maker
         {

            template<typename A>
            class Base
            {
            public:
               typedef A archive_type;

               virtual ~Base() = default;
               virtual archive_type& archive() = 0;
               virtual void serialize() = 0;
            };


            namespace holder
            {

               template<typename T>
               class Base
               {
               public:
                  typedef std::unique_ptr<maker::Base<T>> base_type;

                  Base( base_type&& base) : m_base( std::move( base)) {}
                  Base( Base&& rhs) = default;
                  ~Base() = default;

               protected:
                  base_type m_base;
               };

            } // holder


         } // maker


         namespace reader
         {
            typedef maker::holder::Base<Reader> Base;

            class Holder : public Base
            {
            public:

               using Base::Base;

               template< typename T>
               Holder& operator & ( T&& value)
               {
                  m_base->serialize();
                  m_base->archive() & std::forward< T>( value);
                  return *this;
               }

               template< typename T>
               Holder& operator >> ( T&& value)
               {
                  m_base->serialize();
                  m_base->archive() >> std::forward< T>( value);
                  return *this;
               }
            };

            namespace from
            {
               Holder file( const std::string& name);
            } // from

         } // reader


         namespace writer
         {
            typedef maker::holder::Base<Writer> Base;

            class Holder : public Base
            {
            public:

               using Base::Base;

               template< typename T>
               Holder& operator & ( T&& value)
               {
                  m_base->archive() & std::forward< T>( value);
                  m_base->serialize();
                  return *this;
               }

               template< typename T>
               Holder& operator << ( T&& value)
               {
                  m_base->archive() << std::forward< T>( value);
                  m_base->serialize();
                  return *this;
               }

            };

            namespace from
            {
               Holder file( const std::string& name);
            } // from

         } // writer

      } // archive

   } // sf

} // casual



#endif /* CASUAL_SF_ARCHIVE_MAKER_H_ */
