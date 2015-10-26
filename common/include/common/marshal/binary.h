//!
//! binary.h
//!
//! Created on: Dec 20, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MARSHAL_BINARY_H_
#define CASUAL_COMMON_MARSHAL_BINARY_H_



#include "common/ipc.h"
#include "common/algorithm.h"
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
               static constexpr std::size_t size( T&&) { return sizeof( T);}

               template< typename Iter, typename T>
               static void write( Iter buffer, T&& value)
               {
                  memcpy( buffer, &value, sizeof( T));
               }

               template< typename Iter, typename T>
               static void read( T& value, Iter buffer)
               {
                  memcpy( &value, buffer, sizeof( T));
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
                  auto offset = expand( last - first);

                  std::copy(
                     first,
                     last,
                     std::begin( m_buffer) + offset);
               }

               template< typename C>
               void append( C&& range)
               {
                  auto offset = expand( range.size());
                  range::copy( std::forward< C>( range), std::begin( m_buffer) + offset);
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
                  writePod( value);
               }


               template< typename T>
               void writePod( T&& value)
               {
                  auto offset = expand( policy_type::size( value));

                  policy_type::write( &m_buffer[ offset], value);
               }

               template< typename T>
               void write( const std::vector< T>& value)
               {
                  writePod( value.size());

                  for( auto& current : value)
                  {
                     *this << current;
                  }
               }

               void write( const std::string& value)
               {
                  writePod( value.size());

                  append(
                     std::begin( value),
                     std::end( value));
               }

               void write( const platform::binary_type& value)
               {
                  writePod( value.size());

                  append(
                     std::begin( value),
                     std::end( value));
               }

               std::size_t expand( std::size_t size)
               {
                  auto offset = m_buffer.size();

                  m_buffer.resize( m_buffer.size() + size);

                  return offset;
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
                  readPod( value);
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
               void readPod( T& value)
               {
                  const auto size = policy_type::size( value);

                  assert( m_offset + size <= m_buffer.size());

                  policy_type::read( value,  &m_buffer[ m_offset]);
                  m_offset += size;
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
      } // marshal

      namespace ipc
      {
         namespace message
         {
            template< typename M>
            ipc::message::Complete& operator >> ( ipc::message::Complete& complete, M& message)
            {
               assert( complete.type == message.message_type);

               message.correlation = complete.correlation;

               marshal::binary::Input marshal{ complete.payload};
               marshal >> message;

               return complete;
            }

         } // message
      } // ipc

      namespace marshal
      {
         template< typename M, typename C = binary::create::Output>
         ipc::message::Complete complete( M&& message, C creator = binary::create::Output{})
         {
            if( ! message.execution)
            {
               message.execution = execution::id();
            }

            ipc::message::Complete complete( message.message_type, message.correlation ? message.correlation : uuid::make());

            auto marshal = creator( complete.payload);
            marshal << message;

            complete.offset = complete.payload.size();

            return complete;
         }

         template< typename M, typename C = binary::create::Input>
         void complete( ipc::message::Complete& complete, M& message, C creator = binary::create::Input{})
         {
            assert( complete.type == message.message_type);

            message.correlation = complete.correlation;

            auto marshal = creator( complete.payload);
            marshal >> message;
         }

      } // marshal


   } // common

} // casual

#endif // BINARY_H_
