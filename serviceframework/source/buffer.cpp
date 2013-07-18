//!
//! buffer.cpp
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
//!

#include "sf/buffer.h"
#include "common/exception.h"


#include "xatmi.h"

//
// std
//
#include <array>

namespace casual
{
   namespace sf
   {
      namespace buffer
      {
         Type type( common::raw_buffer_type buffer)
         {


            std::array< char, 8 + 1> type;
            std::array< char, 16 + 1> subtype;

            type.back() = '\0';
            subtype.back() = '\0';

            tptypes( buffer, type.data(), subtype.data());

            Type result;

            result.name = type.data();
            result.subname = subtype.data();

            return result;
         }

         Raw::Raw( common::raw_buffer_type p_buffer, std::size_t p_size) : buffer( p_buffer), size( p_size)
         {

         }

         Base::Base( Raw buffer) : m_buffer( buffer.buffer), m_size( buffer.size)
         {

         }

         Base::Base( Type&& type, std::size_t size)
            : Base{ Raw{ tpalloc( type.name.c_str(), type.subname.c_str(), size), size}}
         {
         }

         Base::~Base()
         {

         }

         Raw Base::raw()
         {
            return Raw( m_buffer.get(), m_size);
         }

         std::size_t Base::size() const
         {
            return m_size;
         }

         Raw Base::release()
         {
            Raw result( m_buffer.release(), m_size);

            m_size = 0;

            return result;
         }

         Type Base::type() const
         {
            return buffer::type( m_buffer.get());
         }

         void Base::reset( Raw buffer)
         {
            doReset( buffer);
         }

         void Base::doReset( Raw buffer)
         {
            m_buffer.reset( buffer.buffer);
            m_size = buffer.size;
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
               throw common::exception::NotReallySureWhatToNameThisException();

            }

         }


         void Base::xatmi_deleter::operator ()( common::raw_buffer_type xatmiBuffer) const
         {
            tpfree( xatmiBuffer);
         }



         Binary::Binary( Binary&&) = default;
         Binary& Binary::operator = ( Binary&&) = default;


         Binary::Binary() : Base( Type{ "X_OCTET", "binary"}, 1024)
         {

         }

         //Binary::Binary( Base&& base) : Base( std::move( base)) {}




         X_Octet::X_Octet( const std::string& subtype) : X_Octet( subtype, 1024)
         {
         }

         X_Octet::X_Octet( const std::string& subtype, std::size_t size) : Base( Type{ "X_OCTET", subtype}, size)
         {
            if( size)
            {
               m_buffer.get()[ 0] = '\0';
            }

         }

         X_Octet::X_Octet( Raw buffer) : Base( buffer)
         {
         }

         std::string X_Octet::str() const
         {
            return m_buffer.get();
         }

         void X_Octet::str( const std::string& new_string)
         {
            if( new_string.size() > size())
            {
               expand( new_string.size() - size() + 1);
            }
            memcpy( m_buffer.get(), new_string.c_str(), new_string.size());
         }

      } // buffer
   } // sf
} // casual


