//!
//! test_server_database.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "queue/group/database.h"


#include "common/file.h"
#include "common/exception.h"
#include "common/environment.h"




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

            common::message::queue::enqueue::Request message( const group::Queue& queue, const common::transaction::ID& trid = nullId)
            {
               common::message::queue::enqueue::Request result;

               result.queue = queue.id;
               result.trid = trid;

               result.message.id = common::uuid::make();
               result.message.reply = "someQueue";
               result.message.type.type = "X_OCTET";
               result.message.type.subtype = "binary";
               result.message.payload.resize( 128);

               result.message.avalible = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::clock_type::now());
               //result.message.timestamp = common::platform::clock_type::now();

               return result;
            }

            common::message::queue::dequeue::Request request( const group::Queue& queue, const common::transaction::ID& trid = nullId)
            {
               common::message::queue::dequeue::Request result;

               result.queue = queue.id;
               result.trid = trid;

               return result;
            }

         } // <unnamed>
      } // local

      TEST( casual_queue_group_database, create_queue)
      {
         auto path = local::file();
         group::Database database( path);

         database.update( { group::Queue{ "unittest_queue"}}, {});

         auto queues = database.queues();

         // there is always a error-queue, and a global error-queue
         ASSERT_TRUE( queues.size() == 3);
         EXPECT_TRUE( queues.at( 0).name == "error-queue") << "queues.at( 0).name: " << queues.at( 0).name;
         EXPECT_TRUE( queues.at( 1).name == "unittest_queue_error");
         EXPECT_TRUE( queues.at( 2).name == "unittest_queue");
      }

      TEST( casual_queue_group_database, remove_queue)
      {
         auto path = local::file();
         group::Database database( path);

         auto queue = database.update( {group::Queue{ "unittest_queue"}}, {});

         database.update( {}, { queue.at( 0).id});

         EXPECT_TRUE( database.queues().size() == 1) << database.queues().size();
      }

      TEST( casual_queue_group_database, update_queue)
      {
         auto path = local::file();
         group::Database database( path);

         auto queue = database.update( { group::Queue{ "unittest_queue"}}, {});

         queue.at( 0).name = "foo-bar";

         database.update( { queue.at( 0)}, {});

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 3) << queues.size();
         EXPECT_TRUE( queues.at( 0).name == "error-queue");
         EXPECT_TRUE( queues.at( 1).name == "foo-bar_error");
         EXPECT_TRUE( queues.at( 2).name == "foo-bar");
      }


      TEST( casual_queue_group_database, create_queue_on_disc_and_open_again)
      {
         common::file::scoped::Path path{
            common::file::unique(
               common::environment::directory::temporary() + "/",
               "unittest_queue_server_database.db")
            };

         {
            group::Database database( path);

            database.create( group::Queue{ "unittest_queue"});
         }

         {
            //
            // open again
            //
            group::Database database( path);

            auto queues = database.queues();

            // there is always a error-queue, and a global error-queue
            ASSERT_TRUE( queues.size() == 3);
            //EXPECT_TRUE( queues.at( 0).name == "unittest_queue_server_database-error-queue") << queues.at( 0).name;
            EXPECT_TRUE( queues.at( 1).name == "unittest_queue_error");
            EXPECT_TRUE( queues.at( 2).name == "unittest_queue");
         }

      }

      TEST( casual_queue_group_database, create_5_queue_on_disc_and_open_again)
      {
         common::file::scoped::Path path{
            common::file::unique(
               common::environment::directory::temporary() + "/",
               "unittest_queue_server_database.db")
            };

         {
            group::Database database( path);

            for( int index = 1; index <= 5; ++index)
            {
               database.create( group::Queue{ "unittest_queue_" + std::to_string( index)});
            }
         }

         {
            //
            // open again
            //
            group::Database database( path);

            auto queues = database.queues();

            // there is always a error-queue, and a global error-queue
            ASSERT_TRUE( queues.size() == 5 * 2 + 1);
            //EXPECT_TRUE( queues.at( 0).name == "unittest_queue_server_database-error-queue") << queues.at( 0).name;
            EXPECT_TRUE( queues.at( 1).name == "unittest_queue_1_error") << queues.at( 1).name;
            EXPECT_TRUE( queues.at( 2).name == "unittest_queue_1") << queues.at( 2).name;
         }

      }


      TEST( casual_queue_group_database, create_100_queues)
      {
         auto path = local::file();
         group::Database database( path);

         {
            group::scoped::Writer writer{ database};

            auto count = 0;

            while( count++ < 100)
            {
               database.create( group::Queue{ "unittest_queue" + std::to_string( count)});
            }
         }

         auto queues = database.queues();

         // there is always an error-queue for each queue, and a global error-queue
         ASSERT_TRUE( queues.size() == 201) << "size: " << queues.size();
         EXPECT_TRUE( queues.at( 1).name == "unittest_queue1_error") << queues.at( 200).name;
      }


      TEST( casual_queue_group_database, enqueue_one_message)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto message = local::message( queue);


         EXPECT_NO_THROW({
            EXPECT_TRUE( static_cast< bool>( database.enqueue( message).id));
         });

      }

      TEST( casual_queue_group_database, enqueue_one_message__get_queue_info)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto message = local::message( queue);

         EXPECT_NO_THROW({
            database.enqueue( message);
         });

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 3);
         EXPECT_TRUE( queues.at( 2).name == "unittest_queue");
         EXPECT_TRUE( queues.at( 2).messages == 1) << " queues.at( 2).messages: " <<  queues.at( 2).messages;

      }

      TEST( casual_queue_group_database, enqueue_one_message__get_message_info)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto message = local::message( queue);

         EXPECT_NO_THROW({
            database.enqueue( message);
         });

         auto messages = database.messages( queue.id);

         ASSERT_TRUE( messages.size() == 1);
         EXPECT_TRUE( messages.at( 0).id  == message.message.id);
      }


      TEST( casual_queue_group_database, dequeue_one_message)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue));


         ASSERT_TRUE( fetched.message.size() == 1);
         EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
         EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
         EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
         EXPECT_TRUE( origin.message.avalible == fetched.message.at( 0).avalible);
         //EXPECT_TRUE( origin.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();

      }


      TEST( casual_queue_group_database, dequeue_100_message)
      {

         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});

         using message_type = decltype( local::message( queue));


         std::vector< message_type > messages;

         auto count = 0;
         while( count++ < 100)
         {
            auto m = local::message( queue);
            m.message.type.type = std::to_string( count);
            messages.push_back( std::move( m));
         }

         {
            group::scoped::Writer writer{ database};
            common::range::for_each( messages, [&]( const message_type& m){
               database.enqueue( m);});
         }

         group::scoped::Writer writer{ database};

         common::range::for_each( messages,[&]( const message_type& origin){

            auto fetched = database.dequeue( local::request( queue));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type) << "origin.type: " << origin.message.type << " fetched.type; " << fetched.message.at( 0).type;
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.avalible == fetched.message.at( 0).avalible);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

      }


      TEST( casual_queue_group_database, enqueue_one_message_in_transaction)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});


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

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.avalible == fetched.message.at( 0).avalible);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.message.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();

         }

      }


      TEST( casual_queue_group_database, dequeue_one_message_in_transaction)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});


         //common::transaction::ID xid = common::transaction::ID::create();
         auto origin = local::message( queue);
         database.enqueue( origin);

         {
            common::transaction::ID xid = common::transaction::ID::create();

            auto fetched = database.dequeue( local::request( queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);

            database.commit( xid);
         }

         //
         // Should be empty
         //
         EXPECT_TRUE( database.dequeue( local::request( queue)).message.empty());

      }

      TEST( casual_queue_group_database, enqueue_one_message_in_transaction_rollback)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});


         common::transaction::ID xid = common::transaction::ID::create();
         auto origin = local::message( queue, xid);
         database.enqueue( origin);

         database.rollback( xid);

         //
         // Should be empty
         //
         //local::print( database.queues());
         EXPECT_TRUE( database.dequeue( local::request( queue)).message.empty());

      }


      TEST( casual_queue_group_database, enqueue_dequeu_one_message_in_transaction_rollback)
      {
         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});


         auto origin = local::message( queue);

         {
            common::transaction::ID xid = common::transaction::ID::create();

            database.enqueue( origin);
            database.commit( xid);

            //local::print( database.queues());
         }

         {
            common::transaction::ID xid = common::transaction::ID::create();

            auto fetched = database.dequeue( local::request( queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);

            //local::print( database.queues());

            database.rollback( xid);

            //local::print( database.queues());

            auto affected = database.affected();
            EXPECT_TRUE( affected == 1) << "affected: " << affected;
            EXPECT_TRUE( database.dequeue( local::request( queue, xid)).message.empty());
         }

         //
         // Should be in error queue
         //
         {
            //local::print( database.queues());

            common::transaction::ID xid = common::transaction::ID::create();
            auto errorQ = queue;
            errorQ.id = queue.error;

            auto fetched = database.dequeue( local::request( errorQ, xid));

            ASSERT_TRUE( fetched.message.size() == 1) << "errorQ.id; " << errorQ.id << " queue.id: " << queue.id;
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);

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

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
         }

      }

      TEST( casual_queue_group_database, dequeue_100_message_in_transaction)
      {

         auto path = local::file();
         group::Database database( path);
         auto queue = database.create( group::Queue{ "unittest_queue"});

         using message_type = decltype( local::message( queue));


         std::vector< message_type > messages;
         messages.reserve( 100);

         {
            common::transaction::ID xid = common::transaction::ID::create();

            auto count = 0;
            while( count++ < 100)
            {
               auto m = local::message( queue, xid);
               m.message.type.subtype = std::to_string( count);
               database.enqueue( m);
               messages.push_back( std::move( m));
            }

            database.commit( xid);
         }


         common::transaction::ID xid = common::transaction::ID::create();

         common::range::for_each( messages,[&]( const message_type& origin){

            auto fetched = database.dequeue( local::request( queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type) << "origin.type: " << origin.message.type << " fetched.type; " << fetched.message.at( 0).type;
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.avalible == fetched.message.at( 0).avalible);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

         database.commit( xid);

         EXPECT_TRUE( database.dequeue( local::request( queue)).message.empty());


      }



   } // queue
} // casual
