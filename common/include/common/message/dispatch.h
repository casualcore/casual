//!
//! message_dispatch.h
//!
//! Created on: Dec 1, 2012
//!     Author: Lazan
//!

#ifndef MESSAGE_DISPATCH_H_
#define MESSAGE_DISPATCH_H_

#include "common/marshal/binary.h"
#include "common/queue.h"


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

               using message_type = platform::message_type_type;

               Handler()  = default;

               template< typename... Args>
               Handler( Args&& ...handlers) : m_handlers( assign( std::forward< Args>( handlers)...))
               {

               }

               //!
               //! Dispatch a message.
               //!
               //! @return true if the message was handled.
               //!
               template< typename M>
               bool dispatch( M&& complete)
               {
                  return do_dispatch( complete);
               }

               std::size_t size() const;

               //!
               //! @return all message-types that this instance handles
               //!
               std::vector< message_type> types() const;


            private:


               bool do_dispatch( ipc::message::Complete& complete);
               bool do_dispatch( std::vector<ipc::message::Complete>& complete);

               class base_handler
               {
               public:
                  virtual ~base_handler() = default;
                  virtual void marshal( ipc::message::Complete& complete) = 0;


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

                  void marshal( ipc::message::Complete& complete)
                  {
                     message_type message;
                     complete >> message;

                     m_handler.dispatch( message);
                  }

               private:
                  handler_type m_handler;
               };


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
