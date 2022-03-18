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
 
         using range_type = range::type_t< std::vector< strong::file::descriptor::id>>;
         
         struct Set
         {
            void add( strong::file::descriptor::id descriptor) noexcept;
            void remove( strong::file::descriptor::id descriptor) noexcept;

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

            inline auto descriptors() const noexcept
            { 
               return range::make( m_descriptors);
            }

            //! @returns the highest value descriptor in the `Set`, 'nil' descriptor if empty.
            inline auto highest() const noexcept { return m_highest;}
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_descriptors, "descriptors");
            )

         private:
            // mutable so we can 'filter' for ready - semantically we don't change the state
            mutable std::vector< strong::file::descriptor::id> m_descriptors;
            strong::file::descriptor::id m_highest{};
         };

         struct Ready
         {
            Ready( range_type read_ready, range_type write_ready)
               : m_descriptors( read_ready.size() + write_ready.size()), 
                  read{ range::make( std::begin( m_descriptors), read_ready.size())},
                  write{ range::make( std::end( read), write_ready.size())} 
            {
               algorithm::copy( read_ready, std::begin( read));
               algorithm::copy( write_ready, std::begin( write));
            }

         private:
            std::vector< strong::file::descriptor::id> m_descriptors;

         public:
            range_type read;
            range_type write;

            inline explicit operator bool() const noexcept { return read || write;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( read);
               CASUAL_SERIALIZE( write);
            )

         };

      } // directive


      struct Directive 
      {
         directive::Set read;
         directive::Set write;

         //! @returns the highest value descriptor in the `Directive`, 'nil' descriptor if empty.
         inline auto highest() const noexcept { return std::max( read.highest(), write.highest());}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( read);
            CASUAL_SERIALIZE( write);
         )
      };

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
            directive::Ready select( const Directive& directive);

            namespace consume
            {
               template< typename H>
               auto dispatch( H& handler, traits::priority::tag< 1>) 
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

               //! sentinel
               constexpr bool dispatch() { return false;}

               template< typename H, typename... Hs> 
               bool dispatch( H& handler, Hs&... handlers)
               {
                  return dispatch( handler, traits::priority::tag< 1>{}) || dispatch( handlers...);
               }
            } // consume
         
            namespace handle
            {
               //! blocking read handler
               template< typename H> 
               auto dispatch( directive::Ready& ready, H& handler, traits::priority::tag< 2>)
                  -> decltype( void( handler( range::front( ready.read), tag::read{})))
               {
                  // keep the reads that did not find a handler.
                  ready.read = algorithm::filter( ready.read, [&handler]( auto descriptor)
                  {
                     return ! handler( descriptor, tag::read{});
                  });
               }

               //! blocking write handler
               template< typename H> 
               auto dispatch( directive::Ready& ready, H& handler, traits::priority::tag< 1>)
                  -> decltype( void( handler( range::front( ready.write), tag::write{})))
               {
                  // keep the writes that did not find a handler.
                  ready.write = algorithm::filter( ready.write, [&handler]( auto descriptor)
                  {
                     return ! handler( descriptor, tag::write{});
                  });
               }

               //! 'consume' handler - no op
               template< typename H> 
               auto dispatch( directive::Ready& ready, H& handler, traits::priority::tag< 0>) 
                  -> decltype( void( handler( tag::consume{})))
               { 
                  // no-op.
               }

               template< typename H> 
               void dispatch( directive::Ready& ready, H& handler)
               {
                  handle::dispatch( ready, handler, traits::priority::tag< 2>{});
               }
            } // handle

            // "sentinel"
            template< typename F, typename R> 
            void iterate( F&& functor, R& ready) {}

            template< typename F, typename H, typename... Hs> 
            void iterate( F&& functor, directive::Ready& ready, H& handler, Hs&... handlers)
            {
               if( ! ready)
                  return;
               
               functor( ready, handler);
               iterate( functor, ready, handlers...);
            }

            template< typename... Hs> 
            void dispatch( directive::Ready ready, Hs&... handlers)
            {
               // 'iterate' over all handlers and dispatch on ready...
               // this might be done with fold expressions...
               iterate(
                  []( auto& ready, auto& handler)
                  {
                     handle::dispatch( ready, handler);
                  },
                  ready,
                  handlers...);
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
                        // make sure we try to consume from the 'consume-handlers' before
                        // we might block forever. This gives possibilities to consume cached messages
                        // that wont be triggered via multiplexing on file descriptors
                        while( detail::consume::dispatch( handlers...))
                           if( condition::detail::invoke< condition::detail::tag::done>( condition))
                              return;

                        // we're idle
                        condition::detail::invoke< condition::detail::tag::idle>( condition);

                        // we might be done after idle
                        if( condition::detail::invoke< condition::detail::tag::done>( condition))
                           return;

                        // we block in `detail::select` with the read and write sets.
                        detail::dispatch( 
                           detail::select( directive),
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

         //! block until descriptor is ready for write.
         void write( strong::file::descriptor::id descriptor);

      } // block
   } // common::communication::select
} // casual