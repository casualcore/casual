//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/queue/c/queue.h"
#include "queue/api/queue.h"

#include "common/buffer/type.h"
#include "common/signal.h"
#include "common/code/signal.h"

#include "domain/unittest/manager.h"

#include <xatmi.h>

namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            constexpr auto configuration = R"(
domain: 
   name: queue-domain

   groups: 
      - name: base
      - name: queue
        dependencies: [ base]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: bin/casual-queue-manager
        memberships: [ queue]

   queue:
      groups:
         - alias: group_A
           queuebase: ":memory:"
           queues:
            - name: A1
            - name: A2
            - name: A3
         - alias: group_B
           queuebase: ":memory:"
           queues:
            - name: B1
            - name: B2
            - name: B3
)";
               

            auto domain()
            {
               return domain::unittest::manager( configuration);
            }

            namespace buffer
            {
               struct Deleter
               {
                  void operator () ( const char* data)
                  {
                     ::tpfree( data);
                  }
               };

               struct Type : std::unique_ptr< char, Deleter>
               {
                  using std::unique_ptr< char, Deleter>::unique_ptr;

                  auto size() const 
                  {
                     return ::tptypes( get(), nullptr, nullptr);
                  }

               };

               auto allocate( long size = 1024)
               {
                  auto result = Type{ ::tpalloc( X_OCTET, nullptr, size)};
                  common::unittest::random::range( common::range::make( result.get(), size));
                  return result;
               }

            } // buffer

         } // <unnamed>
      } // local
      
      TEST( casual_queue_c_api, crete_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto descriptor  = casual_queue_message_create( { nullptr, 0l});

         EXPECT_TRUE( descriptor >= 0);
         EXPECT_TRUE( casual_qerrno == 0);

         EXPECT_TRUE( casual_queue_message_delete( descriptor) != -1);
      }

      TEST( casual_queue_c_api, message_get_set_attributes)
      {
         common::unittest::Trace trace;
         using namespace std::string_view_literals;

         auto descriptor  = casual_queue_message_create( { nullptr, 0l});

         ASSERT_TRUE( descriptor >= 0);
         casual_queue_message_attribute_set_properties( descriptor, "a");
         casual_queue_message_attribute_set_reply( descriptor, "b");
         casual_queue_message_attribute_set_available( descriptor, 42);

         EXPECT_TRUE( casual_queue_message_attribute_get_properties( descriptor) == "a"sv);
         EXPECT_TRUE( casual_queue_message_attribute_get_reply( descriptor) == "b"sv);
         long ms_since_epoch{};
         EXPECT_TRUE( casual_queue_message_attribute_get_available( descriptor, &ms_since_epoch) == 0);
         EXPECT_TRUE( ms_since_epoch == 42);

         EXPECT_TRUE( casual_queue_message_delete( descriptor) != -1);
      }

      TEST( casual_queue_c_api, message_get_set_attributes__invalid_descriptor)
      {
         common::unittest::Trace trace;
         using namespace std::string_view_literals;

         auto descriptor  = casual_queue_message_create( { nullptr, 0l});
         EXPECT_TRUE( casual_queue_message_delete( descriptor) != -1);

         ASSERT_TRUE( descriptor >= 0);
         EXPECT_TRUE( casual_queue_message_attribute_set_properties( descriptor, "a") == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS);
         EXPECT_TRUE( casual_queue_message_attribute_set_reply( descriptor, "b") == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS);
         EXPECT_TRUE( casual_queue_message_attribute_set_available( descriptor, 42) == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS);

         EXPECT_TRUE( casual_queue_message_attribute_get_properties( descriptor) == nullptr);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS);
         EXPECT_TRUE( casual_queue_message_attribute_get_reply( descriptor) == nullptr);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS);
         EXPECT_TRUE( casual_queue_message_attribute_get_available( descriptor, nullptr) == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS);
      }

      TEST( casual_queue_c_api, invalid_message_descriptor)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();
         
         EXPECT_TRUE( casual_queue_enqueue( "A1", 4242) == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS) << "casual_qerrno: " << casual_qerrno;
      }

      TEST( casual_queue_c_api, enqueue_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::buffer::allocate();

         auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});
         
         EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;

         EXPECT_TRUE( casual_queue_message_delete( message) != -1);
      }

      TEST( casual_queue_c_api, enqueue_message_non_existing_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::buffer::allocate();

         auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});

         EXPECT_TRUE( casual_queue_enqueue( "non_existing_queue", message) == -1);

         EXPECT_EQ( casual_qerrno, CASUAL_QE_NO_QUEUE);

         EXPECT_TRUE( casual_queue_message_delete( message) != -1);
      }


      TEST( casual_queue_c_api, enqueue_dequeue_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::buffer::allocate();

         auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});
         
         EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;
         
         // use common::Uuid for simplicity
         common::Uuid id;
         EXPECT_TRUE( casual_queue_message_get_id( message, &id.get()) != -1);
         EXPECT_TRUE( ! id.empty());
         EXPECT_TRUE( casual_queue_message_delete( message) != -1);

         message = casual_queue_dequeue( "A1", CASUAL_QUEUE_NO_SELECTOR);
         EXPECT_TRUE( message >= 0);
         
         casual_buffer_t result_buffer{};
         ASSERT_TRUE( casual_queue_message_get_buffer( message, &result_buffer) != -1);

         // so we delete the allocated buffer
         auto deleter = local::buffer::Type{ result_buffer.data};

         auto expected = common::range::make( buffer.get(), buffer.size());
         auto result = common::range::make( result_buffer.data, result_buffer.size);

         EXPECT_TRUE( common::algorithm::equal( expected, result));
         
         // make sure it's the same message (id)
         common::Uuid result_id;
         EXPECT_TRUE( casual_queue_message_get_id( message, &result_id.get()) != -1);

         EXPECT_TRUE( id == result_id);

      }

      TEST( casual_queue_c_api, enqueue_peek_dequeue_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::buffer::allocate();

         auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});

         EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;

         // use common::Uuid for simplicity
         common::Uuid id;
         EXPECT_TRUE( casual_queue_message_get_id( message, &id.get()) != -1);
         EXPECT_TRUE( ! id.empty());
         EXPECT_TRUE( casual_queue_message_delete( message) != -1);

         {
            message = casual_queue_peek( "A1", CASUAL_QUEUE_NO_SELECTOR);
            EXPECT_TRUE( message >= 0);

            casual_buffer_t result_buffer{};
            ASSERT_TRUE( casual_queue_message_get_buffer( message, &result_buffer) != -1);

            // so we delete the allocated buffer
            auto deleter = local::buffer::Type{ result_buffer.data};

            auto expected = common::range::make( buffer.get(), buffer.size());
            auto result = common::range::make( result_buffer.data, result_buffer.size);

            EXPECT_TRUE( common::algorithm::equal( expected, result));

            // make sure it's the same message (id)
            common::Uuid result_id;
            EXPECT_TRUE( casual_queue_message_get_id( message, &result_id.get()) != -1);

            EXPECT_EQ( id, result_id);
            EXPECT_TRUE( casual_queue_message_delete( message) != -1);
         }
         {
            // make sure message still exists
            message = casual_queue_dequeue( "A1", CASUAL_QUEUE_NO_SELECTOR);
            EXPECT_TRUE( message >= 0);

            casual_buffer_t result_buffer{};
            ASSERT_TRUE( casual_queue_message_get_buffer( message, &result_buffer) != -1);

            // so we delete the allocated buffer
            auto deleter = local::buffer::Type{ result_buffer.data};

            auto expected = common::range::make( buffer.get(), buffer.size());
            auto result = common::range::make( result_buffer.data, result_buffer.size);

            EXPECT_TRUE( common::algorithm::equal( expected, result));

            // make sure it's the same message (id)
            common::Uuid result_id;
            EXPECT_TRUE( casual_queue_message_get_id( message, &result_id.get()) != -1);

            EXPECT_TRUE( id == result_id);
         }

      }



      TEST( casual_queue_c_api, enqueue_dequeue_message__properties)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         // enqueue
         {
            auto buffer = local::buffer::allocate();

            auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});

            // enqueue one message without properties
            EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;
            
            // One with properties, to select on further down
            ASSERT_TRUE( casual_queue_message_attribute_set_properties( message, "foo") != -1);
            
            EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;
            EXPECT_TRUE( casual_queue_message_delete( message) != -1);
         }

         // dequeue
         {
            auto selector = casual_queue_selector_create();
            EXPECT_TRUE( casual_queue_selector_set_properties( selector, "foo") != -1);

            auto message = casual_queue_dequeue( "A1", selector);
            EXPECT_TRUE( message >= 0);

            ASSERT_TRUE( casual_queue_message_attribute_get_properties( message) == std::string{ "foo"});

            EXPECT_TRUE( casual_queue_selector_delete( selector) != -1);

         }
      }

      TEST( casual_queue_c_api, enqueue_peek_dequeue_message__properties)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         // enqueue
         {
            auto buffer = local::buffer::allocate();

            auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});

            // enqueue one message without properties
            EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;

            // One with properties, to select on further down
            ASSERT_TRUE( casual_queue_message_attribute_set_properties( message, "foo") != -1);

            EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;
            EXPECT_TRUE( casual_queue_message_delete( message) != -1);
         }

         // peek
         {
            auto selector = casual_queue_selector_create();
            EXPECT_TRUE( casual_queue_selector_set_properties( selector, "foo") != -1);

            auto message = casual_queue_peek( "A1", selector);
            EXPECT_TRUE( message >= 0);

            ASSERT_TRUE( casual_queue_message_attribute_get_properties( message) == std::string{ "foo"});

            EXPECT_TRUE( casual_queue_selector_delete( selector) != -1);
            EXPECT_TRUE( casual_queue_message_delete( message) != -1);

         }

         // dequeue
         {
            auto selector = casual_queue_selector_create();
            EXPECT_TRUE( casual_queue_selector_set_properties( selector, "foo") != -1);

            auto message = casual_queue_dequeue( "A1", selector);
            EXPECT_TRUE( message >= 0);

            ASSERT_TRUE( casual_queue_message_attribute_get_properties( message) == std::string{ "foo"});

            EXPECT_TRUE( casual_queue_selector_delete( selector) != -1);
            EXPECT_TRUE( casual_queue_message_delete( message) != -1);
         }
      }


      TEST( casual_queue_c_api, enqueue_10__browse_peek__return_false_after_half___expect_half_the_messages)
      {
         common::unittest::Trace trace;

         static constexpr auto number_of_messages = 10;
         static constexpr auto half_number_of_messages = number_of_messages / 2;

         static constexpr auto calculate_size = []( auto value) { return ( value * 10) + 10;};

         auto domain = local::domain();

         // use the "real" api to enqueue
         common::algorithm::for_n< number_of_messages>( []( auto index)
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = common::unittest::random::binary( calculate_size( index));
            
            ASSERT_TRUE( queue::enqueue( "B1", message));
         });

         auto handle_message = []( auto id, void* state)
         {
            auto& count = *static_cast< platform::size::type*>( state);
            
            casual_buffer_t buffer{};
            EXPECT_TRUE( casual_queue_message_get_buffer( id, &buffer) == 0);
            EXPECT_TRUE( buffer.size == calculate_size( count));

            ++count;

            if( count == half_number_of_messages)
               return 0;

            return 1;
         };

         platform::size::type count = 0;

         EXPECT_TRUE( casual_queue_browse_peek( "B1", handle_message, &count) == 0) << "error: " << casual_queue_error_string( casual_qerrno);

         EXPECT_TRUE( count == half_number_of_messages) << CASUAL_NAMED_VALUE( count);

      }

      TEST( casual_queue_c_api, browse_peek_non_existent_queue__expect_CASUAL_QE_NO_QUEUE)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto handle_message = []( auto, void*)
         {
            return 1;
         };

         EXPECT_TRUE( casual_queue_browse_peek( "non-existent-queue", handle_message, nullptr) == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_NO_QUEUE) << "error: " << casual_queue_error_string( casual_qerrno);
      }


      TEST( casual_queue_c_api, dequeue__signal_interrupt__expect_signaled_error)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::buffer::allocate();
         auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});
         EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "error: " << casual_queue_error_string( casual_qerrno);

         common::signal::send( common::process::id(), common::code::signal::interrupt);

         EXPECT_TRUE( casual_queue_dequeue( "A1", CASUAL_QUEUE_NO_SELECTOR) == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_SIGNALED) << "error: " << casual_queue_error_string( casual_qerrno);

         EXPECT_TRUE( casual_queue_message_delete( message) != -1);
      }
   } // queue
} // casual