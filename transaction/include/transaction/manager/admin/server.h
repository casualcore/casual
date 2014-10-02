#ifndef CASUALTRANSACTIONADMIN_SERVER_H
#define CASUALTRANSACTIONADMIN_SERVER_H


#include "transaction/manager/admin/vo/transaction.h"

#include "common/server_context.h"

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
            //! List all current transactions
            //!
            //! @return list of transacions
            //!
            std::vector< vo::Transaction> listTransactions();

         private:
            static Manager* m_manager;

         };

      } // admin
   } // transaction
} // casual

#endif 
