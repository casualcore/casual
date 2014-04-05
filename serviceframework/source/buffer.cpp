//!
//! buffer.cpp
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
//!

#include "sf/buffer.h"
#include "common/exception.h"
#include "common/algorithm.h"


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
         Type type( platform::raw_buffer_type buffer)
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


         //namespace raw
         //{

            Buffer::Buffer( const Type& type, std::size_t size)
               : m_buffer{ tpalloc( type.name.c_str(), type.subname.c_str(), size), deleter_type{}}, m_size( size)
            {
               if( ! m_buffer)
               {
                  m_size = 0;
                  throw exception::memory::Allocation{ "could not allocate buffer: " + common::error::xatmi::error( tperrno)};
               }
            }

            Buffer::Buffer( const Raw& buffer)
               : m_buffer{ buffer.buffer, deleter_type{}}, m_size( buffer.size)
            {

            }

            Buffer::Buffer( Buffer&&) = default;
            Buffer::~Buffer() = default;


            Buffer::buffer_type Buffer::data() noexcept
            {
               return m_buffer.get();
            }

            const Buffer::buffer_type Buffer::data() const noexcept
            {
               return m_buffer.get();
            }

            Buffer::size_type Buffer::size() const noexcept
            {
               return m_size;
            }

            Buffer::Raw Buffer::release() noexcept
            {
               Raw result{ m_buffer.release(), m_size};
               m_size = 0;
               return result;
            }

            void Buffer::resize( size_type size)
            {
               Buffer buffer{ Raw{ tprealloc( data(), size), size}};

               if( ! buffer.m_buffer)
               {
                  throw exception::memory::Allocation{ "failed to resize buffer: " + common::error::xatmi::error( tperrno)};
               }

               swap( buffer);
               buffer.release();
            }

            void Buffer::swap( Buffer& buffer) noexcept
            {
               std::swap( m_buffer, buffer.m_buffer);
               std::swap( m_size, buffer.m_size);
            }


            void Buffer::deleter_type::operator () ( buffer_type buffer) const
            {
               tpfree( buffer);
            }

            Type type( const Buffer& source)
            {
               return buffer::type( source.data());
            }

            Buffer copy( const Buffer& source)
            {
               Buffer result{ type( source), source.size()};

               common::range::copy( source, std::begin( result));

               return result;
            }



         //} // raw

         Raw::Raw( platform::raw_buffer_type buffer, std::size_t size) : buffer( buffer), size( size)
         {

         }

         Raw allocate( const Type& type, std::size_t size)
         {
            Raw result{ tpalloc( type.name.c_str(), type.subname.c_str(), size), size};

            if( result.buffer == nullptr)
            {
               throw exception::memory::Allocation{ "could not allocate buffer: " + common::error::xatmi::error( tperrno)};
            }
            return result;
         }

         Raw copy( const Raw& buffer)
         {
            Raw result = allocate( type( buffer.buffer), buffer.size);
            common::range::copy( buffer, result.buffer);
            return result;
         }

         Base::Base( Raw buffer) : m_buffer( buffer.buffer), m_size( buffer.size)
         {

         }

         Base::Base( Type&& type, std::size_t size)
            : Base{ allocate( std::move( type), size)}
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

         void Base::swap( Base& other)
         {
            std::swap( m_buffer, other.m_buffer);
            std::swap( m_size, other.m_size);
         }

         void Base::reset( Raw buffer)
         {
            doReset( buffer);
         }

         void Base::clear()
         {
            doClear();
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


         void Base::xatmi_deleter::operator ()( platform::raw_buffer_type xatmiBuffer) const
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


