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
                  casual::common::platform::time::unit sleep;
                  casual::common::platform::time::unit work;
               } global;
            } // <unnamed>
         } // local

         extern "C"
         {
            int tpsvrinit( int argc, char* argv[])
            {
               return common::exception::guard( [&]()
               {
                  auto arguments = common::range::make( argv + 1, argc - 1);
                  common::log::line( common::log::category::information, "example server started with arguments: ", arguments);

                  common::argument::Parse{ "Shows a few ways services can be develop",
                     common::argument::Option( []( const std::string& value){
                        local::global.sleep = common::chronology::from::string( value);
                     }, {"--sleep"}, "sleep time"),
                     common::argument::Option( []( const std::string& value){
                        local::global.work = common::chronology::from::string( value);
                     }, {"--work"}, "work time ")
                  }( argc, argv);


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
               auto deadline = common::platform::time::clock::type::now() + local::global.work;

               while( deadline < common::platform::time::clock::type::now())
               {
                  ; // no-op
               }

               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }
         }
      } // server
   } // example
} // casual

