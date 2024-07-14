//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/communication/ipc.h"
#include "common/communication/ipc/send.h"
#include "common/communication/select.h"
#include "common/message/domain.h"
#include "common/message/service.h"
#include "common/sink.h"

#include "common/unittest/eventually/send.h"

#include <random>
#include <thread>

namespace casual
{
   namespace common::communication::ipc
   {
      namespace local
      {
         namespace
         {
            namespace payload
            {
               const platform::binary::type& get()
               {
                  static auto result = unittest::random::binary( 1000);
                  return result;
               }

               using range_type = range::type_t< platform::binary::type>;

               std::vector< ipc::message::Transport> parts( std::size_t size, common::message::Type type, const platform::binary::type& payload = get())
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
                     ipc::message::Transport transport;
                     transport.message.header.type = type;
                     transport.message.header.size = payload.size();
                     transport.message.header.count = std::distance( current, last);
                     transport.message.header.offset = std::distance( std::begin( payload), current);

                     algorithm::copy( range::make( current, last), std::begin( transport.message.payload));

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
         common::unittest::Trace trace;

         auto complete = local::payload::complete( local::payload::parts( 100, common::message::Type::event_service_call));

         EXPECT_TRUE( static_cast< bool>( complete));
         EXPECT_TRUE( complete.complete()) << "complete: " << complete;

         EXPECT_TRUE( complete.payload == local::payload::get());
      }

      TEST( common_communication_ipc, instantiate)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device device;
      }


      TEST( common_communication_ipc, non_blocking_receive__expect_no_messages)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device device;

         common::message::domain::process::lookup::Reply message;
         EXPECT_FALSE( ( device.receive( message, ipc::policy::non::Blocking{})));

      }

      TEST( common_communication_ipc, send_receive__small_message)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device destination;


         auto send = []( strong::ipc::id id)
         {
            common::message::domain::process::lookup::Reply message;
            return device::non::blocking::send( id, message);
         };

         auto correlation = send( destination.connector().handle().ipc());

         common::message::domain::process::lookup::Reply message;
         EXPECT_TRUE( ( device::non::blocking::receive( destination, message, correlation)));
      }

      TEST( common_communication_ipc, send_receive__large_message___flush__expect_cached)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device destination;

         const auto binary = unittest::random::binary( 12 * 1024);

         auto correlation = unittest::eventually::send( destination.connector().handle().ipc(), unittest::Message{ binary});
         
         while( destination.complete() == 0)
            destination.flush();

         unittest::Message message;
         EXPECT_TRUE( device::blocking::receive( destination, message, correlation));
         EXPECT_TRUE( message.payload == binary);

