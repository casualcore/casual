


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

         namespace message
         {
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
                  payload_max_size = common::platform::message_size - sizeof( Header)
               };

               static_assert( message_max_size - payload_max_size < payload_max_size, "Payload is to small");

               typedef std::array< char, payload_max_size> payload_type;

               Transport() : m_size( message_max_size)
               {
                  //static_assert( message_max_size - payload_max_size < payload_max_size, "Payload is to small");
                  memset( &payload, 0, sizeof( Payload));
               }

               struct Payload
               {

                  message_type_type type;

                  Header header;

                  payload_type payload;

               } payload;


               void* raw() { return &payload;}

               std::size_t size() const { return m_size; }

               void size( std::size_t size)
               {
                  m_size = size;
               }

               std::size_t paylodSize() { return m_size - sizeof( Header);}
               void paylodSize( std::size_t size) { m_size = size +  sizeof( Header);}

               friend std::ostream& operator << ( std::ostream& out, const Transport& value)
               {
                  return out << "{type: " << value.payload.type << " header: {correlation: " << common::uuid::string( value.payload.header.correlation)
                        << " count:" << value.payload.header.count << "} size: " << value.size() << "}";
               }


            private:

               std::size_t m_size;
            };


            struct Complete
            {
               typedef platform::message_type_type message_type_type;
               typedef platform::binary_type payload_type;

               Complete() = default;

               Complete( Transport& transport);

               Complete( message_type_type messageType, platform::binary_type&& buffer)
                  : type( messageType), correlation( Uuid::make()), payload( std::move( buffer)), complete( true)
               {}

               Complete( Complete&&) = default;
               Complete& operator = ( Complete&&) = default;

               void add( Transport& transport);

               message_type_type type;
               Uuid correlation;
               payload_type payload;
               bool complete = false;

               friend std::ostream& operator << ( std::ostream& out, const Complete& value)
               {
                  return out << "{ type: " << value.type << " correlation: " << value.correlation << " size: "
                        << value.payload.size() << " complete: " << value.complete << '}';
               }
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
               bool operator () ( const message::Complete& message) const
               {
                  return send( message, 0);
               }

               //!
               //! Tries to send the logical message
               //!
               //! @return true if sent, false otherwise
               //!
               bool operator () ( const message::Complete& message, const long flags) const
               {
                  return send( message, flags);
               }

            private:

               bool send( message::Transport& message, const long flags) const;
               bool send( const message::Complete& message, const long flags) const;
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
               //! Clear and discard all messages in queue.
               //!
               void clear();

            private:

               typedef std::vector< message::Complete> cache_type;
               using range_type = decltype( range::make( cache_type::iterator(), cache_type::iterator()));

               template< typename P>
               range_type find( P predicate, const long flags);

               bool receive( message::Transport& message, const long flags);

               range_type cache( message::Transport& message);

               common::file::scoped::Path m_path;

               cache_type m_cache;
            };
         }

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
