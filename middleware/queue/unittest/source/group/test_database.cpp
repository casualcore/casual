//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"


#include "queue/group/database.h"
#include "queue/common/transform.h"


#include "common/file.h"
#include "common/environment.h"




namespace casual
{
   namespace queue
   {

      namespace local
      {
         namespace
         {

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
               result.message.type = common::buffer::type::binary();

               common::algorithm::copy( common::uuid::string( common::uuid::make()), result.message.payload);


               result.message.available = std::chrono::time_point_cast< std::chrono::microseconds>( common::platform::time::clock::type::now());
               //result.message.timestamp = common::platform::time::clock::type::now();

               return result;
            }

            common::message::queue::dequeue::Request request( const group::Queue& queue, const common::transaction::ID& trid = nullId)
            {
               common::message::queue::dequeue::Request result;

               result.queue = queue.id;
               result.trid = trid;

               return result;
            }

            namespace peek
            {
               common::message::queue::peek::information::Request information( common::strong::queue::id queue)
               {
                  common::message::queue::peek::information::Request result;

                  result.queue = queue;

                  return result;
               }

            } // peek

         } // <unnamed>
      } // local


      TEST( casual_queue_group_database, create_database)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
      }


      TEST( casual_queue_group_database, create_queue)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");

         database.update( { group::Queue{ "unittest_queue"}}, {});

         auto queues = database.queues();

         // there is always an error-queue, and a global error-queue
         ASSERT_TRUE( queues.size() == 3);
         EXPECT_TRUE( queues.at( 0).name == "test_group.group.error") << "queues.at( 0).name: " << queues.at( 0).name;
         EXPECT_TRUE( queues.at( 1).name == "unittest_queue.error");

         EXPECT_TRUE( queues.at( 2).name == "unittest_queue");
         EXPECT_TRUE( queues.at( 2).count == 0) << "count: " << queues.at( 2).count;
         EXPECT_TRUE( queues.at( 2).size == 0);
         EXPECT_TRUE( queues.at( 2).uncommitted == 0);
      }

      TEST( casual_queue_group_database, remove_queue)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");

         auto queue = database.update( {group::Queue{ "unittest_queue"}}, {});

         database.update( {}, { queue.at( 0).id});

         EXPECT_TRUE( database.queues().size() == 1) << database.queues().size();
      }

      TEST( casual_queue_group_database, update_queue)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");

         auto queue = database.update( { group::Queue{ "unittest_queue"}}, {});

         queue.at( 0).name = "foo-bar";

         database.update( { queue.at( 0)}, {});

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 3) << queues.size();
         EXPECT_TRUE( queues.at( 0).name.find_first_of( ".group.error") != std::string::npos);
         EXPECT_TRUE( queues.at( 1).name == "foo-bar.error");
         EXPECT_TRUE( queues.at( 2).name == "foo-bar");
      }


      TEST( casual_queue_group_database, create_queue_on_disc_and_open_again)
      {
         common::unittest::Trace trace;

         common::file::scoped::Path path{
            common::file::name::unique(
               common::environment::directory::temporary() + "/",
               "unittest_queue_server_database.db")
            };

         {
            group::Database database( path, "test_group");

            database.create( group::Queue{ "unittest_queue"});
         }

         {
            //
            // open again
            //
            group::Database database( path, "test_group");

            auto queues = database.queues();

            // there is always a error-queue, and a global error-queue
            ASSERT_TRUE( queues.size() == 3);
            //EXPECT_TRUE( queues.at( 0).name == "unittest_queue_server_database-error-queue") << queues.at( 0).name;
            EXPECT_TRUE( queues.at( 1).name == "unittest_queue.error");
            EXPECT_TRUE( queues.at( 2).name == "unittest_queue");
         }

      }

      TEST( casual_queue_group_database, create_5_queue_on_disc_and_open_again)
      {
         common::unittest::Trace trace;

         common::file::scoped::Path path{
            common::file::name::unique(
               common::environment::directory::temporary() + "/",
               "unittest_queue_server_database.db")
            };

         {
            group::Database database( path, "test_group");

            for( int index = 1; index <= 5; ++index)
            {
               database.create( group::Queue{ "unittest_queue_" + std::to_string( index)});
            }
         }

         {
            //
            // open again
            //
            group::Database database( path, "test_group");

            auto queues = database.queues();

            // there is always a error-queue, and a global error-queue
            ASSERT_TRUE( queues.size() == 5 * 2 + 1);
            //EXPECT_TRUE( queues.at( 0).name == "unittest_queue_server_database-error-queue") << queues.at( 0).name;
            EXPECT_TRUE( queues.at( 1).name == "unittest_queue_1.error") << queues.at( 1).name;
            EXPECT_TRUE( queues.at( 2).name == "unittest_queue_1") << queues.at( 2).name;
         }

      }


      TEST( casual_queue_group_database, create_100_queues)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");

         {
            auto writer = sql::database::scoped::write( database);

            auto count = 0;

            while( count++ < 100)
            {
               database.create( group::Queue{ "unittest_queue" + std::to_string( count)});
            }
         }

         auto queues = database.queues();

         // there is always an error-queue for each queue, and a global error-queue
         ASSERT_TRUE( queues.size() == 201) << "size: " << queues.size();
         EXPECT_TRUE( queues.at( 1).name == "unittest_queue1.error") << queues.at( 200).name;
      }


      TEST( casual_queue_group_database, enqueue_one_message)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto message = local::message( queue);


         EXPECT_NO_THROW({
            EXPECT_TRUE( static_cast< bool>( database.enqueue( message).id));
         });

      }

      TEST( casual_queue_group_database, enqueue_one_message__get_queue_info)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto message = local::message( queue);

         EXPECT_NO_THROW({
            database.enqueue( message);
         });

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 3);
         EXPECT_TRUE( queues.at( 2).name == "unittest_queue");
         EXPECT_TRUE( queues.at( 2).count == 1) << " queues.at( 2).count: " <<  queues.at( 2).count;
         EXPECT_TRUE( queues.at( 2).size == static_cast< common::platform::size::type>( message.message.payload.size()));
      }



      TEST( casual_queue_group_database, enqueue_one_message__get_message_info)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
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
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue));


         ASSERT_TRUE( fetched.message.size() == 1);
         EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
         EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
         EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
         EXPECT_TRUE( origin.message.payload == fetched.message.at( 0).payload);
         EXPECT_TRUE( origin.message.available == fetched.message.at( 0).available);
      }

      TEST( casual_queue_group_database, enqueue_deque__info__expect__count_0__size_0)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "enqueue_deque__info__expect__count_0__size_0"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue));
         EXPECT_TRUE( fetched.message.size() == 1);

         auto queues = database.queues();

         ASSERT_TRUE( queues.at( 2).id == queue.id);
         EXPECT_TRUE( queues.at( 2).count == 0) << " queues.at( 2).count: " <<  queues.at( 2).count;
         EXPECT_TRUE( queues.at( 2).size == 0);
      }


      TEST( casual_queue_group_database, dequeue_message__from_id)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "dequeue_message__from_id"});

         auto origin = local::message( queue);

         database.enqueue( origin);

         auto reqest = local::request( queue);
         reqest.selector.id = origin.message.id;
         auto fetched = database.dequeue( reqest);


         ASSERT_TRUE( fetched.message.size() == 1);
         EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
         EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
         EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
         EXPECT_TRUE( origin.message.available == fetched.message.at( 0).available);
      }

      TEST( casual_queue_group_database, dequeue_message__from_properties)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         origin.message.properties = "some: properties";
         database.enqueue( origin);

         auto reqest = local::request( queue);
         reqest.selector.properties = origin.message.properties;
         auto fetched = database.dequeue( reqest);


         ASSERT_TRUE( fetched.message.size() == 1);
         EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
         EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
         EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
         EXPECT_TRUE( origin.message.payload == fetched.message.at( 0).payload);
         EXPECT_TRUE( origin.message.available == fetched.message.at( 0).available);
      }


      TEST( casual_queue_group_database, dequeue_message__from_non_existent_properties__expect_0_messages)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         origin.message.properties = "some: properties";
         database.enqueue( origin);

         auto reqest = local::request( queue);
         reqest.selector.properties = "some other properties";
         auto fetched = database.dequeue( reqest);

         EXPECT_TRUE( fetched.message.size() == 0);
      }



      TEST( casual_queue_group_database, dequeue_100_message)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         using message_type = decltype( local::message( queue));


         std::vector< message_type > messages;

         auto count = 0;
         while( count++ < 100)
         {
            auto m = local::message( queue);
            m.message.type = std::to_string( count);
            messages.push_back( std::move( m));
         }

         {
            auto writer = sql::database::scoped::write( database);
            common::algorithm::for_each( messages, [&]( const message_type& m){
               database.enqueue( m);});
         }

         auto writer = sql::database::scoped::write( database);

         common::algorithm::for_each( messages,[&]( const message_type& origin){

            auto fetched = database.dequeue( local::request( queue));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type) << "origin.type: " << origin.message.type << " fetched.type; " << fetched.message.at( 0).type;
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.payload == fetched.message.at( 0).payload);
            EXPECT_TRUE( origin.message.available == fetched.message.at( 0).available);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

      }


      TEST( casual_queue_group_database, enqueue_one_message_in_transaction)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});


         common::transaction::ID xid = common::transaction::id::create();
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
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type);
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.available == fetched.message.at( 0).available);
         }

      }


      TEST( casual_queue_group_database, dequeue_one_message_in_transaction)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});


         auto origin = local::message( queue);
         database.enqueue( origin);

         {
            common::transaction::ID xid = common::transaction::id::create();

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

      TEST( casual_queue_group_database, dequeue_group_error_queue__rollback__expect__message_not_moved)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         group::Queue group_queue;
         group_queue.id = database.error();


         auto origin = local::message( group_queue);
         database.enqueue( origin);

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( group_queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);

            database.rollback( xid);
         }

         //
         // Message should still be there
         //
         auto queues = database.queues();

         ASSERT_TRUE( queues.at( 0).id == database.error());
         EXPECT_TRUE( queues.at( 0).count == 1);
         EXPECT_TRUE( queues.at( 0).size == static_cast< common::platform::size::type>( origin.message.payload.size()));

         auto fetched = database.dequeue( local::request( group_queue));

         ASSERT_TRUE( fetched.message.size() == 1);
         EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
      }

      TEST( casual_queue_group_database, deque_in_transaction__info__expect__count_0__size_0)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});


         auto origin = local::message( queue);
         database.enqueue( origin);

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);

            database.commit( xid);
         }

         auto queues = database.queues();

         ASSERT_TRUE( queues.at( 2).id == queue.id);
         EXPECT_TRUE( queues.at( 2).count == 0) << " queues.at( 2).count: " <<  queues.at( 2).count;
         EXPECT_TRUE( queues.at( 2).size == 0);
      }

      TEST( casual_queue_group_database, enqueue_one_message_in_transaction_rollback)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});


         common::transaction::ID xid = common::transaction::id::create();
         auto origin = local::message( queue, xid);
         database.enqueue( origin);

         database.rollback( xid);

         //
         // Should be empty
         //
         //local::print( database.queues());
         EXPECT_TRUE( database.dequeue( local::request( queue)).message.empty());
      }

      TEST( casual_queue_group_database, enqueue_one_message_in_transaction_rollback__info__expect__count_0__size_0)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});


         common::transaction::ID xid = common::transaction::id::create();
         auto origin = local::message( queue, xid);
         database.enqueue( origin);

         database.rollback( xid);

         auto queues = database.queues();

         ASSERT_TRUE( queues.at( 2).id == queue.id);
         EXPECT_TRUE( queues.at( 2).count == 0) << " queues.at( 2).count: " <<  queues.at( 2).count;
         EXPECT_TRUE( queues.at( 2).size == 0);

      }


      TEST( casual_queue_group_database, enqueue_dequeue_one_message_in_transaction_rollback)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});


         auto origin = local::message( queue);

         {
            common::transaction::ID xid = common::transaction::id::create();

            database.enqueue( origin);
            database.commit( xid);

            auto queues = database.queues();

            ASSERT_TRUE( queues.at( 2).id == queue.id);
            EXPECT_TRUE( queues.at( 2).count == 1) << " queues.at( 2).count: " <<  queues.at( 2).count;
            EXPECT_TRUE( queues.at( 2).size ==  static_cast< common::platform::size::type>( origin.message.payload.size()));
         }

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);

            database.rollback( xid);

            auto affected = database.affected();
            EXPECT_TRUE( affected == 1) << "affected: " << affected;
            EXPECT_TRUE( database.dequeue( local::request( queue, xid)).message.empty());
         }

         //
         // Should be in error queue
         //
         {
            auto queues = database.queues();

            ASSERT_TRUE( queues.at( 1).id == queue.error);
            EXPECT_TRUE( queues.at( 1).count == 1) << " queues.at( 2).count: " <<  queues.at( 1).count;
            EXPECT_TRUE( queues.at( 1).size ==  static_cast< common::platform::size::type>( origin.message.payload.size()));

            common::transaction::ID xid = common::transaction::id::create();
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
            auto queues = database.queues();

            ASSERT_TRUE( queues.at( 0).id == database.error());
            EXPECT_TRUE( queues.at( 0).count == 1);
            EXPECT_TRUE( queues.at( 0).size ==  static_cast< common::platform::size::type>( origin.message.payload.size()));

            common::transaction::ID xid = common::transaction::id::create();

            auto errorQ = queue;
            errorQ.id = database.error();

            auto fetched = database.dequeue( local::request( errorQ, xid));
            database.commit( xid);

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
         }

         //
         // All queues should have count = 0 and size = 0
         //
         auto queues = database.queues();

         ASSERT_TRUE( queues.at( 0).id == database.error());
         EXPECT_TRUE( queues.at( 0).count == 0);
         EXPECT_TRUE( queues.at( 0).size == 0);

         ASSERT_TRUE( queues.at( 1).id == queue.error);
         EXPECT_TRUE( queues.at( 1).count == 0);
         EXPECT_TRUE( queues.at( 1).size == 0);

         ASSERT_TRUE( queues.at( 2).id == queue.id);
         EXPECT_TRUE( queues.at( 2).count == 0);
         EXPECT_TRUE( queues.at( 2).size == 0);
      }


      TEST( casual_queue_group_database, dequeue_100_message_in_transaction)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         using message_type = decltype( local::message( queue));


         std::vector< message_type > messages;
         messages.reserve( 100);

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto count = 0;
            while( count++ < 100)
            {
               auto m = local::message( queue, xid);
               m.message.type = std::to_string( count);
               database.enqueue( m);
               messages.push_back( std::move( m));
            }

            database.commit( xid);
         }


         common::transaction::ID xid = common::transaction::id::create();

         common::algorithm::for_each( messages,[&]( const message_type& origin){

            auto fetched = database.dequeue( local::request( queue, xid));

            ASSERT_TRUE( fetched.message.size() == 1);
            EXPECT_TRUE( origin.message.id == fetched.message.at( 0).id);
            EXPECT_TRUE( origin.message.type == fetched.message.at( 0).type) << "origin.type: " << origin.message.type << " fetched.type; " << fetched.message.at( 0).type;
            EXPECT_TRUE( origin.message.reply == fetched.message.at( 0).reply);
            EXPECT_TRUE( origin.message.available == fetched.message.at( 0).available);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

         database.commit( xid);

         EXPECT_TRUE( database.dequeue( local::request( queue)).message.empty());
      }


      TEST( casual_queue_group_database, restore_empty_error_queue__expect_0_affected)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto restored = database.restore( queue.id);
         EXPECT_TRUE( restored == 0) << "restored: " << restored;
      }

      TEST( casual_queue_group_database, restore__1_message_in_error_queue__expect_1_affected)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto message = local::message( queue);
         {
            database.enqueue( message);

            auto trid = common::transaction::id::create();
            database.dequeue( local::request( queue, trid));
            database.rollback( trid);

            auto info = database.peek( local::peek::information( queue.error));
            ASSERT_TRUE( info.messages.size() == 1);
            EXPECT_TRUE( info.messages.at( 0).id == message.message.id);
            EXPECT_TRUE( info.messages.at( 0).origin == queue.id);
            EXPECT_TRUE( info.messages.at( 0).queue == queue.error);
         }

         auto restored = database.restore( queue.id);

         EXPECT_TRUE( restored == 1) << "restored: " << restored;
         {
            auto reply = database.dequeue( local::request(queue));
            ASSERT_TRUE( reply.message.size() == 1) << "reply: " << reply;
            EXPECT_TRUE( reply.message.at( 0).payload == message.message.payload);
         }
      }


      TEST( casual_queue_group_database, pending_empty)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         auto pending = database.pending();

         EXPECT_TRUE( pending.empty());
      }

      TEST( casual_queue_group_database, pending_set__expect_no_throw)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         EXPECT_NO_THROW(
            database.pending_set( queue.id, 0);
         );
      }

      TEST( casual_queue_group_database, pending_add___expect_queue_pending_1)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         database.pending_add( queue.id);

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 3);
         EXPECT_TRUE( queues.at( 2).id == queue.id);
         EXPECT_TRUE( queues.at( 2).pending == 1);
      }

      TEST( casual_queue_group_database, pending_add_check_pending__expect_no_pending)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         EXPECT_NO_THROW(
            database.pending_add( queue.id);
         );

         EXPECT_TRUE( database.get_pending().empty());
      }

      TEST( casual_queue_group_database, pending_add_twice__expect_no_throw)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         EXPECT_NO_THROW(
            database.pending_add( queue.id);
            database.pending_add( queue.id);
         );
      }

      TEST( casual_queue_group_database, pending_enqueue_one_message__expect_pending_result)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         database.pending_add( queue.id);

         database.enqueue( local::message( queue));

         auto pending = database.get_pending();

         ASSERT_TRUE( pending.size() == 1) << "queue: " << database.queues().at( 2);
         EXPECT_TRUE( pending.at( 0).id == queue.id);
         EXPECT_TRUE( pending.at( 0).count == 1);
      }

      TEST( casual_queue_group_database, pending_enqueue_two_message__expect_pending_result)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto queue = database.create( group::Queue{ "unittest_queue"});

         database.pending_add( queue.id);

         database.enqueue( local::message( queue));
         database.enqueue( local::message( queue));

         auto pending = database.get_pending();

         ASSERT_TRUE( ! pending.empty());
         EXPECT_TRUE( pending.at( 0).id == queue.id);
         EXPECT_TRUE( pending.at( 0).count == 2);
      }

      TEST( casual_queue_group_database, pending_q1_enqueue_q2___expect_0_pending_q1)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto q1 = database.create( group::Queue{ "q1"});
         auto q2 = database.create( group::Queue{ "q2"});

         database.pending_add( q1.id);

         database.enqueue( local::message( q2));

         auto pending = database.get_pending();
         ASSERT_TRUE( pending.empty());
      }

      TEST( casual_queue_group_database, pending_q1_enqueue_q1_q2___expect_1_pending_q1)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto q1 = database.create( group::Queue{ "q1"});
         auto q2 = database.create( group::Queue{ "q2"});

         database.pending_add( q1.id);

         database.enqueue( local::message( q1));
         database.enqueue( local::message( q2));

         auto pending = database.get_pending();

         ASSERT_TRUE( ! pending.empty());
         EXPECT_TRUE( pending.at( 0).id == q1.id);
         EXPECT_TRUE( pending.at( 0).count == 1);
      }

      TEST( casual_queue_group_database, pending_q1_g2_enqueue_q1_q2___expect__pending_1_q1__1_g2)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Database database( path, "test_group");
         auto q1 = database.create( group::Queue{ "q1"});
         auto q2 = database.create( group::Queue{ "q2"});

         database.pending_add( q1.id);
         database.pending_add( q2.id);

         database.enqueue( local::message( q1));
         database.enqueue( local::message( q2));

         auto pending = database.get_pending();

         ASSERT_TRUE( pending.size() == 2);
         EXPECT_TRUE( pending.at( 0).id == q1.id);
         EXPECT_TRUE( pending.at( 0).count == 1);
         EXPECT_TRUE( pending.at( 1).id == q2.id);
         EXPECT_TRUE( pending.at( 1).count == 1);
      }


      namespace local
      {
         namespace
         {
            namespace dequeue
            {
               common::message::queue::dequeue::Request request( const group::Queue& queue, bool block = true, const common::process::Handle& process = common::process::handle())
               {
                  common::message::queue::dequeue::Request result;

                  result.process = process;
                  result.queue = queue.id;
                  result.block = block;

                  return result;
               }
            } // dequeue

            namespace enqueue
            {
               common::message::queue::enqueue::Request request( const group::Queue& queue, const common::process::Handle& process = common::process::handle())
               {
                  common::message::queue::enqueue::Request result;

                  result.process = process;
                  result.queue = queue.id;
                  result.message.payload = { 'a', 'b'};

                  return result;
               }
            } // dequeue


            namespace memory 
            {
               struct Database : group::Database 
               {
                  Database() : group::Database{ ":memory:", "test_group"},
                     q1( group::Database::create( group::Queue{ "q1"})),
                     q2( group::Database::create( group::Queue{ "q2"}))
                  {}

                  group::Queue q1;
                  group::Queue q2;
               };

               Database database()
               {
                  return {};
               }
            } // memory 

         } // <unnamed>
      } // local
      TEST( casual_queue_group_database, db_empty__pending_empty__expect_no_pending)
      {
         common::unittest::Trace trace;

         auto database = local::memory::database();

         auto pending = database.pending();

         EXPECT_TRUE( pending.empty());
      }

      TEST( casual_queue_group_pending, block_dequeue_q1__enqueue_q1___expect_1_consumed_pending)
      {
         common::unittest::Trace trace;

         auto database = local::memory::database();

         {
            auto result = database.dequeue( local::dequeue::request( database.q1));
            EXPECT_TRUE( result.message.empty());
         }

         {
            database.enqueue( local::enqueue::request( database.q1));
         }

         auto pending = database.pending();
         ASSERT_TRUE( pending.size() == 1);

         // we expect to have consumed the pending
         EXPECT_TRUE( database.pending().empty());
         
         {
            auto p = database.get_pending();
            EXPECT_TRUE( p.empty()) << "p.at( 0).count: " << p.at( 0).count;
         }
      }

      TEST( casual_queue_group_pending, block_dequeue_q1__enqueue_q2___expect_0_consumed_pending)
      {
         common::unittest::Trace trace;

         auto database = local::memory::database();

         {
            auto result = database.dequeue( local::dequeue::request( database.q1));
            EXPECT_TRUE( result.message.empty());
         }

         database.enqueue( local::enqueue::request( database.q2));

         auto pending = database.pending();
         EXPECT_TRUE( pending.empty());
      }

      TEST( casual_queue_group_pending, block_dequeue_q1_g2__enqueue_g1_q2___expect_2_consumed_pending)
      {
         common::unittest::Trace trace;

         auto database = local::memory::database();

         {
            auto result = database.dequeue( local::dequeue::request( database.q1));
            EXPECT_TRUE( result.message.empty());
         }

         {
            auto result = database.dequeue( local::dequeue::request( database.q2));
            EXPECT_TRUE( result.message.empty());
         }

         database.enqueue( local::enqueue::request( database.q1));
         database.enqueue( local::enqueue::request( database.q2));

         auto pending = database.pending();
         EXPECT_TRUE( pending.size() == 2);

         EXPECT_TRUE( database.get_pending().empty());   
      }

      TEST( casual_queue_group_pending, block_dequeue_q1_g1__enqueue_g1_q2___expect_1_consumed_pending)
      {
         common::unittest::Trace trace;

         auto database = local::memory::database();

         {
            auto result = database.dequeue( local::dequeue::request( database.q1));
            EXPECT_TRUE( result.message.empty());
         }

         {
            auto result = database.dequeue( local::dequeue::request( database.q1));
            EXPECT_TRUE( result.message.empty());
         }

         database.enqueue( local::enqueue::request( database.q1));
         database.enqueue( local::enqueue::request( database.q2));

         auto pending = database.pending();
         EXPECT_TRUE( pending.size() == 1);

         EXPECT_TRUE( database.get_pending().size() == 1);   
      }


      TEST( casual_queue_group, expect_version_1_0)
      {
         common::unittest::Trace trace;

         auto database = local::memory::database();

         auto version = database.version();

         EXPECT_TRUE( version.major == 1);
         EXPECT_TRUE( version.minor == 0);
      }

   } // queue
} // casual
