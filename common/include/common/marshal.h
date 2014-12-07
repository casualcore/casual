//!
//! casual_archive.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_ARCHIVE_H_
#define CASUAL_ARCHIVE_H_



//#include "common/message/type.h"
#include "common/ipc.h"
//#include "common/uuid.h"
#include "common/algorithm.h"



#include <vector>
#include <cassert>

//
// We need to get private stuff for unittest
//
#include "gtest/gtest_prod.h"



#define CASUAL_CONST_CORRECT_MARSHAL( statement) \
   template< typename A> \
   void marshal( A& archive)  \
   {  \
         statement \
   } \
   template< typename A> \
   void marshal( A& archive) const \
   {  \
         statement \
   } \





namespace casual
{
   template< typename T, typename M>
   void casual_marshal_value( T& value, M& marshler)
   {
      value.marshal( marshler);
   }

   template< typename T, typename M>
   void casual_unmarshal_value( T& value, M& unmarshler)
   {
      value.marshal( unmarshler);
   }

   /*
   //!
   //! Overload for Uuid
   //!
   template< typename M>
   void casual_marshal_value( casual::common::Uuid& value, M& marshler)
   {
      marshler << value.get();
   }

   template< typename M>
   void casual_unmarshal_value( casual::common::Uuid& value, M& unmarshler)
   {
      unmarshler >> value.get();
   }
   */

   namespace common
   {
      namespace marshal
      {
         namespace output
         {
            struct Binary
            {
               typedef platform::binary_type buffer_type;

               Binary( ipc::message::Complete& message)
                  : m_message(  message)
               {}


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
                  const auto oldSize = expand( last - first);

                  std::copy(
                     first,
                     last,
                     std::begin( m_message.payload) + oldSize);
               }

               template< typename C>
               void append( C&& range)
               {
                  const auto offset = expand( range.size());
                  range::copy( std::forward< C>( range), std::begin( m_message.payload) + offset);
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
                  const auto oldSize = expand( sizeof( T));

                  memcpy( &m_message.payload[ oldSize], &value, sizeof( T));
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
                  const auto oldSize = m_message.payload.size();

                  if( m_message.payload.capacity() - m_message.payload.size() < size)
                  {
                     m_message.payload.reserve( m_message.payload.capacity() * 2);
                  }

                  m_message.payload.resize( m_message.payload.size() + size);
                  return oldSize;
               }

               ipc::message::Complete& m_message;
            };



         } // output

         namespace input
         {

            struct Binary
            {
               typedef platform::binary_type buffer_type;
               typedef buffer_type::size_type offest_type;
               typedef ipc::message::Transport transport_type;
               typedef transport_type::message_type_type message_type_type;


               Binary( ipc::message::Complete& message)
                  : m_message(  message)
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
                  assert( m_message.payload.size() >= size + m_offset);

                  std::copy(
                     std::begin( m_message.payload) + m_offset,
                     std::begin( m_message.payload) + m_offset + size, out);

                  m_offset += size;
               }


            private:

               FRIEND_TEST( casual_common_message_dispatch, dispatch__gives_correct_dispatch);
               FRIEND_TEST( casual_common_message_dispatch, dispatch__gives_no_found_handler);

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

                  assert( m_message.payload.size() >= size + m_offset);

                  memcpy( &value, &m_message.payload[ m_offset], size);
                  m_offset += size;
               }

               ipc::message::Complete& m_message;
               offest_type m_offset = 0;
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

               marshal::input::Binary marshal{ complete};
               marshal >> message;

               return complete;
            }


            template< typename M>
            ipc::message::Complete& operator << ( ipc::message::Complete& complete, M&& message)
            {
               complete.correlation = message.correlation ? message.correlation : uuid::make();
               complete.type = message.message_type;

               marshal::output::Binary marshal{ complete};
               marshal << message;

               complete.complete = true;

               return complete;
            }

         } // message
      } // ipc

      namespace marshal
      {

         template< typename M>
         ipc::message::Complete complete( M&& message)
         {
            ipc::message::Complete complete;

            complete << message;
            return complete;
         }


      } // marshal

   } // common
} // casual



#endif /* CASUAL_ARCHIVE_H_ */
