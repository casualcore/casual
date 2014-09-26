#ifndef CASUALBROKERADMINSERVERIMPLEMENTATION_H
#define CASUALBROKERADMINSERVERIMPLEMENTATION_H



#include <vector>
#include <string>


#include "broker/admin/brokervo.h"

#include "common/server_context.h"

namespace casual
{

   namespace common
   {
      namespace server
      {

      } // server
   } // common

namespace broker
{
   class Broker;
   class State;

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
         std::vector< admin::ServerVO> listServers();


         //!
         //! @return a list of all services
         //!
         std::vector< admin::ServiceVO> listServices();

         //!
         //!
         //!
         void updateInstances( const std::vector< admin::update::InstancesVO>& instances);


      private:
         static Broker* m_broker;
      };

   } // admin

} // broker
} // casual

#endif 
