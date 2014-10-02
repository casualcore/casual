//!
//! server.h
//!
//! Created on: Sep 30, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_SERVER_H_
#define CASUAL_QUEUE_BROKER_ADMIN_SERVER_H_


#include "queue/broker/admin/queuevo.h"

#include "common/server_context.h"

namespace casual
{
   namespace queue
   {
      class Broker;

      namespace broker
      {
         namespace admin
         {

            class Server
            {

            public:

               static common::server::Arguments services( Broker& broker);

               Server( int argc, char **argv);


               //!
               //! @return a list of all servers
               //!
               std::vector< admin::GroupVO> listGroups();



            private:
               static Broker* m_broker;
            };

         } // admin
      } // broker
   } // queue


} // casual

#endif // SERVER_H_
