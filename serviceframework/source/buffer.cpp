//!
//! buffer.cpp
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
//!

#include "sf/buffer.h"
#include "utility/exception.h"

#include "xatmi.h"

namespace casual
{
   namespace sf
   {
      namespace buffer
      {
         Base::Base( char* buffer, std::size_t size) : m_buffer( buffer), m_size( size)
         {

         }

         Base::~Base()
         {

         }

         char* Base::raw()
         {
            return m_buffer.get();
         }

         std::size_t Base::size() const
         {
            return m_size;
         }

         void Base::expand( std::size_t expansion)
         {
            //
            // Take the greater of double current size and current size + expansion
            //
            const std::size_t newSize = m_size * 2 > m_size + expansion ? m_size * 2 : m_size + expansion;

            char* buffer = m_buffer.release();

            m_buffer.reset( tprealloc( buffer, newSize));

            if( m_buffer.get() == nullptr)
            {
               m_size = 0;
               throw utility::exception::NotReallySureWhatToNameThisException();

            }

         }


         void Base::xatmi_deleter::operator ()( char* xatmiBuffer) const
         {
            tpfree( xatmiBuffer);
         }

         namespace source
         {

            Binary::Binary( Base&& base) : Base( std::move( base))
            {

            }

         }


         namespace target
         {
            Binary::Binary() : Base( tpalloc( "X_OCTET", "binary", 1024), 1024), m_offset{ 0}
            {

            }


         }

      }
   }
}


