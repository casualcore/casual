//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/serialize.h"

#include "queue/manager/transform.h"
#include "queue/manager/state.h"


namespace casual
{
   using namespace common;
   namespace queue::manager
   {
      TEST( queue_manager_transform, empty)
      {
         common::unittest::Trace trace;

         constexpr std::string_view group_replies = R"(
alias: A
)";
         auto reply = unittest::serialize::create::value< ipc::message::group::state::Reply>( "yaml", group_replies);
         EXPECT_TRUE( reply.alias == "A");
      }

      TEST( queue_manager_transform, state_to_model)
      {
         common::unittest::Trace trace;

         constexpr auto group_replies = R"(
-  alias: A
   process:
      pid: 42
   queuebase: ':memory:'
   note: note-A
   queues:
      -  id: 10
         name: a.error

      -  id: 11
         name: a
         retry:
            count: 10
            delay: 1000000000
         error: 10
         metric:
            count: 5
            size: 1024
            uncommitted: 1
            last: 2000000000
            dequeued: 20
            enqueued: 21
         created: 3000000000

      -  id: 12
         name: b.error

      -  id: 13
         name: b
         retry:
            count: 1
            delay: 4000000000
         error: 12
         metric:
            count: 1
            size: 444
            uncommitted: 0
            last: 5000000000
            dequeued: 10
            enqueued: 11
         created: 6000000000
)";


         constexpr auto forward_replies = R"(
-  alias: F1
   process:
      pid: 43
   note: note-F1
   services:
      -  alias: fa
         note: note-fa
         source: a
         target:
            service: foo
         instances:
            configured: 5
            running: 4
         reply:
            queue: b
            delay: 1000000000
         metric:
            commit:
               count: 10
               last: 2000000000
            rollback:
               count: 3
               last: 3000000000
)";

         auto model = transform::model::state( 
            unittest::serialize::create::value< std::vector< ipc::message::group::state::Reply>>( "yaml", group_replies),
            unittest::serialize::create::value< std::vector< ipc::message::forward::group::state::Reply>>( "yaml", forward_replies)
         );

         constexpr auto expected_model = R"(
groups:
   -  name: A
      process:
         pid: 42
      queuebase: ':memory:'
      note: note-A
queues:
   -  id: 10
      group: 42
      name: a.error

   -  id: 11
      group: 42
      name: a
      retry:
         count: 10
         delay: 1000000000
      error: 10
      count: 5
      size: 1024
      uncommitted: 1
      last: 2000000000 
      metric:
         dequeued: 20
         enqueued: 21
      created: 3000000000

   -  id: 12
      group: 42
      name: b.error

   -  id: 13
      group: 42
      name: b
      retry:
         count: 1
         delay: 4000000000
      error: 12
      count: 1
      size: 444
      uncommitted: 0
      last: 5000000000
      metric:
         dequeued: 10
         enqueued: 11
      created: 6000000000
forward:
   groups:
      -  alias: F1
         process:
            pid: 43
         note: note-F1
   services:
      -  group: 43
         alias: fa
         source: a
         note: note-fa
         target:
            service: foo
         instances:
            configured: 5
            running: 4
         reply:
            queue: b
            delay: 1000000000
         metric:
            commit:
               count: 10
               last: 2000000000
            rollback:
               count: 3
               last: 3000000000
         
)";


         auto expected = unittest::serialize::create::value< admin::model::State>( "yaml", expected_model);


         EXPECT_TRUE( unittest::serialize::hash( model) == unittest::serialize::hash( expected))
            << "   " << CASUAL_NAMED_VALUE( model)
            << '\n' << CASUAL_NAMED_VALUE( expected);
      }


   } // queue::manager
} // casual
