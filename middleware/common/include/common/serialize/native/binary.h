//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/value.h"

#include "common/traits.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/execution/context.h"

#include "casual/assert.h"

#include <cassert>

// temp
#include <iostream>


namespace casual
{
   namespace common::serialize::native
   {
      namespace binary
      {
         static_assert( concepts::container::array< std::array< int, 4>>);

         struct Policy
         {
            constexpr static auto archive_properties() { return common::serialize::archive::Property::order;}

            template< typename T>
            static void write( const T& value, platform::binary::type& buffer)
            {
               memory::append( value, buffer);
            }

            template< typename T>
            static void write_size( const T& value, platform::binary::type& buffer)
            {
               write( value, buffer);
            }

            template< typename T>
            static platform::size::type read( const platform::binary::type& buffer, platform::size::type offset, T& value)
            {
               return memory::copy( buffer, offset, value);
            }

            template< typename T>
            static platform::size::type read_size( const platform::binary::type& buffer, platform::size::type offset, T& value)
            {
               return read( buffer, offset, value);
            }

         };

         template< typename P>
         struct basic_writer
         {
            using policy_type = P;
            constexpr static auto archive_properties() { return policy_type::archive_properties();}

            basic_writer()
            {
               m_buffer.reserve( 128);
            }

            template< typename T>
            basic_writer& operator << ( T&& value)
            {
               serialize::value::write( *this, std::forward< T>( value), nullptr);
               return *this;
            }

            inline void container_start( platform::size::type size, const char*) 
            { 
               write_size( size);
            }
            inline void container_end( const char*) { /* no op */}
            inline bool composite_start( const char*) { return true;}
            inline void composite_end(  const char* name) { /* no-op */ }


            template< concepts::arithmetic T>
            auto write( T value, const char*)
            {
               policy_type::write( value, m_buffer);
            }

            void write( std::string_view value, const char*) 
            { 
               write_size( value.size());
               append( common::binary::span::make( value));
            }

            void write( std::u8string_view value, const char*) 
            {
               write_size( value.size());
               append( common::binary::span::make( value));
            }

            void write( std::span< const std::byte> value, const char*) 
            { 
               write_size( value.size());
               append( value);
            }
            
            void write( common::binary::span::Fixed< const std::byte> value, const char*) 
            {
               append( value);
            }

            void write_size( platform::size::type value)
            {
               policy_type::write_size( value, m_buffer);
            }

            platform::binary::type consume() 
            {
               return std::exchange( m_buffer, {});
            }

         private:

            template< concepts::binary::like Range>
            void append( Range&& range)
            {
               m_buffer.insert(
                  std::end( m_buffer),
                  std::begin( range),
                  std::end( range));
            }

            platform::binary::type m_buffer;
         };


         using Writer = basic_writer< Policy>;

         template< typename P>
         struct basic_reader
         {
            using policy_type = P;
            constexpr static auto archive_properties() { return policy_type::archive_properties();}

            basic_reader( const platform::binary::type& buffer, platform::size::type offset)
               : m_buffer( buffer), m_offset{ offset} {}

            basic_reader( const platform::binary::type& buffer) : m_buffer( buffer){}

            template< typename T>
            basic_reader& operator >> ( T&& value)
            {
               serialize::value::read( *this, value, nullptr);
               return *this;
            }

            inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char*) 
            { 
               return { read_size(), true};
            }
            inline void container_end( const char*) {} // no-op

            inline bool composite_start( const char*) { return true;}
            inline void composite_end(  const char* name) {} // no-op

            template< concepts::arithmetic T>
            bool read( T& value, const char*)
            {
               m_offset = policy_type::read( m_buffer, m_offset, value);
               return true;
            }

            bool read( std::string& value, const char*)
            {
               value.resize( read_size());
               consume( common::binary::span::make( value));
               return true;
            }

            bool read( std::u8string& value, const char*)
            {
               value.resize( read_size());
               consume( common::binary::span::make( value));
               return true;
            }

            bool read( platform::binary::type& value, const char*)
            {
               value.resize( read_size());
               consume( value);
               return true;
            }

            bool read( common::binary::span::Fixed< std::byte> value, const char*)
            {
               consume( value);
               return true;
            }

         private:

            template< concepts::binary::like Range>
            void consume( Range&& range)
            {
               auto source = range::make( std::begin( m_buffer) + m_offset, range.size());
               casual::assertion( m_offset + source.size() <= range::size( m_buffer), "m_offset: ", m_offset, ", source.size(): ", source.size(), " buffer.size: ", range::size( m_buffer));
               //assert( m_offset + source.size() <= range::size( m_buffer));

               algorithm::copy( source, range);
               m_offset += range.size();
            }

            platform::size::type read_size()
            {
               platform::size::type size;
               m_offset = policy_type::read_size( m_buffer, m_offset, size);
               return size;
            }

            const platform::binary::type& m_buffer;
            platform::size::type m_offset = 0;
         };

         using Reader = basic_reader< Policy>;

         inline auto writer() { return binary::Writer{};}
         inline auto reader( const platform::binary::type& buffer) { return binary::Reader{ buffer};}

         namespace create
         {
            struct Writer
            {
               inline auto operator () () const
               {
                  return binary::writer();
               }
            };

            struct Reader
            {
               inline auto operator () ( const platform::binary::type& buffer) const
               {
                  return binary::reader( buffer);
               }
            };

         } // create
      } // binary

      namespace create
      {
         template< typename T>
         struct reverse;

         template<>
         struct reverse< binary::create::Writer> { using type = binary::create::Reader;};

         template<>
         struct reverse< binary::create::Reader> { using type = binary::create::Writer;};



         template< typename T>
         using reverse_t = typename reverse< T>::type;

      } // create
   
   } // common::serialize::native
} // casual


