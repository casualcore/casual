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
#include "common/marshal/binary.h"
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
               void apply();
            };

            struct Timeout
            {
               void apply();
            };

            struct NoAction
            {
               void apply();
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
                        m_state.process( death);
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

               struct Next
               {
                  template< typename... Args>
                  inline ipc::message::Complete operator () ( ipc::receive::Queue& ipc, Args&& ...args)
                  {
                     auto message = ipc( std::forward< Args>( args)..., flags);

                     assert( ! message.empty());

                     return std::move( message.front());
                  }
               };

               struct Fetch
               {
                  template< typename M, typename... Args>
                  inline void operator () ( ipc::receive::Queue& ipc, M& message, Args&& ...args)
                  {
                     auto complete = ipc( std::forward< Args>( args)..., flags);

                     assert( ! complete.empty());
                     assert( complete.front().type == message.message_type);
                     complete.front() >> message;

                  }
               };

            };

            struct NonBlocking
            {
               enum Flags
               {
                  flags = ipc::receive::Queue::cNoBlocking
               };

               struct Next
               {
                  template< typename... Args>
                  inline std::vector< ipc::message::Complete> operator () ( ipc::receive::Queue& ipc, Args&& ...args)
                  {
                     return ipc( std::forward< Args>( args)..., flags);
                  }

               };

               struct Fetch
               {
                  template< typename M, typename... Args>
                  inline bool operator () ( ipc::receive::Queue& ipc, M& message, Args&& ...args)
                  {
                     auto complete = ipc( std::forward< Args>( args)..., flags);

                     if( ! complete.empty())
                     {
                        assert( complete.front().type == message.message_type);
                        complete.front() >> message;
                        return true;
                     }

                     return false;
                  }
               };

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
               Uuid operator () ( platform::queue_id_type target, M&& message)
               {
                  auto complete = marshal::complete( message);

                  ipc::send::Queue ipc( target);

                  return send( complete, ipc);
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
               Uuid send( platform::queue_id_type target, const ipc::message::Complete& complete)
               {
                  ipc::send::Queue ipc( target);

                  return send( complete, ipc);
               }

            private:

               Uuid send( const ipc::message::Complete& complete, ipc::send::Queue& ipc)
               {
                  while( true)
                  {
                     try
                     {
                        return ipc( complete, block_policy::flags);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
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
               Uuid operator () ( M&& message)
               {
                  return base_type::operator ()( m_ipc, std::forward< M>( message));
               }


               //!
               //! Sends/Writes a "complete" message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @return depending on block_policy, if blocking void, if non-blocking  true if message is sent, false otherwise
               //!
               Uuid send( const ipc::message::Complete& transport)
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
               using next_type = typename block_policy::Next;
               using fetch_type = typename block_policy::Fetch;
               typedef P policy_type;
               using type_type = platform::message_type_type;

               template< typename... Args>
               basic_reader( ipc::receive::Queue& ipc, Args&&... args)
                  : m_ipc( ipc), m_policy( std::forward< Args>( args)...) {}


            private:

               template< typename... Args>
               auto get_next( Args&& ...args) -> decltype( next_type()( std::declval< ipc::receive::Queue&>(), std::forward< Args>( args)...))
               {
                  while( true)
                  {
                     try
                     {
                        return next_type()( m_ipc, std::forward< Args>( args)...);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

               template< typename M, typename... Args>
               auto fetch( M& message, Args&& ...args) -> decltype( fetch_type()( std::declval< ipc::receive::Queue&>(), message, std::forward< Args>( args)...))
               {
                  while( true)
                  {
                     try
                     {
                        return fetch_type()( m_ipc, message, std::forward< Args>( args)...);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

            public:

               //!
               //! Gets the next binary-message from queue.
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               //! Depending on the block_policy, it will either block or not.
               //!
               auto next() -> decltype( std::declval< basic_reader>().get_next())
               {
                  return get_next();
               }


               //!
               //! Gets the next binary-message from queue that is any of @p types
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               //! Depending on the block_policy, it will either block or not.
               //!
               auto next( const std::vector< type_type>& types) -> decltype( std::declval< basic_reader>().get_next( types))
               {
                  return get_next( types);
               }


               //!
               //! Tries to read a specific message from the queue.
               //! If other message types is consumed before the requested type
               //! they will be cached.
               //!
               //! Depending on the block_policy, it will either block or not.
               //!
               template< typename M>
               auto operator () ( M& message) -> decltype( std::declval< basic_reader>().fetch( message))
               {
                  return fetch( message, message.message_type);
               }

               template< typename M>
               auto operator () ( M& message, const Uuid& correlation) -> decltype( std::declval< basic_reader>().fetch( message, correlation))
               {
                  return fetch( message, correlation);
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

            template< typename M>
            auto call( platform::queue_id_type destination, M&& message) -> decltype( message::reverse::type( std::forward< M>( message)))
            {
               auto correlation = Send{}( destination, message);

               auto reply = message::reverse::type( std::forward< M>( message));
               blocking::Reader receive{ ipc::receive::queue()};
               receive( reply, correlation);

               return reply;
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

            using Send = basic_send< policy::NoAction>;

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


         template< typename QS, typename QR, typename I, typename O>
         void batch( QS&& send, I&& input, QR&& receive, O& output)
         {
            for( auto& message : input)
            {
               std::get< 1>( message).correlation = send( std::get< 0>( message), std::get< 1>( message));
            }

            for( auto& holder : input)
            {
               auto& message = std::get< 1>( holder);
               if( message.correlation)
               {
                  typename std::decay< decltype( output.front())>::type reply;

                  receive( reply, message.correlation);
                  output.push_back( std::move( reply));
               }
            }
         }

         template< typename QS, typename QR, typename I, typename O, typename T>
         void batch( QS&& send, I&& input, QR&& receive, O& output, T transform)
         {
            std::vector< typename std::decay< decltype( transform( input.front()))>::type> requests;

            range::transform( input, requests, transform);

            batch( send, requests, receive, output);

         }


      } // queue
   } // common
} // casual


#endif /* CASUAL_QUEUE_H_ */
