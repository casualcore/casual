//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/task.h"

#include "common/message/type.h"
#include "common/message/event.h"
#include "common/array.h"

namespace casual
{
   using namespace common;

   namespace local
   {
      namespace
      {
         namespace message
         {
            using A = common::message::basic_request< common::message::Type::cli_payload>;
            using B = common::message::basic_request< common::message::Type::cli_pipe_done>;
            using C = common::message::basic_request< common::message::Type::cli_pipe_error_fatal>;
            using D = common::message::basic_request< common::message::Type::cli_queue_message_id>;
            using E = common::message::basic_request< common::message::Type::cli_transaction_associated>;

         } // message

         constexpr auto action( task::unit::action::Outcome outcome = task::unit::action::Outcome::success)
         {
            return [ outcome]( task::unit::id id){ return outcome;};
         }
      } // <unnamed>
   } // local


   TEST( common_task_unit, action)
   {
      common::unittest::Trace trace;
      
      auto action = 0;
      auto invokes = 0;

      auto unit = task::create::unit( 
         [ &action]( task::unit::id id){ ++action; return task::unit::action::Outcome::success;},
         [ &invokes]( task::unit::id id, const local::message::A&){ ++invokes; return task::unit::Dispatch::pending;},
         [ &invokes]( task::unit::id id, const local::message::B&){ ++invokes; return task::unit::Dispatch::pending;}
      );
      
      EXPECT_TRUE( unit( local::message::A{}) == task::unit::Dispatch::pending);
      EXPECT_TRUE( unit( local::message::B{}) == task::unit::Dispatch::pending);
      
      EXPECT_TRUE( action = 1);
      EXPECT_TRUE( invokes == 2);
   }


   TEST( common_task_unit, action_description)
   {
      common::unittest::Trace trace;
      
      std::vector< std::string_view> descriptions;

      auto action = task::create::action( "A", [ &descriptions]( task::unit::id id, std::string_view description){ descriptions.push_back( description); return task::unit::action::Outcome::success;});

      auto unit = task::create::unit( 
         std::move( action),
         [ &descriptions]( task::unit::id id, const local::message::A&, const std::string& description){ descriptions.push_back( description); return task::unit::Dispatch::pending;},
         [ &descriptions]( task::unit::id id, const local::message::B&, std::string_view description){ descriptions.push_back( description); return task::unit::Dispatch::pending;}
      );

      // invoke the unit, and trigger the action part
      unit();
      
      EXPECT_TRUE( unit( local::message::A{}) == task::unit::Dispatch::pending);
      EXPECT_TRUE( unit( local::message::B{}) == task::unit::Dispatch::pending);
      
      EXPECT_TRUE( descriptions.size() == 3) << CASUAL_NAMED_VALUE( descriptions);
   }


   TEST( common_task_unit, dispatch__not_done)
   {
      common::unittest::Trace trace;

      auto invokes = 0;

      // use a 'big' message to verify that the message dispatch works 
      const auto origin = common::unittest::Message{ 1024};

      auto unit = task::create::unit(
         local::action(),
         [ &invokes]( task::unit::id id, const local::message::A&){ ++invokes; return task::unit::Dispatch::pending;},
         [ &invokes, &origin]( task::unit::id id, const common::unittest::Message& message)
         {
            EXPECT_TRUE( message == origin); 
            ++invokes; 
            return task::unit::Dispatch::pending;
         }
      );
      
      EXPECT_TRUE( unit( local::message::A{}) == task::unit::Dispatch::pending);
      EXPECT_TRUE( unit( origin) == task::unit::Dispatch::pending);

      EXPECT_TRUE( invokes == 2);
   }

   TEST( common_task_unit, dispatch__done)
   {
      common::unittest::Trace trace;

      auto unit = task::create::unit(
         local::action(),
         []( task::unit::id id, const local::message::A&){ return task::unit::Dispatch::pending;},
         []( task::unit::id id, const local::message::B&){ return task::unit::Dispatch::done;}
      );
      
      EXPECT_TRUE( unit( local::message::A{}) == task::unit::Dispatch::pending);
      EXPECT_TRUE( unit( local::message::B{}) == task::unit::Dispatch::done);
   }

      TEST( common_task_group, dispatch__done)
   {
      common::unittest::Trace trace;

      task::unit::Dispatch dispatch = task::unit::Dispatch::pending;

      auto group = task::create::group( task::create::unit(
         local::action(),
         [ &dispatch]( task::unit::id id, const local::message::A&){ return dispatch;},
         [ &dispatch]( task::unit::id id, const local::message::B&){ return dispatch;}
      ));
      
      EXPECT_TRUE( group( local::message::A{}) == task::unit::Dispatch::pending);
      EXPECT_TRUE( group( local::message::B{}) == task::unit::Dispatch::pending);

      dispatch = task::unit::Dispatch::done;
      EXPECT_TRUE( group( local::message::B{}) == task::unit::Dispatch::done);
   }

