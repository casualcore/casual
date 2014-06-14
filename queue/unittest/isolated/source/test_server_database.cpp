//!
//! test_server_database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "queue/server/database.h"


#include "common/file.h"
#include "common/exception.h"


namespace casual
{
   namespace queue
   {

      namespace local
      {
         namespace
         {
            /*
            common::file::scoped::Path file()
            {
               return common::file::scoped::Path{ "unittest_queue_server_database.db"};
            }
            */

            std::string file()
            {
               return ":memory:";
            }


            queue::Message message()
            {
               queue::Message message;

               message.correlation = common::Uuid::make();
               message.reply = "someQueue";
               message.type = 42;
               message.payload.resize( 128);

               message.avalible = common::platform::clock_type::now();
               message.timestamp = common::platform::clock_type::now();

               return message;
            }

            static const common::transaction::ID nullId{};

         } // <unnamed>
      } // local

      TEST( casual_queue_server_database, create_queue)
      {
         auto path = local::file();
         server::Database database( path);

         database.create( server::Queue{ "unittest_queue"});

         auto queues = database.queues();

         // there is always a error-queue, and a global error-queue
         ASSERT_TRUE( queues.size() == 3);
         EXPECT_TRUE( queues.at( 0).name == "casual-error-queue");
         EXPECT_TRUE( queues.at( 1).name == "unittest_queue");
         EXPECT_TRUE( queues.at( 2).name == "unittest_queue_error");
      }


      TEST( casual_queue_server_database, create_100_queues)
      {
         auto path = local::file();
         server::Database database( path);

         {
            server::scoped::Writer writer{ database};

            auto count = 0;

            while( count++ < 100)
            {
               database.create( server::Queue{ "unittest_queue" + std::to_string( count)});
            }
         }

         auto queues = database.queues();

         // there is always an error-queue for each queue, and a global error-queue
         ASSERT_TRUE( queues.size() == 201) << "size: " << queues.size();
         EXPECT_TRUE( queues.at( 1).name == "unittest_queue1") << queues.at( 200).name;
      }


      TEST( casual_queue_server_database, enqueue_one_message)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});

         Message message = local::message();

         EXPECT_NO_THROW({
            database.enqueue( queue.id, local::nullId, message);
         });
      }


      TEST( casual_queue_server_database, dequeue_one_message)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});

         Message origin = local::message();
         database.enqueue( queue.id, local::nullId, origin);

         Message fetched = database.dequeue( queue.id, local::nullId);

         EXPECT_TRUE( origin.correlation == fetched.correlation);
         EXPECT_TRUE( origin.type == fetched.type);
         EXPECT_TRUE( origin.reply == fetched.reply);
         EXPECT_TRUE( origin.avalible == fetched.avalible);
         EXPECT_TRUE( origin.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();

      }


      TEST( casual_queue_server_database, dequeue_100_message)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});

         std::vector< queue::Message> messages;

         auto count = 0;
         while( count++ < 100)
         {
            auto m = local::message();
            m.type = count;
            messages.push_back( std::move( m));
         }

         {
            server::scoped::Writer writer{ database};
            common::range::for_each( messages, [&]( const queue::Message& m){
               database.enqueue( queue.id, local::nullId, m);});
         }

         server::scoped::Writer writer{ database};

         common::range::for_each( messages,[&]( const queue::Message& origin){

            Message fetched = database.dequeue( queue.id, local::nullId);

            EXPECT_TRUE( origin.correlation == fetched.correlation);
            EXPECT_TRUE( origin.type == fetched.type) << "origin.type: " << origin.type << " fetched.type; " << fetched.type;
            EXPECT_TRUE( origin.reply == fetched.reply);
            EXPECT_TRUE( origin.avalible == fetched.avalible);
            EXPECT_TRUE( origin.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

      }


      TEST( casual_queue_server_database, enqueue_one_message_in_transaction)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});


         Message origin = local::message();

         {
            common::transaction::ID xid = common::transaction::ID::create();

            database.enqueue( queue.id, xid, origin);

            EXPECT_THROW({
               database.dequeue( queue.id, local::nullId);
            }, common::exception::Base);

            EXPECT_THROW({
               database.dequeue( queue.id, xid);
            }, common::exception::Base);

            database.commit( xid);
         }

         {
            common::transaction::ID xid = common::transaction::ID::create();

            Message fetched = database.dequeue( queue.id, xid);

            EXPECT_TRUE( origin.correlation == fetched.correlation);
            EXPECT_TRUE( origin.type == fetched.type);
            EXPECT_TRUE( origin.reply == fetched.reply);
            EXPECT_TRUE( origin.avalible == fetched.avalible);
            EXPECT_TRUE( origin.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();

         }

      }


      TEST( casual_queue_server_database, enqueue_one_message_in_transaction_rollback)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});


         Message origin = local::message();

         {
            common::transaction::ID xid = common::transaction::ID::create();

            database.enqueue( queue.id, xid, origin);
            database.commit( xid);
         }

         {
            common::transaction::ID xid = common::transaction::ID::create();

            Message fetched = database.dequeue( queue.id, xid);

            EXPECT_TRUE( origin.correlation == fetched.correlation);

            database.rollback( xid);

            EXPECT_THROW({
               database.dequeue( queue.id, xid);
            }, common::exception::Base);
         }

         //
         // Should be in error queue
         //
         {
            common::transaction::ID xid = common::transaction::ID::create();
            Message fetched = database.dequeue( queue.error, xid);

            EXPECT_TRUE( origin.correlation == fetched.correlation);

            database.rollback( xid);
         }

         //
         // Should be in global error queue
         //
         {
            common::transaction::ID xid = common::transaction::ID::create();
            Message fetched = database.dequeue( database.error(), xid);

            EXPECT_TRUE( origin.correlation == fetched.correlation);
         }

      }


   } // queue
} // casual
