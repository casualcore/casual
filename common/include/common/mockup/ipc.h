//!
//! ipc.h
//!
//! Created on: May 30, 2014
//!     Author: Lazan
//!

#ifndef IPC_H_
#define IPC_H_

#include "common/ipc.h"
#include "common/move.h"
#include "common/platform.h"
#include "common/marshal.h"
#include "common/message.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace ipc
         {

            struct Sender
            {
               typedef platform::queue_id_type queue_id_type;

               Sender();
               ~Sender();

               void add( queue_id_type destination, common::ipc::message::Complete&& message) const;

               template< typename M>
               void add( queue_id_type destination, M&& message) const
               {
                  marshal::output::Binary archive;
                  archive << message;

                  auto type = message::type( message);
                  add( destination, common::ipc::message::Complete( type, archive.release()));
               }

               void start();

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };

            struct Receiver
            {
               Receiver();
               ~Receiver();

               using type_type =  common::ipc::message::Complete::message_type_type;
               using id_type = platform::queue_id_type;

               id_type id() const;

               std::vector< common::ipc::message::Complete> operator () ( const long flags);
               std::vector< common::ipc::message::Complete> operator () ( type_type type, const long flags);
               std::vector< common::ipc::message::Complete> operator () ( const std::vector< type_type>& types, const long flags);

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };


         } // ipc
      } // mockup
   } // common


} // casual

#endif // IPC_H_
