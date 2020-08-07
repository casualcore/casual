//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/file.h"

#include "common/communication/pipe.h"

#include "common/message/type.h"

#include <thread>

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
               namespace eventually
               {
                  template< typename D>
                  auto send( const D& device, communication::message::Complete&& complete, platform::size::type count = 1)
                  {
                     auto correlation = complete.correlation;
                     auto dispatch = []( const pipe::Address& address, communication::message::Complete&& complete, platform::size::type count)
                     {
                        try
                        {
                           pipe::outbound::Device device{ address};

                           while( count-- > 0)
                              device.put( complete, device.policy_blocking());
                           }
                        catch( ...)
                        {
                           exception::handle( log::category::error);
                        }
                     };
                     
                     std::thread{ std::move( dispatch), device.connector().id().address(), std::move( complete), count}.detach();

                     return correlation;

                  }

                  template< typename D, typename M, typename C = serialize::native::binary::create::Output>
                  auto send( const D& device, M&& message, platform::size::type count = 1)
                  {
                     return send( device, serialize::native::complete( std::forward< M>( message)), count);
                  }
               } // eventually

               struct Message : common::message::basic_message< common::message::Type::shutdown_request>
               {
                  pipe::Address reply;
                  platform::binary::type data;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( reply);
                     CASUAL_SERIALIZE( data);
                  })

               };
            } // <unnamed>
         } // local
         TEST( common_communication_pipe, native_create)
         {  
            unittest::Trace trace;

            //file::scoped::Path file{ file::name::unique()};
            //auto handle = pipe::native::create( file);
            //EXPECT_TRUE( handle.id());
            
         } 


         TEST( common_communication_pipe, inbound_construct)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            
            EXPECT_TRUE( device.connector().id());
         } 

         TEST( common_communication_pipe, send_receive_message)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            auto address = pipe::Address::create();

            Uuid correlation;

            // send
            {
               local::Message message;
               message.reply = address;
               
               correlation = local::eventually::send( device, message);
            }

            // receive
            {
               local::Message message;
               pipe::blocking::receive( device, message);
               
               EXPECT_TRUE( message.reply == address);
               EXPECT_TRUE( message.correlation == correlation);
            }
         } 

         TEST( common_communication_pipe, send_receive_long_message)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            auto address = pipe::Address::create();

            const auto data = unittest::random::binary( pipe::message::transport::max_payload_size() * 10);

            // send
            {
               local::Message message;
               message.reply = address;
               message.data = data;
               
               local::eventually::send( device, message);
            }

            // receive
            {
               local::Message message;
               pipe::blocking::receive( device, message);
               
               EXPECT_TRUE( message.reply == address);
               EXPECT_TRUE( message.data == data);
            }
         } 

         TEST( common_communication_pipe, send_receive_10_message)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            auto address = pipe::Address::create();

            Uuid correlation;

            auto count = 10;

            // send
            {
               local::Message message;
               message.reply = address;
               
               correlation = local::eventually::send( device, message, count);
            }

            // receive
            while( count-- > 0)
            {
               local::Message message;
               pipe::blocking::receive( device, message);
               
               EXPECT_TRUE( message.reply == address);
               EXPECT_TRUE( message.correlation == correlation);
            }
            
         } 

         TEST( common_communication_pipe, send_receive_10_long_message)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            auto address = pipe::Address::create();

            Uuid correlation;
            const auto data = unittest::random::binary( pipe::message::transport::max_payload_size() * 10);
            auto count = 10;

            // send
            {
               local::Message message;
               message.reply = address;
               message.data = data;
               
               correlation = local::eventually::send( device, message, count);
            }

            // receive
            while( count-- > 0)
            {
               local::Message message;
               pipe::blocking::receive( device, message);
               
               EXPECT_TRUE( message.reply == address);
               EXPECT_TRUE( message.correlation == correlation);
               EXPECT_TRUE( message.data == data);
            }
            
         } 


         TEST( common_communication_pipe, send_10_messages_from_10_callers__receive_any)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            auto address = pipe::Address::create();

            const auto callers = 10;
            const auto messages = 10;

            // send
            for( auto count = callers; count > 0; --count)
            {
               local::Message message;
               message.reply = address;
               
               local::eventually::send( device, message, messages);
            }

            // receive
            for( auto count = messages * callers; count > 0; --count)
            {
               local::Message message;
               pipe::blocking::receive( device, message);
               
               EXPECT_TRUE( message.reply == address);
            }
         } 

         TEST( common_communication_pipe, send_10_messages_from_10_callers__receive_correlation)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            auto address = pipe::Address::create();

            const auto callers = 10;
            const auto messages = 10;

            std::vector< Uuid> correlations;

            // send
            for( auto count = callers; count > 0; --count)
            {
               local::Message message;
               message.reply = address;
               
               correlations.push_back( local::eventually::send( device, message, messages));
            }

            // receive
            for( auto& correlation : correlations)
            {
               for( auto count = messages; count > 0; --count)
               {
                  local::Message message;
                  pipe::blocking::receive( device, message, correlation);
                  
                  EXPECT_TRUE( message.reply == address);
                  EXPECT_TRUE( message.correlation == correlation);
               }
            }
         } 

         TEST( common_communication_pipe, send_10_long_messages_from_10_callers__receive_any)
         {  
            unittest::Trace trace;

            pipe::inbound::Device device;
            auto address = pipe::Address::create();
            const auto data = unittest::random::binary( pipe::message::transport::max_payload_size() * 10);

            const auto callers = 10;
            const auto messages = 10;

            // send
            for( auto count = callers; count > 0; --count)
            {
               local::Message message;
               message.reply = address;
               message.data = data;
               
               local::eventually::send( device, message, messages);
            }

            // receive
            for( auto count = messages * callers; count > 0; --count)
            {
               local::Message message;
               pipe::blocking::receive( device, message);
               
               EXPECT_TRUE( message.reply == address);
               EXPECT_TRUE( message.data == data);
            }
         } 
      } // communication
      
   } // common
} // casual

