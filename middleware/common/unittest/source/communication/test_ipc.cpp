//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/communication/ipc.h"
#include "common/message/domain.h"
#include "common/message/service.h"
#include "common/mockup/ipc.h"

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
                  const common::platform::binary::type& get()
                  {
                     auto create = [](){
                        common::platform::binary::type result( 1000);
                        range::numeric::iota( result, 0);
                        std::random_device rd;
                        std::shuffle( std::begin( result), std::end( result), std::mt19937{ rd()});

                        return result;
                     };
                     static auto result = create();
                     return result;
                  }

                  using range_type = range::traits< common::platform::binary::type>;

                  std::vector< ipc::message::Transport> parts( std::size_t size, common::message::Type type, const common::platform::binary::type& payload = get())
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

                     auto& front = parts.front();

                     message::Complete message{ front.type(), front.correlation(), front.complete_size(), front};

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
            common::unittest::Trace trace;

            auto complete = local::payload::complete( local::payload::parts( 100, common::message::Type::event_service_call));

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
            common::unittest::Trace trace;

            auto parts = local::payload::parts( 100, common::message::Type::event_service_call);
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
            common::unittest::Trace trace;

            ipc::inbound::Device device;
         }

         TEST( casual_common_communication_ipc, exists)
         {
            common::unittest::Trace trace;

            ipc::inbound::Device device;
            EXPECT_TRUE( ipc::exists( device.connector().id()));

         }

         TEST( casual_common_communication_ipc, non_blocking_receive__expect_no_messages)
         {
            common::unittest::Trace trace;

            ipc::inbound::Device device;

            common::message::domain::process::lookup::Reply message;
            EXPECT_FALSE( ( device.receive( message, ipc::policy::non::Blocking{})));

         }

         TEST( casual_common_communication_ipc, send_receivce__small_message)
         {
            common::unittest::Trace trace;

            ipc::inbound::Device destination;


            auto send = []( ipc::handle_type id)
            {
               common::message::domain::process::lookup::Reply message;
               return ipc::non::blocking::send( id, message);
            };

            auto correlation = send( destination.connector().id());


            common::message::domain::process::lookup::Reply message;
            EXPECT_TRUE( ( ipc::non::blocking::receive( destination, message, correlation)));
         }


         TEST( casual_common_communication_ipc, send_receivce__1_exactly_transport_size__expect_exactly_1_transport_message)
         {
            common::unittest::Trace trace;

            auto send_message = unittest::random::message( ipc::message::transport::max_payload_size());

            mockup::ipc::eventually::send( ipc::inbound::id(), send_message);


            unittest::Message receive_message;
            {

               ipc::message::Transport transport;
               EXPECT_TRUE( ipc::native::receive( ipc::inbound::id(), transport, {}));
               EXPECT_TRUE( transport.message.header.offset == 0);
               EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

               // we expect no more transports
               {
                  ipc::message::Transport dummy;
                  EXPECT_FALSE( ipc::native::receive( ipc::inbound::id(), dummy, { ipc::native::Flag::non_blocking}));
               }

               message::Complete complete{ transport.type(), transport.correlation(), transport.complete_size(), transport};
               marshal::complete( complete, receive_message);
            }

            EXPECT_TRUE( ( range::equal( receive_message.payload, send_message.payload)));
         }


         TEST( casual_common_communication_ipc, send_receivce__1__2x_transport_size__expect_exactly_2_transport_message)
         {
            common::unittest::Trace trace;

            auto send_message = unittest::random::message( 2 * ipc::message::transport::max_payload_size());
            mockup::ipc::eventually::send( ipc::inbound::id(), send_message);


            unittest::Message receive_message;
            {

               ipc::message::Transport transport;
               EXPECT_TRUE( ipc::native::receive( ipc::inbound::id(), transport, {}));
               EXPECT_TRUE( transport.message.header.offset == 0);
               EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

               message::Complete complete{ transport.type(), transport.correlation(), transport.complete_size(), transport};

               EXPECT_TRUE( ipc::native::receive( ipc::inbound::id(), transport, {}));
               EXPECT_TRUE( transport.message.header.offset == transport.message.header.count);
               EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

               // we expect no more transports
               {
                  ipc::message::Transport dummy;
                  EXPECT_FALSE( ipc::native::receive( ipc::inbound::id(), dummy, { ipc::native::Flag::non_blocking}));
               }
               complete.add( transport);

               marshal::complete( complete, receive_message);
            }

            EXPECT_TRUE( ( range::equal( receive_message.payload, send_message.payload)));
         }


         TEST( casual_common_communication_ipc, send_receivce__1__10x_transport_size__expect_correct_assembly)
         {
            common::unittest::Trace trace;

            auto send_message = unittest::random::message( 10 * ipc::message::transport::max_payload_size());

            mockup::ipc::eventually::send( ipc::inbound::id(), send_message);


            unittest::Message receive_message;
            ipc::blocking::receive( ipc::inbound::device(), receive_message);
            EXPECT_TRUE( ( range::equal( receive_message.payload, send_message.payload)));

         }

         TEST( casual_common_communication_ipc, send_receivce__1__message_size_1103__expect_correct_assembly)
         {
            common::unittest::Trace trace;

            auto send_message = unittest::random::message( 1103);

            mockup::ipc::eventually::send( ipc::inbound::id(), send_message);


            unittest::Message receive_message;
            ipc::blocking::receive( ipc::inbound::device(), receive_message);
            EXPECT_TRUE( receive_message.size() == 1103);
            EXPECT_TRUE( ( range::equal( receive_message.payload, send_message.payload)));

         }



         TEST( casual_common_communication_ipc, send_receivce__10__3x_transport_size__expect_correct_assembly)
         {
            common::unittest::Trace trace;

            std::vector< common::Uuid> correlations;

            auto send_message = unittest::random::message( 3 * ipc::message::transport::max_payload_size());

            for( int count = 10; count > 0; --count)
            {
               correlations.push_back( mockup::ipc::eventually::send( ipc::inbound::id(), send_message));
            }


            // We reverse the order we fetch the messages
            for( auto& correlation : common::range::reverse( correlations))
            {
               unittest::Message receive_message;

               ipc::blocking::receive( ipc::inbound::device(), receive_message, correlation);

               EXPECT_TRUE( receive_message.correlation == correlation);
               EXPECT_TRUE( ( range::equal( receive_message.payload, send_message.payload)));
            }
         }

         TEST( casual_common_communication_ipc, send_receive__service_call_reply)
         {
            common::unittest::Trace trace;

            common::message::service::call::Reply message;
            {
               message.correlation = uuid::make();
               message.execution = uuid::make();
               message.transaction.trid = transaction::ID::create( process::handle());
               message.transaction.state = common::message::service::Transaction::State::rollback;
               message.buffer.type = ".binary";
               message.status = 0;
               message.code = 0;
               message.buffer.memory = unittest::random::binary( 1200);
            }

            const auto correlation = mockup::ipc::eventually::send( ipc::inbound::id(), message);

            common::message::service::call::Reply receive_message;
            ipc::blocking::receive( ipc::inbound::device(), receive_message, correlation);


            EXPECT_TRUE( receive_message.correlation == correlation);
            EXPECT_TRUE( receive_message.execution == message.execution);
            EXPECT_TRUE( receive_message.transaction.trid == message.transaction.trid);
            EXPECT_TRUE( receive_message.transaction.state == message.transaction.state);
            EXPECT_TRUE( receive_message.buffer.memory == message.buffer.memory);
         }


      } // communication
   } // common
} // casual