//!
//! test_server_context.cpp
//!
//! Created on: Dec 8, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "common/server_context.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            struct queue_policy
            {
               template< typename W>
               struct broker_writer : public W
               {
                  broker_writer() : W( ipc::getBrokerQueue()) {}
               };

               typedef broker_writer< queue::blocking::Writer> blocking_broker_writer;
               typedef broker_writer< queue::non_blocking::Writer> non_blocking_broker_writer;
               typedef queue::basic_queue< ipc::send::Queue, queue::blocking::Writer> blocking_send_writer;

               static ipc::receive::Queue::queue_key_type receiveKey() { return ipc::getReceiveQueue().getKey(); }

            };

         }

      }


      TEST( casual_common_service_context, queue_writer_blocking_reader)
      {

      }

   }
}



