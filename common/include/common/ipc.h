//!
//! casual_ipc.h
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_IPC_H_
#define CASUAL_IPC_H_

#include "xatmi.h"

#include "common/file.h"
#include "common/uuid.h"
#include "common/platform.h"

#include "common/types.h"

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
               typedef common::platform::queue_id_type queue_id_type;
               typedef common::platform::queue_key_type queue_key_type;
               typedef common::platform::message_type_type message_type_type;
               typedef common::Uuid::uuid_type correalation_type;


               struct Header
               {
                  correalation_type m_correlation;
                  std::size_t m_pid;
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
         }


         namespace internal
         {
            class base_queue
            {
            public:
               typedef common::platform::queue_id_type queue_id_type;
               typedef common::platform::queue_key_type queue_key_type;

               base_queue() = default;

               base_queue( base_queue&& rhs)
               {
                  m_key = rhs.m_key;
                  m_id = rhs.m_id;
                  rhs.m_key = 0;
                  rhs.m_id = 0;
               }

               queue_key_type getKey() const;

            protected:
               queue_key_type m_key = 0;
               queue_id_type m_id = 0;
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

               Queue( queue_key_type key);

               Queue( Queue&&) = default;


               Queue( const Queue&) = delete;
               Queue& operator = ( const Queue&) = delete;


               bool operator () ( message::Transport& message) const
               {
                  return send( message, 0);
               }

               bool operator () ( message::Transport& message, const long flags) const
               {
                  return send( message, flags);
               }

            private:

               bool send( message::Transport& message, const long flags) const;
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


               bool operator () ( message::Transport& message) const
               {
                  return receive( message, 0);
               }

               bool operator () ( message::Transport& message, const long flags) const
               {
                  // TODO: constraint on flags?
                  return receive( message, flags);
               }

            private:


               bool receive( message::Transport& message, const long flags) const;

               common::file::ScopedPath m_scopedPath;
            };
         }


         send::Queue& getBrokerQueue();

         receive::Queue& getReceiveQueue();

      }
   }
}




#endif /* CASUAL_IPC_H_ */
