#ifndef CASUALBROKERADMINSERVERIMPLEMENTATION_H
#define CASUALBROKERADMINSERVERIMPLEMENTATION_H



#include <vector>
#include <string>


#include "broker/admin/brokervo.h"

#include "common/server/argument.h"

namespace casual
{


namespace broker
{
   struct State;

   namespace admin
   {
      common::server::Arguments services( broker::State& state);

   } // admin

} // broker
} // casual

#endif 
