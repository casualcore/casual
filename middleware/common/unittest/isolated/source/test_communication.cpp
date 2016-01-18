//!
//! test_communication.cpp
//!
//! Created on: Jan 5, 2016
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/communication/ipc.h"
#include "common/message/type.h"

#include <random>

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace local
         {
            namespace
            {
               namespace payload
               {
                  const common::platform::binary_type& get()
                  {
                     auto create = [](){
                        common::platform::binary_type result( 1000);
                        range::numeric::iota( result, 0);
                        std::random_device rd;
                        std::shuffle( std::begin( result), std::end( result), std::mt19937{ rd()});

                        return result;
                     };
                     static auto result = create();
                     return result;
                  }

                  using range_type = range::traits< common::platform::binary_type>;

                  std::vector< ipc::message::Transport> parts( std::size_t size, common::message::Type type, const common::platform::binary_type& payload = get())
                  {
                     std::vector< ipc::message::Transport> result;

                     auto current = std::begin( payload);
                     while( current != std::end( payload))
                     {
                        auto last = current + size;
                        if( last > std::end( payload))
                        {
                           last = std::end( payload);
                        }
                        ipc::message::Transport transport{ type};
                        transport.message.header.complete_size = payload.size();
                        transport.message.header.count = std::distance( current, last);
                        transport.message.header.offset = std::distance( std::begin( payload), current);

                        range::copy( range::make( current, last), std::begin( transport.message.payload));

                        result.push_back( transport);
                        current = last;
                     }

                     return result;
                  }

                  message::Complete complete( const std::vector< ipc::message::Transport>& parts)
                  {
                     assert( ! parts.empty());

                     message::Complete message{ parts.front()};

                     for( auto& transport : range::make( std::begin( parts) + 1, std::end( parts)))
                     {
                        message.add( transport);
                     }
                     return message;
                  }


               } // payload
            } // <unnamed>
         } // local

         TEST( casual_common_communication_message, complete_add__ordered)
         {
            auto complete = local::payload::complete( local::payload::parts( 100, common::message::Type::traffic_event));

            EXPECT_TRUE( static_cast< bool>( complete));
            EXPECT_TRUE( complete.complete()) << "complete.unhandled().size()" << complete.unhandled().size();
            if( ! complete.complete())
            {
               for( auto a : complete.unhandled())
               {
                  std::cerr << "size: " << a.size() << std::endl;
               }
            }

            EXPECT_TRUE( complete.payload == local::payload::get());
         }

         TEST( casual_common_communication_message, complete_add__reverse_ordered)
         {
            auto parts = local::payload::parts( 100, common::message::Type::traffic_event);
            range::reverse( parts);
            auto complete = local::payload::complete( parts);

            EXPECT_TRUE( static_cast< bool>( complete));
            EXPECT_TRUE( complete.complete()) << "complete.unhandled().size()" << complete.unhandled().size();
            if( ! complete.complete())
            {
               for( auto a : complete.unhandled())
               {
                  std::cerr << "size: " << a.size() << std::endl;
               }
            }
            EXPECT_TRUE( complete.payload == local::payload::get());
         }


         TEST( casual_common_communication_ipc, instanciate)
         {
            ipc::inbound::Device device;
         }

         TEST( casual_common_communication_ipc, exists)
         {
            ipc::inbound::Device device;
            EXPECT_TRUE( ipc::exists( device.connector().id()));

         }

         TEST( casual_common_communication_ipc, non_blocking_receive__expect_no_messages)
         {
            ipc::inbound::Device device;

            common::message::lookup::process::Reply message;
            EXPECT_FALSE( ( device.receive( message, ipc::policy::non::Blocking{})));

         }

         TEST( casual_common_communication_ipc, send_receivce__small_message)
         {
            ipc::inbound::Device destination;


            auto send = []( ipc::handle_type id)
            {
               common::message::lookup::process::Reply message;
               message.domain = "charlie";
               return ipc::non::blocking::send( id, message);
            };

            auto correlation = send( destination.connector().id());


            common::message::lookup::process::Reply message;
            EXPECT_TRUE( ( ipc::non::blocking::receive( destination, message, correlation)));

         }

      } // communication

   } // common

} // casual
