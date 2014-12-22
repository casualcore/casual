


//!
//! casual_ipc.h
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_IPC_H_
#define CASUAL_IPC_H_


#include "common/file.h"
#include "common/uuid.h"
#include "common/platform.h"
#include "common/algorithm.h"



//
// std
//
#include <string>
#include <array>

namespace casual
{
   namespace common
   {
      namespace ipc
      {

         typedef platform::queue_id_type id_type;


         namespace message
         {

            struct Transport;

            //!
            //! Send a @p transport message to ipc-queue with @p id
            //!
            //! @return true if message was sent
            //!
            //! @todo document semantics with flags, and what is thrown
            //!
            bool send( id_type id, const Transport& transport, long flags);

            //!
            //! Receive a @p transport message from ipc-queue with @p id
            //!
            //! @return true if message was received
            //!
            //! @todo document semantics with flags, and what is thrown
            //!
            bool receive( id_type id, Transport& transport, long flags);


            struct Transport
            {
               typedef platform::message_type_type message_type_type;
               typedef Uuid::uuid_type correalation_type;


               struct Header
               {
                  correalation_type correlation;
                  long count;

               };

               enum
               {
                  message_max_size = common::platform::message_size,
                  header_size = sizeof( Header),
                  payload_max_size = message_max_size - header_size,

               };

               static_assert( message_max_size - payload_max_size < payload_max_size, "Payload is to small");

               typedef std::array< char, payload_max_size> payload_type;



               struct Message
               {
                  //
                  // type has to be first!
                  //
                  message_type_type type;
                  Header header;
                  payload_type payload;

               } message;


               static_assert( std::is_pod< Message>::value, "Message has be a POD");
               static_assert( sizeof( Message) - sizeof( message_type_type) == message_max_size, "something is wrong with padding");


               Transport();


               //!
               //! @return payload size
               //!
               std::size_t size() const;


               template< typename Iter>
               void assign( Iter first, Iter last)
               {
                  assert( first <= last && last - first <= payload_max_size);
                  std::copy( first, last, std::begin( message.payload));

                  m_size = last - first;
               }

               friend std::ostream& operator << ( std::ostream& out, const Transport& value);

               friend bool send( id_type id, const Transport& transport, long flags);
               friend bool receive( id_type id, Transport& transport, long flags);

            private:

               std::size_t m_size = 0;
            };


            struct Complete
            {
               typedef platform::message_type_type message_type_type;
               typedef platform::binary_type payload_type;

               Complete();
               Complete( Transport& transport);

               Complete( Complete&&) noexcept;
               Complete& operator = ( Complete&&) noexcept;

               Complete( const Complete&) = delete;
               Complete& operator = ( const Complete&) = delete;

               void add( Transport& transport);

               message_type_type type;
               Uuid correlation;
               payload_type payload;
               bool complete = false;

               friend std::ostream& operator << ( std::ostream& out, const Complete& value);
            };

         } // message





         namespace internal
         {

            enum
            {
               cInvalid = -1
            };

            class base_queue
            {
            public:
               typedef platform::queue_id_type id_type;

               base_queue( const base_queue&) = delete;

               id_type id() const;

            protected:

               base_queue() = default;
               base_queue( id_type id) : m_id( id) {}

               base_queue( base_queue&& rhs)
               {
                  std::swap( m_id, rhs.m_id);
               }

               id_type m_id = cInvalid;
            };

         }


         namespace send
         {

            class Queue : public internal::base_queue
            {
            public:

               enum
               {
                  cNoBlocking = common::platform::cIPC_NO_WAIT
               };

               Queue( id_type id);

               Queue( Queue&&) = default;


               Queue( const Queue&) = delete;
               Queue& operator = ( const Queue&) = delete;


               //!
               //! Tries to send the logical message
               //!
               //! @return true if sent, false otherwise
               //!
               Uuid operator () ( const message::Complete& message) const
               {
                  return send( message, 0);
               }

               //!
               //! Tries to send the logical message
               //!
               //! @return true if sent, false otherwise
               //!
               Uuid operator () ( const message::Complete& message, const long flags) const
               {
                  return send( message, flags);
               }

            private:

               Uuid send( const message::Complete& message, const long flags) const;
            };

         }

         namespace receive
         {
            class Queue : public internal::base_queue
            {
            public:

               enum
               {
                  cNoBlocking = common::platform::cIPC_NO_WAIT
               };

               using type_type = message::Complete::message_type_type;

               //!
               //! Creates and manage an ipc-queue
               //!
               Queue();


               ~Queue();

               Queue( Queue&&) = default;

               Queue( const Queue&) = delete;
               Queue& operator = ( const Queue&) = delete;


               //!
               //! Tries to find the first logic complete message
               //!
               //! @return 0..1 occurrences of a logical complete message.
               //!
               std::vector< message::Complete> operator () ( const long flags);

               //!
               //! Tries to find the first logic complete message with a specific type
               //!
               //! @return 0..1 occurrences of a logical complete message.
               //!
               std::vector< message::Complete> operator () ( type_type type, const long flags);

               //!
               //! Tries to find the first logic complete message with any of the types in @p types
               //!
               //! @return 0..1 occurrences of a logical complete message.
               //!
               std::vector< message::Complete> operator () ( const std::vector< type_type>& types, const long flags);

               //!
               //! Tries to find the logic complete message with correlation @p correlation
               //!
               //! @return 0..1 occurrences of a logical complete message.
               //!
               std::vector< message::Complete> operator () ( const Uuid& correlation, const long flags);

               //!
               //! Clear and discard all messages in queue.
               //!
               void clear();

            private:

               typedef std::vector< message::Complete> cache_type;
               using range_type = decltype( range::make( cache_type::iterator(), cache_type::iterator()));

               template< typename P>
               range_type find( P predicate, const long flags);

               range_type cache( message::Transport& message);

               common::file::scoped::Path m_path;

               cache_type m_cache;
            };
         } // receive

         namespace broker
         {
            send::Queue::id_type id();

            //send::Queue& queue();

         } // broker


         namespace receive
         {
            receive::Queue::id_type id();

            receive::Queue& queue();

         } // receive



         //!
         //! Removes an ipc-queue resource.
         //!
         void remove( platform::queue_id_type id);


      } // ipc
   } // common
} // casual


#endif /* CASUAL_IPC_H_ */
