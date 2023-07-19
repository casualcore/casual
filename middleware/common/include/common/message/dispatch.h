//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/execution.h"
#include "common/traits.h"
#include "common/serialize/native/complete.h"
#include "common/communication/device.h"
#include "common/log/stream.h"
#include "common/functional.h"
#include "common/message/type.h"
#include "common/log.h"

#include "casual/overloaded.h"

#include <map>
#include <memory>

namespace casual
{
   namespace common::message::dispatch
   {

      template< typename Complete>
      class basic_handler
      {
      public:

         using complete_type = Complete;

         basic_handler()  = default;

         basic_handler( basic_handler&&) noexcept = default;
         basic_handler& operator = ( basic_handler&&) noexcept = default;

         template< typename... Args>
         explicit basic_handler( Args&& ...handlers) : m_handlers( assign( std::forward< Args>( handlers)...))
         {
         }

         //! Dispatch a message.
         //!
         //! @return true if the message was handled.
         template< typename M>
         auto operator () ( M&& complete)
         {
            return dispatch( complete);
         }

         platform::size::type size() const noexcept { return m_handlers.size();}

         //! @return all message-types that this instance handles
         auto types() const
         {
            return algorithm::transform( m_handlers, []( auto& entry){ return entry.first;});
         }

         //! Inserts handler, that is, adds new handlers
         //!
         //! @param handlers
         template< typename... Args>
         void insert( Args&&... handlers)
         {
            assign( m_handlers, std::forward< Args>( handlers)...);
         }

         basic_handler& operator += ( basic_handler other)
         {
            add( m_handlers, std::move( other));
            return *this;
         }

         friend basic_handler operator + ( basic_handler lhs, basic_handler&& rhs)
         {
            lhs += std::exchange( rhs, {});
            return lhs;
         }

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_handlers, "handlers");
         )

      private:

         strong::correlation::id dispatch( complete_type& complete)
         {
            if( ! complete)
               return {};

            if( auto found = algorithm::find( m_handlers, complete.type()))
            {
               if( found->second( complete))
                  return complete.correlation();
               else
                  return {};
            }

            log::line( log::category::error, code::casual::internal_unexpected_value, " message type: ", complete.type(), " not recognized - action: discard");
            return {};
         }

         using concept_t = common::unique_function< bool( complete_type&)>;


         template< typename H>
         struct model final
         {
            using handler_type = H;
            using traits_type = traits::function< H>;

            static_assert( traits_type::arguments() == 1, "handlers has to have this signature: void( <some message>), can be declared const");
            static_assert(
               concepts::any_of< typename traits_type::result_type, void, bool>,
               "handlers has to have this signature: void|bool( <some message>), can be declared const");

            using message_type = std::decay_t< typename traits_type::template argument< 0>::type>;


            model( model&&) = default;
            model& operator = ( model&&) = default;

            model( handler_type&& handler) : m_handler( std::move( handler)) {}

            bool operator() ( complete_type& complete)
            {
               message_type message;

               serialize::native::complete( complete, message);
               execution::id( message.execution);
               
               // if the handler returns bool, we return it
               if constexpr( std::is_same_v< typename traits_type::result_type, bool>)
               {
                  return model::invoke( m_handler, message, traits::priority::tag< 1>{});
               }
               else
               {
                  // if not, we return true.
                  model::invoke( m_handler, message, traits::priority::tag< 1>{});
                  return true;
               }
            }

         private:

            template< typename M>
            static auto invoke( handler_type& handler, M& message, traits::priority::tag< 1>) -> decltype( handler( std::move( message)))
            {
               return handler( std::move( message));
            }

            template< typename M>
            static auto invoke( handler_type& handler, M& message, traits::priority::tag< 0>) -> decltype( handler( message))
            {
               return handler( message);
            }

            handler_type m_handler;
         };


         using handlers_type = std::map< message::Type, concept_t>;


