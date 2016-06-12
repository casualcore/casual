//!
//! message_dispatch.h
//!
//! Created on: Dec 1, 2012
//!     Author: Lazan
//!

#ifndef MESSAGE_DISPATCH_H_
#define MESSAGE_DISPATCH_H_

#include "common/marshal/binary.h"
#include "common/execution.h"
#include "common/communication/message.h"
#include "common/traits.h"


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

               using message_type = message::Type;

               Handler()  = default;

               Handler( Handler&&) = default;
               Handler& operator = ( Handler&&) = default;

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
               bool operator () ( M&& complete) const
               {
                  return dispatch( complete);
               }

               std::size_t size() const;

               //!
               //! @return all message-types that this instance handles
               //!
               std::vector< message_type> types() const;


               //!
               //! Inserts handler, that is, adds new handlers
               //!
               //! @param handlers
               template< typename... Args>
               void insert( Args&&... handlers)
               {
                  assign( m_handlers, std::forward< Args>( handlers)...);
               }

            private:

               bool dispatch( communication::message::Complete& complete) const;

               class base_handler
               {
               public:
                  virtual ~base_handler() = default;
                  virtual void dispatch( communication::message::Complete& complete) = 0;
               };


               template< typename H>
               class handle_holder : public base_handler
               {
               public:

                  typedef H handler_type;

                  using traits_type = traits::function< H>;

                  static_assert( traits_type::arguments() == 1, "handlers has to have this signature: void( <some message>), can be declared const");
                  static_assert(
                        std::is_same< typename traits_type::result_type, void>::value
                        || std::is_same< typename traits_type::result_type, bool>::value , "handlers has to have this signature: void|bool( <some message>), can be declared const");

                  using message_type = typename std::decay< typename traits_type::template argument< 0>::type>::type;


                  handle_holder( handle_holder&&) = default;
                  handle_holder& operator = ( handle_holder&&) = default;


                  handle_holder( handler_type&& handler) : m_handler( std::move( handler)) {}


                  void dispatch( communication::message::Complete& complete) override
                  {
                     message_type message;
                     complete >> message;

                     execution::id( message.execution);

                     m_handler( message);
                  }

               private:

                  handler_type m_handler;
               };


               typedef std::map< message_type, std::unique_ptr< base_handler> > handlers_type;


               static void assign( handlers_type& result)
               {
               }

               template< typename H, typename... Args>
               static void assign( handlers_type& result, H&& handler, Args&& ...handlers)
               {
                  using handle_type = handle_holder< typename std::decay< H>::type>;

                  //
                  //  We need to override handlers in unittest.
                  //
                  // assert( result.count( handle_type::message_type::type()) == 0);

                  std::unique_ptr< base_handler> holder{ new handle_type( std::forward< H>( handler))};

                  result.emplace(
                        handle_type::message_type::type(),
                        std::move( holder));

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

            template< typename D, typename Policy>
            void pump( Handler& handler, D& device, Policy&& policy)
            {
               while( handler( device.next( policy)))
               {
                  ;
               }
            }

            namespace blocking
            {
               template< typename D>
               void pump( Handler& handler, D& device)
               {
                  using device_type = typename std::decay< decltype( device)>::type;

                  while( true)
                  {
                     handler( device.next( typename device_type::blocking_policy{}));
                  }
               }

            } // blocking

         } // dispatch
      } // message
   } // common
} // casual


#endif /* MESSAGE_DISPATCH_H_ */
