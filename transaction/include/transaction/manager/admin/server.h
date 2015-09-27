#ifndef CASUALTRANSACTIONADMIN_SERVER_H
#define CASUALTRANSACTIONADMIN_SERVER_H



#include "common/server/argument.h"


namespace casual
{
   namespace transaction
   {
      class State;

      namespace admin
      {

         common::server::Arguments services( State& state);


      } // admin
   } // transaction
} // casual

#endif 
