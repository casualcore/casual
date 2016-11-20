//!
//! casual
//!

#include "sf/buffer.h"
#include "common/exception.h"
#include "common/algorithm.h"
#include "common/buffer/pool.h"

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

         namespace type
         {
            Type get( platform::raw_buffer_type buffer)
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

            Type get( const Buffer& source)
            {
               return get( source.data());
            }

         } // type




         Buffer::Buffer( const Type& type, std::size_t size)
            : m_buffer{ tpalloc( type.name.c_str(), type.subname.c_str(), size), deleter_type{}}, m_size( size)
         {
            if( size && ! m_buffer)
            {
               m_size = 0;
               throw exception::memory::Allocation{ "could not allocate buffer: " + common::error::xatmi::error( tperrno)};
            }
         }

         Buffer::Buffer( const Type& type) : Buffer{ type, 0}
         {

         }

         Buffer::Buffer( const Raw& buffer)
            : m_buffer{ buffer.buffer, deleter_type{}}, m_size( buffer.size)
         {

         }

         Buffer::Buffer( Buffer&&) = default;
         Buffer& Buffer::operator = ( Buffer&&) = default;
         Buffer::~Buffer() = default;


         Buffer::buffer_type Buffer::data() noexcept
         {
            return m_buffer.get();
         }

         Buffer::buffer_type Buffer::data() const noexcept
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

         void Buffer::reset( Raw raw)
         {
            Buffer buffer{ raw};
            swap( buffer);
         }



         void Buffer::swap( Buffer& buffer) noexcept
         {
            std::swap( m_buffer, buffer.m_buffer);
            std::swap( m_size, buffer.m_size);
            //std::swap( m_write_offset, buffer.m_write_offset);
            //std::swap( m_read_offset, buffer.m_read_offset);
         }

         /*
         void Buffer::clear() noexcept
         {
            //m_write_offset = 0;
            //m_read_offset = 0;
         }
         */


         void Buffer::deleter_type::operator () ( buffer_type buffer) const
         {
            tpfree( buffer);
         }



         Buffer copy( const Buffer& source)
         {
            Buffer result{ type::get( source), source.size()};

            common::range::copy( source, std::begin( result));

            common::log::internal::debug << "copy - source: " << source << " result: " << result << std::endl;

            return result;
         }


         namespace binary
         {
            Stream::Stream() : Buffer( type::binary(), 128) {}

            void Stream::clear() noexcept
            {
               m_write_offset = 0;
               m_read_offset = 0;
            }

            void Stream::swap( Stream& stream) noexcept
            {
               Buffer::swap( stream);
               std::swap( m_write_offset, stream.m_write_offset);
               std::swap( m_read_offset, stream.m_read_offset);
            }


         } // binary



         std::string Binary::str() const
         {
            return std::string{ std::begin( *this), std::end( *this)};
         }

         void Binary::str( const std::string& value)
         {
            if( value.size() > size())
            {
               resize( value.size() + 1);
            }

            common::memory::copy( common::range::make( value), common::range::make( data(), size()));
         }




      } // buffer
   } // sf

} // casual


