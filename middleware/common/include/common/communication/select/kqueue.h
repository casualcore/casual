//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#if defined(CASUAL_PLATFORM_BSD)

#include "common/strong/id.h"
#include "common/functional.h"
#include "common/algorithm.h"
#include "common/message/dispatch.h"

#include <vector>

#include	<sys/event.h>


namespace casual
{
   namespace common::communication::select
   {
      namespace directive
      {

         using kevent_t = struct ::kevent;


         namespace ready
         {
            inline strong::file::descriptor::id descriptor( const kevent_t& event) noexcept
            {
               return strong::file::descriptor::id{ event.ident};
            }
            
         } // ready
         
         //! view type of the read and write events
         struct Ready
         {
            std::span< kevent_t> read;
            std::span< kevent_t> write;

            inline explicit operator bool() const noexcept { return ! read.empty() || ! write.empty();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( read);
               CASUAL_SERIALIZE( write);
            )
         };

         namespace detail
         {

            enum struct Filter : int16_t
            {
               none = 0,                     // no filter
               read = EVFILT_READ,            // data available for reading
               write = EVFILT_WRITE,          // data can be written without blocking
               vnode = EVFILT_VNODE,          // file system event
               signal = EVFILT_SIGNAL,        // signal event
               timer = EVFILT_TIMER,          // timer event
               user = EVFILT_USER,            // user defined event
               proc = EVFILT_PROC,            // process event
            };

            std::string_view description( Filter filter);

            struct Entry
            {
               strong::file::descriptor::id descriptor;
               Filter filter{};
               
               inline friend bool operator == ( const Entry& lhs, strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;}
               inline friend bool operator == ( const Entry& lhs, const Entry& rhs) = default;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( descriptor);
                  CASUAL_SERIALIZE( filter);
               )
            };
            
         } // detail

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

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_kqueue);
            CASUAL_SERIALIZE( m_entries);
         )

         inline auto descriptor() const { return m_kqueue;}
         inline auto& events() { return m_events; }

      private:

         void add( strong::file::descriptor::id descriptor, directive::detail::Filter filter);
         void remove( strong::file::descriptor::id descriptor, directive::detail::Filter filter);

         strong::file::descriptor::id m_kqueue;
         std::vector< directive::detail::Entry> m_entries;
         //! this is more like a "buffer" for the events
         std::vector< directive::kevent_t> m_events;
      };


      namespace dispatch::detail
      {
         directive::Ready select( Directive& directive);

      } // dispatch::detail

   } // common::communication::select
} // casual


#endif // CASUAL_PLATFORM_BSD
