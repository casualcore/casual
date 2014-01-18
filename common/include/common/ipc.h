


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
                  correalation_type m_correlation;
                  long m_count;

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
                  memset( &m_payload, 0, sizeof( Payload));
               }

               struct Payload
               {

                  message_type_type m_type;

                  Header m_header;

                  payload_type m_payload;

               } m_payload;


               void* raw() { return &m_payload;}

               std::size_t size() { return m_size; }

               void size( std::size_t size)
               {
                  m_size = size;
               }

               std::size_t paylodSize() { return m_size - sizeof( Header);}
               void paylodSize( std::size_t size) { m_size = size +  sizeof( Header);}

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

            };


         } // message





         namespace internal
         {
            class base_queue
            {
            public:
               typedef platform::queue_id_type id_type;

               base_queue() = default;
               base_queue( id_type id) : m_id( id) {}

               base_queue( base_queue&& rhs)
               {
                  m_id = rhs.m_id;
                  rhs.m_id = 0;
               }


               base_queue( const base_queue&) = delete;

               id_type id() const;

            protected:
               id_type m_id = 0;
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
               std::vector< message::Complete> operator () ( message::Complete::message_type_type type, const long flags);

            private:

               typedef std::deque< message::Complete> cache_type;

               template< typename P>
               cache_type::iterator find( P predicate, const long flags);

               bool receive( message::Transport& message, const long flags);

               cache_type::iterator cache( message::Transport& message);

               common::file::ScopedPath m_scopedPath;

               cache_type m_cache;
            };
         }

         namespace broker
         {
            send::Queue::id_type id();

            send::Queue& queue();

         } // broker


         //!
         //! @deprecated use broker::queue(), or probably easier: ipc::broker::id();
         //!
         send::Queue& getBrokerQueue();


         namespace receive
         {
            receive::Queue::id_type id();

            receive::Queue& queue();

         } // receive


         //!
         //! @deprecated use receive::queue()
         //!
         receive::Queue& getReceiveQueue();


         //!
         //! Removes an ipc-queue resource
         //!
         void remove( platform::queue_id_type id);


      } // ipc
   } // common
} // casual


#endif /* CASUAL_IPC_H_ */
