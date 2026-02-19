
//!
//! Copyright (c) 2026, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/communication/select.h"
#include "common/communication/ipc.h"
#include "common/array.h"

#include "casual/platform.h"

namespace casual
{
   namespace common::communication
   {

      namespace local
      {
         namespace
         {
            auto pending_device()
            {
               ipc::inbound::Device device;
               
               // send something to the device to make ready to read (not too big so it fits in kernel buffer)
               device::blocking::send( device.connector().handle().ipc(), unittest::Message{ 16});
               
               return device;
            }
         } // <unnamed>
      } // local

      TEST( common_communication_select, add_write_descriptor)
      {
         unittest::Trace trace;

         auto device = local::pending_device();

         select::Directive directive;
         directive.write_add( device.descriptor());

         auto ready = select::dispatch::detail::select( directive);

         ASSERT_TRUE( ready.write.size() == 1);
         EXPECT_TRUE( select::directive::descriptor( ready.write[ 0]) == device.descriptor());
         EXPECT_TRUE( ready.read.size() == 0);
      }

      TEST( common_communication_select, add_read_descriptor)
      {
         unittest::Trace trace;

         auto device = local::pending_device();

         select::Directive directive;
         directive.read_add( device.descriptor());

         auto ready = select::dispatch::detail::select( directive);
         EXPECT_TRUE( ready.read.size() == 1);
         EXPECT_TRUE( ready.write.size() == 0);
      }

      TEST( common_communication_select, add_read_write_descriptor)
      {
         unittest::Trace trace;

         auto device = local::pending_device();

         select::Directive directive;
         directive.read_add( device.descriptor());
         directive.write_add( device.descriptor());

         auto ready = select::dispatch::detail::select( directive);
         ASSERT_TRUE( ready.write.size() == 1);
         EXPECT_TRUE( select::directive::descriptor( ready.write[ 0]) == device.descriptor());
         ASSERT_TRUE( ready.read.size() == 1);
         EXPECT_TRUE( select::directive::descriptor( ready.read[ 0]) == device.descriptor());
      }

      TEST( common_communication_select, add_read_write_descriptor__remove_write__expect_only_read)
      {
         unittest::Trace trace;

         auto device = local::pending_device();

         select::Directive directive;
         directive.read_add( device.descriptor());
         directive.write_add( device.descriptor());

         directive.write_remove( device.descriptor());

         auto ready = select::dispatch::detail::select( directive);
         EXPECT_TRUE( ready.write.size() == 0);
         EXPECT_TRUE( ready.read.size() == 1);
      }

      TEST( common_communication_select, add_read_write_descriptor__remove_read__expect_only_write)
      {
         unittest::Trace trace;

         auto device = local::pending_device();

         select::Directive directive;
         directive.read_add( device.descriptor());
         directive.write_add( device.descriptor());

         directive.read_remove( device.descriptor());

         auto ready = select::dispatch::detail::select( directive);
         EXPECT_TRUE( ready.write.size() == 1);
         EXPECT_TRUE( ready.read.size() == 0);
      }

      TEST( common_communication_select, add_read_descriptor__remove_read__expect_no_ready)
      {
         unittest::Trace trace;

         // we need two devices to make sure that we can remove one. epoll needs at least one fd to wait on.
         auto devices = array::make( local::pending_device(), local::pending_device());

         select::Directive directive;
         directive.read_add( devices[0].descriptor());
         directive.read_add( devices[1].descriptor());

         EXPECT_TRUE( directive.entries().size() == 2);

         directive.read_remove( devices[0].descriptor());

         EXPECT_TRUE( directive.entries().size() == 1);

         auto ready = select::dispatch::detail::select( directive);
         EXPECT_TRUE( ready.write.size() == 0);
         // only one device should be ready for read, the other was removed
         EXPECT_TRUE( ready.read.size() == 1);
      }


      
   } // common::communication

} // casual


// epoll specific stuff
#if defined(CASUAL_PLATFORM_LINUX)

#include "common/communication/select/epoll.h"

namespace casual
{
   namespace common::communication
   {
      
      TEST( common_communication_select_epoll, error_events_are_reported_as_ready)
      {
         unittest::Trace trace;

 
         auto has_event = []( std::span<const ::epoll_event> events, int fd)
         {
            auto is_event = [ fd]( const ::epoll_event& event)
            {
               return event.data.fd == fd;
            };

            return std::ranges::any_of( events, is_event);
         };


         auto entries = array::make(
            select::directive::detail::Entry{ .descriptor = strong::file::descriptor::id{ 1}, .event = select::directive::detail::Flags::in},
            select::directive::detail::Entry{ .descriptor = strong::file::descriptor::id{ 2}, .event = select::directive::detail::Flags::out},
            select::directive::detail::Entry{ .descriptor = strong::file::descriptor::id{ 3}, .event = select::directive::detail::Flags::in | select::directive::detail::Flags::out},
            select::directive::detail::Entry{ .descriptor = strong::file::descriptor::id{ 4}, .event = select::directive::detail::Flags::in},
            select::directive::detail::Entry{ .descriptor = strong::file::descriptor::id{ 5}, .event = select::directive::detail::Flags::out},
            select::directive::detail::Entry{ .descriptor = strong::file::descriptor::id{ 6}, .event = select::directive::detail::Flags::in | select::directive::detail::Flags::out}
         );

         auto events = array::make(
            ::epoll_event{ .events = EPOLLERR, .data = { .fd = 1}},
            ::epoll_event{ .events = EPOLLERR, .data = { .fd = 2}},
            ::epoll_event{ .events = EPOLLERR, .data = { .fd = 3}},
            ::epoll_event{ .events = EPOLLIN, .data = { .fd = 4}},
            ::epoll_event{ .events = EPOLLOUT, .data = { .fd = 5}},
            ::epoll_event{ .events = EPOLLIN | EPOLLOUT, .data = { .fd = 6}}
         );

         auto ready = select::dispatch::detail::ready( entries, events);

         EXPECT_TRUE( has_event( ready.read, 1));
         EXPECT_TRUE( ! has_event( ready.read, 2));
         EXPECT_TRUE( has_event( ready.read, 3));
         EXPECT_TRUE( has_event( ready.read, 4));
         EXPECT_TRUE( ! has_event( ready.read, 5));
         EXPECT_TRUE( has_event( ready.read, 6));

         EXPECT_TRUE( ! has_event( ready.write, 1));
         EXPECT_TRUE( has_event( ready.write, 2));
         // enent fd 3 has error, and we only choose to report it as read ready.
         //EXPECT_TRUE( has_event( ready.write, 3));
         EXPECT_TRUE( ! has_event( ready.write, 4));
         EXPECT_TRUE( has_event( ready.write, 5));
         EXPECT_TRUE( has_event( ready.write, 6));
      
      }

   } // common::communication
   
} // casual


#endif // CASUAL_PLATFORM_LINUX

