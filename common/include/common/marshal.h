//!
//! casual_archive.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_H_
#define CASUAL_ARCHIVE_H_



#include "common/ipc.h"
#include "common/types.h"
#include "common/uuid.h"



#include <vector>
#include <cassert>

//
// We need to get private stuff for unittest
//
#include "gtest/gtest_prod.h"



namespace casual
{

   template< typename M, typename T>
   void marshal_value( M& marshler, T& value)
   {
      value.marshal( marshler);
   }

   template< typename M, typename T>
   void unmarshal_value( M& unmarshler, T& value)
   {
      value.marshal( unmarshler);
   }

   //!
   //! Overload for Uuid
   //!
   template< typename M>
   void marshal_value( M& marshler, common::Uuid& value)
   {
      marshler << value.get();
   }

   template< typename M>
   void unmarshal_value( M& unmarshler, common::Uuid& value)
   {
      unmarshler >> value.get();
   }



   namespace common
   {
      namespace marshal
      {
         namespace output
         {
            struct Binary
            {
               typedef common::binary_type buffer_type;

               Binary() = default;
               Binary( Binary&&) = default;
               Binary( const Binary&) = delete;
               Binary& operator = ( const Binary&) = delete;

               const buffer_type& get() const
               {
                  return m_buffer;
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

            private:

               template< typename T>
               typename std::enable_if< ! std::is_pod< T>::value>::type
               write( T& value)
               {
                  casual::marshal_value( *this, value);
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
                  Binary::buffer_type::size_type size = m_buffer.size();

                  m_buffer.resize( size + sizeof( T));

                  memcpy( &m_buffer[ size], &value, sizeof( T));
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
                  const Binary::buffer_type::size_type size = m_buffer.size();

                  m_buffer.resize( size + value.size());

                  memcpy( &m_buffer[ size], value.c_str(), value.size());
               }

               void write( common::binary_type& value)
               {
                  writePod( value.size());

                  m_buffer.insert( m_buffer.end(), value.begin(), value.end());
               }

               buffer_type m_buffer;
            };

         } // output

         namespace input
         {

            struct Binary
            {
               typedef common::binary_type buffer_type;
               typedef buffer_type::size_type offest_type;
               typedef ipc::message::Transport transport_type;
               typedef transport_type::message_type_type message_type_type;


               Binary() = default;
               Binary( Binary&&) = default;
               Binary( const Binary&) = delete;
               Binary& operator = ( const Binary&) = delete;


               //!
               //! Only for unittest
               //!
               Binary( output::Binary&& rhs)
               {
                  m_buffer = std::move( rhs.get());
               }

               message_type_type type() const
               {
                  return m_messageType;
               }



               void add( transport_type& message)
               {
                  m_messageType = message.m_payload.m_type;
                  const std::size_t size = message.paylodSize();
                  const std::size_t bufferSize = m_buffer.size();

                  m_buffer.resize( m_buffer.size() + size);

                  std::copy(
                     message.m_payload.m_payload.begin(),
                     message.m_payload.m_payload.begin() + size,
                     m_buffer.begin() + bufferSize);

                  //memcpy( &m_buffer[ m_buffer.size()], message.m_payload.m_payload, size);
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


            private:

               FRIEND_TEST( casual_common_message_dispatch, dispatch__gives_correct_dispatch);
               FRIEND_TEST( casual_common_message_dispatch, dispatch__gives_no_found_handler);

               template< typename T>
               typename std::enable_if< ! std::is_pod< T>::value>::type
               read( T& value)
               {
                  casual::unmarshal_value( *this, value);
               }

               template< typename T>
               typename std::enable_if< std::is_pod< T>::value>::type
               read( T& value)
               {
                  readPod( value);
               }

               /*
               void read( long& value)
               {
                  readPod( value);
               }

               void read( std::size_t& value)
               {
                  readPod( value);
               }

               void read( int& value)
               {
                  readPod( value);
               }
               */

               template< typename T>
               void read( std::vector< T>& value)
               {
                  typename std::vector< T>::size_type size;
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

                  std::copy(
                     m_buffer.begin() + m_offset,
                     m_buffer.begin() + m_offset + size,
                     value.begin());

                  m_offset += size;
               }

               void read( common::binary_type& value)
               {
                  std::string::size_type size;
                  *this >> size;

                  value.assign(
                     m_buffer.begin() + m_offset,
                     m_buffer.begin() + m_offset + size);

                  m_offset += size;
               }

               template< typename T>
               void readPod( T& value)
               {
                  assert( m_buffer.size() >= ( sizeof( T) +  m_offset));

                  memcpy( &value, &m_buffer[ m_offset], sizeof( T));
                  m_offset += sizeof( T);
               }

               /*
               void read( utility::Uuid& value)
               {
                  read( value.get());
               }
               */


               buffer_type m_buffer;
               offest_type m_offset = 0;
               message_type_type m_messageType = 0;
            };

         } // output
      } // marshal
   } // common
} // casual



#endif /* CASUAL_ARCHIVE_H_ */
