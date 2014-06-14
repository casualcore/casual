//!
//! type.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "common/message/type.h"

#include "common/process.h"
#include "common/ipc.h"


namespace casual
{

   namespace common
   {
      namespace message
      {
         namespace server
         {
            Id::Id() : queue_id{ 0}, pid{ process::id()}
            {

            }

            Id::Id( queue_id_type id, pid_type pid) : queue_id( id), pid( pid)
            {}

            Id Id::current()
            {
               return Id{ ipc::receive::id(), process::id()};
            }


         } // server

      } // message
   } // common
} // casual
