//!
//! binary.h
//!
//! Created on: Dec 20, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MARSHAL_BINARY_H_
#define CASUAL_COMMON_MARSHAL_BINARY_H_



#include "common/communication/message.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/marshal/marshal.h"
#include "common/execution.h"

#include <vector>
#include <cassert>


namespace casual
{
   namespace common
   {

      namespace marshal
      {
         namespace binary
         {
            namespace detail
            {

               template< typename T>
               using is_native_marshable = std::integral_constant<bool,
                     std::is_arithmetic<T>::value ||
                     ( std::is_array<T>::value && sizeof( typename std::remove_all_extents<T>::type) == 1 ) ||
                     std::is_enum< T>::value>;

            } // detail

            struct Policy
            {

               template< typename T>
               static void write( const T& value, platform::binary_type& buffer)
               {
                  memory::append( value, buffer);
               }

               template< typename T>
               static std::size_t read( const platform::binary_type& buffer, std::size_t offset, T& value)
               {
                  return memory::copy( buffer, offset, value);
               }

            };

            template< typename P>
            struct basic_output
            {
               using policy_type = P;

               basic_output( platform::binary_type& buffer)
                  : m_buffer(  buffer)
               {
                  m_buffer.reserve( 128);
               }


               template< typename T>
               basic_output& operator & ( T& value)
               {
                  return *this << value;
               }

               template< typename T>
               basic_output& operator << ( const T& value)
               {
                  write( value);
                  return *this;
               }


               template< typename Iter>
               void append( Iter first, Iter last)
               {
                  m_buffer.insert(
                        std::end( m_buffer),
                        first,
                        last);
               }

               template< typename C>
               void append( C&& range)
               {
                  append( std::begin( range), std::end( range));
               }

            private:

               template< typename T>
               typename std::enable_if< ! detail::is_native_marshable< T>::value>::type
               write( T& value)
               {
                  casual_marshal_value( value, *this);
               }

               template< typename T>
               typename std::enable_if< detail::is_native_marshable< T>::value>::type
               write( T& value)
               {
                  write_pod( value);
               }


               template< typename T>
               void write_pod( T&& value)
               {
                  policy_type::write( value, m_buffer);
               }

               template< typename T>
               void write( const std::vector< T>& value)
               {
                  write_pod( value.size());

                  for( auto& current : value)
                  {
                     *this << current;
                  }
               }

               void write( const std::string& value)
               {
                  write_pod( value.size());

                  append(
                     std::begin( value),
                     std::end( value));
               }

               void write( const platform::binary_type& value)
               {
                  write_pod( value.size());

                  append(
                     std::begin( value),
                     std::end( value));
               }

               platform::binary_type& m_buffer;
            };


            using Output = basic_output< Policy>;

            template< typename P>
            struct basic_input
            {
               using policy_type = P;

               basic_input( platform::binary_type& buffer)
                  : m_buffer( buffer), m_offset( 0)
               {

               }


               template< typename T>
               basic_input& operator & ( T& value)
               {
                  return *this >> value;
               }

               template< typename T>
               basic_input& operator >> ( T& value)
               {
                  read( value);
                  return *this;
               }


               template< typename Iter>
               void consume( Iter out, std::size_t size)
               {
                  assert( m_offset + size <= m_buffer.size());

                  const auto first = std::begin( m_buffer) + m_offset;

                  std::copy(
                     first,
                     first + size, out);

                  m_offset += size;
               }


            private:

               template< typename T>
               typename std::enable_if< ! detail::is_native_marshable< T>::value>::type
               read( T& value)
               {
                  casual_unmarshal_value( value, *this);
               }

               template< typename T>
               typename std::enable_if< detail::is_native_marshable< T>::value>::type
               read( T& value)
               {
                  read_pod( value);
               }


               template< typename T>
               void read( std::vector< T>& value)
               {
                  decltype( value.size()) size;
                  *this >> size;

                  value.resize( size);

                  for( auto& current : value)
                  {
                     *this >> current;
                  }
               }

               void read( std::string& value)
               {
                  std::string::size_type size;
                  *this >> size;

                  value.resize( size);

                  consume( std::begin( value), size);
               }

               void read( platform::binary_type& value)
               {
                  std::string::size_type size;
                  *this >> size;

                  value.resize( size);

                  consume( std::begin( value), size);
               }

               template< typename T>
               void read_pod( T& value)
               {
                  m_offset = policy_type::read( m_buffer, m_offset, value);
               }

               platform::binary_type& m_buffer;
               platform::binary_type::size_type m_offset;
            };

            using Input = basic_input< Policy>;


            namespace create
            {
               struct Output
               {
                  binary::Output operator () ( platform::binary_type& buffer) const
                  {
                     return binary::Output{ buffer};
                  }
               };

               struct Input
               {
                  binary::Input operator () ( platform::binary_type& buffer) const
                  {
                     return binary::Input{ buffer};
                  }
               };



            } // create

         } // binary

         namespace create
         {
            template< typename T>
            struct reverse;

            template<>
            struct reverse< binary::create::Output> { using type = binary::create::Input;};

            template<>
            struct reverse< binary::create::Input> { using type = binary::create::Output;};



            template< typename T>
            using reverse_t = typename reverse< T>::type;

         } // create



      } // marshal

      namespace communication
      {
         namespace message
         {
            template< typename M>
            communication::message::Complete& operator >> ( communication::message::Complete& complete, M& message)
            {
               assert( complete.type == message.type());

               message.correlation = complete.correlation;

               marshal::binary::Input marshal{ complete.payload};
               marshal >> message;

               return complete;
            }

         } // message
      } // communication


   } // common

} // casual

#endif // BINARY_H_
