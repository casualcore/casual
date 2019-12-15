//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/execution.h"
#include "common/communication/message.h"
#include "common/traits.h"
#include "common/serialize/native/complete.h"
#include "common/log/stream.h"


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

               basic_handler( basic_handler&&) noexcept = default;
               basic_handler& operator = ( basic_handler&&) noexcept = default;

               template< typename... Args>
               basic_handler( Args&& ...handlers) : m_handlers( assign( std::forward< Args>( handlers)...))
               {
               }

               //! Dispatch a message.
               //!
               //! @return true if the message was handled.
               template< typename M>
               bool operator () ( M&& complete) const
               {
                  return dispatch( complete);
               }

               platform::size::type size() const { return m_handlers.size();}

               //! @return all message-types that this instance handles
               auto types() const
               {
                  std::vector< message_type> result;

                  for( auto& entry : m_handlers)
                  {
                     result.push_back( entry.first);
                  }

                  return result;
               }

               //! Inserts handler, that is, adds new handlers
               //!
               //! @param handlers
               template< typename... Args>
               void insert( Args&&... handlers)
               {
                  assign( m_handlers, std::forward< Args>( handlers)...);
               }

               basic_handler& operator += ( basic_handler&& other)
               {
                  add( m_handlers, std::move( other));
                  return *this;
               }

               friend basic_handler operator + ( basic_handler&& lhs, basic_handler&& rhs)
               {
                  lhs += std::move( rhs);
                  return std::move( lhs);
               }

               // for logging only
               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               {
                  CASUAL_SERIALIZE_NAME( m_handlers, "handlers");
               })

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
                        log::line( log::category::error, "message_type: ", complete.type, " not recognized - action: discard");
                     }
                  }
                  return false;
               }

               struct concept
               {
                  virtual ~concept() = default;
                  virtual void dispatch( communication::message::Complete& complete) = 0;
               };


               template< typename H>
               struct model final : public concept
               {
                  using handler_type = H;
                  using traits_type = traits::function< H>;

                  static_assert( traits_type::arguments() == 1, "handlers has to have this signature: void( <some message>), can be declared const");
                  static_assert(
                        std::is_same< typename traits_type::result_type, void>::value
                        || std::is_same< typename traits_type::result_type, bool>::value , "handlers has to have this signature: void|bool( <some message>), can be declared const");

                  using message_type = std::decay_t< typename traits_type::template argument< 0>::type>;


                  model( model&&) = default;
                  model& operator = ( model&&) = default;

                  model( handler_type&& handler) : m_handler( std::move( handler)) {}

                  void dispatch( communication::message::Complete& complete) override
                  {
                     message_type message;

                     serialize::native::complete( complete, message, unmarshal_type{});
                     execution::id( message.execution);

                     m_handler( message);
                  }

               private:
                  handler_type m_handler;
               };


               using handlers_type = std::map< message_type, std::unique_ptr< concept>>;


               template< typename H>
               static void add( handlers_type& result, H&& handler)
               {
                  using handle_type = model< typename std::decay< H>::type>;

                  auto holder = std::make_unique< handle_type>( std::forward< H>( handler));

                  result[ handle_type::message_type::type()] = std::move( holder);
               }


               static void add( handlers_type& result, basic_handler&& holder)
               {
                  for( auto&& handler : holder.m_handlers)
                  {
                     result[ handler.first] = std::move( handler.second);
                  }
               }

               static void assign( handlers_type& result) { }

               template< typename H, typename... Args>
               static void assign( handlers_type& result, H&& handler, Args&& ...handlers)
               {
                  add( result, std::forward< H>( handler));
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

            
            namespace empty
            {
               //! 
               template< typename Unmarshal, typename D, typename BP, typename EC, typename NBL = int>
               void pump( 
                  basic_handler< Unmarshal>& handler, 
                  D& device, 
                  BP&& blocking_predicate, 
                  EC&& empty_callback,
                  NBL non_blocking_limit = std::numeric_limits< NBL>::max())
               {
                  using device_type = std::decay_t< decltype( device)>;

                  while( true)
                  {
                     if( blocking_predicate())
                     {
                        handler( device.next( typename device_type::blocking_policy{}));
                     }
                     else
                     {
                        while( handler( device.next( typename device_type::non_blocking_policy{})) && non_blocking_limit-- > 0)
                           ; /* no op */

                        if( non_blocking_limit > 0)
                           empty_callback();
                     }
                  }   
               }

               template< typename Unmarshal, typename D, typename EC>
               void pump( 
                  basic_handler< Unmarshal>& handler, 
                  D& device, 
                  EC&& empty_callback)
               {
                  using device_type = std::decay_t< decltype( device)>;

                  while( true)
                  {
                     while( handler( device.next( typename device_type::non_blocking_policy{})))
                     {
                        ; /* no op */
                     }

                     // input is empty, we call the callback
                     empty_callback();

                     // we block
                     handler( device.next( typename device_type::blocking_policy{}));
                  }   
               }
            } // empty
            

            namespace blocking
            {
               template< typename Unmarshal, typename D>
               void pump( basic_handler< Unmarshal>& handler, D& device)
               {
                  using device_type = std::decay_t< decltype( device)>;

                  while( true)
                     handler( device.next( typename device_type::blocking_policy{}));
               }

               namespace restriced
               {
                  // only consume messages that handler can handle
                  template< typename Unmarshal, typename D>
                  void pump( basic_handler< Unmarshal>& handler, D& device)
                  {
                     using device_type = std::decay_t< decltype( device)>;

                     const auto types = handler.types();

                     while( true)
                        handler( device.next( types, typename device_type::blocking_policy{}));
                  }
               } // restriced
            } // blocking

         } // dispatch
      } // message
   } // common
} // casual



