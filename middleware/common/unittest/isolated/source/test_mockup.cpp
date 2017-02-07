//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"


#include "common/message/service.h"
#include "common/message/domain.h"
#include "common/service/lookup.h"

#include "common/communication/ipc.h"
#include "common/log.h"
#include "common/trace.h"
#include "common/internal/log.h"

#include "common/environment.h"


namespace casual
{
   namespace common
   {

      TEST( casual_common_mockup, ipc_Collector_startup)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            mockup::ipc::Collector instance;
         });
      }


      TEST( casual_common_mockup, ipc_Instance_one_message)
      {
         common::unittest::Trace trace;

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Collector instance;

         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            communication::ipc::blocking::send( instance.input(), request);
         }

         {
            message::service::lookup::Request request;
            communication::ipc::blocking::receive( instance.output(), request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());

         }
      }


      TEST( casual_common_mockup, ipc_link_2_Collector__send_one_message)
      {
         common::unittest::Trace trace;

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Collector source;
         mockup::ipc::Collector destination;

         //
         // Link "output" of source to "input" of destination
         //
         mockup::ipc::Link link{ source.output().connector().id(), destination.input()};

         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            communication::ipc::blocking::send( source.input(), request);
         }

         {
            message::service::lookup::Request request;
            communication::ipc::blocking::receive( destination.output(), request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());

         }
      }


      TEST( casual_common_mockup, ipc_Collector_200_messages)
      {
         common::unittest::Trace trace;

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Collector instance;


         {
            trace::Scope trace( "sender.add  200");
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               request.requested = temp + std::to_string( count);
               communication::ipc::blocking::send( instance.input(), request);
            }
         }

         {
            trace::Scope trace( "read( ipc::receive::queue())  200");

            message::service::lookup::Request request;

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               const auto service = temp + std::to_string( count);

               communication::ipc::blocking::receive( instance.output(), request);

               EXPECT_TRUE( request.requested == service) << "want: " << request.requested << " have: " << service;
               EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());
            }
         }
      }

      TEST( casual_common_mockup, ipc_link_2_Collector__send_200_messages)
      {
         common::unittest::Trace trace;

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Collector source;
         mockup::ipc::Collector destination;

         //
         // Link "output" of source to "input" of destination
         //
         mockup::ipc::Link link{ source.output().connector().id(), destination.input()};


         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               request.requested = temp + std::to_string( count);
               communication::ipc::blocking::send( source.input(), request);
            }
         }

         {
            trace::Scope trace( "read( ipc::receive::queue())  200");

            message::service::lookup::Request request;

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               const auto service = temp + std::to_string( count);

               communication::ipc::blocking::receive( destination.output(), request);

               EXPECT_TRUE( request.requested == service) << "want: " << request.requested << " have: " << service;
               EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());
            }
         }
      }

      TEST( casual_common_mockup, domain_manager__instanciate)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            mockup::domain::Manager manager;
         });
      }

      TEST( casual_common_mockup, domain_manager__process_connect__expect_ok)
      {
         common::unittest::Trace trace;

         mockup::domain::Manager manager;

         message::domain::process::connect::Request request;
         request.process = process::handle();

         auto reply = communication::ipc::call( communication::ipc::domain::manager::device(), request);

         EXPECT_TRUE( reply.directive == decltype( reply)::Directive::start);

      }

      TEST( casual_common_mockup, domain_manager__process_connect__process_lookup___expect_found)
      {
         common::unittest::Trace trace;

         mockup::domain::Manager manager;

         {
            message::domain::process::connect::Request request;
            request.process = process::handle();

            auto reply = communication::ipc::call( communication::ipc::domain::manager::device(), request);

            EXPECT_TRUE( reply.directive == decltype( reply)::Directive::start);
         }

         {
            message::domain::process::lookup::Request request;
            request.process = process::handle();
            request.pid = process::id();

            auto reply = communication::ipc::call( communication::ipc::domain::manager::device(), request);

            EXPECT_TRUE( reply.process == process::handle());
         }
      }


      TEST( casual_common_mockup, domain_manager__process_connect__singleton___process_lookup___expect_found)
      {
         common::unittest::Trace trace;

         mockup::domain::Manager manager;

         auto identification = uuid::make();

         {
            message::domain::process::connect::Request request;
            request.process = process::handle();
            request.identification = identification;

            auto reply = communication::ipc::call( communication::ipc::domain::manager::device(), request);

            EXPECT_TRUE( reply.directive == decltype( reply)::Directive::start);
         }

         {
            message::domain::process::lookup::Request request;
            request.process = process::handle();
            request.identification = identification;

            auto reply = communication::ipc::call( communication::ipc::domain::manager::device(), request);

            EXPECT_TRUE( reply.process == process::handle());
         }
      }

      TEST( casual_common_mockup, domain_manager__process_lookup_singleton___process_connect____expect_found)
      {
         common::unittest::Trace trace;

         mockup::domain::Manager manager;

         auto identification = uuid::make();

         auto lookup = [&]()
         {
            message::domain::process::lookup::Request request;
            request.process = process::handle();
            request.directive = message::domain::process::lookup::Request::Directive::wait;
            request.identification = identification;

            return communication::ipc::blocking::send( communication::ipc::domain::manager::device(), request);
         };

         auto correlation = lookup();

         {
            message::domain::process::connect::Request request;
            request.process = process::handle();
            request.identification = identification;

            auto reply = communication::ipc::call( communication::ipc::domain::manager::device(), request);

            EXPECT_TRUE( reply.directive == decltype( reply)::Directive::start);
         }

         {
            message::domain::process::lookup::Reply reply;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply, correlation);

            EXPECT_TRUE( reply.process == process::handle()) << "reply.process: " << reply.process;
         }
      }


      TEST( casual_common_mockup, minimal_domain__service1_lookup__expect_found)
      {
         common::unittest::Trace trace;

         mockup::domain::minimal::Domain domain;

         auto reply = service::Lookup{ "service1"}();
         EXPECT_TRUE( reply.state == decltype( reply)::State::idle);
         EXPECT_TRUE( reply.service.name == "service1") << "reply.service.name: " << reply.service.name;
         EXPECT_TRUE( reply.process == domain.server.process()) << "reply.process: " << reply.process;
      }





   } // common
} // casual
