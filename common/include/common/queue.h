//!
//! casual_queue.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_QUEUE_H_
#define CASUAL_COMMON_QUEUE_H_

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
            struct Ignore
            {
               inline void apply() {}
            };

            struct NoAction
            {
               inline void apply()
               {
                  throw;
               }
            };

            //!
            //! A common policy that removes pids from state when
            //! process terminates
            //!
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


               template< typename M>
               static Uuid send( M&& message, ipc::send::Queue& ipc)
               {
                  return ipc( std::forward< M>( message), flags);
               }

               static marshal::input::Binary next( ipc::receive::Queue& ipc);

               static marshal::input::Binary next( ipc::receive::Queue& ipc, const std::vector< platform::message_type_type>& types);

               template< typename M>
               static void fetch( ipc::receive::Queue& ipc, M& message)
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

               template< typename M>
               static Uuid send( M&& message, ipc::send::Queue& ipc)
               {
                  return ipc( std::forward< M>( message), flags);
               }

               static std::vector< marshal::input::Binary> next( ipc::receive::Queue& ipc);

               static std::vector< marshal::input::Binary> next( ipc::receive::Queue& ipc, const std::vector< platform::message_type_type>& types);


               template< typename M>
               static bool fetch( ipc::receive::Queue& ipc, M& message)
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
            template< typename BP, typename P>
            struct basic_send
            {
            public:
               typedef BP block_policy;
               typedef P policy_type;


               template< typename... Args>
               basic_send( Args&&... args)
                  : m_policy( std::forward< Args>( args)...) {}


               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @param target the ipc-queue-id that the message will be sent to
               //! @param message the message that will be sent
               //!
               //! @note depening on policy, it's either blocking och non-blockin
               //! @return non-blocking: true if the whole message is sent. false otherwise - void on blocking
               //!
               template< typename M>
               Uuid operator () ( platform::queue_id_type target, M&& message) //-> decltype( block_policy::send( message, std::declval< ipc::send::Queue&>()))
               {
                  auto transport = prepare( std::forward< M>( message));

                  ipc::send::Queue ipc( target);

                  return send( transport, ipc);
               }


               //!
               //! Sends/Writes a "complete" message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @param target the ipc-queue-id that the message will be sent to
               //! @param transport the 'complete' message that will be sent
               //!
               //! @return depending on block_policy, if blocking void, if non-blocking  true if message is sent, false otherwise
               //!
               Uuid send( platform::queue_id_type target, const ipc::message::Complete& transport) // -> decltype( block_policy::send( transport, std::declval< ipc::send::Queue&>()))
               {
                  ipc::send::Queue ipc( target);

                  return send( transport, ipc);
               }

            private:

               Uuid send( const ipc::message::Complete& transport, ipc::send::Queue& ipc) // -> decltype( block_policy::send( transport, ipc))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::send( transport, ipc);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

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

               policy_type m_policy;
            };


            //!
            //! @deprecated use basic_send instead...
            //!
            template< typename BP, typename P>
            struct basic_writer : public basic_send< BP, P>
            {
            public:
               using base_type = basic_send< BP, P>;
               typedef BP block_policy;
               typedef P policy_type;


               template< typename... Args>
               basic_writer( platform::queue_id_type id, Args&&... args)
                  : base_type( std::forward< Args>( args)...), m_ipc( id) {}



               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @note depening on policy, it's either blocking och non-blockin
               //! @return non-blocking: true if the whole message is sent. false otherwise - void on blocking
               //!
               template< typename M>
               auto operator () ( M&& message) -> decltype( block_policy::send( message, std::declval< ipc::send::Queue&>()))
               {
                  return base_type::operator ()( m_ipc, std::forward< M>( message));
               }


               //!
               //! Sends/Writes a "complete" message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @return depending on block_policy, if blocking void, if non-blocking  true if message is sent, false otherwise
               //!
               auto send( const ipc::message::Complete& transport) -> decltype( block_policy::send( transport, std::declval< ipc::send::Queue&>()))
               {
                  return base_type::send( m_ipc, transport);
               }

            private:

               platform::queue_id_type m_ipc;
            };



            template< typename BP, typename P>
            class basic_reader
            {
            public:

               typedef BP block_policy;
               typedef P policy_type;
               //typedef IPC ipc_value_type;
               //using ipc_type = typename std::decay< ipc_value_type>::type;
               using type_type = platform::message_type_type;

               template< typename... Args>
               basic_reader( ipc::receive::Queue& ipc, Args&&... args)
                  : m_ipc( ipc), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Gets the next binary-message from queue.
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               auto next() -> decltype( block_policy::next( std::declval< ipc::receive::Queue&>()))
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
               auto next( const std::vector< type_type>& types) -> decltype( block_policy::next( std::declval< ipc::receive::Queue&>(), { type_type()}))
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
               auto operator () ( M& message) -> decltype( block_policy::fetch( std::declval< ipc::receive::Queue&>(), message))
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

               const ipc::receive::Queue& ipc() const
               {
                  return m_ipc;
               }

            private:

               ipc::receive::Queue& m_ipc;
               policy_type m_policy;
            };



         } // internal


         namespace blocking
         {

            template< typename P>
            using basic_send = internal::basic_send< policy::Blocking, P>;

            //! @deprecated
            template< typename P>
            using basic_writer = internal::basic_writer< policy::Blocking, P>;

            //! @deprecated
            using Writer = basic_writer< policy::NoAction>;

            using Send = basic_send< policy::NoAction>;


            template< typename P>
            using basic_reader = internal::basic_reader< policy::Blocking, P>;

            typedef basic_reader< policy::NoAction> Reader;

            inline Reader reader( ipc::receive::Queue& ipc)
            {
               return Reader( ipc);
            }

            namespace remove
            {
               template< typename S>
               using basic_send = basic_send< policy::RemoveOnTerminate< S>>;

               //! @deprecated
               template< typename S>
               using basic_writer = basic_writer< policy::RemoveOnTerminate< S>>;

               template< typename S>
               using basic_reader = basic_reader< policy::RemoveOnTerminate< S>>;
            }



         } // blocking

         namespace non_blocking
         {
            template< typename P>
            using basic_send = internal::basic_send< policy::NonBlocking, P>;

            template< typename P>
            using basic_writer = internal::basic_writer< policy::NonBlocking, P>;

            typedef basic_writer< policy::NoAction> Writer;

            template< typename P>
            using basic_reader = internal::basic_reader< policy::NonBlocking, P>;

            typedef basic_reader< policy::NoAction> Reader;


            inline Reader reader( ipc::receive::Queue& ipc)
            {
               return Reader( ipc);
            }

            namespace remove
            {
               template< typename S>
               using basic_send = basic_send< policy::RemoveOnTerminate< S>>;

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
