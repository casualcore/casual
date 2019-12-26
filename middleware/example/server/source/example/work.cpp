//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi.h"

#include "common/algorithm.h"
#include "common/process.h"
#include "common/log/category.h"
#include "common/exception/handle.h"
#include "common/argument.h"
#include "common/environment/string.h"
#include "common/chronology.h"


#include <locale>

namespace casual
{

   namespace example
   {
      namespace server
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
               } global;
            } // <unnamed>
         } // local

         extern "C"
         {
            void casual_example_echo( TPSVCINFO* info);
            
            int tpsvrinit( int argc, char* argv[])
            {
               return common::exception::guard( [&]()
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
                  }( argc, argv);

                  if( local::global.startup != platform::time::unit::zero())
                     common::process::sleep( local::global.startup);

                  tpadvertise( "casual/example/advertised/echo", &casual_example_echo);

                  common::log::line( common::log::category::information, "sleep: ", local::global.sleep);
                  common::log::line( common::log::category::information, "work: ", local::global.work);
               });
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
      } // server
   } // example
} // casual

