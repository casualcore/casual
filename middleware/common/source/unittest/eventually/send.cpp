//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest/eventually/send.h"

#include "common/unittest/log.h"

#include "common/communication/ipc.h"
#include "common/exception/handle.h"

#include <future>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace eventually
         {
            namespace local
            {
               namespace
               {
                  void send( strong::ipc::id destination, communication::message::Complete&& complete)
                  {
                     try
                     {
                        log::line( log::debug, "unittest::eventually::send::local::send");

                        signal::thread::scope::Block block{};
                        communication::device::blocking::send( destination, complete);
                        
                     }
                     catch( ...)
                     {
                        exception::handle( log::category::error, "unittest::eventually::send::local::send");
                     }
                  }

                  struct Instance 
                  {
                     static Instance& instance()
                     {
                        static Instance singleton;
                        return singleton;
                     }

                     void send( strong::ipc::id destination, communication::message::Complete&& complete)
                     {
                        clean();
                        calls.push_back( std::async( &local::send, destination, std::move( complete)));
                     }

                     void clean() 
                     {
                        algorithm::trim( calls, algorithm::remove_if( calls, []( auto& f)
                        {
                           return f.wait_for( std::chrono::microseconds{ 0}) == std::future_status::ready;
                        }));
                     }

                  private:
                     Instance() = default;

                     std::vector< std::future< void>> calls;
                  };
               } // <unnamed>
            } // local
            Uuid send( strong::ipc::id destination, communication::message::Complete&& complete)
            {
               auto result = complete.correlation;
               local::Instance::instance().send( destination, std::move( complete));
               return result;
            }

            

         } // eventually
      } // unittest
   } // common
} // casual