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

            class Handler
            {
            public:

               Handler()  = default;

               template< typename... Args>
               Handler( Args&& ...handlers) : m_handlers( assign( std::forward< Args>( handlers)...))
               {

               }


               template< typename... Args>
               void add( Args&& ...handlers)
               {
                  assign( m_handlers, std::forward< Args>( handlers)...);
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


                  handle_holder( handler_type&& handler) : m_handler( std::move( handler)) {}

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

               typedef std::map< platform::message_type_type, std::unique_ptr< base_handler> > handlers_type;

               template< typename H>
               static std::unique_ptr< base_handler> assign_helper( H&& handler)
               {
                  // TODO: change to std::make_unique
                  return std::unique_ptr< base_handler>(
                        new handle_holder< H>{ std::forward< H>( handler)});

               }

               template< typename H>
               static void assign( handlers_type& result, H&& handler)
               {
                  result.emplace( H::message_type::message_type, assign_helper( std::forward< H>( handler)));
               }

               template< typename H, typename... Args>
               static void assign( handlers_type& result, H&& handler, Args&& ...handlers)
               {
                  result.emplace( H::message_type::message_type, assign_helper( std::forward< H>( handler)));
                  assign( result, std::forward< Args>( handlers)...);
               }

               template< typename... Args>
               static handlers_type assign( Args&& ...handlers)
               {
                  handlers_type result;

                  assign( result, std::forward< Args>( handlers)...);

                  return result;
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
