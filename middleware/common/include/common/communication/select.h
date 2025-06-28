//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/id.h"
#include "common/functional.h"
#include "common/algorithm.h"
#include "common/message/dispatch.h"


#include <vector>

#if defined(CASUAL_PLATFORM_BSD)
#include "common/communication/select/kqueue.h"
#elif defined(CASUAL_PLATFORM_LINUX)
#include "common/communication/select/epoll.h"
#else
#error "Unsupported platform"
#endif

namespace casual
{
   namespace common::communication::select
   {
      namespace tag
      {
         struct read{};
         struct write{};
         struct consume{};
      } // tag

      namespace dispatch
      {
         namespace condition
         {
            using namespace common::message::dispatch::condition;
         } // condition

         namespace detail
         {
            namespace consume
            {
               template< typename H>
               constexpr auto dispatch( H& handler, traits::priority::tag< 1>) 
                  -> decltype( handler( tag::consume{}))
               {
                  return handler( tag::consume{});
               }
               
               //! no-op for all others...
               template< typename H> 
               constexpr auto dispatch( H& handler, traits::priority::tag< 0>)
               {
                  return false;
               }

               template< typename... Hs> 
               constexpr bool dispatch( Hs&... handlers)
               {
                  return ( dispatch( handlers, traits::priority::tag< 1>{}) || ... );
               }
            } // consume
         
            namespace handle
            {
               namespace read
               {
                  //! blocking read handler
                  template< typename H> 
                  auto dispatch( directive::Ready& ready, H& handler, traits::priority::tag< 1>)
                     -> decltype( predicate::boolean( handler( strong::file::descriptor::id{}, tag::read{})))
                  {
                     // keep the reads that did not find a handler.
                     ready.read = algorithm::filter( ready.read, [ &handler]( const auto& event)
                     {
                        return ! handler( directive::ready::descriptor( event), tag::read{});
                     });
                     return predicate::boolean( ready);
                  }

                  //! "non read" handler - no op
                  template< typename H> 
                  constexpr auto dispatch( directive::Ready& ready, H& handler, traits::priority::tag< 0>) noexcept { return true;};
               } // read

               namespace write
               {
                  //! blocking write handler
                  template< typename H> 
                  auto dispatch( directive::Ready& ready, H& handler, traits::priority::tag< 1>)
                     -> decltype( predicate::boolean( handler( strong::file::descriptor::id{}, tag::write{})))
                  {
                     // keep the writes that did not find a handler.
                     ready.write = algorithm::filter( ready.write, [ &handler]( const auto& event)
                     {
                        return ! handler( directive::ready::descriptor( event), tag::write{});
                     });
                     return predicate::boolean( ready);
                  }

                  //! "non write" handler - no op
                  template< typename H> 
                  constexpr auto dispatch( directive::Ready& ready, H& handler, traits::priority::tag< 0>) noexcept { return true;};
               } // write

               template< typename H> 
               auto read_write( directive::Ready& ready, H& handler)
               {
                  return read::dispatch( ready, handler, traits::priority::tag< 1>{}) 
                     && write::dispatch( ready, handler, traits::priority::tag< 1>{});
               }

               template< typename... Hs> 
               void dispatch( directive::Ready ready, Hs&... handlers)
               {
                  // Left-fold -  will short circuit when `ready` is _consumed_.
                  (  ... && read_write( ready, handlers) );
               }

            } // handle

            namespace pump
            {
               template< typename C, typename... Ts>
               auto dispatch( C&& condition, Directive& directive, Ts&&... handlers) 
               {
                  condition::detail::invoke< condition::detail::tag::prelude>( condition);

                  while( true)
                  {
                     try 
                     {
                        // we're idle (most likely)
                        condition::detail::invoke< condition::detail::tag::idle>( condition);

                        // make sure we try to consume from the 'consume-handlers' before
                        // we might block forever. This gives possibilities to consume cached messages
                        // that wont be triggered via multiplexing on file descriptors
                        while( detail::consume::dispatch( handlers...))
                           ; // no-op

                        // we might be done after idle, and consumed
                        if( condition::detail::invoke< condition::detail::tag::done>( condition))
                           return;

                        // we block in `detail::select` with the read and write sets.
                        handle::dispatch( 
                           detail::select( directive),
                           handlers...);
                     }
                     catch( ...)
                     {
                        if( auto error = condition::detail::handle::error())
                           condition::detail::invoke< condition::detail::tag::error>( condition, *error);
                     }
                  } 
               }
            } // pump

         } // detail


         //! handlers has to comply with (any of) the following:
         //! * tag::read
         //!   * `bool <callable>( descriptor, tag::read)`
         //!      * return true if descriptor is one that <callable> handled - can be blocking
         //! * tag::write
         //!   * `bool <callable>( descriptor, tag::write)`
         //!      * return true if descriptor is one that <callable> handled - must be non blocking
         //! * tag::consume
         //!   * `bool <callable>( tag::consume)`
         //!      * return true a message is consumed from _cache_ - do not use this for _read_ or _write_! 
         //! @{
         template< typename C, typename... Ts>  
         void pump( C&& condition, Directive& directive, Ts&&... handlers)
         {
            detail::pump::dispatch( std::forward< C>( condition), directive, std::forward< Ts>( handlers)...);
         }

         template< typename... Ts>  
         void pump( Directive& directive, Ts&&... handlers)
         {
            detail::pump::dispatch( condition::compose(), directive, std::forward< Ts>( handlers)...);
         }
         //! @}

      } // dispatch

      namespace block
      {
         //! block until descriptor is ready for read.
         void read( strong::file::descriptor::id descriptor);

         //! block until descriptor is ready for write.
         void write( strong::file::descriptor::id descriptor);

      } // block
   } // common::communication::select
} // casual