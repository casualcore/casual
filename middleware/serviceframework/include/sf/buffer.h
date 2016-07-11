//!
//! caual
//!

#ifndef SF_BUFFER_H_
#define SF_BUFFER_H_

#include "sf/exception.h"
#include "sf/platform.h"


#include "common/algorithm.h"
#include "common/internal/log.h"
#include "common/string.h"
#include "common/network/byteorder.h"
#include "common/buffer/type.h"
#include "common/memory.h"

//
// std
//
#include <memory>
#include <string>
#include <iosfwd>


#include <cstring>


#include <xatmi.h>



namespace casual
{
   namespace sf
   {
      namespace buffer
      {
         using Type = common::buffer::Type;

         namespace type
         {
            using namespace common::buffer::type;

            Type api();

            Type get( platform::raw_buffer_type buffer);
         } // type





         struct Buffer
         {

            enum
            {
               defaultSize = 256
            };

            using buffer_type =  platform::raw_buffer_type;
            using iterator = buffer_type;
            using const_iterator = platform::const_raw_buffer_type;
            using size_type = std::size_t;
            using value_type = char;

            using range_type = common::Range< iterator>;
            using const_range_type = common::Range< const_iterator>;

            //!
            //! Holds the buffer and its size together. Has no resource responsibility
            //!
            struct Raw
            {
               Raw( buffer_type buffer, size_type size) : buffer{ buffer}, size{ size} {}

               buffer_type buffer;
               size_type size;
            };


            //!
            //! Allocates a buffer of @p type with @size size
            //!
            Buffer( const Type& type, size_type size);


            //!
            //! Crates a 'null-buffer' with @p type type
            //!
            Buffer( const Type& type);


            //!
            //! Takes over ownership of the raw buffer.
            //!
            Buffer( const Raw& buffer);

            Buffer( Buffer&&);
            Buffer& operator = ( Buffer&&);
            ~Buffer();

            buffer_type data() noexcept;
            const buffer_type data() const noexcept;

            size_type size() const noexcept;

            //!
            //! Releases ownership of buffer
            //!
            //! @return the raw buffer
            //!
            Raw release() noexcept;

            //!
            //! Resets the internal buffer, and take ownership
            //! of the raw buffer.
            //! The current buffer is freed (if one exists).
            //!
            //! @param raw the new buffer
            //!
            void reset( Raw raw);

            //!
            //! Resize the buffer.
            //! @note could be a smaller size than the current buffer
            //!   which could lead to freeing memory
            //!
            void resize( size_type size);

            void swap( Buffer& buffer) noexcept;


            iterator begin() noexcept { return m_buffer.get();}
            iterator end() noexcept { return m_buffer.get() + m_size;}
            const_iterator begin() const noexcept { return m_buffer.get();}
            const_iterator end() const noexcept { return m_buffer.get() + m_size;}


            //void clear() noexcept;

            friend std::ostream& operator << ( std::ostream& out, const Buffer& buffer);

         private:
            struct deleter_type
            {
               void operator () ( buffer_type buffer) const;
            };

            using holder_type = std::unique_ptr< typename std::remove_pointer< buffer_type>::type, deleter_type>;

            holder_type m_buffer;
            size_type m_size;
         };


         inline std::ostream& operator << ( std::ostream& out, const Buffer& buffer)
         {
            out << "@" << static_cast< void*>( buffer.data()) << " size: " << buffer.size();
            return out;
         }



         namespace type
         {
            Type get( const Buffer& source);

         } // type



         Buffer copy( const Buffer& source);



         namespace binary
         {
            struct Stream : public Buffer
            {
               using Buffer::Buffer;

               Stream();

               template< typename T>
               Stream& operator << ( const T& value)
               {
                  write( value);
                  return *this;
               }

               template< typename T>
               Stream& operator >> ( T& value)
               {
                  read( value);
                  return *this;
               }

               void clear() noexcept;

               void swap( Stream& buffer) noexcept;

            private:

               template< typename Range>
               void consume( Range destination)
               {
                  if( m_read_offset + destination.size() > size())
                  {
                     throw exception::Validation( "Attempt to read out of bounds  ro: " + std::to_string( m_read_offset) + " size: " + std::to_string( size()));
                  }

                  auto source = common::range::make( begin() + m_read_offset, destination.size());

                  m_read_offset += common::memory::copy( source, destination);
               }

               template< typename Range>
               void append( Range source)
               {
                  while( m_write_offset + source.size() > size())
                  {
                     resize( size() * 2);
                  }

                  auto destination = common::range::make( begin() + m_write_offset, source.size());

                  m_write_offset += common::memory::copy( source, destination);
               }

               template< typename T>
               void write( const T& value)
               {
                  const auto encoded = common::network::byteorder::encode( value);
                  append( common::memory::range::make( encoded));
               }

               void write( const platform::binary_type& value)
               {
                  //
                  // TODO: Write the size as some-common_size_type
                  //
                  write( value.size());
                  append( common::range::make( value));
               }

               void write( const std::string& value)
               {
                  //
                  // TODO: Write the size as some-common_size_type or just
                  // write the string null-terminated that will save us a few
                  // bytes that might be substantial
                  //
                  // append( value.c_str(), value.size() + 1)
                  //
                  write( value.size());
                  append( common::range::make( value));
               }


               template< typename T>
               void read( T& value)
               {
                  common::network::byteorder::type<T> encoded;
                  consume( common::memory::range::make( encoded));
                  value = common::network::byteorder::decode< T>( encoded);
               }

               void read( std::string& value)
               {
                  //
                  // TODO: Read the size as some-common_size_type or just read
                  // until first null-termination and then advance the cursor
                  // that might save us a few bytes per string that might be
                  // substantial
                  //
                  // pseudo-code
                  // value = buffer + offset;
                  // advance( value.size() + 1);
                  //
                  auto size = value.size();
                  read( size);
                  value.resize( size);
                  consume( common::range::make( value));
               }

               void read( platform::binary_type& value)
               {
                  //
                  // TODO: Read the size as some-common_size_type
                  //
                  auto size = value.size();
                  read( size);
                  value.resize( size);
                  consume( common::range::make( value));
               }

               size_type m_write_offset = 0;
               size_type m_read_offset = 0;
            };



         } // binary


         inline Buffer::Raw raw( TPSVCINFO* serviceInfo)
         {
            return Buffer::Raw( serviceInfo->data, serviceInfo->len);
         }




         class Binary : public binary::Stream
         {
         public:
            using binary::Stream::Stream;

            std::string str() const;
            void str( const std::string& value);

         private:
            void doClear() {}

         };



         /*
         template< typename T>
         class allocator;

         template<>
         class allocator< char>
         {
         public:
            typedef char value_type;
            typedef char* pointer;
            typedef const char* const_pointer;
            typedef char& reference;
            typedef const char& const_reference;
            typedef std::size_t size_type;
            typedef std::ptrdiff_t difference_type;

         };
         */

         //typedef std::vector< char, buffer::allocator< char>> test_buffer;



      } // buffer
   } // sf
} // casual




#endif /* BUFFER_H_ */
