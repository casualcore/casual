//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi.h"

#include "common/algorithm.h"
#include "common/process.h"
#include "common/log/category.h"
#include "common/exception/guard.h"
#include "common/argument.h"
#include "common/environment/string.h"
#include "common/chronology.h"
#include "common/domain.h"


#include <locale>

namespace casual
{
   namespace example::server
   {

      namespace local
      {
         namespace
         {
            struct
            {
               casual::platform::time::unit startup{};
               casual::platform::time::unit sleep{};
               casual::platform::time::unit work{};
               std::string forward;
            } global;
         } // <unnamed>
      } // local

      extern "C"
      {
         void casual_example_echo( TPSVCINFO* info);
         
         int tpsvrinit( int argc, char* argv[])
         {
            return common::exception::main::log::guard( [&]()
            {
               auto arguments = common::range::make( argv + 1, argc - 1);
               common::log::line( common::log::category::information, "example server started with arguments: ", arguments);

               auto time_value = []( auto& time)
               {
                  return [&time]( std::string value)
                  {
                     time = common::chronology::from::string( common::environment::string( value));
                  };
               };

               common::argument::Parse{ "Shows a few ways services can be develop",
                  common::argument::Option{ time_value( local::global.startup), {"--startup"}, "startup time"},
                  common::argument::Option{ time_value( local::global.sleep), {"--sleep"}, "sleep time"},
                  common::argument::Option{ time_value( local::global.work), {"--work"}, "work time"},
                  common::argument::Option{ std::tie( local::global.forward), {"--forward"}, "service that casual/example/forward should call"},
               }( argc, argv);

               if( local::global.startup != platform::time::unit::zero())
                  common::process::sleep( local::global.startup);

               auto advertise_echo = []( std::string_view name)
               {
                  tpadvertise( name.data(), &casual_example_echo);
               };

               advertise_echo( "casual/example/advertised/echo");
               advertise_echo( common::string::compose( "casual/example/domain/echo/", common::domain::identity().name));
               advertise_echo( common::string::compose( "casual/example/domain/echo/", common::domain::identity().id));


               common::log::line( common::log::category::information, "sleep: ", local::global.sleep);
               common::log::line( common::log::category::information, "work: ", local::global.work);
            });
         }

        void casual_example_forward( TPSVCINFO* info)
         {
            auto len = info->len;
            if( tpcall( local::global.forward.data(), info->data, info->len, &info->data, &len, 0) == 0)
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);

            tpreturn( TPFAIL, 0, nullptr, 0, 0);
         }
         
         void casual_example_sleep( TPSVCINFO* info)
         {
            common::process::sleep( local::global.sleep);
            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         }

         void casual_example_work( TPSVCINFO* info)
         {
            auto deadline = platform::time::clock::type::now() + local::global.work;

            while( deadline < platform::time::clock::type::now())
            {
               ; // no-op
            }

            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         }
      }

   } // example::server
} // casual

