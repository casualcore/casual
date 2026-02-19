//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#if defined(CASUAL_PLATFORM_LINUX)

#include "common/strong/id.h"
#include "common/functional.h"
#include "common/algorithm.h"
#include "common/message/dispatch.h"

#include <vector>
#include <span>
#include <string_view>

#include <sys/epoll.h>

std::ostream& operator << ( std::ostream& out, const ::epoll_event& event);

namespace casual
{
   namespace common::communication::select
   {
      namespace directive
      {
         namespace ready
         {
            inline strong::file::descriptor::id descriptor( const ::epoll_event& event) noexcept
            {
               return strong::file::descriptor::id{ event.data.fd};
            }
            
         } // ready
         
         //! view type of the read and write events
         struct Ready
         {
            std::span< ::epoll_event> read;
            std::span< ::epoll_event> write;

            inline explicit operator bool() const noexcept { return ! read.empty() || ! write.empty();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( read);
               CASUAL_SERIALIZE( write);
            )
         };

         namespace detail
         {

            enum struct Flags : std::uint32_t
            {
               none = 0,
               in = EPOLLIN,
               out = EPOLLOUT,
               rd_hup = EPOLLRDHUP, // read half closed
               prio = EPOLLPRI, // high priority data available
               error = EPOLLERR, // error condition
               hup = EPOLLHUP, // hang up
               edge = EPOLLET, // edge triggered
               oneshot = EPOLLONESHOT, // only report one occurrence
               wakeup = EPOLLWAKEUP, // wake up the process when the event is ready
               exclusive = EPOLLEXCLUSIVE // exclusive wakeup for this event
            };

            [[maybe_unused]] consteval void casual_enum_as_flag( Flags){};

            std::string_view description( Flags flag);

            struct Entry
            {
               strong::file::descriptor::id descriptor;
               Flags event{};
               
               inline friend bool operator == ( const Entry& lhs, strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( descriptor);
                  CASUAL_SERIALIZE( event);
               )
            };
            
         } // detail

         //! @returns the descriptor from the event
         strong::file::descriptor::id descriptor( const ::epoll_event& event) noexcept;

      } // directive


      struct Directive 
      {
         Directive();
         ~Directive();

         Directive( Directive&&) noexcept;
         Directive& operator = ( Directive&&) noexcept;
         void read_add( strong::file::descriptor::id descriptor);
         void read_remove( strong::file::descriptor::id descriptor);


         template< concepts::range_value_convertible_to< strong::file::descriptor::id> R>
         void read_remove( R&& descriptors)
         { 
            for( auto descriptor : descriptors)
               read_remove( descriptor);
         }

         void write_add( strong::file::descriptor::id descriptor);
         void write_remove( strong::file::descriptor::id descriptor);

         template< concepts::range_value_convertible_to< strong::file::descriptor::id> R>
         void write_remove( R&& descriptors)
         {
            for( auto descriptor : descriptors)
               write_remove( descriptor);
         }

         //! removes `descriptor` from _read_ and _write_
         template< typename R>
         void remove( R&& descriptors) 
         { 
            read_remove( descriptors);
            write_remove( descriptors);
         }

         inline auto descriptor() const { return m_epoll;}
         inline auto& events() { return m_events; }

         const auto& entries() const { return m_entries;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_epoll);
            CASUAL_SERIALIZE( m_entries);
         )


      private:

         void add( strong::file::descriptor::id descriptor, directive::detail::Flags flag);
         void remove( strong::file::descriptor::id descriptor, directive::detail::Flags flag);

         strong::file::descriptor::id m_epoll;
         std::vector< directive::detail::Entry> m_entries;

         //! this is more like a "buffer" for the events
         std::vector< ::epoll_event> m_events;
      };


      namespace dispatch::detail
      {
         // only exposed for unittests.
         directive::Ready ready( std::span< const directive::detail::Entry> entries, std::span< ::epoll_event> events);

         directive::Ready select( Directive& directive);

      } // dispatch::detail

   } // common::communication::select
} // casual

#endif // CASUAL_PLATFORM_LINUX
