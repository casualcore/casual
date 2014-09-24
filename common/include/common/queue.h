//!
//! casual_queue.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_H_
#define CASUAL_QUEUE_H_

#include "common/ipc.h"
#include "common/message/type.h"
#include "common/marshal.h"
#include "common/process.h"


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

            template< typename S>
            struct RemoveOnTerminate
            {
               using state_type = S;

               RemoveOnTerminate( state_type& state) : m_state( state) {}

               void apply()
               {
                  try
                  {
                     throw;
                  }
                  catch( const exception::signal::child::Terminate& exception)
                  {
                     auto terminated = process::lifetime::ended();
                     for( auto& death : terminated)
                     {
                        switch( death.reason)
                        {
                           case process::lifetime::Exit::Reason::core:
                              log::error << death << std::endl;
                              break;
                           default:
                              log::internal::debug << death << std::endl;
                              break;
                        }

                        m_state.removeProcess( death.pid);
                     }
                  }
               }

            protected:
               state_type& m_state;
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

               template< typename IPC>
               static marshal::input::Binary next( IPC&& ipc, const std::vector< platform::message_type_type>& types)
               {
                  std::vector< marshal::input::Binary> result;

                  auto message = ipc( types, flags);

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

               template< typename IPC>
               static std::vector< marshal::input::Binary> next( IPC&& ipc, const std::vector< platform::message_type_type>& types)
               {
                  std::vector< marshal::input::Binary> result;

                  auto message = ipc( types, flags);

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

               template< typename ipc_id, typename... Args>
               basic_writer( ipc_id&& ipc, Args&&... args)
                  : m_ipc( std::forward< ipc_id>( ipc)), m_policy( std::forward< Args>( args)...) {}


               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @note non-blocking
               //! @return true if the whole message is sent. false otherwise
               //!
               template< typename M>
               auto operator () ( M&& message) -> decltype( block_policy::send( std::declval< ipc::message::Complete>(), std::declval< ipc_value_type&>()))
               {
                  auto transport = prepare( std::forward< M>( message));

                  return send( transport);
               }

               const ipc_value_type ipc() const
               {
                  return m_ipc;
               }


               //!
               //! Sends/Writes a "complete" message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @return depending on block_policy, if blocking void, if non-blocking  true if message is sent, false otherwise
               //!
               auto send( const ipc::message::Complete& transport) -> decltype( block_policy::send( transport, std::declval< ipc_value_type&>()))
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

            private:

               template< typename M>
               static ipc::message::Complete prepare( M&& message)
               {
                  //
                  // Serialize the message
                  //
                  marshal::output::Binary archive;
                  archive << message;

                  auto type = message::type( message);
                  return ipc::message::Complete( type, archive.release());
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
               using type_type = platform::message_type_type;

               template< typename... Args>
               basic_reader( ipc_value_type ipc, Args&&... args)
                  : m_ipc( ipc), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Gets the next binary-message from queue.
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               auto next() -> decltype( block_policy::next( std::declval< ipc_value_type>()))
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


               //!
               //! Gets the next binary-message from queue that is any of @p types
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               auto next( const std::vector< type_type>& types) -> decltype( block_policy::next( std::declval< ipc_value_type>(), { type_type()}))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::next( m_ipc, types);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
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

               const ipc_value_type ipc() const
               {
                  return m_ipc;
               }

            private:

               ipc_value_type m_ipc;
               policy_type m_policy;
            };



         } // internal


         namespace blocking
         {

            template< typename P>
            using basic_writer = internal::basic_writer< policy::Blocking, P, ipc::send::Queue>;

            typedef basic_writer< policy::NoAction> Writer;


            template< typename P>
            using basic_reader = internal::basic_reader< policy::Blocking, P, ipc::receive::Queue&>;

            typedef basic_reader< policy::NoAction> Reader;

            template< typename IPC>
            internal::basic_reader< policy::Blocking, policy::NoAction, IPC&> reader( IPC& ipc)
            {
               return internal::basic_reader< policy::Blocking, policy::NoAction, IPC&>( ipc);
            }

            namespace remove
            {
               template< typename S>
               using basic_writer = basic_writer< policy::RemoveOnTerminate< S>>;

               template< typename S>
               using basic_reader = basic_reader< policy::RemoveOnTerminate< S>>;
            }

         } // blocking

         namespace non_blocking
         {

            template< typename P>
            using basic_writer = internal::basic_writer< policy::NonBlocking, P, ipc::send::Queue>;

            typedef basic_writer< policy::NoAction> Writer;

            template< typename P>
            using basic_reader = internal::basic_reader< policy::NonBlocking, P, ipc::receive::Queue&>;

            typedef basic_reader< policy::NoAction> Reader;


            template< typename IPC>
            internal::basic_reader< policy::NonBlocking, policy::NoAction, IPC&> reader( IPC& ipc)
            {
               return internal::basic_reader< policy::NonBlocking, policy::NoAction, IPC&>( ipc);
            }

            namespace remove
            {
               template< typename S>
               using basic_writer = basic_writer< policy::RemoveOnTerminate< S>>;

               template< typename S>
               using basic_reader = basic_reader< policy::RemoveOnTerminate< S>>;
            }


         } // non_blocking

         template< typename Q>
         struct is_blocking : public std::integral_constant< bool, std::is_same< typename Q::block_policy, policy::Blocking>::value>
         {

         };


      } // queue
   } // common
} // casual


#endif /* CASUAL_QUEUE_H_ */
