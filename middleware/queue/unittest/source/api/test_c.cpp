//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/queue/c/queue.h"

#include "domain/manager/unittest/process.h"

#include <xatmi.h>

namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            struct Domain 
            {

               domain::manager::unittest::Process domain{ { Domain::configuration}};

               static constexpr auto configuration = R"(
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
            };

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

         local::Domain domain;

         auto descriptor  = casual_queue_message_create( { nullptr, 0l});

         EXPECT_TRUE( descriptor >= 0);
         EXPECT_TRUE( casual_qerrno == 0);

         EXPECT_TRUE( casual_queue_message_delete( descriptor) != -1);
      }

      TEST( casual_queue_c_api, invalid_message_descriptor)
      {
         common::unittest::Trace trace;

         local::Domain domain;
         
         EXPECT_TRUE( casual_queue_enqueue( "A1", 4242) == -1);
         EXPECT_TRUE( casual_qerrno == CASUAL_QE_INVALID_ARGUMENTS) << "casual_qerrno: " << casual_qerrno;
      }

      TEST( casual_queue_c_api, enqueue_message)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = local::buffer::allocate();

         auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});
         
         EXPECT_TRUE( casual_queue_enqueue( "A1", message) != -1) << "casual_qerrno: " << casual_qerrno;

         EXPECT_TRUE( casual_queue_message_delete( message) != -1);
      }

      TEST( casual_queue_c_api, enqueue_message_non_existing_queue)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = local::buffer::allocate();

         auto message  = casual_queue_message_create( { buffer.get(), buffer.size()});

         EXPECT_TRUE( casual_queue_enqueue( "non_existing_queue", message) == -1);

         EXPECT_TRUE( casual_qerrno == CASUAL_QE_NO_QUEUE);

         EXPECT_TRUE( casual_queue_message_delete( message) != -1);
      }


      TEST( casual_queue_c_api, enqueue_dequeue_message)
      {
         common::unittest::Trace trace;

         local::Domain domain;

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

         local::Domain domain;

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

         local::Domain domain;

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

         local::Domain domain;

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
   } // queue
} // casual