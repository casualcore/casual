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

#include "common/communication/ipc.h"
#include "common/message/type.h"




#include <fstream>

namespace casual
{
   namespace queue
   {
      namespace environment
      {

         namespace ipc
         {
            namespace broker
            {
               common::communication::ipc::outbound::instance::Device& device()
               {
                  static common::communication::ipc::outbound::instance::Device singleton{
                     identity::broker(), common::environment::variable::name::ipc::queue::broker()};

                  return singleton;
               }

            } // broker

         } // ipc

         namespace identity
         {
            const common::Uuid& broker()
            {
               static const common::Uuid id{ "c0c5a19dfc27465299494ad7a5c229cd"};
               return id;
            }

         } // identity


      } // environment
   } // queue
} // casual
