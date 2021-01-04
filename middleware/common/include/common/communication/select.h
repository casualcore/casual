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
   namespace common::communication::select
   {
      namespace directive
      {
         namespace native
         {
            struct Set
            {
               Set();

               void add( strong::file::descriptor::id descriptor) noexcept;
               void remove( strong::file::descriptor::id descriptor) noexcept;
               bool ready( strong::file::descriptor::id descriptor) const noexcept;
               
               inline ::fd_set* native() noexcept { return &m_set;}

            private:
               ::fd_set m_set;
            };
         } // native
         
         struct Set
         {
            void add( strong::file::descriptor::id descriptor) noexcept;
            void remove( strong::file::descriptor::id descriptor) noexcept;
            bool ready( strong::file::descriptor::id descriptor) const noexcept;

            //! @returns a range of descriptors that are found 'ready' in the set
            inline auto ready( const native::Set& set) const noexcept
            {
               return algorithm::filter( m_descriptors, [&set]( auto descriptor){ return set.ready( descriptor);});
            }
            
            template< typename Range>
            auto add( Range&& descriptors) noexcept
               -> decltype( add( range::front( descriptors)))
            {
               algorithm::for_each( descriptors, [&]( auto descriptor){ add( descriptor);});
            }

            template< typename Range>
            auto remove( Range&& descriptors) noexcept
               -> decltype( remove( range::front( descriptors)))
            {
               algorithm::for_each( descriptors, [&]( auto descriptor){ remove( descriptor);});
            }
            
            inline const native::Set& set() const noexcept { return m_set;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_descriptors, "descriptors");
            )

         private:
            // mutable so we can 'filter' for ready - semantically we don't change the state
            mutable std::vector< strong::file::descriptor::id> m_descriptors;
            native::Set m_set;
         };
      } // directive

      struct Directive 
      {
         directive::Set read;
         // directive::Set write; no need for write as of now

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( read);
         )
      };

      namespace dispatch
      {
         namespace condition
         {
            using namespace common::message::dispatch::condition;
         } // condition

         namespace detail
         {
            directive::native::Set select( const directive::native::Set& read);
            
            namespace consume
            {
               template< typename H>
               auto handle( H& handler, traits::priority::tag< 1>) -> decltype( handler())
               {
                  return handler();
               }
               
               //! no-op for 'blocking'
               template< typename H> 
               constexpr auto handle( H& handler, traits::priority::tag< 0>)
                  -> decltype( handler( strong::file::descriptor::id{}))
               { 
                  return false;
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
               //! blocking handler
               template< typename R, typename H> 
               auto read( R&& ready, H& handler, traits::priority::tag< 1>)
                  -> decltype( handler( range::front( ready)))
               {
                  return algorithm::any_of( ready, std::ref( handler));
               }

               //! no-op for 'consume'
               template< typename R, typename H> 
               auto read( R&& ready, H& handler, traits::priority::tag< 0>) 
                  -> decltype( handler())
               { 
                  return false;
               }

               template< typename R, typename H> 
               auto dispatch( R&& ready, H& handler)
                  -> decltype( handle::read( ready, handler, traits::priority::tag< 1>{}))
               {
                  return handle::read( ready, handler, traits::priority::tag< 1>{});
               }
            } // handle



            // "sentinel"
            template< typename F> 
            void iterate( F&& functor) {}

            template< typename F, typename H, typename... Hs> 
            void iterate( F&& functor, H& handler, Hs&... handlers)
            {
               functor( handler);
               iterate( functor, handlers...);
            }

            template< typename R, typename... Hs> 
            void dispatch( R&& ready, Hs&... handlers)
            {
               // take care of blocking, if any
               iterate( 
                  [ready = std::forward< R>( ready)]( auto& handler)
                  {
                     handle::dispatch( ready, handler);
                  }, 
                  handlers...);

               // take care of consume, if any.
               consume::dispatch( handlers...);
            }



            namespace handle
            {
               void error();
            } // handle

            namespace pump
            {
               template< typename C, typename... Ts>
               auto dispatch( C&& condition, const Directive& directive, Ts&&... handlers) 
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

                        // we block in `detail::select` with the read set.
                        detail::dispatch( 
                           directive.read.ready( detail::select( directive.read.set())), 
                           handlers...);
                     }
                     catch( ...)
                     {
                        condition::detail::invoke< condition::detail::tag::error>( condition);
                     }
                  } 
               }
            } // pump

         } // detail


         //! handlers has to comply with the following
         //! * either:
         //!   * `bool <callable>( descriptor)`
         //!      *  return true if descriptor is one that <callable> handled - can be blocking
         //!   * `bool <callable>()`
         //!      *  return true if descriptor is one that <callable> handled - can be NON blocking
         //! @{
         template< typename C, typename... Ts>  
         void pump( C&& condition, const Directive& directive, Ts&&... handlers)
         {
            detail::pump::dispatch( std::forward< C>( condition), directive, std::forward< Ts>( handlers)...);
         }

         template< typename... Ts>  
         void pump( const Directive& directive, Ts&&... handlers)
         {
            detail::pump::dispatch( condition::compose(), directive, std::forward< Ts>( handlers)...);
         }
         //! @}

      } // dispatch

      namespace block
      {
         //! block until descriptor is ready for read.
         void read( strong::file::descriptor::id descriptor);

      } // block
   } // common::communication::select
} // casual