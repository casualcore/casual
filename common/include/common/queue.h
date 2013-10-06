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
         } // policy


         namespace blocking
         {
            class base_writer
            {
            public:
               typedef ipc::send::Queue ipc_type;

               base_writer( ipc_type& queue);

            protected:

              void send( marshal::output::Binary& archive, message_type_type type);

              ipc_type& m_queue;
            };


            //!
            //! Policy based queue writer abstraction.
            //!
            //! @note blocking
            //!
            //! @tparam P policy type that shall implement void apply() and execute throw; Hence react to exception
            //!
            //! @attention template parameter B is only there to enable other bases for unittest, shold not be used in other code
            //!
            template< typename P, typename B = base_writer>
            class basic_writer : public B
            {
            public:
               typedef P policy_type;
               typedef B base_type;
               typedef typename base_type::ipc_type ipc_type;

               template< typename... Args>
               basic_writer( ipc_type& queue, Args&&... args)
                  : base_type( queue), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               template< typename M>
               void operator () ( M&& message)
               {
                  //
                  // Serialize the message
                  //
                  marshal::output::Binary archive;
                  archive << message;

                  message_type_type type = message::type( message);

                  policy_send( archive, type);
               }

            private:

               void policy_send( marshal::output::Binary& archive, message_type_type type)
               {
                  try
                  {
                     base_type::send( archive, type);
                  }
                  catch( ...)
                  {
                     m_policy.apply();
                     policy_send( archive, type);
                  }
               }

               policy_type m_policy;
            };

            typedef basic_writer< policy::NoAction> Writer;



            class base_reader
            {
            public:

               typedef ipc::receive::Queue ipc_type;

               base_reader( ipc::receive::Queue& queue);

            protected:

               marshal::input::Binary next();

               marshal::input::Binary read( message_type_type type);

               ipc_type& m_queue;

            };


            //!
            //! Policy based queue reader abstraction.
            //!
            //! @note blocking
            //!
            //! @tparam P policy type that shall implement void apply() and execute throw; Hence react to exception
            //!
            //! @attention template parameter B is only there to enable other bases for unittest, should not be used in other code
            //!
            template< typename P, typename B = base_reader>
            class basic_reader : public B
            {
            public:

               typedef P policy_type;
               typedef B base_type;
               typedef typename base_type::ipc_type ipc_type;

               template< typename... Args>
               basic_reader( ipc_type& queue, Args&&... args)
                  : base_type( queue), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Gets the next binary-message from queue.
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               marshal::input::Binary next()
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
               void operator () ( M&& message)
               {
                  message_type_type type = message::type( message);

                  marshal::input::Binary archive = policy_read( type);

                  archive >> message;
               }


            private:

               marshal::input::Binary policy_next()
               {
                  try
                  {
                     return base_type::next();
                  }
                  catch( ...)
                  {
                     m_policy.apply();
                     return policy_next();
                  }
               }

               marshal::input::Binary policy_read( message_type_type type)
               {
                  try
                  {
                     return base_type::read( type);
                  }
                  catch( ...)
                  {
                     m_policy.apply();
                     return policy_read( type);
                  }
               }

               policy_type m_policy;
            };

            typedef basic_reader< policy::NoAction> Reader;

         } // blocking

         namespace non_blocking
         {

            class base_writer
            {
            public:

               typedef ipc::send::Queue ipc_type;

               base_writer( ipc_type& queue);

            protected:

               bool send( marshal::output::Binary& archive, message_type_type type);

               ipc_type& m_queue;
            };


            //!
            //! Policy based queue writer abstraction.
            //!
            //! @note non-blocking
            //!
            //! @tparam P policy type that shall implement void apply() and execute throw; Hence react to exception
            //!
            //! @attention template parameter B is only there to enable other bases for unittest, should not be used in other code
            //!
            template< typename P, typename B = base_writer>
            class basic_writer : public B
            {
            public:

               typedef P policy_type;
               typedef B base_type;
               typedef typename base_type::ipc_type ipc_type;

               template< typename... Args>
               basic_writer( ipc_type& queue, Args&&... args)
                  : base_type( queue), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @note non-blocking
               //! @return true if the whole message is sent. false otherwise
               //!
               template< typename M>
               bool operator () ( M&& message)
               {
                  //
                  // Serialize the message
                  //
                  marshal::output::Binary archive;
                  archive << message;

                  message_type_type type = message::type( message);

                  return policy_send( archive, type);

               }
            private:

               bool policy_send( marshal::output::Binary& archive, message_type_type type)
               {
                  try
                  {
                     return base_type::send( archive, type);
                  }
                  catch( ...)
                  {
                     m_policy.apply();
                     return policy_send( archive, type);
                  }
               }

               policy_type m_policy;
            };

            typedef basic_writer< policy::NoAction> Writer;


            class base_reader
            {

            protected:

               typedef ipc::receive::Queue ipc_type;

               base_reader( ipc::receive::Queue& queue);

               std::vector< marshal::input::Binary> next();

               std::vector< marshal::input::Binary> read( message_type_type type);

            private:
               ipc_type& m_queue;
            };

            //!
            //! Policy based queue reader abstraction.
            //!
            //! @note non-blocking
            //!
            //! @tparam P policy type that shall implement void apply() and execute throw; Hence react to exception
            //!
            //! @attention template parameter B is only there to enable other bases for unittest, should not be used in other code
            //!
            template< typename P, typename B = base_reader>
            class basic_reader : public B
            {
            public:

               typedef P policy_type;
               typedef B base_type;
               typedef typename base_type::ipc_type ipc_type;

               template< typename... Args>
               basic_reader( ipc_type& queue, Args&&... args)
                  : base_type( queue), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Tries to get the next binary-message from queue.
               //!
               //! @return 0..1 binary-marshal that should be used to deserialize an actual message.
               //!
               std::vector< marshal::input::Binary> next()
               {
                  return policy_next();
               }


               //!
               //! Tries to read a specific message from the queue.
               //! non-blocking
               //! @return true if the specific message-type is read. false otherwise.
               //!
               template< typename M>
               bool operator() ( M&& message)
               {
                  message_type_type type = message::type( message);

                  auto binary = policy_read( type);

                  if( ! binary.empty())
                  {
                     binary.front() >> message;
                     return true;
                  }
                  return false;
               }

            private:

               std::vector< marshal::input::Binary> policy_next()
               {
                  try
                  {
                     return base_type::next();
                  }
                  catch( ...)
                  {
                     m_policy.apply();
                     return policy_next();
                  }
               }

               std::vector< marshal::input::Binary> policy_read( message_type_type type)
               {
                  try
                  {
                     return base_type::read( type);
                  }
                  catch( ...)
                  {
                     m_policy.apply();
                     return policy_read( type);
                  }
               }

               policy_type m_policy;

            };

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
