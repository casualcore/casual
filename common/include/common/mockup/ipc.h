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
#include "common/message/type.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace ipc
         {
            using id_type = platform::queue_id_type;

            struct Sender
            {
               typedef platform::queue_id_type queue_id_type;

               Sender();
               ~Sender();

               void add( queue_id_type destination, const common::ipc::message::Complete& message) const;

               template< typename M>
               void add( queue_id_type destination, M& message) const
               {
                  marshal::output::Binary archive;
                  archive << message;

                  auto type = message::type( message);
                  add( destination, common::ipc::message::Complete( type, archive.release()));
               }

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };

            struct Receiver
            {
               Receiver();
               ~Receiver();
               Receiver( Receiver&&);
               Receiver& operator = ( Receiver&&);

               using type_type =  common::ipc::message::Complete::message_type_type;
               using id_type = platform::queue_id_type;

               id_type id() const;

               void clear();

               std::vector< common::ipc::message::Complete> operator () ( const long flags);
               std::vector< common::ipc::message::Complete> operator () ( type_type type, const long flags);
               std::vector< common::ipc::message::Complete> operator () ( const std::vector< type_type>& types, const long flags);

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };

            namespace broker
            {

               Receiver& queue();

               id_type id();

            } // broker

            namespace receive
            {
               struct Sender
               {
                  bool operator () ( const common::ipc::message::Complete& message) const;

                  bool operator () ( const common::ipc::message::Complete& message, const long flags) const;

               private:
                  mockup::ipc::Sender m_sender;
               };

               Sender& queue();

               id_type id();
            }

         } // ipc


         template< platform::pid_type PID>
         struct Instance
         {
            platform::pid_type pid()
            {
               return PID;
            }

            ipc::Receiver& queue()
            {
               static ipc::Receiver singleton;
               return singleton;
            }

         };

      } // mockup
   } // common


} // casual

#endif // IPC_H_
