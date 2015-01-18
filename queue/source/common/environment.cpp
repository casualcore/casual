//!
//! environment.h
//!
//! Created on: Jul 9, 2014
//!     Author: Lazan
//!

#include "queue/common/environment.h"

#include "common/environment.h"
#include "common/internal/log.h"
#include "common/exception.h"
#include "common/process.h"




#include <fstream>

namespace casual
{
   namespace queue
   {
      namespace environment
      {
         namespace broker
         {

            namespace queue
            {
               namespace local
               {
                  namespace
                  {
                     common::platform::queue_id_type initializeBrokerQueueId()
                     {
                        std::ifstream file;

                        //
                        // Not sure if the queue-broker is up and running, we need try a few times
                        // TODO: Should we implement some functionality in the real broker that can act like a
                        // dispatch for queries like these? Not sure what we will use as a key in that case though...
                        //

                        auto sleep = std::chrono::milliseconds{ 2};

                        while( sleep < std::chrono::seconds{ 10})
                        {
                           file.open( path());

                           if( file)
                           {
                              common::platform::queue_id_type id{ 0};
                              file >> id;

                              return id;
                           }
                           common::process::sleep( sleep);
                           sleep *= 2;
                        }


                        common::log::internal::queue << "failed to open queue-broker queue-key-file" << std::endl;
                        throw common::exception::xatmi::SystemError( "failed to open queue-broker queue-key-file: " + path());
                     }
                  }
               }
               std::string path()
               {
                  return casual::common::environment::domain::singleton::path() + "/.casual-queue-broker-queue";
               }


               common::platform::queue_id_type id()
               {
                  static const auto id = local::initializeBrokerQueueId();
                  return id;
               }


            } // queue

         } // broker

      } // environment
   } // queue
} // casual
