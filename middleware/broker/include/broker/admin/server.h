#ifndef CASUALBROKERADMINSERVERIMPLEMENTATION_H
#define CASUALBROKERADMINSERVERIMPLEMENTATION_H



#include <vector>
#include <string>


#include "broker/admin/brokervo.h"

#include "common/server/argument.h"

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
   struct State;

   namespace admin
   {


      admin::StateVO broker_state( const broker::State& state);

      void update_instances( broker::State& state, const std::vector< admin::update::InstancesVO>& instances);

      admin::ShutdownVO shutdown( broker::State& state, bool broker);



      common::server::Arguments services( broker::State& state);



   } // admin

} // broker
} // casual

#endif 
