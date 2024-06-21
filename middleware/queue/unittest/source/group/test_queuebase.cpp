//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "queue/group/queuebase.h"

#include "common/file.h"
#include "common/environment.h"
#include "common/chronology.h"
#include "common/buffer/type.h"
#include "common/unittest/file.h"


namespace casual
{
   namespace queue::group
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

            auto message( const queuebase::Queue& queue, const common::transaction::ID& trid = nullId)
            {
               ipc::message::group::enqueue::Request result;

               result.queue = queue.id;
               result.trid = trid;

               result.message.id = common::uuid::make();
               result.message.attributes.reply = "someQueue";
               result.message.payload.type = common::buffer::type::binary;

               result.message.payload.data = common::unittest::random::binary( 64);

               return result;
            }

            template< typename Q>
            auto request( const Q& queue, const common::transaction::ID& trid = nullId)
            {
               ipc::message::group::dequeue::Request result;

               result.queue = queue.id;
               result.trid = trid;

               return result;
            }

            namespace peek
            {
               auto information( common::strong::queue::id queue)
               {
                  ipc::message::group::message::meta::peek::Request result;

                  result.queue = queue;
                  

                  return result;
               }

            } // peek

            std::optional< ipc::message::group::state::Queue> get_queue( group::Queuebase& queuebase, common::strong::queue::id id)
            {
               auto result = queuebase.queues();
               auto found = common::algorithm::find( result, id);
               if( found)
                  return { std::move( *found)};
               
               return {};
            }

         } // <unnamed>
      } // local


      TEST( casual_queue_group_database, create_database)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
      }


      TEST( casual_queue_group_database, create_queue)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);

         database.update( { queuebase::Queue{ "unittest_queue"}}, {});

         auto queues = database.queues();

         // there is always an error-queue
         ASSERT_TRUE( queues.size() == 2);
         EXPECT_TRUE( queues.at( 0).name == "unittest_queue.error");

         EXPECT_TRUE( queues.at( 1).name == "unittest_queue");
         EXPECT_TRUE( queues.at( 1).metric.count == 0) << "count: " << queues.at( 2).metric.count;
         EXPECT_TRUE( queues.at( 1).metric.size == 0);
         EXPECT_TRUE( queues.at( 1).metric.uncommitted == 0);
      }

      TEST( casual_queue_group_database, remove_queue)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);

         auto queue = database.update( {queuebase::Queue{ "unittest_queue"}}, {});

         database.update( {}, { queue.at( 0).id});

         EXPECT_TRUE( database.queues().empty()) << database.queues().size();
      }

      TEST( casual_queue_group_database, update_queue)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);

         auto queue = database.update( { queuebase::Queue{ "unittest_queue"}}, {});

         queue.at( 0).name = "foo-bar";

         database.update( { queue.at( 0)}, {});

         auto queues = database.queues();

         ASSERT_TRUE( queues.size() == 2) << queues.size();
         EXPECT_TRUE( queues.at( 0).name == "foo-bar.error");
         EXPECT_TRUE( queues.at( 1).name == "foo-bar");
      }

      namespace local
      {
         namespace
         {
            auto path()
            {
               common::file::scoped::Path result{ common::environment::directory::temporary()};
               result /= common::uuid::string( common::uuid::make());
               result += "unittest_queue_server_database.db";
               return result;
            }
         } //
      } // local


      TEST( casual_queue_group_database, create_queue_on_disc_and_open_again)
      {
         common::unittest::Trace trace;

         auto path = local::path();
         
         {
            group::Queuebase database( path);
            database.create( queuebase::Queue{ "unittest_queue"});
         }

         {
            // open again
            group::Queuebase database( path);

            auto queues = database.queues();

            // there is always a error-queue
            ASSERT_TRUE( queues.size() == 2);
            //EXPECT_TRUE( queues.at( 0).name == "unittest_queue_server_database-error-queue") << queues.at( 0).name;
            EXPECT_TRUE( queues.at( 0).name == "unittest_queue.error");
            EXPECT_TRUE( queues.at( 1).name == "unittest_queue");
         }

      }

      TEST( casual_queue_group_database, create_5_queue_on_disc_and_open_again)
      {
         common::unittest::Trace trace;

         auto path = local::path();
         
         {
            group::Queuebase database( path);

            for( int index = 1; index <= 5; ++index)
            {
               database.create( queuebase::Queue{ "unittest_queue_" + std::to_string( index)});
            }
         }

         {
            // open again
            group::Queuebase database( path);

            auto queues = database.queues();

            // there is always a error-queue, and a global error-queue
            ASSERT_TRUE( queues.size() == 5 * 2);
            //EXPECT_TRUE( queues.at( 0).name == "unittest_queue_server_database-error-queue") << queues.at( 0).name;
            EXPECT_TRUE( queues.at( 0).name == "unittest_queue_1.error") << queues.at( 1).name;
            EXPECT_TRUE( queues.at( 1).name == "unittest_queue_1") << queues.at( 2).name;
         }

      }


      TEST( casual_queue_group_database, create_100_queues)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase queuebase( path);

         common::algorithm::for_n< 100>( [&queuebase]( auto index)
         {
            queuebase.create( queuebase::Queue{ "unittest_queue" + std::to_string( index)});
         });
         queuebase.persist();


         auto queues = queuebase.queues();

         // there is always an error-queue for each queue
         ASSERT_TRUE( queues.size() == 200) << "size: " << queues.size();
         EXPECT_TRUE( queues.at( 0).name == "unittest_queue0.error") << queues.at( 0).name;
      }


      TEST( casual_queue_group_database, enqueue_one_message)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto message = local::message( queue);


         EXPECT_NO_THROW({
            EXPECT_TRUE( static_cast< bool>( database.enqueue( message).id));
         });

      }

      TEST( casual_queue_group_database, enqueue_one_message__get_queue_info)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto message = local::message( queue);

         EXPECT_NO_THROW({
            database.enqueue( message);
         });

         auto info = local::get_queue( database, queue.id).value();

         EXPECT_TRUE( info.name == "unittest_queue");
         EXPECT_TRUE( info.metric.count == 1) << "info.metric.count: " << info.metric.count;
         EXPECT_TRUE( info.metric.size == static_cast< platform::size::type>( message.message.payload.data.size()));
      }



      TEST( casual_queue_group_database, enqueue_one_message__get_message_info)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto message = local::message( queue);

         EXPECT_NO_THROW({
            database.enqueue( message);
         });

         auto messages = database.meta( queue.id);

         ASSERT_TRUE( messages.size() == 1);
         EXPECT_TRUE( messages.at( 0).id  == message.message.id);
      }


      TEST( casual_queue_group_database, dequeue_one_message)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue), platform::time::clock::type::now());


         ASSERT_TRUE( fetched.message);
         EXPECT_TRUE( origin.message.id == fetched.message->id);
         EXPECT_TRUE( origin.message.attributes.reply == fetched.message->attributes.reply);
         EXPECT_TRUE( origin.message.payload.type == fetched.message->payload.type);
         EXPECT_TRUE( origin.message.payload.data == fetched.message->payload.data);
         EXPECT_TRUE( origin.message.attributes.available == fetched.message->attributes.available);
      }

      TEST( casual_queue_group_database, enqueue_deque__info__expect__count_0__size_0)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "enqueue_deque__info__expect__count_0__size_0"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue), platform::time::clock::type::now());
         EXPECT_TRUE( fetched.message);

         auto info = local::get_queue( database, queue.id).value();

         EXPECT_TRUE( info.metric.count == 0) << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.metric.size == 0);
      }

      TEST( casual_queue_group_database, enqueue_deque__info__expect_metric_to_reflect)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "enqueue_deque__info__expect_metric_to_reflect"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue), platform::time::clock::type::now());
         EXPECT_TRUE( fetched.message);

         auto info = local::get_queue( database, queue.id).value();

         EXPECT_TRUE( info.metric.enqueued  == 1) << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.metric.dequeued  == 1) << CASUAL_NAMED_VALUE( info);
      }

      TEST( casual_queue_group_database, enqueue_deque__metric_reset__info__expect_no_metric)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "enqueue_deque__info__expect_metric_to_reflect"});

         auto origin = local::message( queue);
         database.enqueue( origin);

         auto fetched = database.dequeue( local::request( queue), platform::time::clock::type::now());
         EXPECT_TRUE( fetched.message);

         database.metric_reset( { queue.id });

         auto info = local::get_queue( database, queue.id).value();

         EXPECT_TRUE( info.metric.enqueued  == 0) << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.metric.dequeued  == 0) << CASUAL_NAMED_VALUE( info);
      }


      TEST( casual_queue_group_database, dequeue_message__from_id)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "dequeue_message__from_id"});

         auto origin = local::message( queue);

         database.enqueue( origin);

         auto request = local::request( queue);
         request.selector.id = origin.message.id;
         auto fetched = database.dequeue( request, platform::time::clock::type::now());


         ASSERT_TRUE( fetched.message);
         EXPECT_TRUE( origin.message.id == fetched.message->id);
         EXPECT_TRUE( origin.message.payload.type == fetched.message->payload.type);
         EXPECT_TRUE( origin.message.attributes.reply == fetched.message->attributes.reply);
         EXPECT_TRUE( origin.message.attributes.available == fetched.message->attributes.available);
      }

      TEST( casual_queue_group_database, dequeue_message__from_properties)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         origin.message.attributes.properties = "some: properties";
         database.enqueue( origin);

         auto request = local::request( queue);
         request.selector.properties = origin.message.attributes.properties;
         auto fetched = database.dequeue( request, platform::time::clock::type::now());

         ASSERT_TRUE( fetched.message);
         EXPECT_TRUE( origin.message.id == fetched.message->id);
         EXPECT_TRUE( origin.message.payload.type == fetched.message->payload.type);
         EXPECT_TRUE( origin.message.payload.data == fetched.message->payload.data);
         EXPECT_TRUE( origin.message.attributes.reply == fetched.message->attributes.reply);
         EXPECT_TRUE( origin.message.attributes.available == fetched.message->attributes.available);
      }


      TEST( casual_queue_group_database, dequeue_message__from_non_existent_properties__expect_0_messages)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto origin = local::message( queue);
         origin.message.attributes.properties = "some: properties";
         database.enqueue( origin);

         auto request = local::request( queue);
         request.selector.properties = "some other properties";
         auto fetched = database.dequeue( request, platform::time::clock::type::now());

         EXPECT_TRUE( ! fetched.message);
      }



      TEST( casual_queue_group_database, dequeue_100_message)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto messages = common::algorithm::generate_n< 100>( [&]( auto index)
         {
            auto message = local::message( queue);
            message.message.payload.type = std::to_string( index);
            return message;
         });

         common::algorithm::for_each( messages, [&]( auto& message)
         {
            database.enqueue( message);
         });

         database.persist();

         common::algorithm::for_each( messages,[&]( auto& origin)
         {
            auto fetched = database.dequeue( local::request( queue), platform::time::clock::type::now());

            ASSERT_TRUE( fetched.message);
            EXPECT_TRUE( origin.message.id == fetched.message->id);
            EXPECT_TRUE( origin.message.payload == fetched.message->payload) << CASUAL_NAMED_VALUE( origin.message.payload) << " fetched; " << CASUAL_NAMED_VALUE( fetched.message->payload);
            EXPECT_TRUE( origin.message.attributes == fetched.message->attributes);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

         auto info = local::get_queue( database, queue.id).value();

         EXPECT_TRUE( info.metric.enqueued  == 100) << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.metric.dequeued  == 100) << CASUAL_NAMED_VALUE( info);

      }


      TEST( casual_queue_group_database, enqueue_one_message_in_transaction)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         common::transaction::ID xid = common::transaction::id::create();
         auto origin = local::message( queue, xid);

         {
            database.enqueue( origin);

            // Message should not be available.
            EXPECT_TRUE( ! database.dequeue( local::request( queue), platform::time::clock::type::now()).message);
            // Not even within the same transaction
            EXPECT_TRUE( ! database.dequeue( local::request( queue, xid), platform::time::clock::type::now()).message);

            database.commit( xid);
         }

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( queue, xid), platform::time::clock::type::now());

            ASSERT_TRUE( fetched.message) << CASUAL_NAMED_VALUE( fetched) << "\n" << CASUAL_NAMED_VALUE( database.queues());
            EXPECT_TRUE( origin.message.id == fetched.message->id);
            EXPECT_TRUE( origin.message.payload.type == fetched.message->payload.type);
            EXPECT_TRUE( origin.message.attributes.reply == fetched.message->attributes.reply);
            EXPECT_TRUE( origin.message.attributes.available == fetched.message->attributes.available);
         }

      }


      TEST( casual_queue_group_database, dequeue_one_message_in_transaction)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});


         auto origin = local::message( queue);
         database.enqueue( origin);

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( queue, xid), platform::time::clock::type::now());

            ASSERT_TRUE( fetched.message);
            EXPECT_TRUE( origin.message.id == fetched.message->id);

            database.commit( xid);
         }

         // Should be empty
         EXPECT_TRUE( ! database.dequeue( local::request( queue), platform::time::clock::type::now()).message);
      }


      TEST( casual_queue_group_database, deque_in_transaction__info__expect__count_0__size_0)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});


         auto origin = local::message( queue);
         database.enqueue( origin);

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( queue, xid), platform::time::clock::type::now());

            ASSERT_TRUE( fetched.message);
            EXPECT_TRUE( origin.message.id == fetched.message->id);

            database.commit( xid);
         }

         auto info = local::get_queue( database, queue.id).value();

         EXPECT_TRUE( info.metric.count == 0) << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.metric.size == 0);
      }

      TEST( casual_queue_group_database, enqueue_one_message_in_transaction_rollback)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});


         common::transaction::ID xid = common::transaction::id::create();
         auto origin = local::message( queue, xid);
         database.enqueue( origin);

         database.rollback( xid);

         // Should be empty
         //local::print( database.queues());
         EXPECT_TRUE( ! database.dequeue( local::request( queue), platform::time::clock::type::now()).message);
      }

      TEST( casual_queue_group_database, enqueue_one_message_in_transaction_rollback__info__expect__count_0__size_0)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});


         common::transaction::ID xid = common::transaction::id::create();
         auto origin = local::message( queue, xid);
         database.enqueue( origin);

         database.rollback( xid);

         auto info = local::get_queue( database, queue.id).value();

         EXPECT_TRUE( info.metric.count == 0) << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.metric.size == 0);

      }


      TEST( casual_queue_group_database, enqueue_dequeue_one_message_in_transaction_rollback)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});


         auto origin = local::message( queue);

         {
            common::transaction::ID trid = common::transaction::id::create();
            origin.trid = trid;

            database.enqueue( origin);
            database.commit( trid);

            auto info = local::get_queue( database, queue.id).value();

            EXPECT_TRUE( info.metric.count == 1) << CASUAL_NAMED_VALUE( info);
            EXPECT_TRUE( info.metric.size ==  static_cast< platform::size::type>( origin.message.payload.data.size()));
         }

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( queue, xid), platform::time::clock::type::now());

            ASSERT_TRUE( fetched.message);
            EXPECT_TRUE( origin.message.id == fetched.message->id);

            database.rollback( xid);

            auto affected = database.affected();
            EXPECT_TRUE( affected == 1) << "affected: " << affected;
            EXPECT_TRUE( ! database.dequeue( local::request( queue, xid), platform::time::clock::type::now()).message);
         }

         // Should be in error queue
         {
            auto error = local::get_queue( database, queue.error).value();
         
            EXPECT_TRUE( error.metric.count == 1) << " queues.at( 1).count: " <<  error.metric.count;
            EXPECT_TRUE( error.metric.size ==  static_cast< platform::size::type>( origin.message.payload.data.size()));

            common::transaction::ID xid = common::transaction::id::create();
         
            auto fetched = database.dequeue( local::request( error, xid), platform::time::clock::type::now());

            ASSERT_TRUE( fetched.message) << CASUAL_NAMED_VALUE( error);
            EXPECT_TRUE( origin.message.id == fetched.message->id);

            database.rollback( xid);
         }

         // Should still be in error queue
         {
            auto error = local::get_queue( database, queue.error).value();
         
            EXPECT_TRUE( error.metric.count == 1) << " queues.at( 1).count: " <<  error.metric.count;
            EXPECT_TRUE( error.metric.size == static_cast< platform::size::type>( origin.message.payload.data.size()));

            common::transaction::ID xid = common::transaction::id::create();

            auto fetched = database.dequeue( local::request( error, xid), platform::time::clock::type::now());
            database.commit( xid);

            ASSERT_TRUE( fetched.message);
            EXPECT_TRUE( origin.message.id == fetched.message->id);
         }

         // All queues should have count = 0 and size = 0
         auto is_empty = []( auto& queue){ return queue.metric.count == 0 && queue.metric.size == 0;};
         EXPECT_TRUE( common::algorithm::all_of( database.queues(), is_empty));
      }

      TEST( casual_queue_group_database, dequeue_100_message_in_transaction)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         using message_type = decltype( local::message( queue));


         std::vector< message_type > messages;
         messages.reserve( 100);

         {
            common::transaction::ID xid = common::transaction::id::create();

            auto count = 0;
            while( count++ < 100)
            {
               auto m = local::message( queue, xid);
               m.message.payload.type = std::to_string( count);
               database.enqueue( m);
               messages.push_back( std::move( m));
            }

            database.commit( xid);
         }


         common::transaction::ID xid = common::transaction::id::create();

         common::algorithm::for_each( messages,[&]( const message_type& origin){

            auto fetched = database.dequeue( local::request( queue, xid), platform::time::clock::type::now());

            ASSERT_TRUE( fetched.message);
            EXPECT_TRUE( origin.message.id == fetched.message->id);
            EXPECT_TRUE( origin.message.payload.type == fetched.message->payload.type) << "origin.type: " << origin.message.payload.type << " fetched.type; " << fetched.message->payload.type;
            EXPECT_TRUE( origin.message.attributes.reply == fetched.message->attributes.reply);
            EXPECT_TRUE( origin.message.attributes.available == fetched.message->attributes.available);
            //EXPECT_TRUE( origin.message.timestamp <= fetched.timestamp) << "origin: " << origin.timestamp.time_since_epoch().count() << " fetched: " << fetched.timestamp.time_since_epoch().count();
         });

         database.commit( xid);

         EXPECT_TRUE( ! database.dequeue( local::request( queue), platform::time::clock::type::now()).message);
      }


      TEST( casual_queue_group_database, restore_empty_error_queue__expect_0_affected)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto restored = database.restore( queue.id);
         EXPECT_TRUE( restored == 0) << "restored: " << restored;
      }

      TEST( casual_queue_group_database, pre_statements)
      {
         common::unittest::Trace trace;

         auto directory = common::unittest::directory::temporary::Scoped{};

         auto content = common::unittest::file::content( directory.path() / "a.pre.statements", R"(
PRAGMA foreign_keys;
PRAGMA journal_mode;
)");

         EXPECT_NO_THROW( 
            group::Queuebase database( directory.path() / "a.qb"))
         ;
      }

      TEST( casual_queue_group_database, post_statements)
      {
         common::unittest::Trace trace;

         auto directory = common::unittest::directory::temporary::Scoped{};

         auto content = common::unittest::file::content( directory.path() / "a.post.statements", R"(
PRAGMA foreign_keys;
PRAGMA journal_mode;
)");

         EXPECT_NO_THROW( 
            group::Queuebase database( directory.path() / "a.qb"))
         ;
      }

      TEST( casual_queue_group_database, restore__1_message_in_error_queue__expect_1_affected)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue"});

         auto message = local::message( queue);
         {
            database.enqueue( message);

            auto trid = common::transaction::id::create();
            database.dequeue( local::request( queue, trid), platform::time::clock::type::now());
            database.rollback( trid);

            auto info = database.peek( local::peek::information( queue.error));
            ASSERT_TRUE( info.messages.size() == 1);
            EXPECT_TRUE( info.messages.at( 0).id == message.message.id);
            EXPECT_TRUE( info.messages.at( 0).origin == queue.id);
            EXPECT_TRUE( info.messages.at( 0).queue == queue.error);
         }

         auto restored = database.restore( queue.id);

         EXPECT_TRUE( restored == 1) << CASUAL_NAMED_VALUE( restored);
         {
            auto reply = database.dequeue( local::request(queue), platform::time::clock::type::now());
            ASSERT_TRUE( reply.message) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.message->payload == message.message.payload);
         }
      }

      TEST( casual_queue_group_database, retry_count_3_delay_1h__rollback___expect_unabvailable)
      {
         common::unittest::Trace trace;

         // start time so we can make sure that available will be later than this + 1h
         auto start = platform::time::clock::type::now();

         auto path = local::file();
         group::Queuebase database( path);
         auto queue = database.create( queuebase::Queue{ "unittest_queue", queuebase::queue::Retry{ 3, std::chrono::hours{ 1}}});
         
         {
            auto message = local::message( queue);
            database.enqueue( message);

            auto trid = common::transaction::id::create();
            database.dequeue( local::request( queue, trid), platform::time::clock::type::now());
            database.rollback( trid);

            auto info = database.peek( local::peek::information( queue.id));
            ASSERT_TRUE( info.messages.size() == 1);
            EXPECT_TRUE( info.messages.at( 0).id == message.message.id);
            EXPECT_TRUE( info.messages.at( 0).origin == queue.id);
            EXPECT_TRUE( info.messages.at( 0).queue == queue.id);
            // duration is converted to us, and could be rounded so it's less then _start_ + 1h, we use 59min to be safe.
            EXPECT_TRUE( info.messages.at( 0).available > start + std::chrono::minutes{ 59}) 
               << "available: " << common::chronology::utc::offset( info.messages.at( 0).available) 
               << "\nlimit: " << common::chronology::utc::offset( start + std::chrono::hours{ 1});
            // check that we've not over-delayed it ( 2s should be safe on really slow machines)
            EXPECT_TRUE( info.messages.at( 0).available < start + std::chrono::hours{ 1} + std::chrono::seconds{ 2});
         }

         // message is not available until next hour
         EXPECT_TRUE( ! database.dequeue( local::request( queue), platform::time::clock::type::now()).message);
      }
      

      TEST( casual_queue_group, expect_version_3_0)
      {
         common::unittest::Trace trace;

         auto path = local::file();
         group::Queuebase database( path);

         auto version = database.version();

         EXPECT_TRUE( version.major == 3);
         EXPECT_TRUE( version.minor == 0);
      }

   } // queue::group
} // casual
