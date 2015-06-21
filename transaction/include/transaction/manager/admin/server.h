#ifndef CASUALTRANSACTIONADMIN_SERVER_H
#define CASUALTRANSACTIONADMIN_SERVER_H


#include "transaction/manager/admin/transactionvo.h"

#include "common/server/argument.h"

#include <vector>

namespace casual
{
   namespace transaction
   {
      class Manager;
      namespace admin
      {

         class Server
         {

         public:

            static common::server::Arguments services( Manager& manager);

            Server( int argc, char **argv);

            ~Server();

            //!
            //! @return total state
            //!
            vo::State state();

         private:
            static Manager* m_manager;

         };

      } // admin
   } // transaction
} // casual

#endif 
