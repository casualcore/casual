//!
//! casual_queue.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_H_
#define CASUAL_QUEUE_H_

#include "common/ipc.h"
#include "common/message.h"
#include "common/marshal.h"
#include "common/signal.h"


#include <list>

namespace casual
{
   namespace common
   {

      namespace queue
      {
         typedef ipc::message::Transport transport_type;
         typedef transport_type::message_type_type message_type_type;

         namespace blocking
         {

            class Writer
            {
            public:

               typedef ipc::send::Queue ipc_type;

               Writer( ipc::send::Queue& queue);

               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               template< typename M>
               void operator () ( M& message)
               {
                  //
                  // Serialize the message
                  //
                  marshal::output::Binary archive;
                  archive << message;

                  message_type_type type = message::type( message);

                  send( archive, type);

               }
            private:

               void send( marshal::output::Binary& archive, message_type_type type);

               ipc_type& m_queue;
            };


            //!
            //!
            //!
            class Reader
            {
            public:

               typedef ipc::receive::Queue ipc_type;

               Reader( ipc::receive::Queue& queue);

               //!
               //! Gets the next binary-message from queue.
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               marshal::input::Binary next();

               //!
               //! Tries to read a specific message from the queue.
               //! If other message types is consumed before the requested type
               //! they will be cached.
               //!
               //! @attention Will block until the specific message-type can be read from the queue
               //!
               template< typename M>
               void operator () ( M& message)
               {
                  message_type_type type = message::type( message);

                  marshal::input::Binary archive;

                  correlate( archive, type);

                  archive >> message;
               }


            private:

               void correlate( marshal::input::Binary& archive, message_type_type type);

               ipc_type& m_queue;

            };
         } // blocking

         namespace non_blocking
         {
            //!
            //! Non blocking writer. There are only a few cases where we use this semantic
            //!
            class Writer
            {
            public:

               typedef ipc::send::Queue ipc_type;

               Writer( ipc::send::Queue& queue);

               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @note non-blocking
               //! @return true if the whole message is sent. false otherwise
               //!
               template< typename M>
               bool operator () ( M& message)
               {
                  //
                  // Serialize the message
                  //
                  marshal::output::Binary archive;
                  archive << message;

                  message_type_type type = message::type( message);

                  return send( archive, type);

               }
            private:

               bool send( marshal::output::Binary& archive, message_type_type type);

               ipc_type& m_queue;
            };


            //!
            //! Non-blocking reader
            //!
            class Reader
            {
            public:

               typedef ipc::receive::Queue ipc_type;

               Reader( ipc::receive::Queue& queue);

               //!
               //! Tries to get the next binary-message from queue.
               //!
               //! @return 0..1 binary-marshal that can be used to deserialize an actual message.
               //!
               std::vector< marshal::input::Binary> next();


               //!
               //! Tries to read a specific message from the queue.
               //! non-blocking
               //! @return true if the specific message-type is read. false otherwise.
               //!
               template< typename M>
               bool operator() ( M& message)
               {
                  message_type_type type = message::type( message);

                  marshal::input::Binary archive;

                  if( correlate( archive, type))
                  {
                     archive >> message;
                     return true;
                  }
                  return false;
               }

               //!
               //! Consumes all transport messages that is present on the ipc-queue, and
               //! stores these to cache.
               //!
               //! @note non blocking
               //!
               bool consume();


            private:

               bool correlate( marshal::input::Binary& archive, message_type_type type);

               ipc_type& m_queue;

            };

         } // non_blocking


         //!
         //! Wrapper that exposes the queue interface and holds the ipc resource
         //!
         template< typename Q, typename I = typename Q::ipc_type>
         struct ipc_wrapper
         {
            typedef Q queue_type;
            typedef I ipc_type;

            template< typename... Args>
            ipc_wrapper( Args&& ...args) : m_ipcQueue( std::forward< Args>( args)...), m_queue( m_ipcQueue) {}

            ipc_wrapper( ipc_wrapper&&) = default;


            //!
            //! Reads or writes to/from the queue, depending on the type of queue_type
            //!
            template< typename T>
            auto operator () ( T& value) -> decltype( std::declval<Q>()( value))
            {
               return m_queue( value);
            }

            const ipc_type& ipc() const
            {
               return m_ipcQueue;
            }

         private:
            ipc_type m_ipcQueue;
            queue_type m_queue;
         };


      } // queue
   } // common
} // casual


#endif /* CASUAL_QUEUE_H_ */
