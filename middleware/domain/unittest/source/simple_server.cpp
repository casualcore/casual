//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/handle.h"
#include "common/exception/handle.h"

namespace casual
{
   namespace local
   {
      namespace
      {
         void main(int argc, char **argv)
         {
            common::communication::instance::connect();

            auto handler = common::communication::ipc::inbound::device().handler(
                  common::message::handle::Ping{},
                  common::message::handle::Shutdown{}
            );

            common::message::dispatch::blocking::pump( handler, common::communication::ipc::inbound::device());
         }

      } // <unnamed>
   } // local

} // casual

int main(int argc, char **argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::local::main( argc, argv);
   });
}