         template< typename H>
         static void add( handlers_type& result, H&& handler)
         {
            using handle_type = model< typename std::decay< H>::type>;
            result[ handle_type::message_type::type()] = handle_type{ std::forward< H>( handler)};
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

      namespace condition
      {
         namespace detail
         {
            namespace handle
            {
               //! If the "error" is an interrupt, signals are taken care of
               //! and no error return
               //! If it's not an interrupt, the error is returned.
               std::optional< std::system_error> error();
            } // handle

            namespace tag
            {
               struct prelude{};
               struct idle{};
               struct done{};
               struct error{};

               template< typename Tag>
               struct default_invoke
               {
                  template< typename... Ts>
                  static constexpr void invoke( Ts&&... ts) {};
               };

               template<>
               struct default_invoke< tag::done>
               {
                  static constexpr bool invoke() { return false;};
               };

               template<>
               struct default_invoke< tag::error>
               {
                  static void invoke( const std::system_error& error) 
                  { 
                     throw error;
                  };
               };
            } // tag


            template< typename Tag, typename C>
            struct tagged
            {
               tagged( C callable) : callable{ std::move( callable)} {}

               template< typename... Ts>
               auto operator() ( Tag tag, Ts&&... ts) { return callable( std::forward< Ts>( ts)...);}

               C callable;
            };

            namespace dispatch
            {
               // uses the user provided 'condition'
               template< typename Tag, typename C, typename... Ts> 
               auto invoke( C& condition, traits::priority::tag< 1>, Ts&&... ts) -> decltype( condition( Tag{}, std::forward< Ts>( ts)...))
               {
                  return condition( Tag{}, std::forward< Ts>( ts)...);
               }

               // default behaviour, if not provided
               template< typename Tag, typename C, typename... Ts> 
               auto invoke( C& condition, traits::priority::tag< 0>, Ts&&... ts) -> decltype( tag::default_invoke< Tag>::invoke( std::forward< Ts>( ts)...))
               {
                  return tag::default_invoke< Tag>::invoke( std::forward< Ts>( ts)...);
               }

            } // dispatch

            template< typename Tag, typename C, typename... Ts> 
            auto invoke( C& condition, Ts&&... ts) -> decltype( dispatch::invoke< Tag>( condition, traits::priority::tag< 1>{}, std::forward< Ts>( ts)...))
            {
               return dispatch::invoke< Tag>( condition, traits::priority::tag< 1>{}, std::forward< Ts>( ts)...);
            }

            namespace pump
            {
               //! used if there are an error condition provided
               template< typename C, typename H, typename D>
               auto dispatch( C&& condition, H&& handler, D& device) 
               {
                  Trace trace{ "common::message::dispatch::detail::pump::dispatch"};

                  detail::invoke< detail::tag::prelude>( condition);

                  while( ! detail::invoke< detail::tag::done>( condition))
                  {
                     try 
                     {   
                        while( handler( device, communication::device::policy::non::blocking( device)))
                           if( detail::invoke< detail::tag::done>( condition))
                              return;

                        // we're idle
                        detail::invoke< detail::tag::idle>( condition);

                        // we might be done after idle
                        if( detail::invoke< detail::tag::done>( condition))
                           return;

                        // we block
                        handler( device, communication::device::policy::blocking( device));
                     }
                     catch( ...)
                     {
                        if( auto error = handle::error())
                           detail::invoke< detail::tag::error>( condition, *error);
                     }
                  } 
               }


               namespace consume
               {
                  template< typename H> 
                  auto relaxed( H& handler)
                  {
                     return [ &handler, types = handler.types()]( auto& device, auto policy)
                     {
                        return handler( device.next( types, policy));
                     };                    
                  }

                  template< typename H> 
                  auto strict( H& handler)
                  {
                     return [ &handler]( auto& device, auto policy)
                     {
                        return handler( device.next( policy));
                     };
                  }
               } // consume
            } // pump

         } // detail

         //! provided `callable` will be invoked before the message pump starts
         template< typename T>
         auto prelude( T callable) { return detail::tagged< detail::tag::prelude, T>{ std::move( callable)};}

         //! provided `callable` will be invoked once every time the _inbound device_ is empty
         template< typename T>
         auto idle( T callable) { return detail::tagged< detail::tag::idle, T>{ std::move( callable)};}

         //! provided `callable` will be invoked and if the result is `true` the pump stops, 
         //! and control is return to caller.
         //! @attention `done` should not alter any state, just answer if the "pump" is done.
         //! @attention `done` shall not consume any messages or otherwise communicate in any form.
         template< typename T>
         auto done( T callable) { return detail::tagged< detail::tag::done, T>{ std::move( callable)};}

         //! provided `callable` will be invoked if an exception is thrown, the `callable` needs to rethrow the
         //! ongoing exception to do a _dispatch_
         template< typename T>
         auto error( T callable) { return detail::tagged< detail::tag::error, T>{ std::move( callable)};}


         template< typename... Ts> 
         auto compose( Ts&&... ts) 
         {
            return casual::overloaded{ std::forward< Ts>( ts)...};
         };

      } // condition

      //! Creates a corresponding message-dispatch-handler to this
      //! inbound device
      template< typename D, typename... Args>
      auto handler( D&& device, Args&&... args)
      {
         using handler_type = basic_handler< typename std::decay_t< D>::complete_type>;
         return handler_type{ std::forward< Args>( args)...};
      }

      //! conditional pump. 
      //! takes a composed condition via condition::compose( condition::(prelude|idle|done|error))
      //! the provided 'callbacks' will be used during _message pump_
      template< typename C, typename H, typename D>
      void pump( C&& condition, H&& handler, D& device)
      {
         auto consume = condition::detail::pump::consume::strict( handler);
         condition::detail::pump::dispatch( std::forward< C>( condition), consume, device);
      }

      namespace relaxed
      {
         //! only consume messages that handler can handle, and leaves the rest for later consumption.
         //! otherwise same as dispatch::pump
         template< typename C, typename H, typename D>
         void pump( C&& condition, H&& handler, D& device)
         {
            auto consume = condition::detail::pump::consume::relaxed( handler);
            condition::detail::pump::dispatch( std::forward< C>( condition), consume, device);
         }
      } // relaxed

      //! uses the default behaviour 
      template< typename H, typename D>
      void pump( H&& handler, D& device)
      {
         pump( condition::compose(), handler, device);
      }

   } // common::message::dispatch
} // casual



