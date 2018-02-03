//!
//! casual 
//!

#include "xatmi.h"

#include "common/algorithm.h"
#include "common/process.h"
#include "common/log/category.h"
#include "common/exception/handle.h"
#include "common/arguments.h"
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
                  std::chrono::microseconds sleep;
                  std::chrono::microseconds work;
               } global;
            } // <unnamed>
         } // local

         extern "C"
         {
            int tpsvrinit( int argc, char* argv[])
            {
               try
               {
                  auto arguments = common::range::make( argv + 1, argc - 1);
                  common::log::line( common::log::category::information, "example server started with arguments: ", arguments);

                  common::Arguments parser{ {
                     common::argument::directive( {"--sleep"}, "sleep time", []( const std::string& value){
                        local::global.sleep = common::chronology::from::string( value);
                     }),
                     common::argument::directive( {"--work"}, "work time (us)", []( const std::string& value){
                        local::global.work = common::chronology::from::string( value);
                     })
                  }};

                  parser.parse( argc, argv);


                  common::log::line( common::log::category::information, "sleep: ", local::global.sleep);
                  common::log::line( common::log::category::information, "work: ", local::global.work);

               }
               catch( ...)
               {
                  return common::exception::handle();
               }
               return 0;
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

