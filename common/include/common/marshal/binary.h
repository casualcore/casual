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

#include <vector>
#include <cassert>


namespace casual
{
   namespace common
   {

      namespace marshal
      {
         namespace output
         {
            struct Binary
            {

               Binary( platform::binary_type& buffer)
                  : m_buffer(  buffer)
               {
                  m_buffer.reserve( 128);
               }


               template< typename T>
               Binary& operator & ( T& value)
               {
                  //return *this << std::forward< T>( value);
                  return *this << value;
               }

               template< typename T>
               Binary& operator << ( T& value)
               {
                  write( value);
                  return *this;
               }


               //
               // Be friend with free marshal function so we can use more
               // bare-bone stuff when we do non-intrusive marshal for third-party types
               //
               //template< typename M, typename T>
               //friend void casual_marshal_value( M& marshler, T& value);


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
               typename std::enable_if< ! std::is_pod< T>::value>::type
               write( T& value)
               {
                  casual_marshal_value( value, *this);
               }

               template< typename T>
               typename std::enable_if< std::is_pod< T>::value>::type
               write( T& value)
               {
                  writePod( value);
               }


               template< typename T>
               void writePod( T&& value)
               {
                  auto offset = expand( sizeof( T));

                  memcpy( &m_buffer[ offset], &value, sizeof( T));
               }

               template< typename T>
               void write( std::vector< T>& value)
               {
                  writePod( value.size());

                  for( auto& current : value)
                  {
                     *this << current;
                  }
               }

               void write( std::string& value)
               {
                  writePod( value.size());

                  append(
                     std::begin( value),
                     std::end( value));
               }

               void write( platform::binary_type& value)
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



         } // output

         namespace input
         {

            struct Binary
            {
               Binary( platform::binary_type& buffer)
                  : m_buffer( buffer), m_offset( 0)
               {

               }


               template< typename T>
               Binary& operator & ( T& value)
               {
                  return *this >> value;
               }

               template< typename T>
               Binary& operator >> ( T& value)
               {
                  read( value);
                  return *this;
               }

               //
               // Be friend with free marshal function so we can use more
               // bare-bone stuff when we do non-intrusive marshal for third-party types
               //
               //template< typename M, typename T>
               //friend void casual_unmarshal_value( M& marshler, T& value);


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
               typename std::enable_if< ! std::is_pod< T>::value>::type
               read( T& value)
               {
                  casual_unmarshal_value( value, *this);
               }

               template< typename T>
               typename std::enable_if< std::is_pod< T>::value>::type
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
                  const auto size = sizeof( T);

                  assert( m_offset + size <= m_buffer.size());

                  memcpy( &value, &m_buffer[ m_offset], size);
                  m_offset += size;
               }

               platform::binary_type& m_buffer;
               platform::binary_type::size_type m_offset;
            };

         } // input
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

               marshal::input::Binary marshal{ complete.payload};
               marshal >> message;

               return complete;
            }

         } // message
      } // ipc

      namespace marshal
      {

         template< typename M>
         ipc::message::Complete complete( M&& message)
         {
            ipc::message::Complete complete( message.message_type, message.correlation ? message.correlation : uuid::make());

            marshal::output::Binary marshal{ complete.payload};
            marshal << message;

            return complete;
         }


      } // marshal

   } // common

} // casual

#endif // BINARY_H_
