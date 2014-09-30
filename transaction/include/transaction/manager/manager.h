//!
//! monitor.h
//!
//! Created on: Jul 13, 2013
//!     Author: Lazan
//!

#ifndef MONITOR_H_
#define MONITOR_H_

#include "transaction/manager/state.h"


#include "common/ipc.h"


//
// std
//
#include <string>

namespace casual
{
   namespace transaction
   {
      struct Settings
      {
         std::string database;
      };





      class Manager
      {
      public:


         Manager( const Settings& settings);

         ~Manager();

         void start();

      private:


         void handlePending();


         common::file::scoped::Path m_queueFilePath;
         common::ipc::receive::Queue& m_receiveQueue;
         State m_state;


      };



   } // transaction
} // casual



#endif /* MONITOR_H_ */
