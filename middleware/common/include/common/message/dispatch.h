//!
//! casual
//!

#ifndef MESSAGE_DISPATCH_H_
#define MESSAGE_DISPATCH_H_

#include "common/execution.h"
#include "common/communication/message.h"
#include "common/traits.h"
#include "common/marshal/complete.h"


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

            template< typename Unmarshal>
            class basic_handler
            {
            public:

               using unmarshal_type = Unmarshal;
               using message_type = message::Type;

               basic_handler()  = default;

               basic_handler( basic_handler&&) = default;
               basic_handler& operator = ( basic_handler&&) = default;

               template< typename... Args>
               basic_handler( Args&& ...handlers) : m_handlers( assign( std::forward< Args>( handlers)...))
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

               std::size_t size() const { return m_handlers.size();}

               //!
               //! @return all message-types that this instance handles
               //!
               std::vector< message_type> types() const
               {
                  std::vector< message_type> result;

                  for( auto& entry : m_handlers)
                  {
                     result.push_back( entry.first);
                  }

                  return result;
               }


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

               bool dispatch( communication::message::Complete& complete) const
               {
                  if( complete)
                  {
                     auto findIter = m_handlers.find( complete.type);

                     if( findIter != std::end( m_handlers))
                     {
                        findIter->second->dispatch( complete);
                        return true;
                     }
                     else
                     {
                        common::log::error << "message_type: " << complete.type << " not recognized - action: discard" << std::endl;
                     }
                  }
                  return false;
               }

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

                     marshal::complete( complete, message, unmarshal_type{});
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

                  auto holder = make::unique< handle_type>( std::forward< H>( handler));

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

            template< typename Unmarshal, typename D, typename Policy>
            void pump( basic_handler< Unmarshal>& handler, D& device, Policy&& policy)
            {
               while( handler( device.next( policy)))
               {
                  ;
               }
            }

            namespace blocking
            {
               template< typename Unmarshal, typename D>
               void pump( basic_handler< Unmarshal>& handler, D& device)
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
