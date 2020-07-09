//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/id.h"
#include "common/functional.h"
#include "common/algorithm.h"
#include "common/communication/log.h"
#include "common/message/dispatch.h"


#include <vector>

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace select
         {
            namespace directive
            {
               struct Set
               {
                  Set();

                  void add( strong::file::descriptor::id descriptor);
                  void remove( strong::file::descriptor::id descriptor);
                  
                  inline ::fd_set* native() { return &m_set;}

                  bool ready( strong::file::descriptor::id descriptor) const;

                  friend std::ostream& operator << ( std::ostream& out, const Set& value);

               private:
                  ::fd_set m_set;
               };
            } // directive
            struct Directive 
            {
               directive::Set read;
               // directive::Set write; no need for write as of now

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( read);
               })
            };

            namespace dispatch
            {
               namespace condition
               {
                  using namespace common::message::dispatch::condition;
               } // condition

               namespace detail
               {
                  template< typename C> 
                  struct basic_reader 
                  {
                     basic_reader( strong::file::descriptor::id descriptor, C&& callback)
                        : m_descriptor( std::move( descriptor)), m_callback( std::move( callback)) {}

                     inline void read( strong::file::descriptor::id descriptor) { m_callback( descriptor);}
                     inline strong::file::descriptor::id descriptor() const { return m_descriptor;}

                  private:
                     strong::file::descriptor::id m_descriptor;
                     C m_callback;
                  };

                  Directive select( const Directive& directive);


                  namespace descriptor
                  {
                     template< typename H, typename F> 
                     auto dispatch( H& handler, F&& functor, traits::priority::tag< 0>) -> decltype( void( handler.descriptor()))
                     {
                        functor( handler, handler.descriptor());
                     }
                     
                     template< typename H, typename F> 
                     auto dispatch( H& handler, F&& functor, traits::priority::tag< 1>) -> decltype( void( handler.descriptors()))
                     {
                        for( auto descriptor : handler.descriptors())
                           functor( handler, descriptor);
                     }
                  } // descriptor
                  

                  namespace handle
                  {
                     template< typename H> 
                     void read( H& handler, strong::file::descriptor::id descriptor, traits::priority::tag< 0>) {}
                     
                     template< typename H> 
                     auto read( H& handler, strong::file::descriptor::id descriptor, traits::priority::tag< 1>)
                        -> decltype( handler.read( descriptor), void()) 
                     {
                        handler.read( descriptor);
                     }


                     template< typename H>
                     void dispatch( const Directive& directive, H& handler)
                     {
                        descriptor::dispatch( handler, [&directive]( auto& handler, auto descriptor)
                        {
                           if( directive.read.ready( descriptor))
                              handle::read( handler, descriptor, traits::priority::tag< 1>{});

                        }, traits::priority::tag< 1>{});
                     }
                  } // handle



                  // "sentinel"
                  template< typename F> 
                  void iterate( F&& functor) {}

                  template< typename F, typename H, typename... Hs> 
                  void iterate( F&& functor, H& handler, Hs&... handlers)
                  {
                     functor( handler);
                     iterate( std::forward< F>( functor), handlers...);
                  }


                  template< typename... Hs> 
                  void dispatch( const Directive& directive, Hs&... handlers)
                  {
                     auto invoke = [&directive]( auto& handler){
                        handle::dispatch( directive, handler);
                     };
                     iterate( invoke, handlers...);
                  }

                  namespace consume
                  {
                     template< typename H> 
                     auto handle( H& handler, traits::priority::tag< 0>) { return false;}

                     template< typename H>
                     auto handle( H& handler, traits::priority::tag< 1>) -> decltype( handler.consume())
                     {
                        return handler.consume();
                     }
                     
                     //! sentinel
                     constexpr bool dispatch() { return false;}

                     template< typename H, typename... Hs> 
                     bool dispatch( H& handler, Hs&... handlers)
                     {
                        return handle( handler, traits::priority::tag< 1>{}) || dispatch( handlers...);
                     }
                  } // consume

                  namespace handle
                  {
                     void error();
                  } // handle

                  namespace pump
                  {
                     //! used if there are no error condition provided
                     template< typename C, typename... Ts>
                     void dispatch( C&& condition, const Directive& directive, traits::priority::tag< 0>, Ts&&... handlers)
                     {
                        condition::detail::invoke< condition::detail::tag::prelude>( condition);

                        while( ! condition::detail::invoke< condition::detail::tag::done>( condition))
                        {
                           // make sure we try to consume from the handlers before
                           // we might block forever. Handlers could have cached messages
                           // that wont be triggered via multiplexing on file descriptors
                           while( detail::consume::dispatch( handlers...))
                              if( condition::detail::invoke< condition::detail::tag::done>( condition))
                                 return;

                           // we're idle
                           condition::detail::invoke< condition::detail::tag::idle>( condition);

                           // we might be done after idle
                           if( condition::detail::invoke< condition::detail::tag::done>( condition))
                              return;

                           // we block
                           auto result = detail::select( directive);
                              detail::dispatch( result, handlers...);
                        }
                     }   

                     //! used if there are an error condition provided
                     template< typename C, typename... Ts>
                     auto dispatch( C&& condition, const Directive& directive, traits::priority::tag< 1>, Ts&&... handlers) 
                        -> decltype( condition.invoke( condition::detail::tag::error{}))
                     {
                       condition::detail::invoke< condition::detail::tag::prelude>( condition);

                        while( ! condition::detail::invoke< condition::detail::tag::done>( condition))
                        {
                           try 
                           {   
                              // make sure we try to consume from the handlers before
                              // we might block forever. Handlers could have cached messages
                              // that wont be triggered via multiplexing on file descriptors
                              while( detail::consume::dispatch( handlers...))
                                 if( condition::detail::invoke< condition::detail::tag::done>( condition))
                                    return;

                              // we're idle
                              condition::detail::invoke< condition::detail::tag::idle>( condition);

                              // we might be done after idle
                              if( condition::detail::invoke< condition::detail::tag::done>( condition))
                                 return;

                              // we block
                              auto result = detail::select( directive);
                                 detail::dispatch( result, handlers...);
                           }
                           catch( ...)
                           {
                              condition::detail::invoke< condition::detail::tag::error>( condition);
                           }
                        } 
                     }
                  } // pump

               } // detail

               namespace create
               {
                  template< typename C>
                  auto reader( strong::file::descriptor::id descriptor, C&& callback)
                  {
                     return detail::basic_reader< std::decay_t< C>>{ descriptor, std::forward< C>( callback)};
                  }
               } // create



               template< typename C, typename... Ts>  
               void pump( C&& condition, const Directive& directive, Ts&&... handlers)
               {
                  detail::pump::dispatch( std::forward< C>( condition), directive, traits::priority::tag< 1>{}, std::forward< Ts>( handlers)...);
               }

               template< typename... Ts>  
               void pump( const Directive& directive, Ts&&... handlers)
               {
                  detail::pump::dispatch( condition::compose(), directive, traits::priority::tag< 1>{}, std::forward< Ts>( handlers)...);
               }

            } // dispatch

            namespace block
            {
               //! block until descriptor is ready for read.
               void read( strong::file::descriptor::id descriptor);

            } // block
         } // select
      } // communication
   } // common
} // casual