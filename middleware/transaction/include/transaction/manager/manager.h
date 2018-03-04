//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef TRANSACTION_MANAGER_MANAGER_H_
#define TRANSACTION_MANAGER_MANAGER_H_

#include "transaction/manager/state.h"



//
// std
//
#include <string>

namespace casual
{


   namespace transaction
   {
      namespace environment
      {
         namespace log
         {
            std::string file();
         } // log

      } // environment


      struct Settings
      {
         Settings();

         std::string log;
      };


      class Manager
      {
      public:

         Manager( Settings settings);
         ~Manager();

         void start();

         const State& state() const;

      private:

         void handle_pending();

         State m_state;
      };


   } // transaction
} // casual



#endif /* MONITOR_H_ */
