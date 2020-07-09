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
#include "common/communication/device.h"
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
               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( m_handlers, "handlers");
               })

            private:

               bool dispatch( communication::message::Complete& complete) const
               {
                  if( ! complete)
                     return false;

                  if( auto found = algorithm::find( m_handlers, complete.type))
                  {
                     found->second->dispatch( complete);
                     return true;
                  }

                  log::line( log::category::error, "message_type: ", complete.type, " not recognized - action: discard");
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
                        || std::is_same< typename traits_type::result_type, bool>::value, 
                        "handlers has to have this signature: void|bool( <some message>), can be declared const");

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

            namespace condition
            {
               namespace detail
               {
                  namespace tag
                  {
                     struct prelude{};
                     struct idle{};
                     struct done{};
                     struct error{};

                     template< typename Tag>
                     struct default_invoke
                     {
                        static constexpr void invoke() {};
                     };

                     template<>
                     struct default_invoke< tag::done>
                     {
                        static constexpr bool invoke() { return false;};
                     };

                     template<>
                     struct default_invoke< tag::error>
                     {
                        static void invoke() { throw;};
                     };
                  } // tag


                  template< typename T, typename Tag>
                  struct strong_t
                  {
                     strong_t( T value) : callable{ std::move( value)} {}

                     auto operator() ( Tag tag) { return callable();}

                     T callable;
                  };
                  
                  template< typename Tag, typename T>
                  auto make_strong( T&& callable) { return strong_t< std::decay_t< T>, Tag>{ std::forward< T>( callable)};}

                  // TODO c++17 - we can use the _overloaded_ idiom
                  template< typename... Ts>
                  struct composition_t{};

                  template< typename T, typename... Ts>
                  struct composition_t< T, Ts...> : T, composition_t< Ts...>
                  {
                     using base_type = composition_t< Ts...>;
                     composition_t( T head, Ts... tail) : T{ head}, base_type{ tail...} {}

                     using T::operator();
                     using base_type::operator();
                  };

                  template< typename T>
                  struct composition_t< T> : T
                  {
                     composition_t( T head) : T{ head} {}
                     using T::operator();
                  };

                  namespace dispatch
                  {
                     // default behaviour, if not provided
                     template< typename Tag, typename T> 
                     auto invoke( T& condition, traits::priority::tag< 0>) -> decltype( tag::default_invoke< Tag>::invoke())
                     {
                        return tag::default_invoke< Tag>::invoke();
                     }

                     // uses the user provided 'condition'
                     template< typename Tag, typename T> 
                     auto invoke( T& condition, traits::priority::tag< 1>) -> decltype( condition( Tag{}))
                     {
                        return condition( Tag{});
                     }
                  } // dispatch


                  template< typename Tag, typename T> 
                  auto invoke( T& condition) -> decltype( dispatch::invoke< Tag>( condition, traits::priority::tag< 1>{}))
                  {
                     return dispatch::invoke< Tag>( condition, traits::priority::tag< 1>{});
                  }

                  namespace pump
                  {

                     //! used if there are no error condition provided
                     template< typename C, typename H, typename D>
                     void dispatch( C&& condition, H&& handler, D& device, traits::priority::tag< 0>)
                     {
                        detail::invoke< detail::tag::prelude>( condition);

                        while( ! detail::invoke< detail::tag::done>( condition))
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
                     }   

                     //! used if there are an error condition provided
                     template< typename C, typename H, typename D>
                     auto dispatch( C&& condition, H&& handler, D& device, traits::priority::tag< 1>) 
                        -> decltype( condition.invoke( tag::error{}))
                     {
                        using device_type = std::decay_t< decltype( device)>;

                        detail::invoke< detail::tag::prelude>( condition);

                        while( ! detail::invoke< detail::tag::done>( condition))
                        {
                           try 
                           {   
                              while( handler( device, typename device_type::non_blocking_policy{}))
                                 if( detail::invoke< detail::tag::done>( condition))
                                    return;

                              // we're idle
                              detail::invoke< detail::tag::idle>( condition);

                              // we might be done after idle
                              if( detail::invoke< detail::tag::done>( condition))
                                 return;

                              // we block
                              handler( device, typename device_type::blocking_policy{});
                           }
                           catch( ...)
                           {
                              detail::invoke< detail::tag::error>( condition);
                           }
                        } 
                     }
                     namespace consume
                     {
                        template< typename H> 
                        auto relaxed( H& handler)
                        {
                           return [&handler, types = handler.types()]( auto& device, auto policy)
                           {
                              return handler( device.next( types, policy));
                           };
                        }

                        template< typename H> 
                        auto strict( H& handler)
                        {
                           return [&handler]( auto& device, auto policy)
                           {
                              return handler( device.next( policy));
                           };
                        }
                     } // consume
                  } // pump

               } // detail

               //! provided `callable` will be invoked before the message pump starts
               template< typename T>
               auto prelude( T&& callable) { return detail::make_strong< detail::tag::prelude>( std::forward< T>( callable));}

               //! provided `callable` will be invoked once every time the _inbound device_ is empty
               template< typename T>
               auto idle( T&& callable) { return detail::make_strong< detail::tag::idle>( std::forward< T>( callable));}

               //! provided `callable` will be invoked and if the result is `true` the control is return to caller
               template< typename T>
               auto done( T&& callable) { return detail::make_strong< detail::tag::done>( std::forward< T>( callable));}

               //! provided `callable` will be invoked if an exception is thrown, the `callable` needs to rethrow the
               //! ongoing exception to do a _dispatch_
               template< typename T>
               auto error( T&& callable) { return detail::make_strong< detail::tag::error>( std::forward< T>( callable));}


               template< typename... Ts> 
               auto compose( Ts&&... ts) 
               {
                  return detail::composition_t< Ts...>{ std::forward< Ts>( ts)...};
               };

            } // condition

            //! Creates a corresponding message-dispatch-handler to this
            //! inbound device
            template< typename D, typename... Args>
            static auto handler( D&& device, Args&&... args)
            {
               using handler_type = basic_handler< typename std::decay_t< D>::deserialize_type>;
               return handler_type{ std::forward< Args>( args)...};
            }

            //! conditional pump. 
            //! takes a composed condition via condition::compose( condition::(prelude|idle|done|error))
            //! the provided 'callbacks' will be used during _message pump_
            template< typename C, typename H, typename D>
            void pump( C&& condition, H&& handler, D& device)
            {
               auto consume = condition::detail::pump::consume::strict( handler);
               condition::detail::pump::dispatch( std::forward< C>( condition), consume, device, traits::priority::tag< 1>{});
            }

            namespace relaxed
            {
               //! only consume messages that handler can handle, and leaves the rest for later consumption.
               //! otherwise same as dispatch::pump
               template< typename C, typename H, typename D>
               void pump( C&& condition, H&& handler, D& device)
               {
                  auto consume = condition::detail::pump::consume::relaxed( handler);
                  condition::detail::pump::dispatch( std::forward< C>( condition), consume, device, traits::priority::tag< 1>{});
               }
            } // relaxed

            //! uses the default behaviour 
            template< typename H, typename D>
            void pump( H&& handler, D& device)
            {
               pump( condition::compose(), handler, device);
            }

         } // dispatch
      } // message
   } // common
} // casual



