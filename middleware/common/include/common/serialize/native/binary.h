//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/value.h"

#include "common/traits.h"
#include "common/communication/message.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/execution.h"

#include <vector>
#include <cassert>


namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace native
         {
            namespace binary
            {
               using size_type = platform::size::type;

               struct Policy
               {

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
                  static size_type read( const platform::binary::type& buffer, size_type offset, T& value)
                  {
                     return memory::copy( buffer, offset, value);
                  }

                  template< typename T>
                  static size_type read_size( const platform::binary::type& buffer, size_type offset, T& value)
                  {
                     return read( buffer, offset, value);
                  }

               };

               template< typename P>
               struct basic_writer
               {
                  constexpr static auto archive_type = archive::Type::static_order_type;
                  using policy_type = P;

                  basic_writer( platform::binary::type& buffer)
                     : m_buffer( buffer)
                  {
                     m_buffer.reserve( 128);
                  }

                  template< typename T>
                  basic_writer& operator & ( T&& value)
                  {
                     return *this << std::forward< T>( value);
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


                  template< typename T> 
                  void write( T&& value, const char*) 
                  { 
                     save( std::forward< T>( value));
                  }

               private:

                  template< typename Range>
                  void append( Range&& range)
                  {
                     m_buffer.insert(
                        std::end( m_buffer),
                        std::begin( range),
                        std::end( range));
                  }

                  template< typename T>
                  auto save( T&& value) -> std::enable_if_t< std::is_arithmetic< common::traits::remove_cvref_t< T>>::value>
                  {
                     policy_type::write( std::forward< T>( value), m_buffer);
                  }

                  void save( const std::string& value) 
                  { 
                     write_size( value.size());
                     append( value);
                  }
                  void save( const platform::binary::type& value) 
                  { 
                     write_size( value.size());
                     append( value);
                  }
                  
                  void save( view::immutable::Binary value) 
                  {
                     append( value);
                  }

                  void write_size( size_type value)
                  {
                     policy_type::write_size( value, m_buffer);
                  }

                  platform::binary::type& m_buffer;
               };


               using Writer = basic_writer< Policy>;

               template< typename P>
               struct basic_reader
               {
                  constexpr static auto archive_type = archive::Type::static_order_type;
                  using policy_type = P;

                  basic_reader( const platform::binary::type& buffer, platform::size::type offset)
                     : m_buffer( buffer), m_offset{ offset} {}

                  basic_reader( const platform::binary::type& buffer) : m_buffer( buffer){}

                  template< typename T>
                  basic_reader& operator & ( T&& value)
                  {
                     return *this >> value;
                  }

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

                  template< typename T> 
                  bool read( T&& value, const char*) 
                  { 
                     load( value);
                     return true;
                  }

               private:

                  template< typename Range>
                  void consume( Range&& range)
                  {
                     auto source = range::make( std::begin( m_buffer) + m_offset, range.size());
                     assert( m_offset + source.size() <= range::size( m_buffer));

                     algorithm::copy( source, range);

                     m_offset += range.size();
                  }

                  template< typename T>
                  auto load( T& value) -> std::enable_if_t< std::is_arithmetic< common::traits::remove_cvref_t< T>>::value>
                  {
                     m_offset = policy_type::read( m_buffer, m_offset, value);
                  }

                  void load( std::string& value)
                  {
                     value.resize( read_size());
                     consume( value);
                  }

                  void load( platform::binary::type& value)
                  {
                     value.resize( read_size());
                     consume( value);
                  }

                  void load( view::Binary value)
                  {
                     consume( value);
                  }

                  auto read_size()
                  {
                     size_type size;
                     m_offset = policy_type::read_size( m_buffer, m_offset, size);
                     return size;
                  }

               private:

                  const platform::binary::type& m_buffer;
                  platform::size::type m_offset = 0;
               };

               using Reader = basic_reader< Policy>;

               inline auto writer( platform::binary::type& buffer) { return binary::Writer{ buffer};}
               inline auto reader( const platform::binary::type& buffer) { return binary::Reader{ buffer};}

               namespace create
               {
                  struct Writer
                  {
                     inline auto operator () ( platform::binary::type& buffer) const
                     {
                        return binary::writer( buffer);
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
         
         } // native
      } // serialize
   } // common
} // casual