   TEST( common_task_group, cancel)
   {
      common::unittest::Trace trace;

      auto action = 0;

      auto tasks = task::Coordinator{};

      tasks.then( task::create::unit(
         [ &action]( task::unit::id){ ++action; return task::unit::action::Outcome::success;},
         []( task::unit::id id, const local::message::A&){ return task::unit::Dispatch::pending;},
         []( task::unit::id id, const local::message::B&){ return task::unit::Dispatch::done;}
      )).then( task::create::unit(
         [ &action]( task::unit::id){ ++action; return task::unit::action::Outcome::success;},
         []( task::unit::id id, const local::message::C&){ return task::unit::Dispatch::pending;},
         []( task::unit::id id, const local::message::D&){ return task::unit::Dispatch::done;}
      ));

      EXPECT_TRUE( tasks.continuations().size() == 2);
      
      // cancel the non started.
      tasks.cancel();

      EXPECT_TRUE( tasks.continuations().size() == 1);

      // dispatch the two messages -> will trigger done 
      // for the first group, and the second is canceled -> done
      tasks( local::message::A{});
      tasks( local::message::B{});
      EXPECT_TRUE( action == 1);

      EXPECT_TRUE( tasks.empty());

   }

   TEST( common_task_group, then_dispatch__done)
   {
      common::unittest::Trace trace;

      auto action = 0;

      auto tasks = task::Coordinator{};

      tasks.then( task::create::unit(
         [ &action]( task::unit::id){ ++action; return task::unit::action::Outcome::success;},
         []( task::unit::id id, const local::message::A&){ return task::unit::Dispatch::pending;},
         []( task::unit::id id, const local::message::B&){ return task::unit::Dispatch::done;}
      )).then( task::create::unit(
         [ &action]( task::unit::id){ ++action; return task::unit::action::Outcome::success;},
         []( task::unit::id id, const local::message::C&){ return task::unit::Dispatch::pending;},
         []( task::unit::id id, const local::message::D&){ return task::unit::Dispatch::done;}
      ));
      
      // Will invoke action, and "start" the first group.
      EXPECT_TRUE( action == 1) << "action: " << action << "\n" << CASUAL_NAMED_VALUE( tasks);

      // dispatch the two messages -> will trigger done 
      // for the first group, and start the next group.
      tasks( local::message::A{});
      tasks( local::message::B{});
      EXPECT_TRUE( action == 2);
   }

   TEST( common_task_group, then_dispatch__action_fail___expect_done)
   {
      common::unittest::Trace trace;

      auto invocations = std::vector< task::unit::id>{};

      auto tasks = task::Coordinator{};

      // will invoke and discard the first group, due to Outcome::abort
      tasks.then( task::create::unit(
         local::action( task::unit::action::Outcome::abort),
         [ &invocations]( task::unit::id id, const local::message::A&){ invocations.push_back( id); return task::unit::Dispatch::pending;},
         [ &invocations]( task::unit::id id, const local::message::B&){ invocations.push_back( id); return task::unit::Dispatch::done;}
      )).then( task::create::unit(
         local::action(),
         [ &invocations]( task::unit::id id, const local::message::C&){ invocations.push_back( id); return task::unit::Dispatch::pending;},
         [ &invocations]( task::unit::id id, const local::message::D&){ invocations.push_back( id); return task::unit::Dispatch::done;}
      )).then( task::create::unit( 
         local::action(),
         [ &invocations]( task::unit::id id, const local::message::E&){ invocations.push_back( id); return task::unit::Dispatch::done;})
      );

      // the first unint is done and gone, we only expect the last two
      const auto ids = tasks.ids();
      ASSERT_TRUE( ids.size() == 2);

      // the following two messages has no task::unit active that handles 
      // these messages (any more), hence no invocations
      tasks( local::message::A{});
      tasks( local::message::B{});
      EXPECT_TRUE( invocations.empty()) << CASUAL_NAMED_VALUE( invocations) << " " << CASUAL_NAMED_VALUE( tasks);

      tasks( local::message::C{});
      EXPECT_TRUE( invocations.size() == 1);

      tasks( local::message::C{});
      tasks( local::message::C{});
      tasks( local::message::D{}); // will trigger done for the second group

      tasks( local::message::E{}); // will trigger done for the third group

      auto expected = array::make( 
         ids[ 0], ids[ 0], ids[ 0], ids[ 0],
         ids[ 1]);

      EXPECT_TRUE( algorithm::equal( invocations, expected)) << CASUAL_NAMED_VALUE( invocations);

      EXPECT_TRUE( tasks.empty());
   }   

   
} // casual