//!
//! mockup.h
//!
//! @attention Only for unittest purposes
//!
//! Created on: Jul 16, 2013
//!     Author: Lazan
//!

#ifndef MOCKUP_H_
#define MOCKUP_H_

#include "common/queue.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace queue
         {
            template< typename M>
            struct WriteMessage
            {
               typedef common::platform::queue_key_type key_type;
               typedef M message_type;

               //! so it can be used with ipc_wrapper
               typedef key_type ipc_type;

               WriteMessage( key_type key)
               {
                  reset();
                  queue_key = key;

               }

               template< typename T>
               bool operator () ( T& value)
               {
                  //
                  // Value as a lvalue, and we can't just move it.
                  // So, why not write and read to a real queue, hence also test the queue stuff...
                  //

                  common::queue::ipc_wrapper< common::queue::blocking::Reader> reader;

                  common::queue::ipc_wrapper< common::queue::blocking::Writer> writer( reader.ipc().id());
                  writer( value);

                  T result;
                  reader( result);
                  replies.push_back( std::move( result));

                  return true;
               }

               static void reset()
               {
                  replies.clear();
                  queue_key = 0;
               }

               static key_type queue_key;
               static std::vector< message_type> replies;
            };

            template< typename M>
            typename WriteMessage< M>::key_type WriteMessage< M>::queue_key = 0;

            template< typename M>
            std::vector< M> WriteMessage< M>::replies = std::vector< message_type>{};




            template< typename M>
            struct ReadMessage
            {
               typedef common::platform::queue_key_type key_type;
               typedef M message_type;

               //! so it can be used with ipc_wrapper
               typedef key_type ipc_type;

               ReadMessage( key_type key)
               {
                  queue_key = key;
               }

               template< typename T>
               void operator () ( T& value)
               {
                  if( ! replies.empty())
                  {
                     value = std::move( replies.front());
                     replies.pop_front();
                  }
               }

               static void reset()
               {
                  replies.clear();
                  queue_key = 0;
               }

               static key_type queue_key;
               static std::deque< message_type> replies;
            };

            template< typename M>
            typename ReadMessage< M>::key_type ReadMessage< M>::queue_key = 0;

            template< typename M>
            std::deque< M> ReadMessage< M>::replies = std::deque< message_type>{};

         } // queue

         namespace xa_switch
         {
            struct State
            {


            };

         }


      } // mockup
   } // common
} // casual

extern "C"
{
   extern struct xa_switch_t casual_mockup_xa_switch_static;
}


#endif /* MOCKUP_H_ */
