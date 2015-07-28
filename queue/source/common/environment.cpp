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

#include "common/queue.h"
#include "common/message/type.h"




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
                     common::platform::queue_id_type initialize_broker_queue_id()
                     {

                        common::message::lookup::process::Request request;
                        request.directive = common::message::lookup::process::Request::Directive::wait;
                        request.identification = broker::identification();
                        request.process = common::process::handle();

                        auto reply = common::queue::blocking::call( common::ipc::broker::id(), request);

                        return reply.process.queue;
                     }
                  }
               }


               common::platform::queue_id_type id()
               {
                  static const auto id = local::initialize_broker_queue_id();
                  return id;
               }


            } // queue

            const common::Uuid& identification()
            {
               static const common::Uuid id{ "c0c5a19dfc27465299494ad7a5c229cd"};
               return id;
            }
         } // broker

      } // environment
   } // queue
} // casual
