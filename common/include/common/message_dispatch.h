//!
//! message_dispatch.h
//!
//! Created on: Dec 1, 2012
//!     Author: Lazan
//!

#ifndef MESSAGE_DISPATCH_H_
#define MESSAGE_DISPATCH_H_

#include "common/marshal.h"
#include "common/queue.h"
#include "common/logger.h"


#include <map>
#include <memory>

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace dispatch
         {

            class base_handler
            {
            public:
               virtual ~base_handler() = default;
               virtual void marshal( marshal::input::Binary& binary) = 0;


            };

            template< typename H>
            class handle_holder : public base_handler
            {
            public:

               typedef H handler_type;
               typedef typename handler_type::message_type message_type;

               template< typename... Arguments>
               handle_holder( Arguments&&... arguments) : m_handler( std::forward< Arguments>( arguments)...) {}

               void marshal( marshal::input::Binary& binary)
               {
                  message_type message;
                  binary >> message;

                  m_handler.dispatch( message);
               }

            private:
               handler_type m_handler;
            };


            /*
             * To complicated... But works, and I got to "code-masturbate" for an hour or so...

            namespace policy
            {
               struct Default
               {
                  Default() : m_receiveQueue{ ipc::getReceiveQueue()} {}
                  Default( ipc::receive::Queue& ipc) : m_receiveQueue{ ipc} {}



                  template< typename T>
                  void start( T& handler)
                  {
                     while( true)
                     {
                        auto marshal = m_receiveQueue.next();

                        if( ! handler.dispatch( marshal))
                        {
                           common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
                        }
                     }
                  }


               private:
                  queue::blocking::Reader m_receiveQueue;
               };

            }

            namespace detail
            {
               template<class>
               struct sfinae_true : std::true_type{};


               template< typename T, typename ...Args>
               static auto test_start( int, Args&& ...args) -> sfinae_true< decltype( std::declval<T>().start( std::forward< Args>( args)...))>;

               template< typename, typename ...Args>
               static auto test_start( long, Args&&...) -> std::false_type;


               template< typename T, typename ...Args>
               struct has_start : decltype( test_start< T>( 0, std::declval< Args...>())) {};
            }



            template< typename P>
            class basic_handler
            {
            public:

               typedef P policy_type;
               typedef std::map< platform::message_type_type, std::unique_ptr< base_handler> > handlers_type;

               template< typename... Arguments>
               basic_handler( Arguments&& ...arguments) : m_policy( std::forward< Arguments>( arguments)...) {}

               template< typename H, typename... Arguments>
               void add( Arguments&& ...arguments)
               {
                  // TODO: change to std::make_unique
                  handlers_type::mapped_type handler(
                        new handle_holder< H>{ std::forward< Arguments>( arguments)...});

                  m_handlers[ H::message_type::message_type] = std::move( handler);
               }

               bool dispatch( marshal::input::Binary& binary)
               {
                  auto findIter = m_handlers.find( binary.type());

                  if( findIter != std::end( m_handlers))
                  {
                     findIter->second->marshal( binary);
                     return true;
                  }
                  return false;
               }

               template< typename ...Args>
               typename std::enable_if< detail::has_start< policy_type, basic_handler&, Args...>::value>::type
               start( Args&& ...args)
               {
                  return m_policy.start( *this, std::forward< Args>( args)...);
               }



               std::size_t size() const
               {
                  return m_handlers.size();
               }

            private:
               policy_type m_policy;
               handlers_type m_handlers;
            };

            typedef basic_handler< policy::Default> Handler;

            */

            class Handler
            {
            public:

               typedef std::map< platform::message_type_type, std::unique_ptr< base_handler> > handlers_type;

               template< typename H, typename... Arguments>
               void add( Arguments&& ...arguments)
               {
                  // TODO: change to std::make_unique
                  handlers_type::mapped_type handler(
                        new handle_holder< H>{ std::forward< Arguments>( arguments)...});

                  m_handlers[ H::message_type::message_type] = std::move( handler);
               }

               bool dispatch( marshal::input::Binary& binary)
               {
                  auto findIter = m_handlers.find( binary.type());

                  if( findIter != std::end( m_handlers))
                  {
                     findIter->second->marshal( binary);
                     return true;
                  }
                  return false;
               }


               std::size_t size() const
               {
                  return m_handlers.size();
               }

            private:
               handlers_type m_handlers;
            };


            void pump( Handler& handler, ipc::receive::Queue& ipc = ipc::getReceiveQueue())
            {
               queue::blocking::Reader receiveQueue( ipc);

               while( true)
               {
                  auto marshal = receiveQueue.next();

                  if( ! handler.dispatch( marshal))
                  {
                     common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
                  }
               }
            }


         } // dispatch
      } // message
   } // common
} // casual


#endif /* MESSAGE_DISPATCH_H_ */
