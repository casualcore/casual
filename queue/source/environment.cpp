//!
//! environment.h
//!
//! Created on: Jul 9, 2014
//!     Author: Lazan
//!

#include "queue/environment.h"

#include "common/environment.h"
#include "common/internal/log.h"
#include "common/exception.h"




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
                        std::ifstream file( path());

                        if( ! file)
                        {
                           common::log::internal::ipc << "failed to open queue-broker queue-key-file" << std::endl;
                           throw common::exception::xatmi::SystemError( "failed to open queue-broker queue-key-file: " + path());
                        }

                        common::platform::queue_id_type id{ 0};
                        file >> id;

                        return id;
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
