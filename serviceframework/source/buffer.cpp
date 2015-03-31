//!
//! buffer.cpp
//!
//! Created on: Dec 23, 2012
//!     Author: Lazan
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

         namespace pool
         {
            struct API : public common::buffer::pool::default_pool
            {
               static const std::vector< Type>& types()
               {
                  static const std::vector< Type> result{
                        type::api::binary(), type::api::json(), type::api::yaml(), type::api::xml()
                     };
                  return result;
               }
            };

            //
            // Register the pool to the pool-holder at the end of this file...
            //

         } // pool

         namespace type
         {
            namespace api
            {
               Type binary() { return { ".api", type::binary().name};}
               Type json() { return { ".api", type::json().name};}
               Type yaml() { return { ".api", type::yaml().name};}
               Type xml() { return { ".api", type::xml().name};}

               const std::vector< Type>& types() { return pool::API::types();}
            }

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
         Buffer& Buffer::operator = ( Buffer&&) = default;
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


            void Stream::consume( void* value, size_type count)
            {
               //common::log::internal::debug << "consume - range: " << range.size() << " buffer: " << *this << std::endl;

               if( m_read_offset + count > size())
               {
                  throw exception::NotReallySureWhatToCallThisExcepion( "Attempt to read out of bounds  ro: " + std::to_string( m_read_offset) + " size: " + std::to_string( size()));
               }

               memcpy( value, begin() + m_read_offset, count);
               m_read_offset += count;
            }

            void Stream::append( const void* value, size_type count)
            {
               //common::log::internal::debug << "append - range: " << range.size()  << " buffer: " << *this << std::endl;

               while( m_write_offset + count > size())
               {
                  resize( size() * 2);
               }

               memcpy(  begin() + m_write_offset, value, count);
               m_write_offset += count;
            }

         } // binary



         std::string Binary::str() const
         {
            return std::string{ std::begin( *this), std::end( *this)};
         }

         void Binary::str( const std::string& new_string)
         {
            if( new_string.size() > size())
            {
               resize( new_string.size() + 1);
            }
            memcpy( data(), new_string.c_str(), new_string.size());
         }




      } // buffer
   } // sf

   namespace common
   {
      namespace buffer
      {


         //
         // Registrate the pool to the pool-holder
         //
         template class ::casual::common::buffer::pool::Registration< sf::buffer::pool::API>;

      } // buffer
   } // common

} // casual


