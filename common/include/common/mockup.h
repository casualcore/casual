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
#include "common/message.h"


#include <cassert>

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         namespace ipc
         {
            class Queue
            {
            public:
               typedef platform::queue_id_type id_type;
               typedef common::ipc::message::Complete message_type;

               enum
               {
                  cNoBlocking = common::platform::cIPC_NO_WAIT
               };

               Queue( id_type id) : m_id{ id} {};

               /*
               Queue( Queue&&) = default;
               Queue( const Queue&) = delete;
               Queue& operator = ( const Queue&) = delete;
               */

               //!
               //! Writes message to mockup-queue
               //! @{
               bool operator () ( const message_type& message) const
               {
                  return operator() ( message, 0);
               }
               bool operator () ( const message_type& message, const long flags) const;
               //! @}

               //!
               //! Tries to find the first logic complete message
               //!
               //! @return 0..1 occurrences of a logical complete message.
               //!
               //! @attention mockup - use only with unittest
               //!
               std::vector< message_type> operator () ( const long flags);

               //!
               //! Tries to find the first logic complete message with a specific type
               //!
               //! @return 0..1 occurrences of a logical complete message.
               //!
               //! @attention mockup - use only with unittest
               //!
               std::vector< message_type> operator () ( message_type::message_type_type type, const long flags);


               id_type id() const { return m_id;}

            private:

               id_type m_id = 0;
            };

         } // ipc


         namespace queue
         {

            void clearAllQueues();

            typedef platform::message_type_type message_type_type;



            namespace non_blocking
            {

               template< typename P>
               using basic_writer = common::queue::internal::basic_writer< common::queue::policy::NonBlocking, P, ipc::Queue>;

               typedef basic_writer< common::queue::policy::NoAction> Writer;

               template< typename P>
               using basic_reader = common::queue::internal::basic_reader< common::queue::policy::NonBlocking, P, ipc::Queue>;

               typedef basic_reader< common::queue::policy::NoAction> Reader;


            } // non_blocking


            namespace blocking
            {

               template< typename P>
               using basic_writer = common::queue::internal::basic_writer< common::queue::policy::Blocking, P, ipc::Queue>;

               typedef basic_writer< common::queue::policy::NoAction> Writer;

               template< typename P>
               using basic_reader = common::queue::internal::basic_reader< common::queue::policy::Blocking, P, ipc::Queue>;

               typedef basic_reader< common::queue::policy::NoAction> Reader;


            }// blocking
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
