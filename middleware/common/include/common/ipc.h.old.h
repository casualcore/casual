


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
#include "common/process.h"
#include "common/message/type.h"



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
         namespace receive
         {
            class Queue;

         } // receive

         namespace send
         {
            class Queue;

         } // send


         typedef platform::queue_id_type id_type;

         namespace message
         {

            struct Transport;

            enum Flags
            {
               cNoBlocking = common::platform::cIPC_NO_WAIT
            };

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


            namespace ignore
            {
               namespace signal
               {
                  //!
                  //! Send a @p transport message to ipc-queue with @p id
                  //!
                  //! ignores signals
                  //!
                  bool send( id_type id, const Transport& transport, long flags);

                  //!
                  //! Send a @p transport message to ipc-queue with @p id
                  //!
                  //! ignores signals
                  //!
                  bool receive( id_type id, Transport& transport, long flags);

               } // signal
            } // ignore

            struct Transport
            {
               typedef platform::message_type_type message_type_type;
               typedef Uuid::uuid_type correalation_type;


               struct header_t
               {
                  correalation_type correlation;

                  //!
                  //! Information of the complete logical message.
                  //!
                  struct complete_t
                  {
                     //!
                     //! index in the complete message
                     //!
                     std::uint64_t index;

                     //!
                     //! size of the complete message
                     //!
                     std::uint64_t size;

                  } complete;
               };

               enum
               {
                  message_max_size = common::platform::message_size,
                  header_size = sizeof( header_t),
                  payload_max_size = message_max_size - header_size,

               };


               struct message_t
               {
                  using payload_type = std::array< char, payload_max_size>;

                  //
                  // type has to be first!
                  //
                  message_type_type type;
                  header_t header;
                  payload_type payload;

               } message;


               static_assert( message_max_size - payload_max_size < payload_max_size, "Payload is to small");
               static_assert( std::is_pod< message_t>::value, "Message has be a POD");
               static_assert( sizeof( message_t) - sizeof( message_type_type) == message_max_size, "something is wrong with padding");


               Transport();

               //!
               //! @return the message type
               //!
               common::message::Type type() const { return static_cast< common::message::Type>( message.type);}

               //!
               //! Sets the message type
               //!
               //! @param type type to set
               //!
               void type( common::message::Type type) { message.type = static_cast< decltype( message.type)>( type);}


               //!
               //! @return payload size
               //!
               std::size_t size() const;


               //!
               //! @return true if this transport is the last of the complete message
               //!
               bool last() const;


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
               friend bool ignore::signal::send( id_type id, const Transport& transport, long flags);
               friend bool ignore::signal::receive( id_type id, Transport& transport, long flags);


            private:

               std::size_t m_size = 0;
            };


            struct Complete
            {
               using message_type_type = common::message::Type;
               using payload_type = platform::binary_type;

               Complete( message_type_type type, const Uuid& correlation);


               Complete( Complete&&) noexcept;
               Complete& operator = ( Complete&&) noexcept;

               Complete( const Complete&) = delete;
               Complete& operator = ( const Complete&) = delete;


               explicit operator bool() const;

               bool complete() const;

               message_type_type type;
               Uuid correlation;
               payload_type payload;
               payload_type::size_type offset = 0;

               friend class receive::Queue;
               friend class send::Queue;

               friend std::ostream& operator << ( std::ostream& out, const Complete& value);

               //!
               //! Only usefull to receive::Queue, and should be private.
               //! But it's a pain in the butt to make vector and allocotor friends
               //! in a portable way.
               //!
               Complete( Transport& transport);


            private:
               void add( Transport& transport);
            };


         } // message



         namespace queue
         {

            struct State
            {

               typedef std::vector< message::Complete> cache_type;
               using range_type = decltype( range::make( cache_type::iterator(), 0));


               template< typename IPC, typename P>
               range_type find( IPC ipc, P predicate, const long flags);


               template< typename P>
               range_type find( P predicate, const long flags);

               range_type cache( message::Transport& message);

               bool discard( message::Transport& message);

               common::file::scoped::Path m_path;

               cache_type m_cache;

               std::vector< Uuid> m_discarded;
            };




            message::Complete receive( State& state);


         } // queue


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
               //! flushes the messages on the ipc-queue into cache. (ie, make the ipc-queue writable if it was full)
               //!
               void flush();

               //!
               //! Discards any message that correlates.
               //!
               void discard( const Uuid& correlation);

               //!
               //! Clear and discard all messages in queue.
               //!
               void clear();

            private:

               typedef std::vector< message::Complete> cache_type;
               using range_type = decltype( range::make( cache_type::iterator(), cache_type::iterator()));


               template< typename IPC, typename P>
               range_type find( IPC ipc, P predicate, const long flags);


               template< typename P>
               range_type find( P predicate, const long flags);

               range_type cache( message::Transport& message);

               bool discard( message::Transport& message);

               common::file::scoped::Path m_path;

               cache_type m_cache;

               std::vector< Uuid> m_discarded;

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
         bool remove( platform::queue_id_type id);

         //!
         //! Removes an ipc-queue resource, if last receive is done by owner
         //!
         bool remove( const process::Handle& owner);


      } // ipc
   } // common
} // casual


#endif /* CASUAL_IPC_H_ */
