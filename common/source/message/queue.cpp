//!
//! queue.cpp
//!
//! Created on: May 2, 2015
//!     Author: Lazan
//!

#include "common/message/queue.h"


namespace casual
{
   namespace common
   {

      namespace message
      {
         namespace queue
         {
            namespace dequeue
            {

               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ qid: " << value.queue
                     << ", block: " << std::boolalpha << value.block
                     << ", parent: " << value.process
                     << ", process: " << value.process
                     << ", trid: " << value.trid << '}';
               }


            } // monitor
         } // traffic
      } // message
   } // common
} // casual
