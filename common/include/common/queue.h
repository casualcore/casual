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


#include <list>

namespace casual
{
   namespace common
   {

      namespace queue
      {
         typedef ipc::message::Transport transport_type;
         typedef transport_type::message_type_type message_type_type;


         namespace policy
         {
            struct NoAction
            {
               void apply()
               {
                  throw;
               }
            };


            struct Blocking
            {
               enum Flags
               {
                  flags = 0
               };


               template< typename M, typename IPC>
               static void send( M&& message, IPC&& ipc)
               {
                  ipc( std::forward< M>( message), flags);
               }


               template< typename IPC>
               static marshal::input::Binary next( IPC&& ipc)
               {
                  auto message = ipc( flags);

                  assert( ! message.empty());

                  return marshal::input::Binary( std::move( message.front()));
               }

               template< typename IPC, typename M>
               static void fetch( IPC&& ipc, M& message)
               {
                  auto type = message::type( message);

                  auto transport = ipc( type, flags);

                  assert( ! transport.empty());

                  marshal::input::Binary marshal( std::move( transport.front()));
                  marshal >> message;
               }

            };

            struct NonBlocking
            {
               enum Flags
               {
                  flags = ipc::receive::Queue::cNoBlocking
               };

               template< typename M, typename IPC>
               static bool send( M&& message, IPC& ipc)
               {
                  return ipc( std::forward< M>( message), flags);
               }


               template< typename IPC>
               static std::vector< marshal::input::Binary> next( IPC&& ipc)
               {
                  std::vector< marshal::input::Binary> result;

                  auto message = ipc( flags);

                  if( ! message.empty())
                  {
                     result.emplace_back( std::move( message.front()));
                  }

                  return result;
               }

               template< typename IPC, typename M>
               static bool fetch( IPC&& ipc, M& message)
               {
                  auto type = message::type( message);

                  auto transport = ipc( type, flags);

                  if( ! transport.empty())
                  {
                     marshal::input::Binary marshal( std::move( transport.front()));
                     marshal >> message;
                     return true;
                  }

                  return false;
               }

            };


         } // policy

         namespace internal
         {


            template< typename BP, typename P, typename IPC>
            struct basic_writer
            {
            public:
               typedef BP block_policy;
               typedef P policy_type;
               typedef IPC ipc_value_type;
               using ipc_type = typename std::decay< ipc_value_type>::type;

               template< typename... Args>
               basic_writer( ipc_value_type ipc, Args&&... args)
                  : m_ipc( ipc), m_policy( std::forward< Args>( args)...) {}


               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @note non-blocking
               //! @return true if the whole message is sent. false otherwise
               //!
               template< typename M>
               auto operator () ( M&& message) -> decltype( block_policy::send( std::declval< ipc::message::Complete>(), std::declval< ipc_value_type>()))
               {
                  auto transport = prepare( std::forward< M>( message));

                  return policy_send( transport);
               }

               const ipc_value_type ipc() const
               {
                  return m_ipc;
               }

            private:

               template< typename M>
               static ipc::message::Complete prepare( M&& message)
               {
                  //
                  // Serialize the message
                  //
                  marshal::output::Binary archive;
                  archive << message;

                  message_type_type type = message::type( message);
                  return ipc::message::Complete( type, archive.release());
               }

               auto policy_send( ipc::message::Complete& transport) -> decltype( block_policy::send( transport, std::declval< ipc_value_type>()))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::send( transport, m_ipc);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

               ipc_value_type m_ipc;
               policy_type m_policy;

            };



            template< typename BP, typename P, typename IPC>
            class basic_reader
            {
            public:

               typedef BP block_policy;
               typedef P policy_type;
               typedef IPC ipc_value_type;
               using ipc_type = typename std::decay< ipc_value_type>::type;

               template< typename... Args>
               basic_reader( ipc_value_type ipc, Args&&... args)
                  : m_ipc( ipc), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Gets the next binary-message from queue.
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               auto next() -> decltype( block_policy::next( std::declval< ipc_value_type>()))
               {
                  return policy_next();
               }

               //!
               //! Tries to read a specific message from the queue.
               //! If other message types is consumed before the requested type
               //! they will be cached.
               //!
               //! @attention Will block until the specific message-type can be read from the queue
               //!
               template< typename M>
               auto operator () ( M& message) -> decltype( block_policy::fetch( std::declval< ipc_value_type>(), message))
               {
                  return policy_read( message);
               }

               const ipc_value_type ipc() const
               {
                  return m_ipc;
               }

            private:

               auto policy_next() -> decltype( block_policy::next( std::declval< ipc_value_type>()))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::next( m_ipc);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

               template< typename M>
               auto policy_read( M& message) -> decltype( block_policy::fetch( std::declval< ipc_value_type>(), message))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::fetch( m_ipc, message);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

               ipc_value_type m_ipc;
               policy_type m_policy;
            };



         } // internal


         namespace blocking
         {

            template< typename P>
            using basic_writer = internal::basic_writer< policy::Blocking, P, ipc::send::Queue&>;

            typedef basic_writer< policy::NoAction> Writer;


            template< typename P>
            using basic_reader = internal::basic_reader< policy::Blocking, P, ipc::receive::Queue&>;

            typedef basic_reader< policy::NoAction> Reader;


         } // blocking

         namespace non_blocking
         {

            template< typename P>
            using basic_writer = internal::basic_writer< policy::NonBlocking, P, ipc::send::Queue&>;

            typedef basic_writer< policy::NoAction> Writer;

            template< typename P>
            using basic_reader = internal::basic_reader< policy::NonBlocking, P, ipc::receive::Queue&>;

            typedef basic_reader< policy::NoAction> Reader;


         } // non_blocking


         //!
         //! Wrapper that exposes the queue interface and holds the ipc resource
         //!
         template< typename Q, typename I = typename Q::ipc_type>
         struct ipc_wrapper
         {
            typedef Q queue_type;
            typedef I ipc_type;
            typedef typename ipc_type::id_type id_type;


            template< typename... Args>
            ipc_wrapper( id_type id, Args&& ...args) : m_ipcQueue{ id}, m_queue{ m_ipcQueue, std::forward< Args>( args)...} {}


            ipc_wrapper( ipc_wrapper&&) = default;


            //!
            //! Reads or writes to/from the queue, depending on the type of queue_type
            //!
            template< typename T>
            auto operator () ( T&& value) -> decltype( std::declval<Q>()( std::forward< T>( value)))
            {
               return m_queue( std::forward< T>( value));
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