         EXPECT_TRUE( destination.complete() == 0);
         EXPECT_TRUE( destination.size() == 0);
      }

      TEST( common_communication_ipc, send_receive__10_large_message___flush__expect_cached)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device destination;

         constexpr platform::size::type count = 10;

         const auto binary = unittest::random::binary( 12 * 1024);

         auto correlations = algorithm::generate_n< count>( [ipc = destination.connector().handle().ipc(), &binary]()
         {
            return unittest::eventually::send( ipc, unittest::Message{ binary});
         });

         while( destination.complete() < count)
            destination.flush();

         for( auto& correlation : correlations)
         {
            unittest::Message message;
            EXPECT_TRUE( device::blocking::receive( destination, message, correlation));
            EXPECT_TRUE( message.payload == binary);
         };

         EXPECT_TRUE( destination.complete() == 0);
         EXPECT_TRUE( destination.size() == 0);
      }


      TEST( common_communication_ipc, send_receive__1_exactly_transport_size__expect_exactly_1_transport_message)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( ipc::message::transport::max_payload_size());

         unittest::eventually::send( ipc::inbound::ipc(), send_message);

         unittest::Message receive_message;
         {

            ipc::message::Transport transport;
            EXPECT_TRUE( ipc::native::receive( ipc::inbound::handle(), transport, ipc::native::Flag::none));
            EXPECT_TRUE( transport.message.header.offset == 0);
            EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size()) 
               << CASUAL_NAMED_VALUE( transport.message.header.count) << '\n'
               << CASUAL_NAMED_VALUE( ipc::message::transport::max_payload_size());

            // we expect no more transports
            {
               ipc::message::Transport dummy;
               EXPECT_FALSE( ipc::native::receive( ipc::inbound::handle(), dummy, ipc::native::Flag::non_blocking));
            }

            message::Complete complete{ transport};
            serialize::native::complete( complete, receive_message);
         }

         EXPECT_TRUE( ( algorithm::equal( receive_message.payload, send_message.payload)));
      }

      TEST( common_communication_ipc, send_receive__1__2x_transport_size__expect_exactly_2_transport_message)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( 2 * ipc::message::transport::max_payload_size());
         unittest::eventually::send( ipc::inbound::ipc(), send_message);


         unittest::Message receive_message;
         {

            ipc::message::Transport transport;
            EXPECT_TRUE( ipc::native::receive( ipc::inbound::handle(), transport, ipc::native::Flag::none));
            EXPECT_TRUE( transport.message.header.offset == 0);
            EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

            message::Complete complete{ transport};

            EXPECT_TRUE( ipc::native::receive( ipc::inbound::handle(), transport, ipc::native::Flag::none));
            EXPECT_TRUE( transport.message.header.offset == transport.message.header.count);
            EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

            // we expect no more transports
            {
               ipc::message::Transport dummy;
               EXPECT_FALSE( ipc::native::receive( ipc::inbound::handle(), dummy, ipc::native::Flag::non_blocking));
            }
            complete.add( transport);

            serialize::native::complete( complete, receive_message);
         }

         EXPECT_TRUE( ( algorithm::equal( receive_message.payload, send_message.payload)));
      }


      TEST( common_communication_ipc, send_receive__1__10x_transport_size__expect_correct_assembly)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( 10 * ipc::message::transport::max_payload_size());

         unittest::eventually::send( ipc::inbound::ipc(), send_message);

         unittest::Message receive_message;
         device::blocking::receive( ipc::inbound::device(), receive_message);
         EXPECT_TRUE( ( algorithm::equal( receive_message.payload, send_message.payload)));
      }

      TEST( common_communication_ipc, send_receive__1__message_size_1103__expect_correct_assembly)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( 1103);
         send_message.correlation = strong::correlation::id::generate();

         unittest::eventually::send( ipc::inbound::ipc(), send_message);

         unittest::Message receive_message;
         device::blocking::receive( ipc::inbound::device(), receive_message);
         EXPECT_TRUE( send_message == receive_message);

      }



      TEST( common_communication_ipc, send_receive__10__3x_transport_size__expect_correct_assembly)
      {
         common::unittest::Trace trace;

         std::vector< strong::correlation::id> correlations;

         auto send_message = unittest::message::transport::size( 3 * ipc::message::transport::max_payload_size());

         for( int count = 10; count > 0; --count)
         {
            correlations.push_back( unittest::eventually::send( ipc::inbound::ipc(), send_message));
         }

         // We reverse the order we fetch the messages
         for( auto& correlation : common::algorithm::reverse( correlations))
         {
            unittest::Message receive_message;

            device::blocking::receive( ipc::inbound::device(), receive_message, correlation);

            ASSERT_TRUE( receive_message.correlation == correlation);
            ASSERT_TRUE( algorithm::equal( receive_message.payload, send_message.payload));
         }
      }

      TEST( common_communication_ipc, send_receive__service_call_reply)
      {
         common::unittest::Trace trace;

         common::message::service::call::Reply message;
         {
            message.correlation = strong::correlation::id::generate();
            message.execution = strong::execution::id::generate();
            message.transaction_state = decltype( message.transaction_state)::rollback;
            message.buffer.type = ".binary";
            message.code.result = common::code::xatmi::ok;
            message.code.user = 0;
            message.buffer.data = unittest::random::binary( 1200);
         }

         const auto correlation = unittest::eventually::send( ipc::inbound::ipc(), message);

         common::message::service::call::Reply receive_message;
         device::blocking::receive( ipc::inbound::device(), receive_message, correlation);


         EXPECT_TRUE( receive_message.correlation == correlation);
         EXPECT_TRUE( receive_message.execution == message.execution);
         EXPECT_TRUE( receive_message.transaction_state == message.transaction_state);
         EXPECT_TRUE( receive_message.buffer.data == message.buffer.data);
      }

      TEST( common_communication_ipc, send_multiplex_to_absent_ipc__expect_callback_invocation__and_empty_state_when_done)
      {
         common::unittest::Trace trace;

         select::Directive directive;
         communication::ipc::send::Coordinator coordinator{ directive};

         auto absent_ipc = strong::ipc::id{ uuid::make()};

         platform::size::type invocations{};

         algorithm::for_n< 10>( [&]()
         {
            coordinator.send( absent_ipc, unittest::Message{ 1 * 1024}, [ &invocations]( auto&, auto&)
            {
               ++invocations;
            });
         });

         EXPECT_TRUE( invocations == 10) << "invocations: " << invocations << "\n" << CASUAL_NAMED_VALUE( coordinator);
         EXPECT_TRUE( coordinator.empty());
      }

      TEST( common_communication_ipc, send_multiplex_to_inbound__fill_the_ipc__and_cache_the_rest__sink_the_inbounds__expect_callback_invocation__and_empty_state_when_done)
      {
         common::unittest::Trace trace;

         select::Directive directive;
         communication::ipc::send::Coordinator coordinator{ directive};

         std::vector< communication::ipc::inbound::Device> destinations( 10);

         const auto message = unittest::Message{ platform::ipc::transport::size * 16};
         platform::size::type errors{};

         algorithm::for_n< 31>( [&]()
         {
            for( auto& destination : destinations)
            {
               auto ipc = destination.connector().handle().ipc();
               coordinator.send( ipc, message, [ &errors]( auto&, auto&)
               {
                  ++errors;
               });
            }
         });
         
         // should hold state for the (part of) messages that did not get any room.
         EXPECT_TRUE( ! coordinator.empty());
         
         const auto count = coordinator.pending();
         EXPECT_TRUE( count > 0);

         common::sink( std::move( destinations));

         // try send the cached messages, which will of course fail, and _errors_ will increase.
         coordinator.send();

         EXPECT_TRUE( errors == count) << "invocations: " << errors;
         EXPECT_TRUE( coordinator.empty()) << CASUAL_NAMED_VALUE( coordinator);
      }

      TEST( common_communication_ipc, send_multiplex__10_destinations__10__10K__messages)
      {
         common::unittest::Trace trace;
         
         // the state for the unittest.
         struct State
         {
            State()
            {
               // add all inbounds to read directive
               for( auto& device : destinations)
                  directive.read.add( device.connector().descriptor());
            }

            communication::select::Directive directive;
            communication::ipc::send::Coordinator coordinator{ directive};
            std::array< communication::ipc::inbound::Device, 10> destinations;

            struct
            {
               using map_type = std::map< strong::ipc::id, std::vector< strong::correlation::id>>;
               map_type sent;
               map_type receive;
            } correlation;

         };

         State state; 

         const auto message = unittest::Message{ 10 * 1024};


         // send 10 messages to each destination
         algorithm::for_n< 10>( [&]()
         {
            for( auto& destination : state.destinations)
            {
               const auto& ipc = destination.connector().handle().ipc();
               state.correlation.sent[ ipc].push_back( state.coordinator.send( ipc, message));
            }
         });

         auto conditions = []( auto& state)
         {
            return communication::select::dispatch::condition::compose(
               // we're done when we've received all messages that was sent.
               communication::select::dispatch::condition::done( [ &state]() { return state.correlation.sent == state.correlation.receive;})
            );
         };

         [[maybe_unused]] constexpr auto consumer = []( auto& state)
         {
            return [&state]( strong::file::descriptor::id descriptor, communication::select::tag::read)
            {
               common::Trace trace{ "consumer operator()"};
               common::log::line( common::verbose::log, "descriptor: ", descriptor);
               if( auto found = algorithm::find( state.destinations, descriptor))
               {
                  common::log::line( common::verbose::log, "found: ", *found);
                  if( auto complete = communication::device::non::blocking::next( *found))
                     state.correlation.receive[ found->connector().handle().ipc()].push_back( complete.correlation());
                  return true;
               }
               return false;
            };
         };

         communication::select::dispatch::pump(
            conditions( state),
            state.directive, 
            consumer( state), 
            state.coordinator
         );
      }

      TEST( common_communication_ipc, send_multiplex__remove_inbound__expect_detection)
      {
         common::unittest::Trace trace;

         communication::ipc::inbound::Device inbound;
         const auto ipc = inbound.connector().handle().ipc();

         communication::select::Directive directive;
         communication::ipc::send::Coordinator coordinator{ directive};

         const unittest::Message message{ 64};

         bool error_detected = false;

         // "fill" the inbound
         while( coordinator.empty())
            coordinator.send( ipc, message, [ &error_detected]( auto& id, auto& complete){ error_detected = true;});

         EXPECT_TRUE( ! error_detected);

         // The coordinator now have a socket "bound" to the inbound.
         // Lets destroy the inbound.
         common::sink( std::move( inbound));

         EXPECT_TRUE( ! communication::ipc::exists( ipc));

         // coordinator tries to send the pending message to the 
         // "bound" socket, this should be detected, and error callback should
         // be invoked.
         EXPECT_TRUE( coordinator.send());

         EXPECT_TRUE( error_detected);

      }

      namespace local
      {
         namespace
         {
            struct Message : unittest::Message
            {
               using unittest::Message::Message;
               
               platform::size::type index = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  unittest::Message::serialize( archive);
                  CASUAL_SERIALIZE( index);
               })
            };

            void send_messages( strong::ipc::id destination, Message message, platform::size::type count)
            {
               for( ; message.index < count; ++message.index)
               {
                  device::blocking::send( destination, message);
               }
            };


            void test_send( platform::size::type size, platform::size::type count)
            {
               local::Message origin( size);
               
               auto sender = std::thread{ &local::send_messages, ipc::inbound::ipc(), origin, count};

               // TODO: May probably be replaced by C++20 std::jthread
               const auto joiner = execute::scope( [&sender]() { sender.join();});

               local::Message message;
               for( auto index = 0; index < count; ++index)
               {
                  device::blocking::receive( ipc::inbound::device(), message);
                  ASSERT_EQ( message.index, index);
                  ASSERT_EQ( message.payload, origin.payload);
               }

            }
         } // <unnamed>
      } // local

      TEST( common_communication_ipc, send_receive_100_messages__1kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024, 100);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_1000_messages__1kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024, 1000);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_1000_messages__10kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024 * 10, 1000);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_1000_messages__100kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024 * 100, 1000);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_10000_messages__100kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024 * 100, 10000);
      }

      TEST( common_communication_ipc, inbound_device__discard_message_while_partially_received__expect_successfull_discard)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device device;
         ipc::partial::Destination destination{ device.connector().handle().ipc()};

         // A complete that will require 2 transports
         auto complete = serialize::native::complete< ipc::inbound::Device::complete_type>( unittest::message::transport::size( 2 * ipc::message::transport::max_payload_size()));
         const auto correlation = complete.correlation();

         auto send_single_transport = [ &destination]( auto& complete, auto part_begin)
         {
            message::Transport transport{ complete.type(), complete.size(), complete.correlation()};

            auto part_end = std::distance( part_begin, std::end( complete.payload)) > ipc::message::transport::max_payload_size() ?
                  part_begin + ipc::message::transport::max_payload_size() : std::end( complete.payload);

            transport.assign( range::make( part_begin, part_end));
            transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

            native::blocking::send( destination.socket(), destination.address(), transport);

            return part_end;
         };

         EXPECT_NO_THROW({
            // The sender sends the first half of the complete message
            auto part_begin = send_single_transport( complete, std::begin( complete.payload));

            // The receiver discards the message by correlation
            device.discard( correlation);

            // Flush message to cache to ensure that we are able to receive more
            device.flush();

            // Send the second half
            send_single_transport( complete, part_begin);

            // There should be no message for us to receive since the receiver has discarded it
            unittest::Message receive_message;
            EXPECT_FALSE( device::non::blocking::receive( device, receive_message, correlation));
         });
      }

   } // common::communication::ipc
} // casual
