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

            static const common::transaction::ID nullId{};

            common::message::queue::enqueue::Request message( const server::Queue& queue, const common::transaction::ID& xid = nullId)
            {
               common::message::queue::enqueue::Request result;

               result.queue = queue.id;
               result.xid.xid = xid;

               result.message.correlation = common::Uuid::make();
               result.message.reply = "someQueue";
               result.message.type = 42;
               result.message.payload.resize( 128);

               result.message.avalible = common::platform::clock_type::now();
               //result.message.timestamp = common::platform::clock_type::now();

               return result;
            }

            common::message::queue::dequeue::Request request( const server::Queue& queue, const common::transaction::ID& xid = nullId)
            {
               common::message::queue::dequeue::Request result;

               result.queue = queue.id;
               result.xid.xid = xid;

               return result;
            }


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

         auto message = local::message( queue);

         EXPECT_NO_THROW({
            database.enqueue( message);
         });
      }


      TEST( casual_queue_server_database, dequeue_one_message)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue));

         ASSERT_TRUE( fetched.message.size() == 1);
         EXPECT_TRUE( origin.message.correlation == fetched.message.at( 0).correlation);
         EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
         EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
         EXPECT_TRUE( origin.message.avalible == fetched.message.at( 0).avalible);
         //EXPECT_TRUE( origin.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();

      }


      TEST( casual_queue_server_database, dequeue_100_message)
      {

         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});

         using message_type = decltype( local::message( queue));


         std::vector< message_type > messages;

         auto count = 0;
         while( count++ < 2)
         {
            auto m = local::message( queue);
            m.message.type = count;
            messages.push_back( std::move( m));
         }

         {
            server::scoped::Writer writer{ database};
            common::range::for_each( messages, [&]( const message_type& m){
               database.enqueue( m);});
         }

         server::scoped::Writer writer{ database};

         common::range::for_each( messages,[&]( const message_type& origin){

            auto fetched = database.dequeue( local::request( queue));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.correlation == fetched.message.at( 0).correlation);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type) << "origin.type: " << origin.message.type << " fetched.type; " << fetched.message.at( 0).type;
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.avalible == fetched.message.at( 0).avalible);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

      }


      TEST( casual_queue_server_database, enqueue_one_message_in_transaction)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});


         common::transaction::ID xid = common::transaction::ID::create();
         auto origin = local::message( queue, xid);

         {
            database.enqueue( origin);

            //
            // Message should not be available.
            //
            EXPECT_TRUE( database.dequeue( local::request( queue)).message.empty());

            //
            // Not even within the same transaction
            //
            EXPECT_TRUE( database.dequeue( local::request( queue, xid)).message.empty());

            database.commit( xid);
         }

         {
            common::transaction::ID xid = common::transaction::ID::create();

            auto fetched = database.dequeue( local::request( queue, xid));

            EXPECT_TRUE( origin.message.correlation == fetched.message.at( 0).correlation);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.avalible == fetched.message.at( 0).avalible);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.message.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();

         }

      }


      TEST( casual_queue_server_database, enqueue_one_message_in_transaction_rollback)
      {
         auto path = local::file();
         server::Database database( path);
         auto queue = database.create( server::Queue{ "unittest_queue"});


         auto origin = local::message( queue);

         {
            common::transaction::ID xid = common::transaction::ID::create();

            database.enqueue( origin);
            database.commit( xid);
         }

         {
            common::transaction::ID xid = common::transaction::ID::create();

            auto fetched = database.dequeue( local::request( queue, xid));

            EXPECT_TRUE( origin.message.correlation == fetched.message.at( 0).correlation);

            database.rollback( xid);

            EXPECT_TRUE( database.dequeue( local::request( queue, xid)).message.empty());
         }

         //
         // Should be in error queue
         //
         {
            common::transaction::ID xid = common::transaction::ID::create();
            auto errorQ = queue;
            errorQ.id = queue.error;

            auto fetched = database.dequeue( local::request( errorQ, xid));

            EXPECT_TRUE( origin.message.correlation == fetched.message.at( 0).correlation);

            database.rollback( xid);
         }

         //
         // Should be in global error queue
         //
         {
            common::transaction::ID xid = common::transaction::ID::create();

            auto errorQ = queue;
            errorQ.id = database.error();

            auto fetched = database.dequeue( local::request( errorQ, xid));

            EXPECT_TRUE( origin.message.correlation == fetched.message.at( 0).correlation);
         }

      }


   } // queue
} // casual
