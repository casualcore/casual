//!
//! monitor.h
//!
//! Created on: Jul 13, 2013
//!     Author: Lazan
//!

#ifndef MONITOR_H_
#define MONITOR_H_


#include "common/ipc.h"

#include "database/database.hpp"


//
// std
//
#include <string>

namespace casual
{
   namespace transaction
   {
      class Manager
      {
      public:
         Manager( const std::vector< std::string>& arguments);

         void start();

      private:
         common::ipc::receive::Queue& m_receiveQueue;
         database m_database;

         static std::string databaseFileName();

      };

   } // transaction
} // casual



#endif /* MONITOR_H_ */
