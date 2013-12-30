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
#include "common/log.h"


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

               handle_holder( handle_holder&&) = default;
               handle_holder& operator = ( handle_holder&&) = default;


               template< typename... Arguments>
               handle_holder( Arguments&&... arguments) : m_handler{ std::forward< Arguments>( arguments)...} {}

               void marshal( marshal::input::Binary& binary)
               {
                  message_type message;
                  binary >> message;

                  m_handler.dispatch( message);
               }

            private:
               handler_type m_handler;
            };


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

               template< typename H>
               void add( H&& handler)
               {
                  // TODO: change to std::make_unique
                  handlers_type::mapped_type holder(
                        new handle_holder< H>{ std::forward< H>( handler)});

                  m_handlers[ H::message_type::message_type] = std::move( holder);
               }

               template< typename B>
               bool dispatch( B&& binary)
               {
                  return doDispatch( binary);
               }

               std::size_t size() const
               {
                  return m_handlers.size();
               }

            private:

               bool doDispatch( marshal::input::Binary& binary)
               {
                  auto findIter = m_handlers.find( binary.type());

                  if( findIter != std::end( m_handlers))
                  {
                     findIter->second->marshal( binary);
                     return true;
                  }
                  else
                  {
                     common::log::error << "message_type: " << binary.type() << " not recognized - action: discard" << std::endl;
                  }
                  return false;
               }


               bool doDispatch( std::vector< marshal::input::Binary>& binary)
               {
                  if( binary.empty())
                  {
                     return false;
                  }

                  doDispatch( binary.front());

                  return true;
               }


               handlers_type m_handlers;
            };

            template< typename RQ>
            void pump( Handler& handler, RQ&& receiveQueue)
            {
               while( true)
               {
                  auto marshal = receiveQueue.next();

                   handler.dispatch( marshal);
               }
            }


         } // dispatch
      } // message
   } // common
} // casual


#endif /* MESSAGE_DISPATCH_H_ */
