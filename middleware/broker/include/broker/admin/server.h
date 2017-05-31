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
         namespace service
         {
            namespace name
            {
               constexpr auto state() { return ".casual.broker.state";}

               namespace metric
               {
                  constexpr auto reset() { return ".casual/broker/metric/reset";}
               } // metric
            } // name
         } // service

         common::server::Arguments services( broker::State& state);

      } // admin

   } // broker
} // casual

#endif 
